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

#include "common/world/block/blocks/building/WallBlock.hpp"
#include "common/core/Types.hpp"
#include "common/item/context/BlockItemUseContext.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/physics/shape/BooleanOp.hpp"
#include "common/physics/shape/Shapes.hpp"
#include "common/physics/shape/VoxelShape.hpp"
#include "common/util/Direction.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/util/property/Properties.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/util/property/StateHolder.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockTags.hpp"
#include "common/world/block/FenceGateHelpers.hpp"
#include "common/world/block/WaterLoggableHelpers.hpp"
#include <cstddef>
#include <map>
#include <memory>
#include <utility>
#include <vector>

namespace mc {
namespace blocks {

// ============================================================================
// 静态成员初始化
// ============================================================================

// 墙柱测试形状: 中心 2x16x2 像素柱形区域
// box(7/16, 0, 7/16, 9/16, 1, 9/16)
VoxelShape WallBlock::s_testShapePost = Shapes::box(7.0 / 16.0, 0.0, 7.0 / 16.0, 9.0 / 16.0, 1.0, 9.0 / 16.0);

// 墙臂测试形状: 每个方向对应一个 2x16x9 像素的墙臂测试区域
// 北面方向: box(7/16, 0, 0, 9/16, 1, 9/16)，然后旋转到四个水平方向
std::map<Direction, VoxelShape> WallBlock::s_testShapesWall = {
    {Direction::North, Shapes::box(7.0 / 16.0, 0.0, 0.0, 9.0 / 16.0, 1.0, 9.0 / 16.0)},
    {Direction::South, Shapes::box(7.0 / 16.0, 0.0, 7.0 / 16.0, 9.0 / 16.0, 1.0, 1.0)},
    {Direction::East, Shapes::box(7.0 / 16.0, 0.0, 7.0 / 16.0, 1.0, 1.0, 9.0 / 16.0)},
    {Direction::West, Shapes::box(0.0, 0.0, 7.0 / 16.0, 9.0 / 16.0, 1.0, 9.0 / 16.0)},
};

WallBlock::WallBlock(const BlockProperties& properties)
    : Block(properties)
{
    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(BlockStateProperties::UP())
            .add(BlockStateProperties::WALL_HEIGHT_NORTH())
            .add(BlockStateProperties::WALL_HEIGHT_EAST())
            .add(BlockStateProperties::WALL_HEIGHT_SOUTH())
            .add(BlockStateProperties::WALL_HEIGHT_WEST())
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
                        size_t idx = _getShapeIndex(up != 0,
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

    BlockState state = _calculateState(world, pos, defaultState());
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

    return _calculateState(world, currentPos, state);
}

const CollisionShape& WallBlock::getShape(const BlockState& state) const
{
    size_t index = _getShapeIndex(state.get(BlockStateProperties::UP()),
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

bool WallBlock::canConnectRedstone(const BlockState& state, Direction side) const noexcept
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

bool WallBlock::_shouldRaisePost(const BlockState& state,
    BlockStateProperties::WallHeight northHeight,
    BlockStateProperties::WallHeight eastHeight,
    BlockStateProperties::WallHeight southHeight,
    BlockStateProperties::WallHeight westHeight,
    const IWorld& world,
    const BlockPos& pos) const
{
    // 检查上方方块
    BlockPos abovePos(pos.x, pos.y + 1, pos.z);
    const BlockState* aboveState = world.getBlockState(abovePos);

    // 如果上方是墙且UP=true，强制升起墙柱
    if (aboveState && isWall(*aboveState) && aboveState->get(BlockStateProperties::UP())) {
        return true;
    }

    bool northNone = northHeight == BlockStateProperties::WallHeight::None;
    bool southNone = southHeight == BlockStateProperties::WallHeight::None;
    bool eastNone = eastHeight == BlockStateProperties::WallHeight::None;
    bool westNone = westHeight == BlockStateProperties::WallHeight::None;

    // 无连接时升起墙柱
    if (northNone && southNone && eastNone && westNone) {
        return true;
    }

    // 南北或东西不对称时升起墙柱
    if (northNone != southNone || eastNone != westNone) {
        return true;
    }

    // 对称直线墙：检查两侧是否都为Tall
    // 仅当两侧都为Tall时直线墙不升起墙柱，Low高度的直线墙
    // 需要进一步检查上方覆盖情况
    bool northTall = northHeight == BlockStateProperties::WallHeight::Tall;
    bool southTall = southHeight == BlockStateProperties::WallHeight::Tall;
    bool eastTall = eastHeight == BlockStateProperties::WallHeight::Tall;
    bool westTall = westHeight == BlockStateProperties::WallHeight::Tall;

    if ((northTall && southTall) || (eastTall && westTall)) {
        // 两侧都为Tall的直线墙不升起墙柱
        return false;
    }

    // 非Tall直线墙（Low连接）或角落连接：检查上方覆盖
    if (aboveState && BlockTags::WALL_POST_OVERRIDE().contains(*aboveState)) {
        return true;
    }

    // 检查上方方块的碰撞形状下方面是否覆盖墙柱测试形状(TEST_SHAPE_POST)
    if (aboveState) {
        const CollisionShape& aboveCollision = aboveState->getCollisionShape();
        if (!aboveCollision.isEmpty()) {
            VoxelShape aboveFaceShape = Shapes::fromCollisionShape(aboveCollision).getFaceShape(Direction::Down);
            if (isCovered(s_testShapePost, aboveFaceShape)) {
                return true;
            }
        }
    }

    return false;
}

BlockState WallBlock::_calculateState(const IWorld& world, const BlockPos& pos, const BlockState& state) const
{
    BlockPos northPos(pos.x, pos.y, pos.z - 1);
    BlockPos southPos(pos.x, pos.y, pos.z + 1);
    BlockPos eastPos(pos.x + 1, pos.y, pos.z);
    BlockPos westPos(pos.x - 1, pos.y, pos.z);

    const BlockState* northState = world.getBlockState(northPos);
    const BlockState* southState = world.getBlockState(southPos);
    const BlockState* eastState = world.getBlockState(eastPos);
    const BlockState* westState = world.getBlockState(westPos);

    // 获取上方方块碰撞形状的下方面投影，用于确定墙连接高度(Tall/Low)
    BlockPos abovePos(pos.x, pos.y + 1, pos.z);
    const BlockState* aboveState = world.getBlockState(abovePos);
    VoxelShape aboveFaceShape = Shapes::empty();
    if (aboveState) {
        const CollisionShape& aboveCollision = aboveState->getCollisionShape();
        if (!aboveCollision.isEmpty()) {
            aboveFaceShape = Shapes::fromCollisionShape(aboveCollision).getFaceShape(Direction::Down);
        }
    }

    BlockStateProperties::WallHeight northHeight = northState
        ? _getWallHeight(*northState, Direction::North, aboveFaceShape)
        : BlockStateProperties::WallHeight::None;
    BlockStateProperties::WallHeight southHeight = southState
        ? _getWallHeight(*southState, Direction::South, aboveFaceShape)
        : BlockStateProperties::WallHeight::None;
    BlockStateProperties::WallHeight eastHeight = eastState
        ? _getWallHeight(*eastState, Direction::East, aboveFaceShape)
        : BlockStateProperties::WallHeight::None;
    BlockStateProperties::WallHeight westHeight = westState
        ? _getWallHeight(*westState, Direction::West, aboveFaceShape)
        : BlockStateProperties::WallHeight::None;

    bool hasUp = _shouldRaisePost(state, northHeight, eastHeight, southHeight, westHeight, world, pos);

    return state.with(BlockStateProperties::UP(), hasUp)
        .with(BlockStateProperties::WALL_HEIGHT_NORTH(), northHeight)
        .with(BlockStateProperties::WALL_HEIGHT_EAST(), eastHeight)
        .with(BlockStateProperties::WALL_HEIGHT_SOUTH(), southHeight)
        .with(BlockStateProperties::WALL_HEIGHT_WEST(), westHeight);
}

BlockStateProperties::WallHeight WallBlock::_getWallHeight(
    const BlockState& state, Direction neighborSide, const VoxelShape& aboveFaceShape) const
{
    // 连接判定:
    // 1. 其他墙 -> 总是连接
    // 2. 栅栏门平行时 -> 连接
    // 3. 铁栏杆 -> 连接
    // 4. 固体方块（非连接例外）-> 连接
    // 连接高度判定:
    // - 如果连接且上方方块碰撞形状覆盖了对应方向的测试形状 -> Tall
    // - 如果连接但上方不覆盖 -> Low

    if (isWall(state)) {
        // 墙与墙连接：检查上方覆盖决定高度
        if (isCovered(s_testShapesWall.at(neighborSide), aboveFaceShape)) {
            return BlockStateProperties::WallHeight::Tall;
        }
        return BlockStateProperties::WallHeight::Low;
    }

    if (fencehelpers::isFenceGate(state)) {
        if (fencehelpers::isFenceGateParallel(state, neighborSide)) {
            return BlockStateProperties::WallHeight::Low;
        }
        return BlockStateProperties::WallHeight::None;
    }

    // 铁栏杆连接到墙时，检查上方覆盖决定高度
    if (BlockTags::BARS().contains(state)) {
        if (isCovered(s_testShapesWall.at(neighborSide), aboveFaceShape)) {
            return BlockStateProperties::WallHeight::Tall;
        }
        return BlockStateProperties::WallHeight::Low;
    }

    // 固体方块连接（排除连接例外方块）
    if (!Block::isExceptionForConnection(state) && state.isSolid()) {
        // 固体方块：上方覆盖时返回Tall，否则Low
        if (isCovered(s_testShapesWall.at(neighborSide), aboveFaceShape)) {
            return BlockStateProperties::WallHeight::Tall;
        }
        return BlockStateProperties::WallHeight::Low;
    }

    return BlockStateProperties::WallHeight::None;
}

bool WallBlock::isCovered(const VoxelShape& testShape, const VoxelShape& coverShape)
{
    // isCovered: 判断 coverShape 是否完全覆盖 testShape
    // 使用 OnlyFirst 布尔运算: 如果 testShape 中没有任何部分不被 coverShape 覆盖，则返回 true
    if (Shapes::isBlock(coverShape)) {
        return true;
    }
    if (testShape.isEmpty()) {
        return true;
    }
    if (coverShape.isEmpty()) {
        return false;
    }
    return !Shapes::joinIsNotEmpty(testShape, coverShape, BooleanOps::OnlyFirst());
}

bool WallBlock::isWall(const BlockState& state)
{
    return BlockTags::WALLS().contains(state);
}

size_t WallBlock::_getShapeIndex(bool up,
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
