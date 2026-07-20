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
#include "PacketHandlerInternal.hpp"
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
#include <cmath>
#include <spdlog/spdlog.h>

namespace mc::server::core {

namespace detail {

void sendVehicleMoveCorrection(
    ConnectionManager& connectionManager, PlayerId playerId, const Entity& vehicle, const Vector3& correctionPos)
{
    network::VehicleMovePacket correction;
    correction.setPosition(correctionPos.x, correctionPos.y, correctionPos.z);
    correction.setRotation(vehicle.yaw(), vehicle.pitch());
    auto serialResult = correction.serialize();
    if (serialResult.success()) {
        connectionManager.sendPacketToPlayer(playerId, network::PacketType::VehicleMove, serialResult.value());
    }
}

bool isEntityCollidingWithAnythingNew(
    const IWorld& world, const Entity& vehicle, const AxisAlignedBB& oldAABB, const Vector3& targetPos)
{
    // MC Java 使用 entity.getCollisionBorderSize() 扩展 AABB 做实体碰撞检测。
    // Cubium 默认 getCollisionBorderSize() == 0.0f，载具（船/矿车）未重写，
    // 因此 border 通常为 0，但严格对齐 MC Java 应保留此扩展。
    const f32 border = vehicle.getCollisionBorderSize();

    // 计算从 oldAABB 中心到 targetPos 的偏移量
    // MC Java 的 isEntityCollidingWithAnythingNew 取 entity.getBoundingBox()（已 move 后）
    // 然后 .move(target - entity.pos) 得到目标位置 AABB。等价于直接用 oldAABB 偏移到目标位置。
    const Vector3 oldCenter = oldAABB.center();
    const f32 offsetX = static_cast<f32>(targetPos.x - oldCenter.x);
    const f32 offsetY = static_cast<f32>(targetPos.y - oldAABB.minY); // AABB 底部对齐实体脚部
    const f32 offsetZ = static_cast<f32>(targetPos.z - oldCenter.z);

    // 目标位置的 AABB（用 offsetted 一次性偏移三轴，再 grow border）
    const AxisAlignedBB targetAABB = oldAABB.offsetted(offsetX, offsetY, offsetZ).grow(border);

    // 查询目标 AABB 范围内的所有实体（排除自身），手动过滤 canBeCollidedWith
    // IWorld::getEntitiesInAABB 是 const 方法，ServerWorld 通过 EntityManager 加锁查询。
    // 对齐 MC Java 的 getEntityCollisions(entity, aabb) 谓词：
    //   entity == null ? CAN_BE_COLLIDED_WITH : (NO_SPECTATORS.and(entity::canCollideWith))
    // 即当传入载具时，使用 vehicle.canCollideWith(candidate) 过滤，
    // 该方法内部已包含 canBeCollidedWith 与 isPassengerOfSameVehicle 检查
    // （载具不会与其自身乘客互相碰撞）。
    std::vector<Entity*> nearbyEntities = world.getEntitiesInAABB(targetAABB, &vehicle);
    for (const Entity* candidate : nearbyEntities) {
        if (candidate == nullptr || candidate == &vehicle) {
            continue;
        }
        if (vehicle.canCollideWith(*candidate)) {
            return true;
        }
    }
    return false;
}

} // namespace detail

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
        return PacketHandleResult::Ignore;
    }

    network::PlayerInputPacket packet;
    auto result = packet.deserialize(data, size);

    if (result.failed()) {
        spdlog::error("PacketHandler: Failed to parse player input from player {}", playerId);
        return PacketHandleResult::Error;
    }

    // PlayerInputPacket 用于控制骑乘中的载具
    // strafeSpeed: 左右移动（正值=左，负值=右）
    // forwardSpeed: 前后移动（正值=前，负值=后）
    // jumping: 是否跳跃
    // sneaking: 是否潜行（下马）

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
    EntityInstanceId vehicleId = player->getVehicle();
    if (vehicleId == INVALID_ENTITY_ID) {
        return PacketHandleResult::Success;
    }

    Entity* vehicle = world->getEntity(vehicleId);
    if (vehicle == nullptr) {
        return PacketHandleResult::Success;
    }

    // 检查玩家是否是载具的控制者
    EntityInstanceId controllerId = vehicle->getControllingPassenger();
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
        }
    }

    // 处理潜行（下马）
    if (packet.isSneaking()) {
        // 潜行键用于下马，下马逻辑在 EntityActionPacket 中处理
    }

    return PacketHandleResult::Success;
}

