#include "PlayerManager.hpp"
#include <spdlog/spdlog.h>

namespace mc::server::core {

PlayerManager::PlayerManager(const ServerCoreConfig& config)
    : m_maxPlayers(config.maxPlayers)
{
    m_chunkSyncManager.setDefaultViewDistance(config.viewDistance);
}

ServerPlayerData* PlayerManager::addPlayer(PlayerId playerId,
                                            const std::string& username,
                                            network::ConnectionPtr connection) {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);

    // 使用 try_emplace 避免双重查找
    auto [it, inserted] = m_players.try_emplace(playerId);
    if (!inserted) {
        spdlog::warn("PlayerManager: Player {} already exists", playerId);
        return nullptr;
    }

    // 检查是否已满
    if (static_cast<i32>(m_players.size()) > m_maxPlayers) {
        // 回滚插入
        m_players.erase(it);
        spdlog::warn("PlayerManager: Server is full ({} players)", m_maxPlayers);
        return nullptr;
    }

    auto& player = it->second;
    player.playerId = playerId;
    player.username = username;
    player.connection = connection;
    player.loggedIn = true;
    player.chunkTracker = std::make_shared<network::PlayerChunkTracker>(playerId);

    // 更新区块同步管理器
    (void)m_chunkSyncManager.getTracker(playerId);
    m_chunkSyncManager.updatePlayerPosition(playerId, player.x, player.z);

    spdlog::debug("PlayerManager: Player {} ({}) added", username, playerId);
    return &player;
}

void PlayerManager::removePlayer(PlayerId playerId) {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);

    auto it = m_players.find(playerId);
    if (it == m_players.end()) return;

    std::string username = it->second.username;
    u32 sessionId = it->second.sessionId;

    // 移除会话映射
    if (sessionId != 0) {
        m_sessionToPlayer.erase(sessionId);
    }

    // 移除玩家
    m_players.erase(it);
    m_chunkSyncManager.removeTracker(playerId);

    spdlog::debug("PlayerManager: Player {} ({}) removed", username, playerId);
}

void PlayerManager::removePlayerBySessionId(u32 sessionId) {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);

    auto it = m_sessionToPlayer.find(sessionId);
    if (it == m_sessionToPlayer.end()) return;

    PlayerId playerId = it->second;
    auto playerIt = m_players.find(playerId);
    if (playerIt == m_players.end()) return;

    std::string username = playerIt->second.username;

    // 移除会话映射
    m_sessionToPlayer.erase(sessionId);

    // 移除玩家
    m_players.erase(playerIt);
    m_chunkSyncManager.removeTracker(playerId);

    spdlog::debug("PlayerManager: Player {} ({}) removed by session {}",
                  username, playerId, sessionId);
}

ServerPlayerData* PlayerManager::findBySessionId(u32 sessionId) {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    auto it = m_sessionToPlayer.find(sessionId);
    if (it == m_sessionToPlayer.end()) return nullptr;
    auto playerIt = m_players.find(it->second);
    return playerIt != m_players.end() ? &playerIt->second : nullptr;
}

const ServerPlayerData* PlayerManager::findBySessionId(u32 sessionId) const {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    auto it = m_sessionToPlayer.find(sessionId);
    if (it == m_sessionToPlayer.end()) return nullptr;
    auto playerIt = m_players.find(it->second);
    return playerIt != m_players.end() ? &playerIt->second : nullptr;
}

ServerPlayerData* PlayerManager::getPlayer(PlayerId playerId) {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    auto it = m_players.find(playerId);
    return it != m_players.end() ? &it->second : nullptr;
}

const ServerPlayerData* PlayerManager::getPlayer(PlayerId playerId) const {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    auto it = m_players.find(playerId);
    return it != m_players.end() ? &it->second : nullptr;
}

bool PlayerManager::hasPlayer(PlayerId playerId) const {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    return m_players.find(playerId) != m_players.end();
}

size_t PlayerManager::playerCount() const {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    return m_players.size();
}

bool PlayerManager::isFull() const {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    return static_cast<i32>(m_players.size()) >= m_maxPlayers;
}

std::vector<PlayerId> PlayerManager::getPlayerIds() const {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    std::vector<PlayerId> ids;
    ids.reserve(m_players.size());
    for (const auto& [id, player] : m_players) {
        ids.push_back(id);
    }
    return ids;
}

void PlayerManager::mapSessionToPlayer(u32 sessionId, PlayerId playerId) {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    m_sessionToPlayer[sessionId] = playerId;

    auto it = m_players.find(playerId);
    if (it != m_players.end()) {
        it->second.sessionId = sessionId;
    }
}

void PlayerManager::unmapSession(u32 sessionId) {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);

    auto it = m_sessionToPlayer.find(sessionId);
    if (it == m_sessionToPlayer.end()) return;

    PlayerId playerId = it->second;
    m_sessionToPlayer.erase(sessionId);

    auto playerIt = m_players.find(playerId);
    if (playerIt != m_players.end()) {
        playerIt->second.sessionId = 0;
    }
}

PlayerId PlayerManager::getPlayerIdBySession(u32 sessionId) const {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    auto it = m_sessionToPlayer.find(sessionId);
    return it != m_sessionToPlayer.end() ? it->second : 0;
}

} // namespace mc::server::core
