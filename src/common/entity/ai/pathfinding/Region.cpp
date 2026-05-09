#include "Region.hpp"
#include "world/block/Block.hpp"

namespace mc::entity::ai::pathfinding {

const BlockState* Region::getBlockState(i32 x, i32 y, i32 z) const {
    u32 stateId = getBlockStateId(x, y, z);
    if (stateId == 0) {
        // 空气方块返回 nullptr
        return nullptr;
    }
    return Block::getBlockState(stateId);
}

} // namespace mc::entity::ai::pathfinding
