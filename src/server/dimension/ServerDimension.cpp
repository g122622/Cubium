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
#include "../world/ServerWorld.hpp" // 需要完整定义以使用 unique_ptr
#include "common/core/Result.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/world/lighting/manager/WorldLightManager.hpp"
#include <limits>

namespace mc {

// ============================================================================
// 构造函数
// ============================================================================

ServerDimension::ServerDimension(DimensionId id,
    DimensionType type,
    std::unique_ptr<IChunkGenerator> generator,
    std::unique_ptr<BiomeProvider> biomeProvider,
    u64 seed,
    i32 viewDistance)
    : Dimension(id, std::move(type), std::move(generator), std::move(biomeProvider))
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

    // TODO: 创建 ServerWorld
    // 目前 ServerWorld 需要完整的配置，这里先占位
    // 实际创建将在 ServerDimensionManager 中完成

    m_initialized = true;
    return {};
}

void ServerDimension::shutdown()
{
    if (!m_initialized) {
        return;
    }

    m_players.clear();
    m_portalPositions.clear();

    m_lightManager.reset();
    m_chunkManager.reset();
    m_world.reset();

    m_initialized = false;
}

// ============================================================================
// 更新
// ============================================================================

void ServerDimension::tick()
{
    Dimension::tick();

    // 更新区块管理器
    if (m_chunkManager) {
        m_chunkManager->tick();
    }

    // 更新光照管理器
    // 参考 MC 1.16.5: ServerChunkProvider.ChunkExecutor.driveOne() 中调用 lightManager.func_215588_z_()
    // 光照更新使用 Integer.MAX_VALUE 作为最大更新数量，同时更新天空光照和方块光照
    if (m_lightManager) {
        // 检查是否有待处理的光照工作
        if (m_lightManager->hasLightWork()) {
            // 处理所有待处理的光照更新
            // 参数：maxUpdates=最大整数（处理所有）, updateSkyLight=根据维度类型, updateBlockLight=true
            m_lightManager->tick(std::numeric_limits<i32>::max(), type().hasSkyLight(), true);
        }
    }
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
    m_portalPositions.insert(hashBlockPos(pos));
}

void ServerDimension::forgetPortalPosition(const BlockPos& pos)
{
    m_portalPositions.erase(hashBlockPos(pos));
}

bool ServerDimension::hasPortalAt(const BlockPos& pos) const
{
    return m_portalPositions.find(hashBlockPos(pos)) != m_portalPositions.end();
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

u64 ServerDimension::hashBlockPos(const BlockPos& pos)
{
    // 使用简单的哈希组合
    u64 hx = static_cast<u64>(static_cast<i64>(pos.x) & 0xFFFFFFFFLL);
    u64 hy = static_cast<u64>(static_cast<i64>(pos.y) & 0xFFFFLL);
    u64 hz = static_cast<u64>(static_cast<i64>(pos.z) & 0xFFFFFFFFLL);
    return (hx << 32) | (hy << 16) | hz;
}

} // namespace mc
