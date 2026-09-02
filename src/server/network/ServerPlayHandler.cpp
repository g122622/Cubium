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

#include "common/advancement/AdvancementManager.hpp"
#include "common/advancement/trigger/CriterionTrigger.hpp"
#include "common/advancement/trigger/CriterionTriggers.hpp"
#include "common/advancement/trigger/impl/AnyBlockUseTrigger.hpp"
#include "common/advancement/trigger/impl/EntityTriggers.hpp"
#include "common/core/Types.hpp"
#include "common/entity/effect/EffectInstance.hpp"
#include "common/entity/effect/EffectType.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/entities/vehicle/BoatEntity.hpp"
#include "common/entity/interfaces/IJumpingMount.hpp"
#include "common/entity/registry/VanillaEntityTypeKeys.hpp"
#include "common/item/core/ActionResult.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/core/ItemRegistry.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/items/block/BlockItemRegistry.hpp"
#include "common/network/ir/IrPacket.hpp"
#include "common/network/ir/packets/play/PlayPackets.hpp"
#include "common/network/ir/packets/play/PlayPacketsExtended.hpp"
#include "common/network/protocol/ConnectionProtocol.hpp"
#include "common/network/protocol/GameActions.hpp"
#include "common/profiler/TraceCategories.hpp"
#include "common/profiler/TraceEvents.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/AxisAlignedBB.hpp"
#include "common/util/TimeUtils.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/util/math/MathUtils.hpp"
#include "common/util/math/Vector2.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/blockentity/BlockEntity.hpp"
#include "common/world/blockentity/BlockEntityType.hpp"
#include "common/world/blockentity/interactive/SignEntity.hpp"
#include "common/world/entity/EntityManager.hpp"
#include "common/world/gamerule/GameRules.hpp"
#include "common/world/village/Village.hpp"
#include "server/application/MinecraftServer.hpp"
#include "server/command/CommandRegistry.hpp"
#include "server/command/ServerCommandSource.hpp"
#include "server/core/OpListManager.hpp"
#include "server/core/PlayerManager.hpp"
#include "server/player/ServerPlayer.hpp"
#include "server/world/ServerWorld.hpp"
#include <cmath>
#include <cstddef>
#include <string>
#include <utility>
#include <variant>
#include <spdlog/spdlog.h>

using namespace mc::trace;

