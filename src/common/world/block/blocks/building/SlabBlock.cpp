#include "SlabBlock.hpp"
#include "../../../IWorld.hpp"
#include "../../../../item/context/BlockItemUseContext.hpp"
#include "../../../fluid/Fluid.hpp"
#include "../../../../util/assert/AssertAll.hpp"

namespace mc {
namespace blocks {

// ========== 构造函数 ==========

SlabBlock::SlabBlock(const BlockProperties& properties)
    : Block(properties)
    , m_bottomShape(CollisionShape::box(0.0f, 0.0f, 0.0f, 1.0f, 0.5f, 1.0f))
    , m_topShape(CollisionShape::box(0.0f, 0.5f, 0.0f, 1.0f, 1.0f, 1.0f))
    , m_fullCubeShape(CollisionShape::fullBlock()) {

    // 创建状态容器
    auto container = StateContainer<Block, BlockState>::Builder(*this)
        .add(BlockStateProperties::SLAB_TYPE())
        .add(BlockStateProperties::WATERLOGGED())
        .create([](const Block& block, std::unordered_map<const IProperty*, size_t> values, u32 id) {
            return std::make_unique<BlockState>(block, std::move(values), id);
        });
    createBlockState(std::move(container));

    // 设置默认状态
    setDefaultState(defaultState()
        .with(BlockStateProperties::SLAB_TYPE(), BlockStateProperties::SlabType::Bottom)
        .with(BlockStateProperties::WATERLOGGED(), false));
}

// ========== 放置和更新 ==========

BlockState SlabBlock::getStateForPlacement(BlockItemUseContext& context) {
    BlockPos pos = context.placementPos();
    const BlockState* existingState = context.getWorld().getBlockState(pos.x, pos.y, pos.z);

    // 检查是否含水
    bool waterlogged = false;
    if (existingState != nullptr) {
        const fluid::FluidState* fluid = existingState->getFluidState();
        waterlogged = fluid != nullptr && fluid->isSource();
    }

    // 根据点击位置决定上半/下半
    // 点击上半部分 -> 上半台阶
    // 点击下半部分 -> 下半台阶
    // 但如果点击的是同一类型的台阶，则变成双层
    if (existingState != nullptr && &existingState->getBlock() == this) {
        // 点击同一类型台阶，变成双层
        return existingState->with(BlockStateProperties::SLAB_TYPE(), BlockStateProperties::SlabType::Double)
            .with(BlockStateProperties::WATERLOGGED(), false);
    }

    // 检查相邻位置是否有同类型台阶
    Direction clickedFace = context.getClickedFace();
    BlockPos neighborPos(pos.x + Directions::xOffset(clickedFace),
                         pos.y + Directions::yOffset(clickedFace),
                         pos.z + Directions::zOffset(clickedFace));
    const BlockState* neighborState = context.getWorld().getBlockState(neighborPos.x, neighborPos.y, neighborPos.z);

    if (neighborState != nullptr && &neighborState->getBlock() == this) {
        BlockStateProperties::SlabType neighborType = neighborState->get(BlockStateProperties::SLAB_TYPE());

        // 如果相邻是单层台阶，且点击方向正确，变成双层
        if (neighborType != BlockStateProperties::SlabType::Double) {
            // 检查是否匹配
            bool neighborIsTop = (neighborType == BlockStateProperties::SlabType::Top);
            bool clickedTop = (clickedFace == Direction::Down);

            if (neighborIsTop == clickedTop) {
                // 变成双层
                return defaultState()
                    .with(BlockStateProperties::SLAB_TYPE(), BlockStateProperties::SlabType::Double)
                    .with(BlockStateProperties::WATERLOGGED(), waterlogged);
            }
        }
    }

    // 根据点击位置决定上半/下半
    f32 hitY = context.getHitY();
    bool isTop = (clickedFace == Direction::Down) || (clickedFace != Direction::Up && hitY > 0.5f);

    return defaultState()
        .with(BlockStateProperties::SLAB_TYPE(), isTop ? BlockStateProperties::SlabType::Top
                                                        : BlockStateProperties::SlabType::Bottom)
        .with(BlockStateProperties::WATERLOGGED(), waterlogged);
}

bool SlabBlock::isValidPosition(
    const BlockState& state,
    IBlockReader& world,
    const BlockPos& pos) const {

    MC_UNUSED(state);
    MC_UNUSED(world);
    MC_UNUSED(pos);

    // 台阶可以放置在任何位置（除非需要特殊支撑）
    return true;
}

BlockState SlabBlock::updatePostPlacement(
    const BlockState& state,
    Direction facing,
    const BlockState& facingState,
    IWorld& world,
    const BlockPos& currentPos,
    const BlockPos& facingPos) {

    MC_UNUSED(facing);
    MC_UNUSED(facingState);
    MC_UNUSED(world);
    MC_UNUSED(currentPos);
    MC_UNUSED(facingPos);

    // 双层台阶不需要特殊更新
    // 单层台阶的支撑检查在 isValidPosition 中进行
    return state;
}

// ========== 形状 ==========

const CollisionShape& SlabBlock::getShape(const BlockState& state) const {
    BlockStateProperties::SlabType type = state.get(BlockStateProperties::SLAB_TYPE());

    switch (type) {
        case BlockStateProperties::SlabType::Bottom:
            return m_bottomShape;
        case BlockStateProperties::SlabType::Top:
            return m_topShape;
        case BlockStateProperties::SlabType::Double:
        default:
            return m_fullCubeShape;
    }
}

const CollisionShape& SlabBlock::getCollisionShape(const BlockState& state) const {
    return getShape(state);
}

// ========== 静态方法 ==========

bool SlabBlock::isDouble(const BlockState& state) {
    return state.get(BlockStateProperties::SLAB_TYPE()) == BlockStateProperties::SlabType::Double;
}

} // namespace blocks
} // namespace mc
