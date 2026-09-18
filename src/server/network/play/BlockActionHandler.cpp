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

#include "server/network/play/BlockActionHandler.hpp"

#include "common/advancement/trigger/CriterionTrigger.hpp"
#include "common/advancement/trigger/CriterionTriggers.hpp"
#include "common/advancement/trigger/impl/AnyBlockUseTrigger.hpp"
#include "common/core/Types.hpp"
#include "common/entity/entities/player/Player.hpp"
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
#include "common/util/math/Vector3.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/blockentity/BlockEntity.hpp"
#include "common/world/blockentity/BlockEntityType.hpp"
#include "common/world/blockentity/interactive/SignEntity.hpp"
#include "common/world/entity/EntityManager.hpp"
#include "server/application/MinecraftServer.hpp"
#include "server/core/PlayerManager.hpp"
#include "server/core/ServerPlayerData.hpp"
#include "server/interaction/BlockInteractionManager.hpp"
#include "server/interaction/MiningManager.hpp"
#include "server/player/ServerPlayer.hpp"
#include "server/world/ServerWorld.hpp"
#include "server/world/player/ServerPlayerEntityManager.hpp"
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

void BlockActionHandler::handleBlockInteractionPacket(PlayerId playerId, const mc::network::ir::IrPacket& packet)
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
        "BlockActionHandler::handleBlockInteractionPacket",
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

void BlockActionHandler::handlePlayerItemAction(PlayerId playerId, i32 action)
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

void BlockActionHandler::handleBlockPlacementPacket(PlayerId playerId, const mc::network::ir::IrPacket& packet)
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
    // 协议层 hitX/hitY/hitZ 是相对方块原点（min corner）的偏移（值域约 0~1），
    // 对齐 vanilla FriendlyByteBuf.readBlockHitResult：重建世界绝对命中点
    // Vec3(blockpos + f) 后再做校验。
    const Vector3 hitPosition(
        static_cast<f32>(pos.x) + hit.hitX, static_cast<f32>(pos.y) + hit.hitY, static_cast<f32>(pos.z) + hit.hitZ);

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
        MinecraftServer& server;
        PlayerId playerId;
        ServerWorld* world;
        BlockPos pos;
        Direction face;
        ~PlacementBlockUpdateGuard()
        {
            if (world != nullptr) {
                sendPlacementBlockUpdates(server, playerId, *world, pos, face);
            }
        }
    };
    PlacementBlockUpdateGuard guard{m_server, playerId, placementWorld, pos, face};
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

void BlockActionHandler::handleUseItemPacket(PlayerId playerId, const mc::network::ir::IrPacket& packet)
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

void BlockActionHandler::handleUpdateSignPacket(PlayerId playerId, const mc::network::ir::IrPacket& packet)
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

void BlockActionHandler::_triggerAnyBlockUse(Player& player)
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

} // namespace mc::server::net
