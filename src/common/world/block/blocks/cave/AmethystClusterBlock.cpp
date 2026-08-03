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

#include "AmethystClusterBlock.hpp"
#include "common/core/Types.hpp"
#include "common/item/context/BlockItemUseContext.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/util/Direction.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/property/Properties.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/util/property/StateHolder.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/WaterLoggableHelpers.hpp"
#include "common/world/block/blocks/cave/AmethystBlock.hpp"
#include "common/world/fluid/Fluid.hpp"
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

namespace mc {
namespace blocks {

AmethystClusterBlock::AmethystClusterBlock(const BlockProperties& properties, float height, float width)
    : AmethystBlock(properties)
    , m_height(height)
    , m_width(width)
{
    // 创建各方向的碰撞形状
    // 默认方向为UP（从底部向上生长），高度和宽度指定像素大小
    // 碰撞箱居中于方块，沿FACING方向延伸

    // UP方向：底部居中柱状
    float halfWidth = (16.0f - width) / 2.0f;
    m_shapes[Direction::Up] =
        CollisionShape::fromPixelBox(halfWidth, 0, halfWidth, 16.0f - halfWidth, height, 16.0f - halfWidth);
    // DOWN方向：顶部居中柱状
    m_shapes[Direction::Down] =
        CollisionShape::fromPixelBox(halfWidth, 16.0f - height, halfWidth, 16.0f - halfWidth, 16.0f, 16.0f - halfWidth);
    // NORTH方向：南面居中
    m_shapes[Direction::North] =
        CollisionShape::fromPixelBox(halfWidth, halfWidth, 16.0f - height, 16.0f - halfWidth, 16.0f - halfWidth, 16.0f);
    // SOUTH方向：北面居中
    m_shapes[Direction::South] =
        CollisionShape::fromPixelBox(halfWidth, halfWidth, 0, 16.0f - halfWidth, 16.0f - halfWidth, height);
    // EAST方向：西面居中
    m_shapes[Direction::East] =
        CollisionShape::fromPixelBox(0, halfWidth, halfWidth, height, 16.0f - halfWidth, 16.0f - halfWidth);
    // WEST方向：东面居中
    m_shapes[Direction::West] =
        CollisionShape::fromPixelBox(16.0f - height, halfWidth, halfWidth, 16.0f, 16.0f - halfWidth, 16.0f - halfWidth);

    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(BlockStateProperties::FACING())
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
            .with(BlockStateProperties::FACING(), Direction::Up)
            .with(BlockStateProperties::WATERLOGGED(), false));
}

void AmethystClusterBlock::fillStateContainer(StateContainer<Block, BlockState>& container)
{
    MC_UNUSED(container);
}

BlockState AmethystClusterBlock::getStateForPlacement(BlockItemUseContext& context)
{
    Direction facing = context.getClickedFace();
    BlockState state = defaultState().with(BlockStateProperties::FACING(), facing);

    if (waterloggable::shouldWaterlogAt(context.getWorld(), context.placementPos())) {
        state = state.with(BlockStateProperties::WATERLOGGED(), true);
    }

    return state;
}

BlockState AmethystClusterBlock::updatePostPlacement(const BlockState& state,
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

const CollisionShape& AmethystClusterBlock::getShape(const BlockState& state) const
{
    Direction facing = state.get(BlockStateProperties::FACING());
    auto it = m_shapes.find(facing);
    if (it != m_shapes.end()) {
        return it->second;
    }
    return m_shapes.at(Direction::Up);
}

const fluid::FluidState* AmethystClusterBlock::getFluidState(const BlockState& state) const
{
    return waterloggable::getWaterFluidState(state);
}

const BlockState& AmethystClusterBlock::rotate(const BlockState& state, Rotation rotation) const
{
    Direction facing = state.get(BlockStateProperties::FACING());
    Direction rotated = Directions::rotateDirection(facing, rotation);
    return state.with(BlockStateProperties::FACING(), rotated);
}

const BlockState& AmethystClusterBlock::mirror(const BlockState& state, Mirror mirror) const
{
    Direction facing = state.get(BlockStateProperties::FACING());
    Rotation rotation = Directions::mirrorToRotation(mirror, facing);
    Direction mirrored = Directions::rotateDirection(facing, rotation);
    return state.with(BlockStateProperties::FACING(), mirrored);
}

} // namespace blocks
} // namespace mc
