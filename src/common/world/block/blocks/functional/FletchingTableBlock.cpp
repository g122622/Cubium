#include "FletchingTableBlock.hpp"
#include "../../../IWorld.hpp"
#include "../../../../item/BlockItemUseContext.hpp"
#include "../../../../util/assert/AssertAll.hpp"

namespace mc {
namespace blocks {

// ========== FletchingTableBlock 实现 ==========

FletchingTableBlock::FletchingTableBlock(const BlockProperties& properties)
    : Block(properties) {

    // 制箭台没有特殊状态属性
    auto container = StateContainer<Block, BlockState>::Builder(*this)
        .create([](const Block& block, std::unordered_map<const IProperty*, size_t> values, u32 id) {
            return std::make_unique<BlockState>(block, std::move(values), id);
        });
    createBlockState(std::move(container));

    // 制箭台形状是完整方块
    m_shape = CollisionShape::fullBlock();
}

BlockState FletchingTableBlock::getStateForPlacement(BlockItemUseContext& context) {
    return defaultState();
}

const CollisionShape& FletchingTableBlock::getShape(const BlockState& state) const {
    MC_UNUSED(state);
    return m_shape;
}

} // namespace blocks
} // namespace mc
