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

#include "server/player/movement/PlayerMovementManager.hpp"
#include "common/core/Types.hpp"
#include "common/util/math/Vector2.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/chunk/base/ChunkPos.hpp"
#include "server/player/lifecycle/PlayerManager.hpp"
#include <vector>

namespace mc::server {

PlayerMovementManager::PlayerMovementManager(PlayerManager& playerManager)
    : m_playerManager(playerManager)
{}

bool PlayerMovementManager::updatePosition(PlayerId playerId, f64 x, f64 y, f64 z, f32 yaw, f32 pitch, bool onGround)
{
    auto* player = m_playerManager.getPlayer(playerId);
    if (!player) {
        return false;
    }

    player->x = static_cast<f32>(x);
    player->y = static_cast<f32>(y);
    player->z = static_cast<f32>(z);
    player->yaw = yaw;
    player->pitch = pitch;
    player->onGround = onGround;

    // 更新区块同步管理器
    m_playerManager.chunkSyncManager().updatePlayerPosition(playerId, x, z);

    return true;
}

bool PlayerMovementManager::updatePosition(PlayerId playerId, f64 x, f64 y, f64 z)
{
    auto* player = m_playerManager.getPlayer(playerId);
    if (!player) {
        return false;
    }

    player->x = static_cast<f32>(x);
    player->y = static_cast<f32>(y);
    player->z = static_cast<f32>(z);

    // 更新区块同步管理器
    m_playerManager.chunkSyncManager().updatePlayerPosition(playerId, x, z);

    return true;
}

bool PlayerMovementManager::updateRotation(PlayerId playerId, f32 yaw, f32 pitch)
{
    auto* player = m_playerManager.getPlayer(playerId);
    if (!player) {
        return false;
    }

    player->yaw = yaw;
    player->pitch = pitch;
    return true;
}

Vector3f PlayerMovementManager::getPosition(PlayerId playerId) const
{
    auto* player = m_playerManager.getPlayer(playerId);
    return player ? Vector3f(player->x, player->y, player->z)
                  : Vector3f(0.0f, static_cast<f32>(world::SEA_LEVEL) + 1.0f, 0.0f);
}

Vector2f PlayerMovementManager::getRotation(PlayerId playerId) const
{
    auto* player = m_playerManager.getPlayer(playerId);
    return player ? Vector2f(player->yaw, player->pitch) : Vector2f(0.0f, 0.0f);
}

ChunkPos PlayerMovementManager::getChunkPosition(PlayerId playerId) const
{
    auto* player = m_playerManager.getPlayer(playerId);
    if (!player) {
        return ChunkPos(0, 0);
    }
    return ChunkPos(player->chunkX(), player->chunkZ());
}

bool PlayerMovementManager::isOnGround(PlayerId playerId) const
{
    auto* player = m_playerManager.getPlayer(playerId);
    return player ? player->onGround : true;
}

} // namespace mc::server