PacketHandleResult PacketHandler::handleMoveVehicle(u32 sessionId, const u8* data, size_t size)
{
    PlayerId playerId = m_playerManager.getPlayerIdBySession(sessionId);
    if (playerId == 0) {
        return PacketHandleResult::Ignore;
    }

    network::MoveVehiclePacket packet;
    auto result = packet.deserialize(data, size);

    if (result.failed()) {
        spdlog::error("PacketHandler: Failed to parse move vehicle from player {}", playerId);
        return PacketHandleResult::Error;
    }

    // MoveVehiclePacket 由客户端发送以同步载具位置
    // 服务端需要验证位置并将更新广播给其他玩家

    // 验证服务器接口
    if (m_server == nullptr) {
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

    // 验证玩家是否是载具的控制者
    EntityInstanceId controllerId = vehicle->getControllingPassenger();
    if (controllerId != player->id()) {
        // 玩家不是控制者，忽略移动请求
        return PacketHandleResult::Success;
    }

    // 验证数据包有效性（坐标是否为有限数值）
    if (!std::isfinite(packet.x()) || !std::isfinite(packet.y()) || !std::isfinite(packet.z()) ||
        !std::isfinite(packet.yaw()) || !std::isfinite(packet.pitch())) {
        spdlog::warn("PacketHandler: Player {} sent invalid vehicle position (NaN or Inf)", playerId);
        // 断开连接
        return PacketHandleResult::Disconnect;
    }

    // 速度验证 - 防止作弊
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

    if (deltaSq - vehicleSpeedSq > detail::kMaxVehicleSpeedSq) {
        spdlog::warn("PacketHandler: Player {} vehicle moved too quickly! delta={:.2f}, speed={:.2f}",
            playerId,
            std::sqrt(deltaSq),
            std::sqrt(vehicleSpeedSq));

        // 发送校正包回客户端，恢复到服务端已知位置
        // 对应 MC Java ServerGamePacketListenerImpl.handleMoveVehicle 中
        // 的 ClientboundMoveVehiclePacket.fromEntity(entity) 校正逻辑
        detail::sendVehicleMoveCorrection(m_connectionManager, playerId, *vehicle, vehiclePos);
        return PacketHandleResult::Success;
    }

    // ========== 碰撞检测与 moved wrongly 检测 ==========
    // 对应 MC Java ServerGamePacketListenerImpl.handleMoveVehicle 中
    // entity.move(MoverType.PLAYER, vec3) + moved wrongly 检测逻辑。
    //
    // 流程：
    // 1. 保存移动前的载具位置与 AABB（作为回退点）
    // 2. 调用 moveWithCollision 做带碰撞检测的移动，得到实际移动向量
    // 3. 比较"客户端期望位置"与"碰撞移动后位置"的差距（moved wrongly 判定）
    // 4. 若 moved wrongly 且旧位置无碰撞（说明是客户端穿了墙）→ 回退到旧位置并发校正包
    // 5. 否则接受客户端位置（对齐 MC Java 的 absSnapTo(d3,d4,d5,f,f1)）

    // 步骤 1：保存回退点
    const Vector3 oldVehiclePos = vehiclePos;
    const AxisAlignedBB oldAABB = vehicle->boundingBox();

    // 步骤 2：调用 moveWithCollision 做带碰撞检测的移动
    // moveWithCollision 会更新 m_position / m_boundingBox / m_onGround / 碰撞状态，
    // 并在碰撞时清零对应轴的速度。moved wrongly 判定通过比较"客户端期望位置"
    // 与"碰撞后载具实际位置"（vehicle->position()）的差距来完成，不直接使用返回值。
    vehicle->moveWithCollision(static_cast<f32>(dx), static_cast<f32>(dy), static_cast<f32>(dz));

    // 步骤 3：moved wrongly 检测
    // 计算"客户端期望位置"与"碰撞移动后载具位置"的差距。
    // 对应 MC Java：
    //   d6 = d3 - entity.getX();
    //   d7 = d4 - entity.getY();
    //   if (d7 > -0.5 || d7 < 0.5) d7 = 0.0;   // Y 方向容差
    //   d8 = d5 - entity.getZ();
    //   d10 = d6*d6 + d7*d7 + d8*d8;
    //   flag1 = (d10 > 0.0625);
    //
    // MC Java 的 Y 容差条件 `d7 > -0.5 || d7 < 0.5` 用了 `||`，在数学上恒为真
    // （不存在同时使两个子句为假的 d7），因此 Y 偏差实际上总是被清零，
    // moved wrongly 检测实际只看 X 和 Z 方向的偏差。这里直接清零 diffY 以复刻其运行时效果。
    const Vector3 newVehiclePos = vehicle->position();
    const f64 diffX = packetPos.x - static_cast<f64>(newVehiclePos.x);
    const f64 diffY = 0.0;
    const f64 diffZ = packetPos.z - static_cast<f64>(newVehiclePos.z);
    const f64 movedWronglySq = diffX * diffX + diffY * diffY + diffZ * diffZ;

    const bool movedWrongly = movedWronglySq > detail::kMovedWronglyThresholdSq;

    // 步骤 4：回退判定
    // 对应 MC Java：
    //   if ((flag1 && serverlevel.noCollision(entity, aabb))
    //       || isEntityCollidingWithAnythingNew(serverlevel, entity, aabb, d3, d4, d5))
    //
    // - noCollision(entity, 旧AABB)：旧 AABB 在当前世界无碰撞（说明客户端把载具推进了墙）
    // - isEntityCollidingWithAnythingNew：移动到目标位置后会与新实体碰撞
    //
    // Cubium 中 noCollision 等价于：!hasBlockCollision(box) && !hasEntityCollision(box, except)
    // isEntityCollidingWithAnythingNew 等价于检查目标位置 AABB 是否与"新"实体碰撞
    // （由于 EntityManager::getEntitiesInAABB 不过滤 canBeCollidedWith，需手动过滤）。
    const bool oldPosNoCollision = !world->hasBlockCollision(oldAABB) && !world->hasEntityCollision(oldAABB, vehicle);
    const bool collidingWithNewEntity = detail::isEntityCollidingWithAnythingNew(*world, *vehicle, oldAABB, packetPos);

    if ((movedWrongly && oldPosNoCollision) || collidingWithNewEntity) {
        // 回退到旧位置：恢复载具位置与旋转
        // 注意：moveWithCollision 可能已修改 velocity（碰撞轴清零），这里不恢复 velocity，
        // 让下一帧的载具 tick 自然处理（与 MC Java 行为一致——回退后 entity.removeLatestMovementRecording）。
        vehicle->setPosition(oldVehiclePos.x, oldVehiclePos.y, oldVehiclePos.z);
        vehicle->setRotation(packet.yaw(), packet.pitch());

        // 发送校正包回客户端，恢复到服务端已知位置
        detail::sendVehicleMoveCorrection(m_connectionManager, playerId, *vehicle, oldVehiclePos);

        // 玩家位置也回退到旧位置（保持骑乘关系）
        player->setPosition(oldVehiclePos.x, oldVehiclePos.y, oldVehiclePos.z);
        player->setRotation(player->yaw(), packet.pitch() * 0.5f);

        // 更新位置追踪器（用回退后的位置）
        m_positionTracker.updatePosition(playerId,
            oldVehiclePos.x,
            oldVehiclePos.y,
            oldVehiclePos.z,
            packet.yaw(),
            packet.pitch(),
            vehicle->onGround());

        if (movedWrongly) {
            spdlog::warn(
                "PacketHandler: Player {} vehicle moved wrongly! delta={:.4f}", playerId, std::sqrt(movedWronglySq));
        }
        return PacketHandleResult::Success;
    }

    // 步骤 5：正常路径 - 对齐到客户端请求的精确位置
    // 对应 MC Java 的 entity.absSnapTo(d3, d4, d5, f, f1)
    // （MC Java 在 move 后仍用客户端精确位置覆盖，因为 move 只是用来做碰撞检测）
    vehicle->setPosition(packetPos.x, packetPos.y, packetPos.z);
    vehicle->setRotation(packet.yaw(), packet.pitch());

    // 同时更新玩家的位置（玩家跟随载具）
    player->setPosition(packetPos.x, packetPos.y, packetPos.z);
    // 玩家的 yaw 保持原样，pitch 同步载具的一半
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

    // EntityActionPacket 用于实体动作
    // - PressShiftKey: 按下潜行键（下马）
    // - ReleaseShiftKey: 释放潜行键
    // - StartRidingJump: 开始马跳跃蓄力
    // - StopRidingJump: 停止马跳跃蓄力（释放跳跃）
    // - StartSprinting: 开始疾跑
    // - StopSprinting: 停止疾跑

    // 验证服务器接口
    if (m_server == nullptr) {
        return PacketHandleResult::Success;
    }

    // 获取玩家实体
    ServerWorld* world = m_server->getPlayerWorld(playerId);
    if (world == nullptr) {
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
            // 按下潜行键
            player->setSneaking(true);
            // 如果正在骑乘，触发下马
            if (player->isRiding()) {
                player->stopRiding();
            }
            break;

        case network::EntityActionType::ReleaseShiftKey:
            // 释放潜行键
            player->setSneaking(false);
            break;

        case network::EntityActionType::StartRidingJump:
            // 开始马跳跃蓄力
            if (player->isRiding()) {
                EntityInstanceId vehicleId = player->getVehicle();
                Entity* vehicle = world->getEntity(vehicleId);
                if (vehicle != nullptr) {
                    auto* jumpingMount = dynamic_cast<entity::IJumpingMount*>(vehicle);
                    if (jumpingMount != nullptr && jumpingMount->canJump()) {
                        i32 jumpPower = packet.auxData();
                        jumpingMount->startJumping(jumpPower);
                    }
                }
            }
            break;

        case network::EntityActionType::StopRidingJump:
            // 停止马跳跃蓄力（释放跳跃键）
            if (player->isRiding()) {
                EntityInstanceId vehicleId = player->getVehicle();
                Entity* vehicle = world->getEntity(vehicleId);
                if (vehicle != nullptr) {
                    auto* jumpingMount = dynamic_cast<entity::IJumpingMount*>(vehicle);
                    if (jumpingMount != nullptr) {
                        jumpingMount->stopJumping();
                    }
                }
            }
            break;

        case network::EntityActionType::StartSprinting:
            // 开始疾跑
            player->setSprinting(true);
            break;

        case network::EntityActionType::StopSprinting:
            // 停止疾跑
            player->setSprinting(false);
            break;

        case network::EntityActionType::StartFallFlying:
            // 开始鞘翅滑翔
            // 对应 MC 1.21.11 ServerGamePacketListenerImpl.handlePlayerCommand():
            //   case START_FALL_FLYING:
            //     if (!this.player.tryToStartFallFlying()) {
            //         this.player.stopFallFlying();
            //     }
            //     break;
            // 客户端在玩家于空中按下空格（且穿戴鞘翅）时发送此包。
            // 服务端校验可滑翔条件，若失败则强制收起鞘翅（同步状态）。
            if (!player->tryToStartFallFlying()) {
                player->stopFallFlying();
            }
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

    // SteerBoatPacket 用于同步船的划桨状态
    // leftPaddle: 左桨是否在划动
    // rightPaddle: 右桨是否在划动

    // 验证服务器接口
    if (m_server == nullptr) {
        return PacketHandleResult::Success;
    }

    // 获取玩家实体
    ServerWorld* world = m_server->getPlayerWorld(playerId);
    if (world == nullptr) {
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
    EntityInstanceId vehicleId = player->getVehicle();
    if (vehicleId == INVALID_ENTITY_ID) {
        return PacketHandleResult::Success;
    }

    Entity* vehicle = world->getEntity(vehicleId);
    if (vehicle == nullptr) {
        spdlog::error("PacketHandler: Vehicle entity {} not found for player {}", vehicleId, playerId);
        return PacketHandleResult::Success;
    }

    // 检查载具是否是船，只有船需要处理划桨状态
    auto* boat = dynamic_cast<entity::BoatEntity*>(vehicle);
    if (boat == nullptr) {
        // 不是船，忽略
        return PacketHandleResult::Success;
    }

    // 设置船的桨状态
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

    // 距离检查 - 玩家与实体距离必须小于 36.0 (6格的平方)
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
            // 右键交互（不指定具体位置）
            actionResult = player->interactOn(*target, packet.hand());
            break;

        case network::UseEntityAction::Attack:
            // 左键攻击
            player->attack(*target);
            // 攻击后触发挥手动画
            player->swing(packet.hand() == Hand::MainHand ? Hand::MainHand : Hand::OffHand);
            actionResult = ActionResultType::Success;
            break;

        case network::UseEntityAction::InteractAt:
            // 右键交互（指定具体位置）
            // hitPosition 是相对于实体坐标的局部坐标，用于确定点击的是实体的哪个部位
            {
                Vector3 hitPosition(packet.hitX(), packet.hitY(), packet.hitZ());
                actionResult = target->applyPlayerInteraction(*player, hitPosition, packet.hand());
            }
            break;
    }

    // 成功交互后触发成就和挥手动画
    if (actionResult == ActionResultType::Success || actionResult == ActionResultType::Consume) {
        // 挥手动画
        if (packet.action() != network::UseEntityAction::Attack) {
            // 攻击已经触发了挥手，这里只处理其他交互
            player->swing(packet.hand());
        }

        // 触发 player_interacted_with_entity 成就
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
