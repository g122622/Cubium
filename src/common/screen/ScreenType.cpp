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

#include "screen/ScreenType.hpp"
#include <string>
#include <unordered_map>

namespace mc {

namespace {
const std::unordered_map<ScreenType, std::string> typeToIdMap = {{ScreenType::Inventory, "minecraft:inventory"},
    {ScreenType::CreativeInventory, "minecraft:creative_inventory"},
    {ScreenType::Chest, "minecraft:chest"},
    {ScreenType::DoubleChest, "minecraft:double_chest"},
    {ScreenType::ShulkerBox, "minecraft:shulker_box"},
    {ScreenType::Barrel, "minecraft:barrel"},
    {ScreenType::CraftingTable, "minecraft:crafting_table"},
    {ScreenType::Furnace, "minecraft:furnace"},
    {ScreenType::BlastFurnace, "minecraft:blast_furnace"},
    {ScreenType::Smoker, "minecraft:smoker"},
    {ScreenType::Anvil, "minecraft:anvil"},
    {ScreenType::Grindstone, "minecraft:grindstone"},
    {ScreenType::Stonecutter, "minecraft:stonecutter"},
    {ScreenType::SmithingTable, "minecraft:smithing_table"},
    {ScreenType::Loom, "minecraft:loom"},
    {ScreenType::CartographyTable, "minecraft:cartography_table"},
    {ScreenType::BrewingStand, "minecraft:brewing_stand"},
    {ScreenType::EnchantingScreen, "minecraft:enchanting_table"},
    {ScreenType::Dispenser, "minecraft:dispenser"},
    {ScreenType::Dropper, "minecraft:dropper"},
    {ScreenType::Hopper, "minecraft:hopper"},
    {ScreenType::Beacon, "minecraft:beacon"},
    {ScreenType::Sign, "minecraft:sign"},
    {ScreenType::CommandBlock, "minecraft:command_block"},
    {ScreenType::StructureBlock, "minecraft:structure_block"},
    {ScreenType::JigsawBlock, "minecraft:jigsaw"}};

const std::unordered_map<std::string, ScreenType> idToTypeMap = {{"minecraft:inventory", ScreenType::Inventory},
    {"minecraft:creative_inventory", ScreenType::CreativeInventory},
    {"minecraft:chest", ScreenType::Chest},
    {"minecraft:double_chest", ScreenType::DoubleChest},
    {"minecraft:shulker_box", ScreenType::ShulkerBox},
    {"minecraft:barrel", ScreenType::Barrel},
    {"minecraft:crafting_table", ScreenType::CraftingTable},
    {"minecraft:furnace", ScreenType::Furnace},
    {"minecraft:blast_furnace", ScreenType::BlastFurnace},
    {"minecraft:smoker", ScreenType::Smoker},
    {"minecraft:anvil", ScreenType::Anvil},
    {"minecraft:grindstone", ScreenType::Grindstone},
    {"minecraft:stonecutter", ScreenType::Stonecutter},
    {"minecraft:smithing_table", ScreenType::SmithingTable},
    {"minecraft:loom", ScreenType::Loom},
    {"minecraft:cartography_table", ScreenType::CartographyTable},
    {"minecraft:brewing_stand", ScreenType::BrewingStand},
    {"minecraft:enchanting_table", ScreenType::EnchantingScreen},
    {"minecraft:dispenser", ScreenType::Dispenser},
    {"minecraft:dropper", ScreenType::Dropper},
    {"minecraft:hopper", ScreenType::Hopper},
    {"minecraft:beacon", ScreenType::Beacon},
    {"minecraft:sign", ScreenType::Sign},
    {"minecraft:command_block", ScreenType::CommandBlock},
    {"minecraft:structure_block", ScreenType::StructureBlock},
    {"minecraft:jigsaw", ScreenType::JigsawBlock},
    // 简写形式
    {"inventory", ScreenType::Inventory},
    {"chest", ScreenType::Chest},
    {"crafting_table", ScreenType::CraftingTable},
    {"furnace", ScreenType::Furnace},
    {"hopper", ScreenType::Hopper}};
} // namespace

std::string screenTypeToId(ScreenType type)
{
    auto it = typeToIdMap.find(type);
    if (it != typeToIdMap.end()) {
        return it->second;
    }
    return "minecraft:unknown";
}

ScreenType screenTypeFromId(const std::string& id)
{
    auto it = idToTypeMap.find(id);
    if (it != idToTypeMap.end()) {
        return it->second;
    }
    return ScreenType::Unknown;
}

} // namespace mc
