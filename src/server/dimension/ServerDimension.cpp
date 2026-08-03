/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "ServerDimension.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/profiler/TraceCategories.hpp"
#include "common/profiler/TraceEvents.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/dimension/Dimension.hpp"
#include "common/world/dimension/DimensionManager.hpp"
#include "common/world/dimension/DimensionType.hpp"
#include "common/world/gen/chunk/IChunkGenerator.hpp"
#include "server/sync/BlockUpdateSyncManager.hpp"
#include "server/sync/ChunkSendManager.hpp"
#include "server/world/ServerWorld.hpp"
#include "server/world/spawn/DespawnManager.hpp"
#include "server/world/spawn/NaturalSpawner.hpp"
#include "server/world/spawn/VillageSiege.hpp"
#include <algorithm>
#include <limits>
#include <memory>
#include <optional>
#include <utility>

using namespace mc::trace;

namespace mc {

// ============================================================================
// 构造函数
// ============================================================================

ServerDimension::ServerDimension(
    DimensionId id, DimensionType type, std::unique_ptr<IChunkGenerator> generator, u64 seed, i32 viewDistance)
    : Dimension(id, std::move(type), std::move(generator))
    , m_seed(seed)
    , m_viewDistance(viewDistance)
{}

ServerDimension::~ServerDimension()
{
    shutdown();
}

// ============================================================================
// 初始化
// ============================================================================

Result<void> ServerDimension::initialize()
{
    if (m_initialized) {
        return {};
    }

    MC_ASSERT_RELEASE(m_world != nullptr);

    auto result = m_world->initialize();
    if (result.failed()) {
        return result;
    }

    // 创建同步管理器
    m_chunkSendManager = std::make_unique<server::sync::ChunkSendManager>(
        *m_world->chunkManager(), m_world->chunkManager()->ticketManager());
    m_blockUpdateSyncManager =
        std::make_unique<server::sync::BlockUpdateSyncManager>(m_world->chunkManager()->ticketManager());

    // 设置区块发送管理器指针
    m_world->chunkManager()->setChunkSendManager(m_chunkSendManager.get());

    // 创建生物生成管理器
    m_naturalSpawner = std::make_unique<world::spawn::NaturalSpawner>();
    m_despawnManager = std::make_unique<world::spawn::DespawnManager>();
    m_villageSiege = std::make_unique<server::spawn::VillageSiege>();

    m_initialized = true;
    return {};
}

void ServerDimension::shutdown()
{
    if (!m_initialized) {
        return;
    }

    // per-dimension trace：m_dimensions 是 unordered_map，析构顺序不定，
    // 必须带维度 ID/名称才能在 perfetto 中区分主世界/下界/末地的卸载耗时。
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Initialization,
        "ServerDimension::shutdown",
        "dim",
        static_cast<i32>(id()),
        "dimName",
        type().name());

    // 在世界销毁之前显式清理结构生成缓存（ServerLevel 卸载时立即清理 StructureCheck，
    // 而非等到生成器析构时才清理，避免维度卸载后缓存数据仍驻留内存）。
    if (m_world != nullptr && m_world->chunkManager() != nullptr && m_world->chunkManager()->generator() != nullptr) {
        m_world->chunkManager()->generator()->clearStructureCache();
    }

    // 清理同步管理器（必须在世界之前释放）
    m_blockUpdateSyncManager.reset();
    m_chunkSendManager.reset();

    // 清理生物生成管理器
    m_villageSiege.reset();
    m_despawnManager.reset();
    m_naturalSpawner.reset();

    m_players.clear();
    m_portalPositions.clear();

    // 销毁 ServerWorld，触发 ServerWorld::~ServerWorld() → ServerWorld::shutdown()
    // （含实体落盘 + ServerChunkManager::shutdown，是维度卸载里最重的部分）。
    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Initialization, "ServerDimension::shutdown::DestroyWorld");
        m_world.reset();
    }

    m_initialized = false;
}

// ============================================================================
// 更新
// ============================================================================

