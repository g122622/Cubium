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

#include "PlayerManager.hpp"
#include "common/core/Types.hpp"
#include "common/network/sync/ChunkSync.hpp"
#include "server/core/ServerPlayerData.hpp"
#include "server/network/IServerClientConnection.hpp"
#include <cctype>
#include <cstddef>
#include <memory>
#include <mutex>
#include <string>
#include <vector>
#include <spdlog/spdlog.h>

namespace mc::server::core {

PlayerManager::PlayerManager(i32 maxPlayers)
    : m_maxPlayers(maxPlayers)
{}

ServerPlayerData* PlayerManager::addPlayer(PlayerId playerId,
    const std::string& uuid,
    const std::string& username,
    mc::server::net::IServerClientConnection* connection)
{
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
    player.uuid = uuid;
    player.username = username;
    player.connection = connection;
    player.loggedIn = true;
    player.chunkTracker = std::make_shared<network::PlayerChunkTracker>(playerId);

    // IP 地址：Wire 模式从连接的 TCP 对端地址取（"host:port"），Local 模式（集成服本地
    // 客户端）无网络对端返回空串。供 BanIp 命令按 IP 踢人/封禁。connection 为非拥有指针，
    // 调用方保证在玩家生命周期内有效；此处空指针兜底（addPlayer 测试桩可能传 nullptr）。
    player.ipAddress = (connection != nullptr) ? connection->peerAddress() : std::string{};

    // 更新区块同步管理器
    (void)m_chunkSyncManager.getTracker(playerId);
    m_chunkSyncManager.updatePlayerPosition(playerId, player.x, player.z);

    return &player;
}

void PlayerManager::removePlayer(PlayerId playerId)
{
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
}

void PlayerManager::removePlayerBySessionId(u32 sessionId)
{
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
}

ServerPlayerData* PlayerManager::findBySessionId(u32 sessionId)
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    auto it = m_sessionToPlayer.find(sessionId);
    if (it == m_sessionToPlayer.end()) return nullptr;
    auto playerIt = m_players.find(it->second);
    return playerIt != m_players.end() ? &playerIt->second : nullptr;
}

const ServerPlayerData* PlayerManager::findBySessionId(u32 sessionId) const
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    auto it = m_sessionToPlayer.find(sessionId);
    if (it == m_sessionToPlayer.end()) return nullptr;
    auto playerIt = m_players.find(it->second);
    return playerIt != m_players.end() ? &playerIt->second : nullptr;
}

ServerPlayerData* PlayerManager::getPlayer(PlayerId playerId)
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    auto it = m_players.find(playerId);
    return it != m_players.end() ? &it->second : nullptr;
}

const ServerPlayerData* PlayerManager::getPlayer(PlayerId playerId) const
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    auto it = m_players.find(playerId);
    return it != m_players.end() ? &it->second : nullptr;
}

bool PlayerManager::hasPlayer(PlayerId playerId) const
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    return m_players.find(playerId) != m_players.end();
}

size_t PlayerManager::playerCount() const
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    return m_players.size();
}

bool PlayerManager::isFull() const
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    return static_cast<i32>(m_players.size()) >= m_maxPlayers;
}

std::vector<PlayerId> PlayerManager::getPlayerIds() const
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    std::vector<PlayerId> ids;
    ids.reserve(m_players.size());
    for (const auto& [id, player] : m_players) {
        ids.push_back(id);
    }
    return ids;
}

void PlayerManager::mapSessionToPlayer(u32 sessionId, PlayerId playerId)
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    m_sessionToPlayer[sessionId] = playerId;

    auto it = m_players.find(playerId);
    if (it != m_players.end()) {
        it->second.sessionId = sessionId;
    }
}

void PlayerManager::unmapSession(u32 sessionId)
{
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

PlayerId PlayerManager::getPlayerIdBySession(u32 sessionId) const
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    auto it = m_sessionToPlayer.find(sessionId);
    return it != m_sessionToPlayer.end() ? it->second : 0;
}

std::vector<PlayerId> PlayerManager::getPlayerIdsByAddress(const std::string& ipAddress) const
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    std::vector<PlayerId> result;
    for (const auto& [id, player] : m_players) {
        if (player.ipAddress == ipAddress) {
            result.push_back(id);
        }
    }
    return result;
}

ServerPlayerData* PlayerManager::findByUsername(const std::string& username)
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    for (auto& [id, player] : m_players) {
        // 不区分大小写比较
        if (player.username.size() == username.size()) {
            bool match = true;
            for (size_t i = 0; i < username.size(); ++i) {
                if (std::tolower(player.username[i]) != std::tolower(username[i])) {
                    match = false;
                    break;
                }
            }
            if (match) {
                return &player;
            }
        }
    }
    return nullptr;
}

const ServerPlayerData* PlayerManager::findByUsername(const std::string& username) const
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    for (const auto& [id, player] : m_players) {
        // 不区分大小写比较
        if (player.username.size() == username.size()) {
            bool match = true;
            for (size_t i = 0; i < username.size(); ++i) {
                if (std::tolower(player.username[i]) != std::tolower(username[i])) {
                    match = false;
                    break;
                }
            }
            if (match) {
                return &player;
            }
        }
    }
    return nullptr;
}

ServerPlayerData* PlayerManager::findByUuid(const std::string& uuid)
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    for (auto& [id, player] : m_players) {
        if (player.uuid == uuid) {
            return &player;
        }
    }
    return nullptr;
}

const ServerPlayerData* PlayerManager::findByUuid(const std::string& uuid) const
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    for (const auto& [id, player] : m_players) {
        if (player.uuid == uuid) {
            return &player;
        }
    }
    return nullptr;
}

} // namespace mc::server::core
