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

#include "FlowerPotBlock.hpp"
#include "../../../IWorld.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"

namespace mc {
namespace blocks {

FlowerPotBlock::FlowerPotBlock(const BlockProperties& properties, u32 content)
    : Block(properties)
    , m_content(content)
{
    // 花盆形状：底部圆形 + 顶部边缘
    // 简化为单个盒子
    m_shape = CollisionShape::box(5.0f / 16.0f, 0.0f, 5.0f / 16.0f, 11.0f / 16.0f, 6.0f / 16.0f, 11.0f / 16.0f);
    m_collisionShape =
        CollisionShape::box(5.0f / 16.0f, 0.0f, 5.0f / 16.0f, 11.0f / 16.0f, 6.0f / 16.0f, 11.0f / 16.0f);
}

const CollisionShape& FlowerPotBlock::getShape(const BlockState& state) const
{
    MC_UNUSED(state);
    return m_shape;
}

const CollisionShape& FlowerPotBlock::getCollisionShape(const BlockState& state) const
{
    MC_UNUSED(state);
    return m_collisionShape;
}

bool FlowerPotBlock::isValidPosition(const BlockState& state, IBlockReader& world, const BlockPos& pos) const
{
    MC_UNUSED(state);
    MC_UNUSED(world);
    MC_UNUSED(pos);
    // 花盆可以放置在任何完整方块上，不需要特别检查
    return true;
}

BlockState FlowerPotBlock::updatePostPlacement(const BlockState& state,
    Direction facing,
    const BlockState& facingState,
    IWorld& world,
    const BlockPos& currentPos,
    const BlockPos& facingPos)
{
    MC_UNUSED(facingState);
    MC_UNUSED(facingPos);
    // 如果下方方块被移除，则移除花盆
    if (facing == Direction::Down) {
        const BlockPos belowPos = currentPos.down();
        const BlockState* belowState = world.getBlockState(belowPos);
        if (belowState == nullptr || belowState->isAir()) {
            return VanillaBlocks::AIR->defaultState();
        }
    }
    return state;
}

} // namespace blocks
} // namespace mc
