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

#include "ConcretePowderBlock.hpp"

#include "common/item/context/BlockItemUseContext.hpp"
#include "common/util/Direction.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/blocks/FallingBlock.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/fluid/FluidTags.hpp"

namespace mc {
namespace blocks {

ConcretePowderBlock::ConcretePowderBlock(const BlockProperties& properties, const Block* concrete)
    : FallingBlock(properties)
    , m_concrete(concrete)
{}

BlockState ConcretePowderBlock::getStateForPlacement(BlockItemUseContext& context)
{
    // 放置时检查：如果位置已接触水，直接固化为混凝土
    const BlockState* currentState = context.getWorld().getBlockState(context.placementPos());
    if (currentState != nullptr && shouldSolidify(context.getWorld(), context.placementPos(), *currentState)) {
        return m_concrete->defaultState();
    }
    return FallingBlock::getStateForPlacement(context);
}

BlockState ConcretePowderBlock::updatePostPlacement(const BlockState& state,
    Direction facing,
    const BlockState& facingState,
    IWorld& world,
    const BlockPos& currentPos,
    const BlockPos& facingPos)
{
    // 邻居更新时检查：如果接触水，固化为混凝土
    if (touchesLiquid(world, currentPos)) {
        return m_concrete->defaultState();
    }
    return FallingBlock::updatePostPlacement(state, facing, facingState, world, currentPos, facingPos);
}

void ConcretePowderBlock::onEndFalling(IWorld& world,
    const BlockPos& pos,
    const BlockState& /*fallingState*/,
    const BlockState& hitState,
    entity::FallingBlockEntity& /*entity*/)
{
    // 落地时检查：如果落地点接触水，固化为混凝土
    if (shouldSolidify(world, pos, hitState)) {
        world.setBlockState(pos, &m_concrete->defaultState(), 3);
    }
}

bool ConcretePowderBlock::shouldSolidify(IWorld& world, const BlockPos& pos, const BlockState& state)
{
    return canSolidify(state) || touchesLiquid(world, pos);
}

bool ConcretePowderBlock::canSolidify(const BlockState& state)
{
    // 检查方块的流体状态是否为水（水源或流动水均可导致固化）
    const fluid::FluidState* fluidState = state.getFluidState();
    return fluidState != nullptr && !fluidState->isEmpty() && fluidState->getFluid().isIn(fluid::FluidTags::WATER());
}

bool ConcretePowderBlock::touchesLiquid(IWorld& world, const BlockPos& pos)
{
    // 遍历六个方向检查是否有水流体
    // 对齐 MC 1.21.11 ConcretePowderBlock.touchesLiquid()
    static constexpr Direction directions[] = {
        Direction::Down, Direction::Up, Direction::North, Direction::South, Direction::West, Direction::East};

    for (Direction dir : directions) {
        BlockPos neighborPos = pos.offset(dir);
        const BlockState* neighborState = world.getBlockState(neighborPos);
        if (neighborState != nullptr && canSolidify(*neighborState)) {
            return true;
        }
    }
    return false;
}

} // namespace blocks
} // namespace mc
