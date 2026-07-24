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

#pragma once

#include "ServerPlayerData.hpp"
#include "common/core/Types.hpp"
#include "common/network/connection/IServerConnection.hpp"
#include "common/network/sync/ChunkSync.hpp"
#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>

namespace mc::server::core {

/**
 * @brief 玩家管理器
 *
 * 负责玩家的注册、移除、查询、遍历等生命周期管理。
 * 线程安全：所有公共方法都是线程安全的。
 *
 * 使用示例：
 * @code
 * PlayerManager manager;
 * auto* player = manager.addPlayer(1, "Steve", connection);
 * manager.forEachPlayer([](ServerPlayerData& p) {
 *     spdlog::info("Player: {}", p.username);
 * });
 * @endcode
 */
class PlayerManager {
public:
    /**
     * @brief 构造玩家管理器
     */
    PlayerManager() = default;

    /**
     * @brief 构造玩家管理器（带最大玩家数）
     * @param maxPlayers 最大玩家数
     */
    explicit PlayerManager(i32 maxPlayers);

    // ========== 玩家生命周期 ==========

    /**
     * @brief 添加玩家
     * @param playerId 玩家ID（由调用方生成）
     * @param uuid 玩家UUID（持久化标识符）
     * @param username 用户名
     * @param connection 连接接口
     * @return 玩家数据指针，如果ID已存在则返回 nullptr
     * @note 线程安全
     */
    ServerPlayerData* addPlayer(PlayerId playerId,
        const std::string& uuid,
        const std::string& username,
        mc::server::net::ServerClientConnection* connection);

    /**
     * @brief 移除玩家
     * @param playerId 玩家ID
     * @note 线程安全
     */
    void removePlayer(PlayerId playerId);

    /**
     * @brief 根据会话ID移除玩家
     * @param sessionId 会话ID
     * @note 线程安全
     */
    void removePlayerBySessionId(u32 sessionId);

    /**
     * @brief 根据会话ID查找玩家
     * @param sessionId 会话ID
     * @return 玩家数据指针，如果未找到则返回 nullptr
     * @note 线程安全
     */
    [[nodiscard]] ServerPlayerData* findBySessionId(u32 sessionId);

    /**
     * @brief 根据会话ID查找玩家（const版本）
     * @param sessionId 会话ID
     * @return 玩家数据指针，如果未找到则返回 nullptr
     * @note 线程安全
     */
    [[nodiscard]] const ServerPlayerData* findBySessionId(u32 sessionId) const;

    // ========== 玩家查询 ==========

    /**
     * @brief 获取玩家数据
     * @param playerId 玩家ID
     * @return 玩家数据指针，如果未找到则返回 nullptr
     * @note 线程安全
     */
    [[nodiscard]] ServerPlayerData* getPlayer(PlayerId playerId);

    /**
     * @brief 获取玩家数据（const版本）
     * @param playerId 玩家ID
     * @return 玩家数据指针，如果未找到则返回 nullptr
     * @note 线程安全
     */
    [[nodiscard]] const ServerPlayerData* getPlayer(PlayerId playerId) const;

    /**
     * @brief 检查玩家是否存在
     * @param playerId 玩家ID
     * @return true 如果玩家存在
     * @note 线程安全
     */
    [[nodiscard]] bool hasPlayer(PlayerId playerId) const;

    /**
     * @brief 获取玩家数量
     * @return 当前玩家数量
     * @note 线程安全
     */
    [[nodiscard]] size_t playerCount() const;

    /**
     * @brief 检查是否已满
     * @return true 如果玩家数量已达上限
     * @note 线程安全
     */
    [[nodiscard]] bool isFull() const;

    // ========== 遍历 ==========

    /**
     * @brief 遍历所有玩家
     * @param func 对每个玩家调用的函数
     * @note 线程安全
     */
    template <typename Func>
    void forEachPlayer(Func&& func)
    {
        std::vector<PlayerId> playerIds;
        {
            std::lock_guard<std::recursive_mutex> lock(m_mutex);
            playerIds.reserve(m_players.size());
            for (const auto& [id, player] : m_players) {
                playerIds.push_back(id);
            }
        }

        for (PlayerId playerId : playerIds) {
            std::lock_guard<std::recursive_mutex> lock(m_mutex);
            auto it = m_players.find(playerId);
            if (it != m_players.end()) {
                func(it->second);
            }
        }
    }

