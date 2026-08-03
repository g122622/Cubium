/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so subject to the following conditions:
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
 */

#include "GrowingPlantBodyBlock.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/util/Direction.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/blocks/growing_plant/GrowingPlantBlock.hpp"
#include "common/world/chunk/data/IChunk.hpp"

namespace mc {
namespace blocks {

GrowingPlantBodyBlock::GrowingPlantBodyBlock(
    const BlockProperties& properties, Direction growthDirection, const CollisionShape& shape, bool scheduleFluidTicks)
    : GrowingPlantBlock(properties, growthDirection, shape, scheduleFluidTicks)
{}

BlockState GrowingPlantBodyBlock::updatePostPlacement(const BlockState& state,
    Direction facing,
    const BlockState& facingState,
    IWorld& world,
    const BlockPos& currentPos,
    const BlockPos& facingPos)
{
    MC_UNUSED(facingState);

    // 当生长方向上的方块更新时
    if (facing == m_growthDirection) {
        // 检查生长方向上是否还有头部方块
        const BlockPos headPos(currentPos.x + Directions::xOffset(m_growthDirection),
            currentPos.y + Directions::yOffset(m_growthDirection),
            currentPos.z + Directions::zOffset(m_growthDirection));
        const BlockState* headState = world.getBlockState(headPos);

        const Block* headBlock = getHeadBlock();

        // 如果头部方块被移除，身体方块变成新的头部
        if (headBlock && headState && !headState->is(headBlock)) {
            BlockState newHeadState = updateHeadAfterConvertedFromBody(state);
            return newHeadState;
        }
    }

    // 调用基类处理支撑方向更新
    return GrowingPlantBlock::updatePostPlacement(state, facing, facingState, world, currentPos, facingPos);
}

BlockState GrowingPlantBodyBlock::updateHeadAfterConvertedFromBody(const BlockState& bodyState) const
{
    MC_UNUSED(bodyState);
    // 默认：返回头部方块的默认状态
    const Block* headBlock = getHeadBlock();
    return headBlock ? headBlock->defaultState() : defaultState();
}

} // namespace blocks
} // namespace mc
