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

#include "ServerPlayHandler.hpp"

#include "common/entity/entities/player/Player.hpp"
#include "common/entity/entities/vehicle/BoatEntity.hpp"
#include "common/entity/interfaces/IJumpingMount.hpp"
#include "common/item/items/block/BlockItemRegistry.hpp"
#include "common/network/ir/packets/play/PlayPackets.hpp"
#include "common/network/protocol/ConnectionProtocol.hpp"
#include "common/network/protocol/GameActions.hpp"
#include "common/profiler/TraceEvents.hpp"
#include "common/util/TimeUtils.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/util/math/MathUtils.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/blockentity/BlockEntity.hpp"
#include "common/world/blockentity/BlockEntityType.hpp"
#include "common/world/blockentity/interactive/SignEntity.hpp"
#include "common/world/entity/EntityManager.hpp"
#include "server/application/MinecraftServer.hpp"
#include "server/command/CommandRegistry.hpp"
#include "server/command/ServerCommandSource.hpp"
#include "server/core/PlayerManager.hpp"
#include "server/world/ServerWorld.hpp"
#include <cmath>
#include <spdlog/spdlog.h>

using namespace mc::trace;

namespace mc::server::net {

namespace {

[[nodiscard]] bool isCraftingTableState(const BlockState* state)
{
    return state != nullptr && state->blockLocation() == ResourceLocation("minecraft:crafting_table");
}

} // namespace

void ServerPlayHandler::route(PlayerId playerId, const mc::network::ir::IrPacket& packet)
{
    // 新网络层：按 ir::PlayPacket 变体分发到 handle*Packet。
    // 3 个纯虚（handleHotbarSelect/handleContainerClick/handleCloseContainer）经
    // m_server 虚分发到子类 override；其余为本门面方法直调。
    MC_ASSERT_RELEASE(packet.phase == mc::network::protocol::ConnectionProtocol::Play);
    const auto& play = std::get<mc::network::ir::PlayPacket>(packet.packet);
    namespace irplay = mc::network::ir::play;

    if (std::holds_alternative<irplay::MovePlayerPos>(play) || std::holds_alternative<irplay::MovePlayerPosRot>(play) ||
        std::holds_alternative<irplay::MovePlayerRot>(play) ||
        std::holds_alternative<irplay::MovePlayerStatusOnly>(play)) {
        handlePlayerMovePacket(playerId, packet);
    } else if (std::holds_alternative<irplay::AcceptTeleportation>(play)) {
        handleTeleportConfirmPacket(playerId, packet);
    } else if (std::holds_alternative<irplay::KeepAlive>(play)) {
        handleKeepAlivePacket(playerId, packet);
    } else if (std::holds_alternative<irplay::Chat>(play)) {
        handleChatMessagePacket(playerId, packet);
    } else if (std::holds_alternative<irplay::PlayerAction>(play)) {
        handleBlockInteractionPacket(playerId, packet);
    } else if (std::holds_alternative<irplay::UseItemOn>(play)) {
        handleBlockPlacementPacket(playerId, packet);
    } else if (std::holds_alternative<irplay::SetCarriedItem>(play)) {
        m_server.handleHotbarSelectPacket(playerId, packet);
    } else if (std::holds_alternative<irplay::ContainerClick>(play)) {
        m_server.handleContainerClickPacket(playerId, packet);
    } else if (std::holds_alternative<irplay::ContainerClose>(play)) {
        m_server.handleCloseContainerPacket(playerId, packet);
    } else if (std::holds_alternative<irplay::PlayerCommand>(play)) {
        // PlayerCommand 全 action 分发（疾跑/潜行/起床/骑乘跳跃/开背包/滑翔）
        handlePlayerCommandPacket(playerId, packet);
    } else if (std::holds_alternative<irplay::SignUpdate>(play)) {
        handleUpdateSignPacket(playerId, packet);
    } else if (std::holds_alternative<irplay::PlayerInput>(play)) {
        handlePlayerInputPacket(playerId, packet);
    } else if (std::holds_alternative<irplay::ServerboundMoveVehicle>(play)) {
        handleMoveVehiclePacket(playerId, packet);
    } else if (std::holds_alternative<irplay::PaddleBoat>(play)) {
        handlePaddleBoatPacket(playerId, packet);
    } else if (std::holds_alternative<irplay::Interact>(play)) {
        handleInteractPacket(playerId, packet);
    } else if (std::holds_alternative<irplay::UseItem>(play)) {
        handleUseItemPacket(playerId, packet);
    } else {
        // 未覆盖的 C→S 变体（如 SetCreativeModeSlot 等创造模式/命令相关包）
        spdlog::info("route: unhandled C->S play variant");
    }
}

void ServerPlayHandler::handlePlayerMovePacket(PlayerId playerId, const mc::network::ir::IrPacket& packet)
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

void ServerPlayHandler::handleTeleportConfirmPacket(PlayerId playerId, const mc::network::ir::IrPacket& packet)
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

