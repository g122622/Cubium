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

#include "BlockInteractionManager.hpp"
#include "InventoryManager.hpp"
#include "SignCommandHelper.hpp"
#include "common/core/BlockRaycastResult.hpp"
#include "common/core/Constants.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/entity/ecs/context/EntityRegistry.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/inventory/PlayerInventory.hpp"
#include "common/item/context/BlockItemUseContext.hpp"
#include "common/item/context/ItemUseContext.hpp"
#include "common/item/core/ActionResult.hpp"
#include "common/item/core/BlockActionResult.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/core/ItemRegistry.hpp"
#include "common/item/items/block/BlockItem.hpp"
#include "common/item/items/block/BlockItemRegistry.hpp"
#include "common/mod/bedrock/addon/component/BlockComponentEvents.hpp"
#include "common/mod/bedrock/addon/component/BlockComponentRegistry.hpp"
#include "common/mod/bedrock/addon/component/ItemComponentEvents.hpp"
#include "common/mod/bedrock/addon/component/ItemComponentRegistry.hpp"
#include "common/network/protocol/GameActions.hpp"
#include "common/profiler/TraceCategories.hpp"
#include "common/profiler/TraceEvents.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/AxisAlignedBB.hpp"
#include "common/util/Direction.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/blockentity/BlockEntity.hpp"
#include "common/world/blockentity/BlockEntityType.hpp"
#include "common/world/blockentity/interactive/SignEntity.hpp"
#include "server/application/IServer.hpp"
#include "server/core/PlayerManager.hpp"
#include "server/core/ServerPlayerData.hpp"
#include "server/player/ServerPlayer.hpp"
#include "server/world/ServerWorld.hpp"
#include "server/world/drop/BlockDropHandler.hpp"
#include "server/world/player/ServerPlayerEntityManager.hpp"
#include <functional>
#include <optional>
#include <string>
#include <utility>

using namespace mc::trace;

