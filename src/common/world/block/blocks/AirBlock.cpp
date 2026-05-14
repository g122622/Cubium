#include "AirBlock.hpp"

namespace mc {

AirBlock::AirBlock(BlockProperties properties)
    : Block(properties)
{
    // 空气没有属性，创建空状态容器
    auto container = StateContainer<Block, BlockState>::Builder(*this).create(
        [](const Block& block, std::unordered_map<const IProperty*, size_t> values, u32 id) {
            return std::make_unique<BlockState>(block, std::move(values), id);
        });
    createBlockState(std::move(container));
}

const CollisionShape& AirBlock::getShape(const BlockState& state) const
{
    (void)state;
    return VoxelShapes::empty();
}

const CollisionShape& AirBlock::getCollisionShape(const BlockState& state) const
{
    (void)state;
    return VoxelShapes::empty();
}

bool AirBlock::isAir(const BlockState& state) const
{
    (void)state;
    return true;
}

bool AirBlock::isSolid(const BlockState& state) const
{
    (void)state;
    return false;
}

bool AirBlock::isOpaque(const BlockState& state) const
{
    (void)state;
    return false;
}

} // namespace mc
