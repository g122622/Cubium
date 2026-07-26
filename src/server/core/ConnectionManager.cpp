/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
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

#include "ConnectionManager.hpp"
#include "PlayerManager.hpp"
#include <spdlog/spdlog.h>

namespace mc::server::core {

ConnectionManager::ConnectionManager(PlayerManager& playerManager)
    : m_playerManager(playerManager)
{}

bool ConnectionManager::sendToPlayer(PlayerId playerId, const mc::network::ir::IrPacket& packet)
{
    auto* player = m_playerManager.getPlayer(playerId);
    if (!player) {
        return false;
    }
    // 复制一份：单玩家发送直接移动，但仍需保留原包供调用方
    return player->send(mc::network::ir::IrPacket{packet});
}

void ConnectionManager::broadcast(const mc::network::ir::IrPacket& packet)
{
    m_playerManager.forEachPlayer([&](ServerPlayerData& player) { player.send(mc::network::ir::IrPacket{packet}); });
}

void ConnectionManager::broadcastExcept(PlayerId excludePlayerId, const mc::network::ir::IrPacket& packet)
{
    m_playerManager.forEachPlayer([&](ServerPlayerData& player) {
        if (player.playerId != excludePlayerId) {
            player.send(mc::network::ir::IrPacket{packet});
        }
    });
}

void ConnectionManager::disconnectPlayer(PlayerId playerId, const std::string& reason)
{
    auto* player = m_playerManager.getPlayer(playerId);
    if (!player) return;

    auto* conn = player->getConnection();
    if (conn) {
        conn->disconnect(reason);
    }

    if (reason.empty()) {
        spdlog::info("Player {} ({}) disconnected", player->username, playerId);
    } else {
        spdlog::info("Player {} ({}) disconnected: {}", player->username, playerId, reason);
    }

    m_playerManager.removePlayer(playerId);
}

void ConnectionManager::disconnectAll(const std::string& reason)
{
    // 先收集所有需要断开的连接，避免在遍历时修改
    std::vector<std::pair<PlayerId, std::string>> toDisconnect;
    m_playerManager.forEachPlayer(
        [&](ServerPlayerData& player) { toDisconnect.emplace_back(player.playerId, player.username); });

    for (const auto& [playerId, username] : toDisconnect) {
        auto* player = m_playerManager.getPlayer(playerId);
        if (!player) continue;

        auto* conn = player->getConnection();
        if (conn) {
            conn->disconnect(reason);
        }

        if (reason.empty()) {
            spdlog::info("Player {} ({}) disconnected", username, playerId);
        } else {
            spdlog::info("Player {} ({}) disconnected: {}", username, playerId, reason);
        }
    }

    // 清理所有玩家
    for (const auto& [playerId, _] : toDisconnect) {
        m_playerManager.removePlayer(playerId);
    }
}

size_t ConnectionManager::cleanupDisconnectedPlayers(std::vector<PlayerId>* removedPlayers)
{
    std::vector<PlayerId> toRemove;
    toRemove.reserve(m_playerManager.playerCount());

    m_playerManager.forEachPlayer([&](ServerPlayerData& player) {
        if (!player.hasConnection()) {
            toRemove.push_back(player.playerId);
        }
    });

    for (PlayerId playerId : toRemove) {
        m_playerManager.removePlayer(playerId);
    }

    if (removedPlayers) {
        *removedPlayers = toRemove;
    }

    return toRemove.size();
}

} // namespace mc::server::core