namespace mc::server::interaction {

// ============================================================================
// 辅助方法实现
// ============================================================================

ServerPlayerData* BlockInteractionManager::_validatePlayer(PlayerId playerId) const noexcept
{
    auto* playerData = m_playerManager.getPlayer(playerId);
    if (!playerData || !playerData->loggedIn) {
        return nullptr;
    }
    return playerData;
}

std::optional<Error> BlockInteractionManager::_validateInteractionPreconditions(
    PlayerId playerId, const BlockPos& pos, bool checkYRange) const
{
    // 验证玩家
    auto* playerData = _validatePlayer(playerId);
    if (!playerData) {
        return Error(ErrorCode::InvalidArgument, "Player not found or not logged in");
    }

    // 获取玩家实体以走属性驱动的距离判定（generic.block_interaction_range 属性）
    Player* player = nullptr;
    if (ServerWorld* world = _getPlayerWorld(playerId); world != nullptr) {
        player = _getPlayerEntity(playerId, *world);
    }

    // 验证距离
    if (!_canInteract(player, playerId, pos)) {
        return Error(ErrorCode::InvalidArgument, "Block too far away");
    }

    // 验证 Y 范围（可选）
    if (checkYRange) {
        if (pos.y < world::MIN_BUILD_HEIGHT || pos.y >= world::MAX_BUILD_HEIGHT) {
            return Error(ErrorCode::InvalidArgument, "Block Y out of range");
        }
    }

    return std::nullopt;
}

const BlockState* BlockInteractionManager::_getNonAirBlockState(ServerWorld& world, const BlockPos& pos) const noexcept
{
    const BlockState* state = world.getBlockState(pos);
    if (!state || state->isAir()) {
        return nullptr;
    }
    return state;
}

std::optional<Error> BlockInteractionManager::_checkWorldModificationAllowed(ServerWorld& world) const noexcept
{
    if (world.isDebugWorld()) {
        return Error(ErrorCode::PermissionDenied, "Cannot modify blocks in debug world");
    }
    return std::nullopt;
}

ItemStack BlockInteractionManager::_getHeldTool(PlayerId playerId) const noexcept
{
    if (!m_inventoryManager) {
        return ItemStack();
    }
    return m_inventoryManager->getHeldItem(playerId);
}

Player* BlockInteractionManager::_getPlayerEntity(PlayerId playerId, ServerWorld& world) const noexcept
{
    if (m_server == nullptr) {
        return nullptr;
    }

    return m_server->playerEntityManager().getPlayerEntity(playerId, world);
}

u32 BlockInteractionManager::_setBlockToAir(
    ServerWorld& world, const BlockPos& pos, const BlockState& oldState, PlayerId playerId)
{
    Block* airBlock = Block::getBlock(ResourceLocation("minecraft:air"));
    if (!airBlock) {
        return 0;
    }

    world.setBlockState(pos, &airBlock->defaultState());

    if (m_onBlockBreak) {
        m_onBlockBreak(playerId, pos, oldState);
    }

    return airBlock->defaultState().stateId();
}

// ============================================================================
// 构造函数
// ============================================================================

BlockInteractionManager::BlockInteractionManager(
    core::PlayerManager& playerManager, loot::LootTableManager& lootTableManager)
    : m_playerManager(playerManager)
    , m_lootTableManager(lootTableManager)
{}

ServerWorld* BlockInteractionManager::_getPlayerWorld(PlayerId playerId) const noexcept
{
    if (m_server == nullptr) {
        return nullptr;
    }
    return m_server->getPlayerWorld(playerId);
}

void BlockInteractionManager::setInventoryManager(InventoryManager* inventoryManager)
{
    m_inventoryManager = inventoryManager;
}

// ============================================================================
// 公共方法实现
// ============================================================================

Result<BlockInteractionResult> BlockInteractionManager::handleBlockInteraction(
    PlayerId playerId, const BlockPos& pos, network::BlockInteractionAction action)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.World,
        "BlockInteractionManager::handleBlockInteraction",
        "pos",
        pos.toString(),
        "playerId",
        playerId,
        "action",
        static_cast<u8>(action),
        [flow = ::perfetto::Flow::ProcessScoped(pos.toId())](::perfetto::EventContext ctx) { flow(ctx); });

    // 验证前置条件
    auto preconditionError = _validateInteractionPreconditions(playerId, pos, true);
    if (preconditionError) {
        return std::move(*preconditionError);
    }

    ServerWorld* world = _getPlayerWorld(playerId);
    if (world == nullptr) {
        return Error(ErrorCode::InvalidWorld, "Player world not available");
    }

    // 获取方块状态
    const BlockState* state = _getNonAirBlockState(*world, pos);
    if (!state) {
        return BlockInteractionResult{false, "No block to interact with"};
    }

    // 处理不同动作
    switch (action) {
        case network::BlockInteractionAction::StartDestroyBlock:
            // 开始破坏 - 通常由 MiningManager 处理
            break;

        case network::BlockInteractionAction::AbortDestroyBlock:
            // 中止破坏
            break;

        case network::BlockInteractionAction::StopDestroyBlock:
            // 完成破坏
            if (_canBreakBlock(*world, playerId, pos, state)) {
                // 获取手持物品作为工具
                ItemStack tool = _getHeldTool(playerId);

                // 通知方块玩家即将破坏（如活塞头在创造模式下级联销毁活塞基座）
                Player* stopPlayer = _getPlayerEntity(playerId, *world);
                if (stopPlayer != nullptr) {
                    state->getBlockMutable().playerWillDestroy(*world, pos, *state, *stopPlayer);
                }

                // 创造模式不产生掉落物
                bool isCreativeStop = stopPlayer != nullptr && stopPlayer->isCreative();
                if (!isCreativeStop) {
                    // 生成掉落物
                    _generateBlockDrops(*world, pos, *state, playerId, tool.isEmpty() ? nullptr : &tool);
                }

                // 设置为空气
                _setBlockToAir(*world, pos, *state, playerId);

                // 调用方块的 spawnAfterBreak 回调（如 InfestedBlock 生成蠹虫）
                // 方块已移除后调用，与 MC Java 行为一致：先移除方块再生成额外实体
                const Block& breakBlock = state->getBlock();
                breakBlock.spawnAfterBreak(*world, pos, *state, tool.isEmpty() ? nullptr : &tool, true);

                return BlockInteractionResult{true, "Block destroyed"};
            }
            break;

        default:
            break;
    }

    return BlockInteractionResult{false, "Action not handled"};
}

