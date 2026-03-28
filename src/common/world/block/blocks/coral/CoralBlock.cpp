#include "CoralBlock.hpp"
#include "../../../IWorld.hpp"
#include "../../BlockRegistry.hpp"
#include "../../../../item/BlockItemUseContext.hpp"
#include "../../../../util/Direction.hpp"

namespace mc {
namespace blocks {

// ========== CoralBlock ==========

CoralBlock::CoralBlock(CoralColor color, u32 deadBlock, const BlockProperties& properties)
    : Block(properties)
    , m_color(color)
    , m_deadBlock(deadBlock) {

    // 创建状态容器
    auto container = StateContainer<Block, BlockState>::Builder(*this)
        .add(BlockStateProperties::WATERLOGGED())
        .create([this](const Block& block, std::unordered_map<const IProperty*, size_t> values, u32 id) {
            return std::make_unique<BlockState>(block, std::move(values), id);
        });
    createBlockState(std::move(container));

    // 设置默认状态
    setDefaultState(defaultState().with(BlockStateProperties::WATERLOGGED(), false));
}

BlockState CoralBlock::getStateForPlacement(BlockItemUseContext& context) {
    // 检查是否在水中
    const IWorld& world = context.getWorld();
    BlockPos pos = context.placementPos();

    // TODO: 检查流体状态
    bool waterlogged = false;

    return defaultState().with(BlockStateProperties::WATERLOGGED(), waterlogged);
}

BlockState CoralBlock::updatePostPlacement(
    const BlockState& state,
    Direction facing,
    const BlockState& facingState,
    IWorld& world,
    const BlockPos& currentPos,
    const BlockPos& facingPos) {

    MC_UNUSED(facing);
    MC_UNUSED(facingState);
    MC_UNUSED(facingPos);

    // 如果不在水中，变成死珊瑚
    if (!state.get(BlockStateProperties::WATERLOGGED()) && !isWaterNearby(world, currentPos)) {
        // TODO: 返回死珊瑚方块状态
        // Block* deadBlock = Block::getBlock(m_deadBlock);
        // return deadBlock->defaultState();
    }

    return state;
}

bool CoralBlock::isInWater(const BlockState& state) const {
    return state.get(BlockStateProperties::WATERLOGGED());
}

bool CoralBlock::isWaterNearby(IWorld& world, const BlockPos& pos) const {
    // 检查六个方向是否有水
    for (Direction dir : {Direction::North, Direction::South, Direction::East, Direction::West, Direction::Up, Direction::Down}) {
        BlockPos adjPos = pos.offset(dir);
        const BlockState* adjState = world.getBlockState(adjPos.x, adjPos.y, adjPos.z);

        if (adjState != nullptr) {
            const Material& material = adjState->getMaterial();
            if (material.isLiquid()) {
                return true;
            }
        }
    }
    return false;
}

const CollisionShape& CoralBlock::getShape(const BlockState& state) const {
    MC_UNUSED(state);
    static CollisionShape fullShape = CollisionShape::fullBlock();
    return fullShape;
}

// ========== CoralFanBlock ==========

CoralFanBlock::CoralFanBlock(CoralColor color, u32 deadBlock, const BlockProperties& properties)
    : Block(properties)
    , m_color(color)
    , m_deadBlock(deadBlock) {

    // 创建状态容器
    auto container = StateContainer<Block, BlockState>::Builder(*this)
        .add(BlockStateProperties::WATERLOGGED())
        .add(BlockStateProperties::HORIZONTAL_FACING())
        .create([this](const Block& block, std::unordered_map<const IProperty*, size_t> values, u32 id) {
            return std::make_unique<BlockState>(block, std::move(values), id);
        });
    createBlockState(std::move(container));

    // 设置默认状态
    setDefaultState(defaultState()
        .with(BlockStateProperties::WATERLOGGED(), false)
        .with(BlockStateProperties::HORIZONTAL_FACING(), Direction::North));
}

BlockState CoralFanBlock::getStateForPlacement(BlockItemUseContext& context) {
    Direction facing = context.horizontalDirection();
    const IWorld& world = context.getWorld();
    BlockPos pos = context.placementPos();

    bool waterlogged = false;  // TODO: 检查流体

    return defaultState()
        .with(BlockStateProperties::HORIZONTAL_FACING(), facing)
        .with(BlockStateProperties::WATERLOGGED(), waterlogged);
}

bool CoralFanBlock::isValidPosition(
    const BlockState& state,
    IBlockReader& world,
    const BlockPos& pos) const {

    Direction facing = state.get(BlockStateProperties::HORIZONTAL_FACING());
    return canAttachTo(world, pos, facing);
}

BlockState CoralFanBlock::updatePostPlacement(
    const BlockState& state,
    Direction facing,
    const BlockState& facingState,
    IWorld& world,
    const BlockPos& currentPos,
    const BlockPos& facingPos) {

    // 检查附着面是否仍然有效
    Direction attachDir = state.get(BlockStateProperties::HORIZONTAL_FACING());
    if (facing == attachDir) {
        IBlockReader& blockReader = static_cast<IBlockReader&>(world);
        if (!canAttachTo(blockReader, currentPos, attachDir)) {
            if (auto* airState = BlockRegistry::instance().airState()) {
                return *airState;
            }
        }
    }

    // 检查是否需要变成死珊瑚
    if (!state.get(BlockStateProperties::WATERLOGGED())) {
        bool waterNearby = false;
        for (Direction dir : {Direction::North, Direction::South, Direction::East, Direction::West, Direction::Up, Direction::Down}) {
            BlockPos adjPos = currentPos.offset(dir);
            const BlockState* adjState = world.getBlockState(adjPos.x, adjPos.y, adjPos.z);
            if (adjState != nullptr && adjState->getMaterial().isLiquid()) {
                waterNearby = true;
                break;
            }
        }
        if (!waterNearby) {
            // TODO: 返回死珊瑚扇状态
        }
    }

    return state;
}

const BlockState& CoralFanBlock::rotate(const BlockState& state, Rotation rotation) const {
    Direction facing = state.get(BlockStateProperties::HORIZONTAL_FACING());
    Direction newFacing = Directions::rotateDirection(facing, rotation);
    return state.with(BlockStateProperties::HORIZONTAL_FACING(), newFacing);
}

const BlockState& CoralFanBlock::mirror(const BlockState& state, Mirror mirror) const {
    Direction facing = state.get(BlockStateProperties::HORIZONTAL_FACING());
    Rotation rotation = Directions::mirrorToRotation(mirror, facing);
    Direction newFacing = Directions::rotateDirection(facing, rotation);
    return state.with(BlockStateProperties::HORIZONTAL_FACING(), newFacing);
}

const CollisionShape& CoralFanBlock::getShape(const BlockState& state) const {
    MC_UNUSED(state);
    // 珊瑚扇是薄层
    static CollisionShape shape = CollisionShape::box(0.0f, 0.0f, 0.0f, 1.0f, 0.0625f, 1.0f);
    return shape;
}

bool CoralFanBlock::canAttachTo(IBlockReader& world, const BlockPos& pos, Direction direction) const {
    BlockPos adjPos = pos.offset(direction);
    const BlockState* adjState = world.getBlockState(adjPos.x, adjPos.y, adjPos.z);

    if (adjState == nullptr) {
        return false;
    }

    return adjState->isSolid();
}

// ========== CoralWallFanBlock ==========

CoralWallFanBlock::CoralWallFanBlock(CoralColor color, u32 deadBlock, const BlockProperties& properties)
    : Block(properties)
    , m_color(color)
    , m_deadBlock(deadBlock) {

    // 创建状态容器
    auto container = StateContainer<Block, BlockState>::Builder(*this)
        .add(BlockStateProperties::WATERLOGGED())
        .add(BlockStateProperties::FACING())
        .create([this](const Block& block, std::unordered_map<const IProperty*, size_t> values, u32 id) {
            return std::make_unique<BlockState>(block, std::move(values), id);
        });
    createBlockState(std::move(container));

    // 设置默认状态
    setDefaultState(defaultState()
        .with(BlockStateProperties::WATERLOGGED(), false)
        .with(BlockStateProperties::FACING(), Direction::North));
}

BlockState CoralWallFanBlock::getStateForPlacement(BlockItemUseContext& context) {
    Direction facing = context.getClickedFace();
    const IWorld& world = context.getWorld();
    BlockPos pos = context.placementPos();

    bool waterlogged = false;  // TODO: 检查流体

    return defaultState()
        .with(BlockStateProperties::FACING(), facing)
        .with(BlockStateProperties::WATERLOGGED(), waterlogged);
}

bool CoralWallFanBlock::isValidPosition(
    const BlockState& state,
    IBlockReader& world,
    const BlockPos& pos) const {

    Direction facing = state.get(BlockStateProperties::FACING());
    // 只能附着在水平墙面
    if (facing == Direction::Up || facing == Direction::Down) {
        return false;
    }
    return canAttachTo(world, pos, facing);
}

BlockState CoralWallFanBlock::updatePostPlacement(
    const BlockState& state,
    Direction facing,
    const BlockState& facingState,
    IWorld& world,
    const BlockPos& currentPos,
    const BlockPos& facingPos) {

    MC_UNUSED(facingState);
    MC_UNUSED(facingPos);

    // 检查附着面
    Direction attachDir = state.get(BlockStateProperties::FACING());
    if (facing == attachDir) {
        IBlockReader& blockReader = static_cast<IBlockReader&>(world);
        if (!canAttachTo(blockReader, currentPos, attachDir)) {
            if (auto* airState = BlockRegistry::instance().airState()) {
                return *airState;
            }
        }
    }

    // 检查是否需要变成死珊瑚
    // ... (同 CoralFanBlock)

    return state;
}

const BlockState& CoralWallFanBlock::rotate(const BlockState& state, Rotation rotation) const {
    Direction facing = state.get(BlockStateProperties::FACING());
    Direction newFacing = Directions::rotateDirection(facing, rotation);
    return state.with(BlockStateProperties::FACING(), newFacing);
}

const BlockState& CoralWallFanBlock::mirror(const BlockState& state, Mirror mirror) const {
    Direction facing = state.get(BlockStateProperties::FACING());
    Rotation rotation = Directions::mirrorToRotation(mirror, facing);
    Direction newFacing = Directions::rotateDirection(facing, rotation);
    return state.with(BlockStateProperties::FACING(), newFacing);
}

const CollisionShape& CoralWallFanBlock::getShape(const BlockState& state) const {
    Direction facing = state.get(BlockStateProperties::FACING());

    // 根据朝向返回不同形状
    switch (facing) {
        case Direction::North: {
            static CollisionShape northShape = CollisionShape::box(0.0f, 0.0f, 0.9375f, 1.0f, 1.0f, 1.0f);
            return northShape;
        }
        case Direction::South: {
            static CollisionShape southShape = CollisionShape::box(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0625f);
            return southShape;
        }
        case Direction::East: {
            static CollisionShape eastShape = CollisionShape::box(0.0f, 0.0f, 0.0f, 0.0625f, 1.0f, 1.0f);
            return eastShape;
        }
        case Direction::West: {
            static CollisionShape westShape = CollisionShape::box(0.9375f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);
            return westShape;
        }
        default: {
            static CollisionShape emptyShape = CollisionShape::empty();
            return emptyShape;
        }
    }
}

bool CoralWallFanBlock::canAttachTo(IBlockReader& world, const BlockPos& pos, Direction direction) const {
    BlockPos adjPos = pos.offset(direction);
    const BlockState* adjState = world.getBlockState(adjPos.x, adjPos.y, adjPos.z);

    if (adjState == nullptr) {
        return false;
    }

    return adjState->isSolid();
}

// ========== CoralBlockBlock ==========

CoralBlockBlock::CoralBlockBlock(CoralColor color, const BlockProperties& properties)
    : Block(properties)
    , m_color(color) {
    // 珊瑚块没有状态属性
}

const CollisionShape& CoralBlockBlock::getShape(const BlockState& state) const {
    MC_UNUSED(state);
    static CollisionShape fullShape = CollisionShape::fullBlock();
    return fullShape;
}

} // namespace blocks
} // namespace mc
