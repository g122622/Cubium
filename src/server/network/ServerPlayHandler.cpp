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

#include "common/advancement/trigger/CriterionTrigger.hpp"
#include "common/advancement/trigger/CriterionTriggers.hpp"
#include "common/advancement/trigger/impl/EntityTriggers.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/entities/vehicle/BoatEntity.hpp"
#include "common/entity/interfaces/IJumpingMount.hpp"
#include "common/entity/registry/VanillaEntityTypeKeys.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/core/ItemRegistry.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/items/block/BlockItemRegistry.hpp"
#include "common/network/ir/packets/play/PlayPackets.hpp"
#include "common/network/protocol/ConnectionProtocol.hpp"
#include "common/network/protocol/GameActions.hpp"
#include "common/profiler/TraceEvents.hpp"
#include "common/util/AxisAlignedBB.hpp"
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
#include "server/advancement/TriggerInstantiation.hpp"
#include "server/application/MinecraftServer.hpp"
#include "server/command/CommandRegistry.hpp"
#include "server/command/ServerCommandSource.hpp"
#include "server/core/OpListManager.hpp"
#include "server/core/PlayerManager.hpp"
#include "server/player/ServerPlayer.hpp"
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
    } else if (std::holds_alternative<irplay::ServerboundPingRequest>(play)) {
        handlePingRequestPacket(playerId, packet);
    } else if (std::holds_alternative<irplay::ServerboundPong>(play)) {
        handlePongPacket(playerId, packet);
    } else if (std::holds_alternative<irplay::ServerboundChangeDifficulty>(play)) {
        handleChangeDifficultyPacket(playerId, packet);
    } else if (std::holds_alternative<irplay::LockDifficulty>(play)) {
        handleLockDifficultyPacket(playerId, packet);
    } else {
        // 未覆盖的 C→S 变体（如 SetCreativeModeSlot 等创造模式/命令相关包）
        spdlog::info("route: unhandled C->S play variant");
    }
}

void ServerPlayHandler::handlePingRequestPacket(PlayerId playerId, const mc::network::ir::IrPacket& packet)
{
    // 客户端主动 ping(ping 协议通道)，回 PongResponse(cb:60) 同 time。
    const auto& play = std::get<mc::network::ir::PlayPacket>(packet.packet);
    const auto* evt = std::get_if<mc::network::ir::play::ServerboundPingRequest>(&play);
    if (evt == nullptr) {
        return;
    }
    mc::network::ir::play::PongResponse resp;
    resp.time = evt->time;
    m_server.sendPacketToPlayer(playerId,
        mc::network::ir::IrPacket{
            mc::network::protocol::ConnectionProtocol::Play, mc::network::ir::PlayPacket{std::move(resp)}});
}

void ServerPlayHandler::handlePongPacket(PlayerId playerId, const mc::network::ir::IrPacket& packet)
{
    // common 通道 cb:59 ping 的回声。对齐 Java ServerCommonPacketListenerImpl.handlePong：
    // 服务端不据此计算 RTT（vanilla 为空实现），仅确认回声链路可达。
    const auto& play = std::get<mc::network::ir::PlayPacket>(packet.packet);
    const auto* evt = std::get_if<mc::network::ir::play::ServerboundPong>(&play);
    if (evt == nullptr) {
        return;
    }
    spdlog::info("[Server] Pong from player {} id={}", playerId, evt->id);
}

void ServerPlayHandler::handleChangeDifficultyPacket(PlayerId playerId, const mc::network::ir::IrPacket& packet)
{
    auto* player = m_server.playerManager().getPlayer(playerId);
    if (!player || !player->loggedIn) {
        return;
    }
    const auto& play = std::get<mc::network::ir::PlayPacket>(packet.packet);
    const auto* evt = std::get_if<mc::network::ir::play::ServerboundChangeDifficulty>(&play);
    if (evt == nullptr) {
        return;
    }
    if (evt->difficulty < 0 || evt->difficulty > 3) {
        spdlog::warn("ChangeDifficulty: player {} sent invalid difficulty {}", playerId, evt->difficulty);
        return;
    }
    // 权限校验对齐 Java handleChangeDifficulty：主机 或 OP(>=GameMaster) 方可改难度。
    // 主机判定不依赖 allowCommands（与 resolveOpLevel 的作弊提升解耦）。
    const bool permitted = m_server.isSingleplayerOwner(playerId) ||
        m_server.opListManager().getLevel(player->uuid) >= core::OpLevel::GameMaster;
    if (!permitted) {
        spdlog::warn("ChangeDifficulty: player {} has no permission to change difficulty", playerId);
        return;
    }
    // setDifficulty 内部含锁定守卫与 cb:10 广播，无需在此重复发送。
    m_server.setDifficulty(static_cast<Difficulty>(evt->difficulty));
}