Result<BlockPlacementResult> BlockInteractionManager::handleBlockPlacement(
    PlayerId playerId, const BlockPos& pos, const Vector3& hitPos, Direction face, const ItemStack& heldItem)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.World,
        "BlockInteractionManager::handleBlockPlacement",
        "pos",
        pos.toString(),
        "playerId",
        playerId,
        "face",
        Directions::toString(face),
        [flow = ::perfetto::Flow::ProcessScoped(pos.toId())](::perfetto::EventContext ctx) { flow(ctx); });

    // 检查世界修改权限
    ServerWorld* world = _getPlayerWorld(playerId);
    if (world == nullptr) {
        return Error(ErrorCode::InvalidWorld, "Player world not available");
    }

    auto worldError = _checkWorldModificationAllowed(*world);
    if (worldError) {
        return std::move(*worldError);
    }

    // 验证前置条件（不需要检查 Y 范围，因为放置位置可能在不同高度）
    auto preconditionError = _validateInteractionPreconditions(playerId, pos, false);
    if (preconditionError) {
        return std::move(*preconditionError);
    }

    // 获取玩家数据用于游戏模式检查
    auto* playerData = _validatePlayer(playerId);
    if (!playerData) {
        return Error(ErrorCode::InvalidArgument, "Player not found or not logged in");
    }

    // 检查游戏模式
    if (playerData->gameMode == GameMode::Spectator) {
        return Error(ErrorCode::PermissionDenied, "Cannot place blocks in spectator mode");
    }

    // 检查建造权限和冒险模式 CanPlaceOn 限制
    Player* player = _getPlayerEntity(playerId, *world);
    if (player != nullptr && !player->mayUseItemAt(*world, pos, face, heldItem)) {
        return Error(ErrorCode::PermissionDenied, "Cannot place block: mayUseItemAt check failed");
    }

    // 获取物品对应的方块物品
    const Item* item = heldItem.getItem();
    if (!item) {
        return Error(ErrorCode::InvalidArgument, "No item in hand");
    }

    const BlockItem* blockItem = BlockItemRegistry::instance().getBlockItemByItemId(item->itemId());
    if (!blockItem) {
        return Error(ErrorCode::InvalidArgument, "Item is not a block item");
    }

    // 创建放置上下文（player 为 nullptr，因为我们通过 InventoryManager 管理）
    // playerData 同时提供 yaw 与 pitch，pitch 用于 getNearestLookingDirections 的方向排序
    BlockItemUseContext context(*world, nullptr, heldItem, hitPos, pos, face, playerData->yaw, playerData->pitch);

    // 先检查是否可以放置（位置有效性检查）
    if (!context.canPlace()) {
        return BlockPlacementResult{false, false, false, pos, 0, "Cannot place block here"};
    }

    // 获取放置位置和状态（不实际放置）
    const BlockPos& placePos = context.placementPos();
    const BlockState* newState = blockItem->getStateForPlacement(context);
    if (!newState) {
        return BlockPlacementResult{false, false, false, pos, 0, "No placement state"};
    }

    // 在放置之前检查玩家碰撞
    if (_wouldCollideWithPlayer(playerId, placePos, *newState)) {
        return BlockPlacementResult{false, false, false, placePos, 0, "Cannot place block inside player"};
    }

    // 执行放置
    ActionResultType result = blockItem->tryPlace(context);
    if (result != ActionResultType::Success && result != ActionResultType::Consume) {
        return BlockPlacementResult{false, false, false, pos, 0, "Cannot place block here"};
    }

    // 消耗物品（非创造模式）
    bool itemConsumed = false;
    if (playerData->gameMode != GameMode::Creative && m_inventoryManager) {
        // 获取玩家物品栏
        PlayerInventory* inventory = m_inventoryManager->getInventory(playerId);
        if (inventory) {
            // 减少手持物品数量
            ItemStack selectedStack = inventory->getSelectedStack();
            if (!selectedStack.isEmpty() && selectedStack.getCount() > 0) {
                selectedStack.shrink(1);
                inventory->setItem(inventory->getSelectedSlot(), selectedStack);
                itemConsumed = true;
                // 同步到客户端
                m_inventoryManager->syncToClient(playerId);
            }
        }
    } else if (playerData->gameMode == GameMode::Creative) {
        itemConsumed = true; // 创造模式不实际消耗
    }

    if (m_onBlockPlace) {
        m_onBlockPlace(playerId, placePos, *newState);
    }

    return BlockPlacementResult{true, true, itemConsumed, placePos, newState->stateId(), "Block placed"};
}

