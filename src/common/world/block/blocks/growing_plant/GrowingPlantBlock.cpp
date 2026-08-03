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
 */

#include "GrowingPlantBlock.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/util/Direction.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockRegistry.hpp"

namespace mc {
namespace blocks {

GrowingPlantBlock::GrowingPlantBlock(
    const BlockProperties& properties, Direction growthDirection, const CollisionShape& shape, bool scheduleFluidTicks)
    : Block(properties)
    , m_growthDirection(growthDirection)
    , m_shape(shape)
    , m_scheduleFluidTicks(scheduleFluidTicks)
{}

bool GrowingPlantBlock::isValidPosition(const BlockState& state, IBlockReader& world, const BlockPos& pos) const
{
    MC_UNUSED(state);

    // 检查生长方向的反方向是否有支撑
    // 对于向下生长的植物（如洞穴藤蔓），需要上方有同类或坚固面
    // 对于向上生长的植物（如海带），需要下方有同类或坚固面
    const Direction supportDirection = Directions::opposite(m_growthDirection);
    const BlockPos supportPos(pos.x + Directions::xOffset(supportDirection),
        pos.y + Directions::yOffset(supportDirection),
        pos.z + Directions::zOffset(supportDirection));

    const BlockState* supportState = world.getBlockState(supportPos);
    if (!supportState) {
        return false;
    }

    // 支撑方块是同类头部或身体
    if (supportState->is(getHeadBlock()) || supportState->is(getBodyBlock())) {
        return true;
    }

    // 检查支撑面是否坚固
    return supportState->isSolidSide(world, supportPos, m_growthDirection);
}

BlockState GrowingPlantBlock::updatePostPlacement(const BlockState& state,
    Direction facing,
    const BlockState& facingState,
    IWorld& world,
    const BlockPos& currentPos,
    const BlockPos& facingPos)
{
    MC_UNUSED(facingState);
    MC_UNUSED(facingPos);

    // 当支撑方向方块更新时，重新检查位置有效性
    const Direction supportDirection = Directions::opposite(m_growthDirection);
    if (facing == supportDirection) {
        IBlockReader& blockReader = static_cast<IBlockReader&>(world);
        if (!isValidPosition(state, blockReader, currentPos)) {
            // 支撑丢失，调用破坏逻辑
            // MC 原版：destroyBlock 会掉落物品，这里先返回空气
            if (auto* airState = BlockRegistry::instance().airState()) {
                return *airState;
            }
        }
    }

    return state;
}

const CollisionShape& GrowingPlantBlock::getShape(const BlockState& state) const
{
    MC_UNUSED(state);
    return m_shape;
}

} // namespace blocks
} // namespace mc
