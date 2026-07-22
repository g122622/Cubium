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

#include "ChunkSendManager.hpp"
#include "common/network/sync/ChunkSync.hpp"
#include "common/profiler/TraceEvents.hpp"
#include "common/world/chunk/load/ChunkLoadTicketManager.hpp"
#include "server/world/ServerChunkManager.hpp"
#include <algorithm>
#include <fmt/format.h>
#include <spdlog/spdlog.h>

using namespace mc::trace;

namespace mc::server::sync {

ChunkSendManager::ChunkSendManager(
    ServerChunkManager& chunkManager, world::chunk::ChunkLoadTicketManager& ticketManager)
    : m_chunkManager(chunkManager)
    , m_ticketManager(ticketManager)
{}

void ChunkSendManager::sendChunkToPlayers(
    ChunkCoord x, ChunkCoord z, const std::vector<PlayerId>& players, bool validateTracking)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Lighting, "ChunkSendManager::sendChunkToPlayers");

    if (players.empty()) {
        return;
    }

    // 检查区块是否已加载
    if (m_chunkManager.hasChunkInMem(x, z)) {
        auto chunk = m_chunkManager.tryToGetChunkSharedInMem(x, z);
        if (!chunk) {
            return;
        }

        // 取 ServerCompute 池；启动早期/测试环境未注入时为 nullptr，走同步 fallback。
        util::UniversalWorkerPool* executor = m_chunkManager.radiusAwareExecutor();
        if (executor == nullptr) {
            auto result = network::ChunkSerializer::serializeChunk(*chunk);
            if (result.success()) {
                submitChunkData(x, z, std::move(result.value()), players, validateTracking);
            } else {
                spdlog::warn("Failed to serialize chunk ({}, {}): {}", x, z, result.error().message());
            }
            return;
        }

        // 异步序列化：shared_ptr 捕获保活，worker 在途期间即使主线程卸载区块，ChunkData 引用计数维持存活。
        // writeRadius=0 区域互斥——与 RuntimeLightTask(writeRadius=2) 串行，保证 serialize 读
        // ChunkSection nibble 不与光照写竞争。结果经已有的线程安全 submitChunkData 入 m_readyChunks，
        // 主线程 processPendingSends drain 后真正发包。
        auto task = std::make_unique<util::FunctionTask>(
            util::TaskType::Custom,
            fmt::format("SerializeChunk({},{})", x, z),
            [this, x, z, chunk, players, validateTracking](const std::atomic<bool>& abortSignal) -> bool {
                // 任务可能被取消（关服/区块卸载），检查后安全跳过。
                // shared_ptr 随 lambda 析构释放，无需特殊清理。
                if (abortSignal.load(std::memory_order_acquire)) {
                    return false;
                }
                auto result = network::ChunkSerializer::serializeChunk(*chunk);
                if (result.failed()) {
                    spdlog::warn("Failed to serialize chunk ({}, {}): {}", x, z, result.error().message());
                    return true;
                }
                submitChunkData(x, z, std::move(result.value()), players, validateTracking);
                return true;
            },
            "server_chunk_serialize");

        executor->submit(std::move(task),
            /*callback=*/nullptr,
            /*centerX=*/x,
            /*centerZ=*/z,
            /*writeRadius=*/0,
            util::TaskPriority::Normal);
    } else {
        // 区块未加载，触发异步加载
        // 注意：玩家列表需要复制到回调中
        m_chunkManager.requestChunkAsync(
            x, z, ChunkStatuses::FULL, [this, x, z, players, validateTracking](bool success, ChunkData* chunk) {
                if (success && chunk) {
                    auto loadedChunk = m_chunkManager.tryToGetChunkSharedInMem(x, z);
                    if (!loadedChunk) {
                        spdlog::warn("Failed to get shared chunk ({}, {}) for sending", x, z);
                        return;
                    }

                    auto result = network::ChunkSerializer::serializeChunk(*loadedChunk);
                    if (result.success()) {
                        submitChunkData(x, z, std::move(result.value()), players, validateTracking);
                        // spdlog::info("Chunk ({}, {}) loaded async, serialized for {} players", x, z, players.size());
                    }
                } else {
                    spdlog::warn("Failed to load chunk ({}, {}) for sending", x, z);
                }
            });
    }
}

void ChunkSendManager::sendChunkToTrackingPlayers(ChunkCoord x, ChunkCoord z)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Lighting, "ChunkSendManager::sendChunkToTrackingPlayers");

    auto players = m_ticketManager.getTrackingPlayers(x, z);
    sendChunkToPlayers(x, z, players, true);
}