Result<ItemUseResult> BlockInteractionManager::handleItemUseOn(
    PlayerId playerId, const BlockPos& pos, const Vector3& hitPos, Direction face, Hand hand, const ItemStack& heldItem)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.World,
        "BlockInteractionManager::handleItemUseOn",
        "pos",
        pos.toString(),
        "playerId",
        playerId,
        "face",
        Directions::toString(face),
        [flow = ::perfetto::Flow::ProcessScoped(pos.toId())](::perfetto::EventContext ctx) { flow(ctx); });

    ServerWorld* world = _getPlayerWorld(playerId);
    if (world == nullptr) {
        return Error(ErrorCode::InvalidWorld, "Player world not available");
    }

    auto worldError = _checkWorldModificationAllowed(*world);
    if (worldError) {
        return std::move(*worldError);
    }

    // 验证前置条件（不需要检查 Y 范围，与 handleBlockPlacement 一致）
    auto preconditionError = _validateInteractionPreconditions(playerId, pos, false);
    if (preconditionError) {
        return std::move(*preconditionError);
    }

    auto* playerData = _validatePlayer(playerId);
    if (!playerData) {
        return Error(ErrorCode::InvalidArgument, "Player not found or not logged in");
    }

    // 旁观模式不派发 Item.useOn（对齐 vanilla ServerPlayerGameMode.useItemOn 旁观分支）
    if (playerData->gameMode == GameMode::Spectator) {
        return Error(ErrorCode::PermissionDenied, "Cannot use item in spectator mode");
    }

    Player* player = _getPlayerEntity(playerId, *world);
    if (player != nullptr && !player->mayUseItemAt(*world, pos, face, heldItem)) {
        return Error(ErrorCode::PermissionDenied, "Cannot use item: mayUseItemAt check failed");
    }

    // Item 是无状态策略单例，onItemUse 非 const；经 ItemRegistry 取非 const 句柄调用。
    const Item* itemC = heldItem.getItem();
    if (itemC == nullptr) {
        return Error(ErrorCode::InvalidArgument, "No item in hand");
    }
    Item* item = ItemRegistry::instance().getItem(itemC->itemId());
    if (item == nullptr) {
        return Error(ErrorCode::NotFound, "Item not found in registry");
    }

    // 构造使用上下文。传真实 player（对齐 handleBlockUse:509 的 realPlayer 范式），使 onItemUse 内
    // 可通过 context.getPlayer()->getHeldItem(hand) 直接操作权威手持物——这对"消耗原物品并返回新物品"
    // 的物品（FishBucketItem 鱼桶→空桶）是必需的：此类物品在 onItemUse 内替换手持物，外层无法仅靠
    // shrink 复现。此前传 nullptr 致 FishBucketItem.onItemUse 的 _returnEmptyBucket 不执行（player==nullptr
    // 守卫跳过），生产路径玩家持鱼桶右键地面失去鱼桶却得不到空桶（对齐缺陷）。
    // heldItem 是调用方局部拷贝，onItemUse 内 context.getItemStackMut() 修改的是该拷贝，不回写权威物品栏——
    // 故消耗/替换仍以 player 权威手持物为准，下方同步回 InventoryManager。
    // 调用前先同步 InventoryManager→Player（双数据源，InventoryManager 为权威，对齐 handleBlockUse:515-525）。
    if (player != nullptr && m_inventoryManager != nullptr) {
        PlayerInventory* mgrInventory = m_inventoryManager->getInventory(playerId);
        if (mgrInventory != nullptr) {
            player->inventory().setSelectedSlot(mgrInventory->getSelectedSlot());
            player->inventory().setItem(mgrInventory->getSelectedSlot(), mgrInventory->getSelectedStack());
            player->setGameMode(playerData->gameMode);
        }
    }
    // 记录 onItemUse 前权威槽 itemId+damage：部分物品在 onItemUse 内通过 player->getHeldItem 改权威
    // （Player 镜像）手持物——(1)自管理替换（鱼桶→空桶，itemId 变化）；(2)耐久损耗（打火石/锄/斧/锹
    // hurtAndBreak，damage 变化，itemId 不变）。两种情况外层都不应再 shrink（否则误消耗返回物或把耐久
    // 损耗误当数量消耗）。骨粉等"仅 shrink 原物品不替换不改耐久"的物品 itemId+damage 均不变，仍走外层
    // shrink 补足。从 Player 镜像读（onItemUse 前 InventoryManager 已同步到 Player，两者一致）。
    const ItemId itemIdBefore = [&] {
        if (player == nullptr) return ItemId{0};
        ItemStack sel = player->inventory().getSelectedStack();
        return sel.isEmpty() ? ItemId{0} : sel.getItem()->itemId();
    }();
    const i32 damageBefore = [&] {
        if (player == nullptr) return 0;
        ItemStack sel = player->inventory().getSelectedStack();
        return sel.isEmpty() ? 0 : sel.getDamage();
    }();
    ItemUseContext context(*world, player, heldItem, hitPos, pos, face, hand, playerData->yaw, playerData->pitch);

    ActionResultType result = item->onItemUse(context);
    const bool success = (result == ActionResultType::Success || result == ActionResultType::Consume);

    // 消耗权威物品栏（仅 success 时，对齐 handleBlockPlacement:350-368 的消耗范式）。
    // TODO(副手消耗): 当前 getSelectedStack 取主手槽位，hand==OffHand 时会误消耗主手。
    // vanilla 副手 useOn 场景极少（矿车等通常主手），首版接受此限制。
    bool itemConsumed = false;
    if (success && playerData->gameMode != GameMode::Creative && m_inventoryManager != nullptr) {
        PlayerInventory* inventory = m_inventoryManager->getInventory(playerId);
        if (inventory != nullptr) {
            // 若 onItemUse 已通过 player->getHeldItem 改 Player 镜像手持物（itemId 变化=自管理替换，
            // 或 damage 变化=耐久损耗），把 Player 镜像同步回 InventoryManager，跳过 shrink（物品已自管理
            // 消耗/损耗）。否则（itemId+damage 均不变，如骨粉仅 shrink 拷贝）走原 shrink(1) 补足权威槽消耗。
            // 注意：从 Player 镜像读 after 值（onItemUse 改的是 Player 镜像，InventoryManager 权威尚未同步）。
            const ItemStack playerHeldAfter = player != nullptr ? player->inventory().getSelectedStack() : ItemStack();
            const ItemId itemIdAfter = playerHeldAfter.isEmpty() ? ItemId{0} : playerHeldAfter.getItem()->itemId();
            const i32 damageAfter = playerHeldAfter.isEmpty() ? 0 : playerHeldAfter.getDamage();
            const bool selfManaged = (itemIdAfter != itemIdBefore) || (damageAfter != damageBefore);
            if (player != nullptr && selfManaged) {
                // onItemUse 自管理了消耗/损耗：同步 Player→InventoryManager（对齐 handleBlockUse:617-625）。
                inventory->setItem(inventory->getSelectedSlot(), playerHeldAfter);
                itemConsumed = true;
                m_inventoryManager->syncToClient(playerId);
            } else if (!playerHeldAfter.isEmpty() && playerHeldAfter.getCount() > 0) {
                ItemStack shrunk = playerHeldAfter;
                shrunk.shrink(1);
                inventory->setItem(inventory->getSelectedSlot(), shrunk);
                itemConsumed = true;
                m_inventoryManager->syncToClient(playerId);
            }
        }
    } else if (success && playerData->gameMode == GameMode::Creative) {
        itemConsumed = true; // 创造模式不实际消耗
    }

    return ItemUseResult{success, itemConsumed, result, success ? "Item used on block" : "Item use pass"};
}

