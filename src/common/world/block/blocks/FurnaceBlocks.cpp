#include "world/block/blocks/FurnaceBlocks.hpp"
#include "world/blockentity/processing/FurnaceEntity.hpp"
#include "world/blockentity/processing/BlastFurnaceEntity.hpp"
#include "world/blockentity/processing/SmokerEntity.hpp"
#include "world/World.hpp"
#include "entity/Player.hpp"

namespace mc {
namespace blocks {

// ========== FurnaceBlock ==========

FurnaceBlock::FurnaceBlock(const BlockProperties& properties)
    : AbstractFurnaceBlock(properties) {
}

std::unique_ptr<BlockEntity> FurnaceBlock::createBlockEntity(const BlockPos& pos) {
    return std::make_unique<blockentity::FurnaceEntity>(pos);
}

void FurnaceBlock::interactWith(World& world, const BlockPos& pos, Player& player) {
    // 打开普通熔炉GUI
    // TODO: 实现GUI打开
    // player.openContainer(new FurnaceContainer(player.getInventory(), furnace->getInventory()));
}

// ========== BlastFurnaceBlock ==========

BlastFurnaceBlock::BlastFurnaceBlock(const BlockProperties& properties)
    : AbstractFurnaceBlock(properties) {
}

std::unique_ptr<BlockEntity> BlastFurnaceBlock::createBlockEntity(const BlockPos& pos) {
    return std::make_unique<blockentity::BlastFurnaceEntity>(pos);
}

void BlastFurnaceBlock::interactWith(World& world, const BlockPos& pos, Player& player) {
    // 打开高炉GUI
    // TODO: 实现GUI打开
}

// ========== SmokerBlock ==========

SmokerBlock::SmokerBlock(const BlockProperties& properties)
    : AbstractFurnaceBlock(properties) {
}

std::unique_ptr<BlockEntity> SmokerBlock::createBlockEntity(const BlockPos& pos) {
    return std::make_unique<blockentity::SmokerEntity>(pos);
}

void SmokerBlock::interactWith(World& world, const BlockPos& pos, Player& player) {
    // 打开烟熏炉GUI
    // TODO: 实现GUI打开
}

} // namespace blocks
} // namespace mc
