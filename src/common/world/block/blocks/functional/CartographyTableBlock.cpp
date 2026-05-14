#include "CartographyTableBlock.hpp"
#include "../../../../item/context/BlockItemUseContext.hpp"
#include "../../../../util/assert/AssertAll.hpp"
#include "../../../IWorld.hpp"

namespace mc {
namespace blocks {

// ========== CartographyTableBlock 实现 ==========

CartographyTableBlock::CartographyTableBlock(const BlockProperties& properties)
    : Block(properties)
{

    // 制图台没有特殊状态属性
    auto container = StateContainer<Block, BlockState>::Builder(*this).create(
        [](const Block& block, std::unordered_map<const IProperty*, size_t> values, u32 id) {
            return std::make_unique<BlockState>(block, std::move(values), id);
        });
    createBlockState(std::move(container));

    // 制图台形状是完整方块
    m_shape = CollisionShape::fullBlock();
}

BlockState CartographyTableBlock::getStateForPlacement(BlockItemUseContext& context)
{
    return defaultState();
}

const CollisionShape& CartographyTableBlock::getShape(const BlockState& state) const
{
    MC_UNUSED(state);
    return m_shape;
}

} // namespace blocks
} // namespace mc