Result<BlockInteractionResult> BlockInteractionManager::handleBlockUse(
    PlayerId playerId, const BlockPos& pos, Hand hand, const Vector3& hitPos, Direction face)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.World,
        "BlockInteractionManager::handleBlockUse",
        "pos",
        pos.toString(),
        "playerId",
        playerId,
        "hand",
        hand == Hand::MainHand ? "main" : "off",
        [flow = ::perfetto::Flow::ProcessScoped(pos.toId())](::perfetto::EventContext ctx) { flow(ctx); });

    // 验证前置条件（方块使用不需要检查 Y 范围）
    auto preconditionError = _validateInteractionPreconditions(playerId, pos, false);
    if (preconditionError) {
        return std::move(*preconditionError);
    }

    ServerWorld* world = _getPlayerWorld(playerId);
    if (world == nullptr) {
        return Error(ErrorCode::InvalidWorld, "Player world not available");
    }

    // 获取方块状态
    const BlockState* state = _getNonAirBlockState(*world, pos);
    if (!state) {
        return BlockInteractionResult{false, "No block to use"};
    }

    Block* block = Block::getBlock(state->blockId());
    if (!block) {
        return Error(ErrorCode::NotFound, "Block not found for state");
    }

    // 获取玩家数据用于交互
    auto* playerData = _validatePlayer(playerId);
    if (!playerData) {
        return Error(ErrorCode::InvalidArgument, "Player not found or not logged in");
    }

    // 获取真实的玩家实体（携带 m_inventory、m_gameMode 等状态）
    // 之前的实现使用临时 Player，其 m_inventory 为空且 m_gameMode 为默认值，
    // 导致 ShelfBlock 等方块的 onBlockActivated 修改无法生效到真实玩家。
    Player* realPlayer = _getPlayerEntity(playerId, *world);
    const BlockRaycastResult hitResult = BlockRaycastResult::hit(hitPos, pos, face, 0.0f);

    // 在调用 onBlockActivated 之前，需要保证 Player::m_inventory 与
    // InventoryManager::m_inventories 中的手持物品一致（双数据源同步）。
    // InventoryManager 是服务端权威数据源，因此先将其手持物品同步到 Player。
    if (realPlayer != nullptr && m_inventoryManager != nullptr) {
        PlayerInventory* mgrInventory = m_inventoryManager->getInventory(playerId);
        if (mgrInventory != nullptr) {
            // 同步选中槽位和手持物品
            realPlayer->inventory().setSelectedSlot(mgrInventory->getSelectedSlot());
            ItemStack heldItem = mgrInventory->getSelectedStack();
            realPlayer->inventory().setItem(mgrInventory->getSelectedSlot(), heldItem);
            // 同步游戏模式
            realPlayer->setGameMode(playerData->gameMode);
        }
    }

    BlockActionResult result = [&]() -> BlockActionResult {
        if (realPlayer != nullptr) {
            return block->onBlockActivated(*state, *world, pos, *realPlayer, hand, hitResult);
        }
        // 回退路径：无法获取真实玩家实体时使用临时 Player（保持向后兼容）
        // ECS 迁移：占位 Player 构造需要 registry 句柄，此处无 world 上下文故配静态 registry。
        // TODO: 占位 Player 是临时方案，后续应重构为不构造完整 Player。
        static ecs::EntityRegistry s_interactionPlayerRegistry{"block-interaction"};
        Player interactionPlayer(playerId, playerData->username, s_interactionPlayerRegistry);
        return block->onBlockActivated(*state, *world, pos, interactionPlayer, hand, hitResult);
    }();

    // 派发自定义方块组件回调 - onPlayerInteract
    auto& blockCompReg = mc::mod::bedrock::addon::BlockComponentRegistry::instance();
    std::string blockTypeId = block->blockLocation().toString();
    if (blockCompReg.hasPlayerInteractCallback(blockTypeId)) {
        mc::mod::bedrock::addon::BlockComponentPlayerInteractEvent event;
        event.blockTypeId = blockTypeId;
        event.blockX = pos.x;
        event.blockY = pos.y;
        event.blockZ = pos.z;
        event.dimensionId = world->dimension();
        event.playerId = playerId;
        event.face = static_cast<i32>(face);
        event.faceX = hitPos.x - static_cast<f32>(pos.x);
        event.faceY = hitPos.y - static_cast<f32>(pos.y);
        event.faceZ = hitPos.z - static_cast<f32>(pos.z);
        blockCompReg.dispatchPlayerInteract(blockTypeId, event);
    }

    // 派发自定义物品组件回调 - onUseOn
    // 当玩家右键点击方块时，手持物品触发 onUseOn 回调
    ItemStack heldItem = _getHeldTool(playerId);
    if (!heldItem.isEmpty()) {
        const Item* heldItemPtr = heldItem.getItem();
        if (heldItemPtr != nullptr) {
            auto& itemCompReg = mc::mod::bedrock::addon::ItemComponentRegistry::instance();
            std::string itemTypeId = heldItemPtr->itemLocation().toString();
            if (itemCompReg.hasUseOnCallback(itemTypeId)) {
                mc::mod::bedrock::addon::ItemComponentUseOnEvent useOnEvent;
                useOnEvent.itemTypeId = itemTypeId;
                useOnEvent.sourceId = playerId;
                useOnEvent.blockX = pos.x;
                useOnEvent.blockY = pos.y;
                useOnEvent.blockZ = pos.z;
                useOnEvent.usedOnBlockTypeId = blockTypeId;
                useOnEvent.usedOnBlockPermutationTypeId = blockTypeId;
                useOnEvent.face = static_cast<i32>(face);
                useOnEvent.faceX = hitPos.x - static_cast<f32>(pos.x);
                useOnEvent.faceY = hitPos.y - static_cast<f32>(pos.y);
                useOnEvent.faceZ = hitPos.z - static_cast<f32>(pos.z);
                itemCompReg.dispatchUseOn(itemTypeId, useOnEvent);
            }
        }
    }

    // 如果方块交互成功，检查是否为告示牌并执行命令
    bool handled = (result == ActionResultType::Success || result == ActionResultType::Consume);
    if (handled && m_server != nullptr) {
        // 检查是否为告示牌方块
        BlockEntity* blockEntity = world->getBlockEntity(pos);
        if (blockEntity && blockEntity->getType() == BlockEntityType::Sign) {
            // 获取实际的 ServerPlayer 实体
            ServerPlayerEntityManager& entityManager = m_server->playerEntityManager();
            Player* playerEntity = entityManager.getPlayerEntity(playerId, *world);
            if (playerEntity != nullptr) {
                mc::ServerPlayer* serverPlayer = playerEntity->asServerPlayer();
                if (serverPlayer != nullptr) {
                    // 执行告示牌命令
                    _handleSignCommand(*world, pos, *serverPlayer);
                }
            }
        }
    }

    // 消费 heldItemTransformedTo：将方块交互后的手持物品变更同步回 InventoryManager
    // 参考 MC 1.21.11 ServerPlayerGameMode.useItem 中处理 heldItemTransformedTo 的逻辑：
    // - 如果交互结果携带了 heldItemTransformedTo，使用该值更新玩家物品栏
    // - 否则使用玩家当前手持物品（方块可能通过 player.getHeldItem(hand) 引用直接修改了）
    if (handled && realPlayer != nullptr && m_inventoryManager != nullptr) {
        PlayerInventory* mgrInventory = m_inventoryManager->getInventory(playerId);
        if (mgrInventory != nullptr) {
            i32 selectedSlot = mgrInventory->getSelectedSlot();
            ItemStack newHeldItem;
            bool needUpdate = false;

            if (result.heldItemTransformedTo().has_value()) {
                // 方块显式返回了转换后的手持物品
                newHeldItem = result.heldItemTransformedTo().value();
                needUpdate = true;
            } else {
                // 方块未显式返回转换后物品，检查 Player::m_inventory 是否被修改
                // （方块可能通过 player.getHeldItem(hand) 引用直接修改了手持物品）
                ItemStack playerHeld = realPlayer->inventory().getSelectedStack();
                ItemStack mgrHeld = mgrInventory->getSelectedStack();
                if (!(playerHeld == mgrHeld)) {
                    newHeldItem = playerHeld;
                    needUpdate = true;
                }
            }

            if (needUpdate) {
                mgrInventory->setItem(selectedSlot, newHeldItem);
                m_inventoryManager->syncToClient(playerId);
            }
        }
    }

    return BlockInteractionResult{handled, handled ? "Block used" : "Block use pass"};
}

