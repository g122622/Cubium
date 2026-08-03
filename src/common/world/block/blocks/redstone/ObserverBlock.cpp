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

#include "ObserverBlock.hpp"
#include "common/core/Types.hpp"
#include "common/item/context/BlockItemUseContext.hpp"
#include "common/util/Direction.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/math/random/IRandom.hpp"
#include "common/util/property/Properties.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/util/property/StateHolder.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/redstone/RedstonePower.hpp"
#include "common/world/redstone/RedstoneSystem.hpp"
#include "common/world/tick/base/TickPriority.hpp"
#include "common/world/tick/manager/TickManager.hpp"
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

namespace mc {
namespace blocks {

ObserverBlock::ObserverBlock(const BlockProperties& properties)
    : Block(properties)
{
    // 创建状态容器
    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(BlockStateProperties::FACING())
            .add(BlockStateProperties::POWERED())
            .create([](const Block& block,
                        std::vector<size_t> values,
                        const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
                        const std::vector<BlockState*>* allStates,
                        u32 id) {
                return std::make_unique<BlockState>(block, std::move(values), propertyLayouts, allStates, id);
            });
    createBlockState(std::move(container));

    // 设置默认状态 - 默认朝向是 South
    setDefaultState(defaultState()
            .with(BlockStateProperties::FACING(), Direction::South)
            .with(BlockStateProperties::POWERED(), false));
}

BlockState ObserverBlock::getStateForPlacement(BlockItemUseContext& context)
{
    // 侦测器朝向玩家面向方向的反方向
    // context.horizontalDirection() 是玩家面向的方向
    // 侦测器的输出方向应该是玩家面向方向的反方向
    Direction facing = Directions::opposite(context.horizontalDirection());
    return defaultState().with(BlockStateProperties::FACING(), facing);
}

Direction ObserverBlock::getFacing(const BlockState& state)
{
    return state.get(BlockStateProperties::FACING());
}

bool ObserverBlock::isPowered(const BlockState& state)
{
    return state.get(BlockStateProperties::POWERED());
}

BlockState ObserverBlock::withPowered(BlockState state, bool powered)
{
    return state.with(BlockStateProperties::POWERED(), powered);
}

void ObserverBlock::onBlockAdded(IWorld& world, const BlockPos& pos, const BlockState& state)
{
    // 放置时如果状态是激活的，需要先设置为非激活状态
    // 这通常不应该发生，因为默认状态是非激活的
    if (isPowered(state)) {
        // 如果已经有tick调度，需要先取消
        BlockState unpoweredState = withPowered(state, false);
        world.setBlockState(pos, &unpoweredState, 18);
        _updateNeighborsInFront(world, pos, unpoweredState);
    }
}

void ObserverBlock::onBlockRemoved(IWorld& world, const BlockPos& pos, const BlockState& state)
{
    // 移除时如果正在输出且有tick调度，需要通知邻居
    if (isPowered(state)) {
        _updateNeighborsInFront(world, pos, state.with(BlockStateProperties::POWERED(), false));
    }
}

void ObserverBlock::neighborChanged(
    IWorld& world, const BlockPos& pos, Block& neighborBlock, const BlockPos& neighborPos, bool isMoving)
{
    MC_UNUSED(neighborBlock);
    MC_UNUSED(isMoving);

    const BlockState* state = world.getBlockState(pos);
    if (!state) {
        return;
    }

    // 检查变化是否来自侦测面（背面）
    Direction facing = getFacing(*state);
    Direction observeDir = Directions::opposite(facing);
    BlockPos observePos = pos.offset(observeDir);

    // 只有侦测面的变化才触发
    if (neighborPos == observePos) {
        // 如果当前未激活，调度1 tick延迟后激活
        if (!isPowered(*state)) {
            world.tickManager().scheduleBlockTick(pos, *this, DETECT_DELAY, world::tick::TickPriority::High);
        }
    }
}

BlockState ObserverBlock::updatePostPlacement(const BlockState& state,
    Direction facing,
    const BlockState& facingState,
    IWorld& world,
    const BlockPos& currentPos,
    const BlockPos& facingPos)
{
    MC_UNUSED(facingState);
    MC_UNUSED(facingPos);

    // 当 updatePostPlacement 被调用时，检查更新是否来自观察面
    // 注意：facing 参数是"邻居相对于当前方块的方向"
    // 所以如果观察面被更新，facing 应该是观察方向（输出的反方向）
    Direction outputDir = getFacing(state);
    Direction observeDir = Directions::opposite(outputDir);

    // 当观察面有方块变化时触发检测
    if (facing == observeDir && !isPowered(state)) {
        // 调度 2 tick 延迟后激活
        world.tickManager().scheduleBlockTick(currentPos, *this, DETECT_DELAY, world::tick::TickPriority::High);
    }

    return state;
}

void ObserverBlock::tick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random)
{
    MC_UNUSED(random);
    // 逻辑：
    // 1. 如果未激活 -> 激活并调度 2 tick 后熄灭
    // 2. 如果已激活 -> 熄灭
    if (isPowered(state)) {
        // 脉冲结束，停止输出
        BlockState newState = withPowered(state, false);
        world.setBlockState(pos, &newState, 2);
    } else {
        // 激活并调度熄灭
        BlockState newState = withPowered(state, true);
        world.setBlockState(pos, &newState, 2);
        world.tickManager().scheduleBlockTick(pos, *this, PULSE_DURATION, world::tick::TickPriority::High);
    }

    // 无论激活还是熄灭，都需要通知前方的邻居更新
    _updateNeighborsInFront(world, pos, isPowered(state) ? state : withPowered(state, true));
}

void ObserverBlock::_updateNeighborsInFront(IWorld& world, const BlockPos& pos, const BlockState& state)
{
    // 通知观察面背面的方块（即侦测器指向的反方向）更新
    Direction facing = getFacing(state);
    Direction observeDir = Directions::opposite(facing);
    BlockPos observePos = pos.offset(observeDir);

    // 先通知观察面的方块
    const BlockState* observeState = world.getBlockState(observePos);
    if (observeState && !observeState->isAir()) {
        Block& observeBlock = observeState->getBlockMutable();
        observeBlock.neighborChanged(world, observePos, *this, pos, false);
    }

    // 然后通知观察面周围的其他邻居（除了侦测器本身）
    for (Direction dir : Directions::all()) {
        if (dir == facing) continue; // 跳过侦测器输出方向

        BlockPos neighborPos = observePos.offset(dir);
        const BlockState* neighborState = world.getBlockState(neighborPos);
        if (neighborState && !neighborState->isAir()) {
            Block& neighborBlock = neighborState->getBlockMutable();
            neighborBlock.neighborChanged(world, neighborPos, *this, observePos, false);
        }
    }
}

i32 ObserverBlock::getWeakPower(
    const BlockState& state, IWorld& world, const BlockPos& pos, Direction side) const noexcept
{
    MC_UNUSED(world);
    MC_UNUSED(pos);

    // 只在输出方向输出信号
    if (side != getFacing(state)) {
        return 0;
    }

    return isPowered(state) ? world::redstone::RedstonePower::MAX_POWER : 0;
}

i32 ObserverBlock::getStrongPower(
    const BlockState& state, IWorld& world, const BlockPos& pos, Direction side) const noexcept
{
    // 侦测器只输出弱信号
    return getWeakPower(state, world, pos, side);
}

} // namespace blocks
} // namespace mc
