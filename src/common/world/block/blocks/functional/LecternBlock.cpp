#include "LecternBlock.hpp"
#include "../../../IWorld.hpp"
#include "../../../../item/context/BlockItemUseContext.hpp"
#include "../../../../util/Direction.hpp"
#include "../../../../util/assert/AssertAll.hpp"

namespace mc {
namespace blocks {

// ========== LecternBlock 实现 ==========

LecternBlock::LecternBlock(const BlockProperties& properties)
    : Block(properties) {

    // 创建状态容器
    auto container = StateContainer<Block, BlockState>::Builder(*this)
        .add(BlockStateProperties::HORIZONTAL_FACING())
        .add(BlockStateProperties::POWERED())
        .add(BlockStateProperties::HAS_BOOK())
        .create([](const Block& block, std::unordered_map<const IProperty*, size_t> values, u32 id) {
            return std::make_unique<BlockState>(block, std::move(values), id);
        });
    createBlockState(std::move(container));

    // 设置默认状态
    setDefaultState(defaultState()
        .with(BlockStateProperties::HORIZONTAL_FACING(), Direction::North)
        .with(BlockStateProperties::POWERED(), false)
        .with(BlockStateProperties::HAS_BOOK(), false));

    // 创建讲台形状
    // 底座 + 柱子 + 顶部平台 + 书架斜面
    constexpr f32 P = 1.0f / 16.0f;

    CollisionShape base = CollisionShape::box(0.0f, 0.0f, 0.0f, 16.0f * P, 2.0f * P, 16.0f * P);
    CollisionShape post = CollisionShape::box(4.0f * P, 2.0f * P, 4.0f * P, 12.0f * P, 14.0f * P, 12.0f * P);
    CollisionShape top = CollisionShape::box(0.0f, 15.0f * P, 0.0f, 16.0f * P, 15.0f * P, 16.0f * P);

    m_collisionShape = CollisionShape::combine(CollisionShape::combine(base, post), top);

    // 各朝向的斜面形状（简化为矩形）
    // 北朝向
    CollisionShape slopeN = CollisionShape::box(1.0f * P, 10.0f * P, 0.0f, 14.0f * P, 18.0f * P, 16.0f * P);
    m_shapesByFacing[static_cast<size_t>(Direction::North)] = CollisionShape::combine(m_collisionShape, slopeN);

    // 南朝向
    CollisionShape slopeS = CollisionShape::box(1.0f * P, 10.0f * P, 0.0f, 14.0f * P, 18.0f * P, 16.0f * P);
    m_shapesByFacing[static_cast<size_t>(Direction::South)] = CollisionShape::combine(m_collisionShape, slopeS);

    // 西朝向
    CollisionShape slopeW = CollisionShape::box(0.0f, 10.0f * P, 1.0f * P, 16.0f * P, 18.0f * P, 14.0f * P);
    m_shapesByFacing[static_cast<size_t>(Direction::West)] = CollisionShape::combine(m_collisionShape, slopeW);

    // 东朝向
    CollisionShape slopeE = CollisionShape::box(0.0f, 10.0f * P, 1.0f * P, 16.0f * P, 18.0f * P, 14.0f * P);
    m_shapesByFacing[static_cast<size_t>(Direction::East)] = CollisionShape::combine(m_collisionShape, slopeE);
}

BlockState LecternBlock::getStateForPlacement(BlockItemUseContext& context) {
    Direction facing = context.horizontalDirection();
    return defaultState().with(BlockStateProperties::HORIZONTAL_FACING(), Directions::opposite(facing));
}

const BlockState& LecternBlock::rotate(const BlockState& state, Rotation rotation) const {
    Direction facing = state.get(BlockStateProperties::HORIZONTAL_FACING());
    Direction rotated = Directions::rotateDirection(facing, rotation);
    return state.with(BlockStateProperties::HORIZONTAL_FACING(), rotated);
}

const BlockState& LecternBlock::mirror(const BlockState& state, Mirror mirror) const {
    if (mirror == Mirror::None) {
        return state;
    }

    Direction facing = state.get(BlockStateProperties::HORIZONTAL_FACING());
    Rotation rotation = Directions::mirrorToRotation(mirror, facing);
    return rotate(state, rotation);
}

void LecternBlock::tick(IWorld& world, const BlockPos& pos, BlockState& state) {
    if (state.get(BlockStateProperties::POWERED())) {
        // 关闭红石信号
        BlockState newState = state.with(BlockStateProperties::POWERED(), false);
        world.setBlockState(pos, &newState, 3);
        // TODO: 通知邻居更新
    }
}

const CollisionShape& LecternBlock::getShape(const BlockState& state) const {
    Direction facing = state.get(BlockStateProperties::HORIZONTAL_FACING());
    size_t index = static_cast<size_t>(facing);
    MC_ASSERT(index < Directions::COUNT && Directions::isHorizontal(facing));
    return m_shapesByFacing[index];
}

const CollisionShape& LecternBlock::getCollisionShape(const BlockState& state) const {
    MC_UNUSED(state);
    return m_collisionShape;
}

i32 LecternBlock::getWeakPower(
    const BlockState& state,
    IWorld& world,
    const BlockPos& pos,
    Direction side) const {

    MC_UNUSED(world);
    MC_UNUSED(pos);
    MC_UNUSED(side);

    return state.get(BlockStateProperties::POWERED()) ? 15 : 0;
}

i32 LecternBlock::getStrongPower(
    const BlockState& state,
    IWorld& world,
    const BlockPos& pos,
    Direction side) const {

    MC_UNUSED(world);
    MC_UNUSED(pos);

    // 只向下发出强信号
    if (side == Direction::Up && state.get(BlockStateProperties::POWERED())) {
        return 15;
    }
    return 0;
}

int LecternBlock::getComparatorInputOverride(
    const BlockState& state,
    IWorld& world,
    const BlockPos& pos) const {

    if (!state.get(BlockStateProperties::HAS_BOOK())) {
        return 0;
    }

    // TODO: 从讲台方块实体获取页面信号
    // 需要实现 LecternEntity
    return 0;
}

bool LecternBlock::tryPlaceBook(IWorld& world, const BlockPos& pos, BlockState& state, u32 itemId) {
    if (state.get(BlockStateProperties::HAS_BOOK())) {
        return false;  // 已经有书了
    }

    // TODO: 检查物品是否为书

    setHasBook(world, pos, state, true);
    return true;
}

void LecternBlock::setHasBook(IWorld& world, const BlockPos& pos, BlockState& state, bool hasBook) {
    BlockState newState = state
        .with(BlockStateProperties::HAS_BOOK(), hasBook)
        .with(BlockStateProperties::POWERED(), false);
    world.setBlockState(pos, &newState, 3);

    // 通知邻居更新
    // TODO: world.notifyNeighborsOfStateChange(pos.down(), this);
}

void LecternBlock::pulse(IWorld& world, const BlockPos& pos, BlockState& state) {
    // 开启红石信号
    BlockState newState = state.with(BlockStateProperties::POWERED(), true);
    world.setBlockState(pos, &newState, 3);

    // 安排tick来关闭信号
    // TODO: world.getPendingBlockTicks().scheduleTick(pos, this, 2);

    // TODO: 播放声音
}

} // namespace blocks
} // namespace mc
