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

#include "PaneBlock.hpp"
#include "../../../../item/context/BlockItemUseContext.hpp"
#include "../../../../util/Direction.hpp"
#include "../../../../util/assert/AssertAll.hpp"
#include "../../../IWorld.hpp"
#include "../../WaterLoggableHelpers.hpp"
#include "../building/WallBlock.hpp"

namespace mc {
namespace blocks {

PaneBlock::PaneBlock(const BlockProperties& properties)
    : Block(properties)
{
    // 创建状态容器
    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(BlockStateProperties::NORTH())
            .add(BlockStateProperties::EAST())
            .add(BlockStateProperties::SOUTH())
            .add(BlockStateProperties::WEST())
            .add(BlockStateProperties::WATERLOGGED())
            .create([this](const Block& block,
                        std::vector<size_t> values,
                        const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
                        const std::vector<BlockState*>* allStates,
                        u32 id) {
                return std::make_unique<BlockState>(block, std::move(values), propertyLayouts, allStates, id);
            });
    createBlockState(std::move(container));

    // 设置默认状态
    setDefaultState(defaultState()
            .with(BlockStateProperties::NORTH(), false)
            .with(BlockStateProperties::EAST(), false)
            .with(BlockStateProperties::SOUTH(), false)
            .with(BlockStateProperties::WEST(), false)
            .with(BlockStateProperties::WATERLOGGED(), false));

    constexpr f32 P = 1.0f / 16.0f;

    // 中心柱：2x16x2 像素
    m_centerShape = CollisionShape::box(7.0f * P, 0.0f, 7.0f * P, 9.0f * P, 16.0f * P, 9.0f * P);

    // 边缘形状 (各方向连接面板)
    m_sideShapes[static_cast<size_t>(Direction::North)] =
        CollisionShape::box(7.0f * P, 0.0f, 0.0f, 9.0f * P, 16.0f * P, 7.0f * P);
    m_sideShapes[static_cast<size_t>(Direction::South)] =
        CollisionShape::box(7.0f * P, 0.0f, 9.0f * P, 9.0f * P, 16.0f * P, 16.0f * P);
    m_sideShapes[static_cast<size_t>(Direction::West)] =
        CollisionShape::box(0.0f, 0.0f, 7.0f * P, 7.0f * P, 16.0f * P, 9.0f * P);
    m_sideShapes[static_cast<size_t>(Direction::East)] =
        CollisionShape::box(9.0f * P, 0.0f, 7.0f * P, 16.0f * P, 16.0f * P, 9.0f * P);

    for (size_t i = 0; i < m_shapes.size(); ++i) {
        m_shapes[i] = CollisionShape::empty();
    }

    for (i32 north = 0; north <= 1; ++north) {
        for (i32 east = 0; east <= 1; ++east) {
            for (i32 south = 0; south <= 1; ++south) {
                for (i32 west = 0; west <= 1; ++west) {
                    const size_t index = getShapeIndex(north != 0, east != 0, south != 0, west != 0);

                    CollisionShape shape = m_centerShape;
                    if (north != 0) {
                        shape = CollisionShape::combine(shape, m_sideShapes[static_cast<size_t>(Direction::North)]);
                    }
                    if (east != 0) {
                        shape = CollisionShape::combine(shape, m_sideShapes[static_cast<size_t>(Direction::East)]);
                    }
                    if (south != 0) {
                        shape = CollisionShape::combine(shape, m_sideShapes[static_cast<size_t>(Direction::South)]);
                    }
                    if (west != 0) {
                        shape = CollisionShape::combine(shape, m_sideShapes[static_cast<size_t>(Direction::West)]);
                    }

                    m_shapes[index] = shape;
                }
            }
        }
    }
}

BlockState PaneBlock::getStateForPlacement(BlockItemUseContext& context)
{
    BlockPos pos = context.placementPos();
    const IWorld& world = context.getWorld();

    const BlockState* northState = world.getBlockState(pos.north());
    const BlockState* eastState = world.getBlockState(pos.east());
    const BlockState* southState = world.getBlockState(pos.south());
    const BlockState* westState = world.getBlockState(pos.west());

    bool north = northState != nullptr &&
        shouldConnectTo(const_cast<IWorld&>(world), pos.north(), *northState, Direction::North);
    bool east =
        eastState != nullptr && shouldConnectTo(const_cast<IWorld&>(world), pos.east(), *eastState, Direction::East);
    bool south = southState != nullptr &&
        shouldConnectTo(const_cast<IWorld&>(world), pos.south(), *southState, Direction::South);
    bool west =
        westState != nullptr && shouldConnectTo(const_cast<IWorld&>(world), pos.west(), *westState, Direction::West);
    bool waterlogged = waterloggable::shouldWaterlogAt(world, pos);

    return defaultState()
        .with(BlockStateProperties::NORTH(), north)
        .with(BlockStateProperties::EAST(), east)
        .with(BlockStateProperties::SOUTH(), south)
        .with(BlockStateProperties::WEST(), west)
        .with(BlockStateProperties::WATERLOGGED(), waterlogged);
}

BlockState PaneBlock::updatePostPlacement(const BlockState& state,
    Direction facing,
    const BlockState& facingState,
    IWorld& world,
    const BlockPos& currentPos,
    const BlockPos& facingPos)
{
    if (state.get(BlockStateProperties::WATERLOGGED())) {
        waterloggable::scheduleWaterTick(world, currentPos);
    }

    if (!Directions::isHorizontal(facing)) {
        MC_UNUSED(facingPos);
        return state;
    }

    switch (facing) {
        case Direction::North:
            return state.with(BlockStateProperties::NORTH(), shouldConnectTo(world, facingPos, facingState, facing));
        case Direction::East:
            return state.with(BlockStateProperties::EAST(), shouldConnectTo(world, facingPos, facingState, facing));
        case Direction::South:
            return state.with(BlockStateProperties::SOUTH(), shouldConnectTo(world, facingPos, facingState, facing));
        case Direction::West:
            return state.with(BlockStateProperties::WEST(), shouldConnectTo(world, facingPos, facingState, facing));
        default:
            return state;
    }
}

const CollisionShape& PaneBlock::getCollisionShape(const BlockState& state) const
{
    const size_t index = getShapeIndex(state.get(BlockStateProperties::NORTH()),
        state.get(BlockStateProperties::EAST()),
        state.get(BlockStateProperties::SOUTH()),
        state.get(BlockStateProperties::WEST()));
    MC_ASSERT(index < m_shapes.size());
    return m_shapes[index];
}

const CollisionShape& PaneBlock::getShape(const BlockState& state) const
{
    return getCollisionShape(state);
}

const fluid::FluidState* PaneBlock::getFluidState(const BlockState& state) const
{
    const fluid::FluidState* waterState = waterloggable::getWaterFluidState(state);
    return waterState != nullptr ? waterState : Block::getFluidState(state);
}

bool PaneBlock::connectsTo(const BlockState& state, Direction facing) noexcept
{
    if (facing == Direction::North) {
        return state.get(BlockStateProperties::NORTH());
    } else if (facing == Direction::South) {
        return state.get(BlockStateProperties::SOUTH());
    } else if (facing == Direction::West) {
        return state.get(BlockStateProperties::WEST());
    } else if (facing == Direction::East) {
        return state.get(BlockStateProperties::EAST());
    }
    return false;
}

bool PaneBlock::shouldConnectTo(
    IWorld& world, const BlockPos& pos, const BlockState& neighborState, Direction direction) const
{
    const Block& neighborBlock = neighborState.getBlock();
    const Direction oppositeDirection = Directions::opposite(direction);

    if (&neighborBlock == this) {
        return true;
    }

    if (WallBlock::isWall(neighborState)) {
        return true;
    }

    return neighborBlock.isSolidSide(neighborState, world, pos, oppositeDirection);
}

size_t PaneBlock::getShapeIndex(bool north, bool east, bool south, bool west) noexcept
{
    return static_cast<size_t>(north) | (static_cast<size_t>(east) << 1U) | (static_cast<size_t>(south) << 2U) |
        (static_cast<size_t>(west) << 3U);
}

} // namespace blocks
} // namespace mc
