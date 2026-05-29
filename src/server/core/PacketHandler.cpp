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

#include "PacketHandler.hpp"
#include "ConnectionManager.hpp"
#include "KeepAliveManager.hpp"
#include "PlayerManager.hpp"
#include "PositionTracker.hpp"
#include "TeleportManager.hpp"
#include "TimeManager.hpp"
#include "common/advancement/trigger/CriterionTriggers.hpp"
#include "common/advancement/trigger/impl/EntityTriggers.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/entities/vehicle/BoatEntity.hpp"
#include "common/entity/interfaces/IJumpingMount.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/network/packet/EntityPackets.hpp"
#include "common/network/packet/PacketSerializer.hpp"
#include "common/network/packet/ProtocolPackets.hpp"
#include "common/util/TimeUtils.hpp"
#include "common/util/UuidUtils.hpp"
#include "server/advancement/PlayerAdvancements.hpp"
#include "server/advancement/TriggerInstantiation.hpp"
#include "server/application/IServer.hpp"
#include "server/dimension/ServerDimensionManager.hpp"
#include "server/player/ServerPlayer.hpp"
#include "server/world/ServerWorld.hpp"
#include "server/world/player/ServerPlayerEntityManager.hpp"
#include <algorithm>
#include <spdlog/spdlog.h>

namespace mc::server::core {

PacketHandler::PacketHandler(PlayerManager& playerManager,
    ConnectionManager& connectionManager,
    TeleportManager& teleportManager,
    KeepAliveManager& keepAliveManager,
    PositionTracker& positionTracker,
    TimeManager& timeManager,
    GameMode defaultGameMode)
    : m_playerManager(playerManager)
    , m_connectionManager(connectionManager)
    , m_teleportManager(teleportManager)
    , m_keepAliveManager(keepAliveManager)
    , m_positionTracker(positionTracker)
    , m_timeManager(timeManager)
    , m_defaultGameMode(defaultGameMode)
{}

PacketHandleResult PacketHandler::handlePacket(u32 sessionId, const u8* data, size_t size)
{
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

        case network::PacketType::SteerBoat:
            return handleSteerBoat(sessionId, payload, payloadSize);

        case network::PacketType::UseEntity:
            return handleUseEntity(sessionId, payload, payloadSize);

        case network::PacketType::TeleportConfirm:
            return handleTeleportConfirm(sessionId, payload, payloadSize);

        case network::PacketType::KeepAlive: {
            u64 currentTimeMs = util::TimeUtils::getCurrentTimeMs();
            return handleKeepAlive(sessionId, data, size, currentTimeMs);
        }

        case network::PacketType::ChatMessage:
            return handleChatMessage(sessionId, payload, payloadSize);

        default:
            spdlog::error(
                "PacketHandler: Unhandled packet type {} from session {}", static_cast<int>(packetType), sessionId);
            return PacketHandleResult::Ignore;
    }
}

