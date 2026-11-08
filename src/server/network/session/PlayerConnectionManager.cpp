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

#include "PlayerConnectionManager.hpp"
#include "server/player/lifecycle/PlayerManager.hpp"
#include <vector>

namespace mc::server {
PlayerConnectionManager::PlayerConnectionManager(PlayerManager& players)
    : m_players(players)
{}

void PlayerConnectionManager::disconnectPlayer(PlayerId id, const std::string& reason)
{
    if (auto* player = m_players.getPlayer(id)) {
        if (auto* connection = player->getConnection()) {
            connection->disconnect(reason);
        }
        m_players.removePlayer(id);
    }
}

void PlayerConnectionManager::disconnectAll(const std::string& reason)
{
    for (const PlayerId id : m_players.getPlayerIds()) {
        disconnectPlayer(id, reason);
    }
}

size_t PlayerConnectionManager::cleanupDisconnectedPlayers()
{
    std::vector<PlayerId> disconnected;
    m_players.forEachPlayer([&](const ServerPlayerData& player) {
        if (!player.hasConnection()) {
            disconnected.push_back(player.playerId);
        }
    });
    for (const PlayerId id : disconnected) {
        m_players.removePlayer(id);
    }
    return disconnected.size();
}
} // namespace mc::server
