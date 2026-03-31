#include "ServerDimension.hpp"
#include "../world/ServerWorld.hpp"  // 需要完整定义以使用 unique_ptr
#include "common/world/lighting/manager/WorldLightManager.hpp"
#include "common/core/Result.hpp"
#include "common/util/assert/AssertAll.hpp"

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
{
}

ServerDimension::~ServerDimension() {
    shutdown();
}

// ============================================================================
// 初始化
// ============================================================================

Result<void> ServerDimension::initialize() {
    if (m_initialized) {
        return {};
    }

    // TODO: 创建 ServerWorld
    // 目前 ServerWorld 需要完整的配置，这里先占位
    // 实际创建将在 ServerDimensionManager 中完成

    m_initialized = true;
    return {};
}

void ServerDimension::shutdown() {
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

void ServerDimension::tick() {
    Dimension::tick();

    // 更新区块管理器
    if (m_chunkManager) {
        m_chunkManager->tick();
    }

    // 更新光照管理器
    if (m_lightManager) {
        // TODO: 光照更新
    }
}

// ============================================================================
// 玩家追踪
// ============================================================================

void ServerDimension::addPlayer(PlayerId playerId) {
    if (!hasPlayer(playerId)) {
        m_players.push_back(playerId);
    }
}

void ServerDimension::removePlayer(PlayerId playerId) {
    auto it = std::find(m_players.begin(), m_players.end(), playerId);
    if (it != m_players.end()) {
        m_players.erase(it);
    }
}

bool ServerDimension::hasPlayer(PlayerId playerId) const {
    return std::find(m_players.begin(), m_players.end(), playerId) != m_players.end();
}

// ============================================================================
// 传送门追踪
// ============================================================================

void ServerDimension::recordPortalPosition(const BlockPos& pos) {
    m_portalPositions.insert(hashBlockPos(pos));
}

void ServerDimension::forgetPortalPosition(const BlockPos& pos) {
    m_portalPositions.erase(hashBlockPos(pos));
}

bool ServerDimension::hasPortalAt(const BlockPos& pos) const {
    return m_portalPositions.find(hashBlockPos(pos)) != m_portalPositions.end();
}

Optional<BlockPos> ServerDimension::findNearestPortal(const BlockPos& pos, i32 radius) const {
    BlockPos nearestPos;
    i64 nearestDistSq = std::numeric_limits<i64>::max();
    bool found = false;

    // 遍历范围内的所有可能的传送门位置
    for (i32 dx = -radius; dx <= radius; ++dx) {
        for (i32 dz = -radius; dz <= radius; ++dz) {
            for (i32 dy = -radius; dy <= radius; ++dy) {
                BlockPos checkPos(pos.x + dx, pos.y + dy, pos.z + dz);
                if (hasPortalAt(checkPos)) {
                    i64 distSq = static_cast<i64>(dx) * dx +
                                 static_cast<i64>(dy) * dy +
                                 static_cast<i64>(dz) * dz;
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

u64 ServerDimension::hashBlockPos(const BlockPos& pos) {
    // 使用简单的哈希组合
    u64 hx = static_cast<u64>(static_cast<i64>(pos.x) & 0xFFFFFFFFLL);
    u64 hy = static_cast<u64>(static_cast<i64>(pos.y) & 0xFFFFLL);
    u64 hz = static_cast<u64>(static_cast<i64>(pos.z) & 0xFFFFFFFFLL);
    return (hx << 32) | (hy << 16) | hz;
}

} // namespace mc
