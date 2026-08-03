/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "world/blockentity/BlockEntityType.hpp"
#include "common/resource/ResourceLocation.hpp"
#include <string>
#include <unordered_map>

namespace mc {

namespace {
const std::unordered_map<BlockEntityType, ResourceLocation> typeToIdMap = {
    {BlockEntityType::Chest, ResourceLocation("minecraft", "chest")},
    {BlockEntityType::TrappedChest, ResourceLocation("minecraft", "trapped_chest")},
    {BlockEntityType::EnderChest, ResourceLocation("minecraft", "ender_chest")},
    {BlockEntityType::ShulkerBox, ResourceLocation("minecraft", "shulker_box")},
    {BlockEntityType::Barrel, ResourceLocation("minecraft", "barrel")},
    {BlockEntityType::Furnace, ResourceLocation("minecraft", "furnace")},
    {BlockEntityType::BlastFurnace, ResourceLocation("minecraft", "blast_furnace")},
    {BlockEntityType::Smoker, ResourceLocation("minecraft", "smoker")},
    {BlockEntityType::BrewingStand, ResourceLocation("minecraft", "brewing_stand")},
    {BlockEntityType::Dispenser, ResourceLocation("minecraft", "dispenser")},
    {BlockEntityType::Dropper, ResourceLocation("minecraft", "dropper")},
    {BlockEntityType::Hopper, ResourceLocation("minecraft", "hopper")},
    {BlockEntityType::Piston, ResourceLocation("minecraft", "piston")},
    {BlockEntityType::Comparator, ResourceLocation("minecraft", "comparator")},
    {BlockEntityType::DaylightDetector, ResourceLocation("minecraft", "daylight_detector")},
    {BlockEntityType::Sign, ResourceLocation("minecraft", "sign")},
    {BlockEntityType::Banner, ResourceLocation("minecraft", "banner")},
    {BlockEntityType::StructureBlock, ResourceLocation("minecraft", "structure_block")},
    {BlockEntityType::JigsawBlock, ResourceLocation("minecraft", "jigsaw")},
    {BlockEntityType::Beacon, ResourceLocation("minecraft", "beacon")},
    {BlockEntityType::Bed, ResourceLocation("minecraft", "bed")},
    {BlockEntityType::Bell, ResourceLocation("minecraft", "bell")},
    {BlockEntityType::CommandBlock, ResourceLocation("minecraft", "command_block")},
    {BlockEntityType::EnchantingTable, ResourceLocation("minecraft", "enchanting_table")},
    {BlockEntityType::EndGateway, ResourceLocation("minecraft", "end_gateway")},
    {BlockEntityType::EndPortal, ResourceLocation("minecraft", "end_portal")},
    {BlockEntityType::MobSpawner, ResourceLocation("minecraft", "mob_spawner")},
    {BlockEntityType::Skull, ResourceLocation("minecraft", "skull")},
    {BlockEntityType::Beehive, ResourceLocation("minecraft", "beehive")},
    {BlockEntityType::Campfire, ResourceLocation("minecraft", "campfire")},
    {BlockEntityType::Conduit, ResourceLocation("minecraft", "conduit")},
    {BlockEntityType::Lectern, ResourceLocation("minecraft", "lectern")},
    {BlockEntityType::Jukebox, ResourceLocation("minecraft", "jukebox")},
    {BlockEntityType::Shelf, ResourceLocation("minecraft", "shelf")},
    {BlockEntityType::TrialSpawner, ResourceLocation("minecraft", "trial_spawner")},
    {BlockEntityType::Vault, ResourceLocation("minecraft", "vault")},
    {BlockEntityType::Crafter, ResourceLocation("minecraft", "crafter")},
    {BlockEntityType::SculkSensor, ResourceLocation("minecraft", "sculk_sensor")},
    {BlockEntityType::SculkShrieker, ResourceLocation("minecraft", "sculk_shrieker")},
    {BlockEntityType::DecoratedPot, ResourceLocation("minecraft", "decorated_pot")},
    {BlockEntityType::BrushableBlock, ResourceLocation("minecraft", "brushable_block")},
    {BlockEntityType::CopperGolemStatue, ResourceLocation("minecraft", "copper_golem_statue")}};

const std::unordered_map<std::string, BlockEntityType> idToTypeMap = {{"minecraft:chest", BlockEntityType::Chest},
    {"minecraft:trapped_chest", BlockEntityType::TrappedChest},
    {"minecraft:ender_chest", BlockEntityType::EnderChest},
    {"minecraft:shulker_box", BlockEntityType::ShulkerBox},
    {"minecraft:barrel", BlockEntityType::Barrel},
    {"minecraft:furnace", BlockEntityType::Furnace},
    {"minecraft:blast_furnace", BlockEntityType::BlastFurnace},
    {"minecraft:smoker", BlockEntityType::Smoker},
    {"minecraft:brewing_stand", BlockEntityType::BrewingStand},
    {"minecraft:dispenser", BlockEntityType::Dispenser},
    {"minecraft:dropper", BlockEntityType::Dropper},
    {"minecraft:hopper", BlockEntityType::Hopper},
    {"minecraft:piston", BlockEntityType::Piston},
    {"minecraft:comparator", BlockEntityType::Comparator},
    {"minecraft:daylight_detector", BlockEntityType::DaylightDetector},
    {"minecraft:sign", BlockEntityType::Sign},
    {"minecraft:banner", BlockEntityType::Banner},
    {"minecraft:structure_block", BlockEntityType::StructureBlock},
    {"minecraft:jigsaw", BlockEntityType::JigsawBlock},
    {"minecraft:beacon", BlockEntityType::Beacon},
    {"minecraft:bed", BlockEntityType::Bed},
    {"minecraft:bell", BlockEntityType::Bell},
    {"minecraft:command_block", BlockEntityType::CommandBlock},
    {"minecraft:enchanting_table", BlockEntityType::EnchantingTable},
    {"minecraft:end_gateway", BlockEntityType::EndGateway},
    {"minecraft:end_portal", BlockEntityType::EndPortal},
    {"minecraft:mob_spawner", BlockEntityType::MobSpawner},
    {"minecraft:skull", BlockEntityType::Skull},
    {"minecraft:beehive", BlockEntityType::Beehive},
    {"minecraft:campfire", BlockEntityType::Campfire},
    {"minecraft:conduit", BlockEntityType::Conduit},
    {"minecraft:lectern", BlockEntityType::Lectern},
    {"minecraft:jukebox", BlockEntityType::Jukebox},
    {"minecraft:shelf", BlockEntityType::Shelf},
    {"minecraft:trial_spawner", BlockEntityType::TrialSpawner},
    {"minecraft:vault", BlockEntityType::Vault},
    {"minecraft:crafter", BlockEntityType::Crafter},
    {"minecraft:sculk_sensor", BlockEntityType::SculkSensor},
    {"minecraft:sculk_shrieker", BlockEntityType::SculkShrieker},
    {"minecraft:decorated_pot", BlockEntityType::DecoratedPot},
    {"minecraft:brushable_block", BlockEntityType::BrushableBlock},
    {"minecraft:copper_golem_statue", BlockEntityType::CopperGolemStatue},
    // 简写形式
    {"chest", BlockEntityType::Chest},
    {"furnace", BlockEntityType::Furnace},
    {"blast_furnace", BlockEntityType::BlastFurnace},
    {"smoker", BlockEntityType::Smoker},
    {"hopper", BlockEntityType::Hopper},
    {"dispenser", BlockEntityType::Dispenser},
    {"dropper", BlockEntityType::Dropper}};
} // namespace

ResourceLocation blockEntityTypeToId(BlockEntityType type)
{
    auto it = typeToIdMap.find(type);
    if (it != typeToIdMap.end()) {
        return it->second;
    }
    return ResourceLocation("minecraft", "unknown");
}

BlockEntityType blockEntityTypeFromId(const ResourceLocation& id)
{
    std::string idStr = id.toString();
    auto it = idToTypeMap.find(idStr);
    if (it != idToTypeMap.end()) {
        return it->second;
    }
    return BlockEntityType::Unknown;
}

} // namespace mc
