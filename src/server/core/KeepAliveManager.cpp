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

#include "KeepAliveManager.hpp"
#include "PlayerManager.hpp"
#include "common/core/Constants.hpp"
#include "common/core/Types.hpp"
#include "server/core/ServerPlayerData.hpp"
#include <vector>

namespace mc::server::core {

KeepAliveManager::KeepAliveManager(PlayerManager& playerManager)
    : m_playerManager(playerManager)
{}

bool KeepAliveManager::needsKeepAlive(PlayerId playerId, u64 currentTickMs) const
{
    auto* player = m_playerManager.getPlayer(playerId);
    if (!player) return false;

    u64 lastSent = player->lastKeepAliveSent;
    return (currentTickMs - lastSent) >= network::KEEP_ALIVE_INTERVAL_MS;
}

std::vector<PlayerId> KeepAliveManager::getPlayersNeedingKeepAlive(u64 currentTickMs) const
{
    std::vector<PlayerId> result;
    result.reserve(m_playerManager.playerCount());
    m_playerManager.forEachPlayer([&](const ServerPlayerData& player) {
        u64 lastSent = player.lastKeepAliveSent;
        if ((currentTickMs - lastSent) >= network::KEEP_ALIVE_INTERVAL_MS) {
            result.push_back(player.playerId);
        }
    });
    return result;
}

void KeepAliveManager::recordKeepAliveSent(PlayerId playerId, u64 timestamp, u64 tick)
{
    auto* player = m_playerManager.getPlayer(playerId);
    if (!player) return;

    player->lastKeepAliveSent = timestamp;
    player->lastKeepAliveSentTick = tick;
}

void KeepAliveManager::handleKeepAliveResponse(PlayerId playerId, u64 timestamp, u64 currentTimeMs)
{
    auto* player = m_playerManager.getPlayer(playerId);
    if (!player) return;

    // 验证时间戳是否匹配
    if (player->lastKeepAliveSent != timestamp) {
        return;
    }

    player->lastKeepAliveReceived = currentTimeMs;

    // 计算 ping
    u32 ping = static_cast<u32>(currentTimeMs - timestamp);
    player->ping = ping;
}

void KeepAliveManager::updateKeepAlive(PlayerId playerId, u64 timestamp)
{
    auto* player = m_playerManager.getPlayer(playerId);
    if (!player) return;

    player->lastKeepAliveReceived = timestamp;
}

bool KeepAliveManager::isTimedOut(PlayerId playerId, u64 currentTickMs) const
{
    auto* player = m_playerManager.getPlayer(playerId);
    if (!player) return false;

    u64 lastReceived = player->lastKeepAliveReceived;
    return (currentTickMs - lastReceived) >= network::KEEP_ALIVE_TIMEOUT_MS;
}

std::vector<PlayerId> KeepAliveManager::getTimedOutPlayers(u64 currentTickMs) const
{
    std::vector<PlayerId> result;
    m_playerManager.forEachPlayer([&](const ServerPlayerData& player) {
        u64 lastReceived = player.lastKeepAliveReceived;
        if (lastReceived > 0 && (currentTickMs - lastReceived) >= network::KEEP_ALIVE_TIMEOUT_MS) {
            result.push_back(player.playerId);
        }
    });
    return result;
}

u32 KeepAliveManager::getPlayerPing(PlayerId playerId) const
{
    auto* player = m_playerManager.getPlayer(playerId);
    return player ? player->ping : 0;
}

u64 KeepAliveManager::getLastKeepAliveSent(PlayerId playerId) const
{
    auto* player = m_playerManager.getPlayer(playerId);
    return player ? player->lastKeepAliveSent : 0;
}

u64 KeepAliveManager::getLastKeepAliveReceived(PlayerId playerId) const
{
    auto* player = m_playerManager.getPlayer(playerId);
    return player ? player->lastKeepAliveReceived : 0;
}

} // namespace mc::server::core
