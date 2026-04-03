#include "ChunkDistanceGraph.hpp"
#include "ChunkLoadTicket.hpp"
#include <cmath>
#include <algorithm>

namespace mc::world {

namespace {
inline i32 clampLevel(i32 level) {
    return std::clamp(level, 0, ChunkDistanceGraph::MAX_LEVEL);
}
} // namespace

// ============================================================================
// ChunkDistanceGraph 实现
// ============================================================================

void ChunkDistanceGraph::updateSourceLevel(ChunkCoord x, ChunkCoord z, i32 level, bool isDecreasing) {
    const u64 key = posToKey(x, z);
    const i32 clampedLevel = clampLevel(level);

    auto sourceIt = m_sourceLevels.find(key);
    const i32 oldSourceLevel = (sourceIt != m_sourceLevels.end()) ? sourceIt->second : MAX_LEVEL;
    i32 newSourceLevel = oldSourceLevel;

    if (isDecreasing) {
        // 仅允许降低（更重要）
        newSourceLevel = std::min(oldSourceLevel, clampedLevel);
    } else {
        // 允许升高/移除
        newSourceLevel = clampedLevel;
    }

    if (newSourceLevel >= MAX_LEVEL) {
        if (sourceIt != m_sourceLevels.end()) {
            m_sourceLevels.erase(sourceIt);
            enqueueUpdate(x, z);
        }
        return;
    }

    if (sourceIt == m_sourceLevels.end() || sourceIt->second != newSourceLevel) {
        m_sourceLevels[key] = newSourceLevel;
        enqueueUpdate(x, z);
    }
}

i32 ChunkDistanceGraph::processUpdates(i32 maxToProcess) {
    i32 processed = 0;

    while (!m_updateQueue.empty() && processed < maxToProcess) {
        const u64 key = m_updateQueue.front();
        m_updateQueue.pop();
        m_pendingKeys.erase(key);
        ++processed;

        ChunkCoord x = 0;
        ChunkCoord z = 0;
        keyToPos(key, x, z);
        const i32 currentLevel = getLevel(x, z);

        // 重新计算该区块的最优级别：
        // min(自身源级别, 四邻居级别 + 1)
        i32 recomputedLevel = getSourceLevel(x, z);
        struct Neighbor { ChunkCoord x, z; };
        const Neighbor neighbors[4] = {
            {x, z - 1},  // 北
            {x, z + 1},  // 南
            {x + 1, z},  // 东
            {x - 1, z}   // 西
        };

        for (const auto& neighbor : neighbors) {
            const i32 neighborLevel = getLevel(neighbor.x, neighbor.z);
            const i32 propagatedLevel = (neighborLevel >= MAX_LEVEL) ? MAX_LEVEL : (neighborLevel + 1);
            if (propagatedLevel < recomputedLevel) {
                recomputedLevel = propagatedLevel;
            }
        }

        if (recomputedLevel > MAX_LEVEL) {
            recomputedLevel = MAX_LEVEL;
        }

        if (recomputedLevel == currentLevel) {
            continue;
        }

        if (recomputedLevel >= MAX_LEVEL) {
            m_levels.erase(key);
        } else {
            m_levels[key] = recomputedLevel;
        }

        onLevelChanged(x, z, currentLevel, recomputedLevel);

        // 当前节点级别变化后，邻居的最优值可能也会变化。
        propagateToNeighbors(x, z, recomputedLevel, recomputedLevel < currentLevel);
    }

    return processed;
}

i32 ChunkDistanceGraph::getLevel(ChunkCoord x, ChunkCoord z) const {
    u64 key = posToKey(x, z);
    auto it = m_levels.find(key);
    if (it != m_levels.end()) {
        return it->second;
    }
    return MAX_LEVEL;  // 未加载
}

void ChunkDistanceGraph::clear() {
    m_levels.clear();
    m_sourceLevels.clear();
    m_pendingKeys.clear();
    while (!m_updateQueue.empty()) {
        m_updateQueue.pop();
    }
}

i32 ChunkDistanceGraph::getSourceLevel(ChunkCoord x, ChunkCoord z) const {
    const u64 key = posToKey(x, z);
    auto it = m_sourceLevels.find(key);
    if (it != m_sourceLevels.end()) {
        return it->second;
    }
    return MAX_LEVEL;
}

void ChunkDistanceGraph::onLevelChanged(ChunkCoord x, ChunkCoord z, i32 oldLevel, i32 newLevel) {
    if (m_levelChangeCallback) {
        m_levelChangeCallback(x, z, oldLevel, newLevel);
    }
}

void ChunkDistanceGraph::propagateToNeighbors(ChunkCoord x, ChunkCoord z, i32 level, bool isDecreasing) {
    // 4个相邻区块：北、南、东、西
    struct Neighbor { ChunkCoord x, z; };
    Neighbor neighbors[4] = {
        {x, z - 1},  // 北
        {x, z + 1},  // 南
        {x + 1, z},  // 东
        {x - 1, z}   // 西
    };

    i32 propagatedLevel = computePropagatedLevel(level);
    if (propagatedLevel > MAX_LEVEL) {
        propagatedLevel = MAX_LEVEL;
    }

    (void)propagatedLevel;
    (void)isDecreasing;

    for (i32 i = 0; i < 4; ++i) {
        ChunkCoord nx = neighbors[i].x;
        ChunkCoord nz = neighbors[i].z;

        // 无论升/降级，邻居都可能依赖当前节点，统一触发重计算。
        enqueueUpdate(nx, nz);
    }
}

void ChunkDistanceGraph::enqueueUpdate(ChunkCoord x, ChunkCoord z) {
    const u64 key = posToKey(x, z);
    if (m_pendingKeys.insert(key).second) {
        m_updateQueue.push(key);
    }
}

// ============================================================================
// PlayerChunkTracker 实现
// ============================================================================

PlayerChunkTracker::PlayerChunkTracker(i32 viewDistance)
    : m_viewDistance(viewDistance)
    , m_ticketLevel(viewDistanceToLevel(viewDistance))
{
    // 初始化视距内的区块集合
    updateChunksInRange();
}

void PlayerChunkTracker::setPlayerPosition(ChunkCoord x, ChunkCoord z) {
    if (m_playerX == x && m_playerZ == z && m_positionSet) {
        return;  // 位置没变
    }

    ChunkCoord oldX = m_playerX;
    ChunkCoord oldZ = m_playerZ;

    m_playerX = x;
    m_playerZ = z;
    m_positionSet = true;

    // 更新视距内的区块
    updateChunksInRange();

    // 更新票据级别
    // 玩家离开旧位置（如果之前有设置过位置）
    if (oldX != x || oldZ != z) {
        // 只有当旧位置与当前位置不同时才需要处理
        updateSourceLevel(oldX, oldZ, MAX_LEVEL, false);
    }

    // 玩家进入新位置
    updateSourceLevel(m_playerX, m_playerZ, m_ticketLevel, true);
}

void PlayerChunkTracker::setViewDistance(i32 distance) {
    const i32 clampedDistance = std::clamp(distance, 2, 32);

    if (m_viewDistance == clampedDistance) {
        return;
    }

    m_viewDistance = clampedDistance;
    const i32 oldTicketLevel = m_ticketLevel;
    m_ticketLevel = viewDistanceToLevel(clampedDistance);

    // 更新票据级别：视距增大时级别降低，视距减小时级别升高。
    if (m_positionSet) {
        const bool isDecreasing = m_ticketLevel < oldTicketLevel;
        updateSourceLevel(m_playerX, m_playerZ, m_ticketLevel, isDecreasing);
    }

    // 更新视距内的区块
    updateChunksInRange();
}

bool PlayerChunkTracker::isChunkInRange(ChunkCoord x, ChunkCoord z) const {
    u64 key = posToKey(x, z);
    return m_chunksInRange.count(key) > 0;
}

i32 PlayerChunkTracker::getSourceLevel(ChunkCoord x, ChunkCoord z) const {
    // 如果是玩家当前位置，返回票据级别
    if (x == m_playerX && z == m_playerZ) {
        return m_ticketLevel;
    }
    // 否则返回最大级别（没有票据）
    return MAX_LEVEL;
}

void PlayerChunkTracker::updateChunksInRange() {
    m_chunksInRange.clear();

    // 遍历视距范围内的所有区块
    for (i32 dx = -m_viewDistance; dx <= m_viewDistance; ++dx) {
        for (i32 dz = -m_viewDistance; dz <= m_viewDistance; ++dz) {
            // 使用切比雪夫距离（棋盘距离）
            i32 dist = std::max(std::abs(dx), std::abs(dz));
            if (dist <= m_viewDistance) {
                ChunkCoord cx = m_playerX + dx;
                ChunkCoord cz = m_playerZ + dz;
                u64 key = posToKey(cx, cz);
                m_chunksInRange.insert(key);
            }
        }
    }
}

} // namespace mc::world
