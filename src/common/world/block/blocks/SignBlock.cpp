#include "SignBlock.hpp"
#include "../IWaterLoggable.hpp"
#include "../WaterLoggableHelpers.hpp"
#include "../../IWorld.hpp"
#include "../../blockentity/core/BlockEntityRegistry.hpp"
#include "../../blockentity/BlockEntityType.hpp"
#include "../../blockentity/interactive/SignEntity.hpp"
#include "../../../item/context/BlockItemUseContext.hpp"
#include "../../../util/Direction.hpp"
#include "../../../util/math/MathUtils.hpp"
#include "../../../util/assert/AssertAll.hpp"
#include "../../../entity/entities/player/Player.hpp"
#include "../../../core/Types.hpp"
#include <cmath>

namespace mc {
namespace blocks {

// ========== AbstractSignBlock ==========

AbstractSignBlock::AbstractSignBlock(const BlockProperties& properties, WoodType woodType)
    : Block(properties)
    , m_woodType(woodType) {
    // 子类负责创建状态容器
}

BlockState AbstractSignBlock::updatePostPlacement(
    const BlockState& state,
    Direction facing,
    const BlockState& facingState,
    IWorld& world,
    const BlockPos& currentPos,
    const BlockPos& facingPos) {

    MC_UNUSED(facingState);
    MC_UNUSED(facingPos);

    if (state.get(BlockStateProperties::WATERLOGGED())) {
        waterloggable::scheduleWaterTick(world, currentPos);
    }

    return state;
}

std::unique_ptr<BlockEntity> AbstractSignBlock::createBlockEntity(const BlockPos& pos) {
    return blockentity::BlockEntityRegistry::instance().create(BlockEntityType::Sign, pos);
}

ActionResultType AbstractSignBlock::onBlockActivated(
    const BlockState& state,
    IWorld& world,
    const BlockPos& pos,
    Player& player,
    Hand hand,
    const BlockRaycastResult& hit) {

    MC_UNUSED(state);
    MC_UNUSED(hand);
    MC_UNUSED(hit);

    // MC 1.16.5: 参考 SignBlock.onBlockActivated()
    // 当玩家右键点击告示牌时，执行告示牌上的命令
    BlockEntity* blockEntity = world.getBlockEntity(pos);
    if (blockEntity && blockEntity->getType() == BlockEntityType::Sign) {
        auto* signEntity = static_cast<blockentity::SignEntity*>(blockEntity);
        // 执行告示牌上的命令
        signEntity->executeCommand(world, player);
        return ActionResultType::Success;
    }

    return ActionResultType::Pass;
}

const fluid::FluidState* AbstractSignBlock::getFluidState(const BlockState& state) const {
    const fluid::FluidState* waterState = waterloggable::getWaterFluidState(state);
    return waterState != nullptr ? waterState : Block::getFluidState(state);
}

// ========== StandingSignBlock ==========

StandingSignBlock::StandingSignBlock(const BlockProperties& properties, WoodType woodType)
    : AbstractSignBlock(properties, woodType)
    , m_shape(CollisionShape::box(0.25f, 0.0f, 0.25f, 0.75f, 1.0f, 0.75f)) {

    auto container = StateContainer<Block, BlockState>::Builder(*this)
        .add(BlockStateProperties::ROTATION_0_15())
        .add(BlockStateProperties::WATERLOGGED())
        .create([this](const Block& block, std::unordered_map<const IProperty*, size_t> values, u32 id) {
            return std::make_unique<BlockState>(block, std::move(values), id);
        });
    createBlockState(std::move(container));

    setDefaultState(defaultState()
        .with(BlockStateProperties::ROTATION_0_15(), 0)
        .with(BlockStateProperties::WATERLOGGED(), false));
}

BlockState StandingSignBlock::getStateForPlacement(BlockItemUseContext& context) {
    const IWorld& world = context.getWorld();
    BlockPos pos = context.placementPos();

    // 根据玩家朝向计算旋转（16个方向）
    f32 yaw = context.getPlayerYaw();
    i32 rotation = static_cast<i32>(std::floor((180.0f + yaw) * 16.0f / 360.0f + 0.5f)) & 15;

    bool waterlogged = waterloggable::shouldWaterlogAt(world, pos);

    return defaultState()
        .with(BlockStateProperties::ROTATION_0_15(), rotation)
        .with(BlockStateProperties::WATERLOGGED(), waterlogged);
}

bool StandingSignBlock::isValidPosition(
    const BlockState& state,
    IBlockReader& world,
    const BlockPos& pos) const {

    MC_UNUSED(state);

    // 需要下方有固体支撑
    BlockPos belowPos(pos.x, pos.y - 1, pos.z);
    const BlockState* belowState = world.getBlockState(belowPos);

    return belowState != nullptr && belowState->isSolidSide(world, belowPos, Direction::Up);
}

const BlockState& StandingSignBlock::rotate(const BlockState& state, Rotation rotation) const {
    i32 currentRotation = state.get(BlockStateProperties::ROTATION_0_15());
    i32 newRotation = Directions::rotateRotation(currentRotation, rotation, 16);
    return state.with(BlockStateProperties::ROTATION_0_15(), newRotation);
}

const BlockState& StandingSignBlock::mirror(const BlockState& state, Mirror mirror) const {
    i32 currentRotation = state.get(BlockStateProperties::ROTATION_0_15());
    i32 newRotation = Directions::mirrorRotation(currentRotation, mirror, 16);
    return state.with(BlockStateProperties::ROTATION_0_15(), newRotation);
}

const CollisionShape& StandingSignBlock::getShape(const BlockState& state) const {
    MC_UNUSED(state);
    return m_shape;
}

// ========== WallSignBlock ==========

WallSignBlock::WallSignBlock(const BlockProperties& properties, WoodType woodType)
    : AbstractSignBlock(properties, woodType) {

    // 各方向的碰撞形状（贴在墙面的薄板）
    m_shapesByDirection[Direction::North] = CollisionShape::box(0.0f, 0.28125f, 0.875f, 1.0f, 0.78125f, 1.0f);
    m_shapesByDirection[Direction::South] = CollisionShape::box(0.0f, 0.28125f, 0.0f, 1.0f, 0.78125f, 0.125f);
    m_shapesByDirection[Direction::East] = CollisionShape::box(0.0f, 0.28125f, 0.0f, 0.125f, 0.78125f, 1.0f);
    m_shapesByDirection[Direction::West] = CollisionShape::box(0.875f, 0.28125f, 0.0f, 1.0f, 0.78125f, 1.0f);

    auto container = StateContainer<Block, BlockState>::Builder(*this)
        .add(BlockStateProperties::FACING())
        .add(BlockStateProperties::WATERLOGGED())
        .create([this](const Block& block, std::unordered_map<const IProperty*, size_t> values, u32 id) {
            return std::make_unique<BlockState>(block, std::move(values), id);
        });
    createBlockState(std::move(container));

    setDefaultState(defaultState()
        .with(BlockStateProperties::FACING(), Direction::North)
        .with(BlockStateProperties::WATERLOGGED(), false));
}

BlockState WallSignBlock::getStateForPlacement(BlockItemUseContext& context) {
    const IWorld& world = context.getWorld();
    BlockPos pos = context.placementPos();
    const fluid::FluidState* fluidState = world.getFluidState(pos);
    bool waterlogged = waterloggable::isWaterFluidState(fluidState);

    // 尝试找到可以附着的墙面
    for (Direction dir : context.getNearestLookingDirections()) {
        if (Directions::isHorizontal(dir)) {
            Direction facing = Directions::opposite(dir);
            BlockState state = defaultState()
                .with(BlockStateProperties::FACING(), facing)
                .with(BlockStateProperties::WATERLOGGED(), waterlogged);

            // 使用 IBlockReader 接口检查是否可放置
            IBlockReader& blockReader = const_cast<IBlockReader&>(static_cast<const IBlockReader&>(world));
            if (isValidPosition(state, blockReader, pos)) {
                return state;
            }
        }
    }

    // 无法放置，返回空状态（会让放置失败）
    return defaultState();
}

bool WallSignBlock::isValidPosition(
    const BlockState& state,
    IBlockReader& world,
    const BlockPos& pos) const {

    Direction facing = state.get(BlockStateProperties::FACING());
    Direction oppositeDir = Directions::opposite(facing);
    BlockPos adjPos = pos.offset(oppositeDir);
    const BlockState* adjState = world.getBlockState(adjPos);

    return adjState != nullptr && adjState->isSolidSide(world, adjPos, facing);
}

const BlockState& WallSignBlock::rotate(const BlockState& state, Rotation rotation) const {
    Direction facing = state.get(BlockStateProperties::FACING());
    Direction newFacing = Directions::rotateDirection(facing, rotation);
    return state.with(BlockStateProperties::FACING(), newFacing);
}

const BlockState& WallSignBlock::mirror(const BlockState& state, Mirror mirror) const {
    Direction facing = state.get(BlockStateProperties::FACING());
    Rotation rot = Directions::mirrorToRotation(mirror, facing);
    Direction newFacing = Directions::rotateDirection(facing, rot);
    return state.with(BlockStateProperties::FACING(), newFacing);
}

const CollisionShape& WallSignBlock::getShape(const BlockState& state) const {
    Direction facing = state.get(BlockStateProperties::FACING());
    auto it = m_shapesByDirection.find(facing);
    if (it != m_shapesByDirection.end()) {
        return it->second;
    }
    static CollisionShape emptyShape = CollisionShape::empty();
    return emptyShape;
}

} // namespace blocks
} // namespace mc
