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
#include "common/core/Types.hpp"
#include "common/network/ir/IrPacket.hpp"
#include "common/network/ir/packets/play/PlayPackets.hpp"
#include "common/util/UuidUtils.hpp"
#include "server/core/ServerPlayerData.hpp"
#include <array>
#include <cstddef>
#include <optional>
#include <string>
#include <utility>
#include <vector>
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

    // 离场广播用的 UUID 必须在 removePlayer 之前取——之后 ServerPlayerData 即被销毁。
    // 判据用 uuid 非空而不用 hasConnection()：被踢时连接可能已先行断开（如 KeepAlive 超时后
    // socket 已关闭），hasConnection() 会因 isConnected() 为假而漏掉离场广播。
    std::optional<std::array<u8, 16>> departedUuid;
    if (!player->uuid.empty()) {
        departedUuid = util::uuidFromString(player->uuid);
    }

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

    // 广播离场（ClientboundPlayerInfoRemove，cb 67），否则其余客户端的 Tab 列表会永久残留该玩家。
    // /kick、/ban、白名单拒绝、KeepAlive 超时踢出都走本路径，不补这一包这些场景全会残留。
    // disconnectAll（关服）不广播——对所有玩家踢出而言没有接收方。
    if (departedUuid.has_value()) {
        mc::network::ir::play::PlayerInfoRemove removal;
        removal.uuids.push_back(*departedUuid);
        broadcastExcept(playerId,
            mc::network::ir::IrPacket{
                mc::network::protocol::ConnectionProtocol::Play, mc::network::ir::PlayPacket{removal}});
    }
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

size_t ConnectionManager::cleanupDisconnectedPlayers()
{
    std::vector<PlayerId> disconnected;
    disconnected.reserve(m_playerManager.playerCount());

    m_playerManager.forEachPlayer([&](const ServerPlayerData& player) {
        if (!player.hasConnection()) {
            disconnected.push_back(player.playerId);
        }
    });

    // 分两步：先收集再移除，避免在遍历 m_players 的过程中修改它。
    for (PlayerId playerId : disconnected) {
        m_playerManager.removePlayer(playerId);
    }

    return disconnected.size();
}

} // namespace mc::server::core
