#include "SimpleBlock.hpp"

namespace mc {

SimpleBlock::SimpleBlock(BlockProperties properties)
    : Block(properties)
{
    // 简单方块没有属性
    auto container = StateContainer<Block, BlockState>::Builder(*this).create(
        [](const Block& block, std::unordered_map<const IProperty*, size_t> values, u32 id) {
            return std::make_unique<BlockState>(block, std::move(values), id);
        });
    createBlockState(std::move(container));
}

bool SimpleBlock::isSolid(const BlockState& state) const
{
    (void)state;
    return material().isSolid();
}

} // namespace mc