        updateEntityTrackingForPlayer(playerId, player->x, player->y, player->z);

        // 更新区块管理器的玩家位置（触发区块加载票据和追踪变化）
        // 区块发送由 ChunkLoadTicketManager 的追踪变化回调自动处理
        auto* world = m_server.getPlayerWorld(playerId);
        if (world && world->chunkManager()) {
            world->chunkManager()->updatePlayerPosition(playerId, player->x, player->z);
            world->chunkManager()->processTicketUpdatesSync();
        }
    }
}

void ServerPlayHandler::handleKeepAlivePacket(PlayerId playerId, const mc::network::ir::IrPacket& packet)
{
    const auto& play = std::get<mc::network::ir::PlayPacket>(packet.packet);
    const auto* evt = std::get_if<mc::network::ir::play::KeepAlive>(&play);
    if (evt == nullptr) {
        return;
    }

    u64 currentTimeMs = util::TimeUtils::getCurrentTimeMs();
    m_server.keepAliveManager().handleKeepAliveResponse(playerId, static_cast<u64>(evt->id), currentTimeMs);
}

void ServerPlayHandler::handleChatMessagePacket(PlayerId playerId, const mc::network::ir::IrPacket& packet)
{
    auto* player = m_server.playerManager().getPlayer(playerId);
    if (!player || !player->loggedIn) {
        return;
    }

    const auto& play = std::get<mc::network::ir::PlayPacket>(packet.packet);
    const auto* evt = std::get_if<mc::network::ir::play::Chat>(&play);
    if (evt == nullptr) {
        return;
    }

    const std::string& message = evt->message;

    if (!message.empty() && message[0] == '/') {
        // 执行命令
        DimensionId commandDimension = 0;
        if (auto* playerWorld = m_server.getPlayerWorld(playerId)) {
            commandDimension = playerWorld->dimension();
        }
        // 从玩家管理器查找 Player 实体作为命令执行实体。
        mc::Entity* commandEntity = nullptr;
        if (auto* cmdDim = m_server.dimensionManager().getDimension(commandDimension)) {
            if (auto* cmdWorld = cmdDim->world()) {
                commandEntity = m_server.playerEntityManager().getPlayerEntity(playerId, *cmdWorld);
            }
        }

        mc::command::ServerCommandSource source(&m_server,
            nullptr,
            commandDimension,
            Vector3d(player->x, player->y, player->z),
            Vector2f(player->yaw, player->pitch),
            static_cast<i32>(m_server.resolveOpLevel(player->uuid)),
            playerId,
            player->username,
            commandEntity);
        auto cmdResult = m_server.commandRegistry().execute(message, source);
        if (cmdResult.failed()) {
            spdlog::warn("Command '{}' failed for {}: {}", message, player->username, cmdResult.error().toString());
        } else {
            spdlog::info("Command '{}' executed for {} with result {}", message, player->username, cmdResult.value());
        }
        return;
    }

    spdlog::info("[Chat] {}: {}", player->username, message);
}

