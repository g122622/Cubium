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

#include "WallBlock.hpp"
#include "../../../../item/context/BlockItemUseContext.hpp"
#include "../../../../util/Direction.hpp"
#include "../../../../util/assert/AssertAll.hpp"
#include "../../../IWorld.hpp"
#include "../../WaterLoggableHelpers.hpp"

namespace mc {
namespace blocks {

WallBlock::WallBlock(const BlockProperties& properties)
    : Block(properties)
{
    auto container = StateContainer<Block, BlockState>::Builder(*this)
                         .add(BlockStateProperties::UP())
                         .add(BlockStateProperties::WALL_HEIGHT_NORTH())
                         .add(BlockStateProperties::WALL_HEIGHT_EAST())
                         .add(BlockStateProperties::WALL_HEIGHT_SOUTH())
                         .add(BlockStateProperties::WALL_HEIGHT_WEST())
                         .add(BlockStateProperties::WATERLOGGED())
                         .create([](const Block& block, std::vector<size_t> values, const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts, const std::vector<BlockState*>* allStates, u32 id) {
                             return std::make_unique<BlockState>(block, std::move(values), propertyLayouts, allStates, id);
                         });
    createBlockState(std::move(container));

    setDefaultState(defaultState()
            .with(BlockStateProperties::UP(), true)
            .with(BlockStateProperties::WALL_HEIGHT_NORTH(), BlockStateProperties::WallHeight::None)
            .with(BlockStateProperties::WALL_HEIGHT_EAST(), BlockStateProperties::WallHeight::None)
            .with(BlockStateProperties::WALL_HEIGHT_SOUTH(), BlockStateProperties::WallHeight::None)
            .with(BlockStateProperties::WALL_HEIGHT_WEST(), BlockStateProperties::WallHeight::None)
            .with(BlockStateProperties::WATERLOGGED(), false));

    constexpr f32 P = 1.0f / 16.0f;
    m_pillarShape = CollisionShape::box(4.0f * P, 0.0f, 4.0f * P, 12.0f * P, 16.0f * P, 12.0f * P);
    m_baseShape = m_pillarShape;

    for (size_t i = 0; i < m_shapes.size(); ++i) {
        m_shapes[i] = CollisionShape::empty();
    }

    for (int up = 0; up <= 1; ++up) {
        for (int north = 0; north <= 2; ++north) {
            for (int east = 0; east <= 2; ++east) {
                for (int south = 0; south <= 2; ++south) {
                    for (int west = 0; west <= 2; ++west) {
                        size_t idx = getShapeIndex(up != 0,
                            static_cast<BlockStateProperties::WallHeight>(north),
                            static_cast<BlockStateProperties::WallHeight>(east),
                            static_cast<BlockStateProperties::WallHeight>(south),
                            static_cast<BlockStateProperties::WallHeight>(west));

                        CollisionShape shape = m_pillarShape;
                        if (north > 0) {
                            f32 height = (north == 2) ? 16.0f : 8.0f;
                            shape = CollisionShape::combine(
                                shape, CollisionShape::box(5.0f * P, 0.0f, 0.0f, 11.0f * P, height * P, 8.0f * P));
                        }
                        if (south > 0) {
                            f32 height = (south == 2) ? 16.0f : 8.0f;
                            shape = CollisionShape::combine(
                                shape, CollisionShape::box(5.0f * P, 0.0f, 8.0f * P, 11.0f * P, height * P, 16.0f * P));
                        }
                        if (east > 0) {
                            f32 height = (east == 2) ? 16.0f : 8.0f;
                            shape = CollisionShape::combine(
                                shape, CollisionShape::box(8.0f * P, 0.0f, 5.0f * P, 16.0f * P, height * P, 11.0f * P));
                        }
                        if (west > 0) {
                            f32 height = (west == 2) ? 16.0f : 8.0f;
                            shape = CollisionShape::combine(
                                shape, CollisionShape::box(0.0f, 0.0f, 5.0f * P, 8.0f * P, height * P, 11.0f * P));
                        }

                        if (up != 0) {
                            shape = CollisionShape::combine(shape,
                                CollisionShape::box(4.0f * P, 14.0f * P, 4.0f * P, 12.0f * P, 16.0f * P, 12.0f * P));
                        }

                        m_shapes[idx] = shape;
                    }
                }
            }
        }
    }
}

BlockState WallBlock::getStateForPlacement(BlockItemUseContext& context)
{
    const IWorld& world = context.getWorld();
    BlockPos pos = context.placementPos();

    BlockState state = calculateState(world, pos, defaultState());
    bool waterlogged = waterloggable::shouldWaterlogAt(world, pos);
    return state.with(BlockStateProperties::WATERLOGGED(), waterlogged);
}

BlockState WallBlock::updatePostPlacement(const BlockState& state,
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

    return calculateState(world, currentPos, state);
}

const CollisionShape& WallBlock::getShape(const BlockState& state) const
{
    size_t index = getShapeIndex(state.get(BlockStateProperties::UP()),
        state.get(BlockStateProperties::WALL_HEIGHT_NORTH()),
        state.get(BlockStateProperties::WALL_HEIGHT_EAST()),
        state.get(BlockStateProperties::WALL_HEIGHT_SOUTH()),
        state.get(BlockStateProperties::WALL_HEIGHT_WEST()));
    MC_ASSERT(index < m_shapes.size());
    return m_shapes[index];
}

const CollisionShape& WallBlock::getCollisionShape(const BlockState& state) const
{
    return getShape(state);
}

bool WallBlock::canConnectRedstone(const BlockState& state, Direction side) const
{
    MC_UNUSED(state);
    MC_UNUSED(side);
    return true;
}

const BlockState& WallBlock::rotate(const BlockState& state, Rotation rotation) const
{
    BlockStateProperties::WallHeight north = state.get(BlockStateProperties::WALL_HEIGHT_NORTH());
    BlockStateProperties::WallHeight east = state.get(BlockStateProperties::WALL_HEIGHT_EAST());
    BlockStateProperties::WallHeight south = state.get(BlockStateProperties::WALL_HEIGHT_SOUTH());
    BlockStateProperties::WallHeight west = state.get(BlockStateProperties::WALL_HEIGHT_WEST());

    switch (rotation) {
        case Rotation::Clockwise90:
            return state.with(BlockStateProperties::WALL_HEIGHT_NORTH(), west)
                .with(BlockStateProperties::WALL_HEIGHT_EAST(), north)
                .with(BlockStateProperties::WALL_HEIGHT_SOUTH(), east)
                .with(BlockStateProperties::WALL_HEIGHT_WEST(), south);
        case Rotation::Clockwise180:
            return state.with(BlockStateProperties::WALL_HEIGHT_NORTH(), south)
                .with(BlockStateProperties::WALL_HEIGHT_EAST(), west)
                .with(BlockStateProperties::WALL_HEIGHT_SOUTH(), north)
                .with(BlockStateProperties::WALL_HEIGHT_WEST(), east);
        case Rotation::CounterClockwise90:
            return state.with(BlockStateProperties::WALL_HEIGHT_NORTH(), east)
                .with(BlockStateProperties::WALL_HEIGHT_EAST(), south)
                .with(BlockStateProperties::WALL_HEIGHT_SOUTH(), west)
                .with(BlockStateProperties::WALL_HEIGHT_WEST(), north);
        default:
            return state;
    }
}

const BlockState& WallBlock::mirror(const BlockState& state, Mirror mirror) const
{
    if (mirror == Mirror::None) {
        return state;
    }

    BlockStateProperties::WallHeight north = state.get(BlockStateProperties::WALL_HEIGHT_NORTH());
    BlockStateProperties::WallHeight east = state.get(BlockStateProperties::WALL_HEIGHT_EAST());
    BlockStateProperties::WallHeight south = state.get(BlockStateProperties::WALL_HEIGHT_SOUTH());
    BlockStateProperties::WallHeight west = state.get(BlockStateProperties::WALL_HEIGHT_WEST());

    if (mirror == Mirror::LeftRight) {
        return state.with(BlockStateProperties::WALL_HEIGHT_EAST(), west)
            .with(BlockStateProperties::WALL_HEIGHT_WEST(), east);
    }
    if (mirror == Mirror::FrontBack) {
        return state.with(BlockStateProperties::WALL_HEIGHT_NORTH(), south)
            .with(BlockStateProperties::WALL_HEIGHT_SOUTH(), north);
    }

    return state;
}

BlockState WallBlock::calculateState(const IWorld& world, const BlockPos& pos, const BlockState& state) const
{
    BlockPos northPos(pos.x, pos.y, pos.z - 1);
    BlockPos southPos(pos.x, pos.y, pos.z + 1);
    BlockPos eastPos(pos.x + 1, pos.y, pos.z);
    BlockPos westPos(pos.x - 1, pos.y, pos.z);

    const BlockState* northState = world.getBlockState(northPos);
    const BlockState* southState = world.getBlockState(southPos);
    const BlockState* eastState = world.getBlockState(eastPos);
    const BlockState* westState = world.getBlockState(westPos);

    BlockStateProperties::WallHeight northHeight =
        northState ? getWallHeight(*northState, Direction::North) : BlockStateProperties::WallHeight::None;
    BlockStateProperties::WallHeight southHeight =
        southState ? getWallHeight(*southState, Direction::South) : BlockStateProperties::WallHeight::None;
    BlockStateProperties::WallHeight eastHeight =
        eastState ? getWallHeight(*eastState, Direction::East) : BlockStateProperties::WallHeight::None;
    BlockStateProperties::WallHeight westHeight =
        westState ? getWallHeight(*westState, Direction::West) : BlockStateProperties::WallHeight::None;

    bool hasConnections = northHeight != BlockStateProperties::WallHeight::None ||
        southHeight != BlockStateProperties::WallHeight::None || eastHeight != BlockStateProperties::WallHeight::None ||
        westHeight != BlockStateProperties::WallHeight::None;
    bool straightNorthSouth = northHeight != BlockStateProperties::WallHeight::None &&
        southHeight != BlockStateProperties::WallHeight::None && eastHeight == BlockStateProperties::WallHeight::None &&
        westHeight == BlockStateProperties::WallHeight::None;
    bool straightEastWest = eastHeight != BlockStateProperties::WallHeight::None &&
        westHeight != BlockStateProperties::WallHeight::None && northHeight == BlockStateProperties::WallHeight::None &&
        southHeight == BlockStateProperties::WallHeight::None;

    bool oppositeTallNorthSouth = northHeight == BlockStateProperties::WallHeight::Tall &&
        southHeight == BlockStateProperties::WallHeight::Tall && eastHeight == BlockStateProperties::WallHeight::None &&
        westHeight == BlockStateProperties::WallHeight::None;
    bool oppositeTallEastWest = eastHeight == BlockStateProperties::WallHeight::Tall &&
        westHeight == BlockStateProperties::WallHeight::Tall && northHeight == BlockStateProperties::WallHeight::None &&
        southHeight == BlockStateProperties::WallHeight::None;

    bool hasUp = !hasConnections ||
        (!oppositeTallNorthSouth && !oppositeTallEastWest && !straightNorthSouth && !straightEastWest);

    return state.with(BlockStateProperties::UP(), hasUp)
        .with(BlockStateProperties::WALL_HEIGHT_NORTH(), northHeight)
        .with(BlockStateProperties::WALL_HEIGHT_EAST(), eastHeight)
        .with(BlockStateProperties::WALL_HEIGHT_SOUTH(), southHeight)
        .with(BlockStateProperties::WALL_HEIGHT_WEST(), westHeight);
}

BlockStateProperties::WallHeight WallBlock::getWallHeight(const BlockState& state, Direction neighborSide) const
{

    if (isWall(state)) {
        return BlockStateProperties::WallHeight::Tall;
    }

    if (isFenceGate(state)) {
        if (state.hasProperty(BlockStateProperties::OPEN()) && !state.get(BlockStateProperties::OPEN())) {
            Direction facing = state.get(BlockStateProperties::HORIZONTAL_FACING());
            if (Directions::getAxis(facing) != Directions::getAxis(neighborSide)) {
                return BlockStateProperties::WallHeight::Low;
            }
        }
        return BlockStateProperties::WallHeight::None;
    }

    if (state.hasOpaqueCollisionShape()) {
        return BlockStateProperties::WallHeight::Tall;
    }

    if (state.isSolid()) {
        return BlockStateProperties::WallHeight::Low;
    }

    return BlockStateProperties::WallHeight::None;
}

bool WallBlock::isWall(const BlockState& state)
{
    return state.hasProperty(BlockStateProperties::WALL_HEIGHT_NORTH());
}

bool WallBlock::isFenceGate(const BlockState& state)
{
    return state.hasProperty(BlockStateProperties::OPEN()) && state.hasProperty(BlockStateProperties::IN_WALL());
}

size_t WallBlock::getShapeIndex(bool up,
    BlockStateProperties::WallHeight north,
    BlockStateProperties::WallHeight east,
    BlockStateProperties::WallHeight south,
    BlockStateProperties::WallHeight west)
{

    size_t idx = (up ? 1 : 0);
    idx += static_cast<size_t>(north) * 2;
    idx += static_cast<size_t>(east) * 6;
    idx += static_cast<size_t>(south) * 18;
    idx += static_cast<size_t>(west) * 54;
    return idx;
}

const fluid::FluidState* WallBlock::getFluidState(const BlockState& state) const
{
    const fluid::FluidState* waterState = waterloggable::getWaterFluidState(state);
    return waterState != nullptr ? waterState : Block::getFluidState(state);
}

} // namespace blocks
} // namespace mc
