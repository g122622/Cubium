#include "DirectionalBlock.hpp"
#include "../../../item/context/BlockItemUseContext.hpp"

namespace mc {
namespace blocks {

DirectionalBlock::DirectionalBlock(const BlockProperties& properties)
    : Block(properties)
{
    // 创建状态容器，添加 FACING 属性
    auto container = StateContainer<Block, BlockState>::Builder(*this).add(FACING()).create(
        [](const Block& block, std::unordered_map<const IProperty*, size_t> values, u32 id) {
            return std::make_unique<BlockState>(block, std::move(values), id);
        });
    createBlockState(std::move(container));

    // 默认朝向为北
    setDefaultState(withFacing(defaultState(), Direction::North));
}

BlockState DirectionalBlock::getStateForPlacement(BlockItemUseContext& context)
{
    // 根据玩家朝向设置方块方向
    // 玩家面朝的方向的反方向就是方块的 FACING
    Direction facing = context.horizontalDirection();

    // 对于可放置在墙上/地面的方块，需要根据击中面调整
    // 但这里只处理水平朝向的简单情况
    return withFacing(defaultState(), facing);
}

const BlockState& DirectionalBlock::rotate(const BlockState& state, Rotation rotation) const
{
    Direction currentFacing = getFacing(state);
    Direction newFacing = Directions::rotateDirection(currentFacing, rotation);
    return withFacing(state, newFacing);
}

const BlockState& DirectionalBlock::mirror(const BlockState& state, Mirror mirror) const
{
    Direction currentFacing = getFacing(state);
    Rotation rot = Directions::mirrorToRotation(mirror, currentFacing);
    Direction newFacing = Directions::rotateDirection(currentFacing, rot);
    return withFacing(state, newFacing);
}

Direction DirectionalBlock::getFacing(const BlockState& state) const
{
    return state.get(FACING());
}

const BlockState& DirectionalBlock::withFacing(const BlockState& state, Direction facing) const
{
    return state.with(FACING(), facing);
}

} // namespace blocks
} // namespace mc
