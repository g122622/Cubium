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

#include "TeleportManager.hpp"
#include "ConnectionManager.hpp"
#include "PlayerManager.hpp"
#include "common/network/ir/IrPacket.hpp"
#include "common/network/ir/packets/play/PlayPackets.hpp"
#include "common/network/protocol/ConnectionProtocol.hpp"
#include <spdlog/spdlog.h>

namespace mc::server::core {

TeleportManager::TeleportManager(PlayerManager& playerManager)
    : m_playerManager(playerManager)
{}

u32 TeleportManager::requestTeleport(PlayerId playerId, f64 x, f64 y, f64 z, f32 yaw, f32 pitch)
{
    auto* player = m_playerManager.getPlayer(playerId);
    if (!player) {
        spdlog::warn("TeleportManager: Player {} not found", playerId);
        return 0;
    }

    // 更新玩家位置
    player->x = static_cast<f32>(x);
    player->y = static_cast<f32>(y);
    player->z = static_cast<f32>(z);
    player->yaw = yaw;
    player->pitch = pitch;

    // 生成传送ID
    u32 teleportId = m_nextTeleportId++;
    player->pendingTeleportId = teleportId;
    player->waitingTeleportConfirm = true;

    // 发送传送包（绝对传送：relatives=0、deltas=0）
    mc::network::ir::play::PlayerPosition pos;
    pos.teleportId = static_cast<i32>(teleportId);
    pos.x = x;
    pos.y = y;
    pos.z = z;
    pos.deltaX = 0.0;
    pos.deltaY = 0.0;
    pos.deltaZ = 0.0;
    pos.yRot = yaw;
    pos.xRot = pitch;
    pos.relatives = 0;

    mc::network::ir::IrPacket packet{
        mc::network::protocol::ConnectionProtocol::Play,
        mc::network::ir::PlayPacket{std::move(pos)},
    };
    player->send(mc::network::ir::IrPacket{packet});

    return teleportId;
}

bool TeleportManager::confirmTeleport(PlayerId playerId, u32 teleportId)
{
    auto* player = m_playerManager.getPlayer(playerId);
    if (!player) {
        spdlog::warn("TeleportManager: Player {} not found for teleport confirm", playerId);
        return false;
    }

    if (!player->waitingTeleportConfirm) {
        return false;
    }

    if (player->pendingTeleportId != teleportId) {
        spdlog::warn("TeleportManager: Player {} teleport ID mismatch: expected {}, got {}",
            playerId,
            player->pendingTeleportId,
            teleportId);
        return false;
    }

    player->waitingTeleportConfirm = false;
    return true;
}

bool TeleportManager::isWaitingForConfirm(PlayerId playerId) const
{
    auto* player = m_playerManager.getPlayer(playerId);
    return player && player->waitingTeleportConfirm;
}

u32 TeleportManager::getPendingTeleportId(PlayerId playerId) const
{
    auto* player = m_playerManager.getPlayer(playerId);
    return player ? player->pendingTeleportId : 0;
}

} // namespace mc::server::core
