#include "ServerPlayer.hpp"

#include "common/network/packet/ProtocolPackets.hpp"
#include "../core/ConnectionManager.hpp"
#include <spdlog/spdlog.h>

namespace mc {

ServerPlayer::ServerPlayer(EntityId id, const String& name)
    : Player(id, name) {
}

void ServerPlayer::sendChatMessage(const String& message) {
    network::ChatMessagePacket chatPacket(message, static_cast<PlayerId>(id()));
    network::PacketSerializer payload;
    chatPacket.serialize(payload);

    const auto fullPacket = server::core::ConnectionManager::encapsulatePacket(
        network::PacketType::ChatBroadcast,
        payload.buffer());

    if (!sendFullPacket(fullPacket)) {
        spdlog::debug("ServerPlayer: chat message not sent (player={}, no connection)", username());
    }
}

void ServerPlayer::sendSystemMessage(const String& message) {
    network::ChatMessagePacket chatPacket(message, 0);
    network::PacketSerializer payload;
    chatPacket.serialize(payload);

    const auto fullPacket = server::core::ConnectionManager::encapsulatePacket(
        network::PacketType::ChatBroadcast,
        payload.buffer());

    if (!sendFullPacket(fullPacket)) {
        spdlog::debug("ServerPlayer: system message not sent (player={}, no connection)", username());
    }
}

void ServerPlayer::syncExperience() {
    const auto payloadResult = network::SetExperiencePacket::fromPlayer(*this).serialize();
    if (payloadResult.failed()) {
        spdlog::warn("ServerPlayer: failed to serialize experience packet (player={})", username());
        return;
    }

    const auto fullPacket = server::core::ConnectionManager::encapsulatePacket(
        network::PacketType::SetExperience,
        payloadResult.value());

    if (!sendFullPacket(fullPacket)) {
        spdlog::debug("ServerPlayer: experience sync skipped (player={}, no connection)", username());
    }
}

void ServerPlayer::addExperience(i32 amount) {
    Player::addExperience(amount);
    syncExperience();
}

void ServerPlayer::setExperienceLevel(i32 level) {
    Player::setExperienceLevel(level);
    syncExperience();
}

bool ServerPlayer::sendFullPacket(const std::vector<u8>& packet) const {
    if (!hasConnection()) {
        return false;
    }

    m_connection->send(packet.data(), packet.size());
    return true;
}

} // namespace mc