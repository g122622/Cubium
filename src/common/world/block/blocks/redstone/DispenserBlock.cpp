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

#include "DispenserBlock.hpp"
#include "../../../../entity/entities/item/ItemEntity.hpp"
#include "../../../../entity/inventory/IInventory.hpp"
#include "../../../../item/core/ItemStack.hpp"
#include "../../../../sound/SoundCategory.hpp"
#include "../../../../sound/SoundEvents.hpp"
#include "../../../../util/math/random/Random.hpp"
#include "../../../IWorld.hpp"
#include "../../../blockentity/BlockEntity.hpp"
#include "../../../blockentity/BlockEntityType.hpp"
#include "../../../blockentity/interactive/DispenserBlockEntity.hpp"
#include "../../../redstone/RedstonePower.hpp"
#include "../../../tick/base/TickPriority.hpp"
#include "../../../tick/manager/TickManager.hpp"
#include "../../dispense/DispenseItemBehaviorRegistry.hpp"
#include "common/core/Types.hpp"
#include "common/util/Direction.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/math/random/IRandom.hpp"
#include "common/util/property/Properties.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/util/property/StateHolder.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/dispense/IDispenseItemBehavior.hpp"
#include <algorithm>
#include <chrono>
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

namespace mc {
namespace blocks {

DispenserBlock::DispenserBlock(const BlockProperties& properties)
    : Block(properties)
{

    // 创建状态容器
    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(BlockStateProperties::FACING())
            .add(BlockStateProperties::TRIGGERED())
            .create([](const Block& block,
                        std::vector<size_t> values,
                        const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
                        const std::vector<BlockState*>* allStates,
                        u32 id) {
                return std::make_unique<BlockState>(block, std::move(values), propertyLayouts, allStates, id);
            });
    createBlockState(std::move(container));

    // 设置默认状态
    setDefaultState(defaultState()
            .with(BlockStateProperties::FACING(), Direction::North)
            .with(BlockStateProperties::TRIGGERED(), false));
}

bool DispenserBlock::isTriggered(const BlockState& state)
{
    return state.get(BlockStateProperties::TRIGGERED());
}

BlockState DispenserBlock::withTriggered(BlockState state, bool triggered)
{
    return state.with(BlockStateProperties::TRIGGERED(), triggered);
}

Direction DispenserBlock::getFacing(const BlockState& state)
{
    return state.get(BlockStateProperties::FACING());
}

void DispenserBlock::onBlockAdded(IWorld& world, const BlockPos& pos, const BlockState& state)
{
    // 发射器放置时不触发额外行为
}

void DispenserBlock::neighborChanged(
    IWorld& world, const BlockPos& pos, Block& neighborBlock, const BlockPos& neighborPos, bool isMoving)
{
    MC_UNUSED(neighborBlock);
    MC_UNUSED(neighborPos);
    MC_UNUSED(isMoving);

    const BlockState* state = world.getBlockState(pos);
    if (!state) {
        return;
    }

    // 检查是否应该触发
    bool shouldTrigger = world::redstone::RedstonePower::isPowered(world, pos);
    bool isCurrentlyTriggered = isTriggered(*state);

    if (shouldTrigger != isCurrentlyTriggered) {
        if (shouldTrigger) {
            // 被激活，调度发射
            world.tickManager().scheduleBlockTick(pos, *this, 4, world::tick::TickPriority::High);
        }
        // 更新触发状态
        BlockState newState = withTriggered(*state, shouldTrigger);
        world.setBlockState(pos, &newState, 2);
    }
}

BlockState DispenserBlock::updatePostPlacement(const BlockState& state,
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

void DispenserBlock::tick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random)
{
    // 注意：random 参数未使用，因为发射器使用独立的 thread_local 随机数生成器
    // 这确保发射器的随机行为独立于 tick 系统的随机数序列
    MC_UNUSED(random);
    dispense(world, pos, state);
}

void DispenserBlock::dispense(IWorld& world, const BlockPos& pos, const BlockState& state)
{
    // 尝试发射物品
    if (tryDispense(world, pos, state)) {
        playDispenseSound(world, pos);
    }
}

bool DispenserBlock::tryDispense(IWorld& world, const BlockPos& pos, const BlockState& state)
{
    // 获取方块实体
    BlockEntity* blockEntity = world.getBlockEntity(pos);
    if (blockEntity == nullptr) {
        return false;
    }

    // 转换为发射器方块实体
    blockentity::DispenserBlockEntity* dispenser = dynamic_cast<blockentity::DispenserBlockEntity*>(blockEntity);
    if (dispenser == nullptr) {
        return false;
    }

    // 使用储水池采样算法选择非空槽位
    i32 slot = dispenser->getDispenseSlot();
    if (slot < 0) {
        return false; // 没有物品可发射
    }

    // 获取物品
    IInventory* inventory = dispenser->getInventory();
    if (inventory == nullptr) {
        return false;
    }

    ItemStack stack = inventory->getItem(slot);
    if (stack.isEmpty()) {
        return false;
    }

    // 获取发射方向
    Direction facing = getFacing(state);
    BlockPos targetPos = pos.offset(facing);

    // 发射物品 - 分离一个物品
    ItemStack dispensedStack = stack.split(1);

    // 检查是否有注册的特殊发射行为
    IDispenseItemBehavior* behavior = DispenseItemBehaviorRegistry::instance().getBehavior(dispensedStack);
    if (behavior != nullptr) {
        // 使用特殊发射行为，传递发射器库存以便 consumeWithRemainder 使用
        dispensedStack = behavior->dispense(world, pos, state, dispensedStack, inventory);
    } else {
        // 使用默认行为：发射物品实体
        dispensedStack = defaultDispense(world, pos, facing, targetPos, dispensedStack);
    }

    // 更新或移除原槽位物品
    if (dispensedStack.isEmpty()) {
        inventory->setItem(slot, ItemStack::EMPTY);
    } else {
        // 如果物品未完全发射，返回原槽位
        stack.grow(dispensedStack.getCount());
        inventory->setItem(slot, stack);
    }

    dispenser->setChanged();
    return true;
}

ItemStack DispenserBlock::defaultDispense(
    IWorld& world, const BlockPos& pos, Direction facing, const BlockPos& targetPos, ItemStack stack)
{

    // 检查目标位置是否有容器
    BlockEntity* targetEntity = world.getBlockEntity(targetPos);
    if (targetEntity != nullptr) {
        IInventory* targetInventory = dynamic_cast<IInventory*>(targetEntity);
        if (targetInventory != nullptr) {
            // 首先尝试堆叠到现有槽位
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
                            return ItemStack::EMPTY;
                        }
                    }
                }
            }

