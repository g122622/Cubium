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

namespace mc {

// ============================================================================
// 构造函数
// ============================================================================

ServerDimension::ServerDimension(DimensionId id,
    DimensionType type,
    std::unique_ptr<IChunkGenerator> generator,
    u64 seed,
    i32 viewDistance)
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

    m_world.reset();

    m_initialized = false;
}

// ============================================================================
// 更新
// ============================================================================

void ServerDimension::tick()
{
    Dimension::tick();

    if (m_world != nullptr) {
        m_world->tick();
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
