#include "StainedGlassBlock.hpp"

namespace mc {
namespace block {

StainedGlassBlock::StainedGlassBlock(BlockProperties properties, DyeColor color)
    : Block(properties)
    , m_color(color)
    , m_colorComponents(BeaconColors::getColorComponents(color))
{
}

const std::array<f32, 3>* StainedGlassBlock::getBeaconColorMultiplier(
    const BlockState& state,
    IWorld* world,
    const BlockPos* pos,
    const BlockPos* beaconPos) const
{
    MC_UNUSED(state);
    MC_UNUSED(world);
    MC_UNUSED(pos);
    MC_UNUSED(beaconPos);
    return &m_colorComponents;
}

} // namespace block
} // namespace mc
