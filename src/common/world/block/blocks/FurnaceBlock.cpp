#include "FurnaceBlock.hpp"
#include "../../../entity/entities/player/Player.hpp"
#include "../../../entity/inventory/ContainerTypes.hpp"
#include "../../IWorld.hpp"
#include "../../blockentity/processing/FurnaceEntity.hpp"

namespace mc {
namespace blocks {

FurnaceBlock::FurnaceBlock(const BlockProperties& properties)
    : AbstractFurnaceBlock(properties)
{}

std::unique_ptr<BlockEntity> FurnaceBlock::createBlockEntity(const BlockPos& pos)
{
    return std::make_unique<blockentity::FurnaceEntity>(pos);
}

bool FurnaceBlock::interactWith(IWorld& world, const BlockPos& pos, Player& player)
{
    return world.openContainer(ContainerType::Furnace, pos, player);
}

} // namespace blocks
} // namespace mc