Result<BlockBreakResult> BlockInteractionManager::handleBlockBreak(PlayerId playerId, const BlockPos& pos)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.World,
        "BlockInteractionManager::handleBlockBreak",
        "pos",
        pos.toString(),
        "playerId",
        playerId,
        [flow = ::perfetto::Flow::ProcessScoped(pos.toId())](::perfetto::EventContext ctx) { flow(ctx); });

    // 检查世界修改权限
    ServerWorld* world = _getPlayerWorld(playerId);
    if (world == nullptr) {
        return Error(ErrorCode::InvalidWorld, "Player world not available");
    }

    auto worldError = _checkWorldModificationAllowed(*world);
    if (worldError) {
        return std::move(*worldError);
    }

    // 验证前置条件
    auto preconditionError = _validateInteractionPreconditions(playerId, pos, true);
    if (preconditionError) {
        return std::move(*preconditionError);
    }

    // 获取方块状态
    const BlockState* state = _getNonAirBlockState(*world, pos);
    if (!state) {
        return BlockBreakResult{false, 0, "No block to break"};
    }

    const BlockState oldState = *state;

    // 检查是否可破坏
    if (!_canBreakBlock(*world, playerId, pos, state)) {
        return BlockBreakResult{false, 0, "Cannot break this block"};
    }

    // 获取手持物品作为工具
    ItemStack tool = _getHeldTool(playerId);

    // 派发自定义方块组件回调 - onPlayerBreak
    auto& blockCompReg = mc::mod::bedrock::addon::BlockComponentRegistry::instance();
    std::string blockTypeId = oldState.getBlock().blockLocation().toString();
    if (blockCompReg.hasPlayerBreakCallback(blockTypeId)) {
        mc::mod::bedrock::addon::BlockComponentPlayerBreakEvent event;
        event.blockTypeId = blockTypeId;
        event.blockX = pos.x;
        event.blockY = pos.y;
        event.blockZ = pos.z;
        event.dimensionId = world->dimension();
        event.playerId = playerId;
        event.brokenBlockPermutationTypeId = blockTypeId;
        blockCompReg.dispatchPlayerBreak(blockTypeId, event);
    }
    if (blockCompReg.hasBreakCallback(blockTypeId)) {
        mc::mod::bedrock::addon::BlockComponentBreakEvent breakEvent;
        breakEvent.blockTypeId = blockTypeId;
        breakEvent.blockX = pos.x;
        breakEvent.blockY = pos.y;
        breakEvent.blockZ = pos.z;
        breakEvent.dimensionId = world->dimension();
        breakEvent.brokenBlockPermutationTypeId = blockTypeId;
        breakEvent.entitySourceId = playerId;
        blockCompReg.dispatchBreak(blockTypeId, breakEvent);
    }

    // 通知方块玩家即将破坏（如活塞头在创造模式下级联销毁活塞基座）
    Player* playerEntity = _getPlayerEntity(playerId, *world);
    if (playerEntity != nullptr) {
        oldState.getBlockMutable().playerWillDestroy(*world, pos, oldState, *playerEntity);
    }

    // 创造模式不产生掉落物
    bool isCreative = playerEntity != nullptr && playerEntity->isCreative();
    if (!isCreative) {
        // 生成掉落物
        _generateBlockDrops(*world, pos, oldState, playerId, tool.isEmpty() ? nullptr : &tool);
    }

    // 调用工具的 onBlockDestroyed 回调（用于耐久消耗等）
    if (!tool.isEmpty()) {
        const Item* toolItem = tool.getItem();
        if (toolItem != nullptr) {
            // 获取玩家实体用于 onBlockDestroyed 调用
            Player* playerEntity = _getPlayerEntity(playerId, *world);
            if (playerEntity != nullptr) {
                const_cast<Item*>(toolItem)->onBlockDestroyed(tool, *world, oldState, pos, *playerEntity);
            }

            // 派发自定义物品组件回调 - onMineBlock
            auto& itemCompReg = mc::mod::bedrock::addon::ItemComponentRegistry::instance();
            std::string itemTypeId = toolItem->itemLocation().toString();
            if (itemCompReg.hasMineBlockCallback(itemTypeId)) {
                mc::mod::bedrock::addon::ItemComponentMineBlockEvent mineEvent;
                mineEvent.itemTypeId = itemTypeId;
                mineEvent.sourceId = playerId;
                mineEvent.blockX = pos.x;
                mineEvent.blockY = pos.y;
                mineEvent.blockZ = pos.z;
                mineEvent.blockTypeId = blockTypeId;
                mineEvent.minedBlockPermutationTypeId = blockTypeId;
                mineEvent.itemStackAmount = tool.getCount();
                itemCompReg.dispatchMineBlock(itemTypeId, mineEvent);
            }
        }
    }

    // 设置为空气
    u32 newBlockStateId = _setBlockToAir(*world, pos, oldState, playerId);

    // 调用方块的 spawnAfterBreak 回调（如 InfestedBlock 生成蠹虫）
    // 方块已移除后调用，与 MC Java 行为一致：先移除方块再生成额外实体
    const Block& oldBlock = oldState.getBlock();
    oldBlock.spawnAfterBreak(*world, pos, oldState, tool.isEmpty() ? nullptr : &tool, true);

    return BlockBreakResult{true, newBlockStateId, "Block destroyed"};
}

