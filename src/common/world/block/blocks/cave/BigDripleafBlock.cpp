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

#include "BigDripleafBlock.hpp"
#include "common/item/context/BlockItemUseContext.hpp"
#include "common/util/Direction.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/block/WaterLoggableHelpers.hpp"

namespace mc {
namespace blocks {

BigDripleafBlock::BigDripleafBlock(const BlockProperties& properties)
    : Block(properties)
    , m_fullShape(CollisionShape::fromPixelBox(0, 0, 0, 16, 16, 16))
{
    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(BlockStateProperties::HORIZONTAL_FACING())
            .add(BlockStateProperties::TILT())
            .add(BlockStateProperties::WATERLOGGED())
            .create([](const Block& block,
                        std::vector<size_t> values,
                        const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
                        const std::vector<BlockState*>* allStates,
                        u32 id) {
                return std::make_unique<BlockState>(block, std::move(values), propertyLayouts, allStates, id);
            });
    createBlockState(std::move(container));

    setDefaultState(defaultState()
            .with(BlockStateProperties::HORIZONTAL_FACING(), Direction::North)
            .with(BlockStateProperties::TILT(), BlockStateProperties::Tilt::None)
            .with(BlockStateProperties::WATERLOGGED(), false));
}

void BigDripleafBlock::fillStateContainer(StateContainer<Block, BlockState>& container)
{
    MC_UNUSED(container);
}

BlockState BigDripleafBlock::getStateForPlacement(BlockItemUseContext& context)
{
    Direction horizontalFacing = context.horizontalDirection();
    BlockState state = defaultState().with(BlockStateProperties::HORIZONTAL_FACING(), horizontalFacing);

    if (waterloggable::shouldWaterlogAt(context.getWorld(), context.placementPos())) {
        state = state.with(BlockStateProperties::WATERLOGGED(), true);
    }

    return state;
}

BlockState BigDripleafBlock::updatePostPlacement(const BlockState& state,
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

const CollisionShape& BigDripleafBlock::getShape(const BlockState& state) const
{
    MC_UNUSED(state);
    return m_fullShape;
}

const CollisionShape& BigDripleafBlock::getCollisionShape(const BlockState& state) const
{
    BlockStateProperties::Tilt tilt = state.get(BlockStateProperties::TILT());
    // 完全倾斜时玩家会掉落
    if (tilt == BlockStateProperties::Tilt::Full) {
        return VoxelShapes::empty();
    }
    return m_fullShape;
}

const fluid::FluidState* BigDripleafBlock::getFluidState(const BlockState& state) const
{
    return waterloggable::getWaterFluidState(state);
}

void BigDripleafBlock::tick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random)
{
    MC_UNUSED(world);
    MC_UNUSED(pos);
    MC_UNUSED(state);
    MC_UNUSED(random);
    // TODO: 实现倾斜进度逻辑
}

void BigDripleafBlock::neighborChanged(
    IWorld& world, const BlockPos& pos, Block& neighborBlock, const BlockPos& neighborPos, bool isMoving)
{
    MC_UNUSED(neighborBlock);
    MC_UNUSED(neighborPos);
    MC_UNUSED(isMoving);
    // TODO: 实现红石信号响应，重置倾斜状态
    MC_UNUSED(world);
    MC_UNUSED(pos);
}

const BlockState& BigDripleafBlock::rotate(const BlockState& state, Rotation rotation) const
{
    Direction facing = state.get(BlockStateProperties::HORIZONTAL_FACING());
    Direction rotated = Directions::rotateDirection(facing, rotation);
    return state.with(BlockStateProperties::HORIZONTAL_FACING(), rotated);
}

const BlockState& BigDripleafBlock::mirror(const BlockState& state, Mirror mirror) const
{
    Direction facing = state.get(BlockStateProperties::HORIZONTAL_FACING());
    Rotation rotation = Directions::mirrorToRotation(mirror, facing);
    Direction mirrored = Directions::rotateDirection(facing, rotation);
    return state.with(BlockStateProperties::HORIZONTAL_FACING(), mirrored);
}

} // namespace blocks
} // namespace mc
