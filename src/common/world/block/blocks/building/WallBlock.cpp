#include "WallBlock.hpp"
#include "../../../IWorld.hpp"
#include "../../../../item/BlockItemUseContext.hpp"
#include "../../../fluid/Fluid.hpp"
#include "../../../../util/Direction.hpp"
#include "../../../../util/assert/AssertAll.hpp"

namespace mc {
namespace blocks {

// ========== 构造函数 ==========

WallBlock::WallBlock(const BlockProperties& properties)
    : Block(properties) {

    // 创建状态容器
    auto container = StateContainer<Block, BlockState>::Builder(*this)
        .add(BlockStateProperties::UP())
        .add(BlockStateProperties::WALL_HEIGHT_NORTH())
        .add(BlockStateProperties::WALL_HEIGHT_EAST())
        .add(BlockStateProperties::WALL_HEIGHT_SOUTH())
        .add(BlockStateProperties::WALL_HEIGHT_WEST())
        .add(BlockStateProperties::WATERLOGGED())
        .create([](const Block& block, std::unordered_map<const IProperty*, size_t> values, u32 id) {
            return std::make_unique<BlockState>(block, std::move(values), id);
        });
    createBlockState(std::move(container));

    // 设置默认状态
    setDefaultState(defaultState()
        .with(BlockStateProperties::UP(), true)
        .with(BlockStateProperties::WALL_HEIGHT_NORTH(), BlockStateProperties::WallHeight::None)
        .with(BlockStateProperties::WALL_HEIGHT_EAST(), BlockStateProperties::WallHeight::None)
        .with(BlockStateProperties::WALL_HEIGHT_SOUTH(), BlockStateProperties::WallHeight::None)
        .with(BlockStateProperties::WALL_HEIGHT_WEST(), BlockStateProperties::WallHeight::None)
        .with(BlockStateProperties::WATERLOGGED(), false));

    // 预计算碰撞形状
    // 像素单位
    constexpr f32 P = 1.0f / 16.0f;

    // 墙柱形状（中间4x4x14像素的柱子）
    m_pillarShape = CollisionShape::box(4.0f * P, 0.0f, 4.0f * P, 12.0f * P, 16.0f * P, 12.0f * P);

    // 低连接形状（4x4x8像素，高度从0到8像素）
    // 高连接形状（4x4x16像素，完整高度）

    // 初始化形状缓存
    for (size_t i = 0; i < 162; ++i) {
        m_shapes[i] = CollisionShape::empty();
    }

    // 预计算所有形状组合
    // 索引: up + north*2 + east*6 + south*18 + west*54
    for (int up = 0; up <= 1; ++up) {
        for (int north = 0; north <= 2; ++north) {
            for (int east = 0; east <= 2; ++east) {
                for (int south = 0; south <= 2; ++south) {
                    for (int west = 0; west <= 2; ++west) {
                        size_t idx = getShapeIndex(
                            up != 0,
                            static_cast<BlockStateProperties::WallHeight>(north),
                            static_cast<BlockStateProperties::WallHeight>(east),
                            static_cast<BlockStateProperties::WallHeight>(south),
                            static_cast<BlockStateProperties::WallHeight>(west)
                        );

                        CollisionShape shape = m_pillarShape;

                        // 添加各方向的连接
                        if (north > 0) {
                            f32 height = (north == 2) ? 16.0f : 8.0f;
                            shape = CollisionShape::combine(shape,
                                CollisionShape::box(5.0f * P, 0.0f, 0.0f, 11.0f * P, height * P, 8.0f * P));
                        }
                        if (south > 0) {
                            f32 height = (south == 2) ? 16.0f : 8.0f;
                            shape = CollisionShape::combine(shape,
                                CollisionShape::box(5.0f * P, 0.0f, 8.0f * P, 11.0f * P, height * P, 16.0f * P));
                        }
                        if (east > 0) {
                            f32 height = (east == 2) ? 16.0f : 8.0f;
                            shape = CollisionShape::combine(shape,
                                CollisionShape::box(8.0f * P, 0.0f, 5.0f * P, 16.0f * P, height * P, 11.0f * P));
                        }
                        if (west > 0) {
                            f32 height = (west == 2) ? 16.0f : 8.0f;
                            shape = CollisionShape::combine(shape,
                                CollisionShape::box(0.0f, 0.0f, 5.0f * P, 8.0f * P, height * P, 11.0f * P));
                        }

                        // 如果有顶部突起且无任何连接，添加顶部盖子
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

// ========== 放置和更新 ==========

BlockState WallBlock::getStateForPlacement(BlockItemUseContext& context) {
    const IWorld& world = context.getWorld();
    BlockPos pos = context.placementPos();

    // 计算初始连接状态
    BlockState state = calculateState(world, pos, defaultState());

    // 检查是否含水
    const BlockState* existingState = world.getBlockState(pos.x, pos.y, pos.z);
    bool waterlogged = false;
    if (existingState != nullptr) {
        const fluid::FluidState* fluid = existingState->getFluidState();
        waterlogged = fluid != nullptr && fluid->isSource();
    }

    return state.with(BlockStateProperties::WATERLOGGED(), waterlogged);
}

BlockState WallBlock::updatePostPlacement(
    const BlockState& state,
    Direction facing,
    const BlockState& facingState,
    IWorld& world,
    const BlockPos& currentPos,
    const BlockPos& facingPos) {

    MC_UNUSED(facingState);
    MC_UNUSED(facingPos);

    // 检查是否是水平方向的更新
    if (Directions::getAxis(facing) == Axis::Y) {
        // 垂直方向更新，检查顶部
        bool up = state.get(BlockStateProperties::UP());
        // 重新计算顶部
        return calculateState(world, currentPos, state);
    }

    // 水平方向更新，重新计算连接
    return calculateState(world, currentPos, state);
}

// ========== 形状 ==========

const CollisionShape& WallBlock::getShape(const BlockState& state) const {
    bool up = state.get(BlockStateProperties::UP());
    BlockStateProperties::WallHeight north = state.get(BlockStateProperties::WALL_HEIGHT_NORTH());
    BlockStateProperties::WallHeight east = state.get(BlockStateProperties::WALL_HEIGHT_EAST());
    BlockStateProperties::WallHeight south = state.get(BlockStateProperties::WALL_HEIGHT_SOUTH());
    BlockStateProperties::WallHeight west = state.get(BlockStateProperties::WALL_HEIGHT_WEST());

    size_t index = getShapeIndex(up, north, east, south, west);
    MC_ASSERT(index < 162);
    return m_shapes[index];
}

const CollisionShape& WallBlock::getCollisionShape(const BlockState& state) const {
    return getShape(state);
}

// ========== 红石连接 ==========

bool WallBlock::canConnectRedstone(const BlockState& state, Direction side) const {
    // 墙可以连接红石（顶部）
    MC_UNUSED(state);
    MC_UNUSED(side);
    return true;
}

// ========== 旋转和镜像 ==========

const BlockState& WallBlock::rotate(const BlockState& state, Rotation rotation) const {
    // 旋转90度：north->east->south->west->north
    BlockStateProperties::WallHeight north = state.get(BlockStateProperties::WALL_HEIGHT_NORTH());
    BlockStateProperties::WallHeight east = state.get(BlockStateProperties::WALL_HEIGHT_EAST());
    BlockStateProperties::WallHeight south = state.get(BlockStateProperties::WALL_HEIGHT_SOUTH());
    BlockStateProperties::WallHeight west = state.get(BlockStateProperties::WALL_HEIGHT_WEST());

    switch (rotation) {
        case Rotation::Clockwise90:
            return state
                .with(BlockStateProperties::WALL_HEIGHT_NORTH(), west)
                .with(BlockStateProperties::WALL_HEIGHT_EAST(), north)
                .with(BlockStateProperties::WALL_HEIGHT_SOUTH(), east)
                .with(BlockStateProperties::WALL_HEIGHT_WEST(), south);
        case Rotation::Clockwise180:
            return state
                .with(BlockStateProperties::WALL_HEIGHT_NORTH(), south)
                .with(BlockStateProperties::WALL_HEIGHT_EAST(), west)
                .with(BlockStateProperties::WALL_HEIGHT_SOUTH(), north)
                .with(BlockStateProperties::WALL_HEIGHT_WEST(), east);
        case Rotation::CounterClockwise90:
            return state
                .with(BlockStateProperties::WALL_HEIGHT_NORTH(), east)
                .with(BlockStateProperties::WALL_HEIGHT_EAST(), south)
                .with(BlockStateProperties::WALL_HEIGHT_SOUTH(), west)
                .with(BlockStateProperties::WALL_HEIGHT_WEST(), north);
        default:
            return state;
    }
}

const BlockState& WallBlock::mirror(const BlockState& state, Mirror mirror) const {
    if (mirror == Mirror::None) {
        return state;
    }

    BlockStateProperties::WallHeight north = state.get(BlockStateProperties::WALL_HEIGHT_NORTH());
    BlockStateProperties::WallHeight east = state.get(BlockStateProperties::WALL_HEIGHT_EAST());
    BlockStateProperties::WallHeight south = state.get(BlockStateProperties::WALL_HEIGHT_SOUTH());
    BlockStateProperties::WallHeight west = state.get(BlockStateProperties::WALL_HEIGHT_WEST());

    if (mirror == Mirror::LeftRight) {
        // 左右镜像（X轴）：east<->west
        return state
            .with(BlockStateProperties::WALL_HEIGHT_EAST(), west)
            .with(BlockStateProperties::WALL_HEIGHT_WEST(), east);
    } else if (mirror == Mirror::FrontBack) {
        // 前后镜像（Z轴）：north<->south
        return state
            .with(BlockStateProperties::WALL_HEIGHT_NORTH(), south)
            .with(BlockStateProperties::WALL_HEIGHT_SOUTH(), north);
    }

    return state;
}

// ========== 私有方法 ==========

BlockState WallBlock::calculateState(const IWorld& world, const BlockPos& pos, const BlockState& state) const {
    // 获取四个方向的邻居状态
    BlockPos northPos(pos.x, pos.y, pos.z - 1);
    BlockPos southPos(pos.x, pos.y, pos.z + 1);
    BlockPos eastPos(pos.x + 1, pos.y, pos.z);
    BlockPos westPos(pos.x - 1, pos.y, pos.z);

    const BlockState* northState = world.getBlockState(northPos.x, northPos.y, northPos.z);
    const BlockState* southState = world.getBlockState(southPos.x, southPos.y, southPos.z);
    const BlockState* eastState = world.getBlockState(eastPos.x, eastPos.y, eastPos.z);
    const BlockState* westState = world.getBlockState(westPos.x, westPos.y, westPos.z);

    // 计算各方向的连接高度
    BlockStateProperties::WallHeight northHeight = northState ? getWallHeight(*northState, Direction::North) : BlockStateProperties::WallHeight::None;
    BlockStateProperties::WallHeight southHeight = southState ? getWallHeight(*southState, Direction::South) : BlockStateProperties::WallHeight::None;
    BlockStateProperties::WallHeight eastHeight = eastState ? getWallHeight(*eastState, Direction::East) : BlockStateProperties::WallHeight::None;
    BlockStateProperties::WallHeight westHeight = westState ? getWallHeight(*westState, Direction::West) : BlockStateProperties::WallHeight::None;

    // 计算是否需要顶部
    bool hasUp = false;
    if (northHeight == BlockStateProperties::WallHeight::Tall ||
        southHeight == BlockStateProperties::WallHeight::Tall ||
        eastHeight == BlockStateProperties::WallHeight::Tall ||
        westHeight == BlockStateProperties::WallHeight::Tall) {
        // 有高连接时需要顶部
        hasUp = true;
    } else if (northHeight == BlockStateProperties::WallHeight::None &&
               southHeight == BlockStateProperties::WallHeight::None &&
               eastHeight == BlockStateProperties::WallHeight::None &&
               westHeight == BlockStateProperties::WallHeight::None) {
        // 无任何连接时需要顶部（独立柱子）
        hasUp = true;
    } else {
        // 检查是否所有连接都是低高度且没有交叉
        int lowCount = 0;
        if (northHeight == BlockStateProperties::WallHeight::Low) lowCount++;
        if (southHeight == BlockStateProperties::WallHeight::Low) lowCount++;
        if (eastHeight == BlockStateProperties::WallHeight::Low) lowCount++;
        if (westHeight == BlockStateProperties::WallHeight::Low) lowCount++;

        // 只有一个方向连接或形成角落时，需要顶部
        hasUp = (lowCount <= 2);
    }

    return state
        .with(BlockStateProperties::UP(), hasUp)
        .with(BlockStateProperties::WALL_HEIGHT_NORTH(), northHeight)
        .with(BlockStateProperties::WALL_HEIGHT_EAST(), eastHeight)
        .with(BlockStateProperties::WALL_HEIGHT_SOUTH(), southHeight)
        .with(BlockStateProperties::WALL_HEIGHT_WEST(), westHeight);
}

BlockStateProperties::WallHeight WallBlock::getWallHeight(
    const BlockState& state,
    Direction neighborSide) const {

    const Block& block = state.getBlock();

    // 检查是否为墙
    if (isWall(state)) {
        return BlockStateProperties::WallHeight::Tall;
    }

    // 检查是否为栅栏门
    if (isFenceGate(state)) {
        // 栅栏门只有在关闭时才连接
        if (state.hasProperty(BlockStateProperties::OPEN()) && !state.get(BlockStateProperties::OPEN())) {
            // 栅栏门朝向与检查方向垂直时连接
            Direction facing = state.get(BlockStateProperties::HORIZONTAL_FACING());
            if (Directions::getAxis(facing) != Directions::getAxis(neighborSide)) {
                return BlockStateProperties::WallHeight::Low;
            }
        }
        return BlockStateProperties::WallHeight::None;
    }

    // 检查是否为固体方块（石头、泥土等）
    if (state.isSolid()) {
        // 固体方块产生低连接
        return BlockStateProperties::WallHeight::Low;
    }

    // 检查是否为栅栏
    if (state.hasProperty(BlockStateProperties::NORTH())) {
        // 可能是栅栏
        Direction oppositeSide = Directions::opposite(neighborSide);
        const BooleanProperty* connProp = nullptr;

        switch (oppositeSide) {
            case Direction::North:
                connProp = &BlockStateProperties::NORTH();
                break;
            case Direction::South:
                connProp = &BlockStateProperties::SOUTH();
                break;
            case Direction::East:
                connProp = &BlockStateProperties::EAST();
                break;
            case Direction::West:
                connProp = &BlockStateProperties::WEST();
                break;
            default:
                break;
        }

        if (connProp != nullptr && state.get(*connProp)) {
            return BlockStateProperties::WallHeight::Low;
        }
    }

    return BlockStateProperties::WallHeight::None;
}

bool WallBlock::isWall(const BlockState& state) {
    return state.hasProperty(BlockStateProperties::WALL_HEIGHT_NORTH());
}

bool WallBlock::isFenceGate(const BlockState& state) {
    return state.hasProperty(BlockStateProperties::OPEN()) && state.hasProperty(BlockStateProperties::IN_WALL());
}

size_t WallBlock::getShapeIndex(
    bool up,
    BlockStateProperties::WallHeight north,
    BlockStateProperties::WallHeight east,
    BlockStateProperties::WallHeight south,
    BlockStateProperties::WallHeight west) {

    size_t idx = (up ? 1 : 0);
    idx += static_cast<size_t>(north) * 2;
    idx += static_cast<size_t>(east) * 6;
    idx += static_cast<size_t>(south) * 18;
    idx += static_cast<size_t>(west) * 54;
    return idx;
}

} // namespace blocks
} // namespace mc
