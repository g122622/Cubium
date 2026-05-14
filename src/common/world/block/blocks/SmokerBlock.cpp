#include "SmokerBlock.hpp"
#include "../../../entity/entities/player/Player.hpp"
#include "../../../entity/inventory/ContainerTypes.hpp"
#include "../../IWorld.hpp"
#include "../../blockentity/processing/SmokerEntity.hpp"

namespace mc {
namespace blocks {

SmokerBlock::SmokerBlock(const BlockProperties& properties)
    : AbstractFurnaceBlock(properties)
{}

std::unique_ptr<BlockEntity> SmokerBlock::createBlockEntity(const BlockPos& pos)
{
    return std::make_unique<blockentity::SmokerEntity>(pos);
}

bool SmokerBlock::interactWith(IWorld& world, const BlockPos& pos, Player& player)
{
    return world.openContainer(ContainerType::Furnace, pos, player);
}

} // namespace blocks
} // namespace mc
