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

#include "CarpetBlock.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"

namespace mc {
namespace blocks {

CarpetBlock::CarpetBlock(const BlockProperties& properties)
    : Block(properties)
{
    // 地毯高度为1像素（1/16格）
    m_shape = CollisionShape::box(0.0f, 0.0f, 0.0f, 1.0f, 1.0f / 16.0f, 1.0f);
}

const CollisionShape& CarpetBlock::getShape(const BlockState& state) const
{
    MC_UNUSED(state);
    return m_shape;
}

const CollisionShape& CarpetBlock::getCollisionShape(const BlockState& state) const
{
    MC_UNUSED(state);
    // 地毯没有碰撞箱（可以穿过）
    static CollisionShape emptyShape = CollisionShape::empty();
    return emptyShape;
}

bool CarpetBlock::isValidPosition(const BlockState& state, IBlockReader& world, const BlockPos& pos) const
{
    MC_UNUSED(state);
    // 地毯需要放置在非空气方块上方
    const BlockPos belowPos = pos.down();
    const BlockState* belowState = world.getBlockState(belowPos);
    // 检查下方是否为空气方块
    return belowState != nullptr && !belowState->isAir();
}

BlockState CarpetBlock::updatePostPlacement(const BlockState& state,
    Direction facing,
    const BlockState& facingState,
    IWorld& world,
    const BlockPos& currentPos,
    const BlockPos& facingPos)
{
    MC_UNUSED(facingState);
    MC_UNUSED(facingPos);
    // 如果下方方块被移除，则移除地毯
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
