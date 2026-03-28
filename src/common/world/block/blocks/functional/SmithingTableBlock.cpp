#include "SmithingTableBlock.hpp"
#include "../../../IWorld.hpp"
#include "../../../../item/BlockItemUseContext.hpp"
#include "../../../../util/assert/AssertAll.hpp"

namespace mc {
namespace blocks {

// ========== SmithingTableBlock 实现 ==========

SmithingTableBlock::SmithingTableBlock(const BlockProperties& properties)
    : Block(properties) {

    // 锻造台没有特殊状态属性
    auto container = StateContainer<Block, BlockState>::Builder(*this)
        .create([](const Block& block, std::unordered_map<const IProperty*, size_t> values, u32 id) {
            return std::make_unique<BlockState>(block, std::move(values), id);
        });
    createBlockState(std::move(container));

    // 锻造台形状是完整方块
    m_shape = CollisionShape::fullBlock();
}

BlockState SmithingTableBlock::getStateForPlacement(BlockItemUseContext& context) {
    return defaultState();
}

const CollisionShape& SmithingTableBlock::getShape(const BlockState& state) const {
    MC_UNUSED(state);
    return m_shape;
}

} // namespace blocks
} // namespace mc