            // 如果还有剩余，尝试放入空槽位
            if (!stack.isEmpty()) {
                i32 emptySlot = targetInventory->getFirstEmptySlot();
                if (emptySlot >= 0) {
                    targetInventory->setItem(emptySlot, stack);
                    return ItemStack::EMPTY;
                }
            }

            // 容器无法完全接收，物品返回
            return stack;
        }
    }

    // 没有容器，生成物品实体
    spawnItemEntity(world, pos, facing, stack);
    return ItemStack::EMPTY;
}

void DispenserBlock::spawnItemEntity(IWorld& world, const BlockPos& pos, Direction facing, const ItemStack& stack)
{
    // 发射速度常量
    constexpr f32 DISPENSE_SPEED = 0.2f;
    constexpr f32 INACCURACY = 0.0074999998f;

    // 计算发射位置（发射方向偏移 0.7 格）
    const f32 x = static_cast<f32>(pos.x) + 0.5f + static_cast<f32>(Directions::xOffset(facing)) * 0.7f;
    const f32 y = static_cast<f32>(pos.y) + 0.5f + static_cast<f32>(Directions::yOffset(facing)) * 0.7f;
    const f32 z = static_cast<f32>(pos.z) + 0.5f + static_cast<f32>(Directions::zOffset(facing)) * 0.7f;

    // 计算发射速度
    f32 vx = static_cast<f32>(Directions::xOffset(facing)) * DISPENSE_SPEED;
    f32 vy = static_cast<f32>(Directions::yOffset(facing)) * DISPENSE_SPEED;
    f32 vz = static_cast<f32>(Directions::zOffset(facing)) * DISPENSE_SPEED;

    // 使用 thread_local 随机数生成器生成高斯偏移
    thread_local math::Random rng(static_cast<u64>(std::chrono::steady_clock::now().time_since_epoch().count()));
    vx += rng.nextGaussian(0.0f, INACCURACY);
    vy += rng.nextGaussian(0.0f, INACCURACY) + 0.1f; // Y方向额外加一点，模拟发射时的小跳
    vz += rng.nextGaussian(0.0f, INACCURACY);

    // 创建物品实体
    auto itemEntity = std::make_unique<ItemEntity>(EntityInstanceId(0), stack, x, y, z, vx, vy, vz);

    // 设置拾取延迟，防止立即被玩家拾取
    itemEntity->setPickupDelay(10);

    // 生成实体
    world.spawnEntity(std::move(itemEntity));
}

void DispenserBlock::playDispenseSound(IWorld& world, const BlockPos& pos)
{
    world.playSound(SoundEvents::BLOCK_DISPENSER_DISPENSE, sound::SoundCategory::Blocks, pos.center(), 1.0f, 1.0f);
}

std::unique_ptr<BlockEntity> DispenserBlock::createBlockEntity(const BlockPos& pos)
{
    return std::make_unique<blockentity::DispenserBlockEntity>(BlockEntityType::Dispenser, pos);
}

} // namespace blocks
} // namespace mc
