#include "VineBlock.hpp"
#include "../../../IWorld.hpp"
#include "../../BlockRegistry.hpp"
#include "../../../../item/BlockItemUseContext.hpp"
#include "../../../../util/Direction.hpp"
#include "../../../../util/math/random/Random.hpp"

namespace mc {
namespace blocks {

VineBlock::VineBlock(const BlockProperties& properties)
    : Block(properties) {

    // 创建状态容器
    auto container = StateContainer<Block, BlockState>::Builder(*this)
        .add(BlockStateProperties::UP())
        .add(BlockStateProperties::NORTH())
        .add(BlockStateProperties::SOUTH())
        .add(BlockStateProperties::EAST())
        .add(BlockStateProperties::WEST())
        .create([this](const Block& block, std::unordered_map<const IProperty*, size_t> values, u32 id) {
            return std::make_unique<BlockState>(block, std::move(values), id);
        });
    createBlockState(std::move(container));

    // 设置默认状态：无连接
    setDefaultState(defaultState()
        .with(BlockStateProperties::UP(), false)
        .with(BlockStateProperties::NORTH(), false)
        .with(BlockStateProperties::SOUTH(), false)
        .with(BlockStateProperties::EAST(), false)
        .with(BlockStateProperties::WEST(), false));

    // 创建各方向的形状（薄层）
    constexpr f32 thickness = 0.0625f;  // 1/16 块厚度
    m_northShape = CollisionShape::box(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, thickness);
    m_southShape = CollisionShape::box(0.0f, 0.0f, 1.0f - thickness, 1.0f, 1.0f, 1.0f);
    m_eastShape = CollisionShape::box(1.0f - thickness, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);
    m_westShape = CollisionShape::box(0.0f, 0.0f, 0.0f, thickness, 1.0f, 1.0f);
}

BlockState VineBlock::getStateForPlacement(BlockItemUseContext& context) {
    // 根据点击的面确定初始连接方向
    Direction clickedFace = context.getClickedFace();
    const IWorld& world = context.getWorld();
    BlockPos pos = context.placementPos();

    // 检查是否可以附着
    if (!canAttachTo(const_cast<IBlockReader&>(static_cast<const IBlockReader&>(world)), pos, clickedFace)) {
        return defaultState();
    }

    // 设置对应方向的连接
    bool north = (clickedFace == Direction::North);
    bool south = (clickedFace == Direction::South);
    bool east = (clickedFace == Direction::East);
    bool west = (clickedFace == Direction::West);
    bool up = (clickedFace == Direction::Up);

    return defaultState()
        .with(BlockStateProperties::UP(), up)
        .with(BlockStateProperties::NORTH(), north)
        .with(BlockStateProperties::SOUTH(), south)
        .with(BlockStateProperties::EAST(), east)
        .with(BlockStateProperties::WEST(), west);
}

bool VineBlock::isValidPosition(
    const BlockState& state,
    IBlockReader& world,
    const BlockPos& pos) const {

    // 检查至少有一个方向可以附着
    if (state.get(BlockStateProperties::UP()) && canAttachTo(world, pos, Direction::Up)) {
        return true;
    }
    if (state.get(BlockStateProperties::NORTH()) && canAttachTo(world, pos, Direction::North)) {
        return true;
    }
    if (state.get(BlockStateProperties::SOUTH()) && canAttachTo(world, pos, Direction::South)) {
        return true;
    }
    if (state.get(BlockStateProperties::EAST()) && canAttachTo(world, pos, Direction::East)) {
        return true;
    }
    if (state.get(BlockStateProperties::WEST()) && canAttachTo(world, pos, Direction::West)) {
        return true;
    }

    // 检查下方是否有藤蔓（可以向下延伸）
    BlockPos belowPos(pos.x, pos.y - 1, pos.z);
    const BlockState* belowState = world.getBlockState(belowPos.x, belowPos.y, belowPos.z);
    return belowState != nullptr && belowState->is(this);
}

BlockState VineBlock::updatePostPlacement(
    const BlockState& state,
    Direction facing,
    const BlockState& facingState,
    IWorld& world,
    const BlockPos& currentPos,
    const BlockPos& facingPos) {

    MC_UNUSED(facingState);
    MC_UNUSED(facingPos);

    // 更新对应方向的连接状态
    const BooleanProperty* prop = nullptr;

    switch (facing) {
        case Direction::Up:
            prop = &BlockStateProperties::UP();
            break;
        case Direction::North:
            prop = &BlockStateProperties::NORTH();
            break;
        case Direction::South:
            prop = &BlockStateProperties::SOUTH();
            break;
        case Direction::East:
            prop = &BlockStateProperties::EAST();
            break;
        case Direction::West:
            prop = &BlockStateProperties::WEST();
            break;
        default:
            return state;
    }

    bool currentState = state.get(*prop);
    if (currentState) {
        // 检查是否仍然可以附着
        IBlockReader& blockReader = static_cast<IBlockReader&>(world);
        if (!canAttachTo(blockReader, currentPos, facing)) {
            // 移除该方向的连接
            BlockState newState = state.with(*prop, false);
            // 如果没有任何连接，检查是否可以保持
            if (getConnectionCount(newState) == 0 && !isValidPosition(newState, blockReader, currentPos)) {
                if (auto* airState = BlockRegistry::instance().airState()) {
                    return *airState;
                }
            }
            return newState;
        }
    }

    return state;
}

const BlockState& VineBlock::rotate(const BlockState& state, Rotation rotation) const {
    bool north = state.get(BlockStateProperties::NORTH());
    bool south = state.get(BlockStateProperties::SOUTH());
    bool east = state.get(BlockStateProperties::EAST());
    bool west = state.get(BlockStateProperties::WEST());

    switch (rotation) {
        case Rotation::None:
            return state;
        case Rotation::Clockwise90:
            return state
                .with(BlockStateProperties::NORTH(), west)
                .with(BlockStateProperties::SOUTH(), east)
                .with(BlockStateProperties::EAST(), north)
                .with(BlockStateProperties::WEST(), south);
        case Rotation::Clockwise180:
            return state
                .with(BlockStateProperties::NORTH(), south)
                .with(BlockStateProperties::SOUTH(), north)
                .with(BlockStateProperties::EAST(), west)
                .with(BlockStateProperties::WEST(), east);
        case Rotation::CounterClockwise90:
            return state
                .with(BlockStateProperties::NORTH(), east)
                .with(BlockStateProperties::SOUTH(), west)
                .with(BlockStateProperties::EAST(), south)
                .with(BlockStateProperties::WEST(), north);
        default:
            return state;
    }
}

const BlockState& VineBlock::mirror(const BlockState& state, Mirror mirror) const {
    switch (mirror) {
        case Mirror::None:
            return state;
        case Mirror::LeftRight: {
            bool north = state.get(BlockStateProperties::NORTH());
            bool south = state.get(BlockStateProperties::SOUTH());
            return state
                .with(BlockStateProperties::NORTH(), south)
                .with(BlockStateProperties::SOUTH(), north);
        }
        case Mirror::FrontBack: {
            bool east = state.get(BlockStateProperties::EAST());
            bool west = state.get(BlockStateProperties::WEST());
            return state
                .with(BlockStateProperties::EAST(), west)
                .with(BlockStateProperties::WEST(), east);
        }
        default:
            return state;
    }
}

const CollisionShape& VineBlock::getShape(const BlockState& state) const {
    MC_UNUSED(state);
    // 返回空形状（藤蔓是薄层，没有碰撞）
    static CollisionShape emptyShape = CollisionShape::empty();
    return emptyShape;
}

const CollisionShape& VineBlock::getCollisionShape(const BlockState& state) const {
    MC_UNUSED(state);
    static CollisionShape emptyShape = CollisionShape::empty();
    return emptyShape;
}

void VineBlock::randomTick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random) {
    // TODO: 实现藤蔓生长逻辑
    // 1. 向下延伸
    // 2. 向侧面蔓延
    MC_UNUSED(world);
    MC_UNUSED(pos);
    MC_UNUSED(state);
    MC_UNUSED(random);
}

bool VineBlock::canAttachTo(IBlockReader& world, const BlockPos& pos, Direction direction) const {
    BlockPos adjPos = pos.offset(direction);
    const BlockState* adjState = world.getBlockState(adjPos.x, adjPos.y, adjPos.z);

    if (adjState == nullptr) {
        return false;
    }

    // 藤蔓可以附着在固体方块的侧面
    return adjState->isSolid();
}

i32 VineBlock::getConnectionCount(const BlockState& state) const {
    i32 count = 0;
    if (state.get(BlockStateProperties::UP())) count++;
    if (state.get(BlockStateProperties::NORTH())) count++;
    if (state.get(BlockStateProperties::SOUTH())) count++;
    if (state.get(BlockStateProperties::EAST())) count++;
    if (state.get(BlockStateProperties::WEST())) count++;
    return count;
}

} // namespace blocks
} // namespace mc