void BlockInteractionManager::setOnBlockBreak(
    std::function<void(PlayerId, const BlockPos&, const BlockState&)> callback)
{
    m_onBlockBreak = std::move(callback);
}

void BlockInteractionManager::setOnBlockPlace(
    std::function<void(PlayerId, const BlockPos&, const BlockState&)> callback)
{
    m_onBlockPlace = std::move(callback);
}

bool BlockInteractionManager::_canInteract(const Player* player, PlayerId playerId, const BlockPos& pos) const noexcept
{
    // 优先走属性驱动的精确距离判定（对齐 vanilla isWithinBlockInteractionRange）：
    // 眼位到方块 AABB 最近点距离 < (blockInteractionRange + padding)^2。
    // padding 取 1.0，对齐 vanilla ServerGamePacketListenerImpl.java:695 /
    // ServerPlayerGameMode.java:152 的 isWithinBlockInteractionRange(pos, 1.0)。
    if (player != nullptr) {
        return player->isWithinBlockInteractionRange(pos, 1.0);
    }

    // Player 实体不可用时回退到 PlayerData 中心距离保底（避免完全无门控）。
    auto* playerData = m_playerManager.getPlayer(playerId);
    if (!playerData) {
        return false;
    }

    // 计算距离（玩家眼睛位置到方块中心）
    const f64 eyeX = playerData->x;
    const f64 eyeY = playerData->y + static_cast<f64>(game::PLAYER_EYE_HEIGHT);
    const f64 eyeZ = playerData->z;
    const f64 targetX = pos.x + 0.5;
    const f64 targetY = pos.y + 0.5;
    const f64 targetZ = pos.z + 0.5;

    const f64 dx = targetX - eyeX;
    const f64 dy = targetY - eyeY;
    const f64 dz = targetZ - eyeZ;
    const f64 distanceSquared = dx * dx + dy * dy + dz * dz;

    // 最大交互距离 6 格（回退保底值，正常路径走属性不应到达此处）
    constexpr f64 MAX_INTERACT_DISTANCE_SQ = 36.0; // 6 * 6
    return distanceSquared <= MAX_INTERACT_DISTANCE_SQ;
}