void ServerPlayHandler::handleUpdateSignPacket(PlayerId playerId, const mc::network::ir::IrPacket& packet)
{
    auto* player = m_server.playerManager().getPlayer(playerId);
    if (!player || !player->loggedIn) {
        return;
    }

    const auto& play = std::get<mc::network::ir::PlayPacket>(packet.packet);
    const auto* evt = std::get_if<mc::network::ir::play::SignUpdate>(&play);
    if (evt == nullptr) {
        return;
    }

    const BlockPos signPos = BlockPos::fromLong(evt->blockPosPacked);

    // 获取玩家所在维度的世界
    ServerWorld* world = m_server.getPlayerWorld(playerId);
    if (world == nullptr) {
        spdlog::warn("UpdateSign: player {} has no world", playerId);
        return;
    }

    // 获取告示牌方块实体
    BlockEntity* blockEntity = world->getBlockEntity(signPos);
    if (blockEntity == nullptr || blockEntity->getType() != BlockEntityType::Sign) {
        spdlog::warn(
            "UpdateSign: no sign entity at ({}, {}, {}) for player {}", signPos.x, signPos.y, signPos.z, playerId);
        return;
    }

    auto* signEntity = static_cast<blockentity::SignEntity*>(blockEntity);

    // 安全检查：只有当前编辑者才能更新文本
    // 对应 MC Java 的 SignBlockEntity.setAllowedPlayerEditor 机制
    if (signEntity->getPlayerWhoMayEdit() != player->uuid) {
        spdlog::warn("UpdateSign: player {} is not the allowed editor of sign at ({}, {}, {})",
            playerId,
            signPos.x,
            signPos.y,
            signPos.z);
        return;
    }

    // 涂蜡的告示牌不允许修改文本
    if (signEntity->isWaxed()) {
        spdlog::warn("UpdateSign: sign at ({}, {}, {}) is waxed, ignoring update from player {}",
            signPos.x,
            signPos.y,
            signPos.z,
            playerId);
        signEntity->clearAllowedPlayerEditor();
        return;
    }

    // 更新4行文本
    for (i32 i = 0; i < 4; ++i) {
        signEntity->setLineFromLegacy(i, evt->lines[static_cast<std::size_t>(i)]);
    }

    // 清除编辑锁
    signEntity->clearAllowedPlayerEditor();

    // 标记方块实体已变更，触发区块存档保存
    signEntity->setChanged();

    // 广播 BlockEntity 数据给附近其他玩家，使其能看到更新后的告示牌文本
    // 参考 MC Java: SignBlockEntity.updateSignText -> level.sendBlockUpdated(pos, state, state, 3)
    world->broadcastBlockEntity(signPos);

    spdlog::info("UpdateSign: player {} updated sign at ({}, {}, {})", playerId, signPos.x, signPos.y, signPos.z);
}

// ============================================================================
// 骑乘 / 交互 / 载具移动（C→S）
//
// 以下 6 个处理体对应 MC Java 1.21.11 的 ServerboundPlayerInput /
// ServerboundMoveVehicle / ServerboundPlayerCommand / ServerboundPaddleBoat /
// ServerboundInteract / ServerboundUseItem。可完成项给出实现，依赖未实现
// 子系统（载具物理/反飞行/物品使用）的标 TODO(Phase6)。
// ============================================================================

