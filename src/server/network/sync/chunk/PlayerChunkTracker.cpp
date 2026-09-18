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

#include "server/network/sync/chunk/PlayerChunkTracker.hpp"

#include <algorithm>

namespace mc::server::sync {

PlayerChunkTracker::PlayerChunkTracker(PlayerId playerId)
    : m_playerId(playerId)
{}

void PlayerChunkTracker::addLoadedChunk(ChunkCoord x, ChunkCoord z)
{
    m_loadedChunks.insert(ChunkId(x, z, 0));
}

void PlayerChunkTracker::removeLoadedChunk(ChunkCoord x, ChunkCoord z)
{
    m_loadedChunks.erase(ChunkId(x, z, 0));
}

bool PlayerChunkTracker::hasChunk(ChunkCoord x, ChunkCoord z) const
{
    return m_loadedChunks.find(ChunkId(x, z, 0)) != m_loadedChunks.end();
}

void PlayerChunkTracker::updateCenter(ChunkCoord x, ChunkCoord z)
{
    m_view.centerX = x;
    m_view.centerZ = z;
}

void PlayerChunkTracker::calculateChunkUpdates(
    std::vector<ChunkPos>& chunksToLoad, std::vector<ChunkPos>& chunksToUnload)
{
    m_view.calculateChunkDiff(m_loadedChunks, chunksToLoad, chunksToUnload);
}

void PlayerChunkTracker::setViewDistance(i32 distance)
{
    m_view.viewDistance = std::clamp(distance, 2, 32);
}

void PlayerChunkTracker::clear()
{
    m_loadedChunks.clear();
}

} // namespace mc::server::sync