bool BlockInteractionManager::_canBreakBlock(
    ServerWorld& world, PlayerId playerId, const BlockPos& pos, const BlockState* state) const noexcept
{
    if (!state || state->isAir() || state->hardness() < 0.0f) {
        return false;
    }

    Player* player = _getPlayerEntity(playerId, world);

    // 游戏管理员方块需要 canUseGameMasterBlocks() 权限才能破坏
    const Block& block = state->getBlock();
    if (block.isGameMaster()) {
        if (player == nullptr || !player->canUseGameMasterBlocks()) {
            return false;
        }
    }

    // 检查方块操作权限（旁观模式、冒险模式 CanDestroy 限制）
    if (player != nullptr && player->blockActionRestricted(world, pos)) {
        return false;
    }

    return _canInteract(player, playerId, pos);
}

bool BlockInteractionManager::_wouldCollideWithPlayer(
    PlayerId playerId, const BlockPos& placePos, const BlockState& state) const noexcept
{
    auto* playerData = m_playerManager.getPlayer(playerId);
    if (playerData == nullptr) {
        return false;
    }

    const auto& collisionShape = state.getCollisionShape();
    if (collisionShape.isEmpty()) {
        return false;
    }

    const f32 halfWidth = Player::PLAYER_WIDTH * 0.5f;
    const AxisAlignedBB playerBoundingBox(playerData->x - halfWidth,
        playerData->y,
        playerData->z - halfWidth,
        playerData->x + halfWidth,
        playerData->y + Player::PLAYER_HEIGHT,
        playerData->z + halfWidth);

    return collisionShape.intersects(playerBoundingBox, placePos.x, placePos.y, placePos.z);
}

void BlockInteractionManager::_generateBlockDrops(
    ServerWorld& world, const BlockPos& pos, const BlockState& state, PlayerId playerId, const ItemStack* tool)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.World,
        "BlockInteractionManager::generateBlockDrops",
        "pos",
        pos.toString(),
        "playerId",
        playerId,
        [flow = ::perfetto::Flow::ProcessScoped(pos.toId())](::perfetto::EventContext ctx) { flow(ctx); });

    Player* player = _getPlayerEntity(playerId, world);

    // 使用 BlockDropHandler 生成掉落物
    auto drops = BlockDropHandler::generateDrops(world, pos, state, player, tool, m_lootTableManager);

    if (!drops.empty()) {
        const std::string throwerUuid = player != nullptr ? player->uuid() : std::string();
        BlockDropHandler::spawnDrops(world, pos, drops, throwerUuid);
    }

    // 处理矿石经验掉落
    // 使用随机种子生成器
    const u64 seed = static_cast<u64>(static_cast<u64>(pos.x)) ^ static_cast<u64>(static_cast<u64>(pos.y) << 1) ^
        static_cast<u64>(static_cast<u64>(pos.z) << 2) ^ static_cast<u64>(std::hash<PlayerId>{}(playerId));
    math::Random rng(seed);
    BlockDropHandler::handleBlockBreakExperience(world, pos, state, tool, rng);
}

bool BlockInteractionManager::_handleSignCommand(ServerWorld& world, const BlockPos& pos, mc::ServerPlayer& player)
{
    // 当玩家右键点击告示牌时，执行告示牌上的命令

    // 获取方块实体
    BlockEntity* blockEntity = world.getBlockEntity(pos);
    if (!blockEntity || blockEntity->getType() != BlockEntityType::Sign) {
        return false;
    }

    auto* signEntity = static_cast<blockentity::SignEntity*>(blockEntity);
    if (!signEntity) {
        return false;
    }

    // 使用 SignCommandHelper 执行命令
    return SignCommandHelper::executeSignCommands(*signEntity, player);
}

} // namespace mc::server::interaction
