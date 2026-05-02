#include "FenceBlock.hpp"
#include "../../../IWorld.hpp"
#include "../../../tick/manager/TickManager.hpp"
#include "../../WaterLoggableHelpers.hpp"
#include "../../../../item/context/BlockItemUseContext.hpp"
#include "../../../fluid/Fluid.hpp"
#include "../../../fluid/FluidRegistry.hpp"
#include "../../../fluid/FluidTags.hpp"
#include "../../../../util/Direction.hpp"
#include "../../../../util/assert/AssertAll.hpp"

namespace mc {
namespace blocks {

// ========== 构造函数 ==========

FenceBlock::FenceBlock(const BlockProperties& properties)
    : Block(properties) {

    // 创建状态容器
    auto container = StateContainer<Block, BlockState>::Builder(*this)
        .add(BlockStateProperties::NORTH())
        .add(BlockStateProperties::EAST())
        .add(BlockStateProperties::SOUTH())
        .add(BlockStateProperties::WEST())
        .add(BlockStateProperties::WATERLOGGED())
        .create([](const Block& block, std::unordered_map<const IProperty*, size_t> values, u32 id) {
            return std::make_unique<BlockState>(block, std::move(values), id);
        });
    createBlockState(std::move(container));

    // 设置默认状态
    setDefaultState(defaultState()
        .with(BlockStateProperties::NORTH(), false)
        .with(BlockStateProperties::EAST(), false)
        .with(BlockStateProperties::SOUTH(), false)
        .with(BlockStateProperties::WEST(), false)
        .with(BlockStateProperties::WATERLOGGED(), false));

    // 预计算碰撞形状
    // 像素单位
    constexpr f32 P = 1.0f / 16.0f;

    // 栅栏柱子：中间6x16x6像素（MC 1.16.5: Block.makeCuboidShape(6, 0, 6, 10, 16, 10)）
    CollisionShape pillar = CollisionShape::box(6.0f * P, 0.0f, 6.0f * P, 10.0f * P, 16.0f * P, 10.0f * P);

    // 栅栏横杆：在7-9像素高度，厚2像素
    // 北面横杆
    CollisionShape railNorth = CollisionShape::box(7.0f * P, 6.0f * P, 0.0f, 9.0f * P, 9.0f * P, 6.0f * P);
    // 南面横杆
    CollisionShape railSouth = CollisionShape::box(7.0f * P, 6.0f * P, 10.0f * P, 9.0f * P, 9.0f * P, 16.0f * P);
    // 东面横杆
    CollisionShape railEast = CollisionShape::box(10.0f * P, 6.0f * P, 7.0f * P, 16.0f * P, 9.0f * P, 9.0f * P);
    // 西面横杆
    CollisionShape railWest = CollisionShape::box(0.0f * P, 6.0f * P, 7.0f * P, 6.0f * P, 9.0f * P, 9.0f * P);

    // 初始化所有16种形状
    for (size_t i = 0; i < 16; ++i) {
        m_shapes[i] = CollisionShape::empty();
    }

    for (int north = 0; north <= 1; ++north) {
        for (int east = 0; east <= 1; ++east) {
            for (int south = 0; south <= 1; ++south) {
                for (int west = 0; west <= 1; ++west) {
                    size_t idx = getShapeIndex(north != 0, east != 0, south != 0, west != 0);

                    CollisionShape shape = pillar;

                    if (north) shape = CollisionShape::combine(shape, railNorth);
                    if (east) shape = CollisionShape::combine(shape, railEast);
                    if (south) shape = CollisionShape::combine(shape, railSouth);
                    if (west) shape = CollisionShape::combine(shape, railWest);

                    m_shapes[idx] = shape;
                }
            }
        }
    }
}

// ========== 放置和更新 ==========

BlockState FenceBlock::getStateForPlacement(BlockItemUseContext& context) {
    const IWorld& world = context.getWorld();
    BlockPos pos = context.placementPos();

    // 计算初始连接状态
    const BlockState* northState = world.getBlockState(pos.north());
    const BlockState* eastState = world.getBlockState(pos.east());
    const BlockState* southState = world.getBlockState(pos.south());
    const BlockState* westState = world.getBlockState(pos.west());

    bool connectNorth = northState && canConnect(*northState, northState->isSolid());
    bool connectEast = eastState && canConnect(*eastState, eastState->isSolid());
    bool connectSouth = southState && canConnect(*southState, southState->isSolid());
    bool connectWest = westState && canConnect(*westState, westState->isSolid());

    // 检查是否含水
    bool waterlogged = waterloggable::shouldWaterlogAt(world, pos);

    return defaultState()
        .with(BlockStateProperties::NORTH(), connectNorth)
        .with(BlockStateProperties::EAST(), connectEast)
        .with(BlockStateProperties::SOUTH(), connectSouth)
        .with(BlockStateProperties::WEST(), connectWest)
        .with(BlockStateProperties::WATERLOGGED(), waterlogged);
}

BlockState FenceBlock::updatePostPlacement(
    const BlockState& state,
    Direction facing,
    const BlockState& facingState,
    IWorld& world,
    const BlockPos& currentPos,
    const BlockPos& facingPos) {

    // 处理含水状态
    if (state.get(BlockStateProperties::WATERLOGGED())) {
        waterloggable::scheduleWaterTick(world, currentPos);
    }

    // 只处理水平方向的更新
    if (Directions::getAxis(facing) != Axis::Y) {
        bool shouldConnect = canConnect(facingState, facingState.isSolid());

        switch (facing) {
            case Direction::North:
                return state.with(BlockStateProperties::NORTH(), shouldConnect);
            case Direction::East:
                return state.with(BlockStateProperties::EAST(), shouldConnect);
            case Direction::South:
                return state.with(BlockStateProperties::SOUTH(), shouldConnect);
            case Direction::West:
                return state.with(BlockStateProperties::WEST(), shouldConnect);
            default:
                break;
        }
    }

    return state;
}

// ========== 形状 ==========

const CollisionShape& FenceBlock::getShape(const BlockState& state) const {
    size_t index = getShapeIndex(
        state.get(BlockStateProperties::NORTH()),
        state.get(BlockStateProperties::EAST()),
        state.get(BlockStateProperties::SOUTH()),
        state.get(BlockStateProperties::WEST())
    );
    return m_shapes[index];
}

const CollisionShape& FenceBlock::getCollisionShape(const BlockState& state) const {
    return getShape(state);
}

const CollisionShape& FenceBlock::getOcclusionShape(const BlockState& state) const {
    MC_UNUSED(state);
    // 栅栏不阻挡光线
    static CollisionShape emptyShape = CollisionShape::empty();
    return emptyShape;
}

// ========== 旋转和镜像 ==========

const BlockState& FenceBlock::rotate(const BlockState& state, Rotation rotation) const {
    bool north = state.get(BlockStateProperties::NORTH());
    bool east = state.get(BlockStateProperties::EAST());
    bool south = state.get(BlockStateProperties::SOUTH());
    bool west = state.get(BlockStateProperties::WEST());

    switch (rotation) {
        case Rotation::Clockwise90:
            return state
                .with(BlockStateProperties::NORTH(), west)
                .with(BlockStateProperties::EAST(), north)
                .with(BlockStateProperties::SOUTH(), east)
                .with(BlockStateProperties::WEST(), south);
        case Rotation::Clockwise180:
            return state
                .with(BlockStateProperties::NORTH(), south)
                .with(BlockStateProperties::EAST(), west)
                .with(BlockStateProperties::SOUTH(), north)
                .with(BlockStateProperties::WEST(), east);
        case Rotation::CounterClockwise90:
            return state
                .with(BlockStateProperties::NORTH(), east)
                .with(BlockStateProperties::EAST(), south)
                .with(BlockStateProperties::SOUTH(), west)
                .with(BlockStateProperties::WEST(), north);
        default:
            return state;
    }
}

const BlockState& FenceBlock::mirror(const BlockState& state, Mirror mirror) const {
    if (mirror == Mirror::None) {
        return state;
    }

    bool east = state.get(BlockStateProperties::EAST());
    bool west = state.get(BlockStateProperties::WEST());
    bool north = state.get(BlockStateProperties::NORTH());
    bool south = state.get(BlockStateProperties::SOUTH());

    if (mirror == Mirror::LeftRight) {
        // 左右镜像（X轴）：east<->west
        return state
            .with(BlockStateProperties::EAST(), west)
            .with(BlockStateProperties::WEST(), east);
    } else if (mirror == Mirror::FrontBack) {
        // 前后镜像（Z轴）：north<->south
        return state
            .with(BlockStateProperties::NORTH(), south)
            .with(BlockStateProperties::SOUTH(), north);
    }

    return state;
}

// ========== 私有方法 ==========

bool FenceBlock::canConnect(const BlockState& state, bool isNeighborSolid) const {
    // 连接到固体方块
    if (isNeighborSolid) {
        return true;
    }

    // 连接到其他栅栏
    if (state.hasProperty(BlockStateProperties::NORTH())) {
        return true;
    }

    // 连接到栅栏门
    if (state.hasProperty(BlockStateProperties::OPEN()) && state.hasProperty(BlockStateProperties::IN_WALL())) {
        // 栅栏门：检查朝向
        Direction facing = state.get(BlockStateProperties::HORIZONTAL_FACING());
        // 栅栏门在南北方向连接东西方向的栅栏，在东西方向连接南北方向的栅栏
        // 简化处理：总是连接
        return true;
    }

    // 连接到墙
    if (state.hasProperty(BlockStateProperties::WALL_HEIGHT_NORTH())) {
        return true;
    }

    return false;
}

size_t FenceBlock::getShapeIndex(bool north, bool east, bool south, bool west) {
    size_t index = 0;
    if (north) index |= 1;
    if (east) index |= 2;
    if (south) index |= 4;
    if (west) index |= 8;
    return index;
}

// ========== IWaterLoggable 接口实现 ==========

const fluid::FluidState* FenceBlock::getFluidState(const BlockState& state) const {
    const fluid::FluidState* waterState = waterloggable::getWaterFluidState(state);
    return waterState != nullptr ? waterState : Block::getFluidState(state);
}

} // namespace blocks
} // namespace mc
