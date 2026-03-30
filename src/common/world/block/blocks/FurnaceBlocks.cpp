#include "FurnaceBlocks.hpp"
#include "../../blockentity/processing/FurnaceEntity.hpp"
#include "../../blockentity/processing/BlastFurnaceEntity.hpp"
#include "../../blockentity/processing/SmokerEntity.hpp"
#include "../../IWorld.hpp"
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

void FurnaceBlock::interactWith(IWorld& world, const BlockPos& pos, Player& player) {
    // 打开普通熔炉GUI
    // TODO: 实现GUI打开
    // player.openContainer(new FurnaceContainer(player.getInventory(), furnace->getInventory()));
    (void)world;
    (void)pos;
    (void)player;
}

// ========== BlastFurnaceBlock ==========

BlastFurnaceBlock::BlastFurnaceBlock(const BlockProperties& properties)
    : AbstractFurnaceBlock(properties) {
}

std::unique_ptr<BlockEntity> BlastFurnaceBlock::createBlockEntity(const BlockPos& pos) {
    return std::make_unique<blockentity::BlastFurnaceEntity>(pos);
}

void BlastFurnaceBlock::interactWith(IWorld& world, const BlockPos& pos, Player& player) {
    // 打开高炉GUI
    // TODO: 实现GUI打开
    (void)world;
    (void)pos;
    (void)player;
}

// ========== SmokerBlock ==========

SmokerBlock::SmokerBlock(const BlockProperties& properties)
    : AbstractFurnaceBlock(properties) {
}

std::unique_ptr<BlockEntity> SmokerBlock::createBlockEntity(const BlockPos& pos) {
    return std::make_unique<blockentity::SmokerEntity>(pos);
}

void SmokerBlock::interactWith(IWorld& world, const BlockPos& pos, Player& player) {
    // 打开烟熏炉GUI
    // TODO: 实现GUI打开
    (void)world;
    (void)pos;
    (void)player;
}

} // namespace blocks
} // namespace mc
