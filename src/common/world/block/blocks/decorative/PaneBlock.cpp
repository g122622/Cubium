#include "PaneBlock.hpp"
#include "../../../../item/BlockItemUseContext.hpp"
#include "../../../../util/Direction.hpp"
#include "../../../../util/assert/AssertAll.hpp"

namespace mc {
namespace blocks {

PaneBlock::PaneBlock(const BlockProperties& properties)
    : Block(properties)
    , m_collisionShape(CollisionShape::empty())
{
    // 创建状态容器
    auto container = StateContainer<Block, BlockState>::Builder(*this)
        .add(BlockStateProperties::NORTH())
        .add(BlockStateProperties::EAST())
        .add(BlockStateProperties::SOUTH())
        .add(BlockStateProperties::WEST())
        .add(BlockStateProperties::WATERLOGGED())
        .create([this](const Block& block, std::unordered_map<const IProperty*, size_t> values, u32 id) {
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

    // 创建形状
    // 中心柱：2x16x2 (像素)
    m_centerShape = CollisionShape::box(7.0f, 0.0f, 7.0f, 9.0f, 16.0f, 9.0f);

    // 边缘形状 (各方向连接面板)
    m_sideShapes[static_cast<size_t>(Direction::North)] = CollisionShape::box(7.0f, 0.0f, 0.0f, 9.0f, 16.0f, 7.0f);
    m_sideShapes[static_cast<size_t>(Direction::South)] = CollisionShape::box(7.0f, 0.0f, 9.0f, 9.0f, 16.0f, 16.0f);
    m_sideShapes[static_cast<size_t>(Direction::West)] = CollisionShape::box(0.0f, 0.0f, 7.0f, 7.0f, 16.0f, 9.0f);
    m_sideShapes[static_cast<size_t>(Direction::East)] = CollisionShape::box(9.0f, 0.0f, 7.0f, 16.0f, 16.0f, 9.0f);

    // TODO: 碰撞形状应该根据连接状态动态组合
    // 目前使用中心形状作为基础碰撞形状
    m_collisionShape = m_centerShape;
}

BlockState PaneBlock::getStateForPlacement(BlockItemUseContext& context) {
    // 获取相邻方块状态
    BlockPos pos = context.placementPos();
    const IWorld& world = context.getWorld();

    const BlockState* northState = world.getBlockState(pos.x, pos.y, pos.z - 1);
    const BlockState* eastState = world.getBlockState(pos.x + 1, pos.y, pos.z);
    const BlockState* southState = world.getBlockState(pos.x, pos.y, pos.z + 1);
    const BlockState* westState = world.getBlockState(pos.x - 1, pos.y, pos.z);

    bool north = northState && shouldConnectTo(const_cast<IWorld&>(world), pos, *northState, Direction::North);
    bool east = eastState && shouldConnectTo(const_cast<IWorld&>(world), pos, *eastState, Direction::East);
    bool south = southState && shouldConnectTo(const_cast<IWorld&>(world), pos, *southState, Direction::South);
    bool west = westState && shouldConnectTo(const_cast<IWorld&>(world), pos, *westState, Direction::West);

    // 检查水logged
    bool waterlogged = false; // TODO: 检查流体状态

    return defaultState()
        .with(BlockStateProperties::NORTH(), north)
        .with(BlockStateProperties::EAST(), east)
        .with(BlockStateProperties::SOUTH(), south)
        .with(BlockStateProperties::WEST(), west)
        .with(BlockStateProperties::WATERLOGGED(), waterlogged);
}

BlockState PaneBlock::updatePostPlacement(
    const BlockState& state,
    Direction facing,
    const BlockState& facingState,
    IWorld& world,
    const BlockPos& currentPos,
    const BlockPos& facingPos)
{
    MC_UNUSED(facingPos);

    // 更新连接状态
    if (facing == Direction::North) {
        return state.with(BlockStateProperties::NORTH(), shouldConnectTo(world, currentPos, facingState, facing));
    } else if (facing == Direction::South) {
        return state.with(BlockStateProperties::SOUTH(), shouldConnectTo(world, currentPos, facingState, facing));
    } else if (facing == Direction::West) {
        return state.with(BlockStateProperties::WEST(), shouldConnectTo(world, currentPos, facingState, facing));
    } else if (facing == Direction::East) {
        return state.with(BlockStateProperties::EAST(), shouldConnectTo(world, currentPos, facingState, facing));
    }
    return state;
}

const CollisionShape& PaneBlock::getCollisionShape(const BlockState& state) const {
    MC_UNUSED(state);
    // 碰撞形状由各部分组合
    return m_collisionShape;
}

const CollisionShape& PaneBlock::getShape(const BlockState& state) const {
    MC_UNUSED(state);
    // 返回中心形状（实际渲染需要组合形状）
    return m_centerShape;
}

bool PaneBlock::connectsTo(const BlockState& state, Direction facing) {
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

bool PaneBlock::shouldConnectTo(IWorld& world, const BlockPos& pos,
                                 const BlockState& neighborState, Direction direction) const {
    MC_UNUSED(world);
    MC_UNUSED(pos);
    MC_UNUSED(direction);

    // 检查邻居是否是固体方块或同样类型的方块
    const Block& neighborBlock = neighborState.getBlock();

    // 连接到固体方块
    if (neighborBlock.isSolid(neighborState)) {
        return true;
    }

    // 连接到同类型的方块
    if (&neighborBlock == this) {
        return true;
    }

    // TODO: 检查其他可连接的方块类型（如玻璃板、铁栏杆等）

    return false;
}

} // namespace blocks
} // namespace mc
