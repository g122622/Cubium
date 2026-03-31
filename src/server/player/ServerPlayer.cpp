#include "ServerPlayer.hpp"
#include "../../common/network/packet/PacketSerializer.hpp"
#include "../core/ServerPlayerData.hpp"
#include "../core/PlayerManager.hpp"
#include "../application/IServer.hpp"
#include <spdlog/spdlog.h>

namespace mc {

ServerPlayer::ServerPlayer(EntityId id, const String& name)
    : Player(id, name)
{
}

void ServerPlayer::sendChatMessage(const String& message) {
    // TODO: 实现发送聊天消息给玩家
    spdlog::info("[Chat -> {}] {}", username(), message);
}

void ServerPlayer::sendSystemMessage(const String& message) {
    // TODO: 实现发送系统消息给玩家
    spdlog::info("[System -> {}] {}", username(), message);
}

void ServerPlayer::syncExperience() {
    // 创建经验同步包
    network::SetExperiencePacket packet;
    packet.setExperience(
        experienceManager().getProgress(),
        experienceManager().getTotalExperience(),
        experienceManager().getLevel()
    );

    auto result = packet.serialize();
    if (result.success()) {
        // 封装为完整数据包
        network::PacketSerializer fullPacket;
        fullPacket.writeU32(static_cast<u32>(network::PACKET_HEADER_SIZE + result.value().size()));
        fullPacket.writeU16(static_cast<u16>(network::PacketType::SetExperience));
        fullPacket.writeU16(0);
        fullPacket.writeU16(0);
        fullPacket.writeU16(0);
        fullPacket.writeBytes(result.value());

        // TODO: 通过玩家连接发送数据包
        // 目前记录日志
        spdlog::debug("ServerPlayer: Sync experience to client - level={}, progress={}, total={}",
                      experienceManager().getLevel(),
                      experienceManager().getProgress(),
                      experienceManager().getTotalExperience());
    }
}

void ServerPlayer::addExperience(i32 amount) {
    // 调用父类方法
    Player::addExperience(amount);

    // 同步到客户端
    syncExperience();
}

void ServerPlayer::setExperienceLevel(i32 level) {
    // 调用父类方法
    Player::setExperienceLevel(level);

    // 同步到客户端
    syncExperience();
}

} // namespace mc
