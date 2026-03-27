#include "world/blockentity/processing/FurnaceEntity.hpp"

namespace mc {
namespace blockentity {

FurnaceEntity::FurnaceEntity(const BlockPos& pos)
    : AbstractFurnaceEntity(BlockEntityType::Furnace, pos) {
}

} // namespace blockentity
} // namespace mc
