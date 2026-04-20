#include "FurnaceBlocks.hpp"
#include "../../blockentity/processing/FurnaceEntity.hpp"
#include "../../blockentity/processing/BlastFurnaceEntity.hpp"
#include "../../blockentity/processing/SmokerEntity.hpp"
#include "../../IWorld.hpp"
#include "../../../entity/inventory/ContainerTypes.hpp"
#include "../../../entity/entities/player/Player.hpp"

namespace mc {
namespace blocks {

// ========== FurnaceBlock ==========

FurnaceBlock::FurnaceBlock(const BlockProperties& properties)
    : AbstractFurnaceBlock(properties) {
}

std::unique_ptr<BlockEntity> FurnaceBlock::createBlockEntity(const BlockPos& pos) {
    return std::make_unique<blockentity::FurnaceEntity>(pos);
}

bool FurnaceBlock::interactWith(IWorld& world, const BlockPos& pos, Player& player) {
    return world.openContainer(ContainerType::Furnace, pos, player);
}

// ========== BlastFurnaceBlock ==========

BlastFurnaceBlock::BlastFurnaceBlock(const BlockProperties& properties)
    : AbstractFurnaceBlock(properties) {
}

std::unique_ptr<BlockEntity> BlastFurnaceBlock::createBlockEntity(const BlockPos& pos) {
    return std::make_unique<blockentity::BlastFurnaceEntity>(pos);
}

bool BlastFurnaceBlock::interactWith(IWorld& world, const BlockPos& pos, Player& player) {
    return world.openContainer(ContainerType::Furnace, pos, player);
}

// ========== SmokerBlock ==========

SmokerBlock::SmokerBlock(const BlockProperties& properties)
    : AbstractFurnaceBlock(properties) {
}

std::unique_ptr<BlockEntity> SmokerBlock::createBlockEntity(const BlockPos& pos) {
    return std::make_unique<blockentity::SmokerEntity>(pos);
}

bool SmokerBlock::interactWith(IWorld& world, const BlockPos& pos, Player& player) {
    return world.openContainer(ContainerType::Furnace, pos, player);
}

} // namespace blocks
} // namespace mc