namespace mc::server::net {

namespace {

[[nodiscard]] bool isCraftingTableState(const BlockState* state)
{
    return state != nullptr && state->blockLocation() == ResourceLocation("minecraft:crafting_table");
}

/// 对齐 vanilla ServerGamePacketListenerImpl.java:1339-1340：无论放置成功失败，
/// 定向给操作玩家发命中方块 + 放置目标位置（blockpos.relative(direction)）的
/// BlockUpdate，作为客户端预测状态机的硬同步兜底。立即定向发送，不走批量入队。
void sendPlacementBlockUpdates(
    MinecraftServer& server, PlayerId playerId, ServerWorld& world, const BlockPos& pos, Direction face)
{
    auto sendOne = [&](const BlockPos& p) {
        const BlockState* s = world.getBlockState(p);
        const u32 stateId = (s != nullptr) ? s->stateId() : 0;
        mc::network::ir::play::BlockUpdate pkt;
        pkt.blockPosPacked = p.asLong();
        pkt.blockStateId = static_cast<i32>(stateId);
        server.sendPacketToPlayer(playerId,
            mc::network::ir::IrPacket{
                mc::network::protocol::ConnectionProtocol::Play,
                mc::network::ir::PlayPacket{std::move(pkt)},
            });
    };
    sendOne(pos);
    sendOne(pos.offset(face));
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
    } else if (std::holds_alternative<irplay::ChatCommand>(play)) {
        handleChatCommandPacket(playerId, packet);
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
    } else if (std::holds_alternative<irplay::ConfigurationAcknowledged>(play)) {
        handleConfigurationAcknowledgedPacket(playerId, packet);
    } else if (std::holds_alternative<irplay::SeenAdvancements>(play)) {
        handleSeenAdvancementsPacket(playerId, packet);
    } else if (std::holds_alternative<irplay::PlaceRecipe>(play)) {
        handlePlaceRecipePacket(playerId, packet);
    } else if (std::holds_alternative<irplay::ChunkBatchReceived>(play)) {
        handleChunkBatchReceivedPacket(playerId, packet);
    } else if (std::holds_alternative<irplay::SetCreativeModeSlot>(play)) {
        m_server.handleSetCreativeModeSlotPacket(playerId, packet);
    } else {
        // 未覆盖的 C→S 变体（创造模式/命令相关包）
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

void ServerPlayHandler::handleConfigurationAcknowledgedPacket(
    PlayerId playerId, const mc::network::ir::IrPacket& packet)
{
    // ConfigurationAcknowledged（C→S terminal）：客户端收到服务端 StartConfiguration(S→C) 后回此包，
    // 双方切回 Configuration 阶段。入站阶段由框架 ProtocolSwapHandler 自动切回 Configuration
    // （见 ProtocolSwapHandler.cpp Play 分支识别此 terminal），无需此处手动 setInboundPhase。
    //
    // TODO(Play→Configuration reconfiguration): 完整 reconfiguration 需服务端在此显式
    // setOutboundPhase(Configuration) 后重推 RegistryData/UpdateTags 等；但当前项目无
    // StartConfiguration(S→C) IR 结构体，服务端无发起 reconfiguration 的路径，此包运行时不被
    // 客户端触发。本处理仅确认接收，保证 terminal 自动阶段切换链路不被 route 兜底干扰。
    (void)packet;
    auto* player = m_server.playerManager().getPlayer(playerId);
    if (player == nullptr || !player->loggedIn) {
        return;
    }
    spdlog::info("ConfigurationAcknowledged from player {}, switching back to Configuration phase", playerId);
}

void ServerPlayHandler::handleSeenAdvancementsPacket(PlayerId playerId, const mc::network::ir::IrPacket& packet)
{
    // SeenAdvancements（C→S id=49）：action 0=OPENED_TAB（tab=ResourceLocation），1=CLOSED_SCREEN。
    // 对齐 Java ServerGamePacketListenerImpl.handleSeenAdvancements：
    //  - OPENED_TAB：解析 tab 为 AdvancementPtr 写入 PlayerAdvancements::m_selectedTab（持久化用）。
    //  - CLOSED_SCREEN：vanilla 为空实现（不清除选中标签页）。
    auto* player = m_server.playerManager().getPlayer(playerId);
    if (player == nullptr || !player->loggedIn) {
        return;
    }
    const auto& play = std::get<mc::network::ir::PlayPacket>(packet.packet);
    const auto* evt = std::get_if<mc::network::ir::play::SeenAdvancements>(&play);
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
    auto* serverPlayer = playerEntity->asServerPlayer();
    if (serverPlayer == nullptr) {
        return;
    }
    auto* advancements = serverPlayer->getAdvancements();
    if (advancements == nullptr) {
        return;
    }
    if (evt->action == 0) { // OPENED_TAB
        const auto adv = mc::advancement::AdvancementManager::instance().get(mc::ResourceLocation(evt->tab));
        if (adv == nullptr) {
            spdlog::warn("SeenAdvancements OPENED_TAB: advancement {} not found", evt->tab);
            return;
        }
        advancements->setSelectedTab(adv);
    }
    // action == 1 (CLOSED_SCREEN)：vanilla 空实现，不清除选中标签页。
}

void ServerPlayHandler::handlePlaceRecipePacket(PlayerId playerId, const mc::network::ir::IrPacket& packet)
{
    // PlaceRecipe（C→S id=38）：玩家在配方书中点击配方，请求服务端把材料填入合成网格。
    // 字段：containerId(VarInt)/recipe(VarInt RecipeDisplayId)/useMaxItems(bool)。
    //
    // TODO(配方书网络同步链路): 完整实现需 display-id→ResourceLocation 映射（RecipeDisplayId 是
    // 客户端配方表索引，服务端需维护映射）+ 按配方填充合成网格槽位（容器槽位操作 API）。整个配方书
    // 同步链路（recipe_book_add/remove/settings 下行、place_recipe/recipe_book_change_settings 上行）
    // 尚未打通，此处仅确认接收并记 warn，避免 route 兜底静默丢弃。
    (void)packet;
    auto* player = m_server.playerManager().getPlayer(playerId);
    if (player == nullptr || !player->loggedIn) {
        return;
    }
    spdlog::warn("PlaceRecipe from player {} not implemented (recipe book sync chain pending)", playerId);
}

void ServerPlayHandler::handleChunkBatchReceivedPacket(PlayerId playerId, const mc::network::ir::IrPacket& packet)
{
    // ChunkBatchReceived（C→S id=10）：客户端在区块批次结束后回发本批次实际接收速率
    // chunksPerTick(Float)，服务端据此动态调整下一批次区块发送配额。
    //
    // TODO(服务端区块批次流速控): 当前 ChunkSendManager::processPendingSends 每 tick 抽干
    // m_readyChunks 就绪队列，无批次划分与发送配额，亦不发 ChunkBatchStart/Finished(cb:11/12)。
    // 完整实现需在 ChunkSendManager 引入批次窗口（按 tick 攒批 + Start/Finished 配对下发），
    // 并以本包 chunksPerTick 反馈调整每批次发送上限。此处仅确认接收并记 warn。
    const auto& play = std::get<mc::network::ir::PlayPacket>(packet.packet);
    const auto* evt = std::get_if<mc::network::ir::play::ChunkBatchReceived>(&play);
    if (evt == nullptr) {
        return;
    }
    auto* player = m_server.playerManager().getPlayer(playerId);
    if (player == nullptr || !player->loggedIn) {
        return;
    }
    spdlog::warn("ChunkBatchReceived from player {} chunksPerTick={} not implemented (batch flow control pending)",
        playerId,
        evt->chunksPerTick);
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
        // 命令：message 含 '/' 前缀，CommandDispatcher::parse 自动剥离。
        _executePlayerCommand(playerId, message);
        return;
    }

    spdlog::info("[Chat] {}: {}", player->username, message);
}

void ServerPlayHandler::handleChatCommandPacket(PlayerId playerId, const mc::network::ir::IrPacket& packet)
{
    auto* player = m_server.playerManager().getPlayer(playerId);
    if (!player || !player->loggedIn) {
        return;
    }

    const auto& play = std::get<mc::network::ir::PlayPacket>(packet.packet);
    const auto* evt = std::get_if<mc::network::ir::play::ChatCommand>(&play);
    if (evt == nullptr) {
        return;
    }

    // ChatCommand.command 不含 '/' 前缀（对齐 vanilla ServerboundChatCommandPacket）。
    // CommandDispatcher::parse 对有无 '/' 均自动剥离，故直接传 evt->command，无需补 '/'。
    _executePlayerCommand(playerId, evt->command);
}

void ServerPlayHandler::_executePlayerCommand(PlayerId playerId, const std::string& commandInput)
{
    auto* player = m_server.playerManager().getPlayer(playerId);
    if (!player || !player->loggedIn) {
        return;
    }

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
    // CommandDispatcher::parse 自动剥离前导 '/'，commandInput 含或不含 '/' 均可。
    auto cmdResult = m_server.commandRegistry().execute(commandInput, source);
    if (cmdResult.failed()) {
        spdlog::warn("Command '{}' failed for {}: {}", commandInput, player->username, cmdResult.error().toString());
    } else {
        spdlog::info("Command '{}' executed for {} with result {}", commandInput, player->username, cmdResult.value());
    }
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

    // 距离校验：对齐 vanilla ServerGamePacketListenerImpl.handleInteract
    // (ServerGamePacketListenerImpl.java:729)：player.isWithinEntityInteractionRange(entity, 3.0)。
    // isWithinEntityInteractionRange 内部按 entityInteractionRange() 属性 + padding 计算
    // （生存 3.0、创造 5.0，padding 3.0 为容差），而非此处原先硬编码的 3.0+3.0=6.0。
    // 这样 generic.entity_interaction_range 属性（含创造模式 +2.0 修饰符）才真正生效。
    // ATTACK 走同样阈值（vanilla ATTACK 用 isWithinAttackRange，项目暂无 attack range
    // attribute，统一用交互阈值——TODO: 待引入 entity_attack_range 属性后分离）。
    if (!playerEntity->isWithinEntityInteractionRange(*target, 3.0)) {
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

void ServerPlayHandler::_triggerAnyBlockUse(Player& player)
{
    // 触发 default_block_use 成就（对齐 vanilla CriteriaTriggers.DEFAULT_BLOCK_USE）。
    // 任意方块使用时无条件触发，vanilla 在 consumesAction 时触发，
    // 此处近似为放置/使用方块成功后触发。
    auto* serverPlayer = player.asServerPlayer();
    if (serverPlayer == nullptr) {
        return;
    }
    auto* advancements = serverPlayer->getAdvancements();
    if (advancements == nullptr) {
        return;
    }
    auto* trigger = mc::advancement::CriterionTriggers::instance().getTrigger<mc::advancement::AnyBlockUseTrigger>();
    if (trigger == nullptr) {
        return;
    }
    trigger->AbstractCriterionTrigger<mc::advancement::AnyBlockUseTriggerInstance>::trigger(
        *advancements, [](const mc::advancement::AnyBlockUseTriggerInstance& instance) {
            (void)instance;
            return true;
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

    // 对齐 Java ServerGamePacketListenerImpl.handleUseItem(:1358)：紧跟登录检查之后
    // 无条件 ackBlockChangesUpTo(sequence)，业务结果不影响 ack。vanilla 取 max 累积、
    // 每 tick 末批量发，此处改为调 ServerPlayer::recordBlockChangeAck 累积，由 tick 末发送。
    auto* useItemWorld = m_server.getPlayerWorld(playerId);
    auto* useItemPlayer =
        (useItemWorld != nullptr) ? m_server.playerEntityManager().getPlayerEntity(playerId, *useItemWorld) : nullptr;
    if (useItemPlayer != nullptr) {
        if (auto* sp = useItemPlayer->asServerPlayer()) {
            sp->recordBlockChangeAck(evt->sequence);
        }
    }

    auto* world = useItemWorld;
    if (world == nullptr) {
        return;
    }

    auto* playerEntity = useItemPlayer;
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
    // 冷却门控（对齐 MC Java 1.21.11 ServerPlayerGameMode.useItem:298-299）：
    // 物品在冷却中时直接返回，不进入 onItemRightClick → setActiveHand。否则破盾（斧头
    // 100 tick 冷却）、风弹、紫颂果等设的冷却形同虚设——玩家冷却期内仍可立即重新使用。
    if (playerEntity->hasItemCooldown(heldItemC)) {
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

    // 1.21.11 PlayerAction 的 action 值（对齐 ServerboundPlayerActionPacket.Action 序数）：
    //   0=StartDestroyBlock 1=AbortDestroyBlock 2=StopDestroyBlock（带 sequence，走挖掘）
    //   3=DROP_ALL_ITEMS 4=DROP_ITEM 5=SWAP_ITEM_WITH_OFFHAND 6=RELEASE_USE_ITEM
    //   （不带 sequence，走物品逻辑）。此处仅转发挖掘相关，物品 action 在下方分流。
    const BlockPos pos = BlockPos::fromLong(evt->blockPosPacked);

    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.World,
        "ServerPlayHandler::handleBlockInteractionPacket",
        "pos",
        pos.toString(),
        "playerId",
        playerId,
        [flow = ::perfetto::Flow::ProcessScoped(pos.toId())](::perfetto::EventContext ctx) { flow(ctx); });

    // 物品相关 action 3-6（不带 sequence，不 ack）对齐 Java
    // ServerGamePacketListenerImpl.handlePlayerAction(:1245-1268)。
    if (evt->action == 3 || evt->action == 4 || evt->action == 5 || evt->action == 6) {
        handlePlayerItemAction(playerId, evt->action);
        return;
    }

    // 仅挖掘相关 action 0-2 走 MiningManager，其他非法值忽略
    if (evt->action < 0 || evt->action > 2) {
        return;
    }
    const auto action = static_cast<network::BlockInteractionAction>(evt->action);

    // 距离校验：对齐 vanilla ServerPlayerGameMode.handleBlockBreakAction /
    // ServerGamePacketListenerImpl(:695) 的 player.isWithinBlockInteractionRange(pos, 1.0)。
    // START_DESTROY 与 STOP_DESTROY 都须在交互距离内（padding 1.0 容差），否则忽略，
    // 防止玩家远程挖方块。此前 MiningManager 路径完全无距离门控。
    // ABORT_DESTROY 无目标方块距离语义，跳过校验。
    auto* interactWorld = m_server.getPlayerWorld(playerId);
    auto* interactPlayer =
        (interactWorld != nullptr) ? m_server.playerEntityManager().getPlayerEntity(playerId, *interactWorld) : nullptr;
    if (action != network::BlockInteractionAction::AbortDestroyBlock) {
        if (interactPlayer == nullptr || !interactPlayer->isWithinBlockInteractionRange(pos, 1.0)) {
            return;
        }
    }

    // 对齐 Java ServerGamePacketListenerImpl.handlePlayerAction(:1277)：仅
    // START/ABORT/STOP_DESTROY 三个 action（带 sequence）调 ack；DROP/SWAP/
    // RELEASE_USE_ITEM 不带 sequence 不 ack。业务结果不影响 ack。
    // vanilla 取 max 累积、每 tick 末批量发，此处改为调 ServerPlayer::recordBlockChangeAck
    // 累积，由 tick 末发送。
    if (interactPlayer != nullptr) {
        if (auto* sp = interactPlayer->asServerPlayer()) {
            sp->recordBlockChangeAck(evt->sequence);
        }
    }

    // 处理挖掘状态
    m_server.miningManager().handleBlockInteraction(playerId, pos, action);

    if (action == network::BlockInteractionAction::StopDestroyBlock) {
        // tryCompleteMining 需要 world 引用以计算挖掘速度（0.7 阈值判定）。
        // 返回 false 表示进度不足转 delayed-destroy 续挖、或状态不存在/位置不匹配，
        // 此处仅 debug 级别记录，不再 warn（delayed-destroy 是正常语义）。
        if (interactWorld != nullptr) {
            m_server.miningManager().tryCompleteMining(playerId, pos, *interactWorld);
        }
    }
}

void ServerPlayHandler::handlePlayerItemAction(PlayerId playerId, i32 action)
{
    // 对齐 Java ServerGamePacketListenerImpl.handlePlayerAction(:1245-1268) 的物品分支。
    // action: 3=DROP_ALL_ITEMS 4=DROP_ITEM 5=SWAP_ITEM_WITH_OFFHAND 6=RELEASE_USE_ITEM。
    // 这些 action 不带 sequence、不 ack，不走 MiningManager。

    auto* world = m_server.getPlayerWorld(playerId);
    auto* player = (world != nullptr) ? m_server.playerEntityManager().getPlayerEntity(playerId, *world) : nullptr;
    if (player == nullptr) {
        return;
    }

    switch (action) {
        case 3: // DROP_ALL_ITEMS：丢弃整组
            if (!player->isSpectator()) {
                player->drop(true);
                m_server.syncPlayerInventory(playerId);
            }
            break;

        case 4: // DROP_ITEM：丢弃一个
            if (!player->isSpectator()) {
                player->drop(false);
                m_server.syncPlayerInventory(playerId);
            }
            break;

        case 5: // SWAP_ITEM_WITH_OFFHAND：主副手交换
            if (!player->isSpectator()) {
                // 对齐 Java :1259-1265：交换主手与副手物品
                ItemStack& mainHand = player->inventory().getSelectedStackRef();
                ItemStack& offhand = player->inventory().getOffhandItemRef();
                std::swap(mainHand, offhand);
                // 停止使用物品（若正在使用）——对齐 Java player.stopUsingItem()。
                // 本项目物品使用状态机已实现（LivingEntity::stopActiveHand），交换手时
                // 应停止当前物品使用。
                if (player->isUsingItem()) {
                    player->stopActiveHand();
                }
                m_server.syncPlayerInventory(playerId);
            }
            break;

        case 6: // RELEASE_USE_ITEM：停止使用物品
            // 对齐 Java :1266-1268 player.releaseUsingItem()。
            // 本项目用 stopActiveHand 对应停止使用（释放时若仍在使用则停止，不触发完成）。
            if (player->isUsingItem()) {
                player->stopActiveHand();
                m_server.syncPlayerInventory(playerId);
            }
            break;

        default:
            break;
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

    // 对齐 Java ServerGamePacketListenerImpl.handleUseItemOn(:1299)：紧跟登录检查
    // 之后无条件 ackBlockChangesUpTo(sequence)，业务结果不影响 ack。否则真 Java
    // 客户端方块预测状态机因收不到 ack 而卡死，后续右键静默失效。
    // vanilla 取 max 累积、每 tick 末批量发，此处改为调 ServerPlayer::recordBlockChangeAck
    // 累积，由 tick 末发送。
    auto* placementWorld = m_server.getPlayerWorld(playerId);
    auto* placementPlayer = (placementWorld != nullptr)
        ? m_server.playerEntityManager().getPlayerEntity(playerId, *placementWorld)
        : nullptr;
    if (placementPlayer != nullptr) {
        if (auto* sp = placementPlayer->asServerPlayer()) {
            sp->recordBlockChangeAck(evt->sequence);
        }
    }

    const auto& hit = evt->blockHit;
    const BlockPos pos = BlockPos::fromLong(hit.blockPosPacked);
    auto* playerWorld = placementWorld;
    const BlockState* clickedState = playerWorld ? playerWorld->getBlockState(pos) : nullptr;
    const Hand hand = (evt->hand == static_cast<i32>(Hand::OffHand)) ? Hand::OffHand : Hand::MainHand;
    const Direction face = static_cast<Direction>(hit.direction);
    const Vector3 hitPosition(hit.hitX, hit.hitY, hit.hitZ);

    // 对齐 vanilla ServerGamePacketListenerImpl.java:1308-1310：命中点相对方块中心
    // 的偏移三轴均须 < 1.0000001，否则 reject（反作弊，防止伪造偏离目标方块的
    // hitPos 绕过放置几何约束）。ACK 已在前面无条件累积，业务结果不影响 ack。
    {
        const Vector3 blockCenter(
            static_cast<f32>(pos.x) + 0.5f, static_cast<f32>(pos.y) + 0.5f, static_cast<f32>(pos.z) + 0.5f);
        const Vector3 offset = hitPosition - blockCenter;
        constexpr f32 HIT_PRECISION = 1.0000001f;
        if (std::abs(offset.x) >= HIT_PRECISION || std::abs(offset.y) >= HIT_PRECISION ||
            std::abs(offset.z) >= HIT_PRECISION) {
            spdlog::warn("Rejecting UseItemOn from player {}: hit ({}, {}, {}) too far from block ({}, {}, {})",
                playerId,
                hitPosition.x,
                hitPosition.y,
                hitPosition.z,
                pos.x,
                pos.y,
                pos.z);
            return;
        }
    }

    // 对齐 vanilla ServerGamePacketListenerImpl.java:1313-1314：Y 上限校验。
    // getMaxBuildHeight() 为独占上界（主世界 320），getMaxY()=getMaxBuildHeight()-1=319。
    // 被点击方块 Y 超过 319 则禁止放置并发 build.tooHigh 红字提示。
    if (playerWorld != nullptr) {
        const i32 maxY = playerWorld->getMaxBuildHeight() - 1;
        if (pos.y > maxY) {
            if (placementPlayer != nullptr) {
                if (auto* sp = placementPlayer->asServerPlayer()) {
                    // TODO: 对齐 vanilla Component.translatable("build.tooHigh", i)。
                    //       当前 sendSystemMessage 仅支持纯文本，待补齐翻译组件序列化后改为翻译键。
                    sp->sendSystemMessage("§cCannot place block above build height (" + std::to_string(maxY) + ")");
                }
            }
            return;
        }
    }

    // 对齐 vanilla ServerGamePacketListenerImpl.java:1315 awaitingPositionFromClient == null：
    // 玩家在等待传送确认期间拒绝交互，避免传送瞬间的位置竞态。
    if (m_server.teleportManager().isWaitingForConfirm(playerId)) {
        return;
    }

    // RAII 守卫：方法返回时（无论放置成功失败）定向给操作玩家发两个 BlockUpdate，
    // 作为客户端预测状态机的硬同步兜底（对齐 vanilla :1339-1340）。
    struct PlacementBlockUpdateGuard {
        ServerPlayHandler& self;
        PlayerId playerId;
        ServerWorld* world;
        BlockPos pos;
        Direction face;
        ~PlacementBlockUpdateGuard()
        {
            if (world != nullptr) {
                sendPlacementBlockUpdates(self.m_server, playerId, *world, pos, face);
            }
        }
    };
    PlacementBlockUpdateGuard guard{*this, playerId, placementWorld, pos, face};
    (void)guard;

    const auto tryOpenCrafting = [this, playerId, pos, clickedState]() {
        return isCraftingTableState(clickedState) && m_server.tryOpenCraftingContainer(playerId, pos);
    };

    ItemStack heldStack = m_server.getHeldItemForPlacement(playerId);
    if (heldStack.isEmpty()) {
        if (!tryOpenCrafting()) {
            const auto useResult =
                m_server.blockInteractionManager().handleBlockUse(playerId, pos, hand, hitPosition, face);
            // 对齐 vanilla :1319-1321 consumesAction 时触发 DEFAULT_BLOCK_USE。
            if (useResult.success() && useResult.value().success && placementPlayer != nullptr) {
                _triggerAnyBlockUse(*placementPlayer);
            }
        }
        return;
    }

    // 冷却门控（对齐 MC Java 1.21.11 ServerPlayerGameMode.useItemOn:366）：
    // 物品在冷却中时不派发 useOn/useItem，否则破盾等冷却形同虚设。需取 Player 实体
    // 调 hasItemCooldown（ServerPlayer 是网络会话对象，无冷却接口）。
    if (playerWorld != nullptr) {
        Player* playerEntity = m_server.playerEntityManager().getPlayerEntity(playerId, *playerWorld);
        if (playerEntity != nullptr) {
            if (const Item* heldItemForCooldown = heldStack.getItem();
                heldItemForCooldown != nullptr && playerEntity->hasItemCooldown(heldItemForCooldown)) {
                return;
            }
        }
    }

    const Item* heldItem = heldStack.getItem();
    const bool holdingBlockItem =
        heldItem != nullptr && BlockItemRegistry::instance().getBlockItemByItemId(heldItem->itemId()) != nullptr;

    if (!holdingBlockItem) {
        if (tryOpenCrafting()) {
            return;
        }
        // ② vanilla useWithoutItem（项目 onBlockActivated）。方块交互已处理则短路，不派发 ③。
        auto useResult = m_server.blockInteractionManager().handleBlockUse(playerId, pos, hand, hitPosition, face);
        if (useResult.success() && useResult.value().success) {
            // 对齐 vanilla :1319-1321 consumesAction 时触发 DEFAULT_BLOCK_USE。
            if (placementPlayer != nullptr) {
                _triggerAnyBlockUse(*placementPlayer);
            }
            return;
        }
        // ③ vanilla Item.useOn（矿车/骨粉/桶/火把/锄头等非 block-item 靠此步生效）。
        // handleItemUseOn 内部经 InventoryManager.syncToClient 已同步物品栏，外层无需再同步。
        const auto itemResult =
            m_server.blockInteractionManager().handleItemUseOn(playerId, pos, hitPosition, face, hand, heldStack);
        // 对齐 vanilla :1319-1321 consumesAction 时触发 DEFAULT_BLOCK_USE。
        // ItemUseResult::success 表示 onItemUse 返回 Success/Consume（即 consumesAction）。
        if (itemResult.success() && itemResult.value().success && placementPlayer != nullptr) {
            _triggerAnyBlockUse(*placementPlayer);
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

    // 对齐 vanilla ServerGamePacketListenerImpl.java:1329-1332 swingSource==SERVER 时 swing。
    // 放置成功后主动挥臂并广播给其他玩家。本项目 BlockPlacementResult 不携带 swingSource，
    // 当前用放置成功近似。
    // TODO: 待补齐 ActionResultType/swingSource 字段后精确判断 swing 时机。
    if (interactionResult.success() && interactionResult.value().blockPlaced && placementPlayer != nullptr) {
        placementPlayer->swing(hand);
    }

    // 对齐 vanilla ServerGamePacketListenerImpl.java:1319-1321 consumesAction 时触发
    // DEFAULT_BLOCK_USE（任意方块使用成就）。放置成功即视为 consumesAction。
    if (interactionResult.success() && interactionResult.value().blockPlaced && placementPlayer != nullptr) {
        _triggerAnyBlockUse(*placementPlayer);
    }
}

} // namespace mc::server::net
