#include "BrewingStandBlock.hpp"
#include "../../../IWorld.hpp"
#include "../../../../item/context/BlockItemUseContext.hpp"
#include "../../../../util/assert/AssertAll.hpp"

namespace mc {
namespace blocks {

// ========== BrewingStandBlock 实现 ==========

BrewingStandBlock::BrewingStandBlock(const BlockProperties& properties)
    : Block(properties) {

    // 创建状态容器
    auto container = StateContainer<Block, BlockState>::Builder(*this)
        .add(BlockStateProperties::HAS_BOTTLE_0())
        .add(BlockStateProperties::HAS_BOTTLE_1())
        .add(BlockStateProperties::HAS_BOTTLE_2())
        .create([](const Block& block, std::unordered_map<const IProperty*, size_t> values, u32 id) {
            return std::make_unique<BlockState>(block, std::move(values), id);
        });
    createBlockState(std::move(container));

    // 设置默认状态
    setDefaultState(defaultState()
        .with(BlockStateProperties::HAS_BOTTLE_0(), false)
        .with(BlockStateProperties::HAS_BOTTLE_1(), false)
        .with(BlockStateProperties::HAS_BOTTLE_2(), false));

    // 创建酿造台形状
    // 主体是底座 + 中间的柱子
    constexpr f32 P = 1.0f / 16.0f;
    CollisionShape base = CollisionShape::box(1.0f * P, 0.0f, 1.0f * P, 15.0f * P, 2.0f * P, 15.0f * P);
    CollisionShape pole = CollisionShape::box(7.0f * P, 0.0f, 7.0f * P, 9.0f * P, 14.0f * P, 9.0f * P);
    m_shape = CollisionShape::combine(base, pole);
}

BlockState BrewingStandBlock::getStateForPlacement(BlockItemUseContext& context) {
    return defaultState();
}

const CollisionShape& BrewingStandBlock::getShape(const BlockState& state) const {
    MC_UNUSED(state);
    return m_shape;
}

const CollisionShape& BrewingStandBlock::getCollisionShape(const BlockState& state) const {
    MC_UNUSED(state);
    return m_shape;
}

int BrewingStandBlock::getComparatorInputOverride(
    const BlockState& state,
    IWorld& world,
    const BlockPos& pos) const {

    MC_UNUSED(state);

    // TODO: 从酿造台方块实体获取比较器信号
    // 需要实现 BrewingStandEntity
    // return Container.calcRedstone(tileEntity);

    return 0;
}

} // namespace blocks
} // namespace mc
