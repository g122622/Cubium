/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without including limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permitted persons to whom the Software is
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

#include "server/network/play/MovementHandler.hpp"

#include "common/core/Types.hpp"
#include "common/entity/effect/EffectInstance.hpp"
#include "common/entity/effect/EffectType.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/entities/vehicle/BoatEntity.hpp"
#include "common/network/ir/IrPacket.hpp"
#include "common/network/ir/packets/play/PlayPackets.hpp"
#include "common/network/ir/packets/play/PlayPacketsExtended.hpp"
#include "common/profiler/TraceCategories.hpp"
#include "common/profiler/TraceEvents.hpp"
#include "common/util/math/MathUtils.hpp"
#include "common/util/math/Vector2.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/entity/EntityManager.hpp"
#include "common/world/gamerule/GameRules.hpp"
#include "common/world/village/Village.hpp"
#include "server/application/MinecraftServer.hpp"
#include "server/core/PlayerManager.hpp"
#include "server/core/ServerPlayerData.hpp"
#include "server/player/ServerPlayer.hpp"
#include "server/world/ServerWorld.hpp"
#include <cmath>
#include <string>
#include <variant>
#include <spdlog/spdlog.h>

using namespace mc::trace;

namespace mc::server::net {

// ============================================================================
// 移动 / 载具移动 / 客户端输入
// ============================================================================

void MovementHandler::handlePlayerMovePacket(PlayerId playerId, const mc::network::ir::IrPacket& packet)
{
    auto* player = m_server.playerManager().getPlayer(playerId);
    if (!player || !player->loggedIn) {
        return;
    }

    namespace irplay = mc::network::ir::play;
    const auto& play = std::get<mc::network::ir::PlayPacket>(packet.packet);

    // 解析四个 MovePlayer 变体之一，统一出位置/朝向/onGround 与"是否含位置变更"
    f64 posX = player->x;
    f64 posY = player->y;
    f64 posZ = player->z;
    f32 yaw = player->yaw;
    f32 pitch = player->pitch;
    bool onGround = player->onGround;
    bool hasPosChange = false;
    bool hasRotOnly = false;

    if (auto* p = std::get_if<irplay::MovePlayerPosRot>(&play)) {
        posX = p->x;
        posY = p->y;
        posZ = p->z;
        yaw = p->yRot;
        pitch = p->xRot;
        onGround = p->flags.onGround;
        hasPosChange = true;
    } else if (auto* p = std::get_if<irplay::MovePlayerPos>(&play)) {
        posX = p->x;
        posY = p->y;
        posZ = p->z;
        onGround = p->flags.onGround;
        hasPosChange = true;
    } else if (auto* p = std::get_if<irplay::MovePlayerRot>(&play)) {
        yaw = p->yRot;
        pitch = p->xRot;
        onGround = p->flags.onGround;
        hasRotOnly = true;
    } else if (auto* p = std::get_if<irplay::MovePlayerStatusOnly>(&play)) {
        onGround = p->flags.onGround;
    } else {
        return;
    }

    // 保存旧位置用于村庄进入检测
    BlockPos prevPos(static_cast<i32>(player->x), static_cast<i32>(player->y), static_cast<i32>(player->z));

    // ===== 反飞行阈值校验（moved-too-quickly / moved-wrongly 双闸） =====
    // 对齐 Java ServerGamePacketListenerImpl.handleMovePlayer。仅对含位置变更的包生效；
    // 纯朝向/着地状态包不触发。isSingleplayerOwner 跳过全部校验（单机主人信任）。
    // 超限不改坐标，而是 requestTeleport 回弹至 lastGood 并提前 return，等客户端
    // 回 AcceptTeleportation 收敛。跨 tick 基线落在真实 ServerPlayer 实体上
    // （本处理器为无状态对象，ServerPlayerData 为网络簿记结构无 Entity 能力）。
    if (hasPosChange && !hasRotOnly) {
        auto* world = m_server.getPlayerWorld(playerId);
        mc::ServerPlayer* serverPlayer = nullptr;
        if (world != nullptr) {
            if (auto* entity = m_server.playerEntityManager().getPlayerEntity(playerId, *world); entity != nullptr) {
                serverPlayer = entity->asServerPlayer();
            }
        }
        if (serverPlayer != nullptr && !m_server.isSingleplayerOwner(playerId)) {
            // 等待传送确认期间不处理位置变更，仅更新朝向（对齐 vanilla
            // ServerGamePacketListenerImpl#handleMovePlayer：updateAwaitingTeleport 返回 true 时
            // 只 absSnapRotationTo，跳过位置/反飞行校验）。否则回弹后客户端在 AcceptTeleportation
            // 回包前继续发移动包，触发新回弹形成 teleport ID mismatch 死循环。仅对非单机主人
            // （真 Java 客户端）生效；cpp 本地客户端 isSingleplayerOwner=true 走豁免，不受影响。
            if (m_server.teleportManager().isWaitingForConfirm(playerId)) {
                player->yaw = yaw;
                player->pitch = pitch;
                return;
            }
            // 首次进入 Play 阶段初始化基线为当前坐标。
            if (!serverPlayer->hasAntiFlightBaselineInited()) {
                serverPlayer->resetAntiFlightBaseline(
                    static_cast<f64>(player->x), static_cast<f64>(player->y), static_cast<f64>(player->z));
                serverPlayer->markAntiFlightBaselineInited();
            }

            // moved-too-quickly：本包位移 - 速度模长平方 > 阈值*(收包积压数)。
            const f64 d6 = posX - serverPlayer->firstGoodX();
            const f64 d7 = posY - serverPlayer->firstGoodY();
            const f64 d8 = posZ - serverPlayer->firstGoodZ();
            const f64 d10 = d6 * d6 + d7 * d7 + d8 * d8;
            const f64 d9 = static_cast<f64>(serverPlayer->velocity().lengthSquared());

            serverPlayer->incrementReceivedMovePacketCount();
            i32 backlog = serverPlayer->receivedMovePacketCount() - serverPlayer->knownMovePacketCount();
            if (backlog > 5) {
                // 客户端发包过快，对齐 Java 仅记 warn 并按 1 计。
                spdlog::warn(
                    "Player {} is sending move packets too frequently ({} packets since last tick)", playerId, backlog);
                backlog = 1;
            }

            const bool elytra = serverPlayer->isElytraFlying();
            // 对齐 Java shouldCheckPlayerMovement：elytra 时受 DISABLE_ELYTRA_MOVEMENT_CHECK 开关控制。
            // GameRules 经所在 ServerWorld 取（MinecraftServer 不持有 GameRules，由世界承载）。
            const bool disableElytraCheck = world != nullptr &&
                world->getGameRules().getBoolean(world::gamerule::GameRuleKeys::DISABLE_ELYTRA_MOVEMENT_CHECK);
            const bool checkQuickly = !elytra || !disableElytraCheck;
            if (checkQuickly) {
                const f64 threshold = elytra ? 300.0 : 100.0;
                if (d10 - d9 > threshold * static_cast<f64>(backlog)) {
                    spdlog::warn("Player {} moved too quickly! {},{},{}", playerId, d6, d7, d8);
                    m_server.teleportManager().requestTeleport(playerId,
                        serverPlayer->lastGoodX(),
                        serverPlayer->lastGoodY(),
                        serverPlayer->lastGoodZ(),
                        player->yaw,
                        player->pitch);
                    return;
                }
            }

            // 注：vanilla 的 moved-wrongly（0.0625）校验的是 player.move 物理移动后申报位置与
            // 实际位置（受碰撞影响）的碰撞偏移，用于反穿墙。本项目服务端不做玩家物理 move +
            // 碰撞模拟（玩家位置由客户端权威申报），无碰撞偏移可言；原实现误用"申报位置相对
            // firstGood 的累积位移"，而 firstGood 每 tick 才滚动，真 Java 客户端发包频率与 tick
            // 非 1:1 时跨 tick 累积，正常行走也超 0.0625 → 误判回弹 → 卡顿。故移除该闸，
            // 反穿墙留待未来服务端碰撞校验实现。moved-too-quickly（瞬时速度）已覆盖飞行外挂。
            // 通过校验：滚动 lastGood 为本包坐标（firstGood 由 tick 末滚动）。
            serverPlayer->advanceLastGood(posX, posY, posZ);
        }
    }

    player->x = static_cast<f32>(posX);
    player->y = static_cast<f32>(posY);
    player->z = static_cast<f32>(posZ);
    player->yaw = yaw;
    player->pitch = pitch;
    player->onGround = onGround;

    // 计算新区块坐标
    ChunkCoord newChunkX = math::floorTo<ChunkCoord>(posX / static_cast<f64>(world::CHUNK_WIDTH));
    ChunkCoord newChunkZ = math::floorTo<ChunkCoord>(posZ / static_cast<f64>(world::CHUNK_WIDTH));

    // 检查玩家是否移动到了新区块
    ChunkCoord oldChunkX = math::floorTo<ChunkCoord>(player->x / static_cast<f32>(world::CHUNK_WIDTH));
    ChunkCoord oldChunkZ = math::floorTo<ChunkCoord>(player->z / static_cast<f32>(world::CHUNK_WIDTH));
    bool chunkChanged = (newChunkX != oldChunkX || newChunkZ != oldChunkZ);
    (void)chunkChanged;

    auto* world = m_server.getPlayerWorld(playerId);
    if (world) {
        world->entityManager().forEachEntity([playerId, player](Entity* entity) {
            auto* playerEntity = dynamic_cast<Player*>(entity);
            if (playerEntity == nullptr || playerEntity->playerId() != playerId) {
                return true;
            }

            playerEntity->setPosition(player->x, player->y, player->z);
            playerEntity->setRotation(player->yaw, player->pitch);
            playerEntity->setOnGround(player->onGround);
            return false;
        });
    }

    m_server.positionTracker().updatePosition(
        playerId, player->x, player->y, player->z, player->yaw, player->pitch, player->onGround);
    updateEntityTrackingForPlayer(playerId, player->x, player->y, player->z);

    // 更新区块管理器的玩家位置（触发区块加载票据和追踪变化）
    // 区块发送由 ChunkLoadTicketManager 的追踪变化回调自动处理
    if (world && world->chunkManager()) {
        world->chunkManager()->updatePlayerPosition(playerId, player->x, player->z);
        world->chunkManager()->processTicketUpdatesSync();
    }

    // 村庄进入检测（用于触发袭击）
    // 仅在位置实际改变时检测，村庄/袭击仅存在于主世界
    auto* overworld = m_server.dimensionManager().getOverworld();
    if (overworld && overworld->world() && hasPosChange && !hasRotOnly) {
        auto* villageManager = overworld->world()->villageManager();
        auto* raidManager = overworld->world()->raidManager();
        if (villageManager && raidManager) {
            BlockPos currentPos(static_cast<i32>(player->x), static_cast<i32>(player->y), static_cast<i32>(player->z));
            world::village::Village* enteredVillage = villageManager->checkPlayerEnterVillage(currentPos, prevPos);
            if (enteredVillage) {
                // 玩家进入了村庄，使用回调检查不祥之兆并触发袭击
                raidManager->onPlayerEnterVillageWithCallback(
                    [player](BlockPos) -> i32 {
                        if (player->hasEffect(entity::effect::EffectType::BadOmen)) {
                            const entity::effect::EffectInstance* effect =
                                player->getEffect(entity::effect::EffectType::BadOmen);
                            if (effect != nullptr) {
                                i32 level = effect->getEffectLevel();
                                // 移除不祥之兆效果
                                player->removeEffect(entity::effect::EffectType::BadOmen);
                                return level;
                            }
                        }
                        return 0;
                    },
                    enteredVillage);
            }
        }
    }
}

void MovementHandler::handleTeleportConfirmPacket(PlayerId playerId, const mc::network::ir::IrPacket& packet)
{
    const auto& play = std::get<mc::network::ir::PlayPacket>(packet.packet);
    const auto* evt = std::get_if<mc::network::ir::play::AcceptTeleportation>(&play);
    if (evt == nullptr) {
        return;
    }

    if (m_server.teleportManager().confirmTeleport(playerId, evt->teleportId)) {
        auto* player = m_server.playerManager().getPlayer(playerId);
        if (!player) {
            return;
        }

        // 重置反飞行基线为传送后坐标。回弹/传送后客户端从此点继续移动，若基线仍残留
        // 传送前的旧值，下一移动包的 moved-too-quickly（相对 firstGood 的位移）会跨
        // 传送点累积误判，再次回弹形成死循环。对齐 vanilla resetPosition（teleport 后
        // firstGood/lastGood 同步到 player.position）。
        auto* world = m_server.getPlayerWorld(playerId);
        if (world != nullptr) {
            if (auto* entity = m_server.playerEntityManager().getPlayerEntity(playerId, *world); entity != nullptr) {
                if (auto* serverPlayer = entity->asServerPlayer(); serverPlayer != nullptr) {
                    serverPlayer->resetAntiFlightBaseline(
                        static_cast<f64>(player->x), static_cast<f64>(player->y), static_cast<f64>(player->z));
                    serverPlayer->markAntiFlightBaselineInited();
                }
            }
        }

        updateEntityTrackingForPlayer(playerId, player->x, player->y, player->z);

        // 更新区块管理器的玩家位置（触发区块加载票据和追踪变化）
        // 区块发送由 ChunkLoadTicketManager 的追踪变化回调自动处理
        if (world && world->chunkManager()) {
            world->chunkManager()->updatePlayerPosition(playerId, player->x, player->z);
            world->chunkManager()->processTicketUpdatesSync();
        }
    }
}

void MovementHandler::handlePlayerInputPacket(PlayerId playerId, const mc::network::ir::IrPacket& packet)
{
    // PlayerInput 位掩码：bit0=forward bit1=backward bit2=left bit3=right
    // bit4=jump bit5=shift bit6=sprint（对齐 MC 1.21.11 net.minecraft.world.entity.player.Input）。
    auto* player = m_server.playerManager().getPlayer(playerId);
    if (!player || !player->loggedIn) {
        return;
    }

    const auto& play = std::get<mc::network::ir::PlayPacket>(packet.packet);
    const auto* evt = std::get_if<mc::network::ir::play::PlayerInput>(&play);
    if (evt == nullptr) {
        return;
    }

    auto* world = m_server.getPlayerWorld(playerId);
    if (world == nullptr) {
        return;
    }

    const u8 input = evt->input;
    const bool forward = (input & 0x01) != 0;
    const bool backward = (input & 0x02) != 0;
    const bool left = (input & 0x04) != 0;
    const bool right = (input & 0x08) != 0;
    const bool jump = (input & 0x10) != 0;
    const bool shift = (input & 0x20) != 0;
    const bool sprint = (input & 0x40) != 0;

    auto* playerEntity = m_server.playerEntityManager().getPlayerEntity(playerId, *world);
    if (playerEntity == nullptr) {
        return;
    }

    // 对齐 Java ServerGamePacketListenerImpl.handlePlayerInput：写入玩家最近客户端输入
    // 缓存 + 同步 shift（潜行）状态。jump 位不在此驱动 IJumpingMount——蓄力跳跃由
    // PlayerCommand START_RIDING_JUMP/STOP_RIDING_JUMP 触发（见 handlePlayerCommandPacket），
    // 与 vanilla 一致（PlayerInput.jump 仅影响玩家自身跳跃，不驱动骑乘载具跳跃）。
    playerEntity->setLastClientInput(input);
    playerEntity->setSneaking(shift);

    // 玩家骑乘载具时，输入转发给载具。
    const EntityInstanceId vehicleId = playerEntity->getVehicle();
    if (vehicleId == INVALID_ENTITY_ID) {
        return;
    }

    auto* vehicle = world->getEntity(vehicleId);
    if (vehicle == nullptr) {
        return;
    }

    auto* boat = dynamic_cast<mc::entity::BoatEntity*>(vehicle);
    if (boat != nullptr) {
        // 注意参数顺序：(left, right, forward, backward)（对齐 BoatEntity::handleInput）。
        boat->handleInput(left, right, forward, backward);
        return;
    }

    // 非船载具（马/骆驼/羊驼等 IJumpingMount）：移动输入已写入 playerEntity 的
    // lastClientInput 缓存，待载具 travel() 物理实现后在其 tick 中拾取驱动。
    // 项目当前未实现非船载具的 travel() 物理移动，故此处不额外转发；
    // 蓄力跳跃经 PlayerCommand 链路已通。载具 travel 物理属未实现子系统。
    (void)backward;
    (void)left;
    (void)right;
    (void)jump;
    (void)sprint;
    (void)forward;
}

void MovementHandler::handleMoveVehiclePacket(PlayerId playerId, const mc::network::ir::IrPacket& packet)
{
    // NaN 校验 + moved-too-quickly/wrongly 反飞行 + 写入载具位置 + 回送校正。
    // 对齐 Java ServerGamePacketListenerImpl.handleMoveVehicle。
    auto* player = m_server.playerManager().getPlayer(playerId);
    if (!player || !player->loggedIn) {
        return;
    }

    const auto& play = std::get<mc::network::ir::PlayPacket>(packet.packet);
    const auto* evt = std::get_if<mc::network::ir::play::ServerboundMoveVehicle>(&play);
    if (evt == nullptr) {
        return;
    }

    // 对应 MC Java NetworkValidatorUtils.isInvalidValue：坐标/朝向含 NaN 即拒。
    if (std::isnan(evt->x) || std::isnan(evt->y) || std::isnan(evt->z) || std::isnan(evt->yRot) ||
        std::isnan(evt->xRot)) {
        spdlog::warn("MoveVehicle: player {} sent NaN position, ignoring", playerId);
        return;
    }

    auto* world = m_server.getPlayerWorld(playerId);
    if (world == nullptr) {
        return;
    }

    auto* playerEntity = m_server.playerEntityManager().getPlayerEntity(playerId, *world);
    if (playerEntity == nullptr) {
        return;
    }

    const EntityInstanceId vehicleId = playerEntity->getVehicle();
    if (vehicleId == INVALID_ENTITY_ID) {
        return;
    }

    auto* vehicle = world->getEntity(vehicleId);
    if (vehicle == nullptr) {
        return;
    }

    // 回送载具校正包的辅助 lambda（vanilla 超限/正常均回送权威位置）。
    auto sendVehicleCorrection = [this, playerId, vehicle]() {
        mc::network::ir::play::ClientboundMoveVehicle correction;
        correction.x = static_cast<f64>(vehicle->position().x);
        correction.y = static_cast<f64>(vehicle->position().y);
        correction.z = static_cast<f64>(vehicle->position().z);
        correction.yRot = vehicle->yaw();
        correction.xRot = vehicle->pitch();
        m_server.sendPacketToPlayer(playerId,
            mc::network::ir::IrPacket{
                mc::network::protocol::ConnectionProtocol::Play, mc::network::ir::PlayPacket{std::move(correction)}});
    };

    // 反飞行双闸（载具基线落在骑乘者 ServerPlayer 上）。isSingleplayerOwner 豁免。
    auto* serverPlayer = playerEntity->asServerPlayer();
    if (serverPlayer != nullptr && !m_server.isSingleplayerOwner(playerId)) {
        // 载具切换（下坐再骑乘新载具）时重置基线，避免旧载具坐标误判 quickly。
        if (!serverPlayer->hasVehicleAntiFlightInited() || serverPlayer->lastVehicleId() != vehicleId) {
            serverPlayer->resetVehicleAntiFlightBaseline(static_cast<f64>(vehicle->position().x),
                static_cast<f64>(vehicle->position().y),
                static_cast<f64>(vehicle->position().z));
            serverPlayer->markVehicleAntiFlightInited();
            serverPlayer->setLastVehicleId(vehicleId);
        }

        // moved-too-quickly：本包位移 - 速度模长平方 > 100.0 → 回送校正，不写入坐标。
        const f64 d6 = evt->x - serverPlayer->vehicleFirstGoodX();
        const f64 d7 = evt->y - serverPlayer->vehicleFirstGoodY();
        const f64 d8 = evt->z - serverPlayer->vehicleFirstGoodZ();
        const f64 d9 = static_cast<f64>(vehicle->velocity().lengthSquared());
        const f64 d10 = d6 * d6 + d7 * d7 + d8 * d8;
        if (d10 - d9 > 100.0) {
            spdlog::warn("Player {} (vehicle) moved too quickly! {},{},{}", playerId, d6, d7, d8);
            sendVehicleCorrection();
            return;
        }

        // 注：vanilla moved-wrongly（0.0625）校验的是载具物理 move 后的碰撞偏移，本项目
        // 服务端不做载具物理碰撞模拟，无碰撞偏移可言；原实现用"申报位置相对 vehicleLastGood
        // 的位移"，载具正常移动单包位移²常超 0.0625 → 误判 → 坐标不写入 → 载具卡顿。
        // 故移除该闸，moved-too-quickly（瞬时速度）已覆盖飞行外挂。
        serverPlayer->advanceVehicleLastGood(evt->x, evt->y, evt->z);
    }

    vehicle->setPosition(static_cast<f32>(evt->x), static_cast<f32>(evt->y), static_cast<f32>(evt->z));
    vehicle->setRotation(evt->yRot, evt->xRot);
    vehicle->setOnGround(evt->onGround);

    // 回送校正：服务端权威位置回传客户端，使客户端载具与服务端对齐
    // （对齐 MC Java ServerGamePacketListenerImpl.handleMoveVehicle 发 ClientboundMoveVehicle）。
    sendVehicleCorrection();
}

void MovementHandler::handlePaddleBoatPacket(PlayerId playerId, const mc::network::ir::IrPacket& packet)
{
    auto* player = m_server.playerManager().getPlayer(playerId);
    if (!player || !player->loggedIn) {
        return;
    }

    const auto& play = std::get<mc::network::ir::PlayPacket>(packet.packet);
    const auto* evt = std::get_if<mc::network::ir::play::PaddleBoat>(&play);
    if (evt == nullptr) {
        return;
    }

    auto* world = m_server.getPlayerWorld(playerId);
    if (world == nullptr) {
        return;
    }

    auto* playerEntity = m_server.playerEntityManager().getPlayerEntity(playerId, *world);
    if (playerEntity == nullptr) {
        return;
    }

    const EntityInstanceId vehicleId = playerEntity->getVehicle();
    if (vehicleId == INVALID_ENTITY_ID) {
        return;
    }

    auto* vehicle = world->getEntity(vehicleId);
    if (vehicle == nullptr) {
        return;
    }

    auto* boat = dynamic_cast<mc::entity::BoatEntity*>(vehicle);
    if (boat == nullptr) {
        // 非船载具无桨状态，忽略。
        return;
    }

    boat->setPaddleState(evt->left, evt->right);
}

void MovementHandler::updateEntityTrackingForPlayer(PlayerId playerId, f64 x, f64 y, f64 z)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.World,
        "MovementHandler::updateEntityTrackingForPlayer",
        "playerId",
        playerId,
        "x",
        x,
        "y",
        y,
        "z",
        z);

    auto* world = m_server.getPlayerWorld(playerId);
    if (!world) {
        return;
    }

    auto* player = m_server.playerManager().getPlayer(playerId);
    if (!player || !player->loggedIn) {
        return;
    }

    world->entityTracker().updatePlayerTracking(
        m_server, *world, playerId, Vector3(static_cast<f32>(x), static_cast<f32>(y), static_cast<f32>(z)));
}

} // namespace mc::server::net
