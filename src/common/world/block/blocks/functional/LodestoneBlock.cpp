#include "LodestoneBlock.hpp"
#include "../../../IWorld.hpp"
#include "../../../../item/context/BlockItemUseContext.hpp"
#include "../../../../util/assert/AssertAll.hpp"

namespace mc {
namespace blocks {

// ========== LodestoneBlock 实现 ==========

LodestoneBlock::LodestoneBlock(const BlockProperties& properties)
    : Block(properties) {

    // 磁石没有特殊状态属性
    auto container = StateContainer<Block, BlockState>::Builder(*this)
        .create([](const Block& block, std::unordered_map<const IProperty*, size_t> values, u32 id) {
            return std::make_unique<BlockState>(block, std::move(values), id);
        });
    createBlockState(std::move(container));

    // 磁石形状是完整方块
    m_shape = CollisionShape::fullBlock();
}

BlockState LodestoneBlock::getStateForPlacement(BlockItemUseContext& context) {
    return defaultState();
}

const CollisionShape& LodestoneBlock::getShape(const BlockState& state) const {
    MC_UNUSED(state);
    return m_shape;
}

} // namespace blocks
} // namespace mc
