#include "TeleportManager.hpp"
#include "ConnectionManager.hpp"
#include "PlayerManager.hpp"
#include "common/network/packet/ProtocolPackets.hpp"
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

    // 发送传送包
    network::TeleportPacket teleportPacket(x, y, z, yaw, pitch, teleportId);
    network::PacketSerializer ser;
    teleportPacket.serialize(ser);

    auto packet = ConnectionManager::encapsulatePacket(network::PacketType::Teleport, ser.buffer());
    player->send(packet.data(), packet.size());

    spdlog::debug(
        "TeleportManager: Player {} teleporting to ({}, {}, {}), teleportId={}", playerId, x, y, z, teleportId);

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
        spdlog::debug("TeleportManager: Player {} not waiting for teleport confirm", playerId);
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
    spdlog::debug("TeleportManager: Player {} confirmed teleport {}", playerId, teleportId);
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
