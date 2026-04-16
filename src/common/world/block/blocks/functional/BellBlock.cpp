#include "BellBlock.hpp"
#include "../../../IWorld.hpp"
#include "../../../../item/context/BlockItemUseContext.hpp"
#include "../../../../util/Direction.hpp"
#include "../../../../util/assert/AssertAll.hpp"

namespace mc {
namespace blocks {

// ========== BellBlock 实现 ==========

BellBlock::BellBlock(const BlockProperties& properties)
    : Block(properties) {

    // 创建状态容器
    auto container = StateContainer<Block, BlockState>::Builder(*this)
        .add(BlockStateProperties::HORIZONTAL_FACING())
        .add(BlockStateProperties::BELL_ATTACHMENT())
        .add(BlockStateProperties::POWERED())
        .create([](const Block& block, std::unordered_map<const IProperty*, size_t> values, u32 id) {
            return std::make_unique<BlockState>(block, std::move(values), id);
        });
    createBlockState(std::move(container));

    // 设置默认状态
    setDefaultState(defaultState()
        .with(BlockStateProperties::HORIZONTAL_FACING(), Direction::North)
        .with(BlockStateProperties::BELL_ATTACHMENT(), BlockStateProperties::BellAttachment::Floor)
        .with(BlockStateProperties::POWERED(), false));

    // 创建钟的形状
    constexpr f32 P = 1.0f / 16.0f;

    // 地面附着：钟身 + 支架
    m_floorShape = CollisionShape::box(4.0f * P, 0.0f, 4.0f * P, 12.0f * P, 16.0f * P, 12.0f * P);

    // 天花板附着
    m_ceilingShape = CollisionShape::box(4.0f * P, 0.0f, 4.0f * P, 12.0f * P, 16.0f * P, 12.0f * P);

    // 墙面附着
    m_wallShape = CollisionShape::box(4.0f * P, 4.0f * P, 0.0f, 12.0f * P, 12.0f * P, 16.0f * P);

    // 初始化形状缓存
    for (int i = 0; i < 16; ++i) {
        m_shapesByState[i] = m_floorShape;
    }
}

BlockState BellBlock::getStateForPlacement(BlockItemUseContext& context) {
    Direction facing = context.horizontalDirection();
    Direction clickedFace = context.getClickedFace();

    // 根据点击的面确定附着类型
    BlockStateProperties::BellAttachment attachment;
    if (clickedFace == Direction::Up) {
        attachment = BlockStateProperties::BellAttachment::Floor;
    } else if (clickedFace == Direction::Down) {
        attachment = BlockStateProperties::BellAttachment::Ceiling;
    } else {
        attachment = BlockStateProperties::BellAttachment::SingleWall;
        facing = clickedFace;
    }

    return defaultState()
        .with(BlockStateProperties::HORIZONTAL_FACING(), facing)
        .with(BlockStateProperties::BELL_ATTACHMENT(), attachment);
}

BlockState BellBlock::updatePostPlacement(
    const BlockState& state,
    Direction facing,
    const BlockState& facingState,
    IWorld& world,
    const BlockPos& currentPos,
    const BlockPos& facingPos) {

    BlockStateProperties::BellAttachment attachment = state.get(BlockStateProperties::BELL_ATTACHMENT());
    Direction bellFacing = state.get(BlockStateProperties::HORIZONTAL_FACING());

    // 检查支撑是否仍然有效
    bool valid = true;
    switch (attachment) {
        case BlockStateProperties::BellAttachment::Floor:
            if (facing == Direction::Down) {
                valid = facingState.isSolid();
            }
            break;
        case BlockStateProperties::BellAttachment::Ceiling:
            if (facing == Direction::Up) {
                valid = facingState.isSolid();
            }
            break;
        case BlockStateProperties::BellAttachment::SingleWall:
            if (facing == bellFacing) {
                valid = facingState.isSolid();
            }
            break;
        case BlockStateProperties::BellAttachment::DoubleWall:
            // 双面墙需要两侧都有支撑
            if (facing == bellFacing || facing == Directions::opposite(bellFacing)) {
                valid = facingState.isSolid();
            }
            break;
    }

    if (!valid) {
        // 掉落
        // TODO: 掉落物品
        return world.getBlockState(currentPos)->getBlock().defaultState();
    }

    return state;
}

const BlockState& BellBlock::rotate(const BlockState& state, Rotation rotation) const {
    Direction facing = state.get(BlockStateProperties::HORIZONTAL_FACING());
    Direction rotated = Directions::rotateDirection(facing, rotation);
    return state.with(BlockStateProperties::HORIZONTAL_FACING(), rotated);
}

const BlockState& BellBlock::mirror(const BlockState& state, Mirror mirror) const {
    if (mirror == Mirror::None) {
        return state;
    }

    Direction facing = state.get(BlockStateProperties::HORIZONTAL_FACING());
    Rotation rotation = Directions::mirrorToRotation(mirror, facing);
    return rotate(state, rotation);
}

const CollisionShape& BellBlock::getShape(const BlockState& state) const {
    BlockStateProperties::BellAttachment attachment = state.get(BlockStateProperties::BELL_ATTACHMENT());

    switch (attachment) {
        case BlockStateProperties::BellAttachment::Floor:
            return m_floorShape;
        case BlockStateProperties::BellAttachment::Ceiling:
            return m_ceilingShape;
        case BlockStateProperties::BellAttachment::SingleWall:
        case BlockStateProperties::BellAttachment::DoubleWall:
            return m_wallShape;
        default:
            return m_floorShape;
    }
}

} // namespace blocks
} // namespace mc
