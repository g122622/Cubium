#include "GlazedTerracottaBlock.hpp"
#include "../../../../item/context/BlockItemUseContext.hpp"
#include "../../../../util/Direction.hpp"

namespace mc {
namespace blocks {

GlazedTerracottaBlock::GlazedTerracottaBlock(const BlockProperties& properties)
    : Block(properties)
{
    // 创建状态容器（HORIZONTAL_FACING 属性）
    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(BlockStateProperties::HORIZONTAL_FACING())
            .create([this](const Block& block, std::unordered_map<const IProperty*, size_t> values, u32 id) {
                return std::make_unique<BlockState>(block, std::move(values), id);
            });
    createBlockState(std::move(container));
    setDefaultState(defaultState().with(BlockStateProperties::HORIZONTAL_FACING(), Direction::North));
}

BlockState GlazedTerracottaBlock::getStateForPlacement(BlockItemUseContext& context)
{
    // 根据玩家朝向放置
    Direction facing = context.horizontalDirection();
    return defaultState().with(BlockStateProperties::HORIZONTAL_FACING(), facing);
}

const BlockState& GlazedTerracottaBlock::rotate(const BlockState& state, Rotation rotation) const
{
    Direction facing = state.get(BlockStateProperties::HORIZONTAL_FACING());
    Direction newFacing = Directions::rotateDirection(facing, rotation);
    return state.with(BlockStateProperties::HORIZONTAL_FACING(), newFacing);
}

const BlockState& GlazedTerracottaBlock::mirror(const BlockState& state, Mirror mirror) const
{
    Direction facing = state.get(BlockStateProperties::HORIZONTAL_FACING());
    Rotation rot = Directions::mirrorToRotation(mirror, facing);
    Direction newFacing = Directions::rotateDirection(facing, rot);
    return state.with(BlockStateProperties::HORIZONTAL_FACING(), newFacing);
}

} // namespace blocks
} // namespace mc
