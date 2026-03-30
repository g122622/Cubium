#pragma once

#include "Player.hpp"
#include "../../../core/Result.hpp"
#include <unordered_map>
#include <vector>
#include <memory>
#include <functional>
#include <mutex>

namespace mc {

// ============================================================================
// 玩家管理器
// ============================================================================

class PlayerManager {
public:
    using PlayerPtr = std::shared_ptr<Player>;
    using PlayerMap = std::unordered_map<PlayerId, PlayerPtr>;
    using EntityMap = std::unordered_map<EntityId, PlayerPtr>;

    PlayerManager() = default;
    ~PlayerManager() = default;

    // 禁止拷贝
    PlayerManager(const PlayerManager&) = delete;
    PlayerManager& operator=(const PlayerManager&) = delete;

    // 玩家管理
    [[nodiscard]] PlayerPtr createPlayer(const String& username);
    [[nodiscard]] PlayerPtr createPlayer(PlayerId id, const String& username);
    void removePlayer(PlayerId id);
    void removePlayer(const PlayerPtr& player);

    // 玩家查询
    [[nodiscard]] PlayerPtr getPlayer(PlayerId id) const;
    [[nodiscard]] PlayerPtr getPlayerByUsername(const String& username) const;
    [[nodiscard]] PlayerPtr getPlayerByEntityId(EntityId id) const;

    [[nodiscard]] bool hasPlayer(PlayerId id) const;
    [[nodiscard]] bool hasUsername(const String& username) const;

    [[nodiscard]] size_t playerCount() const;
    [[nodiscard]] std::vector<PlayerPtr> getAllPlayers() const;

    // 遍历
    void forEachPlayer(const std::function<void(const PlayerPtr&)>& func) const;

    // 广播
    template<typename Packet>
    void broadcast(const Packet& packet, const std::function<void(const PlayerPtr&, const Packet&)>& sendFunc);

    // 广播排除特定玩家
    template<typename Packet>
    void broadcastExcept(const Packet& packet, PlayerId excludeId,
                         const std::function<void(const PlayerPtr&, const Packet&)>& sendFunc);

    // 清空
    void clear();

    // ID生成
    [[nodiscard]] PlayerId generatePlayerId();
    [[nodiscard]] EntityId generateEntityId();

    // 最大玩家数
    static constexpr size_t MAX_PLAYERS = 256;

private:
    mutable std::mutex m_mutex;
    PlayerMap m_playersById;
    EntityMap m_playersByEntityId;
    std::unordered_map<String, PlayerId> m_playerIdsByUsername;

    PlayerId m_nextPlayerId = 1;
    EntityId m_nextEntityId = 1000;  // 实体ID从1000开始
};

// ============================================================================
// 模板实现
// ============================================================================

template<typename Packet>
void PlayerManager::broadcast(const Packet& packet,
                               const std::function<void(const PlayerPtr&, const Packet&)>& sendFunc) {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (const auto& pair : m_playersById) {
        sendFunc(pair.second, packet);
    }
}

template<typename Packet>
void PlayerManager::broadcastExcept(const Packet& packet, PlayerId excludeId,
                                     const std::function<void(const PlayerPtr&, const Packet&)>& sendFunc) {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (const auto& pair : m_playersById) {
        if (pair.first != excludeId) {
            sendFunc(pair.second, packet);
        }
    }
}

} // namespace mc
