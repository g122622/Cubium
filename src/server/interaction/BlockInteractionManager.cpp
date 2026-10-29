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
#include "common/entity/experience/ExperienceDropHandler.hpp"
#include "common/entity/inventory/PlayerInventory.hpp"
#include "common/item/context/BlockItemUseContext.hpp"
#include "common/item/context/ItemUseContext.hpp"
#include "common/item/core/ActionResult.hpp"
#include "common/item/core/BlockActionResult.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/core/ItemRegistry.hpp"
#include "common/item/enchantment/EnchantmentHelper.hpp"
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
#include "common/world/WorldEvents.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/blockentity/BlockEntity.hpp"
#include "common/world/blockentity/BlockEntityType.hpp"
#include "common/world/blockentity/interactive/SignEntity.hpp"
#include "common/world/dimension/DimensionManager.hpp"
#include "server/application/IServer.hpp"
#include "server/core/OpListManager.hpp"
#include "server/core/PlayerManager.hpp"
#include "server/core/ServerPlayerData.hpp"
#include "server/dimension/ServerDimensionManager.hpp"
#include "server/player/ServerPlayer.hpp"
#include "server/world/ServerWorld.hpp"
#include "server/world/drop/BlockDropHandler.hpp"
#include "server/world/player/ServerPlayerEntityManager.hpp"
#include <cmath>
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
    PlayerId playerId, const BlockPos& pos, bool checkYRange, bool checkSpawnProtection) const
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
        // 拒绝必须留痕并说明是哪一种失败：player 为空说明「玩家实体不在该维度的
        // EntityManager 里」（跨维度传送后的典型症状），与「真的离得远」是两回事。
        spdlog::warn("Interaction rejected for player {} at {}: {}",
            playerId,
            pos.toString(),
            player == nullptr ? "player entity not found in player's dimension world"
                              : "block is out of interaction range");
        return Error(ErrorCode::InvalidArgument, "Block too far away");
    }

    // 验证 Y 范围（可选）
    if (checkYRange) {
        if (pos.y < world::MIN_BUILD_HEIGHT || pos.y >= world::MAX_BUILD_HEIGHT) {
            return Error(ErrorCode::InvalidArgument, "Block Y out of range");
        }
    }

    // 世界级交互权限（对齐 vanilla ServerLevel.mayInteract）：世界边界之外一律禁止；
    // 主世界出生点保护半径内，非版主权限且非单机主的玩家禁止交互（spawn-protection=0 关闭）。
    // 玩家实体缺失时仍执行与实体无关的边界检查。
    //
    // vanilla 中 mayInteract 在 handleUseItemOn（含 useItemOn 与 Item::useOn）与
    // handleBlockBreakAction 的 START_DESTROY 两条路径都被调用，故放置 / 右键使用 / 破坏
    // 三条路径均应传 checkSpawnProtection=true。
    if (checkSpawnProtection) {
        ServerWorld* world = _getPlayerWorld(playerId);
        if (world != nullptr) {
            const bool allowed =
                (player != nullptr) ? world->mayInteract(*player, pos) : world->worldBorder().contains(pos.x, pos.z);
            if (!allowed) {
                return Error(ErrorCode::PermissionDenied, "Cannot interact at this position");
            }
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

const ItemStack& BlockInteractionManager::_handStackFor(PlayerId playerId, Hand hand) const noexcept
{
    // 静态空堆兜底：物品栏解析器缺失（测试环境未注入）时返回空栈，保持调用方可比较语义。
    static const ItemStack s_empty;
    if (m_inventoryManager == nullptr) {
        return s_empty;
    }
    const PlayerInventory* inventory = m_inventoryManager->getInventory(playerId);
    if (inventory == nullptr) {
        return s_empty;
    }
    return (hand == Hand::OffHand) ? inventory->getOffhandItemRef() : inventory->getSelectedStackRef();
}

bool BlockInteractionManager::_anyHandHasItem(PlayerId playerId) const noexcept
{
    return !_handStackFor(playerId, Hand::MainHand).isEmpty() || !_handStackFor(playerId, Hand::OffHand).isEmpty();
}

Player* BlockInteractionManager::_getPlayerEntity(PlayerId playerId, ServerWorld& world) const noexcept
{
    (void)world;
    if (m_server == nullptr) {
        return nullptr;
    }

    // 玩家实体只存在于其所在维度的 EntityManager 中：玩家仍停留在 A 维度而方块交互被派发到
    // B 维度时，用目标世界查找必然失败，进而退化到占位 Player（缺少真实物品栏/游戏模式）。
    // 因此先按玩家所在维度取世界，再在该世界查找实体。玩家未注册维度时退回目标世界，
    // 保证 SimulatedPlayer 等不经过维度注册的场景仍可解析。
    auto& dimensionManager = m_server->dimensionManager();
    const DimensionId playerDimensionId = dimensionManager.getPlayerDimension(playerId);
    ServerWorld* searchWorld = &world;
    if (playerDimensionId >= 0) {
        ServerDimension* playerDimension = dimensionManager.getDimension(playerDimensionId);
        if (playerDimension != nullptr && playerDimension->world() != nullptr) {
            searchWorld = playerDimension->world();
        }
    }
    return m_server->playerEntityManager().getPlayerEntity(playerId, *searchWorld);
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

Result<BlockPlacementResult> BlockInteractionManager::handleBlockPlacement(
    PlayerId playerId, const BlockPos& pos, const Vector3& hitPos, Direction face, Hand hand, const ItemStack& heldItem)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.World,
        "BlockInteractionManager::handleBlockPlacement",
        "pos",
        pos.toString(),
        "playerId",
        playerId,
        "face",
        Directions::toString(face),
        [flow = ::perfetto::Flow::ProcessScoped(pos.toId())](::perfetto::EventContext ctx) {
            flow(ctx);
        });

    // 检查世界修改权限
    ServerWorld* world = _getPlayerWorld(playerId);
    if (world == nullptr) {
        return Error(ErrorCode::InvalidWorld, "Player world not available");
    }

    auto worldError = _checkWorldModificationAllowed(*world);
    if (worldError) {
        return std::move(*worldError);
    }

    // 验证前置条件（放置路径同样受出生点保护约束，对齐 vanilla handleUseItemOn 中的
    // `serverlevel.mayInteract(this.player, blockpos)` 门控）
    auto preconditionError = _validateInteractionPreconditions(playerId, pos, false, true);
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

    // 权威手持槽按 hand 选择：主手=当前选中槽，副手=OFFHAND 槽（对齐 vanilla getItemInHand）。
    // 传给放置上下文的 stack 即权威引用——对齐 vanilla BlockItem.place 末行
    // `itemstack.consume(1, player)` 直接消耗手中的那一份（创造模式经 hasInfiniteMaterials 免消耗）。
    PlayerInventory* inventory = (m_inventoryManager != nullptr) ? m_inventoryManager->getInventory(playerId) : nullptr;
    ItemStack emptyFallback;
    ItemStack& authoritative = (inventory != nullptr)
        ? ((hand == Hand::OffHand) ? inventory->getOffhandItemRef() : inventory->getSelectedStackRef())
        : emptyFallback;

    const i32 countBefore = authoritative.getCount();
    const bool infiniteMaterials =
        playerData->gameMode == GameMode::Creative || (player != nullptr && player->abilities().invulnerable);

    // 创建放置上下文。传真实玩家，使方块实体的 owner/loot 上下文与 mayUseItemAt 语义正确；
    // playerData 同时提供 yaw 与 pitch，pitch 用于 getNearestLookingDirections 的方向排序。
    BlockItemUseContext context(*world, player, authoritative, hitPos, pos, face, playerData->yaw, playerData->pitch);

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

    // 执行放置（BlockItem::tryPlace 内部对上下文 stack 的消耗直接作用于权威槽）
    ActionResultType result = blockItem->tryPlace(context);
    if (result != ActionResultType::Success && result != ActionResultType::Consume) {
        return BlockPlacementResult{false, false, false, pos, 0, "Cannot place block here"};
    }

    // 兜底消耗：BlockItem::tryPlace 已对权威槽 shrink 一次（非创造模式）；若某子类未消耗，
    // 此处按数量变化判断并补足一次，保持与旧实现的消耗语义一致。创造模式不实际消耗。
    bool itemConsumed = true;
    if (!infiniteMaterials && authoritative.getCount() == countBefore && countBefore > 0) {
        authoritative.shrink(1);
    }

    if (m_inventoryManager != nullptr && inventory != nullptr) {
        m_inventoryManager->syncToClient(playerId);
    }

    if (m_onBlockPlace) {
        m_onBlockPlace(playerId, placePos, *newState);
    }

    return BlockPlacementResult{true, true, itemConsumed, placePos, newState->stateId(), "Block placed"};
}

// heldItem 必须按值传入（局部拷贝），不能让 ItemUseContext 直接持有权威手持槽：
// ItemUseContext 构造函数对该 stack 做 const_cast 后持有可写指针，onItemUse 内
// getItemStackMut() 会直接改它；若指向权威槽，物品侧的自改会绕过下方 itemId/damage
// 对比，导致外层再 shrink 一次（双重消耗）。改成 const 引用即破坏这一隔离。
Result<BlockUseResult> BlockInteractionManager::handleItemUseOn(PlayerId playerId,
    const BlockPos& pos,
    const Vector3& hitPos,
    Direction face,
    Hand hand,
    // NOLINTNEXTLINE(performance-unnecessary-value-param)
    ItemStack heldItem)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.World,
        "BlockInteractionManager::handleItemUseOn",
        "pos",
        pos.toString(),
        "playerId",
        playerId,
        "face",
        Directions::toString(face),
        [flow = ::perfetto::Flow::ProcessScoped(pos.toId())](::perfetto::EventContext ctx) {
            flow(ctx);
        });

    ServerWorld* world = _getPlayerWorld(playerId);
    if (world == nullptr) {
        return Error(ErrorCode::InvalidWorld, "Player world not available");
    }

    auto worldError = _checkWorldModificationAllowed(*world);
    if (worldError) {
        return std::move(*worldError);
    }

    // 验证前置条件（物品在方块上使用同样受出生点保护约束，对齐 vanilla handleUseItemOn 中的
    // `serverlevel.mayInteract(this.player, blockpos)` 门控）
    auto preconditionError = _validateInteractionPreconditions(playerId, pos, false, true);
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

    // block-item：走 BlockItem.place 放置流程（对齐 vanilla BlockItem.useOn → this.place(...)），
    // 由 handleBlockPlacement 统一处理位置校验与权威槽消耗。
    if (const Item* itemForPlace = heldItem.getItem(); itemForPlace != nullptr &&
        BlockItemRegistry::instance().getBlockItemByItemId(itemForPlace->itemId()) != nullptr) {
        auto placement = handleBlockPlacement(playerId, pos, hitPos, face, hand, heldItem);
        const bool placed = placement.success() && placement.value().blockPlaced;
        return BlockUseResult{placed,
            placement.success() && placement.value().itemConsumed,
            BlockUseResult::Kind::ItemUseOn,
            placed ? ActionResultType::Success : ActionResultType::Pass,
            std::nullopt,
            placed ? "Block placed" : (placement.success() ? placement.value().message : "Placement rejected")};
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

    // 构造使用上下文。传真实 player（对齐方块交互的 realPlayer 范式），使 onItemUse 内
    // 可通过 context.getPlayer()->getHeldItem(hand) 直接操作权威手持物——这对「消耗原物品并返回新物品」
    // 的物品（FishBucketItem 鱼桶→空桶）是必需的：此类物品在 onItemUse 内替换手持物，外层无法仅靠
    // shrink 复现。此前传 nullptr 致 FishBucketItem.onItemUse 的 _returnEmptyBucket 不执行（player==nullptr
    // 守卫跳过），生产路径玩家持鱼桶右键地面失去鱼桶却得不到空桶。
    // heldItem 是调用方局部拷贝，onItemUse 内 context.getItemStackMut() 修改的是该拷贝，不回写权威物品栏——
    // 故消耗/替换仍以 player 权威手持物为准，下方同步回 InventoryManager。
    if (player != nullptr) {
        player->setGameMode(playerData->gameMode);
    }

    // 记录 onItemUse 前权威槽的 itemId+damage：部分物品会在 onItemUse 内直接改权威手持物——
    // (1)自管理替换（鱼桶→空桶，itemId 变化）；(2)耐久损耗（打火石/锄/斧/锹 hurtAndBreak，
    // damage 变化，itemId 不变）。两种情况外层都不应再 shrink（否则误消耗返回物或把耐久损耗
    // 误当数量消耗）。骨粉等「仅 shrink 原物品、不替换也不改耐久」的物品 itemId+damage 均不变，
    // 仍走外层 shrink 补足。
    //
    // 权威槽随 hand 选择：主手取当前选中槽，副手取 OFFHAND 槽（对齐 vanilla Player#getItemInHand）。
    // 经 m_inventoryManager 解析（单源权威栏），与下方消耗写回同一来源，保证 before/after 可比。
    const ItemStack heldBefore = [&]() -> ItemStack {
        if (m_inventoryManager != nullptr) {
            if (const PlayerInventory* inventory = m_inventoryManager->getInventory(playerId)) {
                return (hand == Hand::OffHand) ? inventory->getOffhandItem() : inventory->getSelectedStack();
            }
        }
        return ItemStack();
    }();
    const ItemId itemIdBefore = heldBefore.isEmpty() ? ItemId{0} : heldBefore.getItem()->itemId();
    const i32 damageBefore = heldBefore.isEmpty() ? 0 : heldBefore.getDamage();
    ItemUseContext context(*world, player, heldItem, hitPos, pos, face, hand, playerData->yaw, playerData->pitch);

    ActionResultType result = item->onItemUse(context);
    const bool success = (result == ActionResultType::Success || result == ActionResultType::Consume);

    // 消耗权威物品栏（仅 success 时，对齐 handleBlockPlacement 的消耗范式）。
    bool itemConsumed = false;
    if (success && playerData->gameMode == GameMode::Creative) {
        itemConsumed = true; // 创造模式不实际消耗
    } else if (success && m_inventoryManager != nullptr) {
        if (PlayerInventory* inventory = m_inventoryManager->getInventory(playerId)) {
            // 权威槽随 hand 选择：主手=当前选中槽，副手=OFFHAND 槽（对齐 vanilla Player#getItemInHand）。
            ItemStack& authoritative =
                (hand == Hand::OffHand) ? inventory->getOffhandItemRef() : inventory->getSelectedStackRef();
            // onItemUse 若已自行改过权威手持物（itemId 变化=自管理替换，或 damage 变化=耐久损耗），
            // 说明物品已处理完消耗/损耗，外层只推一次同步、不再 shrink。itemId+damage 均不变时
            // （如骨粉只 shrink 了自己的局部拷贝）才由外层 shrink(1) 补足权威槽的消耗。
            const ItemId itemIdAfter = authoritative.isEmpty() ? ItemId{0} : authoritative.getItem()->itemId();
            const i32 damageAfter = authoritative.isEmpty() ? 0 : authoritative.getDamage();
            const bool selfManaged = (itemIdAfter != itemIdBefore) || (damageAfter != damageBefore);
            if (selfManaged) {
                itemConsumed = true;
                m_inventoryManager->syncToClient(playerId);
            } else if (!authoritative.isEmpty() && authoritative.getCount() > 0) {
                authoritative.shrink(1);
                itemConsumed = true;
                m_inventoryManager->syncToClient(playerId);
            }
        }
    }

    // ItemUseOn 分支不承载 heldItemTransformedTo（物品自行改写权威槽）。
    return BlockUseResult{success,
        itemConsumed,
        BlockUseResult::Kind::ItemUseOn,
        result,
        std::nullopt,
        success ? "Item used on block" : "Item use pass"};
}

