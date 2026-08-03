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

#include "TripWireHookBlock.hpp"
#include "../../../../entity/utils/ItemDropHelper.hpp"
#include "../../../../item/core/ItemStack.hpp"
#include "../../../../item/items/block/BlockItemRegistry.hpp"
#include "../../../../util/math/random/Random.hpp"
#include "../../../../util/property/Properties.hpp"
#include "../../../IWorld.hpp"
#include "../../../redstone/RedstoneSystem.hpp"
#include "common/core/Types.hpp"
#include "common/item/items/block/BlockItem.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/util/Direction.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/math/random/IRandom.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/util/property/StateHolder.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

namespace mc {
namespace blocks {

using namespace mc; // Bring BlockStateProperties into scope

namespace {
constexpr f32 P = 1.0f / 16.0f;

const CollisionShape s_hookNorth = CollisionShape::box(5.0f * P, 0.0f, 10.0f * P, 11.0f * P, 10.0f * P, 1.0f);
const CollisionShape s_hookSouth = CollisionShape::box(5.0f * P, 0.0f, 0.0f, 11.0f * P, 10.0f * P, 6.0f * P);
const CollisionShape s_hookWest = CollisionShape::box(10.0f * P, 0.0f, 5.0f * P, 1.0f, 10.0f * P, 11.0f * P);
const CollisionShape s_hookEast = CollisionShape::box(0.0f, 0.0f, 5.0f * P, 6.0f * P, 10.0f * P, 11.0f * P);
} // namespace

TripWireHookBlock::TripWireHookBlock(const BlockProperties& properties)
    : Block(properties)
{

    // 创建状态容器
    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(BlockStateProperties::HORIZONTAL_FACING())
            .add(BlockStateProperties::POWERED())
            .add(BlockStateProperties::ATTACHED())
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
            .with(BlockStateProperties::HORIZONTAL_FACING(), Direction::North)
            .with(BlockStateProperties::POWERED(), false)
            .with(BlockStateProperties::ATTACHED(), false));
}

bool TripWireHookBlock::isPowered(const BlockState& state)
{
    return state.get(BlockStateProperties::POWERED());
}

bool TripWireHookBlock::isConnected(const BlockState& state)
{
    return state.get(BlockStateProperties::ATTACHED());
}

Direction TripWireHookBlock::getFacing(const BlockState& state)
{
    return state.get(BlockStateProperties::HORIZONTAL_FACING());
}

BlockState TripWireHookBlock::withPowered(BlockState state, bool powered)
{
    return state.with(BlockStateProperties::POWERED(), powered);
}

BlockState TripWireHookBlock::withConnected(BlockState state, bool connected)
{
    return state.with(BlockStateProperties::ATTACHED(), connected);
}

void TripWireHookBlock::onBlockAdded(IWorld& world, const BlockPos& pos, const BlockState& state)
{
    // 检查支撑方块
    Direction facing = getFacing(state);
    BlockPos attachPos = pos.offset(Directions::opposite(facing));
    const BlockState* attachState = world.getBlockState(attachPos);

    if (!attachState || !attachState->isSolid()) {
        // 没有支撑，掉落绊线钩物品
        const Block* block = &state.getBlock();
        if (block != nullptr) {
            const BlockItem* blockItem = BlockItemRegistry::instance().getBlockItem(*block);
            if (blockItem != nullptr) {
                ItemStack dropStack(blockItem, 1);
                math::Random rng;
                ItemDropHelper::spawnItemEntity(&world,
                    dropStack,
                    static_cast<f64>(pos.x) + 0.5,
                    static_cast<f64>(pos.y) + 0.5,
                    static_cast<f64>(pos.z) + 0.5,
                    rng);
            }
        }
        world.setBlockState(pos, nullptr, 3);
    }
}

void TripWireHookBlock::neighborChanged(
    IWorld& world, const BlockPos& pos, Block& neighborBlock, const BlockPos& neighborPos, bool isMoving)
{
    MC_UNUSED(neighborBlock);
    MC_UNUSED(neighborPos);
    MC_UNUSED(isMoving);

    const BlockState* state = world.getBlockState(pos);
    if (!state) {
        return;
    }

    Direction facing = getFacing(*state);

    // 检查支撑方块
    BlockPos attachPos = pos.offset(Directions::opposite(facing));
    const BlockState* attachState = world.getBlockState(attachPos);

    if (!attachState || !attachState->isSolid()) {
        // 没有支撑，掉落绊线钩物品
        const Block* block = &state->getBlock();
        if (block != nullptr) {
            const BlockItem* blockItem = BlockItemRegistry::instance().getBlockItem(*block);
            if (blockItem != nullptr) {
                ItemStack dropStack(blockItem, 1);
                math::Random rng;
                ItemDropHelper::spawnItemEntity(&world,
                    dropStack,
                    static_cast<f64>(pos.x) + 0.5,
                    static_cast<f64>(pos.y) + 0.5,
                    static_cast<f64>(pos.z) + 0.5,
                    rng);
            }
        }
        world.setBlockState(pos, nullptr, 3);
    } else {
        // 重新计算状态
        _calculateState(world, pos, facing, *state, true);
    }
}

void TripWireHookBlock::tick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random)
{
    MC_UNUSED(random);
    Direction facing = getFacing(state);
    _calculateState(world, pos, facing, state, false);
}

void TripWireHookBlock::onBlockRemoved(IWorld& world, const BlockPos& pos, const BlockState& state)
{
    // 如果是触发状态，通知相邻方块
    if (isPowered(state)) {
        world::redstone::RedstoneSystem::instance().updateNeighbors(world, pos, *this);
    }
}

