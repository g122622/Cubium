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

#include "ResinClumpBlock.hpp"
#include "common/core/Types.hpp"
#include "common/item/context/BlockItemUseContext.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/util/Direction.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/property/Properties.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/WaterLoggableHelpers.hpp"
#include "common/world/block/blocks/MultifaceBlock.hpp"
#include "common/world/fluid/Fluid.hpp"
#include <cstddef>

namespace mc {
namespace blocks {

ResinClumpBlock::ResinClumpBlock(const BlockProperties& properties)
    : MultifaceBlock(properties)
{
    buildMultifaceStateContainer();

    // 预计算所有 64 种形状组合（2^6 = 64，六个面方向各布尔值）
    // 每个面方向是一个 1 像素厚的薄板：
    //   NORTH: (0,0,0)-(16,16,1)   SOUTH: (0,0,15)-(16,16,16)
    //   EAST:  (15,0,0)-(16,16,16) WEST:  (0,0,0)-(1,16,16)
    //   UP:    (0,15,0)-(16,16,16) DOWN:  (0,0,0)-(16,1,16)
    const CollisionShape northShape = CollisionShape::fromPixelBox(0, 0, 0, 16, 16, 1);
    const CollisionShape southShape = CollisionShape::fromPixelBox(0, 0, 15, 16, 16, 16);
    const CollisionShape eastShape = CollisionShape::fromPixelBox(15, 0, 0, 16, 16, 16);
    const CollisionShape westShape = CollisionShape::fromPixelBox(0, 0, 0, 1, 16, 16);
    const CollisionShape upShape = CollisionShape::fromPixelBox(0, 15, 0, 16, 16, 16);
    const CollisionShape downShape = CollisionShape::fromPixelBox(0, 0, 0, 16, 1, 16);

    for (int down = 0; down <= 1; ++down) {
        for (int up = 0; up <= 1; ++up) {
            for (int north = 0; north <= 1; ++north) {
                for (int south = 0; south <= 1; ++south) {
                    for (int east = 0; east <= 1; ++east) {
                        for (int west = 0; west <= 1; ++west) {
                            size_t idx =
                                _getShapeIndex(down != 0, up != 0, north != 0, south != 0, east != 0, west != 0);

                            CollisionShape shape = CollisionShape::empty();

                            if (north) shape = CollisionShape::combine(shape, northShape);
                            if (south) shape = CollisionShape::combine(shape, southShape);
                            if (east) shape = CollisionShape::combine(shape, eastShape);
                            if (west) shape = CollisionShape::combine(shape, westShape);
                            if (up) shape = CollisionShape::combine(shape, upShape);
                            if (down) shape = CollisionShape::combine(shape, downShape);

                            // 没有面激活时使用完整方块形状（与 MC 原版一致）
                            if (!north && !south && !east && !west && !up && !down) {
                                shape = CollisionShape::fullBlock();
                            }

                            m_shapes[idx] = shape;
                        }
                    }
                }
            }
        }
    }
}

void ResinClumpBlock::fillStateContainer(StateContainer<Block, BlockState>& container)
{
    MC_UNUSED(container);
    // 状态容器由 buildMultifaceStateContainer() 在构造函数中创建
}

BlockState ResinClumpBlock::getStateForPlacement(BlockItemUseContext& context)
{
    // 对齐 vanilla MultifaceBlock.getStateForPlacement(BlockPlaceContext)（:180-189）：
    //   vanilla 用 getNearestLookingDirections()（玩家最近视线方向数组，首个通常为
    //   clickedFace.getOpposite()）逐个尝试 getStateForPlacement(state, level, pos, dir)。
    //   此处简化为 opposite(clickedFace)：点击方块表面放置时，要设的面方向是被点击面的
    //   反方向（如点击顶面 Up → 薄板朝下 Down 贴在支撑方块顶面）。
    //
    //   旧实现误传 clickedFace：导致 MultifaceBlock.getStateForPlacement 检查
    //   neighbor = pos.offset(clickedFace)（点击面前方，通常为空气）→ canAttachTo 失败 → 返回
    //   nullptr → 回落 defaultState()（无任何面，放置后立刻被 updatePostPlacement 销毁）。
    //   修复后传 opposite(clickedFace)：neighbor = pos.offset(opposite) = 支撑方块 →
    //   canAttachTo 检查支撑方块该面 full → true → 设对应面 = true。
    //
    // TODO: 未对齐 getNearestLookingDirections() 多方向优先级：玩家斜视时 vanilla 优先选
    //   最近视线方向的面，此处仅用 opposite(clickedFace) 单方向。待 getNearestLookingDirections
    //   链路补全后对齐。
    const Direction placementFace = Directions::opposite(context.getClickedFace());
    const BlockState* current = context.getWorld().getBlockState(context.placementPos());
    const BlockState* placed =
        MultifaceBlock::getStateForPlacement(current, context.getWorld(), context.placementPos(), placementFace);
    if (placed == nullptr) {
        return defaultState();
    }
    return *placed;
}

BlockState ResinClumpBlock::updatePostPlacement(const BlockState& state,
    Direction facing,
    const BlockState& facingState,
    IWorld& world,
    const BlockPos& currentPos,
    const BlockPos& facingPos)
{
    MC_UNUSED(facing);
    MC_UNUSED(facingState);
    MC_UNUSED(facingPos);

    if (state.get(BlockStateProperties::WATERLOGGED())) {
        waterloggable::scheduleWaterTick(world, currentPos);
    }

    return state;
}

const CollisionShape& ResinClumpBlock::getShape(const BlockState& state) const
{
    size_t index = _getShapeIndex(state.get(BlockStateProperties::DOWN()),
        state.get(BlockStateProperties::UP()),
        state.get(BlockStateProperties::NORTH()),
        state.get(BlockStateProperties::SOUTH()),
        state.get(BlockStateProperties::EAST()),
        state.get(BlockStateProperties::WEST()));
    return m_shapes[index];
}

const fluid::FluidState* ResinClumpBlock::getFluidState(const BlockState& state) const
{
    return waterloggable::getWaterFluidState(state);
}

size_t ResinClumpBlock::_getShapeIndex(bool down, bool up, bool north, bool south, bool east, bool west)
{
    size_t index = 0;
    if (down) index |= 1;
    if (up) index |= 2;
    if (north) index |= 4;
    if (south) index |= 8;
    if (east) index |= 16;
    if (west) index |= 32;
    return index;
}

} // namespace blocks
} // namespace mc