std::optional<BlockUseResult> BlockInteractionManager::_dispatchBlockUse(PlayerId playerId,
    ServerWorld& world,
    const BlockPos& pos,
    Hand hand,
    const Vector3& hitPos,
    Direction face,
    bool withItem,
    ItemStack** outHandStack)
{
    const BlockState* state = _getNonAirBlockState(world, pos);
    if (!state) {
        return std::nullopt;
    }

    Block* block = BlockRegistry::instance().getBlock(state->blockId());
    if (!block) {
        return std::nullopt;
    }

    auto* playerData = _validatePlayer(playerId);
    if (!playerData) {
        return std::nullopt;
    }

    // 获取真实的玩家实体（携带 m_inventory、m_gameMode 等状态）。方块交互直接在
    // realPlayer->inventory() 上操作——那正是权威物品栏，无需预先搬运。
    Player* realPlayer = _getPlayerEntity(playerId, world);
    const BlockRaycastResult hitResult = BlockRaycastResult::hit(hitPos, pos, face, 0.0f);

    if (realPlayer != nullptr) {
        realPlayer->setGameMode(playerData->gameMode);
    }

    // 取 hand 对应的权威手持槽引用：主手=当前选中槽，副手=OFFHAND 槽（对齐 vanilla
    // Player#getItemInHand）。方块侧对 stack 的修改直接落到权威槽，服务端随后统一同步。
    ItemStack emptyFallback;
    ItemStack* stackPtr = &emptyFallback;
    if (m_inventoryManager != nullptr) {
        if (PlayerInventory* inventory = m_inventoryManager->getInventory(playerId)) {
            stackPtr = (hand == Hand::OffHand) ? &inventory->getOffhandItemRef() : &inventory->getSelectedStackRef();
        }
    }
    if (outHandStack != nullptr) {
        *outHandStack = stackPtr;
    }
    ItemStack& authoritative = *stackPtr;
    const ItemStack heldBeforeUse = authoritative;

    BlockActionResult result = [&]() -> BlockActionResult {
        if (realPlayer != nullptr) {
            return withItem ? block->useItemOn(authoritative, *state, world, pos, *realPlayer, hand, hitResult)
                            : block->useWithoutItem(*state, world, pos, *realPlayer, hitResult);
        }
        // 回退路径：玩家实体在维度管理器中不可解析时使用临时 Player，保证方块交互不中断。
        // ECS 迁移：占位 Player 构造需要 registry 句柄，此处无世界上下文故配静态 registry。
        // TODO: 占位 Player 是临时方案，后续应重构为不构造完整 Player；正常在线玩家经
        //   _getPlayerEntity 按所在维度解析后不会走到这里。
        static ecs::EntityRegistry s_interactionPlayerRegistry{"block-interaction"};
        Player interactionPlayer(playerId, playerData->username, s_interactionPlayerRegistry);
        return withItem ? block->useItemOn(authoritative, *state, world, pos, interactionPlayer, hand, hitResult)
                        : block->useWithoutItem(*state, world, pos, interactionPlayer, hitResult);
    }();

    const bool handled = result.consumesAction();

    // 交互成功时检查是否为告示牌并执行命令
    if (handled && m_server != nullptr) {
        BlockEntity* blockEntity = world.getBlockEntity(pos);
        if (blockEntity && blockEntity->getType() == BlockEntityType::Sign) {
            ServerPlayerEntityManager& entityManager = m_server->playerEntityManager();
            Player* playerEntity = entityManager.getPlayerEntity(playerId, world);
            if (playerEntity != nullptr) {
                mc::ServerPlayer* serverPlayer = playerEntity->asServerPlayer();
                if (serverPlayer != nullptr) {
                    _handleSignCommand(world, pos, *serverPlayer);
                }
            }
        }
    }

    // 派发自定义方块组件回调 - onPlayerInteract。
    // 必须放在此处而非单个方块虚方法内部：`handleBlockUse` 可能对同一次右键调用
    // `_dispatchBlockUse` 两次（useItemOn 返回 TRY_WITH_EMPTY_HAND 后回退 useWithoutItem），
    // 放在虚方法内会让 addon 脚本收到重复事件、副作用执行两遍。本函数是每次右键的唯一入口，
    // 故按"一次右键派发一次"的语义落在这里。
    {
        auto& blockCompReg = mc::mod::bedrock::addon::BlockComponentRegistry::instance();
        const std::string blockTypeId = block->blockLocation().toString();
        if (blockCompReg.hasPlayerInteractCallback(blockTypeId)) {
            mc::mod::bedrock::addon::BlockComponentPlayerInteractEvent event;
            event.blockTypeId = blockTypeId;
            event.blockX = pos.x;
            event.blockY = pos.y;
            event.blockZ = pos.z;
            event.dimensionId = world.dimension();
            event.playerId = playerId;
            event.face = static_cast<i32>(face);
            event.faceX = hitPos.x - static_cast<f32>(pos.x);
            event.faceY = hitPos.y - static_cast<f32>(pos.y);
            event.faceZ = hitPos.z - static_cast<f32>(pos.z);
            blockCompReg.dispatchPlayerInteract(blockTypeId, event);
        }
    }

    // 派发自定义物品组件回调 - onUseOn（仅携带物品的交互）
    if (withItem && !heldBeforeUse.isEmpty()) {
        const Item* heldItemPtr = heldBeforeUse.getItem();
        if (heldItemPtr != nullptr) {
            auto& itemCompReg = mc::mod::bedrock::addon::ItemComponentRegistry::instance();
            const std::string itemTypeId = heldItemPtr->itemLocation().toString();
            if (itemCompReg.hasUseOnCallback(itemTypeId)) {
                mc::mod::bedrock::addon::ItemComponentUseOnEvent useOnEvent;
                useOnEvent.itemTypeId = itemTypeId;
                useOnEvent.sourceId = playerId;
                useOnEvent.blockX = pos.x;
                useOnEvent.blockY = pos.y;
                useOnEvent.blockZ = pos.z;
                useOnEvent.usedOnBlockTypeId = block->blockLocation().toString();
                useOnEvent.usedOnBlockPermutationTypeId = block->blockLocation().toString();
                useOnEvent.face = static_cast<i32>(face);
                useOnEvent.faceX = hitPos.x - static_cast<f32>(pos.x);
                useOnEvent.faceY = hitPos.y - static_cast<f32>(pos.y);
                useOnEvent.faceZ = hitPos.z - static_cast<f32>(pos.z);
                itemCompReg.dispatchUseOn(itemTypeId, useOnEvent);
            }
        }
    }

    // 交互可能改动手持物，需推给客户端。两个来源：
    //   1. 方块显式返回 heldItemTransformedTo —— 用它更新 hand 对应槽位；
    //   2. 方块通过 stack 引用直接改了权威槽 —— 与交互前的快照比较即可发现。
    bool itemConsumed = false;
    if (handled && m_inventoryManager != nullptr) {
        if (PlayerInventory* inventory = m_inventoryManager->getInventory(playerId)) {
            ItemStack& finalSlot =
                (hand == Hand::OffHand) ? inventory->getOffhandItemRef() : inventory->getSelectedStackRef();
            if (result.heldItemTransformedTo().has_value()) {
                finalSlot = result.heldItemTransformedTo().value();
                itemConsumed = true;
                m_inventoryManager->syncToClient(playerId);
            } else if (!(finalSlot == heldBeforeUse)) {
                itemConsumed = true;
                m_inventoryManager->syncToClient(playerId);
            }
        }
    }

    return BlockUseResult{handled,
        itemConsumed,
        withItem ? BlockUseResult::Kind::BlockUseItemOn : BlockUseResult::Kind::BlockUseWithoutItem,
        result.getType(),
        result.heldItemTransformedTo(),
        handled ? "Block used" : "Block use pass"};
}

