#include "RailBlock.hpp"

namespace mc {
namespace blocks {

RailBlock::RailBlock(const BlockProperties& properties)
    : AbstractRailBlock(properties, false)
{
    // 创建状态容器
    auto container = StateContainer<Block, BlockState>::Builder(*this)
        .add(SHAPE())
        .create([this](const Block& block, std::unordered_map<const IProperty*, size_t> values, u32 id) {
            return std::make_unique<BlockState>(block, std::move(values), id);
        });
    createBlockState(std::move(container));

    // 设置默认状态
    setDefaultState(defaultState().with(SHAPE(), RailShape::NorthSouth));
}

void RailBlock::fillStateContainer(StateContainer<Block, BlockState>& container) {
    // 状态容器在构造函数中创建，此方法留空
    MC_UNUSED(container);
}

RailShape RailBlock::getRailShape(const BlockState& state) const {
    return state.get(SHAPE());
}

BlockState RailBlock::withRailShape(const BlockState& state, RailShape shape) const {
    return state.with(SHAPE(), shape);
}

} // namespace blocks
} // namespace mc
