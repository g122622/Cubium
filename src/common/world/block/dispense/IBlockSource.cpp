#include "IBlockSource.hpp"
#include "../../IWorld.hpp"
#include "../Block.hpp"

namespace mc {
namespace blocks {

DispensePosition::DispensePosition(IWorld& world, const BlockPos& pos, const BlockState& state,
                                   double offsetX, double offsetY, double offsetZ)
    : m_world(world)
    , m_pos(pos)
    , m_state(state)
    , m_offsetX(offsetX)
    , m_offsetY(offsetY)
    , m_offsetZ(offsetZ) {
}

} // namespace blocks
} // namespace mc
