#include "BedBlock.hpp"
#include "../../../IWorld.hpp"
#include "../../../../item/context/BlockItemUseContext.hpp"
#include "../../../../util/Direction.hpp"
#include "../../../../util/assert/AssertAll.hpp"

namespace mc {
namespace blocks {

// ========== BedBlock 实现 ==========

BedBlock::BedBlock(u32 color, const BlockProperties& properties)
    : Block(properties)
    , m_color(color) {

    // 创建状态容器
    auto container = StateContainer<Block, BlockState>::Builder(*this)
        .add(BlockStateProperties::HORIZONTAL_FACING())
        .add(BlockStateProperties::BED_PART())
        .add(BlockStateProperties::OCCUPIED())
        .create([](const Block& block, std::unordered_map<const IProperty*, size_t> values, u32 id) {
            return std::make_unique<BlockState>(block, std::move(values), id);
        });
    createBlockState(std::move(container));

    // 设置默认状态
    setDefaultState(defaultState()
        .with(BlockStateProperties::HORIZONTAL_FACING(), Direction::North)
        .with(BlockStateProperties::BED_PART(), BlockStateProperties::BedPart::Foot)
        .with(BlockStateProperties::OCCUPIED(), false));

    // 预计算各朝向的形状
    // 床的形状：主体 + 四个床腿
    constexpr f32 P = 1.0f / 16.0f;

    // 主体形状 (高度9像素，从Y=3开始)
    CollisionShape baseShape = CollisionShape::box(0.0f, 3.0f * P, 0.0f, 16.0f * P, 9.0f * P, 16.0f * P);

    // 床腿形状
    CollisionShape legNW = CollisionShape::box(0.0f, 0.0f, 0.0f, 3.0f * P, 3.0f * P, 3.0f * P);
    CollisionShape legNE = CollisionShape::box(13.0f * P, 0.0f, 0.0f, 16.0f * P, 3.0f * P, 3.0f * P);
    CollisionShape legSW = CollisionShape::box(0.0f, 0.0f, 13.0f * P, 3.0f * P, 3.0f * P, 16.0f * P);
    CollisionShape legSE = CollisionShape::box(13.0f * P, 0.0f, 13.0f * P, 16.0f * P, 3.0f * P, 16.0f * P);

    // 北朝向形状 (头部在南)
    m_shapesByFacing[static_cast<size_t>(Direction::North)] =
        CollisionShape::combine(
            CollisionShape::combine(
                CollisionShape::combine(baseShape, legNW),
                legNE
            ),
            CollisionShape::combine(legSW, legSE)
        );

    // 南朝向形状 (头部在北)
    m_shapesByFacing[static_cast<size_t>(Direction::South)] =
        CollisionShape::combine(
            CollisionShape::combine(
                CollisionShape::combine(baseShape, legNW),
                legNE
            ),
            CollisionShape::combine(legSW, legSE)
        );

    // 西朝向形状 (头部在东)
    m_shapesByFacing[static_cast<size_t>(Direction::West)] =
        CollisionShape::combine(
            CollisionShape::combine(
                CollisionShape::combine(baseShape, legNW),
                legNE
            ),
            CollisionShape::combine(legSW, legSE)
        );

    // 东朝向形状 (头部在西)
    m_shapesByFacing[static_cast<size_t>(Direction::East)] =
        CollisionShape::combine(
            CollisionShape::combine(
                CollisionShape::combine(baseShape, legNW),
                legNE
            ),
            CollisionShape::combine(legSW, legSE)
        );
}

BlockState BedBlock::getStateForPlacement(BlockItemUseContext& context) {
    Direction facing = context.horizontalDirection();
    BlockPos pos = context.blockPos();
    BlockPos headPos(pos.x + Directions::xOffset(facing), pos.y, pos.z + Directions::zOffset(facing));

    // 检查头部位置是否可放置
    const BlockState* headState = context.getWorld().getBlockState(headPos.x, headPos.y, headPos.z);
    if (headState != nullptr && headState->isAir()) {
        return defaultState().with(BlockStateProperties::HORIZONTAL_FACING(), facing);
    }

    // 无法放置完整的床
    return defaultState();
}

BlockState BedBlock::updatePostPlacement(
    const BlockState& state,
    Direction facing,
    const BlockState& facingState,
    IWorld& world,
    const BlockPos& currentPos,
    const BlockPos& facingPos) {

    BlockStateProperties::BedPart part = state.get(BlockStateProperties::BED_PART());
    Direction bedFacing = state.get(BlockStateProperties::HORIZONTAL_FACING());

    // 计算另一部分的方向
    Direction otherDir = (part == BlockStateProperties::BedPart::Foot) ? bedFacing : Directions::opposite(bedFacing);

    if (facing == otherDir) {
        // 另一半被移除
        if (facingState.isAir()) {
            return world.getBlockState(currentPos.x, currentPos.y, currentPos.z)->getBlock().defaultState();
        }
        // 同步占用状态
        if (facingState.hasProperty(BlockStateProperties::OCCUPIED())) {
            return state.with(BlockStateProperties::OCCUPIED(), facingState.get(BlockStateProperties::OCCUPIED()));
        }
    }

    return state;
}

const CollisionShape& BedBlock::getShape(const BlockState& state) const {
    Direction facing = state.get(BlockStateProperties::HORIZONTAL_FACING());
    size_t index = static_cast<size_t>(facing);
    MC_ASSERT(index < Directions::COUNT && Directions::isHorizontal(facing));
    return m_shapesByFacing[index];
}

void BedBlock::setOccupied(IWorld& world, const BlockPos& pos, BlockState& state, bool occupied) {
    if (state.hasProperty(BlockStateProperties::OCCUPIED())) {
        world.setBlockState(pos.x, pos.y, pos.z, &state.with(BlockStateProperties::OCCUPIED(), occupied), 2);
    }
}

bool BedBlock::isBed(IWorld& world, const BlockPos& pos) {
    const BlockState* state = world.getBlockState(pos.x, pos.y, pos.z);
    if (state == nullptr) {
        return false;
    }
    // 检查是否为床方块
    // TODO: 添加方块类型检查
    return state->hasProperty(BlockStateProperties::BED_PART());
}

} // namespace blocks
} // namespace mc