void ServerPlayHandler::handlePlayerInputPacket(PlayerId playerId, const mc::network::ir::IrPacket& packet)
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

    // 疾跑状态由 PlayerCommand 维护，这里仅驱动载具。shift（潜行）在本项目
    // 走 PlayerCommand/PlayerInput 之外的状态链路（见 handlePlayerCommandPacket），
    // 载具侧忽略 shift。TODO(Phase6): 跳跃输入驱动 IJumpingMount 载具的蓄力跳跃。
    (void)jump;
    (void)shift;

    auto* playerEntity = m_server.playerEntityManager().getPlayerEntity(playerId, *world);
    if (playerEntity == nullptr) {
        return;
    }

    // 玩家骑乘载具时，输入转发给载具。仅 Boat 已实现 handleInput；其它载具
    // （马/骆驼/羊驼等 IJumpingMount 载具）的输入链路 TODO(Phase6)。
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

    // 非船载具输入处理 TODO(Phase6)。
    (void)sprint;
}

void ServerPlayHandler::handleMoveVehiclePacket(PlayerId playerId, const mc::network::ir::IrPacket& packet)
{
    // 最小实现：NaN 校验 + 写入载具位置 + 回送 ClientboundMoveVehicle 校正。
    // 完整 moved-too-quickly/wrongly 反飞行检测 TODO(Phase6)。
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

    vehicle->setPosition(static_cast<f32>(evt->x), static_cast<f32>(evt->y), static_cast<f32>(evt->z));
    vehicle->setRotation(evt->yRot, evt->xRot);
    vehicle->setOnGround(evt->onGround);

    // 回送校正：服务端权威位置回传客户端，使客户端载具与服务端对齐
    // （对齐 MC Java ServerGamePacketListenerImpl.handleMoveVehicle 发 ClientboundMoveVehicle）。
    mc::network::ir::play::ClientboundMoveVehicle correction;
    correction.x = static_cast<f64>(vehicle->position().x);
    correction.y = static_cast<f64>(vehicle->position().y);
    correction.z = static_cast<f64>(vehicle->position().z);
    correction.yRot = vehicle->yaw();
    correction.xRot = vehicle->pitch();
    m_server.sendPacketToPlayer(playerId,
        mc::network::ir::IrPacket{
            mc::network::protocol::ConnectionProtocol::Play, mc::network::ir::PlayPacket{std::move(correction)}});
}

void ServerPlayHandler::handlePlayerCommandPacket(PlayerId playerId, const mc::network::ir::IrPacket& packet)
{
    // PlayerCommand action（对齐 MC 1.21.11 ServerboundPlayerCommandPacket.Action）：
    // 0=STOP_SLEEPING 1=START_SPRINTING 2=STOP_SPRINTING 3=START_RIDING_JUMP
    // 4=STOP_RIDING_JUMP 5=OPEN_INVENTORY 6=START_FALL_FLYING。
    auto* player = m_server.playerManager().getPlayer(playerId);
    if (!player || !player->loggedIn) {
        return;
    }

    const auto& play = std::get<mc::network::ir::PlayPacket>(packet.packet);
    const auto* evt = std::get_if<mc::network::ir::play::PlayerCommand>(&play);
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

    switch (evt->action) {
        case 0: { // STOP_SLEEPING
            if (auto* serverPlayer = playerEntity->asServerPlayer()) {
                serverPlayer->stopSleepInBed(true, true);
            } else {
                playerEntity->stopSleeping();
            }
            break;
        }
        case 1: // START_SPRINTING
            playerEntity->setSprinting(true);
            break;
        case 2: // STOP_SPRINTING
            playerEntity->setSprinting(false);
            break;
        case 3: { // START_RIDING_JUMP（data=跳跃强度）
            const EntityInstanceId vehicleId = playerEntity->getVehicle();
            if (vehicleId == INVALID_ENTITY_ID) {
                break;
            }
            auto* vehicle = world->getEntity(vehicleId);
            if (vehicle == nullptr) {
                break;
            }
            auto* jumpingMount = dynamic_cast<mc::entity::IJumpingMount*>(vehicle);
            if (jumpingMount != nullptr) {
                jumpingMount->startJumping(evt->data);
            }
            break;
        }
        case 4: { // STOP_RIDING_JUMP
            const EntityInstanceId vehicleId = playerEntity->getVehicle();
            if (vehicleId == INVALID_ENTITY_ID) {
                break;
            }
            auto* vehicle = world->getEntity(vehicleId);
            if (vehicle == nullptr) {
                break;
            }
            auto* jumpingMount = dynamic_cast<mc::entity::IJumpingMount*>(vehicle);
            if (jumpingMount != nullptr) {
                jumpingMount->stopJumping();
            }
            break;
        }
        case 5: // OPEN_INVENTORY
            // 复用既有 Inventory 开包处理体（IntegratedServer 覆写打开菜单）。
            // 经 m_server 虚分发到子类 override，保留 IntegratedServer 开背包覆写。
            m_server.handleOpenPlayerInventoryPacket(playerId, packet);
            break;
        case 6: // START_FALL_FLYING
            if (!playerEntity->tryToStartFallFlying()) {
                playerEntity->stopFallFlying();
            }
            break;
        default:
            spdlog::info("PlayerCommand: player {} sent unknown action {}", playerId, evt->action);
            break;
    }
}