void ServerDimension::tick()
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Tick, "ServerDimension::tick");

    Dimension::tick();

    if (m_world != nullptr) {
        m_world->tick();

        // 区块发送处理
        m_chunkSendManager->processPendingSends();

        // 方块更新同步刷新
        m_blockUpdateSyncManager->flushPendingUpdates();

        // 自然刷怪（仅主世界和下界有 hostile 刷怪）
        bool hostile = (id() == DimensionManager::OVERWORLD || id() == DimensionManager::NETHER);
        bool passive = (id() == DimensionManager::OVERWORLD);
        m_naturalSpawner->tick(*m_world, hostile, passive);

        // 生物消失检查
        m_despawnManager->tick(*m_world);

        // 村庄围攻（僵尸围村），仅主世界与下界有 hostile 刷怪时执行。
        // 调试世界不执行。spawnHostiles 硬编码 true：VillageSiege 内部已有夜晚/午夜/村庄判定，
        // 外层 hostile 仅控维度级开关（末地不执行）。与 NaturalSpawner 的维度开关方式对齐。
        if (!m_world->isDebugWorld() && hostile) {
            MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Tick, "ServerDimension::tick::VillageSiege");
            m_villageSiege->tick(*m_world, true);
        }
    }
}

void ServerDimension::setWorld(std::unique_ptr<server::ServerWorld> world)
{
    MC_ASSERT_RELEASE(!m_initialized);
    m_world = std::move(world);
}

server::ServerChunkManager* ServerDimension::chunkManager()
{
    return m_world != nullptr ? m_world->chunkManager() : nullptr;
}

const server::ServerChunkManager* ServerDimension::chunkManager() const
{
    return m_world != nullptr ? m_world->chunkManager() : nullptr;
}

WorldLightManager* ServerDimension::lightManager()
{
    return m_world != nullptr ? m_world->lightManager() : nullptr;
}

const WorldLightManager* ServerDimension::lightManager() const
{
    return m_world != nullptr ? m_world->lightManager() : nullptr;
}

// ============================================================================
// 玩家追踪
// ============================================================================

void ServerDimension::addPlayer(PlayerId playerId)
{
    if (!hasPlayer(playerId)) {
        m_players.push_back(playerId);
    }
}

void ServerDimension::removePlayer(PlayerId playerId)
{
    auto it = std::find(m_players.begin(), m_players.end(), playerId);
    if (it != m_players.end()) {
        m_players.erase(it);
    }
}

bool ServerDimension::hasPlayer(PlayerId playerId) const
{
    return std::find(m_players.begin(), m_players.end(), playerId) != m_players.end();
}

// ============================================================================
// 传送门追踪
// ============================================================================

void ServerDimension::recordPortalPosition(const BlockPos& pos)
{
    m_portalPositions.insert(_hashBlockPos(pos));
}

void ServerDimension::forgetPortalPosition(const BlockPos& pos)
{
    m_portalPositions.erase(_hashBlockPos(pos));
}

bool ServerDimension::hasPortalAt(const BlockPos& pos) const
{
    return m_portalPositions.find(_hashBlockPos(pos)) != m_portalPositions.end();
}

std::optional<BlockPos> ServerDimension::findNearestPortal(const BlockPos& pos, i32 radius) const
{
    BlockPos nearestPos;
    i64 nearestDistSq = std::numeric_limits<i64>::max();
    bool found = false;

    // 遍历范围内的所有可能的传送门位置
    for (i32 dx = -radius; dx <= radius; ++dx) {
        for (i32 dz = -radius; dz <= radius; ++dz) {
            for (i32 dy = -radius; dy <= radius; ++dy) {
                BlockPos checkPos(pos.x + dx, pos.y + dy, pos.z + dz);
                if (hasPortalAt(checkPos)) {
                    i64 distSq = static_cast<i64>(dx) * dx + static_cast<i64>(dy) * dy + static_cast<i64>(dz) * dz;
                    if (distSq < nearestDistSq) {
                        nearestDistSq = distSq;
                        nearestPos = checkPos;
                        found = true;
                    }
                }
            }
        }
    }

    if (found) {
        return nearestPos;
    }
    return std::nullopt;
}

// ============================================================================
// 工具方法
// ============================================================================

u64 ServerDimension::_hashBlockPos(const BlockPos& pos)
{
    // 使用简单的哈希组合
    u64 hx = static_cast<u64>(static_cast<i64>(pos.x) & 0xFFFFFFFFLL);
    u64 hy = static_cast<u64>(static_cast<i64>(pos.y) & 0xFFFFLL);
    u64 hz = static_cast<u64>(static_cast<i64>(pos.z) & 0xFFFFFFFFLL);
    return (hx << 32) | (hy << 16) | hz;
}

} // namespace mc
