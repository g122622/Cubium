#include "SpawnerBlock.hpp"

namespace mc {
namespace blocks {

SpawnerBlock::SpawnerBlock(const BlockProperties& properties)
    : Block(properties)
{
    // 刷怪笼没有特殊状态
}

ActionResultType SpawnerBlock::onBlockActivated(const BlockState& state,
    IWorld& world,
    const BlockPos& pos,
    Player& player,
    Hand hand,
    const BlockRaycastResult& hit)
{

    MC_UNUSED(state);
    MC_UNUSED(world);
    MC_UNUSED(pos);
    MC_UNUSED(player);
    MC_UNUSED(hand);
    MC_UNUSED(hit);

    // 只有创造模式可以打开刷怪笼界面
    return ActionResultType::Pass;
}

} // namespace blocks
} // namespace mc
