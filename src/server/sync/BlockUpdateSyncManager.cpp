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

#include "BlockUpdateSyncManager.hpp"

#include "common/profiler/TraceEvents.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/world/chunk/load/ChunkLoadTicketManager.hpp"

#include <algorithm>
#include <vector>

using namespace mc::trace;

namespace mc::server::sync {

BlockUpdateSyncManager::BlockUpdateSyncManager(world::chunk::ChunkLoadTicketManager& ticketManager)
    : m_ticketManager(ticketManager)
{}

void BlockUpdateSyncManager::queueBlockUpdate(const BlockPos& pos, u32 blockStateId)
{
    m_pendingBlockUpdates[pos] = blockStateId;
}

void BlockUpdateSyncManager::flushPendingUpdates()
{
    MC_TRACE_SCOPED_EVENT(
        TraceEvents.Server.Network, "FlushBlockUpdates", "pendingCount", m_pendingBlockUpdates.size());

    if (m_pendingBlockUpdates.empty()) {
        return;
    }

    std::vector<PendingBlockUpdate> pendingUpdates;
    pendingUpdates.reserve(m_pendingBlockUpdates.size());

    for (const auto& [pos, blockStateId] : m_pendingBlockUpdates) {
        pendingUpdates.push_back(PendingBlockUpdate{pos, blockStateId});
    }

    m_pendingBlockUpdates.clear();

    std::sort(pendingUpdates.begin(),
        pendingUpdates.end(),
        [](const PendingBlockUpdate& left, const PendingBlockUpdate& right) {
            const u64 leftChunkKey = _chunkKey(left.pos.chunkX(), left.pos.chunkZ());
            const u64 rightChunkKey = _chunkKey(right.pos.chunkX(), right.pos.chunkZ());
            if (leftChunkKey != rightChunkKey) {
                return leftChunkKey < rightChunkKey;
            }

            return left.pos < right.pos;
        });

    for (size_t index = 0; index < pendingUpdates.size();) {
        const PendingBlockUpdate& firstUpdate = pendingUpdates[index];
        const ChunkCoord chunkX = firstUpdate.pos.chunkX();
        const ChunkCoord chunkZ = firstUpdate.pos.chunkZ();
        const u64 currentChunkKey = _chunkKey(chunkX, chunkZ);

        size_t groupEnd = index + 1;
        while (groupEnd < pendingUpdates.size()) {
            const PendingBlockUpdate& nextUpdate = pendingUpdates[groupEnd];
            if (_chunkKey(nextUpdate.pos.chunkX(), nextUpdate.pos.chunkZ()) != currentChunkKey) {
                break;
            }
            ++groupEnd;
        }

        std::vector<PlayerId> players = m_ticketManager.getTrackingPlayers(chunkX, chunkZ);
        if (players.empty()) {
            index = groupEnd;
            continue;
        }

        MC_ASSERT_RELEASE(m_onBlockUpdate != nullptr);
        std::sort(players.begin(), players.end());

        for (size_t updateIndex = index; updateIndex < groupEnd; ++updateIndex) {
            const PendingBlockUpdate& update = pendingUpdates[updateIndex];
            for (PlayerId playerId : players) {
                m_onBlockUpdate(playerId, update.pos.x, update.pos.y, update.pos.z, update.blockStateId);
            }
        }

        index = groupEnd;
    }
}

void BlockUpdateSyncManager::setOnBlockUpdate(std::function<void(PlayerId, i32, i32, i32, u32)> callback)
{
    m_onBlockUpdate = std::move(callback);
}

u64 BlockUpdateSyncManager::_chunkKey(ChunkCoord x, ChunkCoord z)
{
    return (static_cast<u64>(static_cast<u32>(x)) << 32) | static_cast<u32>(z);
}

} // namespace mc::server::sync
