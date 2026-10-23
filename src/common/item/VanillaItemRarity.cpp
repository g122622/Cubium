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

// 本文件由 scripts/baking/gen_item_rarity.py 生成，请勿手改。
// 数据来源：原版 1.21.11 服务端 jar 的 DataComponents.RARITY（见
// tests/diff/java/dev/horizonlab/diff/ItemRarityExport.java）。只含非 common 项。

#include "VanillaItemRarity.hpp"

#include <string_view>
#include <unordered_map>

namespace mc {

namespace {

/// 物品名 → 原版稀有度（只含非 common 项）。
const std::unordered_map<std::string_view, ItemRarity>& rarityTable()
{
    static const std::unordered_map<std::string_view, ItemRarity> table = {
        {"minecraft:angler_pottery_sherd", ItemRarity::Uncommon},
        {"minecraft:archer_pottery_sherd", ItemRarity::Uncommon},
        {"minecraft:arms_up_pottery_sherd", ItemRarity::Uncommon},
        {"minecraft:barrier", ItemRarity::Epic},
        {"minecraft:beacon", ItemRarity::Rare},
        {"minecraft:blade_pottery_sherd", ItemRarity::Uncommon},
        {"minecraft:bolt_armor_trim_smithing_template", ItemRarity::Uncommon},
        {"minecraft:brewer_pottery_sherd", ItemRarity::Uncommon},
        {"minecraft:burn_pottery_sherd", ItemRarity::Uncommon},
        {"minecraft:chain_command_block", ItemRarity::Epic},
        {"minecraft:chainmail_boots", ItemRarity::Uncommon},
        {"minecraft:chainmail_chestplate", ItemRarity::Uncommon},
        {"minecraft:chainmail_helmet", ItemRarity::Uncommon},
        {"minecraft:chainmail_leggings", ItemRarity::Uncommon},
        {"minecraft:coast_armor_trim_smithing_template", ItemRarity::Uncommon},
        {"minecraft:command_block", ItemRarity::Epic},
        {"minecraft:command_block_minecart", ItemRarity::Epic},
        {"minecraft:conduit", ItemRarity::Uncommon},
        {"minecraft:creeper_banner_pattern", ItemRarity::Uncommon},
        {"minecraft:creeper_head", ItemRarity::Uncommon},
        {"minecraft:danger_pottery_sherd", ItemRarity::Uncommon},
        {"minecraft:debug_stick", ItemRarity::Epic},
        {"minecraft:disc_fragment_5", ItemRarity::Uncommon},
        {"minecraft:dragon_breath", ItemRarity::Uncommon},
        {"minecraft:dragon_egg", ItemRarity::Epic},
        {"minecraft:dragon_head", ItemRarity::Epic},
        {"minecraft:dune_armor_trim_smithing_template", ItemRarity::Uncommon},
        {"minecraft:echo_shard", ItemRarity::Uncommon},
        {"minecraft:elytra", ItemRarity::Epic},
        {"minecraft:enchanted_book", ItemRarity::Rare},
        {"minecraft:enchanted_golden_apple", ItemRarity::Rare},
        {"minecraft:experience_bottle", ItemRarity::Uncommon},
        {"minecraft:explorer_pottery_sherd", ItemRarity::Uncommon},
        {"minecraft:eye_armor_trim_smithing_template", ItemRarity::Rare},
        {"minecraft:flow_armor_trim_smithing_template", ItemRarity::Uncommon},
        {"minecraft:flow_banner_pattern", ItemRarity::Rare},
        {"minecraft:flow_pottery_sherd", ItemRarity::Uncommon},
        {"minecraft:friend_pottery_sherd", ItemRarity::Uncommon},
        {"minecraft:goat_horn", ItemRarity::Uncommon},
        {"minecraft:guster_banner_pattern", ItemRarity::Rare},
        {"minecraft:guster_pottery_sherd", ItemRarity::Uncommon},
        {"minecraft:heart_of_the_sea", ItemRarity::Uncommon},
        {"minecraft:heart_pottery_sherd", ItemRarity::Uncommon},
        {"minecraft:heartbreak_pottery_sherd", ItemRarity::Uncommon},
        {"minecraft:heavy_core", ItemRarity::Epic},
        {"minecraft:host_armor_trim_smithing_template", ItemRarity::Uncommon},
        {"minecraft:howl_pottery_sherd", ItemRarity::Uncommon},
        {"minecraft:jigsaw", ItemRarity::Epic},
        {"minecraft:knowledge_book", ItemRarity::Epic},
        {"minecraft:light", ItemRarity::Epic},
        {"minecraft:mace", ItemRarity::Epic},
        {"minecraft:miner_pottery_sherd", ItemRarity::Uncommon},
        {"minecraft:mojang_banner_pattern", ItemRarity::Rare},
        {"minecraft:mourner_pottery_sherd", ItemRarity::Uncommon},
        {"minecraft:music_disc_11", ItemRarity::Uncommon},
        {"minecraft:music_disc_13", ItemRarity::Uncommon},
        {"minecraft:music_disc_5", ItemRarity::Uncommon},
        {"minecraft:music_disc_blocks", ItemRarity::Uncommon},
        {"minecraft:music_disc_cat", ItemRarity::Uncommon},
        {"minecraft:music_disc_chirp", ItemRarity::Uncommon},
        {"minecraft:music_disc_creator", ItemRarity::Rare},
        {"minecraft:music_disc_creator_music_box", ItemRarity::Uncommon},
        {"minecraft:music_disc_far", ItemRarity::Uncommon},
        {"minecraft:music_disc_lava_chicken", ItemRarity::Rare},
        {"minecraft:music_disc_mall", ItemRarity::Uncommon},
        {"minecraft:music_disc_mellohi", ItemRarity::Uncommon},
        {"minecraft:music_disc_otherside", ItemRarity::Rare},
        {"minecraft:music_disc_pigstep", ItemRarity::Rare},
        {"minecraft:music_disc_precipice", ItemRarity::Uncommon},
        {"minecraft:music_disc_relic", ItemRarity::Uncommon},
        {"minecraft:music_disc_stal", ItemRarity::Uncommon},
        {"minecraft:music_disc_strad", ItemRarity::Uncommon},
        {"minecraft:music_disc_tears", ItemRarity::Uncommon},
        {"minecraft:music_disc_wait", ItemRarity::Uncommon},
        {"minecraft:music_disc_ward", ItemRarity::Uncommon},
        {"minecraft:nautilus_shell", ItemRarity::Uncommon},
        {"minecraft:nether_star", ItemRarity::Rare},
        {"minecraft:netherite_upgrade_smithing_template", ItemRarity::Uncommon},
        {"minecraft:ominous_bottle", ItemRarity::Uncommon},
        {"minecraft:piglin_banner_pattern", ItemRarity::Uncommon},
        {"minecraft:piglin_head", ItemRarity::Uncommon},
        {"minecraft:player_head", ItemRarity::Uncommon},
        {"minecraft:plenty_pottery_sherd", ItemRarity::Uncommon},
        {"minecraft:prize_pottery_sherd", ItemRarity::Uncommon},
        {"minecraft:raiser_armor_trim_smithing_template", ItemRarity::Uncommon},
        {"minecraft:recovery_compass", ItemRarity::Uncommon},
        {"minecraft:repeating_command_block", ItemRarity::Epic},
        {"minecraft:rib_armor_trim_smithing_template", ItemRarity::Uncommon},
        {"minecraft:scrape_pottery_sherd", ItemRarity::Uncommon},
        {"minecraft:sentry_armor_trim_smithing_template", ItemRarity::Uncommon},
        {"minecraft:shaper_armor_trim_smithing_template", ItemRarity::Uncommon},
        {"minecraft:sheaf_pottery_sherd", ItemRarity::Uncommon},
        {"minecraft:shelter_pottery_sherd", ItemRarity::Uncommon},
        {"minecraft:silence_armor_trim_smithing_template", ItemRarity::Epic},
        {"minecraft:skeleton_skull", ItemRarity::Uncommon},
        {"minecraft:skull_banner_pattern", ItemRarity::Rare},
        {"minecraft:skull_pottery_sherd", ItemRarity::Uncommon},
        {"minecraft:sniffer_egg", ItemRarity::Uncommon},
        {"minecraft:snort_pottery_sherd", ItemRarity::Uncommon},
        {"minecraft:snout_armor_trim_smithing_template", ItemRarity::Uncommon},
        {"minecraft:spire_armor_trim_smithing_template", ItemRarity::Rare},
        {"minecraft:structure_block", ItemRarity::Epic},
        {"minecraft:structure_void", ItemRarity::Epic},
        {"minecraft:test_block", ItemRarity::Epic},
        {"minecraft:test_instance_block", ItemRarity::Epic},
        {"minecraft:tide_armor_trim_smithing_template", ItemRarity::Uncommon},
        {"minecraft:totem_of_undying", ItemRarity::Uncommon},
        {"minecraft:trident", ItemRarity::Rare},
        {"minecraft:vex_armor_trim_smithing_template", ItemRarity::Rare},
        {"minecraft:ward_armor_trim_smithing_template", ItemRarity::Rare},
        {"minecraft:wayfinder_armor_trim_smithing_template", ItemRarity::Uncommon},
        {"minecraft:wild_armor_trim_smithing_template", ItemRarity::Uncommon},
        {"minecraft:wither_skeleton_skull", ItemRarity::Rare},
        {"minecraft:zombie_head", ItemRarity::Uncommon},
    };
    return table;
}

} // namespace

std::optional<ItemRarity> vanillaItemRarity(std::string_view itemName)
{
    const auto& table = rarityTable();
    const auto found = table.find(itemName);
    if (found == table.end()) {
        return std::nullopt;
    }
    return found->second;
}

} // namespace mc
