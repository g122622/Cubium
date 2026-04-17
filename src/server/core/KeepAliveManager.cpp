#include "KeepAliveManager.hpp"
#include "PlayerManager.hpp"
#include <spdlog/spdlog.h>

namespace mc::server::core {

KeepAliveManager::KeepAliveManager(PlayerManager& playerManager, const ServerCoreConfig& config)
    : m_playerManager(playerManager)
    , m_keepAliveInterval(config.keepAliveInterval)
    , m_keepAliveTimeout(config.keepAliveTimeout)
{
}

bool KeepAliveManager::needsKeepAlive(PlayerId playerId, u64 currentTickMs) const {
    auto* player = m_playerManager.getPlayer(playerId);
    if (!player) return false;

    u64 lastSent = player->lastKeepAliveSent;
    return (currentTickMs - lastSent) >= static_cast<u64>(m_keepAliveInterval);
}

std::vector<PlayerId> KeepAliveManager::getPlayersNeedingKeepAlive(u64 currentTickMs) const {
    std::vector<PlayerId> result;
    result.reserve(m_playerManager.playerCount());
    m_playerManager.forEachPlayer([&](const ServerPlayerData& player) {
        u64 lastSent = player.lastKeepAliveSent;
        if ((currentTickMs - lastSent) >= static_cast<u64>(m_keepAliveInterval)) {
            result.push_back(player.playerId);
        }
    });
    return result;
}

void KeepAliveManager::recordKeepAliveSent(PlayerId playerId, u64 timestamp, u64 tick) {
    auto* player = m_playerManager.getPlayer(playerId);
    if (!player) return;

    player->lastKeepAliveSent = timestamp;
    player->lastKeepAliveSentTick = tick;
    spdlog::trace("KeepAliveManager: Sent keepalive to player {} at {}", playerId, timestamp);
}

void KeepAliveManager::handleKeepAliveResponse(PlayerId playerId, u64 timestamp, u64 currentTimeMs) {
    auto* player = m_playerManager.getPlayer(playerId);
    if (!player) return;

    // 验证时间戳是否匹配
    if (player->lastKeepAliveSent != timestamp) {
        spdlog::debug("KeepAliveManager: Player {} keepalive timestamp mismatch", playerId);
        return;
    }

    player->lastKeepAliveReceived = currentTimeMs;

    // 计算 ping
    u32 ping = static_cast<u32>(currentTimeMs - timestamp);
    player->ping = ping;

    spdlog::trace("KeepAliveManager: Player {} keepalive response, ping={}ms", playerId, ping);
}

void KeepAliveManager::updateKeepAlive(PlayerId playerId, u64 timestamp) {
    auto* player = m_playerManager.getPlayer(playerId);
    if (!player) return;

    player->lastKeepAliveReceived = timestamp;
}

bool KeepAliveManager::isTimedOut(PlayerId playerId, u64 currentTickMs) const {
    auto* player = m_playerManager.getPlayer(playerId);
    if (!player) return false;

    u64 lastReceived = player->lastKeepAliveReceived;
    return (currentTickMs - lastReceived) >= static_cast<u64>(m_keepAliveTimeout);
}

std::vector<PlayerId> KeepAliveManager::getTimedOutPlayers(u64 currentTickMs) const {
    std::vector<PlayerId> result;
    m_playerManager.forEachPlayer([&](const ServerPlayerData& player) {
        u64 lastReceived = player.lastKeepAliveReceived;
        if (lastReceived > 0 && (currentTickMs - lastReceived) >= static_cast<u64>(m_keepAliveTimeout)) {
            result.push_back(player.playerId);
        }
    });
    return result;
}

u32 KeepAliveManager::getPlayerPing(PlayerId playerId) const {
    auto* player = m_playerManager.getPlayer(playerId);
    return player ? player->ping : 0;
}

u64 KeepAliveManager::getLastKeepAliveSent(PlayerId playerId) const {
    auto* player = m_playerManager.getPlayer(playerId);
    return player ? player->lastKeepAliveSent : 0;
}

u64 KeepAliveManager::getLastKeepAliveReceived(PlayerId playerId) const {
    auto* player = m_playerManager.getPlayer(playerId);
    return player ? player->lastKeepAliveReceived : 0;
}

} // namespace mc::server::core
