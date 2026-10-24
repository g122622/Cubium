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

// 本文件由 scripts/baking/gen_item_enchantable.py 生成，请勿手改。
// 数据来源：原版 1.21.11 服务端 jar 的 DataComponents.ENCHANTABLE（见
// tests/diff/java/dev/horizonlab/diff/ItemEnchantableExport.java）。只含非 0 项。

#include "VanillaItemEnchantable.hpp"

#include <string_view>
#include <unordered_map>

namespace mc {

namespace {

/// 物品名 → 原版附魔能力（只含非 0 项）。
const std::unordered_map<std::string_view, i32>& enchantableTable()
{
    static const std::unordered_map<std::string_view, i32> table = {
        {"minecraft:book", 1},
        {"minecraft:bow", 1},
        {"minecraft:chainmail_boots", 12},
        {"minecraft:chainmail_chestplate", 12},
        {"minecraft:chainmail_helmet", 12},
        {"minecraft:chainmail_leggings", 12},
        {"minecraft:copper_axe", 13},
        {"minecraft:copper_boots", 8},
        {"minecraft:copper_chestplate", 8},
        {"minecraft:copper_helmet", 8},
        {"minecraft:copper_hoe", 13},
        {"minecraft:copper_leggings", 8},
        {"minecraft:copper_pickaxe", 13},
        {"minecraft:copper_shovel", 13},
        {"minecraft:copper_spear", 13},
        {"minecraft:copper_sword", 13},
        {"minecraft:crossbow", 1},
        {"minecraft:diamond_axe", 10},
        {"minecraft:diamond_boots", 10},
        {"minecraft:diamond_chestplate", 10},
        {"minecraft:diamond_helmet", 10},
        {"minecraft:diamond_hoe", 10},
        {"minecraft:diamond_leggings", 10},
        {"minecraft:diamond_pickaxe", 10},
        {"minecraft:diamond_shovel", 10},
        {"minecraft:diamond_spear", 10},
        {"minecraft:diamond_sword", 10},
        {"minecraft:fishing_rod", 1},
        {"minecraft:golden_axe", 22},
        {"minecraft:golden_boots", 25},
        {"minecraft:golden_chestplate", 25},
        {"minecraft:golden_helmet", 25},
        {"minecraft:golden_hoe", 22},
        {"minecraft:golden_leggings", 25},
        {"minecraft:golden_pickaxe", 22},
        {"minecraft:golden_shovel", 22},
        {"minecraft:golden_spear", 22},
        {"minecraft:golden_sword", 22},
        {"minecraft:iron_axe", 14},
        {"minecraft:iron_boots", 9},
        {"minecraft:iron_chestplate", 9},
        {"minecraft:iron_helmet", 9},
        {"minecraft:iron_hoe", 14},
        {"minecraft:iron_leggings", 9},
        {"minecraft:iron_pickaxe", 14},
        {"minecraft:iron_shovel", 14},
        {"minecraft:iron_spear", 14},
        {"minecraft:iron_sword", 14},
        {"minecraft:leather_boots", 15},
        {"minecraft:leather_chestplate", 15},
        {"minecraft:leather_helmet", 15},
        {"minecraft:leather_leggings", 15},
        {"minecraft:mace", 15},
        {"minecraft:netherite_axe", 15},
        {"minecraft:netherite_boots", 15},
        {"minecraft:netherite_chestplate", 15},
        {"minecraft:netherite_helmet", 15},
        {"minecraft:netherite_hoe", 15},
        {"minecraft:netherite_leggings", 15},
        {"minecraft:netherite_pickaxe", 15},
        {"minecraft:netherite_shovel", 15},
        {"minecraft:netherite_spear", 15},
        {"minecraft:netherite_sword", 15},
        {"minecraft:stone_axe", 5},
        {"minecraft:stone_hoe", 5},
        {"minecraft:stone_pickaxe", 5},
        {"minecraft:stone_shovel", 5},
        {"minecraft:stone_spear", 5},
        {"minecraft:stone_sword", 5},
        {"minecraft:trident", 1},
        {"minecraft:turtle_helmet", 9},
        {"minecraft:wooden_axe", 15},
        {"minecraft:wooden_hoe", 15},
        {"minecraft:wooden_pickaxe", 15},
        {"minecraft:wooden_shovel", 15},
        {"minecraft:wooden_spear", 15},
        {"minecraft:wooden_sword", 15},
    };
    return table;
}

} // namespace

i32 vanillaItemEnchantable(std::string_view itemName)
{
    const auto& table = enchantableTable();
    const auto found = table.find(itemName);
    return found == table.end() ? 0 : found->second;
}

} // namespace mc