void ChunkSendManager::unloadChunkFromPlayers(ChunkCoord x, ChunkCoord z, const std::vector<PlayerId>& players)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Lighting, "ChunkSendManager::unloadChunkFromPlayers");

    for (PlayerId player : players) {
        if (m_onChunkUnload) {
            m_onChunkUnload(player, x, z);
        }
    }
    if (!players.empty()) {
        // spdlog::info("Sent unload chunk ({}, {}) to {} players", x, z, players.size());
    }
}

void ChunkSendManager::unloadChunkFromTrackingPlayers(ChunkCoord x, ChunkCoord z)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Lighting, "ChunkSendManager::unloadChunkFromTrackingPlayers");

    auto players = m_ticketManager.getTrackingPlayers(x, z);
    unloadChunkFromPlayers(x, z, players);
}

void ChunkSendManager::onPlayerTrackingChange(PlayerId player, ChunkCoord x, ChunkCoord z, bool isTracking)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Lighting, "ChunkSendManager::onPlayerTrackingChange");

    if (isTracking) {
        // 玩家进入区块视距范围
        // 检查区块是否已加载
        if (m_chunkManager.hasChunkInMem(x, z)) {
            // 区块已加载，立即发送
            sendChunkToPlayers(x, z, {player}, true);
        }
        // 区块未加载：票据系统会触发加载，加载完成后通过回调发送
    } else {
        // 玩家离开区块视距范围
        // 发送卸载通知
        unloadChunkFromPlayers(x, z, {player});
    }
}

void ChunkSendManager::onChunkPreUnload(ChunkCoord x, ChunkCoord z)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Lighting, "ChunkSendManager::onChunkPreUnload");

    // 向所有追踪该区块的玩家发送卸载通知
    unloadChunkFromTrackingPlayers(x, z);
}

void ChunkSendManager::removePlayer(PlayerId playerId)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Lighting, "ChunkSendManager::removePlayer", "playerId", playerId);

    // 追踪状态由 ChunkLoadTicketManager 管理。
    // 这里额外清理待发送队列里的目标玩家，避免断开后仍尝试发包。
    std::lock_guard<std::mutex> lock(m_readyChunksMutex);

    auto chunkIt = m_readyChunks.begin();
    while (chunkIt != m_readyChunks.end()) {
        auto& players = chunkIt->players;
        players.erase(std::remove(players.begin(), players.end(), playerId), players.end());

        if (players.empty()) {
            chunkIt = m_readyChunks.erase(chunkIt);
        } else {
            ++chunkIt;
        }
    }
}

void ChunkSendManager::submitChunkData(
    ChunkCoord x, ChunkCoord z, std::vector<u8> data, std::vector<PlayerId> players, bool validateTracking)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Lighting, "ChunkSendManager::submitChunkData");

    std::sort(players.begin(), players.end());
    players.erase(std::unique(players.begin(), players.end()), players.end());
    if (players.empty()) {
        return;
    }

    std::lock_guard<std::mutex> lock(m_readyChunksMutex);
    ReadyChunkData ready;
    ready.x = x;
    ready.z = z;
    ready.data = std::move(data);
    ready.players = std::move(players);
    ready.validateTracking = validateTracking;
    m_readyChunks.emplace_back(std::move(ready));
}

void ChunkSendManager::processPendingSends()
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Chunk, "processPendingSends");

    // 取出准备好的区块数据
    std::vector<ReadyChunkData> chunks;
    {
        std::lock_guard<std::mutex> lock(m_readyChunksMutex);
        chunks = std::move(m_readyChunks);
        m_readyChunks.clear();
    }

    // 处理每个区块
    for (const auto& chunk : chunks) {
        // 发送给每个玩家
        for (PlayerId playerId : chunk.players) {
            if (chunk.validateTracking && !m_ticketManager.isPlayerTracking(playerId, chunk.x, chunk.z)) {
                continue;
            }

            if (m_onChunkSend) {
                m_onChunkSend(playerId, chunk.x, chunk.z, chunk.data);
            }
        }
        if (!chunk.players.empty()) {
            // spdlog::info("Sent chunk ({}, {}) to {} players", x, z, players.size());
        }
    }
}

void ChunkSendManager::setOnChunkSend(
    std::function<void(PlayerId, ChunkCoord, ChunkCoord, const std::vector<u8>&)> callback)
{
    m_onChunkSend = std::move(callback);
}

void ChunkSendManager::setOnChunkUnload(std::function<void(PlayerId, ChunkCoord, ChunkCoord)> callback)
{
    m_onChunkUnload = std::move(callback);
}

} // namespace mc::server::sync