LoginResult PacketHandler::handleLoginRequest(
    u32 sessionId, network::ConnectionPtr connection, const u8* data, size_t size)
{
    LoginResult result;

    network::PacketDeserializer deser(data, size);
    auto packetResult = network::LoginRequestPacket::deserialize(deser);

    if (packetResult.failed()) {
        spdlog::error("PacketHandler: Failed to parse login request from session {}", sessionId);
        result.message = "Invalid login request";
        return result;
    }

    auto& packet = packetResult.value();
    std::string username = packet.username();

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

    // 生成离线模式 UUID（基于用户名）
    // 参考 MC 1.16.5: UUID.nameUUIDFromBytes(("OfflinePlayer:" + username).getBytes(UTF_8))
    Uuid offlineUuid = util::generateOfflineUuid(username);
    std::string uuidStr = util::uuidToString(offlineUuid);

    // 添加玩家
    auto* player = m_playerManager.addPlayer(playerId, uuidStr, username, connection);
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
    player->gameMode = m_defaultGameMode;
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

PacketHandleResult PacketHandler::handlePlayerMove(u32 sessionId, const u8* data, size_t size)
{
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

    m_positionTracker.updatePosition(playerId, pos.x, pos.y, pos.z, pos.yaw, pos.pitch, pos.onGround);

    return PacketHandleResult::Success;
}

PacketHandleResult PacketHandler::handlePlayerInput(u32 sessionId, const u8* data, size_t size)
{
    PlayerId playerId = m_playerManager.getPlayerIdBySession(sessionId);
    if (playerId == 0) {
        // spdlog::trace("PacketHandler: Player input from unknown session {}", sessionId);
        return PacketHandleResult::Ignore;
    }

    network::PlayerInputPacket packet;
    auto result = packet.deserialize(data, size);

    if (result.failed()) {
        spdlog::error("PacketHandler: Failed to parse player input from player {}", playerId);
        return PacketHandleResult::Error;
    }

    // MC 1.16.5: PlayerInputPacket 用于控制骑乘中的载具
    // strafeSpeed: 左右移动（正值=左，负值=右）
    // forwardSpeed: 前后移动（正值=前，负值=后）
    // jumping: 是否跳跃
    // sneaking: 是否潜行（下马）

    // spdlog::trace("PacketHandler: Player {} input: strafe={:.2f}, forward={:.2f}, jump={}, sneak={}",
    //     playerId,
    //     packet.strafeSpeed(),
    //     packet.forwardSpeed(),
    //     packet.isJumping(),
    //     packet.isSneaking());

    // MC 1.16.5: ServerPlayNetHandler.processInput()
    // 将输入传递给玩家骑乘的载具
    if (m_server == nullptr) {
        spdlog::error("PacketHandler: Server not set, cannot process player input");
        return PacketHandleResult::Success;
    }

    // 获取玩家实体
    ServerWorld* world = m_server->getPlayerWorld(playerId);
    if (world == nullptr) {
        spdlog::error("PacketHandler: Player {} world not found", playerId);
        return PacketHandleResult::Success;
    }
    ServerPlayerEntityManager& entityManager = m_server->playerEntityManager();
    Player* player = entityManager.getPlayerEntity(playerId, *world);

    if (player == nullptr) {
        spdlog::error("PacketHandler: Player {} entity not found", playerId);
        return PacketHandleResult::Success;
    }

    // 检查玩家是否正在骑乘
    if (!player->isRiding()) {
        return PacketHandleResult::Success;
    }

    // MC 1.16.5: ServerPlayerEntity.setEntityActionState()
    // 只有在骑乘时才更新移动状态
    f32 strafe = packet.strafeSpeed();
    f32 forward = packet.forwardSpeed();

    // 限制输入范围 [-1.0, 1.0]
    strafe = std::clamp(strafe, -1.0f, 1.0f);
    forward = std::clamp(forward, -1.0f, 1.0f);

    // 设置玩家的移动状态（会传递给载具）
    player->setMoveStrafing(strafe);
    player->setMoveForward(forward);
    player->setJumping(packet.isJumping());
    player->setSneaking(packet.isSneaking());

    // 获取载具实体
    EntityId vehicleId = player->getVehicle();
    if (vehicleId == INVALID_ENTITY_ID) {
        return PacketHandleResult::Success;
    }

    Entity* vehicle = world->getEntity(vehicleId);
    if (vehicle == nullptr) {
        spdlog::trace("PacketHandler: Vehicle entity {} not found for player {}", vehicleId, playerId);
        return PacketHandleResult::Success;
    }

    // 检查玩家是否是载具的控制者
    EntityId controllerId = vehicle->getControllingPassenger();
    if (controllerId != player->id()) {
        // 玩家不是控制者，不处理输入
        return PacketHandleResult::Success;
    }

    // 处理跳跃（马、猪等跳跃载具）
    if (packet.isJumping()) {
        // 检查载具是否实现 IJumpingMount 接口
        auto* jumpingMount = dynamic_cast<entity::IJumpingMount*>(vehicle);
        if (jumpingMount != nullptr && jumpingMount->canJump()) {
            // 开始跳跃蓄力（客户端会在 EntityActionPacket 中发送跳跃力度）
            spdlog::trace("PacketHandler: Player {} jumping on vehicle {}", playerId, vehicleId);
        }
    }

    // 处理潜行（下马）
    if (packet.isSneaking()) {
        // MC 1.16.5: 潜行键用于下马
        // 下马逻辑在 EntityActionPacket 中处理
        spdlog::trace("PacketHandler: Player {} sneaking on vehicle {}", playerId, vehicleId);
    }

    return PacketHandleResult::Success;
}

PacketHandleResult PacketHandler::handleMoveVehicle(u32 sessionId, const u8* data, size_t size)
{
    PlayerId playerId = m_playerManager.getPlayerIdBySession(sessionId);
    if (playerId == 0) {
        // spdlog::trace("PacketHandler: Move vehicle from unknown session {}", sessionId);
        return PacketHandleResult::Ignore;
    }

    network::MoveVehiclePacket packet;
    auto result = packet.deserialize(data, size);

    if (result.failed()) {
        spdlog::error("PacketHandler: Failed to parse move vehicle from player {}", playerId);
        return PacketHandleResult::Error;
    }

    // MC 1.16.5: MoveVehiclePacket 由客户端发送以同步载具位置
    // 服务端需要验证位置并将更新广播给其他玩家

    // spdlog::trace("PacketHandler: Player {} move vehicle: ({:.2f}, {:.2f}, {:.2f}) yaw={:.1f} pitch={:.1f}",
    //     playerId,
    //     packet.x(),
    //     packet.y(),
    //     packet.z(),
    //     packet.yaw(),
    //     packet.pitch());

    // 验证服务器接口
    if (m_server == nullptr) {
        spdlog::trace("PacketHandler: Server not set, cannot process vehicle move");
        return PacketHandleResult::Success;
    }

    // 获取玩家实体
    ServerWorld* world = m_server->getPlayerWorld(playerId);
    if (world == nullptr) {
        spdlog::error("PacketHandler: Player {} world not found for vehicle move", playerId);
        return PacketHandleResult::Success;
    }
    ServerPlayerEntityManager& entityManager = m_server->playerEntityManager();
    Player* player = entityManager.getPlayerEntity(playerId, *world);

    if (player == nullptr) {
        spdlog::error("PacketHandler: Player {} entity not found for vehicle move", playerId);
        return PacketHandleResult::Success;
    }

    // 检查玩家是否正在骑乘
    if (!player->isRiding()) {
        return PacketHandleResult::Success;
    }

    // 获取最底层载具（支持嵌套骑乘）
    Entity* vehicle = player->getLowestRidingEntity();
    if (vehicle == nullptr || vehicle == player) {
        return PacketHandleResult::Success;
    }

    // MC 1.16.5: 验证玩家是否是载具的控制者
    EntityId controllerId = vehicle->getControllingPassenger();
    if (controllerId != player->id()) {
        // 玩家不是控制者，忽略移动请求
        spdlog::trace("PacketHandler: Player {} is not controlling vehicle {}", playerId, vehicle->id());
        return PacketHandleResult::Success;
    }

    // MC 1.16.5: 验证数据包有效性（坐标是否为有限数值）
    // Use __builtin_isfinite to work correctly under -ffast-math
    if (!__builtin_isfinite(packet.x()) || !__builtin_isfinite(packet.y()) || !__builtin_isfinite(packet.z()) ||
        !__builtin_isfinite(packet.yaw()) || !__builtin_isfinite(packet.pitch())) {
        spdlog::warn("PacketHandler: Player {} sent invalid vehicle position (NaN or Inf)", playerId);
        // 断开连接
        return PacketHandleResult::Disconnect;
    }

    // MC 1.16.5: 速度验证 - 防止作弊
    // 计算载具当前位置与数据包位置的差距
    Vector3 vehiclePos = vehicle->position();
    Vector3 packetPos(packet.x(), packet.y(), packet.z());

    Vector3 vehicleVel = vehicle->velocity();
    f64 vehicleSpeedSq = static_cast<f64>(vehicleVel.x) * vehicleVel.x + static_cast<f64>(vehicleVel.y) * vehicleVel.y +
        static_cast<f64>(vehicleVel.z) * vehicleVel.z;

    f64 dx = packetPos.x - vehiclePos.x;
    f64 dy = packetPos.y - vehiclePos.y;
    f64 dz = packetPos.z - vehiclePos.z;
    f64 deltaSq = dx * dx + dy * dy + dz * dz;

    // MC 1.16.5: 如果移动速度超过阈值（100.0），记录警告并校正
    // 注意：这里简化实现，实际 MC 还会追踪上一帧位置
    constexpr f64 MAX_VEHICLE_SPEED_SQ = 100.0;
    if (deltaSq - vehicleSpeedSq > MAX_VEHICLE_SPEED_SQ) {
        spdlog::warn("PacketHandler: Player {} vehicle moved too quickly! delta={:.2f}, speed={:.2f}",
            playerId,
            std::sqrt(deltaSq),
            std::sqrt(vehicleSpeedSq));
        // MC 1.16.5: 发送校正包回客户端，恢复到服务端已知位置
        // 暂时不实现校正包，只记录警告
        // 实际应该发送 SMoveVehiclePacket 回客户端
        return PacketHandleResult::Success;
    }

    // MC 1.16.5: 更新载具位置
    // 暂时简化实现：直接设置载具位置
    // 实际 MC 会进行碰撞检测和位置校正
    vehicle->setPosition(packetPos.x, packetPos.y, packetPos.z);
    vehicle->setRotation(packet.yaw(), packet.pitch());

    // 同时更新玩家的位置（玩家跟随载具）
    player->setPosition(packetPos.x, packetPos.y, packetPos.z);
    // 玩家的 yaw 保持原样，pitch 同步载具的一半（MC 1.16.5 行为）
    player->setRotation(player->yaw(), packet.pitch() * 0.5f);

    // 更新位置追踪器
    m_positionTracker.updatePosition(
        playerId, packetPos.x, packetPos.y, packetPos.z, packet.yaw(), packet.pitch(), vehicle->onGround());

    return PacketHandleResult::Success;
}

PacketHandleResult PacketHandler::handleEntityAction(u32 sessionId, const u8* data, size_t size)
{
    PlayerId playerId = m_playerManager.getPlayerIdBySession(sessionId);
    if (playerId == 0) {
        spdlog::error("PacketHandler: Entity action from unknown session {}", sessionId);
        return PacketHandleResult::Ignore;
    }

    network::EntityActionPacket packet;
    auto result = packet.deserialize(data, size);

    if (result.failed()) {
        spdlog::error("PacketHandler: Failed to parse entity action from player {}", playerId);
        return PacketHandleResult::Error;
    }

    // MC 1.16.5: EntityActionPacket 用于实体动作
    // - PressShiftKey: 按下潜行键（下马）
    // - ReleaseShiftKey: 释放潜行键
    // - StartRidingJump: 开始马跳跃蓄力
    // - StopRidingJump: 停止马跳跃蓄力（释放跳跃）
    // - StartSprinting: 开始疾跑
    // - StopSprinting: 停止疾跑

    spdlog::trace("PacketHandler: Player {} entity action: {} aux={}",
        playerId,
        static_cast<i32>(packet.action()),
        packet.auxData());

    // 验证服务器接口
    if (m_server == nullptr) {
        spdlog::trace("PacketHandler: Server not set, cannot process entity action");
        return PacketHandleResult::Success;
    }

    // 获取玩家实体
    ServerWorld* world = m_server->getPlayerWorld(playerId);
    if (world == nullptr) {
        spdlog::trace("PacketHandler: Player {} world not found for entity action", playerId);
        return PacketHandleResult::Success;
    }
    ServerPlayerEntityManager& entityManager = m_server->playerEntityManager();
    Player* player = entityManager.getPlayerEntity(playerId, *world);

    if (player == nullptr) {
        spdlog::error("PacketHandler: Player {} entity not found for entity action", playerId);
        return PacketHandleResult::Success;
    }

    switch (packet.action()) {
        case network::EntityActionType::PressShiftKey:
            // MC 1.16.5: 按下潜行键
            player->setSneaking(true);
            // 如果正在骑乘，触发下马
            if (player->isRiding()) {
                player->stopRiding();
                spdlog::trace("PacketHandler: Player {} dismounted from vehicle", playerId);
            }
            break;

        case network::EntityActionType::ReleaseShiftKey:
            // MC 1.16.5: 释放潜行键
            player->setSneaking(false);
            break;

        case network::EntityActionType::StartRidingJump:
            // MC 1.16.5: 开始马跳跃蓄力
            if (player->isRiding()) {
                EntityId vehicleId = player->getVehicle();
                Entity* vehicle = world->getEntity(vehicleId);
                if (vehicle != nullptr) {
                    auto* jumpingMount = dynamic_cast<entity::IJumpingMount*>(vehicle);
                    if (jumpingMount != nullptr && jumpingMount->canJump()) {
                        i32 jumpPower = packet.auxData();
                        jumpingMount->startJumping(jumpPower);
                        spdlog::trace(
                            "PacketHandler: Player {} started riding jump with power {}", playerId, jumpPower);
                    }
                }
            }
            break;

        case network::EntityActionType::StopRidingJump:
            // MC 1.16.5: 停止马跳跃蓄力（释放跳跃键）
            if (player->isRiding()) {
                EntityId vehicleId = player->getVehicle();
                Entity* vehicle = world->getEntity(vehicleId);
                if (vehicle != nullptr) {
                    auto* jumpingMount = dynamic_cast<entity::IJumpingMount*>(vehicle);
                    if (jumpingMount != nullptr) {
                        jumpingMount->stopJumping();
                        spdlog::trace("PacketHandler: Player {} stopped riding jump", playerId);
                    }
                }
            }
            break;

        case network::EntityActionType::StartSprinting:
            // MC 1.16.5: 开始疾跑
            player->setSprinting(true);
            break;

        case network::EntityActionType::StopSprinting:
            // MC 1.16.5: 停止疾跑
            player->setSprinting(false);
            break;

        default:
            spdlog::error(
                "PacketHandler: Unhandled entity action {} for player {}", static_cast<i32>(packet.action()), playerId);
            break;
    }

    return PacketHandleResult::Success;
}

PacketHandleResult PacketHandler::handleSteerBoat(u32 sessionId, const u8* data, size_t size)
{
    // MC 1.16.5: ServerPlayNetHandler.processSteerBoat()
    PlayerId playerId = m_playerManager.getPlayerIdBySession(sessionId);
    if (playerId == 0) {
        spdlog::error("PacketHandler: SteerBoat from unknown session {}", sessionId);
        return PacketHandleResult::Ignore;
    }

    network::SteerBoatPacket packet;
    auto result = packet.deserialize(data, size);

    if (result.failed()) {
        spdlog::error("PacketHandler: Failed to parse SteerBoat from player {}", playerId);
        return PacketHandleResult::Error;
    }

    // MC 1.16.5: SteerBoatPacket 用于同步船的划桨状态
    // leftPaddle: 左桨是否在划动
    // rightPaddle: 右桨是否在划动

    spdlog::trace(
        "PacketHandler: Player {} steer boat: left={}, right={}", playerId, packet.leftPaddle(), packet.rightPaddle());

    // 验证服务器接口
    if (m_server == nullptr) {
        spdlog::trace("PacketHandler: Server not set, cannot process steer boat");
        return PacketHandleResult::Success;
    }

    // 获取玩家实体
    ServerWorld* world = m_server->getPlayerWorld(playerId);
    if (world == nullptr) {
        spdlog::trace("PacketHandler: Player {} world not found for steer boat", playerId);
        return PacketHandleResult::Success;
    }
    ServerPlayerEntityManager& entityManager = m_server->playerEntityManager();
    Player* player = entityManager.getPlayerEntity(playerId, *world);

    if (player == nullptr) {
        spdlog::error("PacketHandler: Player {} entity not found for steer boat", playerId);
        return PacketHandleResult::Success;
    }

    // 检查玩家是否正在骑乘
    if (!player->isRiding()) {
        return PacketHandleResult::Success;
    }

    // 获取载具实体
    EntityId vehicleId = player->getVehicle();
    if (vehicleId == INVALID_ENTITY_ID) {
        return PacketHandleResult::Success;
    }

    Entity* vehicle = world->getEntity(vehicleId);
    if (vehicle == nullptr) {
        spdlog::error("PacketHandler: Vehicle entity {} not found for player {}", vehicleId, playerId);
        return PacketHandleResult::Success;
    }

    // MC 1.16.5: 检查载具是否是船
    // 只有船需要处理划桨状态
    auto* boat = dynamic_cast<entity::BoatEntity*>(vehicle);
    if (boat == nullptr) {
        // 不是船，忽略
        return PacketHandleResult::Success;
    }

    // MC 1.16.5: 设置船的桨状态
    boat->setPaddleState(packet.leftPaddle(), packet.rightPaddle());

    return PacketHandleResult::Success;
}

PacketHandleResult PacketHandler::handleTeleportConfirm(u32 sessionId, const u8* data, size_t size)
{
    PlayerId playerId = m_playerManager.getPlayerIdBySession(sessionId);
    if (playerId == 0) {
        spdlog::error("PacketHandler: Teleport confirm from unknown session {}", sessionId);
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

PacketHandleResult PacketHandler::handleKeepAlive(u32 sessionId, const u8* data, size_t size, u64 currentTimeMs)
{
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

PacketHandleResult PacketHandler::handleChatMessage(u32 sessionId, const u8* data, size_t size)
{
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
    std::string message = packet.message();

    spdlog::info("[Chat] {}: {}", player->username, message);

    if (m_onChat) {
        m_onChat(playerId, player->username, message);
    }

    return PacketHandleResult::Success;
}

PacketHandleResult PacketHandler::handleUseEntity(u32 sessionId, const u8* data, size_t size)
{
    // MC 1.16.5: ServerPlayNetHandler.processUseEntity()
    PlayerId playerId = m_playerManager.getPlayerIdBySession(sessionId);
    if (playerId == 0) {
        spdlog::error("PacketHandler: UseEntity from unknown session {}", sessionId);
        return PacketHandleResult::Ignore;
    }

    // 反序列化数据包
    network::UseEntityPacket packet;
    auto result = packet.deserialize(data, size);

    if (result.failed()) {
        spdlog::error("PacketHandler: Failed to parse UseEntity from player {}", playerId);
        return PacketHandleResult::Error;
    }

    // 验证服务器接口
    if (m_server == nullptr) {
        spdlog::error("PacketHandler: Server not set, cannot process use entity");
        return PacketHandleResult::Success;
    }

    // 获取玩家实体和世界
    ServerWorld* world = m_server->getPlayerWorld(playerId);
    if (world == nullptr) {
        spdlog::trace("PacketHandler: Player {} world not found for use entity", playerId);
        return PacketHandleResult::Success;
    }
    ServerPlayerEntityManager& entityManager = m_server->playerEntityManager();
    Player* player = entityManager.getPlayerEntity(playerId, *world);

    if (player == nullptr) {
        spdlog::error("PacketHandler: Player {} entity not found for use entity", playerId);
        return PacketHandleResult::Success;
    }

    // 获取目标实体
    Entity* target = world->getEntity(packet.entityId());
    if (target == nullptr) {
        spdlog::error("PacketHandler: Target entity {} not found", packet.entityId());
        return PacketHandleResult::Ignore;
    }

    // MC 1.16.5: 距离检查 - 玩家与实体距离必须小于 36.0 (6格的平方)
    // 注意：创造模式可以跳过距离检查
    if (!player->isCreative()) {
        f32 distanceSq = player->distanceSqTo(*target);
        constexpr f32 MAX_INTERACTION_DISTANCE_SQ = 36.0f;
        if (distanceSq >= MAX_INTERACTION_DISTANCE_SQ) {
            spdlog::warn("PacketHandler: Player {} too far from entity {} (distance={:.2f})",
                playerId,
                packet.entityId(),
                std::sqrt(distanceSq));
            return PacketHandleResult::Ignore;
        }
    }

    // 根据交互类型处理
    ActionResultType actionResult = ActionResultType::Pass;

    switch (packet.action()) {
        case network::UseEntityAction::Interact:
            // MC 1.16.5: 右键交互（不指定具体位置）
            spdlog::trace("PacketHandler: Player {} INTERACT entity {} hand={}",
                playerId,
                packet.entityId(),
                static_cast<int>(packet.hand()));
            actionResult = player->interactOn(*target, packet.hand());
            break;

        case network::UseEntityAction::Attack:
            // MC 1.16.5: 左键攻击
            spdlog::trace("PacketHandler: Player {} ATTACK entity {}", playerId, packet.entityId());
            player->attack(*target);
            // 攻击后触发挥手动画
            player->swing(packet.hand() == Hand::MainHand ? Hand::MainHand : Hand::OffHand);
            actionResult = ActionResultType::Success;
            break;

        case network::UseEntityAction::InteractAt:
            // MC 1.16.5: 右键交互（指定具体位置）
            // 参考: Entity.applyPlayerInteraction()
            // hitPosition 是相对于实体坐标的局部坐标，用于确定点击的是实体的哪个部位
            spdlog::trace("PacketHandler: Player {} INTERACT_AT entity {} pos=({},{},{}) hand={}",
                playerId,
                packet.entityId(),
                packet.hitX(),
                packet.hitY(),
                packet.hitZ(),
                static_cast<int>(packet.hand()));
            {
                Vector3 hitPosition(packet.hitX(), packet.hitY(), packet.hitZ());
                actionResult = target->applyPlayerInteraction(*player, hitPosition, packet.hand());
            }
            break;
    }

    // MC 1.16.5: 成功交互后触发成就和挥手动画
    if (actionResult == ActionResultType::Success || actionResult == ActionResultType::Consume) {
        // 挥手动画
        if (packet.action() != network::UseEntityAction::Attack) {
            // 攻击已经触发了挥手，这里只处理其他交互
            player->swing(packet.hand());
        }

        // MC 1.16.5: 触发 player_interacted_with_entity 成就
        // 参考: CriteriaTriggers.PLAYER_ENTITY_INTERACTION.trigger(player, stack, entity)
        auto* serverPlayer = player->asServerPlayer();
        if (serverPlayer != nullptr) {
            auto* advancements = serverPlayer->getAdvancements();
            if (advancements != nullptr) {
                auto* trigger = advancement::CriterionTriggers::instance()
                                    .getTrigger<advancement::PlayerInteractedWithEntityTrigger>();
                if (trigger != nullptr) {
                    ItemStack heldItem = player->getHeldItem(packet.hand());
                    trigger->AbstractCriterionTrigger<advancement::PlayerInteractedWithEntityTriggerInstance>::trigger(
                        *advancements,
                        [&heldItem, &target](const advancement::PlayerInteractedWithEntityTriggerInstance& instance) {
                            return instance.test(heldItem, *target);
                        });
                }
            }
        }
    }

    return PacketHandleResult::Success;
}

} // namespace mc::server::core
