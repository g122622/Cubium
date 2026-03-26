#include "ChunkSendManager.hpp"
#include "server/world/ServerChunkManager.hpp"
#include "common/world/chunk/ChunkLoadTicketManager.hpp"
#include "common/network/sync/ChunkSync.hpp"
#include <spdlog/spdlog.h>
#include "common/perfetto/TraceEvents.hpp"

namespace mc::server::sync {

ChunkSendManager::ChunkSendManager(ServerChunkManager& chunkManager,
                                   world::ChunkLoadTicketManager& ticketManager)
    : m_chunkManager(chunkManager)
    , m_ticketManager(ticketManager)
{
}

void ChunkSendManager::sendChunkToPlayers(ChunkCoord x, ChunkCoord z, const std::vector<PlayerId>& players) {
    MC_TRACE_EVENT("server.lighting", "ChunkSendManager::sendChunkToPlayers");

    if (players.empty()) {
        return;
    }

    // 检查区块是否已加载
    if (m_chunkManager.hasChunk(x, z)) {
        ChunkData* chunk = m_chunkManager.getChunk(x, z);
        if (chunk) {
            auto result = network::ChunkSerializer::serializeChunk(*chunk);
            if (result.success()) {
                submitChunkData(x, z, std::move(result.value()), players);
                // spdlog::info("Chunk ({}, {}) serialized for {} players", x, z, players.size());
            } else {
                spdlog::warn("Failed to serialize chunk ({}, {}): {}", x, z, result.error().message());
            }
        }
    } else {
        // 区块未加载，触发异步加载
        // 注意：玩家列表需要复制到回调中
        m_chunkManager.getChunkAsync(x, z, [this, x, z, players](bool success, ChunkData* chunk) {
            if (success && chunk) {
                auto result = network::ChunkSerializer::serializeChunk(*chunk);
                if (result.success()) {
                    submitChunkData(x, z, std::move(result.value()), players);
                    // spdlog::info("Chunk ({}, {}) loaded async, serialized for {} players", x, z, players.size());
                }
            } else {
                spdlog::warn("Failed to load chunk ({}, {}) for sending", x, z);
            }
        });
    }
}

void ChunkSendManager::sendChunkToTrackingPlayers(ChunkCoord x, ChunkCoord z) {
    auto players = m_ticketManager.getTrackingPlayers(x, z);
    sendChunkToPlayers(x, z, players);
}

void ChunkSendManager::unloadChunkFromPlayers(ChunkCoord x, ChunkCoord z, const std::vector<PlayerId>& players) {
    for (PlayerId player : players) {
        if (m_onChunkUnload) {
            m_onChunkUnload(player, x, z);
        }
    }
    if (!players.empty()) {
        // spdlog::info("Sent unload chunk ({}, {}) to {} players", x, z, players.size());
    }
}

void ChunkSendManager::unloadChunkFromTrackingPlayers(ChunkCoord x, ChunkCoord z) {
    auto players = m_ticketManager.getTrackingPlayers(x, z);
    unloadChunkFromPlayers(x, z, players);
}

void ChunkSendManager::onPlayerTrackingChange(PlayerId player, ChunkCoord x, ChunkCoord z, bool isTracking) {
    if (isTracking) {
        // 玩家进入区块视距范围
        // 检查区块是否已加载
        if (m_chunkManager.hasChunk(x, z)) {
            // 区块已加载，立即发送
            sendChunkToPlayers(x, z, {player});
        }
        // 区块未加载：票据系统会触发加载，加载完成后通过回调发送
    } else {
        // 玩家离开区块视距范围
        // 发送卸载通知
        unloadChunkFromPlayers(x, z, {player});
    }
}

void ChunkSendManager::onChunkPreUnload(ChunkCoord x, ChunkCoord z) {
    // 向所有追踪该区块的玩家发送卸载通知
    unloadChunkFromTrackingPlayers(x, z);
}

void ChunkSendManager::removePlayer(PlayerId playerId) {
    // 当前实现中，追踪信息由 ChunkLoadTicketManager 管理
    // 这里只需要确保没有待发送的数据给该玩家
    // 由于 submitChunkData 复制了玩家列表，我们无法取消已提交的数据
    // 但可以确保未来不会发送给该玩家
    (void)playerId;
}

void ChunkSendManager::submitChunkData(ChunkCoord x, ChunkCoord z, std::vector<u8> data, std::vector<PlayerId> players) {
    std::lock_guard<std::mutex> lock(m_readyChunksMutex);
    m_readyChunks.emplace_back(x, z, std::move(data), std::move(players));
}

void ChunkSendManager::processPendingSends() {
    MC_TRACE_EVENT("server.chunk", "processPendingSends");

    // 取出准备好的区块数据
    std::vector<std::tuple<ChunkCoord, ChunkCoord, std::vector<u8>, std::vector<PlayerId>>> chunks;
    {
        std::lock_guard<std::mutex> lock(m_readyChunksMutex);
        chunks = std::move(m_readyChunks);
        m_readyChunks.clear();
    }

    // 处理每个区块
    for (const auto& [x, z, data, players] : chunks) {
        // 发送给每个玩家
        for (PlayerId playerId : players) {
            if (m_onChunkSend) {
                m_onChunkSend(playerId, x, z, data);
            }
        }
        if (!players.empty()) {
            // spdlog::info("Sent chunk ({}, {}) to {} players", x, z, players.size());
        }
    }
}

void ChunkSendManager::setOnChunkSend(std::function<void(PlayerId, ChunkCoord, ChunkCoord, const std::vector<u8>&)> callback) {
    m_onChunkSend = std::move(callback);
}

void ChunkSendManager::setOnChunkUnload(std::function<void(PlayerId, ChunkCoord, ChunkCoord)> callback) {
    m_onChunkUnload = std::move(callback);
}

} // namespace mc::server::sync