Result<BlockUseResult> BlockInteractionManager::handleBlockUse(
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
        [flow = ::perfetto::Flow::ProcessScoped(pos.toId())](::perfetto::EventContext ctx) {
            flow(ctx);
        });

    // 验证前置条件（不检查 Y 范围；出生点保护对所有玩家生效）。
    // vanilla `ServerGamePacketListenerImpl.handleUseItemOn` 在调 `gameMode.useItemOn` 之前
    // 无条件执行 `serverlevel.mayInteract(player, blockpos)`，该门控内含出生点保护，
    // 且 `isUnderSpawnProtection` 只按 op 身份与半径判定、不区分游戏模式，故旁观者同样受限。
    auto preconditionError = _validateInteractionPreconditions(playerId, pos, false, true);
    if (preconditionError) {
        return std::move(*preconditionError);
    }

    ServerWorld* world = _getPlayerWorld(playerId);
    if (world == nullptr) {
        return Error(ErrorCode::InvalidWorld, "Player world not available");
    }

    const BlockState* state = _getNonAirBlockState(*world, pos);
    if (!state) {
        return BlockUseResult{false,
            false,
            BlockUseResult::Kind::BlockUseWithoutItem,
            ActionResultType::Pass,
            std::nullopt,
            "No block to use"};
    }

    auto* playerData = _validatePlayer(playerId);
    if (!playerData) {
        return Error(ErrorCode::InvalidArgument, "Player not found or not logged in");
    }

    Block* block = BlockRegistry::instance().getBlock(state->blockId());
    if (!block) {
        return Error(ErrorCode::NotFound, "Block not found for state");
    }

    Player* player = _getPlayerEntity(playerId, *world);

    // ===== 旁观者分支（对齐 vanilla ServerPlayerGameMode.useItemOn 旁观分支）=====
    // 旁观者不执行任何方块/物品业务，只按方块声明的菜单类型打开界面（箱子、熔炉等）；
    // 打开成功即返回 Consume，否则 Pass。
    if (playerData->gameMode == GameMode::Spectator) {
        if (player != nullptr) {
            if (const auto menuType = block->getMenuProvider(*state, *world, pos)) {
                // 旁观者开箱按方块声明的类型统一打开（对齐 vanilla ServerPlayer.openMenu）。
                if (world->openContainer(*menuType, pos, *player)) {
                    return BlockUseResult{true,
                        false,
                        BlockUseResult::Kind::BlockUseWithoutItem,
                        ActionResultType::Consume,
                        std::nullopt,
                        "Spectator opened block menu"};
                }
            }
        }
        return BlockUseResult{false,
            false,
            BlockUseResult::Kind::BlockUseWithoutItem,
            ActionResultType::Pass,
            std::nullopt,
            "Spectator pass"};
    }

    // ===== 潜行门控（isSecondaryUseActive 语义）=====
    // 玩家按住潜行键且**交互手**持物非空时，跳过方块侧交互直接走物品使用（Item::useOn），
    // 用于「潜行时放置方块而非打开容器 / 触发方块」。门控条件对齐 vanilla
    // ServerPlayerGameMode.useItemOn 的 `flag1 = isSecondaryUseActive() && flag`，其中
    // `flag = 任一手非空`、`flag1` 只决定是否**跳过方块侧**；真正的物品使用仍以
    // `!p_9268_.isEmpty()`（p_9268_ = getItemInHand(交互手)）为前置，空手时落到末尾返回 PASS。
    if (player != nullptr && player->isInputSneaking() && _anyHandHasItem(playerId)) {
        const ItemStack& heldStack = _handStackFor(playerId, hand);
        const Item* heldItem = heldStack.getItem();
        // 交互手为空或物品在冷却中：vanilla 的 `!p_9268_.isEmpty() && !isOnCooldown(p_9268_)`
        // 不成立，直接返回 PASS（不派发方块侧，也不使用物品）。
        if (heldItem == nullptr || heldStack.isEmpty() || player->hasItemCooldown(heldItem)) {
            return BlockUseResult{false,
                false,
                BlockUseResult::Kind::ItemUseOn,
                ActionResultType::Pass,
                std::nullopt,
                "Sneaking but item unusable"};
        }
        // 传局部拷贝：避免物品侧 getItemStackMut() 直接改权威槽导致的双重消耗（见 handleItemUseOn 注释）。
        auto itemUse = handleItemUseOn(playerId, pos, hitPos, face, hand, heldStack);
        return itemUse.success() ? itemUse.value()
                                 : BlockUseResult{false,
                                       false,
                                       BlockUseResult::Kind::ItemUseOn,
                                       ActionResultType::Pass,
                                       std::nullopt,
                                       "Sneaking but item use unhandled"};
    }

    // ===== 常规路径：方块侧 useItemOn → useWithoutItem → 物品侧 Item::useOn =====
    // ③ 方块侧 useItemOn（对齐 vanilla：**任意手都调用**，空手也调——如 CandleBlock 空手熄灭）。
    //    返回 consumesAction 时短路；否则（Pass/Fail/TryEmptyHandInteraction）继续向下。
    const bool heldItemEmpty = _handStackFor(playerId, hand).isEmpty();
    const std::optional<BlockUseResult> itemOn =
        _dispatchBlockUse(playerId, *world, pos, hand, hitPos, face, /*withItem=*/true, nullptr);
    if (itemOn.has_value() && itemOn->success) {
        return *itemOn;
    }

    // ④ 空手交互（对齐 vanilla useWithoutItem）。仅当 useItemOn 返回 TRY_WITH_EMPTY_HAND
    //    且为主线时回退（vanilla `interactionhand == MAIN_HAND`）；返回 Pass/Fail 不回退。
    //
    //    注意这里**不能**追加「手必须为空」的条件：原版 ServerPlayerGameMode.useItemOn
    //    的判据只有 `interactionresult instanceof TRY_WITH_EMPTY_HAND && hand == MAIN_HAND`
    //    （见原版 :333-386），与手上有没有物品无关。方块返回 TRY_WITH_EMPTY_HAND 的语义是
    //    「这件物品我不处理，请走空手分支」，因此手持非空物品同样要回退：
    //    - 日光探测器 DaylightDetectorBlock 只实现 useWithoutItem，手持木棍右键要能切换反相；
    //    - 花盆 FlowerPotBlock.useItemOn 对非可盆栽物品返回 TRY_WITH_EMPTY_HAND，
    //      其 useWithoutItem 对空花盆返回 CONSUME（consumesAction 为真），
    //      故原版手持木棍右键空花盆同样返回 true。
    //    两个用例都依赖这一路径，此前加 heldItemEmpty 门控把它们一起打断了。
    if (hand == Hand::MainHand && itemOn.has_value() &&
        itemOn->actionResult == ActionResultType::TryEmptyHandInteraction) {
        const auto withoutItem =
            _dispatchBlockUse(playerId, *world, pos, hand, hitPos, face, /*withItem=*/false, nullptr);
        if (withoutItem.has_value() && withoutItem->success) {
            return *withoutItem;
        }
    }

    // ⑤ 物品侧交互（对齐 vanilla `itemstack.useOn(new UseOnContext(...))`，需手持非空且不在冷却）。
    //    矿车 / 骨粉 / 桶 / 火把 / 锄头 / 斧头 / 锹 / 刷子等非 block-item 靠此步生效；block-item
    //    的放置也经此步（handleItemUseOn 识别 block-item 后转 handleBlockPlacement）。
    //    手持物一律从权威槽重新解析（`_handStackFor`），不缓存跨调用的指针。
    if (!heldItemEmpty) {
        const ItemStack& heldStack = _handStackFor(playerId, hand);
        const Item* heldItem = heldStack.getItem();
        if (heldItem != nullptr && player != nullptr && player->hasItemCooldown(heldItem)) {
            return BlockUseResult{false,
                false,
                BlockUseResult::Kind::ItemUseOn,
                ActionResultType::Pass,
                std::nullopt,
                "Item on cooldown"};
        }
        // 传局部拷贝：ItemUseContext 持其指针，物品侧 getItemStackMut() 的自改只作用于拷贝，
        // 权威槽的消耗仍由 handleItemUseOn 的 itemId/damage 对比统一判定（避免双重消耗）。
        auto itemUse = handleItemUseOn(playerId, pos, hitPos, face, hand, heldStack);
        if (itemUse.success() && itemUse.value().success) {
            return itemUse.value();
        }
    }

    return BlockUseResult{false,
        false,
        BlockUseResult::Kind::BlockUseWithoutItem,
        ActionResultType::Pass,
        std::nullopt,
        "No interaction handled"};
}