    template <typename Func>
    void forEachPlayer(Func&& func) const
    {
        std::vector<PlayerId> playerIds;
        {
            std::lock_guard<std::recursive_mutex> lock(m_mutex);
            playerIds.reserve(m_players.size());
            for (const auto& [id, player] : m_players) {
                playerIds.push_back(id);
            }
        }

        for (PlayerId playerId : playerIds) {
            std::lock_guard<std::recursive_mutex> lock(m_mutex);
            auto it = m_players.find(playerId);
            if (it != m_players.end()) {
                func(it->second);
            }
        }
    }

    /**
     * @brief 获取所有玩家ID列表
     * @return 玩家ID列表
     * @note 线程安全
     */
    [[nodiscard]] std::vector<PlayerId> getPlayerIds() const;

    // ========== ID 生成 ==========

    /**
     * @brief 获取下一个玩家ID（线程安全）
     * @return 新的玩家ID
     */
    [[nodiscard]] PlayerId nextPlayerId() noexcept { return m_nextPlayerId.fetch_add(1); }

    /**
     * @brief 获取下一个会话ID（线程安全）
     * @return 新的会话ID
     */
    [[nodiscard]] u32 nextSessionId() noexcept { return m_nextSessionId.fetch_add(1); }

    // ========== 配置 ==========

    /**
     * @brief 设置最大玩家数
     * @param maxPlayers 最大玩家数
     */
    void setMaxPlayers(i32 maxPlayers) noexcept { m_maxPlayers = maxPlayers; }

    /**
     * @brief 获取最大玩家数
     */
    [[nodiscard]] i32 maxPlayers() const noexcept { return m_maxPlayers; }

    // ========== 区块同步 ==========

    /**
     * @brief 获取区块同步管理器
     */
    [[nodiscard]] network::ChunkSyncManager& chunkSyncManager() { return m_chunkSyncManager; }
    [[nodiscard]] const network::ChunkSyncManager& chunkSyncManager() const { return m_chunkSyncManager; }

    // ========== 会话映射 ==========

    /**
     * @brief 建立会话ID到玩家ID的映射
     * @param sessionId 会话ID
     * @param playerId 玩家ID
     * @note 线程安全
     */
    void mapSessionToPlayer(u32 sessionId, PlayerId playerId);

    /**
     * @brief 移除会话映射
     * @param sessionId 会话ID
     * @note 线程安全
     */
    void unmapSession(u32 sessionId);

    /**
     * @brief 通过会话ID查找玩家ID
     * @param sessionId 会话ID
     * @return 玩家ID，如果未找到返回 0
     * @note 线程安全
     */
    [[nodiscard]] PlayerId getPlayerIdBySession(u32 sessionId) const;

    /**
     * @brief 通过 IP 地址查找所有玩家ID
     * @param ipAddress IP 地址字符串
     * @return 匹配的玩家ID列表
     * @note 线程安全
     * @note 本地连接的玩家 IP 地址为空字符串
     */
    [[nodiscard]] std::vector<PlayerId> getPlayerIdsByAddress(const std::string& ipAddress) const;

    /**
     * @brief 通过用户名查找玩家（不区分大小写）
     * @param username 用户名
     * @return 玩家数据指针，如果未找到返回 nullptr
     * @note 线程安全
     */
    [[nodiscard]] ServerPlayerData* findByUsername(const std::string& username);

    /**
     * @brief 通过用户名查找玩家（不区分大小写，const版本）
     * @param username 用户名
     * @return 玩家数据指针，如果未找到返回 nullptr
     * @note 线程安全
     */
    [[nodiscard]] const ServerPlayerData* findByUsername(const std::string& username) const;

    /**
     * @brief 通过 UUID 查找玩家
     * @param uuid 玩家 UUID（持久化标识符）
     * @return 玩家数据指针，如果未找到返回 nullptr
     * @note 线程安全
     */
    [[nodiscard]] ServerPlayerData* findByUuid(const std::string& uuid);

    /**
     * @brief 通过 UUID 查找玩家（const版本）
     * @param uuid 玩家 UUID（持久化标识符）
     * @return 玩家数据指针，如果未找到返回 nullptr
     * @note 线程安全
     */
    [[nodiscard]] const ServerPlayerData* findByUuid(const std::string& uuid) const;

private:
    mutable std::recursive_mutex m_mutex;
    std::unordered_map<PlayerId, ServerPlayerData> m_players;
    std::unordered_map<u32, PlayerId> m_sessionToPlayer; ///< 会话ID -> 玩家ID

    std::atomic<PlayerId> m_nextPlayerId{1};
    std::atomic<u32> m_nextSessionId{1};
    i32 m_maxPlayers = 20;

    network::ChunkSyncManager m_chunkSyncManager;
};

} // namespace mc::server::core
