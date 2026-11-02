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

#include "world/block/registry/HarvestToolTable.hpp"

#include "common/world/block/BlockTags.hpp"

#include <algorithm>
#include <unordered_set>
#include <vector>

#include <spdlog/spdlog.h>

namespace mc {
namespace block_registry {

namespace {

/// 工具标签名 → HarvestTool 常量
struct ToolTagBinding {
    const char* tag;
    u8 tool;
};

// 原版数据包只有这四个 mineable 标签（mineable/shears 不存在，剪刀的适用范围由
// ShearsItem 自己判定），这里不要凭空补一个。
constexpr ToolTagBinding kToolTags[] = {
    {"mineable/pickaxe", HarvestTool::Pickaxe},
    {"mineable/axe", HarvestTool::Axe},
    {"mineable/shovel", HarvestTool::Shovel},
    {"mineable/hoe", HarvestTool::Hoe},
};

/// 最低档位标签名 → 档位值（needs_stone 及以上才需要更高的工具）
struct LevelTagBinding {
    const char* tag;
    i32 level;
};

constexpr LevelTagBinding kLevelTags[] = {
    {"needs_stone_tool", 1},
    {"needs_iron_tool", 2},
    {"needs_diamond_tool", 3},
};

} // namespace

namespace {

/**
 * 原版「需要正确工具才掉落」（requiresCorrectToolForDrops）的方块清单。
 *
 * 这一位**不能从数据包推导**：原版的 mineable/<tool> 标签是 833 个方块的并集，
 * 其中只有 411 个真正要求正确工具，其余（按钮、门、活板门、泥土、树叶……）空手
 * 挖掉照样掉落。标签只回答「算不算这种工具的有效方块」，不回答「是否强制」。
 * 该属性在原版是方块注册时硬编码的，数据包里没有任何文件表达它，因此这里以快照形式
 * 内嵌；数值来自原版 1.21.11 jar 的实测（空手与 31 种工具逐一尝试）。
 */
constexpr const char* kRequiresCorrectTool[] = {"amethyst_block",
    "ancient_debris",
    "andesite",
    "andesite_slab",
    "andesite_stairs",
    "andesite_wall",
    "anvil",
    "basalt",
    "black_concrete",
    "black_glazed_terracotta",
    "black_terracotta",
    "blackstone",
    "blackstone_slab",
    "blackstone_stairs",
    "blackstone_wall",
    "blast_furnace",
    "blue_concrete",
    "blue_glazed_terracotta",
    "blue_terracotta",
    "bone_block",
    "brain_coral_block",
    "brick_slab",
    "brick_stairs",
    "brick_wall",
    "bricks",
    "brown_concrete",
    "brown_glazed_terracotta",
    "brown_terracotta",
    "bubble_coral_block",
    "budding_amethyst",
    "calcite",
    "cauldron",
    "chain_command_block",
    "chipped_anvil",
    "chiseled_copper",
    "chiseled_deepslate",
    "chiseled_nether_bricks",
    "chiseled_polished_blackstone",
    "chiseled_quartz_block",
    "chiseled_red_sandstone",
    "chiseled_resin_bricks",
    "chiseled_sandstone",
    "chiseled_stone_bricks",
    "chiseled_tuff",
    "chiseled_tuff_bricks",
    "coal_block",
    "coal_ore",
    "cobbled_deepslate",
    "cobbled_deepslate_slab",
    "cobbled_deepslate_stairs",
    "cobbled_deepslate_wall",
    "cobblestone",
    "cobblestone_slab",
    "cobblestone_stairs",
    "cobblestone_wall",
    "cobweb",
    "command_block",
    "copper_bars",
    "copper_block",
    "copper_bulb",
    "copper_chain",
    "copper_chest",
    "copper_grate",
    "copper_ore",
    "copper_trapdoor",
    "cracked_deepslate_bricks",
    "cracked_deepslate_tiles",
    "cracked_nether_bricks",
    "cracked_polished_blackstone_bricks",
    "cracked_stone_bricks",
    "crimson_nylium",
    "crying_obsidian",
    "cut_copper",
    "cut_copper_slab",
    "cut_copper_stairs",
    "cut_red_sandstone",
    "cut_red_sandstone_slab",
    "cut_sandstone",
    "cut_sandstone_slab",
    "cyan_concrete",
    "cyan_glazed_terracotta",
    "cyan_terracotta",
    "damaged_anvil",
    "dark_prismarine",
    "dark_prismarine_slab",
    "dark_prismarine_stairs",
    "dead_brain_coral",
    "dead_brain_coral_block",
    "dead_brain_coral_fan",
    "dead_brain_coral_wall_fan",
    "dead_bubble_coral",
    "dead_bubble_coral_block",
    "dead_bubble_coral_fan",
    "dead_bubble_coral_wall_fan",
    "dead_fire_coral",
    "dead_fire_coral_block",
    "dead_fire_coral_fan",
    "dead_fire_coral_wall_fan",
    "dead_horn_coral",
    "dead_horn_coral_block",
    "dead_horn_coral_fan",
    "dead_horn_coral_wall_fan",
    "dead_tube_coral",
    "dead_tube_coral_block",
    "dead_tube_coral_fan",
    "dead_tube_coral_wall_fan",
    "deepslate",
    "deepslate_brick_slab",
    "deepslate_brick_stairs",
    "deepslate_brick_wall",
    "deepslate_bricks",
    "deepslate_coal_ore",
    "deepslate_copper_ore",
    "deepslate_diamond_ore",
    "deepslate_emerald_ore",
    "deepslate_gold_ore",
    "deepslate_iron_ore",
    "deepslate_lapis_ore",
    "deepslate_redstone_ore",
    "deepslate_tile_slab",
    "deepslate_tile_stairs",
    "deepslate_tile_wall",
    "deepslate_tiles",
    "diamond_block",
    "diamond_ore",
    "diorite",
    "diorite_slab",
    "diorite_stairs",
    "diorite_wall",
    "dispenser",
    "dripstone_block",
    "dropper",
    "emerald_block",
    "emerald_ore",
    "enchanting_table",
    "end_stone",
    "end_stone_brick_slab",
    "end_stone_brick_stairs",
    "end_stone_brick_wall",
    "end_stone_bricks",
    "exposed_chiseled_copper",
    "exposed_copper",
    "exposed_copper_bars",
    "exposed_copper_bulb",
    "exposed_copper_chain",
    "exposed_copper_chest",
    "exposed_copper_grate",
    "exposed_copper_trapdoor",
    "exposed_cut_copper",
    "exposed_cut_copper_slab",
    "exposed_cut_copper_stairs",
    "exposed_lightning_rod",
    "fire_coral_block",
    "furnace",
    "gilded_blackstone",
    "gold_block",
    "gold_ore",
    "granite",
    "granite_slab",
    "granite_stairs",
    "granite_wall",
    "gray_concrete",
    "gray_glazed_terracotta",
    "gray_terracotta",
    "green_concrete",
    "green_glazed_terracotta",
    "green_terracotta",
    "grindstone",
    "hopper",
    "horn_coral_block",
    "iron_bars",
    "iron_block",
    "iron_chain",
    "iron_ore",
    "iron_trapdoor",
    "jigsaw",
    "lapis_block",
    "lapis_ore",
    "lava_cauldron",
    "light_blue_concrete",
    "light_blue_glazed_terracotta",
    "light_blue_terracotta",
    "light_gray_concrete",
    "light_gray_glazed_terracotta",
    "light_gray_terracotta",
    "lightning_rod",
    "lime_concrete",
    "lime_glazed_terracotta",
    "lime_terracotta",
    "lodestone",
    "magenta_concrete",
    "magenta_glazed_terracotta",
    "magenta_terracotta",
    "magma_block",
    "mossy_cobblestone",
    "mossy_cobblestone_slab",
    "mossy_cobblestone_stairs",
    "mossy_cobblestone_wall",
    "mossy_stone_brick_slab",
    "mossy_stone_brick_stairs",
    "mossy_stone_brick_wall",
    "mossy_stone_bricks",
    "mud_brick_slab",
    "mud_brick_stairs",
    "mud_brick_wall",
    "mud_bricks",
    "nether_brick_fence",
    "nether_brick_slab",
    "nether_brick_stairs",
    "nether_brick_wall",
    "nether_bricks",
    "nether_gold_ore",
    "nether_quartz_ore",
    "netherite_block",
    "netherrack",
    "observer",
    "obsidian",
    "orange_concrete",
    "orange_glazed_terracotta",
    "orange_terracotta",
    "oxidized_chiseled_copper",
    "oxidized_copper",
    "oxidized_copper_bars",
    "oxidized_copper_bulb",
    "oxidized_copper_chain",
    "oxidized_copper_chest",
    "oxidized_copper_grate",
    "oxidized_copper_trapdoor",
    "oxidized_cut_copper",
    "oxidized_cut_copper_slab",
    "oxidized_cut_copper_stairs",
    "oxidized_lightning_rod",
    "petrified_oak_slab",
    "pink_concrete",
    "pink_glazed_terracotta",
    "pink_terracotta",
    "polished_andesite",
    "polished_andesite_slab",
    "polished_andesite_stairs",
    "polished_basalt",
    "polished_blackstone",
    "polished_blackstone_brick_slab",
    "polished_blackstone_brick_stairs",
    "polished_blackstone_brick_wall",
    "polished_blackstone_bricks",
    "polished_blackstone_slab",
    "polished_blackstone_stairs",
    "polished_blackstone_wall",
    "polished_deepslate",
    "polished_deepslate_slab",
    "polished_deepslate_stairs",
    "polished_deepslate_wall",
    "polished_diorite",
    "polished_diorite_slab",
    "polished_diorite_stairs",
    "polished_granite",
    "polished_granite_slab",
    "polished_granite_stairs",
    "polished_tuff",
    "polished_tuff_slab",
    "polished_tuff_stairs",
    "polished_tuff_wall",
    "powder_snow_cauldron",
    "prismarine",
    "prismarine_brick_slab",
    "prismarine_brick_stairs",
    "prismarine_bricks",
    "prismarine_slab",
    "prismarine_stairs",
    "prismarine_wall",
    "purple_concrete",
    "purple_glazed_terracotta",
    "purple_terracotta",
    "purpur_block",
    "purpur_pillar",
    "purpur_slab",
    "purpur_stairs",
    "quartz_block",
    "quartz_bricks",
    "quartz_pillar",
    "quartz_slab",
    "quartz_stairs",
    "raw_copper_block",
    "raw_gold_block",
    "raw_iron_block",
    "red_concrete",
    "red_glazed_terracotta",
    "red_nether_brick_slab",
    "red_nether_brick_stairs",
    "red_nether_brick_wall",
    "red_nether_bricks",
    "red_sandstone",
    "red_sandstone_slab",
    "red_sandstone_stairs",
    "red_sandstone_wall",
    "red_terracotta",
    "redstone_block",
    "redstone_ore",
    "repeating_command_block",
    "resin_brick_slab",
    "resin_brick_stairs",
    "resin_brick_wall",
    "resin_bricks",
    "respawn_anchor",
    "sandstone",
    "sandstone_slab",
    "sandstone_stairs",
    "sandstone_wall",
    "smoker",
    "smooth_basalt",
    "smooth_quartz",
    "smooth_quartz_slab",
    "smooth_quartz_stairs",
    "smooth_red_sandstone",
    "smooth_red_sandstone_slab",
    "smooth_red_sandstone_stairs",
    "smooth_sandstone",
    "smooth_sandstone_slab",
    "smooth_sandstone_stairs",
    "smooth_stone",
    "smooth_stone_slab",
    "snow",
    "snow_block",
    "spawner",
    "stone",
    "stone_brick_slab",
    "stone_brick_stairs",
    "stone_brick_wall",
    "stone_bricks",
    "stone_slab",
    "stone_stairs",
    "stonecutter",
    "structure_block",
    "terracotta",
    "tube_coral_block",
    "tuff",
    "tuff_brick_slab",
    "tuff_brick_stairs",
    "tuff_brick_wall",
    "tuff_bricks",
    "tuff_slab",
    "tuff_stairs",
    "tuff_wall",
    "warped_nylium",
    "water_cauldron",
    "waxed_chiseled_copper",
    "waxed_copper_bars",
    "waxed_copper_block",
    "waxed_copper_bulb",
    "waxed_copper_chain",
    "waxed_copper_chest",
    "waxed_copper_grate",
    "waxed_copper_trapdoor",
    "waxed_cut_copper",
    "waxed_cut_copper_slab",
    "waxed_cut_copper_stairs",
    "waxed_exposed_chiseled_copper",
    "waxed_exposed_copper",
    "waxed_exposed_copper_bars",
    "waxed_exposed_copper_bulb",
    "waxed_exposed_copper_chain",
    "waxed_exposed_copper_chest",
    "waxed_exposed_copper_grate",
    "waxed_exposed_copper_trapdoor",
    "waxed_exposed_cut_copper",
    "waxed_exposed_cut_copper_slab",
    "waxed_exposed_cut_copper_stairs",
    "waxed_exposed_lightning_rod",
    "waxed_lightning_rod",
    "waxed_oxidized_chiseled_copper",
    "waxed_oxidized_copper",
    "waxed_oxidized_copper_bars",
    "waxed_oxidized_copper_bulb",
    "waxed_oxidized_copper_chain",
    "waxed_oxidized_copper_chest",
    "waxed_oxidized_copper_grate",
    "waxed_oxidized_copper_trapdoor",
    "waxed_oxidized_cut_copper",
    "waxed_oxidized_cut_copper_slab",
    "waxed_oxidized_cut_copper_stairs",
    "waxed_oxidized_lightning_rod",
    "waxed_weathered_chiseled_copper",
    "waxed_weathered_copper",
    "waxed_weathered_copper_bars",
    "waxed_weathered_copper_bulb",
    "waxed_weathered_copper_chain",
    "waxed_weathered_copper_chest",
    "waxed_weathered_copper_grate",
    "waxed_weathered_copper_trapdoor",
    "waxed_weathered_cut_copper",
    "waxed_weathered_cut_copper_slab",
    "waxed_weathered_cut_copper_stairs",
    "waxed_weathered_lightning_rod",
    "weathered_chiseled_copper",
    "weathered_copper",
    "weathered_copper_bars",
    "weathered_copper_bulb",
    "weathered_copper_chain",
    "weathered_copper_chest",
    "weathered_copper_grate",
    "weathered_copper_trapdoor",
    "weathered_cut_copper",
    "weathered_cut_copper_slab",
    "weathered_cut_copper_stairs",
    "weathered_lightning_rod",
    "white_concrete",
    "white_glazed_terracotta",
    "white_terracotta",
    "yellow_concrete",
    "yellow_glazed_terracotta",
    "yellow_terracotta"};

/**
 * 原版在物品 `Tool` 组件里硬编码的采集规则（数据包标签之外的部分）。
 *
 * 原版大多数工具的规则来自 `Tool.Rule.minesAndDrops(标签)`，但剑与少数方块是直接写死
 * 方块 ID 的：`ToolMaterial.applySwordProperties` 只给 COBWEB 一条 minesAndDrops 规则，
 * 因此剑（以及其它任何工具）只有在蜘蛛网上才「正确」。蜘蛛网本身要求正确工具
 * （requiresCorrectToolForDrops），于是生存中只有剑能正常获得掉落。
 */
struct HardcodedRule {
    const char* block;
    u8 tool;
    i32 level;
};

constexpr HardcodedRule kHardcodedRules[] = {
    {"cobweb", HarvestTool::Sword, 0},
};

} // namespace

bool HarvestToolTable::requiresCorrectToolForDrops(const ResourceLocation& blockId)
{
    static const std::unordered_set<std::string> kSet = [] {
        std::unordered_set<std::string> s;
        for (const char* name : kRequiresCorrectTool) {
            s.emplace(name);
        }
        return s;
    }();
    return kSet.find(blockId.path()) != kSet.end();
}

std::unordered_map<ResourceLocation, HarvestToolRuleSet>& HarvestToolTable::rules()
{
    static std::unordered_map<ResourceLocation, HarvestToolRuleSet> s_rules;
    return s_rules;
}

void HarvestToolTable::clear()
{
    rules().clear();
}

void HarvestToolTable::rebuild()
{
    clear();
    auto& table = rules();

    // 1. 先按 mineable/<tool> 标签登记「该方块需要这种工具」，档位先记 0（任意档位）。
    for (const ToolTagBinding& binding : kToolTags) {
        const BlockTag* tag = BlockTags::getTag(ResourceLocation("minecraft", binding.tag));
        if (tag == nullptr) {
            // 数据包缺失该标签属于配置错误，但不至于影响其他工具，记录后继续。
            spdlog::warn("HarvestToolTable: data pack is missing tag #minecraft:{}", binding.tag);
            continue;
        }
        std::unordered_set<ResourceLocation> blocks;
        std::unordered_set<ResourceLocation> referencedTags;
        std::unordered_set<ResourceLocation> visited;
        tag->collectIncludingReferences(blocks, referencedTags, visited);
        for (const ResourceLocation& blockId : blocks) {
            HarvestToolRuleSet& set = table[blockId];
            bool already = false;
            for (u8 i = 0; i < set.count; ++i) {
                if (set.entries[i].tool == binding.tool) {
                    already = true;
                    break;
                }
            }
            if (already) {
                continue;
            }
            if (set.count >= 4) {
                // 原版同一方块最多同时属于 4 种工具类别；超出说明标签异常，丢弃多余条目。
                spdlog::warn("HarvestToolTable: block {} belongs to more than 4 tool categories", blockId.toString());
                continue;
            }
            set.entries[set.count].tool = binding.tool;
            set.entries[set.count].level = 0;
            ++set.count;
        }
    }

    // 2. 再按 needs_* 标签抬升档位。该标签的成员必然是某个 mineable 标签的成员
    //    （原版就是这么组织的），否则它形同虚设，只记录不建条目。
    for (const LevelTagBinding& binding : kLevelTags) {
        const BlockTag* tag = BlockTags::getTag(ResourceLocation("minecraft", binding.tag));
        if (tag == nullptr) {
            spdlog::warn("HarvestToolTable: data pack is missing tag #minecraft:{}", binding.tag);
            continue;
        }
        std::unordered_set<ResourceLocation> blocks;
        std::unordered_set<ResourceLocation> referencedTags;
        std::unordered_set<ResourceLocation> visited;
        tag->collectIncludingReferences(blocks, referencedTags, visited);
        for (const ResourceLocation& blockId : blocks) {
            auto it = table.find(blockId);
            if (it == table.end()) {
                spdlog::warn("HarvestToolTable: block {} is in #minecraft:{} but not in any mineable tag",
                    blockId.toString(),
                    binding.tag);
                continue;
            }
            for (u8 i = 0; i < it->second.count; ++i) {
                it->second.entries[i].level = std::max(it->second.entries[i].level, binding.level);
            }
        }
    }

    // 3. 合并原版物品侧的硬编码规则（不在数据包标签里，见 kHardcodedRules 说明）。
    for (const HardcodedRule& rule : kHardcodedRules) {
        HarvestToolRuleSet& set = table[ResourceLocation("minecraft", rule.block)];
        bool already = false;
        for (u8 i = 0; i < set.count; ++i) {
            if (set.entries[i].tool == rule.tool) {
                already = true;
                break;
            }
        }
        if (already || set.count >= 4) {
            continue;
        }
        set.entries[set.count].tool = rule.tool;
        set.entries[set.count].level = rule.level;
        ++set.count;
    }

    spdlog::info("HarvestToolTable: built harvest rules for {} blocks from data pack tags", table.size());
}

const HarvestToolRuleSet* HarvestToolTable::find(const ResourceLocation& blockId)
{
    auto& table = rules();
    auto it = table.find(blockId);
    return it != table.end() ? &it->second : nullptr;
}

const HarvestToolRuleSet* harvestToolRuleSetFor(const ResourceLocation& blockId)
{
    return HarvestToolTable::find(blockId);
}

bool harvestToolRuleSetHas(const HarvestToolRuleSet& rules, u8 tool) noexcept
{
    for (u8 i = 0; i < rules.count; ++i) {
        if (rules.entries[i].tool == tool) {
            return true;
        }
    }
    return false;
}

i32 harvestToolRuleLevel(const HarvestToolRuleSet& rules, u8 tool) noexcept
{
    for (u8 i = 0; i < rules.count; ++i) {
        if (rules.entries[i].tool == tool) {
            return rules.entries[i].level;
        }
    }
    return -1;
}

} // namespace block_registry
} // namespace mc