Result<BlockBreakResult> BlockInteractionManager::handleBlockBreak(PlayerId playerId, const BlockPos& pos)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.World,
        "BlockInteractionManager::handleBlockBreak",
        "pos",
        pos.toString(),
        "playerId",
        playerId,
        [flow = ::perfetto::Flow::ProcessScoped(pos.toId())](::perfetto::EventContext ctx) {
            flow(ctx);
        });

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
    auto preconditionError = _validateInteractionPreconditions(playerId, pos, true, true);
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

    // 工具/物品的破坏前钩子（对齐 vanilla Item.canDestroyBlock：调试棒左键选中属性等）。
    // 返回 false 表示"不允许破坏该方块"，中止后续破坏流程（如调试棒左键仅切换属性、不破坏方块）。
    if (!tool.isEmpty()) {
        if (Player* breakerEntity = _getPlayerEntity(playerId, *world)) {
            if (tool.getItem() != nullptr &&
                !tool.getItem()->canDestroyBlock(tool, oldState, *world, pos, *breakerEntity)) {
                return BlockBreakResult{false, 0, "Block break prevented by item"};
            }
        }
    }

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
        // 方块经验（矿石、幽匿块等）：对齐原版 `spawnAfterBreak(dropExp=true)` 之后的
        // `tryDropExperience`，在掉落物之后、方块移除之前结算。创造模式不掉，
        // 与原版 `Player.destroyBlock` 里 `!isCreative` 的分支一致。
        _dropBlockExperience(*world, pos, oldState, tool.isEmpty() ? nullptr : &tool);
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

    // 发送方块破坏粒子效果（对齐原版 Block.destroy 中 level.levelEvent(2001, pos,
    // Block.getId(state))）：data 为方块状态ID，客户端据此渲染对应方块的碎屑粒子。
    // 此前挖块路径仅发破坏声音（m_onBlockBreak 回调），未发破坏粒子，方块直接消失无碎屑。
    world->playEvent(world::WorldEvents::BREAK_BLOCK_EFFECTS, pos, static_cast<i32>(oldState.stateId()));

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
        [flow = ::perfetto::Flow::ProcessScoped(pos.toId())](::perfetto::EventContext ctx) {
            flow(ctx);
        });

    Player* player = _getPlayerEntity(playerId, world);

    // 使用 BlockDropHandler 生成掉落物
    auto drops = BlockDropHandler::generateDrops(world, pos, state, player, tool, m_lootTableManager);

    if (!drops.empty()) {
        const std::string throwerUuid = player != nullptr ? player->uuid() : std::string();
        BlockDropHandler::spawnDrops(world, pos, drops, throwerUuid);
    }

    // 方块经验不在这里结算：原版由 `spawnAfterBreak(dropExp=true)` 统一处理（见
    // `_dropBlockExperience`），那条路径同时覆盖方块移除后的时机，也带上精准采集判定。
    // 此处曾经用「位置+玩家」拼出的种子自造随机源，与原版消耗世界随机数的行为不同。
}

void BlockInteractionManager::_dropBlockExperience(
    ServerWorld& world, const BlockPos& pos, const BlockState& state, const ItemStack* tool)
{
    // 精准采集不掉经验（vanilla `EnchantmentHelper.processBlockExperience` 在工具带精准采集
    // 且方块掉经验时返回 0）。
    if (tool != nullptr && !tool->isEmpty() && item::enchant::EnchantmentHelper::hasSilkTouch(*tool)) {
        return;
    }
    const i32 experience = state.getBlock().dropExperience(world.getRandom());
    if (experience <= 0) {
        return;
    }
    entity::ExperienceDropHandler::spawnExperienceOrbs(&world,
        static_cast<f64>(pos.x) + 0.5,
        static_cast<f64>(pos.y) + 0.5,
        static_cast<f64>(pos.z) + 0.5,
        experience);
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