void ServerPlayHandler::handlePaddleBoatPacket(PlayerId playerId, const mc::network::ir::IrPacket& packet)
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

void ServerPlayHandler::handleInteractPacket(PlayerId playerId, const mc::network::ir::IrPacket& packet)
{
    // Interact action：0=INTERACT 1=ATTACK 2=INTERACT_AT。
    // 成就 player_interacted_with_entity 与严格物品/距离校验 TODO(Phase6)。
    auto* player = m_server.playerManager().getPlayer(playerId);
    if (!player || !player->loggedIn) {
        return;
    }

    const auto& play = std::get<mc::network::ir::PlayPacket>(packet.packet);
    const auto* evt = std::get_if<mc::network::ir::play::Interact>(&play);
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

    auto* target = world->getEntity(static_cast<EntityInstanceId>(evt->entityId));
    if (target == nullptr) {
        return;
    }

    // 基本距离校验：超出追踪距离（6 块）拒绝，对齐 MC Java 的 maxInteractDistance 思路。
    // 严格反作弊距离检测 TODO(Phase6)。
    constexpr f32 kMaxInteractDistance = 6.0f;
    if (playerEntity->distanceSqTo(*target) > kMaxInteractDistance * kMaxInteractDistance) {
        return;
    }

    const Hand hand = (evt->hand == static_cast<i32>(Hand::OffHand)) ? Hand::OffHand : Hand::MainHand;

    switch (evt->action) {
        case 0: // INTERACT
            (void)playerEntity->interactOn(*target, hand);
            break;
        case 1: // ATTACK
            playerEntity->attack(*target);
            break;
        case 2: { // INTERACT_AT（带命中点）
            const Vector3 hitPosition(evt->hitX, evt->hitY, evt->hitZ);
            (void)target->applyPlayerInteraction(*playerEntity, hitPosition, hand);
            break;
        }
        default:
            spdlog::info("Interact: player {} sent unknown action {}", playerId, evt->action);
            break;
    }
}

void ServerPlayHandler::handleUseItemPacket(PlayerId playerId, const mc::network::ir::IrPacket& packet)
{
    // 客户端暂无出站 UseItem 发送路径，处理骨架 TODO(Phase6)。
    // 完整实现须：取手持物品 → 物品 useOn 空气分支 → 消耗/冷却/同步。
    (void)playerId;
    (void)packet;
}

void ServerPlayHandler::updateEntityTrackingForPlayer(PlayerId playerId, f64 x, f64 y, f64 z)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.World,
        "ServerPlayHandler::updateEntityTrackingForPlayer",
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