void ServerPlayHandler::handleLockDifficultyPacket(PlayerId playerId, const mc::network::ir::IrPacket& packet)
{
    auto* player = m_server.playerManager().getPlayer(playerId);
    if (!player || !player->loggedIn) {
        return;
    }
    const auto& play = std::get<mc::network::ir::PlayPacket>(packet.packet);
    const auto* evt = std::get_if<mc::network::ir::play::LockDifficulty>(&play);
    if (evt == nullptr) {
        return;
    }
    // 权限校验对齐 Java handleLockDifficulty：主机 或 OP(>=GameMaster) 方可锁定。
    const bool permitted = m_server.isSingleplayerOwner(playerId) ||
        m_server.opListManager().getLevel(player->uuid) >= core::OpLevel::GameMaster;
    if (!permitted) {
        spdlog::warn("LockDifficulty: player {} has no permission to lock difficulty", playerId);
        return;
    }
    // setDifficultyLocked 内部已广播 cb:10（携带新 locked 值），客户端据此禁用难度按钮。
    m_server.setDifficultyLocked(evt->locked);
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

    // ===== 反飞行阈值校验（moved-too-quickly / moved-wrongly 双闸） =====
    // 对齐 Java ServerGamePacketListenerImpl.handleMovePlayer。仅对含位置变更的包生效；
    // 纯朝向/着地状态包不触发。isSingleplayerOwner 跳过全部校验（单机主人信任）。
    // 超限不改坐标，而是 requestTeleport 回弹至 lastGood 并提前 return，等客户端
    // 回 AcceptTeleportation 收敛。跨 tick 基线落在真实 ServerPlayer 实体上
    // （ServerPlayHandler 为无状态门面，ServerPlayerData 为网络簿记结构无 Entity 能力）。
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
// ServerboundInteract / ServerboundUseItem。载具输入/反飞行/物品使用/交互校验
// 子系统均已落地（批6），详见各处理体实现。
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

void ServerPlayHandler::handleMoveVehiclePacket(PlayerId playerId, const mc::network::ir::IrPacket& packet)
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
                serverPlayer->stopSleepInBed(true);
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
    // 对齐 Java ServerGamePacketListenerImpl.handleInteract：世界边界 → AABB 距离 →
    // ATTACK 实体黑名单 → 交互成功触发 player_interacted_with_entity 成就 + 挥手。
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

    // 副操作（潜行）状态同步：vanilla player.setShiftKeyDown(usingSecondaryAction)。
    playerEntity->setSneaking(evt->usingSecondaryAction);

    // 世界边界校验：目标方块位越界直接拒绝。
    if (!world->worldBorder().contains(target->onPos())) {
        return;
    }

    // 严格距离校验：玩家眼位到目标 AABB 最近点距离 < (reach + 3.0)^2。
    // 项目无 entityInteractionRange attribute，固定取 3.0（生存默认）。
    // ATTACK 走同样阈值（vanilla ATTACK 用 isWithinAttackRange，项目无 attack range
    // attribute，统一用交互阈值）。
    constexpr f64 kEntityInteractionRange = 3.0;
    constexpr f64 kRangeMargin = 3.0;
    const f64 range = kEntityInteractionRange + kRangeMargin;
    const Vector3 eyePos = playerEntity->getEyePosition();
    const f64 distSq = static_cast<f64>(target->boundingBox().distanceToSqr(eyePos));
    if (distSq >= range * range) {
        return;
    }

    const Hand hand = (evt->hand == static_cast<i32>(Hand::OffHand)) ? Hand::OffHand : Hand::MainHand;

    switch (evt->action) {
        case 0: { // INTERACT
            const ItemStack itemBefore = playerEntity->getHeldItem(hand);
            const ActionResultType result = playerEntity->interactOn(*target, hand);
            if (result == ActionResultType::Success) {
                _triggerPlayerInteractedWithEntity(*playerEntity, itemBefore, *target);
                playerEntity->swing(hand);
            }
            break;
        }
        case 1: { // ATTACK
            // 实体黑名单：掉落物/经验球/自身/箭矢一律不可攻击。
            const auto* type = target->entityType();
            const bool blacklisted = type == entity::VanillaEntityTypeKeys::ITEM ||
                type == entity::VanillaEntityTypeKeys::EXPERIENCE_ORB || type == entity::VanillaEntityTypeKeys::ARROW ||
                type == entity::VanillaEntityTypeKeys::SPECTRAL_ARROW || target == playerEntity;
            if (blacklisted) {
                spdlog::warn("Interact: player {} tried to attack invalid entity", playerId);
                break;
            }
            playerEntity->attack(*target);
            break;
        }
        case 2: { // INTERACT_AT（带命中点）
            const Vector3 hitPosition(evt->hitX, evt->hitY, evt->hitZ);
            const ItemStack itemBefore = playerEntity->getHeldItem(hand);
            const ActionResultType result = target->applyPlayerInteraction(*playerEntity, hitPosition, hand);
            if (result == ActionResultType::Success) {
                _triggerPlayerInteractedWithEntity(*playerEntity, itemBefore, *target);
                playerEntity->swing(hand);
            }
            break;
        }
        default:
            spdlog::info("Interact: player {} sent unknown action {}", playerId, evt->action);
            break;
    }
}

