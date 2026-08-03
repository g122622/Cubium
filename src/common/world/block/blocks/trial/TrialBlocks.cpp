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
 * IMPLIED, INCLUDING BUT NOT LIMITED TO ANY PURPOSE AND NONINFRINGEMENT. IN NO
 * EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES
 * OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "TrialBlocks.hpp"
#include "common/core/BlockRaycastResult.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/entities/item/ItemEntity.hpp"
#include "common/entity/inventory/ContainerTypes.hpp"
#include "common/entity/inventory/CraftingInventory.hpp"
#include "common/entity/inventory/IInventory.hpp"
#include "common/entity/inventory/ISidedInventory.hpp"
#include "common/entity/utils/ItemDropHelper.hpp"
#include "common/item/core/ActionResult.hpp"
#include "common/item/core/BlockActionResult.hpp"
#include "common/item/crafting/RecipeManager.hpp"
#include "common/util/Direction.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/property/Properties.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/util/property/StateHolder.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/blocks/HorizontalBlock.hpp"
#include "common/world/blockentity/BlockEntityType.hpp"
#include "item/context/BlockItemUseContext.hpp"
#include "util/math/random/Random.hpp"
#include "world/WorldEvents.hpp"
#include "world/block/BlockState.hpp"
#include "world/blockentity/trial/CrafterBlockEntity.hpp"
#include "world/blockentity/trial/TrialSpawnerBlockEntity.hpp"
#include "world/blockentity/trial/VaultBlockEntity.hpp"
#include "world/redstone/RedstoneSystem.hpp"
#include "world/tick/base/TickPriority.hpp"
#include "world/tick/manager/TickManager.hpp"
#include <algorithm>
#include <chrono>
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

