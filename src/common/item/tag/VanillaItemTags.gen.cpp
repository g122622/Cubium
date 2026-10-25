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

/**
 * @file VanillaItemTags.gen.cpp
 * @brief 原版物品标签表（生成文件，勿手改）。
 *
 * 数据来源：MC 1.21.11 数据包里 data/minecraft/tags/item 下的全部标签 JSON。
 * 标签内的 #tag 引用已在生成期展开：原版 Tag#getValues() 的口径就是展开后的集合。
 *
 * 重新生成：见 tests/diff 的 item_tags harness 与 README「差分测试」一节；
 * 该 harness 会逐标签比对展开后的成员。
 */

#include "common/item/tag/ItemTags.hpp"

namespace mc::item::tag {

namespace {


const char* const kValues0[] = {
    "acacia_log", "acacia_wood", "stripped_acacia_log", "stripped_acacia_wood",
};

const char* const kValues1[] = {
    "anvil", "chipped_anvil", "damaged_anvil",
};

const char* const kValues2[] = {
    "spider_eye",
};

const char* const kValues3[] = {
    "arrow", "spectral_arrow", "tipped_arrow",
};

const char* const kValues4[] = {
    "copper_axe", "diamond_axe", "golden_axe", "iron_axe", "netherite_axe", "stone_axe", "wooden_axe",
};

const char* const kValues5[] = {
    "tropical_fish_bucket",
};

const char* const kValues6[] = {
    "bamboo_block", "stripped_bamboo_block",
};

const char* const kValues7[] = {
    "black_banner", "blue_banner", "brown_banner", "cyan_banner", "gray_banner", "green_banner",
    "light_blue_banner", "light_gray_banner", "lime_banner", "magenta_banner", "orange_banner", "pink_banner",
    "purple_banner", "red_banner", "white_banner", "yellow_banner",
};

const char* const kValues8[] = {
    "copper_bars", "exposed_copper_bars", "iron_bars", "oxidized_copper_bars", "waxed_copper_bars",
    "waxed_exposed_copper_bars", "waxed_oxidized_copper_bars", "waxed_weathered_copper_bars",
    "weathered_copper_bars",
};

const char* const kValues9[] = {
    "diamond", "emerald", "gold_ingot", "iron_ingot", "netherite_ingot",
};

const char* const kValues10[] = {
    "black_bed", "blue_bed", "brown_bed", "cyan_bed", "gray_bed", "green_bed", "light_blue_bed",
    "light_gray_bed", "lime_bed", "magenta_bed", "orange_bed", "pink_bed", "purple_bed", "red_bed",
    "white_bed", "yellow_bed",
};

const char* const kValues11[] = {
    "allium", "azure_bluet", "blue_orchid", "cactus_flower", "cherry_leaves", "chorus_flower", "cornflower",
    "dandelion", "flowering_azalea", "flowering_azalea_leaves", "lilac", "lily_of_the_valley",
    "mangrove_propagule", "open_eyeblossom", "orange_tulip", "oxeye_daisy", "peony", "pink_petals",
    "pink_tulip", "pitcher_plant", "poppy", "red_tulip", "rose_bush", "spore_blossom", "sunflower",
    "torchflower", "white_tulip", "wildflowers", "wither_rose",
};

const char* const kValues12[] = {
    "birch_log", "birch_wood", "stripped_birch_log", "stripped_birch_wood",
};

const char* const kValues13[] = {
    "acacia_boat", "acacia_chest_boat", "bamboo_chest_raft", "bamboo_raft", "birch_boat", "birch_chest_boat",
    "cherry_boat", "cherry_chest_boat", "dark_oak_boat", "dark_oak_chest_boat", "jungle_boat",
    "jungle_chest_boat", "mangrove_boat", "mangrove_chest_boat", "oak_boat", "oak_chest_boat", "pale_oak_boat",
    "pale_oak_chest_boat", "spruce_boat", "spruce_chest_boat",
};

const char* const kValues14[] = {
    "writable_book",
};

const char* const kValues15[] = {
    "book", "enchanted_book", "knowledge_book", "writable_book", "written_book",
};

const char* const kValues16[] = {
    "copper_axe", "copper_hoe", "copper_pickaxe", "copper_shovel", "copper_sword", "diamond_axe",
    "diamond_hoe", "diamond_pickaxe", "diamond_shovel", "diamond_sword", "golden_axe", "golden_hoe",
    "golden_pickaxe", "golden_shovel", "golden_sword", "iron_axe", "iron_hoe", "iron_pickaxe", "iron_shovel",
    "iron_sword", "mace", "netherite_axe", "netherite_hoe", "netherite_pickaxe", "netherite_shovel",
    "netherite_sword", "stone_axe", "stone_hoe", "stone_pickaxe", "stone_shovel", "stone_sword", "trident",
    "wooden_axe", "wooden_hoe", "wooden_pickaxe", "wooden_shovel", "wooden_sword",
};

const char* const kValues17[] = {
    "blaze_powder",
};

const char* const kValues18[] = {
    "black_bundle", "blue_bundle", "brown_bundle", "bundle", "cyan_bundle", "gray_bundle", "green_bundle",
    "light_blue_bundle", "light_gray_bundle", "lime_bundle", "magenta_bundle", "orange_bundle", "pink_bundle",
    "purple_bundle", "red_bundle", "white_bundle", "yellow_bundle",
};

const char* const kValues19[] = {
    "acacia_button", "bamboo_button", "birch_button", "cherry_button", "crimson_button", "dark_oak_button",
    "jungle_button", "mangrove_button", "oak_button", "pale_oak_button", "polished_blackstone_button",
    "spruce_button", "stone_button", "warped_button",
};

const char* const kValues20[] = {
    "cactus",
};

const char* const kValues21[] = {
    "rabbit_foot",
};

const char* const kValues22[] = {
    "black_candle", "blue_candle", "brown_candle", "candle", "cyan_candle", "gray_candle", "green_candle",
    "light_blue_candle", "light_gray_candle", "lime_candle", "magenta_candle", "orange_candle", "pink_candle",
    "purple_candle", "red_candle", "white_candle", "yellow_candle",
};

const char* const kValues23[] = {
    "cod", "salmon",
};

const char* const kValues24[] = {
    "copper_chain", "exposed_copper_chain", "iron_chain", "oxidized_copper_chain", "waxed_copper_chain",
    "waxed_exposed_copper_chain", "waxed_oxidized_copper_chain", "waxed_weathered_copper_chain",
    "weathered_copper_chain",
};

const char* const kValues25[] = {
    "cherry_log", "cherry_wood", "stripped_cherry_log", "stripped_cherry_wood",
};

const char* const kValues26[] = {
    "chainmail_chestplate", "copper_chestplate", "diamond_chestplate", "golden_chestplate", "iron_chestplate",
    "leather_chestplate", "netherite_chestplate",
};

const char* const kValues27[] = {
    "acacia_chest_boat", "bamboo_chest_raft", "birch_chest_boat", "cherry_chest_boat", "dark_oak_chest_boat",
    "jungle_chest_boat", "mangrove_chest_boat", "oak_chest_boat", "pale_oak_chest_boat", "spruce_chest_boat",
};

const char* const kValues28[] = {
    "beetroot_seeds", "melon_seeds", "pitcher_pod", "pumpkin_seeds", "torchflower_seeds", "wheat_seeds",
};

const char* const kValues29[] = {
    "copper_pickaxe", "diamond_pickaxe", "golden_pickaxe", "iron_pickaxe", "netherite_pickaxe",
    "stone_pickaxe", "wooden_pickaxe",
};

const char* const kValues30[] = {
    "coal_ore", "deepslate_coal_ore",
};

const char* const kValues31[] = {
    "charcoal", "coal",
};

const char* const kValues32[] = {
    "compass", "recovery_compass",
};

const char* const kValues33[] = {
    "acacia_leaves", "acacia_log", "acacia_wood", "azalea_leaves", "birch_leaves", "birch_log", "birch_wood",
    "cherry_leaves", "cherry_log", "cherry_wood", "crimson_hyphae", "crimson_stem", "dark_oak_leaves",
    "dark_oak_log", "dark_oak_wood", "flowering_azalea_leaves", "jungle_leaves", "jungle_log", "jungle_wood",
    "mangrove_leaves", "mangrove_log", "mangrove_wood", "nether_wart_block", "oak_leaves", "oak_log",
    "oak_wood", "pale_oak_leaves", "pale_oak_log", "pale_oak_wood", "spruce_leaves", "spruce_log",
    "spruce_wood", "stripped_acacia_log", "stripped_acacia_wood", "stripped_birch_log", "stripped_birch_wood",
    "stripped_cherry_log", "stripped_cherry_wood", "stripped_crimson_hyphae", "stripped_crimson_stem",
    "stripped_dark_oak_log", "stripped_dark_oak_wood", "stripped_jungle_log", "stripped_jungle_wood",
    "stripped_mangrove_log", "stripped_mangrove_wood", "stripped_oak_log", "stripped_oak_wood",
    "stripped_pale_oak_log", "stripped_pale_oak_wood", "stripped_spruce_log", "stripped_spruce_wood",
    "stripped_warped_hyphae", "stripped_warped_stem", "warped_hyphae", "warped_stem", "warped_wart_block",
};

const char* const kValues34[] = {
    "copper_block", "exposed_copper", "oxidized_copper", "waxed_copper_block", "waxed_exposed_copper",
    "waxed_oxidized_copper", "waxed_weathered_copper", "weathered_copper",
};

const char* const kValues35[] = {
    "copper_chest", "exposed_copper_chest", "oxidized_copper_chest", "waxed_copper_chest",
    "waxed_exposed_copper_chest", "waxed_oxidized_copper_chest", "waxed_weathered_copper_chest",
    "weathered_copper_chest",
};

const char* const kValues36[] = {
    "copper_golem_statue", "exposed_copper_golem_statue", "oxidized_copper_golem_statue",
    "waxed_copper_golem_statue", "waxed_exposed_copper_golem_statue", "waxed_oxidized_copper_golem_statue",
    "waxed_weathered_copper_golem_statue", "weathered_copper_golem_statue",
};

const char* const kValues37[] = {
    "copper_ore", "deepslate_copper_ore",
};

const char* const kValues38[] = {
    "copper_ingot",
};

const char* const kValues39[] = {
    "wheat",
};

const char* const kValues40[] = {
    "music_disc_11", "music_disc_13", "music_disc_blocks", "music_disc_cat", "music_disc_chirp",
    "music_disc_far", "music_disc_mall", "music_disc_mellohi", "music_disc_stal", "music_disc_strad",
    "music_disc_wait", "music_disc_ward",
};

const char* const kValues41[] = {
    "fire_charge", "flint_and_steel",
};

const char* const kValues42[] = {
    "crimson_hyphae", "crimson_stem", "stripped_crimson_hyphae", "stripped_crimson_stem",
};

const char* const kValues43[] = {
    "black_carpet", "black_wool", "blue_carpet", "blue_wool", "brown_carpet", "brown_wool", "cyan_carpet",
    "cyan_wool", "gray_carpet", "gray_wool", "green_carpet", "green_wool", "light_blue_carpet",
    "light_blue_wool", "light_gray_carpet", "light_gray_wool", "lime_carpet", "lime_wool", "magenta_carpet",
    "magenta_wool", "orange_carpet", "orange_wool", "pink_carpet", "pink_wool", "purple_carpet", "purple_wool",
    "red_carpet", "red_wool", "white_carpet", "white_wool", "yellow_carpet", "yellow_wool",
};

const char* const kValues44[] = {
    "dark_oak_log", "dark_oak_wood", "stripped_dark_oak_log", "stripped_dark_oak_wood",
};

const char* const kValues45[] = {
    "angler_pottery_sherd", "archer_pottery_sherd", "arms_up_pottery_sherd", "blade_pottery_sherd",
    "brewer_pottery_sherd", "brick", "burn_pottery_sherd", "danger_pottery_sherd", "explorer_pottery_sherd",
    "flow_pottery_sherd", "friend_pottery_sherd", "guster_pottery_sherd", "heart_pottery_sherd",
    "heartbreak_pottery_sherd", "howl_pottery_sherd", "miner_pottery_sherd", "mourner_pottery_sherd",
    "plenty_pottery_sherd", "prize_pottery_sherd", "scrape_pottery_sherd", "sheaf_pottery_sherd",
    "shelter_pottery_sherd", "skull_pottery_sherd", "snort_pottery_sherd",
};

const char* const kValues46[] = {
    "angler_pottery_sherd", "archer_pottery_sherd", "arms_up_pottery_sherd", "blade_pottery_sherd",
    "brewer_pottery_sherd", "burn_pottery_sherd", "danger_pottery_sherd", "explorer_pottery_sherd",
    "flow_pottery_sherd", "friend_pottery_sherd", "guster_pottery_sherd", "heart_pottery_sherd",
    "heartbreak_pottery_sherd", "howl_pottery_sherd", "miner_pottery_sherd", "mourner_pottery_sherd",
    "plenty_pottery_sherd", "prize_pottery_sherd", "scrape_pottery_sherd", "sheaf_pottery_sherd",
    "shelter_pottery_sherd", "skull_pottery_sherd", "snort_pottery_sherd",
};

const char* const kValues47[] = {
    "deepslate_diamond_ore", "diamond_ore",
};

const char* const kValues48[] = {
    "diamond",
};

const char* const kValues49[] = {
    "coarse_dirt", "dirt", "grass_block", "moss_block", "mud", "muddy_mangrove_roots", "mycelium",
    "pale_moss_block", "podzol", "rooted_dirt",
};

const char* const kValues50[] = {
    "acacia_door", "bamboo_door", "birch_door", "cherry_door", "copper_door", "crimson_door", "dark_oak_door",
    "exposed_copper_door", "iron_door", "jungle_door", "mangrove_door", "oak_door", "oxidized_copper_door",
    "pale_oak_door", "spruce_door", "warped_door", "waxed_copper_door", "waxed_exposed_copper_door",
    "waxed_oxidized_copper_door", "waxed_weathered_copper_door", "weathered_copper_door",
};

const char* const kValues51[] = {
    "trident",
};

const char* const kValues52[] = {
    "amethyst_shard",
};

const char* const kValues53[] = {
    "leather_boots", "leather_chestplate", "leather_helmet", "leather_horse_armor", "leather_leggings",
    "wolf_armor",
};

const char* const kValues54[] = {
    "blue_egg", "brown_egg", "egg",
};

const char* const kValues55[] = {
    "deepslate_emerald_ore", "emerald_ore",
};

const char* const kValues56[] = {
    "chainmail_boots", "chainmail_chestplate", "chainmail_helmet", "chainmail_leggings", "copper_boots",
    "copper_chestplate", "copper_helmet", "copper_leggings", "diamond_boots", "diamond_chestplate",
    "diamond_helmet", "diamond_leggings", "golden_boots", "golden_chestplate", "golden_helmet",
    "golden_leggings", "iron_boots", "iron_chestplate", "iron_helmet", "iron_leggings", "leather_boots",
    "leather_chestplate", "leather_helmet", "leather_leggings", "netherite_boots", "netherite_chestplate",
    "netherite_helmet", "netherite_leggings", "turtle_helmet",
};

const char* const kValues57[] = {
    "bow",
};

const char* const kValues58[] = {
    "chainmail_chestplate", "copper_chestplate", "diamond_chestplate", "golden_chestplate", "iron_chestplate",
    "leather_chestplate", "netherite_chestplate",
};

const char* const kValues59[] = {
    "crossbow",
};

const char* const kValues60[] = {
    "bow", "brush", "carrot_on_a_stick", "chainmail_boots", "chainmail_chestplate", "chainmail_helmet",
    "chainmail_leggings", "copper_axe", "copper_boots", "copper_chestplate", "copper_helmet", "copper_hoe",
    "copper_leggings", "copper_pickaxe", "copper_shovel", "copper_spear", "copper_sword", "crossbow",
    "diamond_axe", "diamond_boots", "diamond_chestplate", "diamond_helmet", "diamond_hoe", "diamond_leggings",
    "diamond_pickaxe", "diamond_shovel", "diamond_spear", "diamond_sword", "elytra", "fishing_rod",
    "flint_and_steel", "golden_axe", "golden_boots", "golden_chestplate", "golden_helmet", "golden_hoe",
    "golden_leggings", "golden_pickaxe", "golden_shovel", "golden_spear", "golden_sword", "iron_axe",
    "iron_boots", "iron_chestplate", "iron_helmet", "iron_hoe", "iron_leggings", "iron_pickaxe", "iron_shovel",
    "iron_spear", "iron_sword", "leather_boots", "leather_chestplate", "leather_helmet", "leather_leggings",
    "mace", "netherite_axe", "netherite_boots", "netherite_chestplate", "netherite_helmet", "netherite_hoe",
    "netherite_leggings", "netherite_pickaxe", "netherite_shovel", "netherite_spear", "netherite_sword",
    "shears", "shield", "stone_axe", "stone_hoe", "stone_pickaxe", "stone_shovel", "stone_spear",
    "stone_sword", "trident", "turtle_helmet", "warped_fungus_on_a_stick", "wooden_axe", "wooden_hoe",
    "wooden_pickaxe", "wooden_shovel", "wooden_spear", "wooden_sword",
};

const char* const kValues61[] = {
    "carved_pumpkin", "chainmail_boots", "chainmail_chestplate", "chainmail_helmet", "chainmail_leggings",
    "copper_boots", "copper_chestplate", "copper_helmet", "copper_leggings", "creeper_head", "diamond_boots",
    "diamond_chestplate", "diamond_helmet", "diamond_leggings", "dragon_head", "elytra", "golden_boots",
    "golden_chestplate", "golden_helmet", "golden_leggings", "iron_boots", "iron_chestplate", "iron_helmet",
    "iron_leggings", "leather_boots", "leather_chestplate", "leather_helmet", "leather_leggings",
    "netherite_boots", "netherite_chestplate", "netherite_helmet", "netherite_leggings", "piglin_head",
    "player_head", "skeleton_skull", "turtle_helmet", "wither_skeleton_skull", "zombie_head",
};

const char* const kValues62[] = {
    "copper_spear", "copper_sword", "diamond_spear", "diamond_sword", "golden_spear", "golden_sword",
    "iron_spear", "iron_sword", "mace", "netherite_spear", "netherite_sword", "stone_spear", "stone_sword",
    "wooden_spear", "wooden_sword",
};

const char* const kValues63[] = {
    "fishing_rod",
};

const char* const kValues64[] = {
    "chainmail_boots", "copper_boots", "diamond_boots", "golden_boots", "iron_boots", "leather_boots",
    "netherite_boots",
};

const char* const kValues65[] = {
    "chainmail_helmet", "copper_helmet", "diamond_helmet", "golden_helmet", "iron_helmet", "leather_helmet",
    "netherite_helmet", "turtle_helmet",
};

const char* const kValues66[] = {
    "chainmail_leggings", "copper_leggings", "diamond_leggings", "golden_leggings", "iron_leggings",
    "leather_leggings", "netherite_leggings",
};

const char* const kValues67[] = {
    "copper_spear", "diamond_spear", "golden_spear", "iron_spear", "netherite_spear", "stone_spear",
    "wooden_spear",
};

const char* const kValues68[] = {
    "mace",
};

const char* const kValues69[] = {
    "copper_spear", "copper_sword", "diamond_spear", "diamond_sword", "golden_spear", "golden_sword",
    "iron_spear", "iron_sword", "netherite_spear", "netherite_sword", "stone_spear", "stone_sword",
    "wooden_spear", "wooden_sword",
};

const char* const kValues70[] = {
    "copper_axe", "copper_hoe", "copper_pickaxe", "copper_shovel", "diamond_axe", "diamond_hoe",
    "diamond_pickaxe", "diamond_shovel", "golden_axe", "golden_hoe", "golden_pickaxe", "golden_shovel",
    "iron_axe", "iron_hoe", "iron_pickaxe", "iron_shovel", "netherite_axe", "netherite_hoe",
    "netherite_pickaxe", "netherite_shovel", "shears", "stone_axe", "stone_hoe", "stone_pickaxe",
    "stone_shovel", "wooden_axe", "wooden_hoe", "wooden_pickaxe", "wooden_shovel",
};

const char* const kValues71[] = {
    "copper_axe", "copper_hoe", "copper_pickaxe", "copper_shovel", "diamond_axe", "diamond_hoe",
    "diamond_pickaxe", "diamond_shovel", "golden_axe", "golden_hoe", "golden_pickaxe", "golden_shovel",
    "iron_axe", "iron_hoe", "iron_pickaxe", "iron_shovel", "netherite_axe", "netherite_hoe",
    "netherite_pickaxe", "netherite_shovel", "stone_axe", "stone_hoe", "stone_pickaxe", "stone_shovel",
    "wooden_axe", "wooden_hoe", "wooden_pickaxe", "wooden_shovel",
};

const char* const kValues72[] = {
    "copper_axe", "copper_spear", "copper_sword", "diamond_axe", "diamond_spear", "diamond_sword",
    "golden_axe", "golden_spear", "golden_sword", "iron_axe", "iron_spear", "iron_sword", "netherite_axe",
    "netherite_spear", "netherite_sword", "stone_axe", "stone_spear", "stone_sword", "wooden_axe",
    "wooden_spear", "wooden_sword",
};

const char* const kValues73[] = {
    "copper_sword", "diamond_sword", "golden_sword", "iron_sword", "netherite_sword", "stone_sword",
    "wooden_sword",
};

const char* const kValues74[] = {
    "trident",
};

const char* const kValues75[] = {
    "bow", "brush", "carrot_on_a_stick", "carved_pumpkin", "chainmail_boots", "chainmail_chestplate",
    "chainmail_helmet", "chainmail_leggings", "compass", "copper_axe", "copper_boots", "copper_chestplate",
    "copper_helmet", "copper_hoe", "copper_leggings", "copper_pickaxe", "copper_shovel", "copper_spear",
    "copper_sword", "creeper_head", "crossbow", "diamond_axe", "diamond_boots", "diamond_chestplate",
    "diamond_helmet", "diamond_hoe", "diamond_leggings", "diamond_pickaxe", "diamond_shovel", "diamond_spear",
    "diamond_sword", "dragon_head", "elytra", "fishing_rod", "flint_and_steel", "golden_axe", "golden_boots",
    "golden_chestplate", "golden_helmet", "golden_hoe", "golden_leggings", "golden_pickaxe", "golden_shovel",
    "golden_spear", "golden_sword", "iron_axe", "iron_boots", "iron_chestplate", "iron_helmet", "iron_hoe",
    "iron_leggings", "iron_pickaxe", "iron_shovel", "iron_spear", "iron_sword", "leather_boots",
    "leather_chestplate", "leather_helmet", "leather_leggings", "mace", "netherite_axe", "netherite_boots",
    "netherite_chestplate", "netherite_helmet", "netherite_hoe", "netherite_leggings", "netherite_pickaxe",
    "netherite_shovel", "netherite_spear", "netherite_sword", "piglin_head", "player_head", "shears", "shield",
    "skeleton_skull", "stone_axe", "stone_hoe", "stone_pickaxe", "stone_shovel", "stone_spear", "stone_sword",
    "trident", "turtle_helmet", "warped_fungus_on_a_stick", "wither_skeleton_skull", "wooden_axe",
    "wooden_hoe", "wooden_pickaxe", "wooden_shovel", "wooden_spear", "wooden_sword", "zombie_head",
};

const char* const kValues76[] = {
    "copper_axe", "copper_spear", "copper_sword", "diamond_axe", "diamond_spear", "diamond_sword",
    "golden_axe", "golden_spear", "golden_sword", "iron_axe", "iron_spear", "iron_sword", "mace",
    "netherite_axe", "netherite_spear", "netherite_sword", "stone_axe", "stone_spear", "stone_sword",
    "wooden_axe", "wooden_spear", "wooden_sword",
};

const char* const kValues77[] = {
    "acacia_fence_gate", "bamboo_fence_gate", "birch_fence_gate", "cherry_fence_gate", "crimson_fence_gate",
    "dark_oak_fence_gate", "jungle_fence_gate", "mangrove_fence_gate", "oak_fence_gate", "pale_oak_fence_gate",
    "spruce_fence_gate", "warped_fence_gate",
};

const char* const kValues78[] = {
    "acacia_fence", "bamboo_fence", "birch_fence", "cherry_fence", "crimson_fence", "dark_oak_fence",
    "jungle_fence", "mangrove_fence", "nether_brick_fence", "oak_fence", "pale_oak_fence", "spruce_fence",
    "warped_fence",
};

const char* const kValues79[] = {
    "cod", "cooked_cod", "cooked_salmon", "pufferfish", "salmon", "tropical_fish",
};

const char* const kValues80[] = {
    "allium", "azure_bluet", "blue_orchid", "cactus_flower", "cherry_leaves", "chorus_flower",
    "closed_eyeblossom", "cornflower", "dandelion", "flowering_azalea", "flowering_azalea_leaves", "lilac",
    "lily_of_the_valley", "mangrove_propagule", "open_eyeblossom", "orange_tulip", "oxeye_daisy", "peony",
    "pink_petals", "pink_tulip", "pitcher_plant", "poppy", "red_tulip", "rose_bush", "spore_blossom",
    "sunflower", "torchflower", "white_tulip", "wildflowers", "wither_rose",
};

const char* const kValues81[] = {
    "chainmail_boots", "copper_boots", "diamond_boots", "golden_boots", "iron_boots", "leather_boots",
    "netherite_boots",
};

const char* const kValues82[] = {
    "glow_berries", "sweet_berries",
};

const char* const kValues83[] = {
    "leather_boots", "leather_chestplate", "leather_helmet", "leather_horse_armor", "leather_leggings",
};

const char* const kValues84[] = {
    "slime_ball",
};

const char* const kValues85[] = {
    "charcoal", "coal",
};

const char* const kValues86[] = {
    "carved_pumpkin",
};

const char* const kValues87[] = {
    "wheat",
};

const char* const kValues88[] = {
    "deepslate_gold_ore", "gold_ore", "nether_gold_ore",
};

const char* const kValues89[] = {
    "gold_ingot",
};

const char* const kValues90[] = {
    "acacia_hanging_sign", "bamboo_hanging_sign", "birch_hanging_sign", "cherry_hanging_sign",
    "crimson_hanging_sign", "dark_oak_hanging_sign", "jungle_hanging_sign", "mangrove_hanging_sign",
    "oak_hanging_sign", "pale_oak_hanging_sign", "spruce_hanging_sign", "warped_hanging_sign",
};

const char* const kValues91[] = {
    "snowball",
};

const char* const kValues92[] = {
    "black_harness", "blue_harness", "brown_harness", "cyan_harness", "gray_harness", "green_harness",
    "light_blue_harness", "light_gray_harness", "lime_harness", "magenta_harness", "orange_harness",
    "pink_harness", "purple_harness", "red_harness", "snowball", "white_harness", "yellow_harness",
};

const char* const kValues93[] = {
    "black_harness", "blue_harness", "brown_harness", "cyan_harness", "gray_harness", "green_harness",
    "light_blue_harness", "light_gray_harness", "lime_harness", "magenta_harness", "orange_harness",
    "pink_harness", "purple_harness", "red_harness", "white_harness", "yellow_harness",
};

const char* const kValues94[] = {
    "chainmail_helmet", "copper_helmet", "diamond_helmet", "golden_helmet", "iron_helmet", "leather_helmet",
    "netherite_helmet", "turtle_helmet",
};

const char* const kValues95[] = {
    "copper_hoe", "diamond_hoe", "golden_hoe", "iron_hoe", "netherite_hoe", "stone_hoe", "wooden_hoe",
};

const char* const kValues96[] = {
    "crimson_fungus",
};

const char* const kValues97[] = {
    "apple", "carrot", "enchanted_golden_apple", "golden_apple", "golden_carrot", "hay_block", "sugar",
    "wheat",
};

const char* const kValues98[] = {
    "enchanted_golden_apple", "golden_apple", "golden_carrot",
};

const char* const kValues99[] = {
    "leather",
};

const char* const kValues100[] = {
    "deepslate_iron_ore", "iron_ore",
};

const char* const kValues101[] = {
    "iron_ingot",
};

const char* const kValues102[] = {
    "jungle_log", "jungle_wood", "stripped_jungle_log", "stripped_jungle_wood",
};

const char* const kValues103[] = {
    "copper_lantern", "exposed_copper_lantern", "lantern", "oxidized_copper_lantern", "soul_lantern",
    "waxed_copper_lantern", "waxed_exposed_copper_lantern", "waxed_oxidized_copper_lantern",
    "waxed_weathered_copper_lantern", "weathered_copper_lantern",
};

const char* const kValues104[] = {
    "deepslate_lapis_ore", "lapis_ore",
};

const char* const kValues105[] = {
    "acacia_leaves", "azalea_leaves", "birch_leaves", "cherry_leaves", "dark_oak_leaves",
    "flowering_azalea_leaves", "jungle_leaves", "mangrove_leaves", "oak_leaves", "pale_oak_leaves",
    "spruce_leaves",
};

const char* const kValues106[] = {
    "writable_book", "written_book",
};

const char* const kValues107[] = {
    "chainmail_leggings", "copper_leggings", "diamond_leggings", "golden_leggings", "iron_leggings",
    "leather_leggings", "netherite_leggings",
};

const char* const kValues108[] = {
    "exposed_lightning_rod", "lightning_rod", "oxidized_lightning_rod", "waxed_exposed_lightning_rod",
    "waxed_lightning_rod", "waxed_oxidized_lightning_rod", "waxed_weathered_lightning_rod",
    "weathered_lightning_rod",
};

const char* const kValues109[] = {
    "hay_block", "wheat",
};

const char* const kValues110[] = {
    "hay_block",
};

const char* const kValues111[] = {
    "acacia_log", "acacia_wood", "birch_log", "birch_wood", "cherry_log", "cherry_wood", "crimson_hyphae",
    "crimson_stem", "dark_oak_log", "dark_oak_wood", "jungle_log", "jungle_wood", "mangrove_log",
    "mangrove_wood", "oak_log", "oak_wood", "pale_oak_log", "pale_oak_wood", "spruce_log", "spruce_wood",
    "stripped_acacia_log", "stripped_acacia_wood", "stripped_birch_log", "stripped_birch_wood",
    "stripped_cherry_log", "stripped_cherry_wood", "stripped_crimson_hyphae", "stripped_crimson_stem",
    "stripped_dark_oak_log", "stripped_dark_oak_wood", "stripped_jungle_log", "stripped_jungle_wood",
    "stripped_mangrove_log", "stripped_mangrove_wood", "stripped_oak_log", "stripped_oak_wood",
    "stripped_pale_oak_log", "stripped_pale_oak_wood", "stripped_spruce_log", "stripped_spruce_wood",
    "stripped_warped_hyphae", "stripped_warped_stem", "warped_hyphae", "warped_stem",
};

const char* const kValues112[] = {
    "acacia_log", "acacia_wood", "birch_log", "birch_wood", "cherry_log", "cherry_wood", "dark_oak_log",
    "dark_oak_wood", "jungle_log", "jungle_wood", "mangrove_log", "mangrove_wood", "oak_log", "oak_wood",
    "pale_oak_log", "pale_oak_wood", "spruce_log", "spruce_wood", "stripped_acacia_log",
    "stripped_acacia_wood", "stripped_birch_log", "stripped_birch_wood", "stripped_cherry_log",
    "stripped_cherry_wood", "stripped_dark_oak_log", "stripped_dark_oak_wood", "stripped_jungle_log",
    "stripped_jungle_wood", "stripped_mangrove_log", "stripped_mangrove_wood", "stripped_oak_log",
    "stripped_oak_wood", "stripped_pale_oak_log", "stripped_pale_oak_wood", "stripped_spruce_log",
    "stripped_spruce_wood",
};

const char* const kValues113[] = {
    "mangrove_log", "mangrove_wood", "stripped_mangrove_log", "stripped_mangrove_wood",
};

const char* const kValues114[] = {
    "carved_pumpkin",
};

const char* const kValues115[] = {
    "beef", "chicken", "cooked_beef", "cooked_chicken", "cooked_mutton", "cooked_porkchop", "cooked_rabbit",
    "mutton", "porkchop", "rabbit", "rotten_flesh",
};

const char* const kValues116[] = {
    "cod_bucket", "pufferfish_bucket", "salmon_bucket", "tropical_fish_bucket",
};

const char* const kValues117[] = {
    "cod", "cod_bucket", "cooked_cod", "cooked_salmon", "pufferfish", "pufferfish_bucket", "salmon",
    "salmon_bucket", "tropical_fish", "tropical_fish_bucket",
};

const char* const kValues118[] = {
    "pufferfish", "pufferfish_bucket",
};

const char* const kValues119[] = {
    "netherite_ingot",
};

const char* const kValues120[] = {
    "crimson_button", "crimson_door", "crimson_fence", "crimson_fence_gate", "crimson_hanging_sign",
    "crimson_hyphae", "crimson_planks", "crimson_pressure_plate", "crimson_shelf", "crimson_sign",
    "crimson_slab", "crimson_stairs", "crimson_stem", "crimson_trapdoor", "stripped_crimson_hyphae",
    "stripped_crimson_stem", "stripped_warped_hyphae", "stripped_warped_stem", "warped_button", "warped_door",
    "warped_fence", "warped_fence_gate", "warped_hanging_sign", "warped_hyphae", "warped_planks",
    "warped_pressure_plate", "warped_shelf", "warped_sign", "warped_slab", "warped_stairs", "warped_stem",
    "warped_trapdoor",
};

const char* const kValues121[] = {
    "creeper_head", "dragon_head", "piglin_head", "player_head", "skeleton_skull", "wither_skeleton_skull",
    "zombie_head",
};

const char* const kValues122[] = {
    "oak_log", "oak_wood", "stripped_oak_log", "stripped_oak_wood",
};

const char* const kValues123[] = {
    "cod", "salmon",
};

const char* const kValues124[] = {
    "pale_oak_log", "pale_oak_wood", "stripped_pale_oak_log", "stripped_pale_oak_wood",
};

const char* const kValues125[] = {
    "bamboo", "cake",
};

const char* const kValues126[] = {
    "bamboo",
};

const char* const kValues127[] = {
    "beetroot_seeds", "melon_seeds", "pitcher_pod", "pumpkin_seeds", "torchflower_seeds", "wheat_seeds",
};

const char* const kValues128[] = {
    "cookie",
};

const char* const kValues129[] = {
    "copper_pickaxe", "diamond_pickaxe", "golden_pickaxe", "iron_pickaxe", "netherite_pickaxe",
    "stone_pickaxe", "wooden_pickaxe",
};

const char* const kValues130[] = {
    "beetroot", "carrot", "potato",
};

const char* const kValues131[] = {
    "cooked_porkchop", "porkchop",
};

const char* const kValues132[] = {
    "bell", "clock", "deepslate_gold_ore", "enchanted_golden_apple", "gilded_blackstone",
    "glistering_melon_slice", "gold_block", "gold_ingot", "gold_ore", "golden_apple", "golden_axe",
    "golden_boots", "golden_carrot", "golden_chestplate", "golden_helmet", "golden_hoe", "golden_horse_armor",
    "golden_leggings", "golden_nautilus_armor", "golden_pickaxe", "golden_shovel", "golden_spear",
    "golden_sword", "light_weighted_pressure_plate", "nether_gold_ore", "raw_gold", "raw_gold_block",
};

const char* const kValues133[] = {
    "crossbow", "golden_spear",
};

const char* const kValues134[] = {
    "soul_campfire", "soul_lantern", "soul_torch",
};

const char* const kValues135[] = {
    "golden_boots", "golden_chestplate", "golden_helmet", "golden_leggings",
};

const char* const kValues136[] = {
    "crossbow",
};

const char* const kValues137[] = {
    "acacia_planks", "bamboo_planks", "birch_planks", "cherry_planks", "crimson_planks", "dark_oak_planks",
    "jungle_planks", "mangrove_planks", "oak_planks", "pale_oak_planks", "spruce_planks", "warped_planks",
};

const char* const kValues138[] = {
    "carrot", "dandelion", "golden_carrot",
};

const char* const kValues139[] = {
    "activator_rail", "detector_rail", "powered_rail", "rail",
};

const char* const kValues140[] = {
    "deepslate_redstone_ore", "redstone_ore",
};

const char* const kValues141[] = {
    "iron_ingot",
};

const char* const kValues142[] = {
    "copper_ingot",
};

const char* const kValues143[] = {
    "diamond",
};

const char* const kValues144[] = {
    "gold_ingot",
};

const char* const kValues145[] = {
    "iron_ingot",
};

const char* const kValues146[] = {
    "leather",
};

const char* const kValues147[] = {
    "netherite_ingot",
};

const char* const kValues148[] = {
    "turtle_scute",
};

const char* const kValues149[] = {
    "armadillo_scute",
};

const char* const kValues150[] = {
    "red_sand", "sand", "suspicious_sand",
};

const char* const kValues151[] = {
    "acacia_sapling", "azalea", "birch_sapling", "cherry_sapling", "dark_oak_sapling", "flowering_azalea",
    "jungle_sapling", "mangrove_propagule", "oak_sapling", "pale_oak_sapling", "spruce_sapling",
};

const char* const kValues152[] = {
    "poppy",
};

const char* const kValues153[] = {
    "wheat",
};

const char* const kValues154[] = {
    "copper_shovel", "diamond_shovel", "golden_shovel", "iron_shovel", "netherite_shovel", "stone_shovel",
    "wooden_shovel",
};

const char* const kValues155[] = {
    "black_shulker_box", "blue_shulker_box", "brown_shulker_box", "cyan_shulker_box", "gray_shulker_box",
    "green_shulker_box", "light_blue_shulker_box", "light_gray_shulker_box", "lime_shulker_box",
    "magenta_shulker_box", "orange_shulker_box", "pink_shulker_box", "purple_shulker_box", "red_shulker_box",
    "shulker_box", "white_shulker_box", "yellow_shulker_box",
};

const char* const kValues156[] = {
    "acacia_sign", "bamboo_sign", "birch_sign", "cherry_sign", "crimson_sign", "dark_oak_sign", "jungle_sign",
    "mangrove_sign", "oak_sign", "pale_oak_sign", "spruce_sign", "warped_sign",
};

const char* const kValues157[] = {
    "bow",
};

const char* const kValues158[] = {
    "creeper_head", "dragon_head", "piglin_head", "player_head", "skeleton_skull", "wither_skeleton_skull",
    "zombie_head",
};

const char* const kValues159[] = {
    "acacia_slab", "andesite_slab", "bamboo_mosaic_slab", "bamboo_slab", "birch_slab", "blackstone_slab",
    "brick_slab", "cherry_slab", "cobbled_deepslate_slab", "cobblestone_slab", "crimson_slab",
    "cut_copper_slab", "cut_red_sandstone_slab", "cut_sandstone_slab", "dark_oak_slab", "dark_prismarine_slab",
    "deepslate_brick_slab", "deepslate_tile_slab", "diorite_slab", "end_stone_brick_slab",
    "exposed_cut_copper_slab", "granite_slab", "jungle_slab", "mangrove_slab", "mossy_cobblestone_slab",
    "mossy_stone_brick_slab", "mud_brick_slab", "nether_brick_slab", "oak_slab", "oxidized_cut_copper_slab",
    "pale_oak_slab", "petrified_oak_slab", "polished_andesite_slab", "polished_blackstone_brick_slab",
    "polished_blackstone_slab", "polished_deepslate_slab", "polished_diorite_slab", "polished_granite_slab",
    "polished_tuff_slab", "prismarine_brick_slab", "prismarine_slab", "purpur_slab", "quartz_slab",
    "red_nether_brick_slab", "red_sandstone_slab", "resin_brick_slab", "sandstone_slab", "smooth_quartz_slab",
    "smooth_red_sandstone_slab", "smooth_sandstone_slab", "smooth_stone_slab", "spruce_slab",
    "stone_brick_slab", "stone_slab", "tuff_brick_slab", "tuff_slab", "warped_slab", "waxed_cut_copper_slab",
    "waxed_exposed_cut_copper_slab", "waxed_oxidized_cut_copper_slab", "waxed_weathered_cut_copper_slab",
    "weathered_cut_copper_slab",
};

const char* const kValues160[] = {
    "allium", "azure_bluet", "blue_orchid", "closed_eyeblossom", "cornflower", "dandelion",
    "lily_of_the_valley", "open_eyeblossom", "orange_tulip", "oxeye_daisy", "pink_tulip", "poppy", "red_tulip",
    "torchflower", "white_tulip", "wither_rose",
};

const char* const kValues161[] = {
    "red_sand", "sand",
};

const char* const kValues162[] = {
    "torchflower_seeds",
};

const char* const kValues163[] = {
    "soul_sand", "soul_soil",
};

const char* const kValues164[] = {
    "copper_spear", "diamond_spear", "golden_spear", "iron_spear", "netherite_spear", "stone_spear",
    "wooden_spear",
};

const char* const kValues165[] = {
    "spruce_log", "spruce_wood", "stripped_spruce_log", "stripped_spruce_wood",
};

const char* const kValues166[] = {
    "acacia_stairs", "andesite_stairs", "bamboo_mosaic_stairs", "bamboo_stairs", "birch_stairs",
    "blackstone_stairs", "brick_stairs", "cherry_stairs", "cobbled_deepslate_stairs", "cobblestone_stairs",
    "crimson_stairs", "cut_copper_stairs", "dark_oak_stairs", "dark_prismarine_stairs",
    "deepslate_brick_stairs", "deepslate_tile_stairs", "diorite_stairs", "end_stone_brick_stairs",
    "exposed_cut_copper_stairs", "granite_stairs", "jungle_stairs", "mangrove_stairs",
    "mossy_cobblestone_stairs", "mossy_stone_brick_stairs", "mud_brick_stairs", "nether_brick_stairs",
    "oak_stairs", "oxidized_cut_copper_stairs", "pale_oak_stairs", "polished_andesite_stairs",
    "polished_blackstone_brick_stairs", "polished_blackstone_stairs", "polished_deepslate_stairs",
    "polished_diorite_stairs", "polished_granite_stairs", "polished_tuff_stairs", "prismarine_brick_stairs",
    "prismarine_stairs", "purpur_stairs", "quartz_stairs", "red_nether_brick_stairs", "red_sandstone_stairs",
    "resin_brick_stairs", "sandstone_stairs", "smooth_quartz_stairs", "smooth_red_sandstone_stairs",
    "smooth_sandstone_stairs", "spruce_stairs", "stone_brick_stairs", "stone_stairs", "tuff_brick_stairs",
    "tuff_stairs", "warped_stairs", "waxed_cut_copper_stairs", "waxed_exposed_cut_copper_stairs",
    "waxed_oxidized_cut_copper_stairs", "waxed_weathered_cut_copper_stairs", "weathered_cut_copper_stairs",
};

const char* const kValues167[] = {
    "chiseled_stone_bricks", "cracked_stone_bricks", "mossy_stone_bricks", "stone_bricks",
};

const char* const kValues168[] = {
    "polished_blackstone_button", "stone_button",
};

const char* const kValues169[] = {
    "blackstone", "cobbled_deepslate", "cobblestone",
};

const char* const kValues170[] = {
    "blackstone", "cobbled_deepslate", "cobblestone",
};

const char* const kValues171[] = {
    "warped_fungus",
};

const char* const kValues172[] = {
    "warped_fungus", "warped_fungus_on_a_stick",
};

const char* const kValues173[] = {
    "copper_sword", "diamond_sword", "golden_sword", "iron_sword", "netherite_sword", "stone_sword",
    "wooden_sword",
};

const char* const kValues174[] = {
    "black_terracotta", "blue_terracotta", "brown_terracotta", "cyan_terracotta", "gray_terracotta",
    "green_terracotta", "light_blue_terracotta", "light_gray_terracotta", "lime_terracotta",
    "magenta_terracotta", "orange_terracotta", "pink_terracotta", "purple_terracotta", "red_terracotta",
    "terracotta", "white_terracotta", "yellow_terracotta",
};

const char* const kValues175[] = {
    "acacia_trapdoor", "bamboo_trapdoor", "birch_trapdoor", "cherry_trapdoor", "copper_trapdoor",
    "crimson_trapdoor", "dark_oak_trapdoor", "exposed_copper_trapdoor", "iron_trapdoor", "jungle_trapdoor",
    "mangrove_trapdoor", "oak_trapdoor", "oxidized_copper_trapdoor", "pale_oak_trapdoor", "spruce_trapdoor",
    "warped_trapdoor", "waxed_copper_trapdoor", "waxed_exposed_copper_trapdoor",
    "waxed_oxidized_copper_trapdoor", "waxed_weathered_copper_trapdoor", "weathered_copper_trapdoor",
};

const char* const kValues176[] = {
    "amethyst_shard", "copper_ingot", "diamond", "emerald", "gold_ingot", "iron_ingot", "lapis_lazuli",
    "netherite_ingot", "quartz", "redstone", "resin_brick",
};

const char* const kValues177[] = {
    "chainmail_boots", "chainmail_chestplate", "chainmail_helmet", "chainmail_leggings", "copper_boots",
    "copper_chestplate", "copper_helmet", "copper_leggings", "diamond_boots", "diamond_chestplate",
    "diamond_helmet", "diamond_leggings", "golden_boots", "golden_chestplate", "golden_helmet",
    "golden_leggings", "iron_boots", "iron_chestplate", "iron_helmet", "iron_leggings", "leather_boots",
    "leather_chestplate", "leather_helmet", "leather_leggings", "netherite_boots", "netherite_chestplate",
    "netherite_helmet", "netherite_leggings", "turtle_helmet",
};

const char* const kValues178[] = {
    "seagrass",
};

const char* const kValues179[] = {
    "beetroot", "beetroot_seeds", "bread", "carrot", "pitcher_pod", "potato", "torchflower_seeds", "wheat",
    "wheat_seeds",
};

const char* const kValues180[] = {
    "beetroot_seeds", "carrot", "pitcher_pod", "potato", "torchflower_seeds", "wheat_seeds",
};

const char* const kValues181[] = {
    "andesite_wall", "blackstone_wall", "brick_wall", "cobbled_deepslate_wall", "cobblestone_wall",
    "deepslate_brick_wall", "deepslate_tile_wall", "diorite_wall", "end_stone_brick_wall", "granite_wall",
    "mossy_cobblestone_wall", "mossy_stone_brick_wall", "mud_brick_wall", "nether_brick_wall",
    "polished_blackstone_brick_wall", "polished_blackstone_wall", "polished_deepslate_wall",
    "polished_tuff_wall", "prismarine_wall", "red_nether_brick_wall", "red_sandstone_wall", "resin_brick_wall",
    "sandstone_wall", "stone_brick_wall", "tuff_brick_wall", "tuff_wall",
};

const char* const kValues182[] = {
    "stripped_warped_hyphae", "stripped_warped_stem", "warped_hyphae", "warped_stem",
};

const char* const kValues183[] = {
    "nether_wart_block", "warped_wart_block",
};

const char* const kValues184[] = {
    "bow", "crossbow",
};

const char* const kValues185[] = {
    "beef", "chicken", "cod", "cooked_beef", "cooked_chicken", "cooked_cod", "cooked_mutton",
    "cooked_porkchop", "cooked_rabbit", "cooked_salmon", "mutton", "porkchop", "pufferfish", "rabbit",
    "rabbit_stew", "rotten_flesh", "salmon", "tropical_fish",
};

const char* const kValues186[] = {
    "acacia_button", "bamboo_button", "birch_button", "cherry_button", "crimson_button", "dark_oak_button",
    "jungle_button", "mangrove_button", "oak_button", "pale_oak_button", "spruce_button", "warped_button",
};

const char* const kValues187[] = {
    "acacia_door", "bamboo_door", "birch_door", "cherry_door", "crimson_door", "dark_oak_door", "jungle_door",
    "mangrove_door", "oak_door", "pale_oak_door", "spruce_door", "warped_door",
};

const char* const kValues188[] = {
    "acacia_fence", "bamboo_fence", "birch_fence", "cherry_fence", "crimson_fence", "dark_oak_fence",
    "jungle_fence", "mangrove_fence", "oak_fence", "pale_oak_fence", "spruce_fence", "warped_fence",
};

const char* const kValues189[] = {
    "acacia_pressure_plate", "bamboo_pressure_plate", "birch_pressure_plate", "cherry_pressure_plate",
    "crimson_pressure_plate", "dark_oak_pressure_plate", "jungle_pressure_plate", "mangrove_pressure_plate",
    "oak_pressure_plate", "pale_oak_pressure_plate", "spruce_pressure_plate", "warped_pressure_plate",
};

const char* const kValues190[] = {
    "acacia_shelf", "bamboo_shelf", "birch_shelf", "cherry_shelf", "crimson_shelf", "dark_oak_shelf",
    "jungle_shelf", "mangrove_shelf", "oak_shelf", "pale_oak_shelf", "spruce_shelf", "warped_shelf",
};

const char* const kValues191[] = {
    "acacia_slab", "bamboo_slab", "birch_slab", "cherry_slab", "crimson_slab", "dark_oak_slab", "jungle_slab",
    "mangrove_slab", "oak_slab", "pale_oak_slab", "spruce_slab", "warped_slab",
};

const char* const kValues192[] = {
    "acacia_stairs", "bamboo_stairs", "birch_stairs", "cherry_stairs", "crimson_stairs", "dark_oak_stairs",
    "jungle_stairs", "mangrove_stairs", "oak_stairs", "pale_oak_stairs", "spruce_stairs", "warped_stairs",
};

const char* const kValues193[] = {
    "acacia_planks", "bamboo_planks", "birch_planks", "cherry_planks", "crimson_planks", "dark_oak_planks",
    "jungle_planks", "mangrove_planks", "oak_planks", "pale_oak_planks", "spruce_planks", "warped_planks",
};

const char* const kValues194[] = {
    "acacia_trapdoor", "bamboo_trapdoor", "birch_trapdoor", "cherry_trapdoor", "crimson_trapdoor",
    "dark_oak_trapdoor", "jungle_trapdoor", "mangrove_trapdoor", "oak_trapdoor", "pale_oak_trapdoor",
    "spruce_trapdoor", "warped_trapdoor",
};

const char* const kValues195[] = {
    "black_wool", "blue_wool", "brown_wool", "cyan_wool", "gray_wool", "green_wool", "light_blue_wool",
    "light_gray_wool", "lime_wool", "magenta_wool", "orange_wool", "pink_wool", "purple_wool", "red_wool",
    "white_wool", "yellow_wool",
};

const char* const kValues196[] = {
    "black_carpet", "blue_carpet", "brown_carpet", "cyan_carpet", "gray_carpet", "green_carpet",
    "light_blue_carpet", "light_gray_carpet", "lime_carpet", "magenta_carpet", "orange_carpet", "pink_carpet",
    "purple_carpet", "red_carpet", "white_carpet", "yellow_carpet",
};

const char* const kValues197[] = {
    "red_mushroom",
};

} // namespace

const VanillaItemTagEntry* vanillaItemTagTable(size_t& count) noexcept
{
    static const VanillaItemTagEntry kTags[] = {
        {"acacia_logs", 4, kValues0},
        {"anvil", 3, kValues1},
        {"armadillo_food", 1, kValues2},
        {"arrows", 3, kValues3},
        {"axes", 7, kValues4},
        {"axolotl_food", 1, kValues5},
        {"bamboo_blocks", 2, kValues6},
        {"banners", 16, kValues7},
        {"bars", 9, kValues8},
        {"beacon_payment_items", 5, kValues9},
        {"beds", 16, kValues10},
        {"bee_food", 29, kValues11},
        {"birch_logs", 4, kValues12},
        {"boats", 20, kValues13},
        {"book_cloning_target", 1, kValues14},
        {"bookshelf_books", 5, kValues15},
        {"breaks_decorated_pots", 37, kValues16},
        {"brewing_fuel", 1, kValues17},
        {"bundles", 17, kValues18},
        {"buttons", 14, kValues19},
        {"camel_food", 1, kValues20},
        {"camel_husk_food", 1, kValues21},
        {"candles", 17, kValues22},
        {"cat_food", 2, kValues23},
        {"chains", 9, kValues24},
        {"cherry_logs", 4, kValues25},
        {"chest_armor", 7, kValues26},
        {"chest_boats", 10, kValues27},
        {"chicken_food", 6, kValues28},
        {"cluster_max_harvestables", 7, kValues29},
        {"coal_ores", 2, kValues30},
        {"coals", 2, kValues31},
        {"compasses", 2, kValues32},
        {"completes_find_tree_tutorial", 57, kValues33},
        {"copper", 8, kValues34},
        {"copper_chests", 8, kValues35},
        {"copper_golem_statues", 8, kValues36},
        {"copper_ores", 2, kValues37},
        {"copper_tool_materials", 1, kValues38},
        {"cow_food", 1, kValues39},
        {"creeper_drop_music_discs", 12, kValues40},
        {"creeper_igniters", 2, kValues41},
        {"crimson_stems", 4, kValues42},
        {"dampens_vibrations", 32, kValues43},
        {"dark_oak_logs", 4, kValues44},
        {"decorated_pot_ingredients", 24, kValues45},
        {"decorated_pot_sherds", 23, kValues46},
        {"diamond_ores", 2, kValues47},
        {"diamond_tool_materials", 1, kValues48},
        {"dirt", 10, kValues49},
        {"doors", 21, kValues50},
        {"drowned_preferred_weapons", 1, kValues51},
        {"duplicates_allays", 1, kValues52},
        {"dyeable", 6, kValues53},
        {"eggs", 3, kValues54},
        {"emerald_ores", 2, kValues55},
        {"enchantable/armor", 29, kValues56},
        {"enchantable/bow", 1, kValues57},
        {"enchantable/chest_armor", 7, kValues58},
        {"enchantable/crossbow", 1, kValues59},
        {"enchantable/durability", 83, kValues60},
        {"enchantable/equippable", 38, kValues61},
        {"enchantable/fire_aspect", 15, kValues62},
        {"enchantable/fishing", 1, kValues63},
        {"enchantable/foot_armor", 7, kValues64},
        {"enchantable/head_armor", 8, kValues65},
        {"enchantable/leg_armor", 7, kValues66},
        {"enchantable/lunge", 7, kValues67},
        {"enchantable/mace", 1, kValues68},
        {"enchantable/melee_weapon", 14, kValues69},
        {"enchantable/mining", 29, kValues70},
        {"enchantable/mining_loot", 28, kValues71},
        {"enchantable/sharp_weapon", 21, kValues72},
        {"enchantable/sweeping", 7, kValues73},
        {"enchantable/trident", 1, kValues74},
        {"enchantable/vanishing", 92, kValues75},
        {"enchantable/weapon", 22, kValues76},
        {"fence_gates", 12, kValues77},
        {"fences", 13, kValues78},
        {"fishes", 6, kValues79},
        {"flowers", 30, kValues80},
        {"foot_armor", 7, kValues81},
        {"fox_food", 2, kValues82},
        {"freeze_immune_wearables", 5, kValues83},
        {"frog_food", 1, kValues84},
        {"furnace_minecart_fuel", 2, kValues85},
        {"gaze_disguise_equipment", 1, kValues86},
        {"goat_food", 1, kValues87},
        {"gold_ores", 3, kValues88},
        {"gold_tool_materials", 1, kValues89},
        {"hanging_signs", 12, kValues90},
        {"happy_ghast_food", 1, kValues91},
        {"happy_ghast_tempt_items", 17, kValues92},
        {"harnesses", 16, kValues93},
        {"head_armor", 8, kValues94},
        {"hoes", 7, kValues95},
        {"hoglin_food", 1, kValues96},
        {"horse_food", 8, kValues97},
        {"horse_tempt_items", 3, kValues98},
        {"ignored_by_piglin_babies", 1, kValues99},
        {"iron_ores", 2, kValues100},
        {"iron_tool_materials", 1, kValues101},
        {"jungle_logs", 4, kValues102},
        {"lanterns", 10, kValues103},
        {"lapis_ores", 2, kValues104},
        {"leaves", 11, kValues105},
        {"lectern_books", 2, kValues106},
        {"leg_armor", 7, kValues107},
        {"lightning_rods", 8, kValues108},
        {"llama_food", 2, kValues109},
        {"llama_tempt_items", 1, kValues110},
        {"logs", 44, kValues111},
        {"logs_that_burn", 36, kValues112},
        {"mangrove_logs", 4, kValues113},
        {"map_invisibility_equipment", 1, kValues114},
        {"meat", 11, kValues115},
        {"nautilus_bucket_food", 4, kValues116},
        {"nautilus_food", 10, kValues117},
        {"nautilus_taming_items", 2, kValues118},
        {"netherite_tool_materials", 1, kValues119},
        {"non_flammable_wood", 32, kValues120},
        {"noteblock_top_instruments", 7, kValues121},
        {"oak_logs", 4, kValues122},
        {"ocelot_food", 2, kValues123},
        {"pale_oak_logs", 4, kValues124},
        {"panda_eats_from_ground", 2, kValues125},
        {"panda_food", 1, kValues126},
        {"parrot_food", 6, kValues127},
        {"parrot_poisonous_food", 1, kValues128},
        {"pickaxes", 7, kValues129},
        {"pig_food", 3, kValues130},
        {"piglin_food", 2, kValues131},
        {"piglin_loved", 27, kValues132},
        {"piglin_preferred_weapons", 2, kValues133},
        {"piglin_repellents", 3, kValues134},
        {"piglin_safe_armor", 4, kValues135},
        {"pillager_preferred_weapons", 1, kValues136},
        {"planks", 12, kValues137},
        {"rabbit_food", 3, kValues138},
        {"rails", 4, kValues139},
        {"redstone_ores", 2, kValues140},
        {"repairs_chain_armor", 1, kValues141},
        {"repairs_copper_armor", 1, kValues142},
        {"repairs_diamond_armor", 1, kValues143},
        {"repairs_gold_armor", 1, kValues144},
        {"repairs_iron_armor", 1, kValues145},
        {"repairs_leather_armor", 1, kValues146},
        {"repairs_netherite_armor", 1, kValues147},
        {"repairs_turtle_helmet", 1, kValues148},
        {"repairs_wolf_armor", 1, kValues149},
        {"sand", 3, kValues150},
        {"saplings", 11, kValues151},
        {"shearable_from_copper_golem", 1, kValues152},
        {"sheep_food", 1, kValues153},
        {"shovels", 7, kValues154},
        {"shulker_boxes", 17, kValues155},
        {"signs", 12, kValues156},
        {"skeleton_preferred_weapons", 1, kValues157},
        {"skulls", 7, kValues158},
        {"slabs", 62, kValues159},
        {"small_flowers", 16, kValues160},
        {"smelts_to_glass", 2, kValues161},
        {"sniffer_food", 1, kValues162},
        {"soul_fire_base_blocks", 2, kValues163},
        {"spears", 7, kValues164},
        {"spruce_logs", 4, kValues165},
        {"stairs", 58, kValues166},
        {"stone_bricks", 4, kValues167},
        {"stone_buttons", 2, kValues168},
        {"stone_crafting_materials", 3, kValues169},
        {"stone_tool_materials", 3, kValues170},
        {"strider_food", 1, kValues171},
        {"strider_tempt_items", 2, kValues172},
        {"swords", 7, kValues173},
        {"terracotta", 17, kValues174},
        {"trapdoors", 21, kValues175},
        {"trim_materials", 11, kValues176},
        {"trimmable_armor", 29, kValues177},
        {"turtle_food", 1, kValues178},
        {"villager_picks_up", 9, kValues179},
        {"villager_plantable_seeds", 6, kValues180},
        {"walls", 26, kValues181},
        {"warped_stems", 4, kValues182},
        {"wart_blocks", 2, kValues183},
        {"wither_skeleton_disliked_weapons", 2, kValues184},
        {"wolf_food", 18, kValues185},
        {"wooden_buttons", 12, kValues186},
        {"wooden_doors", 12, kValues187},
        {"wooden_fences", 12, kValues188},
        {"wooden_pressure_plates", 12, kValues189},
        {"wooden_shelves", 12, kValues190},
        {"wooden_slabs", 12, kValues191},
        {"wooden_stairs", 12, kValues192},
        {"wooden_tool_materials", 12, kValues193},
        {"wooden_trapdoors", 12, kValues194},
        {"wool", 16, kValues195},
        {"wool_carpets", 16, kValues196},
        {"zombie_horse_food", 1, kValues197},
    };
    count = sizeof(kTags) / sizeof(kTags[0]);
    return kTags;
}

} // namespace mc::item::tag
