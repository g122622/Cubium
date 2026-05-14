#include "RedstoneBlock.hpp"

namespace mc {
namespace blocks {

RedstoneBlock::RedstoneBlock(const BlockProperties& properties)
    : Block(properties)
{}

i32 RedstoneBlock::getWeakPower(const BlockState& state, IWorld& world, const BlockPos& pos, Direction side) const
{
    MC_UNUSED(state);
    MC_UNUSED(world);
    MC_UNUSED(pos);
    MC_UNUSED(side);
    // 红石块始终输出强度15
    return world::redstone::RedstonePower::MAX_POWER;
}

i32 RedstoneBlock::getStrongPower(const BlockState& state, IWorld& world, const BlockPos& pos, Direction side) const
{
    MC_UNUSED(state);
    MC_UNUSED(world);
    MC_UNUSED(pos);
    MC_UNUSED(side);
    // 红石块输出强信号强度15到所有方向
    return world::redstone::RedstonePower::MAX_POWER;
}

} // namespace blocks
} // namespace mc
