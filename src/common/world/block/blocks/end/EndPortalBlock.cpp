#include "EndPortalBlock.hpp"
#include "../../../IWorld.hpp"
#include "../../../../entity/entities/player/Player.hpp"
#include "../../../../item/context/BlockItemUseContext.hpp"
#include "../../../../util/Direction.hpp"
#include "../../../../util/math/random/Random.hpp"
#include "../../../../core/BlockRaycastResult.hpp"

namespace mc {
namespace blocks {

// ========== EndPortalBlock ==========

EndPortalBlock::EndPortalBlock(const BlockProperties& properties)
    : Block(properties) {
    // 传送门没有碰撞箱
    m_shape = CollisionShape::box(0.0f, 0.0f, 0.0f, 1.0f, 0.75f, 1.0f);
}

void EndPortalBlock::onEntityCollision(const BlockState& state, IWorld& world, const BlockPos& pos, Entity& entity) {
    MC_UNUSED(state);
    MC_UNUSED(world);
    MC_UNUSED(pos);
    MC_UNUSED(entity);
    // TODO: 传送到末地
}

const CollisionShape& EndPortalBlock::getShape(const BlockState& state) const {
    MC_UNUSED(state);
    return m_shape;
}

const CollisionShape& EndPortalBlock::getCollisionShape(const BlockState& state) const {
    MC_UNUSED(state);
    static CollisionShape emptyShape = CollisionShape::empty();
    return emptyShape;
}

// ========== EndPortalFrameBlock ==========

EndPortalFrameBlock::EndPortalFrameBlock(const BlockProperties& properties)
    : Block(properties) {

    // 创建状态容器
    auto container = StateContainer<Block, BlockState>::Builder(*this)
        .add(BlockStateProperties::EYE())
        .add(BlockStateProperties::HORIZONTAL_FACING())
        .create([this](const Block& block, std::unordered_map<const IProperty*, size_t> values, u32 id) {
            return std::make_unique<BlockState>(block, std::move(values), id);
        });
    createBlockState(std::move(container));

    // 设置默认状态
    setDefaultState(defaultState()
        .with(BlockStateProperties::EYE(), false)
        .with(BlockStateProperties::HORIZONTAL_FACING(), Direction::North));

    // 创建形状
    m_frameShape = CollisionShape::box(0.0f, 0.0f, 0.0f, 1.0f, 0.8125f, 1.0f);
    m_frameWithEyeShape = CollisionShape::box(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);
}

bool EndPortalFrameBlock::hasEye(const BlockState& state) const {
    return state.get(BlockStateProperties::EYE());
}

Direction EndPortalFrameBlock::getFacing(const BlockState& state) const {
    return state.get(BlockStateProperties::HORIZONTAL_FACING());
}

BlockState EndPortalFrameBlock::getStateForPlacement(BlockItemUseContext& context) {
    Direction facing = context.horizontalDirection();
    return defaultState().with(BlockStateProperties::HORIZONTAL_FACING(), facing);
}

const BlockState& EndPortalFrameBlock::rotate(const BlockState& state, Rotation rotation) const {
    Direction facing = state.get(BlockStateProperties::HORIZONTAL_FACING());
    Direction newFacing = Directions::rotateDirection(facing, rotation);
    return state.with(BlockStateProperties::HORIZONTAL_FACING(), newFacing);
}

const BlockState& EndPortalFrameBlock::mirror(const BlockState& state, Mirror mirror) const {
    Direction facing = state.get(BlockStateProperties::HORIZONTAL_FACING());
    Rotation rotation = Directions::mirrorToRotation(mirror, facing);
    Direction newFacing = Directions::rotateDirection(facing, rotation);
    return state.with(BlockStateProperties::HORIZONTAL_FACING(), newFacing);
}

const CollisionShape& EndPortalFrameBlock::getShape(const BlockState& state) const {
    return hasEye(state) ? m_frameWithEyeShape : m_frameShape;
}

// ========== EndGatewayBlock ==========

EndGatewayBlock::EndGatewayBlock(const BlockProperties& properties)
    : Block(properties) {
    m_shape = CollisionShape::box(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);
}

void EndGatewayBlock::onEntityCollision(const BlockState& state, IWorld& world, const BlockPos& pos, Entity& entity) {
    MC_UNUSED(state);
    MC_UNUSED(world);
    MC_UNUSED(pos);
    MC_UNUSED(entity);
    // TODO: 折跃门传送逻辑
}

const CollisionShape& EndGatewayBlock::getShape(const BlockState& state) const {
    MC_UNUSED(state);
    return m_shape;
}

const CollisionShape& EndGatewayBlock::getCollisionShape(const BlockState& state) const {
    MC_UNUSED(state);
    static CollisionShape emptyShape = CollisionShape::empty();
    return emptyShape;
}

// ========== ChorusPlantBlock ==========

ChorusPlantBlock::ChorusPlantBlock(const BlockProperties& properties)
    : Block(properties) {

    // 创建状态容器（6个方向的连接）
    auto container = StateContainer<Block, BlockState>::Builder(*this)
        .add(BlockStateProperties::NORTH())
        .add(BlockStateProperties::SOUTH())
        .add(BlockStateProperties::EAST())
        .add(BlockStateProperties::WEST())
        .add(BlockStateProperties::DOWN())
        .add(BlockStateProperties::UP())
        .create([this](const Block& block, std::unordered_map<const IProperty*, size_t> values, u32 id) {
            return std::make_unique<BlockState>(block, std::move(values), id);
        });
    createBlockState(std::move(container));

    // 设置默认状态（无连接）
    setDefaultState(defaultState()
        .with(BlockStateProperties::NORTH(), false)
        .with(BlockStateProperties::SOUTH(), false)
        .with(BlockStateProperties::EAST(), false)
        .with(BlockStateProperties::WEST(), false)
        .with(BlockStateProperties::DOWN(), false)
        .with(BlockStateProperties::UP(), false));

    // 创建形状
    m_centerShape = CollisionShape::box(0.25f, 0.0f, 0.25f, 0.75f, 1.0f, 0.75f);
    for (int i = 0; i < 6; ++i) {
        // 各方向的连接形状
        Direction dir = static_cast<Direction>(i);
        switch (dir) {
            case Direction::North:
                m_armShapes[i] = CollisionShape::box(0.25f, 0.25f, 0.0f, 0.75f, 0.75f, 0.25f);
                break;
            case Direction::South:
                m_armShapes[i] = CollisionShape::box(0.25f, 0.25f, 0.75f, 0.75f, 0.75f, 1.0f);
                break;
            case Direction::East:
                m_armShapes[i] = CollisionShape::box(0.75f, 0.25f, 0.25f, 1.0f, 0.75f, 0.75f);
                break;
            case Direction::West:
                m_armShapes[i] = CollisionShape::box(0.0f, 0.25f, 0.25f, 0.25f, 0.75f, 0.75f);
                break;
            case Direction::Up:
                m_armShapes[i] = CollisionShape::box(0.25f, 0.75f, 0.25f, 0.75f, 1.0f, 0.75f);
                break;
            case Direction::Down:
                m_armShapes[i] = CollisionShape::box(0.25f, 0.0f, 0.25f, 0.75f, 0.25f, 0.75f);
                break;
        }
    }
}

BlockState ChorusPlantBlock::getStateForPlacement(BlockItemUseContext& context) {
    const IWorld& world = context.getWorld();
    BlockPos pos = context.placementPos();

    // 计算各方向的连接
    const IBlockReader& blockReader = static_cast<const IBlockReader&>(world);
    bool north = canConnect(const_cast<IBlockReader&>(blockReader), pos, Direction::North);
    bool south = canConnect(const_cast<IBlockReader&>(blockReader), pos, Direction::South);
    bool east = canConnect(const_cast<IBlockReader&>(blockReader), pos, Direction::East);
    bool west = canConnect(const_cast<IBlockReader&>(blockReader), pos, Direction::West);
    bool up = canConnect(const_cast<IBlockReader&>(blockReader), pos, Direction::Up);
    bool down = canConnect(const_cast<IBlockReader&>(blockReader), pos, Direction::Down);

    return defaultState()
        .with(BlockStateProperties::NORTH(), north)
        .with(BlockStateProperties::SOUTH(), south)
        .with(BlockStateProperties::EAST(), east)
        .with(BlockStateProperties::WEST(), west)
        .with(BlockStateProperties::UP(), up)
        .with(BlockStateProperties::DOWN(), down);
}

bool ChorusPlantBlock::isValidPosition(
    const BlockState& state,
    IBlockReader& world,
    const BlockPos& pos) const {

    MC_UNUSED(state);

    // 检查是否有至少一个连接
    for (int i = 0; i < 6; ++i) {
        Direction dir = static_cast<Direction>(i);
        if (canConnect(world, pos, dir)) {
            return true;
        }
    }

    return false;
}

BlockState ChorusPlantBlock::updatePostPlacement(
    const BlockState& state,
    Direction facing,
    const BlockState& facingState,
    IWorld& world,
    const BlockPos& currentPos,
    const BlockPos& facingPos) {

    MC_UNUSED(facingPos);

    IBlockReader& blockReader = static_cast<IBlockReader&>(world);
    bool connected = canConnect(blockReader, currentPos, facing);

    switch (facing) {
        case Direction::North:
            return state.with(BlockStateProperties::NORTH(), connected);
        case Direction::South:
            return state.with(BlockStateProperties::SOUTH(), connected);
        case Direction::East:
            return state.with(BlockStateProperties::EAST(), connected);
        case Direction::West:
            return state.with(BlockStateProperties::WEST(), connected);
        case Direction::Up:
            return state.with(BlockStateProperties::UP(), connected);
        case Direction::Down:
            return state.with(BlockStateProperties::DOWN(), connected);
        default:
            return state;
    }
}

const CollisionShape& ChorusPlantBlock::getShape(const BlockState& state) const {
    MC_UNUSED(state);
    // TODO: 返回组合形状
    return m_centerShape;
}

bool ChorusPlantBlock::canConnect(IBlockReader& world, const BlockPos& pos, Direction direction) const {
    BlockPos adjPos = pos.offset(direction);
    const BlockState* adjState = world.getBlockState(adjPos.x, adjPos.y, adjPos.z);

    if (adjState == nullptr) {
        return false;
    }

    // 连接到紫颂植物或紫颂花
    return adjState->is(this);  // TODO: 也连接到紫颂花
}

// ========== ChorusFlowerBlock ==========

ChorusFlowerBlock::ChorusFlowerBlock(const BlockProperties& properties)
    : Block(properties) {

    // 创建状态容器
    auto container = StateContainer<Block, BlockState>::Builder(*this)
        .add(BlockStateProperties::AGE_0_5())
        .create([this](const Block& block, std::unordered_map<const IProperty*, size_t> values, u32 id) {
            return std::make_unique<BlockState>(block, std::move(values), id);
        });
    createBlockState(std::move(container));

    // 设置默认状态
    setDefaultState(defaultState().with(BlockStateProperties::AGE_0_5(), 0));

    // 创建各年龄形状
    for (int i = 0; i < 6; ++i) {
        m_shapesByAge[i] = CollisionShape::box(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);
    }
}

i32 ChorusFlowerBlock::getAge(const BlockState& state) const {
    return state.get(BlockStateProperties::AGE_0_5());
}

BlockState ChorusFlowerBlock::withAge(i32 age) const {
    return defaultState().with(BlockStateProperties::AGE_0_5(), std::min(age, 5));
}

BlockState ChorusFlowerBlock::getStateForPlacement(BlockItemUseContext& context) {
    return defaultState();
}

bool ChorusFlowerBlock::isValidPosition(
    const BlockState& state,
    IBlockReader& world,
    const BlockPos& pos) const {

    MC_UNUSED(state);

    // 检查下方
    BlockPos belowPos(pos.x, pos.y - 1, pos.z);
    const BlockState* belowState = world.getBlockState(belowPos.x, belowPos.y, belowPos.z);

    if (belowState == nullptr) {
        return false;
    }

    // 需要在紫颂植物上或末地石上
    // TODO: 检查特定方块
    return belowState->isSolid();
}

void ChorusFlowerBlock::randomTick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random) {
    i32 age = getAge(state);

    if (age < getMaxAge()) {
        // 随机生长
        if (random.nextInt(5) == 0) {
            world.setBlockState(pos.x, pos.y, pos.z, &withAge(age + 1), 2);
        }
    }
}

const CollisionShape& ChorusFlowerBlock::getShape(const BlockState& state) const {
    i32 age = getAge(state);
    return m_shapesByAge[std::min(age, 5)];
}

// ========== DragonEggBlock ==========

DragonEggBlock::DragonEggBlock(const BlockProperties& properties)
    : Block(properties) {

    // 龙蛋形状
    m_shape = CollisionShape::box(0.0625f, 0.0f, 0.0625f, 0.9375f, 1.0f, 0.9375f);
}

BlockState DragonEggBlock::getStateForPlacement(BlockItemUseContext& context) {
    return defaultState();
}

ActionResultType DragonEggBlock::onBlockActivated(
    const BlockState& state,
    IWorld& world,
    const BlockPos& pos,
    Player& player,
    Hand hand,
    const BlockRaycastResult& hit) {

    MC_UNUSED(player);
    MC_UNUSED(hand);
    MC_UNUSED(hit);

    // 点击传送
    teleport(world, pos, state);
    return ActionResultType::Success;
}

void DragonEggBlock::neighborChanged(
    IWorld& world,
    const BlockPos& pos,
    Block& neighborBlock,
    const BlockPos& neighborPos,
    bool isMoving) {

    MC_UNUSED(neighborBlock);
    MC_UNUSED(neighborPos);
    MC_UNUSED(isMoving);

    // 邻居变化时可能传送
    // teleport(world, pos, ...);
}

void DragonEggBlock::teleport(IWorld& world, const BlockPos& pos, const BlockState& state) {
    // TODO: 实现传送逻辑
    MC_UNUSED(world);
    MC_UNUSED(pos);
    MC_UNUSED(state);
}

const CollisionShape& DragonEggBlock::getShape(const BlockState& state) const {
    MC_UNUSED(state);
    return m_shape;
}

} // namespace blocks
} // namespace mc
