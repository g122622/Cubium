#include "PacketHandler.hpp"
#include "PlayerManager.hpp"
#include "ConnectionManager.hpp"
#include "TeleportManager.hpp"
#include "KeepAliveManager.hpp"
#include "PositionTracker.hpp"
#include "TimeManager.hpp"
#include "common/network/packet/PacketSerializer.hpp"
#include "common/network/packet/ProtocolPackets.hpp"
#include "common/network/packet/EntityPackets.hpp"
#include "common/util/TimeUtils.hpp"
#include <spdlog/spdlog.h>

namespace mc::server::core {

PacketHandler::PacketHandler(PlayerManager& playerManager,
                              ConnectionManager& connectionManager,
                              TeleportManager& teleportManager,
                              KeepAliveManager& keepAliveManager,
                              PositionTracker& positionTracker,
                              TimeManager& timeManager,
                              const ServerCoreConfig& config)
    : m_playerManager(playerManager)
    , m_connectionManager(connectionManager)
    , m_teleportManager(teleportManager)
    , m_keepAliveManager(keepAliveManager)
    , m_positionTracker(positionTracker)
    , m_timeManager(timeManager)
    , m_config(config)
{
}

PacketHandleResult PacketHandler::handlePacket(u32 sessionId, const u8* data, size_t size) {
    if (size < network::PACKET_HEADER_SIZE) {
        spdlog::warn("PacketHandler: Packet too small from session {}", sessionId);
        return PacketHandleResult::Error;
    }

    // 解析包头
    network::PacketDeserializer deser(data, size);
    auto sizeResult = deser.readU32();
    auto typeResult = deser.readU16();

    if (sizeResult.failed() || typeResult.failed()) {
        spdlog::warn("PacketHandler: Failed to read packet header from session {}", sessionId);
        return PacketHandleResult::Error;
    }

    network::PacketType packetType = static_cast<network::PacketType>(typeResult.value());
    const u8* payload = data + network::PACKET_HEADER_SIZE;
    size_t payloadSize = size - network::PACKET_HEADER_SIZE;

    // 根据数据包类型分发处理
    switch (packetType) {
        case network::PacketType::LoginRequest:
            // 登录请求需要特殊处理（需要连接接口）
            spdlog::warn("PacketHandler: LoginRequest should be handled separately");
            return PacketHandleResult::Ignore;

        case network::PacketType::PlayerMove:
            return handlePlayerMove(sessionId, payload, payloadSize);

        case network::PacketType::PlayerInput:
            return handlePlayerInput(sessionId, payload, payloadSize);

        case network::PacketType::MoveVehicle:
            return handleMoveVehicle(sessionId, payload, payloadSize);

        case network::PacketType::EntityAction:
            return handleEntityAction(sessionId, payload, payloadSize);

        case network::PacketType::TeleportConfirm:
            return handleTeleportConfirm(sessionId, payload, payloadSize);

        case network::PacketType::KeepAlive: {
            u64 currentTimeMs = util::TimeUtils::getCurrentTimeMs();
            return handleKeepAlive(sessionId, data, size, currentTimeMs);
        }

        case network::PacketType::ChatMessage:
            return handleChatMessage(sessionId, payload, payloadSize);

        default:
            spdlog::trace("PacketHandler: Unhandled packet type {} from session {}",
                         static_cast<int>(packetType), sessionId);
            return PacketHandleResult::Ignore;
    }
}

LoginResult PacketHandler::handleLoginRequest(u32 sessionId, network::ConnectionPtr connection,
                                               const u8* data, size_t size) {
    LoginResult result;

    network::PacketDeserializer deser(data, size);
    auto packetResult = network::LoginRequestPacket::deserialize(deser);

    if (packetResult.failed()) {
        spdlog::error("PacketHandler: Failed to parse login request from session {}", sessionId);
        result.message = "Invalid login request";
        return result;
    }

    auto& packet = packetResult.value();
    String username = packet.username();

    spdlog::info("PacketHandler: Player '{}' attempting to join from session {}", username, sessionId);

    // 检查服务器是否已满
    if (m_playerManager.isFull()) {
        result.message = "Server is full";
        if (m_onLoginFail) {
            m_onLoginFail(sessionId, result.message);
        }
        return result;
    }

    // 分配玩家ID
    PlayerId playerId = m_playerManager.nextPlayerId();

    // 添加玩家
    auto* player = m_playerManager.addPlayer(playerId, username, connection);
    if (!player) {
        result.message = "Failed to add player";
        if (m_onLoginFail) {
            m_onLoginFail(sessionId, result.message);
        }
        return result;
    }

    // 建立会话映射
    m_playerManager.mapSessionToPlayer(sessionId, playerId);

    // 设置初始状态
    player->gameMode = m_config.defaultGameMode;
    player->loggedIn = true;

    result.success = true;
    result.playerId = playerId;
    result.username = username;
    result.message = "Welcome!";

    spdlog::info("PacketHandler: Player '{}' (ID: {}) logged in", username, playerId);

    if (m_onLoginSuccess) {
        m_onLoginSuccess(playerId, username);
    }

    return result;
}

PacketHandleResult PacketHandler::handlePlayerMove(u32 sessionId, const u8* data, size_t size) {
    PlayerId playerId = m_playerManager.getPlayerIdBySession(sessionId);
    if (playerId == 0) {
        spdlog::trace("PacketHandler: Player move from unknown session {}", sessionId);
        return PacketHandleResult::Ignore;
    }

    network::PacketDeserializer deser(data, size);
    auto result = network::PlayerMovePacket::deserialize(deser);

    if (result.failed()) {
        spdlog::error("PacketHandler: Failed to parse player move from player {}", playerId);
        return PacketHandleResult::Error;
    }

    auto& packet = result.value();
    const auto& pos = packet.position();

    m_positionTracker.updatePosition(playerId, pos.x, pos.y, pos.z,
                                      pos.yaw, pos.pitch, pos.onGround);

    return PacketHandleResult::Success;
}

PacketHandleResult PacketHandler::handlePlayerInput(u32 sessionId, const u8* data, size_t size) {
    PlayerId playerId = m_playerManager.getPlayerIdBySession(sessionId);
    if (playerId == 0) {
        spdlog::trace("PacketHandler: Player input from unknown session {}", sessionId);
        return PacketHandleResult::Ignore;
    }

    network::PacketDeserializer deser(data, size);
    auto result = network::PlayerInputPacket::deserialize(deser);

    if (result.failed()) {
        spdlog::error("PacketHandler: Failed to parse player input from player {}", playerId);
        return PacketHandleResult::Error;
    }

    auto& packet = result.value();

    // MC 1.16.5: PlayerInputPacket 用于控制骑乘中的载具
    // strafeSpeed: 左右移动（正值=左，负值=右）
    // forwardSpeed: 前后移动（正值=前，负值=后）
    // jumping: 是否跳跃
    // sneaking: 是否潜行（下马）

    // TODO: 将输入传递给玩家骑乘的载具
    // 这需要通过玩家ID获取玩家实体，然后获取其骑乘的载具
    spdlog::trace("PacketHandler: Player {} input: strafe={:.2f}, forward={:.2f}, jump={}, sneak={}",
                  playerId, packet.strafeSpeed(), packet.forwardSpeed(),
                  packet.isJumping(), packet.isSneaking());

    return PacketHandleResult::Success;
}

PacketHandleResult PacketHandler::handleMoveVehicle(u32 sessionId, const u8* data, size_t size) {
    PlayerId playerId = m_playerManager.getPlayerIdBySession(sessionId);
    if (playerId == 0) {
        spdlog::trace("PacketHandler: Move vehicle from unknown session {}", sessionId);
        return PacketHandleResult::Ignore;
    }

    network::PacketDeserializer deser(data, size);
    auto result = network::MoveVehiclePacket::deserialize(deser);

    if (result.failed()) {
        spdlog::error("PacketHandler: Failed to parse move vehicle from player {}", playerId);
        return PacketHandleResult::Error;
    }

    auto& packet = result.value();

    // MC 1.16.5: MoveVehiclePacket 由客户端发送以同步载具位置
    // 服务端需要验证位置并将更新广播给其他玩家

    // TODO: 验证并更新载具位置
    spdlog::trace("PacketHandler: Player {} move vehicle: ({:.2f}, {:.2f}, {:.2f}) yaw={:.1f} pitch={:.1f}",
                  playerId, packet.x(), packet.y(), packet.z(), packet.yaw(), packet.pitch());

    return PacketHandleResult::Success;
}

PacketHandleResult PacketHandler::handleEntityAction(u32 sessionId, const u8* data, size_t size) {
    PlayerId playerId = m_playerManager.getPlayerIdBySession(sessionId);
    if (playerId == 0) {
        spdlog::trace("PacketHandler: Entity action from unknown session {}", sessionId);
        return PacketHandleResult::Ignore;
    }

    network::PacketDeserializer deser(data, size);
    auto result = network::EntityActionPacket::deserialize(deser);

    if (result.failed()) {
        spdlog::error("PacketHandler: Failed to parse entity action from player {}", playerId);
        return PacketHandleResult::Error;
    }

    auto& packet = result.value();

    // MC 1.16.5: EntityActionPacket 用于实体动作
    // - PressShiftKey: 按下潜行键（下马）
    // - ReleaseShiftKey: 释放潜行键
    // - StartRidingJump: 开始马跳跃蓄力
    // - StopRidingJump: 停止马跳跃蓄力（释放跳跃）
    // - StartSprinting: 开始疾跑
    // - StopSprinting: 停止疾跑

    spdlog::trace("PacketHandler: Player {} entity action: {} aux={}",
                  playerId, static_cast<i32>(packet.action()), packet.auxData());

    return PacketHandleResult::Success;
}

PacketHandleResult PacketHandler::handleTeleportConfirm(u32 sessionId, const u8* data, size_t size) {
    PlayerId playerId = m_playerManager.getPlayerIdBySession(sessionId);
    if (playerId == 0) {
        spdlog::trace("PacketHandler: Teleport confirm from unknown session {}", sessionId);
        return PacketHandleResult::Ignore;
    }

    network::PacketDeserializer deser(data, size);
    auto result = network::TeleportConfirmPacket::deserialize(deser);

    if (result.failed()) {
        spdlog::error("PacketHandler: Failed to parse teleport confirm from player {}", playerId);
        return PacketHandleResult::Error;
    }

    auto& packet = result.value();

    if (!m_teleportManager.confirmTeleport(playerId, packet.teleportId())) {
        spdlog::error("PacketHandler: Teleport confirm failed for player {}", playerId);
        return PacketHandleResult::Error;
    }

    return PacketHandleResult::Success;
}

PacketHandleResult PacketHandler::handleKeepAlive(u32 sessionId, const u8* data, size_t size, u64 currentTimeMs) {
    PlayerId playerId = m_playerManager.getPlayerIdBySession(sessionId);
    if (playerId == 0) {
        spdlog::error("PacketHandler: Keepalive from unknown session {}", sessionId);
        return PacketHandleResult::Ignore;
    }

    network::KeepAlivePacket packet;
    auto result = packet.deserialize(data, size);

    if (result.failed()) {
        spdlog::error("PacketHandler: Failed to parse keepalive from player {}", playerId);
        return PacketHandleResult::Error;
    }

    m_keepAliveManager.handleKeepAliveResponse(playerId, packet.timestamp(), currentTimeMs);

    return PacketHandleResult::Success;
}

PacketHandleResult PacketHandler::handleChatMessage(u32 sessionId, const u8* data, size_t size) {
    PlayerId playerId = m_playerManager.getPlayerIdBySession(sessionId);
    if (playerId == 0) {
        spdlog::error("PacketHandler: Chat message from unknown session {}", sessionId);
        return PacketHandleResult::Ignore;
    }

    auto* player = m_playerManager.getPlayer(playerId);
    if (!player) {
        spdlog::error("PacketHandler: Chat message from unknown player {}", playerId);
        return PacketHandleResult::Ignore;
    }

    network::PacketDeserializer deser(data, size);
    auto result = network::ChatMessagePacket::deserialize(deser);

    if (result.failed()) {
        spdlog::error("PacketHandler: Failed to parse chat message from player {}", playerId);
        return PacketHandleResult::Error;
    }

    auto& packet = result.value();
    String message = packet.message();

    spdlog::info("[Chat] {}: {}", player->username, message);

    if (m_onChat) {
        m_onChat(playerId, player->username, message);
    }

    return PacketHandleResult::Success;
}

} // namespace mc::server::core
