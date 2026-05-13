#include "SoulFireBlock.hpp"
#include "../../../IWorld.hpp"
#include "../../BlockRegistry.hpp"
#include "../../BlockTags.hpp"

namespace mc {
namespace blocks {

SoulFireBlock::SoulFireBlock(const BlockProperties& properties)
    : FireBlock(properties, 2) {  // 灵魂火伤害更高 (2)
}

bool SoulFireBlock::isValidPosition(
    const BlockState& state,
    IBlockReader& world,
    const BlockPos& pos) const {

    MC_UNUSED(state);

    // MC 1.16.5: SoulFireBlock.isValidPosition
    // 灵魂火只能在灵魂沙或灵魂土上方存在
    const BlockState* belowState = world.getBlockState(pos.down());
    return belowState != nullptr && isSoulFireBase(&belowState->getBlock());
}

BlockState SoulFireBlock::updatePostPlacement(
    const BlockState& state,
    Direction facing,
    const BlockState& facingState,
    IWorld& world,
    const BlockPos& currentPos,
    const BlockPos& facingPos) {

    MC_UNUSED(facing);
    MC_UNUSED(facingState);
    MC_UNUSED(facingPos);

    // MC 1.16.5: SoulFireBlock.updatePostPlacement
    // 如果下方不再是灵魂沙/土，则移除火焰
    IBlockReader& blockReader = static_cast<IBlockReader&>(world);
    if (!isValidPosition(state, blockReader, currentPos)) {
        if (auto* airState = BlockRegistry::instance().airState()) {
            return *airState;
        }
    }

    return state;
}

bool SoulFireBlock::isSoulFireBase(const Block* block) {
    // MC 1.16.5: SoulFireBlock.func_235577_c_
    // 检查方块是否在 soul_fire_base_blocks 标签中
    return block != nullptr && BlockTags::SOUL_FIRE_BASE_BLOCKS().contains(block);
}

bool SoulFireBlock::canBurn(IBlockReader& world, const BlockPos& pos) const {
    MC_UNUSED(world);
    MC_UNUSED(pos);
    // 灵魂火不会蔓延燃烧其他方块
    return false;
}

} // namespace blocks
} // namespace mc
