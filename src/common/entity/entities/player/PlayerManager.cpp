#include "PlayerManager.hpp"

namespace mc {

// ============================================================================
// PlayerManager 实现
// ============================================================================

PlayerManager::PlayerPtr PlayerManager::createPlayer(const String& username) {
    std::lock_guard<std::mutex> lock(m_mutex);

    // 检查玩家数量限制
    if (m_playersById.size() >= MAX_PLAYERS) {
        return nullptr;
    }

    // 检查用户名是否已存在
    if (m_playerIdsByUsername.find(username) != m_playerIdsByUsername.end()) {
        return nullptr;
    }

    // 生成ID（内部版本，不加锁）
    PlayerId id = m_nextPlayerId++;
    EntityId entityId = m_nextEntityId++;

    // 创建玩家
    auto player = std::make_shared<Player>(entityId, username);
    player->setPlayerId(id);

    // 添加到映射
    m_playersById[id] = player;
    m_playersByEntityId[entityId] = player;
    m_playerIdsByUsername[username] = id;

    return player;
}

PlayerManager::PlayerPtr PlayerManager::createPlayer(PlayerId id, const String& username) {
    std::lock_guard<std::mutex> lock(m_mutex);

    // 检查玩家数量限制
    if (m_playersById.size() >= MAX_PLAYERS) {
        return nullptr;
    }

    // 检查用户名是否已存在
    if (m_playerIdsByUsername.find(username) != m_playerIdsByUsername.end()) {
        return nullptr;
    }

    // 检查ID是否已存在
    if (m_playersById.find(id) != m_playersById.end()) {
        return nullptr;
    }

    // 生成实体ID（内部版本，不加锁）
    EntityId entityId = m_nextEntityId++;

    // 创建玩家
    auto player = std::make_shared<Player>(entityId, username);
    player->setPlayerId(id);

    // 添加到映射
    m_playersById[id] = player;
    m_playersByEntityId[entityId] = player;
    m_playerIdsByUsername[username] = id;

    return player;
}

void PlayerManager::removePlayer(PlayerId id) {
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_playersById.find(id);
    if (it == m_playersById.end()) {
        return;
    }

    auto player = it->second;

    // 从所有映射中移除
    m_playersByEntityId.erase(player->id());
    m_playerIdsByUsername.erase(player->username());
    m_playersById.erase(it);
}

void PlayerManager::removePlayer(const PlayerPtr& player) {
    if (player) {
        removePlayer(player->playerId());
    }
}

PlayerManager::PlayerPtr PlayerManager::getPlayer(PlayerId id) const {
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_playersById.find(id);
    if (it != m_playersById.end()) {
        return it->second;
    }
    return nullptr;
}

PlayerManager::PlayerPtr PlayerManager::getPlayerByUsername(const String& username) const {
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_playerIdsByUsername.find(username);
    if (it != m_playerIdsByUsername.end()) {
        auto playerIt = m_playersById.find(it->second);
        if (playerIt != m_playersById.end()) {
            return playerIt->second;
        }
    }
    return nullptr;
}

PlayerManager::PlayerPtr PlayerManager::getPlayerByEntityId(EntityId id) const {
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_playersByEntityId.find(id);
    if (it != m_playersByEntityId.end()) {
        return it->second;
    }
    return nullptr;
}

bool PlayerManager::hasPlayer(PlayerId id) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_playersById.find(id) != m_playersById.end();
}

bool PlayerManager::hasUsername(const String& username) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_playerIdsByUsername.find(username) != m_playerIdsByUsername.end();
}

size_t PlayerManager::playerCount() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_playersById.size();
}

std::vector<PlayerManager::PlayerPtr> PlayerManager::getAllPlayers() const {
    std::lock_guard<std::mutex> lock(m_mutex);

    std::vector<PlayerPtr> players;
    players.reserve(m_playersById.size());
    for (const auto& pair : m_playersById) {
        players.push_back(pair.second);
    }
    return players;
}

void PlayerManager::forEachPlayer(const std::function<void(const PlayerPtr&)>& func) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (const auto& pair : m_playersById) {
        func(pair.second);
    }
}

void PlayerManager::clear() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_playersById.clear();
    m_playersByEntityId.clear();
    m_playerIdsByUsername.clear();
}

PlayerId PlayerManager::generatePlayerId() {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_nextPlayerId++;
}

EntityId PlayerManager::generateEntityId() {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_nextEntityId++;
}

} // namespace mc
