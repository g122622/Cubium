#include "PlayerChunkTracker.hpp"
#include <algorithm>

namespace mc::world {

PlayerChunkTracker::PlayerChunkTracker(i32 viewDistance)
    : m_viewDistance(std::clamp(viewDistance, 2, 32))
{
}

void PlayerChunkTracker::setPlayerPosition(ChunkCoord x, ChunkCoord z,
                                           ChunkChangeCallback enterCallback,
                                           ChunkChangeCallback leaveCallback) {
    if (m_playerX == x && m_playerZ == z && m_positionSet) {
        return;  // 位置没变
    }

    m_playerX = x;
    m_playerZ = z;
    m_positionSet = true;

    updateChunksInRange(std::move(enterCallback), std::move(leaveCallback));
}

void PlayerChunkTracker::setViewDistance(i32 distance,
                                         ChunkChangeCallback enterCallback,
                                         ChunkChangeCallback leaveCallback) {
    const i32 clampedDistance = std::clamp(distance, 2, 32);

    if (m_viewDistance == clampedDistance) {
        return;
    }

    m_viewDistance = clampedDistance;

    if (m_positionSet) {
        updateChunksInRange(std::move(enterCallback), std::move(leaveCallback));
    }
}

bool PlayerChunkTracker::isChunkInRange(ChunkCoord x, ChunkCoord z) const {
    u64 key = posToKey(x, z);
    return m_chunksInRange.count(key) > 0;
}

void PlayerChunkTracker::clear(ChunkChangeCallback leaveCallback) {
    if (leaveCallback) {
        for (u64 key : m_chunksInRange) {
            ChunkCoord x, z;
            keyToPos(key, x, z);
            leaveCallback(x, z, false);
        }
    }

    m_chunksInRange.clear();
    m_positionSet = false;
}

i32 PlayerChunkTracker::getDistanceToPlayer(ChunkCoord x, ChunkCoord z) const {
    if (!m_positionSet) {
        return -1;
    }

    i32 dx = std::abs(x - m_playerX);
    i32 dz = std::abs(z - m_playerZ);
    i32 dist = std::max(dx, dz);

    if (dist <= m_viewDistance) {
        return dist;
    }
    return -1;
}

void PlayerChunkTracker::updateChunksInRange(ChunkChangeCallback enterCallback,
                                             ChunkChangeCallback leaveCallback) {
    std::unordered_set<u64> newChunks;
    newChunks.reserve(static_cast<size_t>((2 * m_viewDistance + 1) * (2 * m_viewDistance + 1)));

    // 遍历视距范围内的所有区块
    for (i32 dx = -m_viewDistance; dx <= m_viewDistance; ++dx) {
        for (i32 dz = -m_viewDistance; dz <= m_viewDistance; ++dz) {
            // 使用切比雪夫距离（棋盘距离）
            i32 dist = std::max(std::abs(dx), std::abs(dz));
            if (dist <= m_viewDistance) {
                ChunkCoord cx = m_playerX + dx;
                ChunkCoord cz = m_playerZ + dz;
                u64 key = posToKey(cx, cz);
                newChunks.insert(key);
            }
        }
    }

    // 触发离开回调
    if (leaveCallback) {
        for (u64 key : m_chunksInRange) {
            if (newChunks.find(key) == newChunks.end()) {
                ChunkCoord x, z;
                keyToPos(key, x, z);
                leaveCallback(x, z, false);
            }
        }
    }

    // 触发进入回调
    if (enterCallback) {
        for (u64 key : newChunks) {
            if (m_chunksInRange.find(key) == m_chunksInRange.end()) {
                ChunkCoord x, z;
                keyToPos(key, x, z);
                enterCallback(x, z, true);
            }
        }
    }

    m_chunksInRange = std::move(newChunks);
}

} // namespace mc::world
