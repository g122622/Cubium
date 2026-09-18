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

#include "server/network/sync/chunk/ChunkSyncManager.hpp"

#include <memory>
#include <utility>

namespace mc::server::sync {

std::shared_ptr<PlayerChunkTracker> ChunkSyncManager::getTracker(PlayerId playerId)
{
    auto it = m_trackers.find(playerId);
    if (it != m_trackers.end()) {
        return it->second;
    }

    auto tracker = std::make_shared<PlayerChunkTracker>(playerId);
    tracker->setViewDistance(m_defaultViewDistance);
    m_trackers[playerId] = tracker;
    return tracker;
}

void ChunkSyncManager::removeTracker(PlayerId playerId)
{
    auto it = m_trackers.find(playerId);
    if (it == m_trackers.end()) return;

    // 从区块订阅中移除该玩家
    auto& chunks = it->second->loadedChunks();
    for (const auto& chunkId : chunks) {
        auto subIt = m_chunkSubscribers.find(chunkId);
        if (subIt != m_chunkSubscribers.end()) {
            subIt->second.erase(playerId);
            if (subIt->second.empty()) {
                m_chunkSubscribers.erase(subIt);
            }
        }
    }

    m_trackers.erase(it);
}

void ChunkSyncManager::updatePlayerPosition(PlayerId playerId, f64 x, f64 z)
{
    auto tracker = getTracker(playerId);

    ChunkCoord newChunkX = blockToChunk(x);
    ChunkCoord newChunkZ = blockToChunk(z);

    ChunkCoord oldChunkX = tracker->view().centerX;
    ChunkCoord oldChunkZ = tracker->view().centerZ;

    if (newChunkX != oldChunkX || newChunkZ != oldChunkZ) {
        tracker->updateCenter(newChunkX, newChunkZ);
    }
}

void ChunkSyncManager::calculateUpdates(
    PlayerId playerId, std::vector<ChunkPos>& chunksToLoad, std::vector<ChunkPos>& chunksToUnload)
{
    chunksToLoad.clear();
    chunksToUnload.clear();

    auto tracker = getTracker(playerId);
    tracker->calculateChunkUpdates(chunksToLoad, chunksToUnload);
}

void ChunkSyncManager::markChunkSent(PlayerId playerId, ChunkCoord x, ChunkCoord z)
{
    auto tracker = getTracker(playerId);

    ChunkId chunkId(x, z, 0);
    tracker->addLoadedChunk(x, z);
    m_chunkSubscribers[chunkId].insert(playerId);
}

void ChunkSyncManager::markChunkUnloaded(PlayerId playerId, ChunkCoord x, ChunkCoord z)
{
    auto tracker = getTracker(playerId);

    ChunkId chunkId(x, z, 0);
    tracker->removeLoadedChunk(x, z);

    auto it = m_chunkSubscribers.find(chunkId);
    if (it != m_chunkSubscribers.end()) {
        it->second.erase(playerId);
        if (it->second.empty()) {
            m_chunkSubscribers.erase(it);
        }
    }
}

} // namespace mc::server::sync
