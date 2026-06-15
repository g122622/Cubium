/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies of substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, NONINFRINGEMENT OF THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR
 * THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include "SmallDripleafBlock.hpp"
#include "common/item/context/BlockItemUseContext.hpp"
#include "common/util/Direction.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/WaterLoggableHelpers.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"

namespace mc {
namespace blocks {

SmallDripleafBlock::SmallDripleafBlock(const BlockProperties& properties)
    : Block(properties)
    , m_shape(CollisionShape::fromPixelBox(2, 0, 2, 14, 16, 14))
{
    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(BlockStateProperties::HORIZONTAL_FACING())
            .add(BlockStateProperties::DOUBLE_BLOCK_HALF())
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
            .with(BlockStateProperties::DOUBLE_BLOCK_HALF(), BlockStateProperties::DoubleBlockHalf::Lower)
            .with(BlockStateProperties::WATERLOGGED(), false));
}

void SmallDripleafBlock::fillStateContainer(StateContainer<Block, BlockState>& container)
{
    MC_UNUSED(container);
}

BlockState SmallDripleafBlock::getStateForPlacement(BlockItemUseContext& context)
{
    Direction horizontalFacing = context.horizontalDirection();
    BlockState state =
        defaultState()
            .with(BlockStateProperties::HORIZONTAL_FACING(), horizontalFacing)
            .with(BlockStateProperties::DOUBLE_BLOCK_HALF(), BlockStateProperties::DoubleBlockHalf::Lower);

    if (waterloggable::shouldWaterlogAt(context.getWorld(), context.placementPos())) {
        state = state.with(BlockStateProperties::WATERLOGGED(), true);
    }

    return state;
}

BlockState SmallDripleafBlock::updatePostPlacement(const BlockState& state,
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

const CollisionShape& SmallDripleafBlock::getShape(const BlockState& state) const
{
    MC_UNUSED(state);
    return m_shape;
}

const fluid::FluidState* SmallDripleafBlock::getFluidState(const BlockState& state) const
{
    return waterloggable::getWaterFluidState(state);
}

const BlockState& SmallDripleafBlock::rotate(const BlockState& state, Rotation rotation) const
{
    Direction facing = state.get(BlockStateProperties::HORIZONTAL_FACING());
    Direction rotated = Directions::rotateDirection(facing, rotation);
    return state.with(BlockStateProperties::HORIZONTAL_FACING(), rotated);
}

const BlockState& SmallDripleafBlock::mirror(const BlockState& state, Mirror mirror) const
{
    Direction facing = state.get(BlockStateProperties::HORIZONTAL_FACING());
    Rotation rotation = Directions::mirrorToRotation(mirror, facing);
    Direction mirrored = Directions::rotateDirection(facing, rotation);
    return state.with(BlockStateProperties::HORIZONTAL_FACING(), mirrored);
}

bool SmallDripleafBlock::canGrow(
    IBlockReader& world, const BlockPos& pos, const BlockState& state, bool isClientSide) const
{
    MC_UNUSED(world);
    MC_UNUSED(pos);
    MC_UNUSED(state);
    MC_UNUSED(isClientSide);
    return true;
}

bool SmallDripleafBlock::canUseBonemeal(
    IWorld& world, math::IRandom& random, const BlockPos& pos, const BlockState& state) const
{
    MC_UNUSED(world);
    MC_UNUSED(pos);
    MC_UNUSED(state);
    MC_UNUSED(random);
    return true;
}

void SmallDripleafBlock::grow(IWorld& world, math::IRandom& random, const BlockPos& pos, const BlockState& state)
{
    // 获取朝向
    Direction facing = state.get(BlockStateProperties::HORIZONTAL_FACING());

    // 如果是上半部分，从下半部分位置开始
    BlockPos basePos = pos;
    if (state.get(BlockStateProperties::DOUBLE_BLOCK_HALF()) == BlockStateProperties::DoubleBlockHalf::Upper) {
        basePos = BlockPos(pos.x, pos.y - 1, pos.z);
    }

    // 随机茎高度1-5格
    i32 stemHeight = 1 + random.nextInt(5);

    // 检查上方是否有足够空间（茎 + 叶片）
    for (i32 i = 0; i <= stemHeight; ++i) {
        BlockPos checkPos(basePos.x, basePos.y + i, basePos.z);
        const BlockState* checkState = world.getBlockState(checkPos);
        if (checkState == nullptr) {
            return;
        }
        // 允许空气和小滴叶自身
        if (!checkState->isAir() && &checkState->getBlock() != this) {
            return;
        }
    }

    // 移除小滴叶（上下两部分）
    const BlockState& airState = VanillaBlocks::AIR->defaultState();
    world.setBlockState(basePos, &airState, 3);
    world.setBlockState(BlockPos(basePos.x, basePos.y + 1, basePos.z), &airState, 3);

    // 放置大滴叶茎
    const BlockState& stemState =
        VanillaBlocks::BIG_DRIPLEAF_STEM->defaultState().with(BlockStateProperties::HORIZONTAL_FACING(), facing);
    for (i32 i = 0; i < stemHeight; ++i) {
        BlockPos stemPos(basePos.x, basePos.y + i, basePos.z);
        world.setBlockState(stemPos, &stemState, 3);
    }

    // 放置大滴叶叶片
    const BlockState& leafState = VanillaBlocks::BIG_DRIPLEAF->defaultState()
                                      .with(BlockStateProperties::HORIZONTAL_FACING(), facing)
                                      .with(BlockStateProperties::TILT(), BlockStateProperties::Tilt::None);
    BlockPos leafPosition(basePos.x, basePos.y + stemHeight, basePos.z);
    world.setBlockState(leafPosition, &leafState, 3);
}

// ========== IPlantable 接口实现 ==========

PlantType SmallDripleafBlock::getPlantType(IBlockReader& world, const BlockPos& pos) const
{
    MC_UNUSED(world);
    MC_UNUSED(pos);
    // MC 1.21.11: SmallDripleafBlock 继承 BushBlock，返回 PlantType.WATER
    // 小滴叶可放置在黏土、泥土、砂土、灰化土、苔藓块、耕地和黏土上
    return PlantType::Water;
}

const BlockState& SmallDripleafBlock::getPlant(IBlockReader& world, const BlockPos& pos) const
{
    MC_UNUSED(world);
    return defaultState();
}

} // namespace blocks
} // namespace mc
