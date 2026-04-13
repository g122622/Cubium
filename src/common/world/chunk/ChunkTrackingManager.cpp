#include "ChunkTrackingManager.hpp"
#include <algorithm>

namespace mc::world {

void ChunkTrackingManager::updatePlayerPosition(PlayerId playerId, ChunkCoord x, ChunkCoord z) {
    // 获取或创建玩家追踪器
    auto it = m_playerTrackers.find(playerId);
    PlayerChunkTracker* tracker = nullptr;

    if (it == m_playerTrackers.end()) {
        auto [insertIt, inserted] = m_playerTrackers.emplace(
            playerId, std::make_unique<PlayerChunkTracker>(m_defaultViewDistance));
        tracker = insertIt->second.get();
    } else {
        tracker = it->second.get();
    }

    // 设置玩家位置，触发追踪变化回调
    tracker->setPlayerPosition(x, z,
        [this, playerId](ChunkCoord cx, ChunkCoord cz, bool isTracking) {
            u64 key = posToKey(cx, cz);
            {
                std::lock_guard<std::mutex> lock(m_trackingMutex);
                if (isTracking) {
                    m_chunkTrackingPlayers[key].insert(playerId);
                } else {
                    auto chunkIt = m_chunkTrackingPlayers.find(key);
                    if (chunkIt != m_chunkTrackingPlayers.end()) {
                        chunkIt->second.erase(playerId);
                        if (chunkIt->second.empty()) {
                            m_chunkTrackingPlayers.erase(chunkIt);
                        }
                    }
                }
            }

            if (m_trackingChangeCallback) {
                m_trackingChangeCallback(playerId, cx, cz, isTracking);
            }
        },
        [this, playerId](ChunkCoord cx, ChunkCoord cz, bool isTracking) {
            u64 key = posToKey(cx, cz);
            {
                std::lock_guard<std::mutex> lock(m_trackingMutex);
                auto chunkIt = m_chunkTrackingPlayers.find(key);
                if (chunkIt != m_chunkTrackingPlayers.end()) {
                    chunkIt->second.erase(playerId);
                    if (chunkIt->second.empty()) {
                        m_chunkTrackingPlayers.erase(chunkIt);
                    }
                }
            }

            if (m_trackingChangeCallback) {
                m_trackingChangeCallback(playerId, cx, cz, isTracking);
            }
        }
    );
}

void ChunkTrackingManager::removePlayer(PlayerId playerId) {
    auto it = m_playerTrackers.find(playerId);
    if (it == m_playerTrackers.end()) {
        return;
    }

    PlayerChunkTracker* tracker = it->second.get();

    // 清除所有追踪关系
    tracker->clear([this, playerId](ChunkCoord cx, ChunkCoord cz, bool isTracking) {
        u64 key = posToKey(cx, cz);
        {
            std::lock_guard<std::mutex> lock(m_trackingMutex);
            auto chunkIt = m_chunkTrackingPlayers.find(key);
            if (chunkIt != m_chunkTrackingPlayers.end()) {
                chunkIt->second.erase(playerId);
                if (chunkIt->second.empty()) {
                    m_chunkTrackingPlayers.erase(chunkIt);
                }
            }
        }

        if (m_trackingChangeCallback) {
            m_trackingChangeCallback(playerId, cx, cz, isTracking);
        }
    });

    m_playerTrackers.erase(it);
}

bool ChunkTrackingManager::hasPlayer(PlayerId playerId) const {
    return m_playerTrackers.find(playerId) != m_playerTrackers.end();
}

const PlayerChunkTracker* ChunkTrackingManager::getPlayerTracker(PlayerId playerId) const {
    auto it = m_playerTrackers.find(playerId);
    return it != m_playerTrackers.end() ? it->second.get() : nullptr;
}

void ChunkTrackingManager::setPlayerViewDistance(PlayerId playerId, i32 distance) {
    auto it = m_playerTrackers.find(playerId);
    if (it == m_playerTrackers.end()) {
        return;
    }

    PlayerChunkTracker* tracker = it->second.get();

    tracker->setViewDistance(distance,
        [this, playerId](ChunkCoord cx, ChunkCoord cz, bool isTracking) {
            u64 key = posToKey(cx, cz);
            {
                std::lock_guard<std::mutex> lock(m_trackingMutex);
                m_chunkTrackingPlayers[key].insert(playerId);
            }

            if (m_trackingChangeCallback) {
                m_trackingChangeCallback(playerId, cx, cz, isTracking);
            }
        },
        [this, playerId](ChunkCoord cx, ChunkCoord cz, bool isTracking) {
            u64 key = posToKey(cx, cz);
            {
                std::lock_guard<std::mutex> lock(m_trackingMutex);
                auto chunkIt = m_chunkTrackingPlayers.find(key);
                if (chunkIt != m_chunkTrackingPlayers.end()) {
                    chunkIt->second.erase(playerId);
                    if (chunkIt->second.empty()) {
                        m_chunkTrackingPlayers.erase(chunkIt);
                    }
                }
            }

            if (m_trackingChangeCallback) {
                m_trackingChangeCallback(playerId, cx, cz, isTracking);
            }
        }
    );
}

std::vector<PlayerId> ChunkTrackingManager::getTrackingPlayers(ChunkCoord x, ChunkCoord z) const {
    u64 key = posToKey(x, z);
    std::lock_guard<std::mutex> lock(m_trackingMutex);

    std::vector<PlayerId> result;
    auto it = m_chunkTrackingPlayers.find(key);
    if (it != m_chunkTrackingPlayers.end()) {
        result.assign(it->second.begin(), it->second.end());
    }
    return result;
}

bool ChunkTrackingManager::isPlayerTracking(PlayerId playerId, ChunkCoord x, ChunkCoord z) const {
    u64 key = posToKey(x, z);
    std::lock_guard<std::mutex> lock(m_trackingMutex);

    auto it = m_chunkTrackingPlayers.find(key);
    if (it == m_chunkTrackingPlayers.end()) {
        return false;
    }
    return it->second.find(playerId) != it->second.end();
}

bool ChunkTrackingManager::hasTrackingPlayers(u64 chunkKey) const {
    std::lock_guard<std::mutex> lock(m_trackingMutex);

    auto it = m_chunkTrackingPlayers.find(chunkKey);
    return it != m_chunkTrackingPlayers.end() && !it->second.empty();
}

size_t ChunkTrackingManager::playerCount() const {
    return m_playerTrackers.size();
}

} // namespace mc::world
