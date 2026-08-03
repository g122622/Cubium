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

#include "RedstoneDiodeBlock.hpp"
#include "../../../IWorld.hpp"
#include "../../../redstone/RedstoneSystem.hpp"
#include "../../../tick/base/TickPriority.hpp"
#include "../../../tick/manager/TickManager.hpp"
#include "common/core/Types.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/util/Direction.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/math/random/IRandom.hpp"
#include "common/util/property/Properties.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/util/property/StateHolder.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/redstone/RedstonePower.hpp"
#include <algorithm>
#include <cstddef>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace mc {
namespace blocks {

RedstoneDiodeBlock::RedstoneDiodeBlock(const std::string& id, const BlockProperties& properties)
    : Block(properties)
    , m_id(id)
{

    // 创建状态容器
    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(BlockStateProperties::HORIZONTAL_FACING())
            .add(BlockStateProperties::POWERED())
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
            .with(BlockStateProperties::POWERED(), false));
}

Direction RedstoneDiodeBlock::getFacing(const BlockState& state)
{
    return state.get(BlockStateProperties::HORIZONTAL_FACING());
}

bool RedstoneDiodeBlock::isPowered(const BlockState& state)
{
    return state.get(BlockStateProperties::POWERED());
}

void RedstoneDiodeBlock::onBlockAdded(IWorld& world, const BlockPos& pos, const BlockState& state)
{
    // 放置时通知邻居更新
    notifyNeighbors(world, pos, state);
}

void RedstoneDiodeBlock::onBlockRemoved(IWorld& world, const BlockPos& pos, const BlockState& state)
{
    // 移除时通知邻居更新
    notifyNeighbors(world, pos, state);
}

void RedstoneDiodeBlock::neighborChanged(
    IWorld& world, const BlockPos& pos, Block& neighborBlock, const BlockPos& neighborPos, bool isMoving)
{
    MC_UNUSED(neighborBlock);
    MC_UNUSED(neighborPos);
    MC_UNUSED(isMoving);

    // 更新状态
    const BlockState* state = world.getBlockState(pos);
    if (state) {
        updateState(world, pos, *state);
    }
}

BlockState RedstoneDiodeBlock::updatePostPlacement(const BlockState& state,
    Direction facing,
    const BlockState& facingState,
    IWorld& world,
    const BlockPos& currentPos,
    const BlockPos& facingPos)
{
    MC_UNUSED(facingState);
    MC_UNUSED(facingPos);

    // 检查是否需要更新
    if (!isLocked(world, currentPos, state)) {
        bool shouldPower = shouldBePowered(world, currentPos, state);
        bool isCurrentlyPowered = isPowered(state);

        if (shouldPower != isCurrentlyPowered) {
            // 调度更新
            world.tickManager().scheduleBlockTick(currentPos, *this, getDelay(state), world::tick::TickPriority::High);
        }
    }

    return state;
}

void RedstoneDiodeBlock::tick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random)
{
    MC_UNUSED(random);
    // 如果被锁定，不更新
    if (isLocked(world, pos, state)) {
        return;
    }

    bool shouldPower = shouldBePowered(world, pos, state);
    bool isCurrentlyPowered = isPowered(state);

    if (shouldPower != isCurrentlyPowered) {
        // 改变状态
        BlockState newState = state.with(BlockStateProperties::POWERED(), shouldPower);
        world.setBlockState(pos, &newState, 2);

        // 通知输出端相邻方块更新
        Direction facing = getFacing(state);
        BlockPos outputPos = pos.offset(facing);
        const BlockState* outputState = world.getBlockState(outputPos);
        if (outputState && !outputState->isAir()) {
            Block& outputBlock = outputState->getBlockMutable();
            outputBlock.neighborChanged(world, outputPos, *this, pos, false);
        }
    }
}

i32 RedstoneDiodeBlock::getWeakPower(
    const BlockState& state, IWorld& world, const BlockPos& pos, Direction side) const noexcept
{
    MC_UNUSED(world);
    MC_UNUSED(pos);

    // 只有在输出方向且已充能时才输出信号
    if (!isPowered(state)) {
        return 0;
    }

    Direction facing = getFacing(state);
    if (side == facing) {
        return calculateOutputSignal(world, pos, state);
    }

    return 0;
}

i32 RedstoneDiodeBlock::getStrongPower(
    const BlockState& state, IWorld& world, const BlockPos& pos, Direction side) const noexcept
{
    // 二极管输出的是强信号，可以充能方块
    return getWeakPower(state, world, pos, side);
}

const CollisionShape& RedstoneDiodeBlock::getShape(const BlockState& state) const
{
    MC_UNUSED(state);
    static const CollisionShape diodeShape = CollisionShape::fromPixelBox(0.0f, 0.0f, 0.0f, 16.0f, 2.0f, 16.0f);
    return diodeShape;
}

i32 RedstoneDiodeBlock::getInputSignal(IWorld& world, const BlockPos& pos, const BlockState& state) const
{
    Direction facing = getFacing(state);
    Direction inputDir = Directions::opposite(facing);
    BlockPos inputPos = pos.offset(inputDir);

    const BlockState* inputState = world.getBlockState(inputPos);
    if (!inputState || inputState->isAir()) {
        return 0;
    }

    const Block& inputBlock = inputState->getBlock();

    // 获取强信号
    i32 power = inputBlock.getStrongPower(*inputState, world, inputPos, facing);

    // 如果强信号为0，检查红石线
    if (power < 15 && inputBlock.canProvidePower(*inputState)) {
        // 可能是红石线
        i32 weakPower = inputBlock.getWeakPower(*inputState, world, inputPos, facing);
        power = std::max(power, weakPower);
    }

    return power;
}