BlockState TripWireHookBlock::updatePostPlacement(const BlockState& state,
    Direction facing,
    const BlockState& facingState,
    IWorld& world,
    const BlockPos& currentPos,
    const BlockPos& facingPos)
{
    MC_UNUSED(facingState);
    MC_UNUSED(world);
    MC_UNUSED(currentPos);
    MC_UNUSED(facingPos);

    // 如果支撑方块被移除，返回空气
    Direction hookFacing = getFacing(state);
    if (facing == Directions::opposite(hookFacing)) {
        if (!facingState.isSolid()) {
            // 将被移除
        }
    }

    return state;
}

[[nodiscard]] i32 TripWireHookBlock::getWeakPower(
    const BlockState& state, IWorld& world, const BlockPos& pos, Direction side) const noexcept
{
    MC_UNUSED(world);
    MC_UNUSED(pos);

    // 只在绊线钩背面（朝向的反方向）输出信号
    Direction facing = getFacing(state);
    if (side == Directions::opposite(facing)) {
        return isPowered(state) ? 15 : 0;
    }
    return 0;
}

[[nodiscard]] i32 TripWireHookBlock::getStrongPower(
    const BlockState& state, IWorld& world, const BlockPos& pos, Direction side) const noexcept
{
    MC_UNUSED(world);
    MC_UNUSED(pos);

    // 强信号同样只在背面输出
    Direction facing = getFacing(state);
    if (side == Directions::opposite(facing)) {
        return isPowered(state) ? 15 : 0;
    }
    return 0;
}

[[nodiscard]] const CollisionShape& TripWireHookBlock::getShape(const BlockState& state) const
{
    Direction facing = getFacing(state);
    switch (facing) {
        case Direction::North:
            return s_hookNorth;
        case Direction::South:
            return s_hookSouth;
        case Direction::West:
            return s_hookWest;
        case Direction::East:
        default:
            return s_hookEast;
    }
}

bool TripWireHookBlock::_calculateState(
    IWorld& world, const BlockPos& pos, Direction facing, const BlockState& currentState, bool shouldTriggerOnChange)
{
    // 检测绊线链
    BlockPos otherHookPos;
    bool foundChain = _checkForTripwire(world, pos, facing, otherHookPos);

    bool isTripwirePowered = false;
    bool shouldBreak = false;

    if (foundChain) {
        // 沿朝向检查所有绊线，直到另一端的钩
        Direction checkDir = facing;
        BlockPos checkPos = pos.offset(checkDir);

        for (i32 i = 1; i <= 42; ++i) {
            const BlockState* state = world.getBlockState(checkPos);
            if (!state || state->isAir()) {
                shouldBreak = true;
                break;
            }

            // 检查是否到达另一端的钩
            if (state->is(this)) {
                Direction hookFacing = getFacing(*state);
                if (hookFacing == Directions::opposite(facing)) {
                    // 到达另一端的钩，检查完成
                    break;
                }
                // 钩朝向不对，链断开
                shouldBreak = true;
                break;
            }

            // 检查是否是绊线
            if (state->is(VanillaBlocks::TRIPWIRE)) {
                // 检查绊线是否被触发
                // 只有未被拆除(DISARMED=false)且被触发(POWERED=true)的绊线才触发信号
                bool isDisarmed = false;
                bool isPowered = false;

                if (state->hasProperty(BlockStateProperties::DISARMED())) {
                    isDisarmed = state->get(BlockStateProperties::DISARMED());
                }
                if (state->hasProperty(BlockStateProperties::POWERED())) {
                    isPowered = state->get(BlockStateProperties::POWERED());
                }

                // 未被拆除且被触发的绊线会触发信号
                if (!isDisarmed && isPowered) {
                    isTripwirePowered = true;
                }
            } else {
                // 不是绊线，链断开
                shouldBreak = true;
                break;
            }

            checkPos = checkPos.offset(checkDir);
        }
    }

    // 更新状态
    bool wasPowered = isPowered(currentState);
    bool wasConnected = isConnected(currentState);

    // 只有链完整时才可能触发
    bool shouldPower = foundChain && isTripwirePowered && !shouldBreak;

    if (shouldPower != wasPowered || foundChain != wasConnected) {
        BlockState newState = currentState;
        newState = withPowered(newState, shouldPower);
        newState = withConnected(newState, foundChain);
        world.setBlockState(pos, &newState, 3);

        // 通知相邻方块
        if (shouldTriggerOnChange && shouldPower != wasPowered) {
            world::redstone::RedstoneSystem::instance().updateNeighbors(world, pos, *this);
        }
        return true;
    }

    return false;
}

bool TripWireHookBlock::_checkForTripwire(
    IWorld& world, const BlockPos& pos, Direction facing, BlockPos& outOtherHookPos) const
{
    // 沿朝向检查最多42格
    constexpr i32 MAX_DISTANCE = 42;

    for (i32 i = 1; i <= MAX_DISTANCE; ++i) {
        BlockPos checkPos = pos.offset(facing, i);
        const BlockState* state = world.getBlockState(checkPos);

        if (!state) {
            return false;
        }

        // 检查是否是绊线钩
        if (state->is(this)) {
            Direction hookFacing = getFacing(*state);
            if (hookFacing == Directions::opposite(facing)) {
                // 找到另一端的绊线钩
                outOtherHookPos = checkPos;
                return true;
            }
            return false;
        }

        // 检查是否是绊线
        if (state->is(VanillaBlocks::TRIPWIRE)) {
            // 继续扫描，绊线链有效
        } else {
            // 不是绊线也不是绊线钩，链断开
            return false;
        }
    }

    return false;
}

} // namespace blocks
} // namespace mc
