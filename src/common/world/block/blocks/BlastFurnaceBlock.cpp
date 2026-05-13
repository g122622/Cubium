#include "BlastFurnaceBlock.hpp"
#include "../../blockentity/processing/BlastFurnaceEntity.hpp"
#include "../../IWorld.hpp"
#include "../../../entity/inventory/ContainerTypes.hpp"
#include "../../../entity/entities/player/Player.hpp"

namespace mc {
namespace blocks {

BlastFurnaceBlock::BlastFurnaceBlock(const BlockProperties& properties)
    : AbstractFurnaceBlock(properties) {
}

std::unique_ptr<BlockEntity> BlastFurnaceBlock::createBlockEntity(const BlockPos& pos) {
    return std::make_unique<blockentity::BlastFurnaceEntity>(pos);
}

bool BlastFurnaceBlock::interactWith(IWorld& world, const BlockPos& pos, Player& player) {
    return world.openContainer(ContainerType::Furnace, pos, player);
}

} // namespace blocks
} // namespace mc
