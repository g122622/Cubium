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

#include "GlowLichenBlock.hpp"
#include "common/item/context/BlockItemUseContext.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/block/WaterLoggableHelpers.hpp"

namespace mc {
namespace blocks {

GlowLichenBlock::GlowLichenBlock(const BlockProperties& properties)
    : MultifaceBlock(properties)
{
    buildMultifaceStateContainer();

    // 预计算所有64种形状组合（2^6 = 64，六个面方向各布尔值）
    // 每个面方向是一个1像素厚的薄板：
    //   NORTH: (0,0,0)-(16,16,1)  即 z=0 到 z=1 像素
    //   SOUTH: (0,0,15)-(16,16,16) 即 z=15 到 z=16 像素
    //   EAST:  (15,0,0)-(16,16,16) 即 x=15 到 x=16 像素
    //   WEST:  (0,0,0)-(1,16,16)   即 x=0 到 x=1 像素
    //   UP:    (0,15,0)-(16,16,16) 即 y=15 到 y=16 像素
    //   DOWN:  (0,0,0)-(16,1,16)   即 y=0 到 y=1 像素
    // 每个面方向的形状
    const CollisionShape northShape = CollisionShape::fromPixelBox(0, 0, 0, 16, 16, 1);
    const CollisionShape southShape = CollisionShape::fromPixelBox(0, 0, 15, 16, 16, 16);
    const CollisionShape eastShape = CollisionShape::fromPixelBox(15, 0, 0, 16, 16, 16);
    const CollisionShape westShape = CollisionShape::fromPixelBox(0, 0, 0, 1, 16, 16);
    const CollisionShape upShape = CollisionShape::fromPixelBox(0, 15, 0, 16, 16, 16);
    const CollisionShape downShape = CollisionShape::fromPixelBox(0, 0, 0, 16, 1, 16);

    // 遍历所有64种面激活组合，预计算形状
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

                            // 如果没有面激活，使用完整方块形状（与 MC 原版一致）
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

void GlowLichenBlock::fillStateContainer(StateContainer<Block, BlockState>& container)
{
    MC_UNUSED(container);
}

BlockState GlowLichenBlock::getStateForPlacement(BlockItemUseContext& context)
{
    const Direction clickedFace = context.getClickedFace();
    const BlockState* current = context.getWorld().getBlockState(context.placementPos());
    // 委托 MultifaceBlock.getStateForPlacement(state, reader, pos, direction)（对齐 MC）。
    const BlockState* placed =
        MultifaceBlock::getStateForPlacement(current, context.getWorld(), context.placementPos(), clickedFace);
    if (placed == nullptr) {
        return defaultState();
    }
    return *placed;
}

BlockState GlowLichenBlock::updatePostPlacement(const BlockState& state,
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

const CollisionShape& GlowLichenBlock::getShape(const BlockState& state) const
{
    size_t index = _getShapeIndex(state.get(BlockStateProperties::DOWN()),
        state.get(BlockStateProperties::UP()),
        state.get(BlockStateProperties::NORTH()),
        state.get(BlockStateProperties::SOUTH()),
        state.get(BlockStateProperties::EAST()),
        state.get(BlockStateProperties::WEST()));
    return m_shapes[index];
}

const fluid::FluidState* GlowLichenBlock::getFluidState(const BlockState& state) const
{
    return waterloggable::getWaterFluidState(state);
}

u8 GlowLichenBlock::getLightLevel(const BlockState& state, IWorld* world, const BlockPos* pos) const
{
    MC_UNUSED(world);
    MC_UNUSED(pos);

    // 如果任意面激活，返回光照等级7
    if (state.get(BlockStateProperties::NORTH()) || state.get(BlockStateProperties::SOUTH()) ||
        state.get(BlockStateProperties::EAST()) || state.get(BlockStateProperties::WEST()) ||
        state.get(BlockStateProperties::UP()) || state.get(BlockStateProperties::DOWN())) {
        return 7;
    }
    return 0;
}

size_t GlowLichenBlock::_getShapeIndex(bool down, bool up, bool north, bool south, bool east, bool west)
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