i32 RedstoneDiodeBlock::getPowerOnSides(IWorld& world, const BlockPos& pos, const BlockState& state) const
{
    Direction facing = getFacing(state);
    i32 maxPower = 0;

    // 检查两个侧面（不包括前后）
    for (Direction side : Directions::horizontal()) {
        if (side == facing || side == Directions::opposite(facing)) {
            continue;
        }

        BlockPos sidePos = pos.offset(side);
        const BlockState* sideState = world.getBlockState(sidePos);

        if (sideState && !sideState->isAir()) {
            const Block& sideBlock = sideState->getBlock();
            Direction oppositeSide = Directions::opposite(side);

            // 中继器只能被其他二极管的侧面输出锁定
            // 关键：侧面二极管的输出端必须朝向当前中继器
            i32 power = 0;

            // 检查是否是二极管（中继器或比较器）
            if (isDiode(*sideState)) {
                // 对于二极管，只有当其输出端朝向当前中继器时才计入锁定信号
                // 即：侧面二极管的朝向必须与side相同（朝向我们）
                Direction sideFacing = getFacing(*sideState);
                if (sideFacing == oppositeSide && isPowered(*sideState)) {
                    power = sideBlock.getWeakPower(*sideState, world, sidePos, oppositeSide);
                }
            }
            // 注意：红石线和其他信号源不能锁定中继器

            maxPower = std::max(maxPower, power);
        }
    }

    return maxPower;
}

bool RedstoneDiodeBlock::isDiode(const BlockState& state) const
{
    const Block& block = state.getBlock();
    // 检查是否是中继器或比较器
    return dynamic_cast<const RedstoneDiodeBlock*>(&block) != nullptr;
}

bool RedstoneDiodeBlock::isLocked(IWorld& world, const BlockPos& pos, const BlockState& state) const
{
    return getPowerOnSides(world, pos, state) > 0;
}

i32 RedstoneDiodeBlock::calculateOutputSignal(IWorld& world, const BlockPos& pos, const BlockState& state) const
{
    MC_UNUSED(world);
    MC_UNUSED(pos);
    // 默认输出15，子类可以重写
    return isPowered(state) ? world::redstone::RedstonePower::MAX_POWER : 0;
}

void RedstoneDiodeBlock::updateState(IWorld& world, const BlockPos& pos, const BlockState& state)
{
    // 如果被锁定，不更新
    if (isLocked(world, pos, state)) {
        return;
    }

    bool shouldPower = shouldBePowered(world, pos, state);
    bool isCurrentlyPowered = isPowered(state);

    if (shouldPower != isCurrentlyPowered) {
        // 确定优先级
        world::tick::TickPriority priority = world::tick::TickPriority::High;

        if (isFacingTowardsRepeater(world, pos, state)) {
            priority = world::tick::TickPriority::ExtremelyHigh;
        } else if (isCurrentlyPowered) {
            priority = world::tick::TickPriority::VeryHigh;
        }

        // 调度更新
        world.tickManager().scheduleBlockTick(pos, *this, getDelay(state), priority);
    }
}

bool RedstoneDiodeBlock::isFacingTowardsRepeater(IWorld& world, const BlockPos& pos, const BlockState& state) const
{
    Direction facing = getFacing(state);
    BlockPos outputPos = pos.offset(facing);

    const BlockState* outputState = world.getBlockState(outputPos);
    if (!outputState) {
        return false;
    }

    // 检查输出端是否是另一个二极管
    if (!isDiode(*outputState)) {
        return false;
    }

    // 检查二极管是否不是背向自己
    // 即：输出端的二极管朝向不能是自己的反方向
    Direction outputFacing = getFacing(*outputState);
    return outputFacing != Directions::opposite(facing);
}

void RedstoneDiodeBlock::notifyNeighbors(IWorld& world, const BlockPos& pos, const BlockState& state)
{
    // 通知输入端周围的方块更新
    Direction facing = getFacing(state);
    Direction inputDir = Directions::opposite(facing);
    BlockPos inputPos = pos.offset(inputDir);

    // 先通知输入端的方块
    const BlockState* inputState = world.getBlockState(inputPos);
    if (inputState && !inputState->isAir()) {
        Block& inputBlock = inputState->getBlockMutable();
        inputBlock.neighborChanged(world, inputPos, *this, pos, false);
    }

    // 然后通知输入端周围的其他邻居（除了二极管本身）
    for (Direction dir : Directions::all()) {
        if (dir == facing) continue; // 跳过输出方向

        BlockPos neighborPos = inputPos.offset(dir);
        const BlockState* neighborState = world.getBlockState(neighborPos);
        if (neighborState && !neighborState->isAir()) {
            Block& neighborBlock = neighborState->getBlockMutable();
            neighborBlock.neighborChanged(world, neighborPos, *this, inputPos, false);
        }
    }
}

} // namespace blocks
} // namespace mc