namespace mc {
namespace blocks {

// ============================================================================
// TrialSpawnerBlock
// ============================================================================

TrialSpawnerBlock::TrialSpawnerBlock(const BlockProperties& properties)
    : Block(properties)
{
    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(BlockStateProperties::TRIAL_SPAWNER_STATE())
            .add(BlockStateProperties::OMINOUS())
            .create([](const Block& block,
                        std::vector<size_t> values,
                        const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
                        const std::vector<BlockState*>* allStates,
                        u32 id) {
                return std::make_unique<BlockState>(block, std::move(values), propertyLayouts, allStates, id);
            });
    createBlockState(std::move(container));

    setDefaultState(defaultState()
            .with(BlockStateProperties::TRIAL_SPAWNER_STATE(), BlockStateProperties::TrialSpawnerState::Inactive)
            .with(BlockStateProperties::OMINOUS(), false));
}

std::unique_ptr<BlockEntity> TrialSpawnerBlock::createBlockEntity(const BlockPos& pos)
{
    return std::make_unique<TrialSpawnerBlockEntity>(pos);
}

void TrialSpawnerBlock::fillStateContainer(StateContainer<Block, BlockState>& container)
{
    MC_UNUSED(container);
}

BlockState TrialSpawnerBlock::getStateForPlacement(BlockItemUseContext& context)
{
    MC_UNUSED(context);
    return defaultState()
        .with(BlockStateProperties::TRIAL_SPAWNER_STATE(), BlockStateProperties::TrialSpawnerState::Inactive)
        .with(BlockStateProperties::OMINOUS(), false);
}

i32 TrialSpawnerBlock::getComparatorInputOverride(const BlockState& state, IWorld& world, const BlockPos& pos) const
{
    MC_UNUSED(world);
    MC_UNUSED(pos);

    auto trialState = state.get(BlockStateProperties::TRIAL_SPAWNER_STATE());
    switch (trialState) {
        case BlockStateProperties::TrialSpawnerState::WaitingForPlayers:
            return 1;
        case BlockStateProperties::TrialSpawnerState::Active:
            return 2;
        case BlockStateProperties::TrialSpawnerState::WaitingForRewardEjection:
        case BlockStateProperties::TrialSpawnerState::EjectingReward:
            return 3;
        case BlockStateProperties::TrialSpawnerState::Cooldown:
            return 4;
        default:
            return 0;
    }
}

// ============================================================================
// VaultBlock
// ============================================================================

VaultBlock::VaultBlock(const BlockProperties& properties)
    : HorizontalBlock(properties)
{
    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(FACING())
            .add(BlockStateProperties::VAULT_STATE())
            .add(BlockStateProperties::OMINOUS())
            .create([](const Block& block,
                        std::vector<size_t> values,
                        const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
                        const std::vector<BlockState*>* allStates,
                        u32 id) {
                return std::make_unique<BlockState>(block, std::move(values), propertyLayouts, allStates, id);
            });
    createBlockState(std::move(container));

    setDefaultState(defaultState()
            .with(FACING(), Direction::North)
            .with(BlockStateProperties::VAULT_STATE(), BlockStateProperties::VaultState::Inactive)
            .with(BlockStateProperties::OMINOUS(), false));
}

std::unique_ptr<BlockEntity> VaultBlock::createBlockEntity(const BlockPos& pos)
{
    return std::make_unique<VaultBlockEntity>(pos);
}

void VaultBlock::fillStateContainer(StateContainer<Block, BlockState>& container)
{
    MC_UNUSED(container);
}

BlockState VaultBlock::getStateForPlacement(BlockItemUseContext& context)
{
    return defaultState()
        .with(FACING(), Directions::opposite(context.horizontalDirection()))
        .with(BlockStateProperties::VAULT_STATE(), BlockStateProperties::VaultState::Inactive)
        .with(BlockStateProperties::OMINOUS(), false);
}

// ============================================================================
// CrafterBlock
// ============================================================================

CrafterBlock::CrafterBlock(const BlockProperties& properties)
    : HorizontalBlock(properties)
{
    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(FACING())
            .add(BlockStateProperties::TRIGGERED())
            .add(BlockStateProperties::CRAFTING())
            .create([](const Block& block,
                        std::vector<size_t> values,
                        const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
                        const std::vector<BlockState*>* allStates,
                        u32 id) {
                return std::make_unique<BlockState>(block, std::move(values), propertyLayouts, allStates, id);
            });
    createBlockState(std::move(container));

    setDefaultState(defaultState()
            .with(FACING(), Direction::North)
            .with(BlockStateProperties::TRIGGERED(), false)
            .with(BlockStateProperties::CRAFTING(), false));
}

std::unique_ptr<BlockEntity> CrafterBlock::createBlockEntity(const BlockPos& pos)
{
    return std::make_unique<CrafterBlockEntity>(pos);
}

void CrafterBlock::fillStateContainer(StateContainer<Block, BlockState>& container)
{
    MC_UNUSED(container);
}

BlockState CrafterBlock::getStateForPlacement(BlockItemUseContext& context)
{
    return defaultState()
        .with(FACING(), Directions::opposite(context.horizontalDirection()))
        .with(BlockStateProperties::TRIGGERED(), false)
        .with(BlockStateProperties::CRAFTING(), false);
}

void CrafterBlock::neighborChanged(
    IWorld& world, const BlockPos& pos, Block& neighborBlock, const BlockPos& neighborPos, bool isMoving)
{
    MC_UNUSED(neighborBlock);
    MC_UNUSED(neighborPos);
    MC_UNUSED(isMoving);

    const BlockState* state = world.getBlockState(pos);
    if (state == nullptr) {
        return;
    }

    bool isPowered = world::redstone::RedstoneSystem::instance().isBlockPowered(world, pos);
    bool wasTriggered = state->get(BlockStateProperties::TRIGGERED());

    if (isPowered && !wasTriggered) {
        // 红石信号上升沿：调度4 tick延时后执行合成
        world.tickManager().scheduleBlockTick(pos, *this, CRAFTING_TICK_DELAY, world::tick::TickPriority::High);
        BlockState newState = state->with(BlockStateProperties::TRIGGERED(), true);
        world.setBlockState(pos, &newState, 2);

        // 同步方块实体的触发状态
        BlockEntity* be = world.getBlockEntity(pos);
        if (auto* crafter = dynamic_cast<CrafterBlockEntity*>(be)) {
            crafter->setTriggered(true);
        }
    } else if (!isPowered && wasTriggered) {
        // 红石信号下降沿：重置触发状态和合成状态
        BlockState newState =
            state->with(BlockStateProperties::TRIGGERED(), false).with(BlockStateProperties::CRAFTING(), false);
        world.setBlockState(pos, &newState, 2);

        // 同步方块实体的触发状态
        BlockEntity* be = world.getBlockEntity(pos);
        if (auto* crafter = dynamic_cast<CrafterBlockEntity*>(be)) {
            crafter->setTriggered(false);
            crafter->setCraftingTicksRemaining(0);
        }
    }
}

void CrafterBlock::onBlockRemoved(IWorld& world, const BlockPos& pos, const BlockState& state)
{
    MC_UNUSED(state);

    // 方块移除时掉落合成器内的物品
    BlockEntity* entity = world.getBlockEntity(pos);
    if (entity != nullptr && entity->getType() == BlockEntityType::Crafter) {
        auto* crafter = static_cast<CrafterBlockEntity*>(entity);
        IInventory* inventory = crafter->getInventory();

        // 掉落所有物品
        math::Random rng;
        for (i32 i = 0; i < inventory->getContainerSize(); ++i) {
            ItemStack stack = inventory->removeItemNoUpdate(i);
            if (!stack.isEmpty()) {
                ItemDropHelper::spawnItemEntity(&world, stack, pos.x + 0.5, pos.y + 0.5, pos.z + 0.5, rng);
            }
        }

        // 通知比较器更新
        world::redstone::RedstoneSystem::instance().updateComparators(world, pos);
    }

    Block::onBlockRemoved(world, pos, state);
}

BlockActionResult CrafterBlock::onBlockActivated(const BlockState& state,
    IWorld& world,
    const BlockPos& pos,
    Player& player,
    Hand hand,
    const BlockRaycastResult& hit)
{
    MC_UNUSED(state);
    MC_UNUSED(hand);
    MC_UNUSED(hit);

    if (world.isClientSide()) {
        return ActionResultType::Success;
    }

    BlockEntity* entity = world.getBlockEntity(pos);
    if (entity != nullptr && entity->getType() == BlockEntityType::Crafter) {
        if (world.openContainer(ContainerType::Crafter, pos, player)) {
            return ActionResultType::Consume;
        }
    }

    return ActionResultType::Pass;
}

void CrafterBlock::tick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random)
{
    MC_UNUSED(random);
    _dispenseFrom(world, pos, state);
}

void CrafterBlock::_dispenseFrom(IWorld& world, const BlockPos& pos, const BlockState& state)
{
    BlockEntity* be = world.getBlockEntity(pos);
    auto* crafter = dynamic_cast<CrafterBlockEntity*>(be);
    if (crafter == nullptr) {
        return;
    }

    // 构建合成输入并查找匹配配方
    CraftingInventory craftingInput = crafter->asCraftInput();
    const crafting::CraftingRecipe* recipe = crafting::RecipeManager::instance().findMatchingRecipe(craftingInput);

    if (recipe == nullptr) {
        // 没有匹配配方，播放失败音效
        // 参考 MC: CrafterBlock._dispenseFrom() levelEvent(SOUND_CRAFTER_FAIL, pos, 0)
        world.playEvent(world::WorldEvents::CRAFTER_FAIL_SOUND, pos, 0);
        return;
    }

    ItemStack result = recipe->assemble(craftingInput);
    if (result.isEmpty()) {
        // 配方结果为空，播放失败音效
        // 参考 MC: CrafterBlock._dispenseFrom() levelEvent(SOUND_CRAFTER_FAIL, pos, 0)
        world.playEvent(world::WorldEvents::CRAFTER_FAIL_SOUND, pos, 0);
        return;
    }

    // 合成成功：设置合成动画倒计时和 CRAFTING 状态
    crafter->setCraftingTicksRemaining(CrafterBlockEntity::MAX_CRAFTING_TICKS);
    BlockState craftingState = state.with(BlockStateProperties::CRAFTING(), true);
    world.setBlockState(pos, &craftingState, 2);

    // 播放合成成功音效
    // 参考 MC: CrafterBlock._dispenseFrom() levelEvent(SOUND_CRAFTER_CRAFT, pos, 0)
    world.playEvent(world::WorldEvents::CRAFTER_CRAFT_SOUND, pos, 0);

    // 射出合成结果时产生方向性白烟粒子
    // 参考 MC: CrafterBlock._dispenseFrom() levelEvent(PARTICLES_SHOOT_WHITE_SMOKE, pos, direction.get3DDataValue())
    Direction facing = state.get(HorizontalBlock::FACING());
    world.playEvent(world::WorldEvents::SHOOT_WHITE_SMOKE, pos, static_cast<i32>(facing));

    // 射出合成结果
    _spawnItemEntity(world, pos, facing, result);

    // 射出剩余物品（如空桶等容器物品）
    std::vector<ItemStack> remainingItems = recipe->getRemainingItems(craftingInput);
    for (ItemStack& remaining : remainingItems) {
        if (!remaining.isEmpty()) {
            _spawnItemEntity(world, pos, facing, remaining);
        }
    }

    // 消耗原料：每个非空槽位减少1个物品
    IInventory* inventory = crafter->getInventory();
    for (i32 slot = 0; slot < CrafterBlockEntity::CONTAINER_SIZE; ++slot) {
        ItemStack stack = inventory->getItem(slot);
        if (!stack.isEmpty()) {
            stack.shrink(1);
            inventory->setItem(slot, stack.isEmpty() ? ItemStack() : stack);
        }
    }

    crafter->setChanged();
}

void CrafterBlock::_spawnItemEntity(IWorld& world, const BlockPos& pos, Direction facing, ItemStack stack)
{
    if (stack.isEmpty()) {
        return;
    }

    // 尝试将物品注入面前容器
    BlockPos targetPos = pos.offset(facing);
    BlockEntity* targetEntity = world.getBlockEntity(targetPos);
    if (targetEntity != nullptr) {
        IInventory* targetInventory = dynamic_cast<IInventory*>(targetEntity);
        if (targetInventory != nullptr) {
            // 获取插入方向（从容器的视角看，是从facing的反方向插入）
            Direction insertDirection = Directions::opposite(facing);

            // 检查目标是否为侧面受限容器（ISidedInventory）
            auto* sidedInventory = dynamic_cast<ISidedInventory*>(targetInventory);

            if (sidedInventory != nullptr) {
                // 侧面受限容器：只通过允许的槽位插入
                std::vector<i32> slots = sidedInventory->getSlotsForFace(insertDirection);
                for (i32 slot : slots) {
                    if (stack.isEmpty()) {
                        break;
                    }
                    if (!sidedInventory->canInsertItem(slot, stack, insertDirection)) {
                        continue;
                    }
                    // 首先尝试堆叠到现有物品
                    ItemStack existingStack = sidedInventory->getItem(slot);
                    if (!existingStack.isEmpty() && existingStack.isSameItem(stack)) {
                        i32 space = existingStack.getMaxStackSize() - existingStack.getCount();
                        if (space > 0) {
                            i32 toAdd = std::min(space, stack.getCount());
                            existingStack.grow(toAdd);
                            sidedInventory->setItem(slot, existingStack);
                            stack.shrink(toAdd);
                        }
                    }
                }

                // 如果还有剩余，尝试放入空槽位
                if (!stack.isEmpty()) {
                    for (i32 slot : slots) {
                        if (stack.isEmpty()) {
                            break;
                        }
                        if (!sidedInventory->canInsertItem(slot, stack, insertDirection)) {
                            continue;
                        }
                        if (sidedInventory->getItem(slot).isEmpty()) {
                            sidedInventory->setItem(slot, stack);
                            stack = ItemStack();
                        }
                    }
                }

                // ISidedInventory物品全部注入成功，直接返回
                if (stack.isEmpty()) {
                    return;
                }
            } else {
                // 普通容器：首先尝试堆叠到现有槽位
                for (i32 i = 0; i < targetInventory->getContainerSize(); ++i) {
                    ItemStack existingStack = targetInventory->getItem(i);
                    if (!existingStack.isEmpty() && existingStack.isSameItem(stack)) {
                        i32 space = existingStack.getMaxStackSize() - existingStack.getCount();
                        if (space > 0) {
                            i32 toAdd = std::min(space, stack.getCount());
                            existingStack.grow(toAdd);
                            targetInventory->setItem(i, existingStack);
                            stack.shrink(toAdd);
                            if (stack.isEmpty()) {
                                return;
                            }
                        }
                    }
                }

                // 如果还有剩余，尝试放入空槽位
                if (!stack.isEmpty()) {
                    i32 emptySlot = targetInventory->getFirstEmptySlot();
                    if (emptySlot >= 0) {
                        targetInventory->setItem(emptySlot, stack);
                        return;
                    }
                }
            }

            // 容器无法完全接收，物品仍留在stack中，继续弹出到世界
        }
    }

    // 弹出到世界中：在方块面朝方向偏移0.7格处生成物品实体
    constexpr f32 DISPENSE_SPEED = 0.2f;
    constexpr f32 INACCURACY = 0.0074999998f;

    const f32 x = static_cast<f32>(pos.x) + 0.5f + static_cast<f32>(Directions::xOffset(facing)) * 0.7f;
    const f32 y = static_cast<f32>(pos.y) + 0.5f + static_cast<f32>(Directions::yOffset(facing)) * 0.7f;
    const f32 z = static_cast<f32>(pos.z) + 0.5f + static_cast<f32>(Directions::zOffset(facing)) * 0.7f;

    f32 vx = static_cast<f32>(Directions::xOffset(facing)) * DISPENSE_SPEED;
    f32 vy = static_cast<f32>(Directions::yOffset(facing)) * DISPENSE_SPEED + 0.1f;
    f32 vz = static_cast<f32>(Directions::zOffset(facing)) * DISPENSE_SPEED;

    // 添加随机散射
    thread_local math::Random rng(static_cast<u64>(std::chrono::steady_clock::now().time_since_epoch().count()));
    vx += rng.nextGaussian(0.0f, INACCURACY);
    vy += rng.nextGaussian(0.0f, INACCURACY);
    vz += rng.nextGaussian(0.0f, INACCURACY);

    auto itemEntity = std::make_unique<ItemEntity>(EntityInstanceId(0), stack, x, y, z, vx, vy, vz);

    // 直接构造的实体需要显式设置 typeId（注册表路径会自动设置）
    itemEntity->setTypeId(entity::EntityTypeKeys::ITEM);

    itemEntity->setPickupDelay(10);
    world.spawnEntity(std::move(itemEntity));
}

BlockState CrafterBlock::updatePostPlacement(const BlockState& state,
    Direction facing,
    const BlockState& facingState,
    IWorld& world,
    const BlockPos& currentPos,
    const BlockPos& facingPos)
{
    MC_UNUSED(facing);
    MC_UNUSED(facingState);
    MC_UNUSED(world);
    MC_UNUSED(currentPos);
    MC_UNUSED(facingPos);
    return state;
}

i32 CrafterBlock::getWeakPower(
    const BlockState& state, IWorld& world, const BlockPos& pos, Direction side) const noexcept
{
    MC_UNUSED(world);
    MC_UNUSED(pos);
    MC_UNUSED(side);

    // 合成时输出满信号
    if (state.get(BlockStateProperties::CRAFTING())) {
        return 15;
    }
    return 0;
}

i32 CrafterBlock::getComparatorInputOverride(const BlockState& state, IWorld& world, const BlockPos& pos) const
{
    MC_UNUSED(state);

    // 比较器信号强度 = 非空槽位数 + 禁用槽位数
    BlockEntity* be = world.getBlockEntity(pos);
    if (auto* crafter = dynamic_cast<CrafterBlockEntity*>(be)) {
        return crafter->getRedstoneSignal();
    }
    return 0;
}

} // namespace blocks
} // namespace mc