void ServerPlayHandler::handleBlockInteractionPacket(PlayerId playerId, const mc::network::ir::IrPacket& packet)
{
    const auto& play = std::get<mc::network::ir::PlayPacket>(packet.packet);
    const auto* evt = std::get_if<mc::network::ir::play::PlayerAction>(&play);
    if (evt == nullptr) {
        return;
    }

    // 1.21.11 PlayerAction：action 0/1/2 对应 Start/Abort/StopDestroy，与旧 BlockInteractionAction 序数一致。
    //   action 3..6（DROP_ALL/DROP_ITEM/SWAP_HANDS 等）由物品逻辑路径单独处理，此处仅转发挖掘相关。
    const BlockPos pos = BlockPos::fromLong(evt->blockPosPacked);

    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.World,
        "ServerPlayHandler::handleBlockInteractionPacket",
        "pos",
        pos.toString(),
        "playerId",
        playerId,
        [flow = ::perfetto::Flow::ProcessScoped(pos.toId())](::perfetto::EventContext ctx) { flow(ctx); });

    // 仅挖掘相关 action 走 MiningManager
    if (evt->action < 0 || evt->action > 2) {
        return;
    }
    const auto action = static_cast<network::BlockInteractionAction>(evt->action);

    // 处理挖掘状态
    m_server.miningManager().handleBlockInteraction(playerId, pos, action);

    if (action == network::BlockInteractionAction::StopDestroyBlock) {
        if (!m_server.miningManager().tryCompleteMining(playerId, pos)) {
            spdlog::warn("Ignored premature StopDestroyBlock from player {} at {}", playerId, pos.toString());
        }
    }
}

void ServerPlayHandler::handleBlockPlacementPacket(PlayerId playerId, const mc::network::ir::IrPacket& packet)
{
    auto* player = m_server.playerManager().getPlayer(playerId);
    if (!player || !player->loggedIn) {
        return;
    }

    const auto& play = std::get<mc::network::ir::PlayPacket>(packet.packet);
    const auto* evt = std::get_if<mc::network::ir::play::UseItemOn>(&play);
    if (evt == nullptr) {
        return;
    }

    const auto& hit = evt->blockHit;
    const BlockPos pos = BlockPos::fromLong(hit.blockPosPacked);
    auto* playerWorld = m_server.getPlayerWorld(playerId);
    const BlockState* clickedState = playerWorld ? playerWorld->getBlockState(pos) : nullptr;
    const Hand hand = (evt->hand == static_cast<i32>(Hand::OffHand)) ? Hand::OffHand : Hand::MainHand;
    const Direction face = static_cast<Direction>(hit.direction);
    const Vector3 hitPosition(hit.hitX, hit.hitY, hit.hitZ);

    const auto tryOpenCrafting = [this, playerId, pos, clickedState]() {
        return isCraftingTableState(clickedState) && m_server.tryOpenCraftingContainer(playerId, pos);
    };

    ItemStack heldStack = m_server.getHeldItemForPlacement(playerId);
    if (heldStack.isEmpty()) {
        if (!tryOpenCrafting()) {
            (void)m_server.blockInteractionManager().handleBlockUse(playerId, pos, hand, hitPosition, face);
        }
        return;
    }

    const Item* heldItem = heldStack.getItem();
    const bool holdingBlockItem =
        heldItem != nullptr && BlockItemRegistry::instance().getBlockItemByItemId(heldItem->itemId()) != nullptr;

    if (!holdingBlockItem) {
        if (!tryOpenCrafting()) {
            (void)m_server.blockInteractionManager().handleBlockUse(playerId, pos, hand, hitPosition, face);
        }
        return;
    }

    auto interactionResult =
        m_server.blockInteractionManager().handleBlockPlacement(playerId, pos, hitPosition, face, heldStack);

    if (interactionResult.success() && interactionResult.value().blockPlaced &&
        interactionResult.value().itemConsumed) {
        const i32 selectedSlot = m_server.getSelectedHotbarSlot(playerId);
        ItemStack updatedStack = heldStack;
        updatedStack.shrink(1);
        m_server.setInventoryItem(playerId, selectedSlot, updatedStack);
        m_server.syncPlayerInventory(playerId);
    }
}

} // namespace mc::server::net
