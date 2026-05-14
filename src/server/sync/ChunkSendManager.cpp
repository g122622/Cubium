#include "ChunkSendManager.hpp"
#include "common/network/sync/ChunkSync.hpp"
#include "common/perfetto/TraceEvents.hpp"
#include "common/world/chunk/ChunkLoadTicketManager.hpp"
#include "server/world/ServerChunkManager.hpp"
#include <algorithm>
#include <spdlog/spdlog.h>

namespace mc::server::sync {

ChunkSendManager::ChunkSendManager(ServerChunkManager& chunkManager, world::ChunkLoadTicketManager& ticketManager)
    : m_chunkManager(chunkManager)
    , m_ticketManager(ticketManager)
{}

void ChunkSendManager::sendChunkToPlayers(
    ChunkCoord x, ChunkCoord z, const std::vector<PlayerId>& players, bool validateTracking)
{
    MC_TRACE_EVENT("server.lighting", "ChunkSendManager::sendChunkToPlayers");

    if (players.empty()) {
        return;
    }

    // 检查区块是否已加载
    if (m_chunkManager.hasChunkInMem(x, z)) {
        auto chunk = m_chunkManager.tryToGetChunkSharedInMem(x, z);
        if (chunk) {
            auto result = network::ChunkSerializer::serializeChunk(*chunk);
            if (result.success()) {
                submitChunkData(x, z, std::move(result.value()), players, validateTracking);
                // spdlog::info("Chunk ({}, {}) serialized for {} players", x, z, players.size());
            } else {
                spdlog::warn("Failed to serialize chunk ({}, {}): {}", x, z, result.error().message());
            }
        }
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
    MC_TRACE_EVENT("server.lighting", "ChunkSendManager::sendChunkToTrackingPlayers");

    auto players = m_ticketManager.getTrackingPlayers(x, z);
    sendChunkToPlayers(x, z, players, true);
}

void ChunkSendManager::unloadChunkFromPlayers(ChunkCoord x, ChunkCoord z, const std::vector<PlayerId>& players)
{
    MC_TRACE_EVENT("server.lighting", "ChunkSendManager::unloadChunkFromPlayers");

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
    MC_TRACE_EVENT("server.lighting", "ChunkSendManager::unloadChunkFromTrackingPlayers");

    auto players = m_ticketManager.getTrackingPlayers(x, z);
    unloadChunkFromPlayers(x, z, players);
}

void ChunkSendManager::onPlayerTrackingChange(PlayerId player, ChunkCoord x, ChunkCoord z, bool isTracking)
{
    MC_TRACE_EVENT("server.lighting", "ChunkSendManager::onPlayerTrackingChange");

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
    MC_TRACE_EVENT("server.lighting", "ChunkSendManager::onChunkPreUnload");

    // 向所有追踪该区块的玩家发送卸载通知
    unloadChunkFromTrackingPlayers(x, z);
}

void ChunkSendManager::removePlayer(PlayerId playerId)
{
    MC_TRACE_EVENT("server.lighting", "ChunkSendManager::removePlayer", "playerId", playerId);

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
    MC_TRACE_EVENT("server.lighting", "ChunkSendManager::submitChunkData");

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
    MC_TRACE_EVENT("server.chunk", "processPendingSends");

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