void ServerPlayHandler::_triggerPlayerInteractedWithEntity(Player& player, const ItemStack& item, Entity& entity)
{
    // 触发 player_interacted_with_entity 成就。对齐 vanilla
    // CriteriaTriggers.PLAYER_INTERACTED_WITH_ENTITY.trigger(player, item, entity)。
    // PlayerInteractedWithEntityTrigger::trigger 在 common 层为空桩，须按
    // AdvancementEventHandler 既定模式直接调基类 trigger 模板。
    auto* serverPlayer = player.asServerPlayer();
    if (serverPlayer == nullptr) {
        return;
    }
    auto* advancements = serverPlayer->getAdvancements();
    if (advancements == nullptr) {
        return;
    }
    auto* trigger =
        mc::advancement::CriterionTriggers::instance().getTrigger<mc::advancement::PlayerInteractedWithEntityTrigger>();
    if (trigger == nullptr) {
        return;
    }
    trigger->AbstractCriterionTrigger<mc::advancement::PlayerInteractedWithEntityTriggerInstance>::trigger(
        *advancements, [&item, &entity](const mc::advancement::PlayerInteractedWithEntityTriggerInstance& instance) {
            return instance.test(item, entity);
        });
}

void ServerPlayHandler::handleUseItemPacket(PlayerId playerId, const mc::network::ir::IrPacket& packet)
{
    // 右键空气使用物品（对齐 Java ServerGamePacketListenerImpl.handleUseItem）。
    auto* player = m_server.playerManager().getPlayer(playerId);
    if (!player || !player->loggedIn) {
        return;
    }

    const auto& play = std::get<mc::network::ir::PlayPacket>(packet.packet);
    const auto* evt = std::get_if<mc::network::ir::play::UseItem>(&play);
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

    const Hand hand = (evt->hand == static_cast<i32>(Hand::OffHand)) ? Hand::OffHand : Hand::MainHand;

    // 朝向校正：客户端申报朝向与服务端记录不一致时，以客户端为准对齐（对齐 vanilla absSnapRotationTo）。
    if (evt->yRot != playerEntity->yaw() || evt->xRot != playerEntity->pitch()) {
        playerEntity->setRotation(evt->yRot, evt->xRot);
    }

    // 取手持物品；空手不触发使用。
    ItemStack heldStack = playerEntity->getHeldItem(hand);
    if (heldStack.isEmpty()) {
        return;
    }

    const Item* heldItemC = heldStack.getItem();
    if (heldItemC == nullptr) {
        return;
    }
    // Item 是无状态策略单例，onItemRightClick 非 const；经 ItemRegistry 取非 const 句柄调用。
    Item* heldItem = ItemRegistry::instance().getItem(heldItemC->itemId());
    if (heldItem == nullptr) {
        return;
    }

    // 调用物品在空气中的使用逻辑（对应 Java ServerPlayerGameMode.useItem → Item.use）。
    // 即时变换型物品（空地图→已填充地图、玻璃瓶→水瓶等）在 onItemRightClick 内部
    // 直接修改玩家物品栏；持续使用型（食物/弓/弩，返回 Consume）需服务端使用计时体系
    // 驱动 onItemUseFinish，项目当前未实现该体系，故 Consume 结果暂不完成消耗。
    ItemActionResult result = heldItem->onItemRightClick(*world, *playerEntity, hand);

    // Success/Consume 触发挥臂动画（对齐 vanilla swingSource=SERVER 时 swing）。
    if (result.isSuccess() || result.isConsume()) {
        playerEntity->swing(hand);
    }

    // 物品栏变更同步：onItemRightClick 内部已修改 Player 实体 inventory，
    // 经 syncPlayerInventory 下发。主手物品栏同步复用 handleBlockPlacement 同一路径。
    if (result.isSuccess()) {
        const i32 selectedSlot = m_server.getSelectedHotbarSlot(playerId);
        // 取 Player 实体当前主手物品（onItemRightClick 可能已变换/消耗）回写权威管理器。
        ItemStack updatedStack = (hand == Hand::MainHand) ? playerEntity->getHeldItem(hand) : heldStack;
        m_server.setInventoryItem(playerId, selectedSlot, updatedStack);
        m_server.syncPlayerInventory(playerId);
    }
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
