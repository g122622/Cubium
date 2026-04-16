#include "GrindstoneBlock.hpp"
#include "../../../IWorld.hpp"
#include "../../../../item/context/BlockItemUseContext.hpp"
#include "../../../../util/Direction.hpp"
#include "../../../../util/assert/AssertAll.hpp"

namespace mc {
namespace blocks {

// ========== GrindstoneBlock 实现 ==========

GrindstoneBlock::GrindstoneBlock(const BlockProperties& properties)
    : Block(properties) {

    // 创建状态容器
    auto container = StateContainer<Block, BlockState>::Builder(*this)
        .add(BlockStateProperties::HORIZONTAL_FACING())
        .create([](const Block& block, std::unordered_map<const IProperty*, size_t> values, u32 id) {
            return std::make_unique<BlockState>(block, std::move(values), id);
        });
    createBlockState(std::move(container));

    // 设置默认状态
    setDefaultState(defaultState().with(BlockStateProperties::HORIZONTAL_FACING(), Direction::North));

    // 创建砂轮形状
    // 底座 + 侧柱 + 砂轮
    constexpr f32 P = 1.0f / 16.0f;

    // 简化形状：侧柱 + 砂轮
    CollisionShape postLeft = CollisionShape::box(0.0f, 0.0f, 0.0f, 2.0f * P, 14.0f * P, 2.0f * P);
    CollisionShape postRight = CollisionShape::box(14.0f * P, 0.0f, 0.0f, 16.0f * P, 14.0f * P, 2.0f * P);
    CollisionShape wheel = CollisionShape::box(2.0f * P, 4.0f * P, 0.0f, 14.0f * P, 12.0f * P, 2.0f * P);

    CollisionShape baseShape = CollisionShape::combine(CollisionShape::combine(postLeft, postRight), wheel);

    // 各朝向旋转
    // 北朝向
    m_shapesByFacing[static_cast<size_t>(Direction::North)] = baseShape;

    // 南朝向
    m_shapesByFacing[static_cast<size_t>(Direction::South)] = baseShape;

    // 西朝向 - 旋转90度
    CollisionShape postLeftW = CollisionShape::box(0.0f, 0.0f, 0.0f, 2.0f * P, 14.0f * P, 2.0f * P);
    CollisionShape postRightW = CollisionShape::box(0.0f, 0.0f, 14.0f * P, 2.0f * P, 14.0f * P, 16.0f * P);
    CollisionShape wheelW = CollisionShape::box(0.0f, 4.0f * P, 2.0f * P, 2.0f * P, 12.0f * P, 14.0f * P);
    m_shapesByFacing[static_cast<size_t>(Direction::West)] = CollisionShape::combine(
        CollisionShape::combine(postLeftW, postRightW), wheelW);

    // 东朝向
    CollisionShape postLeftE = CollisionShape::box(14.0f * P, 0.0f, 0.0f, 16.0f * P, 14.0f * P, 2.0f * P);
    CollisionShape postRightE = CollisionShape::box(14.0f * P, 0.0f, 14.0f * P, 16.0f * P, 14.0f * P, 16.0f * P);
    CollisionShape wheelE = CollisionShape::box(14.0f * P, 4.0f * P, 2.0f * P, 16.0f * P, 12.0f * P, 14.0f * P);
    m_shapesByFacing[static_cast<size_t>(Direction::East)] = CollisionShape::combine(
        CollisionShape::combine(postLeftE, postRightE), wheelE);

    m_collisionShape = baseShape;
}

BlockState GrindstoneBlock::getStateForPlacement(BlockItemUseContext& context) {
    Direction facing = context.horizontalDirection();
    return defaultState().with(BlockStateProperties::HORIZONTAL_FACING(), Directions::opposite(facing));
}

bool GrindstoneBlock::isValidPosition(
    const BlockState& state,
    IBlockReader& world,
    const BlockPos& pos) const {

    MC_UNUSED(state);

    // 砂轮需要附着在墙上
    // 检查后方是否有支撑
    Direction facing = state.get(BlockStateProperties::HORIZONTAL_FACING());
    BlockPos behindPos(pos.x + Directions::xOffset(facing), pos.y, pos.z + Directions::zOffset(facing));
    const BlockState* behindState = world.getBlockState(behindPos);

    if (behindState == nullptr) {
        return false;
    }

    return behindState->isSolid();
}

BlockState GrindstoneBlock::updatePostPlacement(
    const BlockState& state,
    Direction facing,
    const BlockState& facingState,
    IWorld& world,
    const BlockPos& currentPos,
    const BlockPos& facingPos) {

    Direction grindstoneFacing = state.get(BlockStateProperties::HORIZONTAL_FACING());

    // 检查附着的墙是否还存在
    if (facing == grindstoneFacing) {
        if (!facingState.isSolid()) {
            // 墙被移除，掉落
            // TODO: 掉落物品
            return world.getBlockState(currentPos)->getBlock().defaultState();
        }
    }

    return state;
}

const BlockState& GrindstoneBlock::rotate(const BlockState& state, Rotation rotation) const {
    Direction facing = state.get(BlockStateProperties::HORIZONTAL_FACING());
    Direction rotated = Directions::rotateDirection(facing, rotation);
    return state.with(BlockStateProperties::HORIZONTAL_FACING(), rotated);
}

const BlockState& GrindstoneBlock::mirror(const BlockState& state, Mirror mirror) const {
    if (mirror == Mirror::None) {
        return state;
    }

    Direction facing = state.get(BlockStateProperties::HORIZONTAL_FACING());
    Rotation rotation = Directions::mirrorToRotation(mirror, facing);
    return rotate(state, rotation);
}

const CollisionShape& GrindstoneBlock::getShape(const BlockState& state) const {
    Direction facing = state.get(BlockStateProperties::HORIZONTAL_FACING());
    size_t index = static_cast<size_t>(facing);
    MC_ASSERT(index < Directions::COUNT && Directions::isHorizontal(facing));
    return m_shapesByFacing[index];
}

const CollisionShape& GrindstoneBlock::getCollisionShape(const BlockState& state) const {
    MC_UNUSED(state);
    return m_collisionShape;
}

} // namespace blocks
} // namespace mc
