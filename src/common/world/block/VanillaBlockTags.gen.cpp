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
 * @file VanillaBlockTags.gen.cpp
 * @brief 原版方块标签表（生成文件，勿手改）。
 *
 * 数据来源：MC 1.21.11 数据包里 data/minecraft/tags/block 下的全部标签 JSON。
 * 标签内的 #tag 引用已在生成期展开：原版 Tag#getValues() 的口径就是展开后的集合，
 * 运行期再展开一层只会让「标签少了一层引用」这类错误变得不可见。
 *
 * 重新生成：见 tests/diff 的 block_tags harness 与 README「差分测试」一节；
 * 该 harness 会逐标签比对展开后的成员。
 */

#include "common/world/block/BlockTags.hpp"

namespace mc {

namespace {


const char* const kValues0[] = {
    "acacia_log", "acacia_wood", "stripped_acacia_log", "stripped_acacia_wood",
};

const char* const kValues1[] = {
    "air", "cave_air", "void_air",
};

const char* const kValues2[] = {
    "acacia_hanging_sign", "acacia_wall_hanging_sign", "bamboo_hanging_sign", "bamboo_wall_hanging_sign",
    "birch_hanging_sign", "birch_wall_hanging_sign", "cherry_hanging_sign", "cherry_wall_hanging_sign",
    "crimson_hanging_sign", "crimson_wall_hanging_sign", "dark_oak_hanging_sign", "dark_oak_wall_hanging_sign",
    "jungle_hanging_sign", "jungle_wall_hanging_sign", "mangrove_hanging_sign", "mangrove_wall_hanging_sign",
    "oak_hanging_sign", "oak_wall_hanging_sign", "pale_oak_hanging_sign", "pale_oak_wall_hanging_sign",
    "spruce_hanging_sign", "spruce_wall_hanging_sign", "warped_hanging_sign", "warped_wall_hanging_sign",
};

const char* const kValues3[] = {
    "acacia_hanging_sign", "acacia_sign", "acacia_wall_hanging_sign", "acacia_wall_sign",
    "bamboo_hanging_sign", "bamboo_sign", "bamboo_wall_hanging_sign", "bamboo_wall_sign", "birch_hanging_sign",
    "birch_sign", "birch_wall_hanging_sign", "birch_wall_sign", "cherry_hanging_sign", "cherry_sign",
    "cherry_wall_hanging_sign", "cherry_wall_sign", "crimson_hanging_sign", "crimson_sign",
    "crimson_wall_hanging_sign", "crimson_wall_sign", "dark_oak_hanging_sign", "dark_oak_sign",
    "dark_oak_wall_hanging_sign", "dark_oak_wall_sign", "jungle_hanging_sign", "jungle_sign",
    "jungle_wall_hanging_sign", "jungle_wall_sign", "mangrove_hanging_sign", "mangrove_sign",
    "mangrove_wall_hanging_sign", "mangrove_wall_sign", "oak_hanging_sign", "oak_sign",
    "oak_wall_hanging_sign", "oak_wall_sign", "pale_oak_hanging_sign", "pale_oak_sign",
    "pale_oak_wall_hanging_sign", "pale_oak_wall_sign", "spruce_hanging_sign", "spruce_sign",
    "spruce_wall_hanging_sign", "spruce_wall_sign", "warped_hanging_sign", "warped_sign",
    "warped_wall_hanging_sign", "warped_wall_sign",
};

const char* const kValues4[] = {
    "cobbled_deepslate", "cracked_deepslate_bricks", "cracked_deepslate_tiles", "deepslate",
    "deepslate_brick_slab", "deepslate_brick_stairs", "deepslate_brick_wall", "deepslate_bricks",
    "deepslate_tile_slab", "deepslate_tile_wall", "deepslate_tiles", "gray_wool",
};

const char* const kValues5[] = {
    "grass_block",
};

const char* const kValues6[] = {
    "anvil", "chipped_anvil", "damaged_anvil",
};

const char* const kValues7[] = {
    "brown_terracotta", "coarse_dirt", "grass_block", "light_gray_terracotta", "orange_terracotta", "red_sand",
    "red_terracotta", "terracotta", "white_terracotta", "yellow_terracotta",
};

const char* const kValues8[] = {
    "clay",
};

const char* const kValues9[] = {
    "black_terracotta", "blue_terracotta", "brown_terracotta", "coarse_dirt", "cyan_terracotta", "dirt",
    "grass_block", "gray_terracotta", "green_terracotta", "light_blue_terracotta", "light_gray_terracotta",
    "lime_terracotta", "magenta_terracotta", "moss_block", "mud", "muddy_mangrove_roots", "mycelium",
    "orange_terracotta", "pale_moss_block", "pink_terracotta", "podzol", "powder_snow", "purple_terracotta",
    "red_sand", "red_terracotta", "rooted_dirt", "sand", "snow_block", "suspicious_sand", "terracotta",
    "white_terracotta", "yellow_terracotta",
};

const char* const kValues10[] = {
    "andesite", "black_terracotta", "blue_terracotta", "brown_terracotta", "clay", "coarse_dirt",
    "cyan_terracotta", "deepslate", "diorite", "dirt", "granite", "grass_block", "gravel", "gray_terracotta",
    "green_terracotta", "light_blue_terracotta", "light_gray_terracotta", "lime_terracotta",
    "magenta_terracotta", "moss_block", "mud", "muddy_mangrove_roots", "mycelium", "orange_terracotta",
    "pale_moss_block", "pink_terracotta", "podzol", "powder_snow", "purple_terracotta", "red_sand",
    "red_terracotta", "rooted_dirt", "sand", "snow_block", "stone", "terracotta", "tuff", "white_terracotta",
    "yellow_terracotta",
};

const char* const kValues11[] = {
    "brown_terracotta", "light_gray_terracotta", "orange_terracotta", "red_terracotta", "terracotta",
    "white_terracotta", "yellow_terracotta",
};

const char* const kValues12[] = {
    "bamboo_block", "stripped_bamboo_block",
};

const char* const kValues13[] = {
    "bamboo", "bamboo_sapling", "coarse_dirt", "dirt", "grass_block", "gravel", "moss_block", "mud",
    "muddy_mangrove_roots", "mycelium", "pale_moss_block", "podzol", "red_sand", "rooted_dirt", "sand",
    "suspicious_gravel", "suspicious_sand",
};

const char* const kValues14[] = {
    "black_banner", "black_wall_banner", "blue_banner", "blue_wall_banner", "brown_banner",
    "brown_wall_banner", "cyan_banner", "cyan_wall_banner", "gray_banner", "gray_wall_banner", "green_banner",
    "green_wall_banner", "light_blue_banner", "light_blue_wall_banner", "light_gray_banner",
    "light_gray_wall_banner", "lime_banner", "lime_wall_banner", "magenta_banner", "magenta_wall_banner",
    "orange_banner", "orange_wall_banner", "pink_banner", "pink_wall_banner", "purple_banner",
    "purple_wall_banner", "red_banner", "red_wall_banner", "white_banner", "white_wall_banner",
    "yellow_banner", "yellow_wall_banner",
};

const char* const kValues15[] = {
    "copper_bars", "exposed_copper_bars", "iron_bars", "oxidized_copper_bars", "waxed_copper_bars",
    "waxed_exposed_copper_bars", "waxed_oxidized_copper_bars", "waxed_weathered_copper_bars",
    "weathered_copper_bars",
};

const char* const kValues16[] = {
    "basalt", "blackstone", "netherrack",
};

const char* const kValues17[] = {
    "andesite", "deepslate", "diorite", "granite", "stone", "tuff",
};

const char* const kValues18[] = {
    "andesite", "deepslate", "diorite", "granite", "stone", "tuff",
};

const char* const kValues19[] = {
    "diamond_block", "emerald_block", "gold_block", "iron_block", "netherite_block",
};

const char* const kValues20[] = {
    "black_bed", "blue_bed", "brown_bed", "cyan_bed", "gray_bed", "green_bed", "light_blue_bed",
    "light_gray_bed", "lime_bed", "magenta_bed", "orange_bed", "pink_bed", "purple_bed", "red_bed",
    "white_bed", "yellow_bed",
};

const char* const kValues21[] = {
    "allium", "azure_bluet", "blue_orchid", "cactus_flower", "cherry_leaves", "chorus_flower", "cornflower",
    "dandelion", "flowering_azalea", "flowering_azalea_leaves", "lilac", "lily_of_the_valley",
    "mangrove_propagule", "open_eyeblossom", "orange_tulip", "oxeye_daisy", "peony", "pink_petals",
    "pink_tulip", "pitcher_plant", "poppy", "red_tulip", "rose_bush", "spore_blossom", "sunflower",
    "torchflower", "white_tulip", "wildflowers", "wither_rose",
};

const char* const kValues22[] = {
    "beetroots", "carrots", "cave_vines", "cave_vines_plant", "melon_stem", "pitcher_crop", "potatoes",
    "pumpkin_stem", "sweet_berry_bush", "torchflower_crop", "wheat",
};

const char* const kValues23[] = {
    "bee_nest", "beehive",
};

const char* const kValues24[] = {
    "clay", "coarse_dirt", "dirt", "farmland", "grass_block", "moss_block", "mud", "muddy_mangrove_roots",
    "mycelium", "podzol", "rooted_dirt",
};

const char* const kValues25[] = {
    "birch_log", "birch_wood", "stripped_birch_log", "stripped_birch_wood",
};

const char* const kValues26[] = {
    "barrier", "bedrock",
};

const char* const kValues27[] = {
    "acacia_button", "bamboo_button", "birch_button", "cherry_button", "crimson_button", "dark_oak_button",
    "jungle_button", "mangrove_button", "oak_button", "pale_oak_button", "polished_blackstone_button",
    "spruce_button", "stone_button", "warped_button",
};

const char* const kValues28[] = {
    "black_concrete_powder", "blue_concrete_powder", "brown_concrete_powder", "cyan_concrete_powder",
    "gray_concrete_powder", "green_concrete_powder", "light_blue_concrete_powder",
    "light_gray_concrete_powder", "lime_concrete_powder", "magenta_concrete_powder", "orange_concrete_powder",
    "pink_concrete_powder", "purple_concrete_powder", "red_concrete_powder", "red_sand", "sand",
    "suspicious_sand", "white_concrete_powder", "yellow_concrete_powder",
};

const char* const kValues29[] = {
    "red_sand", "sand", "suspicious_sand",
};

const char* const kValues30[] = {
    "campfire", "soul_campfire",
};

const char* const kValues31[] = {
    "cave_vines", "cave_vines_plant", "twisting_vines", "twisting_vines_plant", "vine", "weeping_vines",
    "weeping_vines_plant",
};

const char* const kValues32[] = {
    "black_candle_cake", "blue_candle_cake", "brown_candle_cake", "candle_cake", "cyan_candle_cake",
    "gray_candle_cake", "green_candle_cake", "light_blue_candle_cake", "light_gray_candle_cake",
    "lime_candle_cake", "magenta_candle_cake", "orange_candle_cake", "pink_candle_cake", "purple_candle_cake",
    "red_candle_cake", "white_candle_cake", "yellow_candle_cake",
};

const char* const kValues33[] = {
    "black_candle", "blue_candle", "brown_candle", "candle", "cyan_candle", "gray_candle", "green_candle",
    "light_blue_candle", "light_gray_candle", "lime_candle", "magenta_candle", "orange_candle", "pink_candle",
    "purple_candle", "red_candle", "white_candle", "yellow_candle",
};

const char* const kValues34[] = {
    "cauldron", "lava_cauldron", "powder_snow_cauldron", "water_cauldron",
};

const char* const kValues35[] = {
    "cave_vines", "cave_vines_plant",
};

const char* const kValues36[] = {
    "acacia_hanging_sign", "bamboo_hanging_sign", "birch_hanging_sign", "cherry_hanging_sign",
    "crimson_hanging_sign", "dark_oak_hanging_sign", "jungle_hanging_sign", "mangrove_hanging_sign",
    "oak_hanging_sign", "pale_oak_hanging_sign", "spruce_hanging_sign", "warped_hanging_sign",
};

const char* const kValues37[] = {
    "copper_chain", "exposed_copper_chain", "iron_chain", "oxidized_copper_chain", "waxed_copper_chain",
    "waxed_exposed_copper_chain", "waxed_oxidized_copper_chain", "waxed_weathered_copper_chain",
    "weathered_copper_chain",
};

const char* const kValues38[] = {
    "cherry_log", "cherry_wood", "stripped_cherry_log", "stripped_cherry_wood",
};

const char* const kValues39[] = {
    "cave_vines", "cave_vines_plant", "ladder", "scaffolding", "twisting_vines", "twisting_vines_plant",
    "vine", "weeping_vines", "weeping_vines_plant",
};

const char* const kValues40[] = {
    "coal_ore", "deepslate_coal_ore",
};

const char* const kValues41[] = {
    "black_carpet", "blue_carpet", "brown_carpet", "crimson_roots", "cyan_carpet", "gray_carpet",
    "green_carpet", "light_blue_carpet", "light_gray_carpet", "lime_carpet", "magenta_carpet", "moss_carpet",
    "nether_sprouts", "orange_carpet", "pale_moss_carpet", "pink_carpet", "purple_carpet", "red_carpet",
    "resin_clump", "snow", "warped_roots", "white_carpet", "yellow_carpet",
};

const char* const kValues42[] = {
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

const char* const kValues43[] = {
    "black_concrete_powder", "blue_concrete_powder", "brown_concrete_powder", "cyan_concrete_powder",
    "gray_concrete_powder", "green_concrete_powder", "light_blue_concrete_powder",
    "light_gray_concrete_powder", "lime_concrete_powder", "magenta_concrete_powder", "orange_concrete_powder",
    "pink_concrete_powder", "purple_concrete_powder", "red_concrete_powder", "white_concrete_powder",
    "yellow_concrete_powder",
};

const char* const kValues44[] = {
    "coarse_dirt", "dirt", "rooted_dirt",
};

const char* const kValues45[] = {
    "copper_block", "exposed_copper", "oxidized_copper", "waxed_copper_block", "waxed_exposed_copper",
    "waxed_oxidized_copper", "waxed_weathered_copper", "weathered_copper",
};

const char* const kValues46[] = {
    "copper_chest", "exposed_copper_chest", "oxidized_copper_chest", "waxed_copper_chest",
    "waxed_exposed_copper_chest", "waxed_oxidized_copper_chest", "waxed_weathered_copper_chest",
    "weathered_copper_chest",
};

const char* const kValues47[] = {
    "copper_golem_statue", "exposed_copper_golem_statue", "oxidized_copper_golem_statue",
    "waxed_copper_golem_statue", "waxed_exposed_copper_golem_statue", "waxed_oxidized_copper_golem_statue",
    "waxed_weathered_copper_golem_statue", "weathered_copper_golem_statue",
};

const char* const kValues48[] = {
    "copper_ore", "deepslate_copper_ore",
};

const char* const kValues49[] = {
    "brain_coral_block", "bubble_coral_block", "fire_coral_block", "horn_coral_block", "tube_coral_block",
};

const char* const kValues50[] = {
    "brain_coral", "bubble_coral", "fire_coral", "horn_coral", "tube_coral",
};

const char* const kValues51[] = {
    "brain_coral", "brain_coral_fan", "bubble_coral", "bubble_coral_fan", "fire_coral", "fire_coral_fan",
    "horn_coral", "horn_coral_fan", "tube_coral", "tube_coral_fan",
};

const char* const kValues52[] = {
    "crimson_hyphae", "crimson_stem", "stripped_crimson_hyphae", "stripped_crimson_stem",
};

const char* const kValues53[] = {
    "beetroots", "carrots", "melon_stem", "pitcher_crop", "potatoes", "pumpkin_stem", "torchflower_crop",
    "wheat",
};

const char* const kValues54[] = {
    "amethyst_block", "budding_amethyst",
};

const char* const kValues55[] = {
    "black_carpet", "black_wool", "blue_carpet", "blue_wool", "brown_carpet", "brown_wool", "cyan_carpet",
    "cyan_wool", "gray_carpet", "gray_wool", "green_carpet", "green_wool", "light_blue_carpet",
    "light_blue_wool", "light_gray_carpet", "light_gray_wool", "lime_carpet", "lime_wool", "magenta_carpet",
    "magenta_wool", "orange_carpet", "orange_wool", "pink_carpet", "pink_wool", "purple_carpet", "purple_wool",
    "red_carpet", "red_wool", "white_carpet", "white_wool", "yellow_carpet", "yellow_wool",
};

const char* const kValues56[] = {
    "dark_oak_log", "dark_oak_wood", "stripped_dark_oak_log", "stripped_dark_oak_wood",
};

const char* const kValues57[] = {
    "deepslate", "tuff",
};

const char* const kValues58[] = {
    "deepslate_diamond_ore", "diamond_ore",
};

const char* const kValues59[] = {
    "coarse_dirt", "dirt", "grass_block", "moss_block", "mud", "muddy_mangrove_roots", "mycelium",
    "pale_moss_block", "podzol", "rooted_dirt",
};

const char* const kValues60[] = {
    "bee_nest", "beehive",
};

const char* const kValues61[] = {
    "acacia_door", "bamboo_door", "birch_door", "cherry_door", "copper_door", "crimson_door", "dark_oak_door",
    "exposed_copper_door", "iron_door", "jungle_door", "mangrove_door", "oak_door", "oxidized_copper_door",
    "pale_oak_door", "spruce_door", "warped_door", "waxed_copper_door", "waxed_exposed_copper_door",
    "waxed_oxidized_copper_door", "waxed_weathered_copper_door", "weathered_copper_door",
};

const char* const kValues62[] = {
    "barrier", "bedrock", "chain_command_block", "command_block", "crying_obsidian", "end_gateway",
    "end_portal", "end_portal_frame", "end_stone", "iron_bars", "jigsaw", "moving_piston", "obsidian",
    "reinforced_deepslate", "repeating_command_block", "respawn_anchor", "structure_block", "test_block",
    "test_instance_block",
};

const char* const kValues63[] = {
    "fire", "light", "soul_fire",
};

const char* const kValues64[] = {
    "andesite", "deepslate", "diorite", "granite", "stone", "tuff",
};

const char* const kValues65[] = {
    "black_terracotta", "blue_terracotta", "brown_terracotta", "coarse_dirt", "cyan_terracotta", "dirt",
    "farmland", "grass_block", "gray_terracotta", "green_terracotta", "light_blue_terracotta",
    "light_gray_terracotta", "lime_terracotta", "magenta_terracotta", "moss_block", "mud",
    "muddy_mangrove_roots", "mycelium", "orange_terracotta", "pale_moss_block", "pink_terracotta", "podzol",
    "purple_terracotta", "red_sand", "red_terracotta", "rooted_dirt", "sand", "suspicious_sand", "terracotta",
    "white_terracotta", "yellow_terracotta",
};

const char* const kValues66[] = {
    "fern", "short_dry_grass", "short_grass", "tall_dry_grass",
};

const char* const kValues67[] = {
    "deepslate_emerald_ore", "emerald_ore",
};

const char* const kValues68[] = {
    "bookshelf",
};

const char* const kValues69[] = {
    "air", "bubble_column", "bush", "cave_air", "crimson_roots", "dead_bush", "fern", "fire", "glow_lichen",
    "hanging_roots", "large_fern", "lava", "leaf_litter", "light", "nether_sprouts", "resin_clump", "seagrass",
    "short_dry_grass", "short_grass", "snow", "soul_fire", "structure_void", "tall_dry_grass", "tall_grass",
    "tall_seagrass", "vine", "void_air", "warped_roots", "water",
};

const char* const kValues70[] = {
    "allium", "azure_bluet", "blue_orchid", "brown_mushroom", "cactus", "cactus_flower", "carved_pumpkin",
    "clay", "closed_eyeblossom", "coarse_dirt", "cornflower", "crimson_fungus", "crimson_nylium",
    "crimson_roots", "dandelion", "dirt", "grass_block", "gravel", "lily_of_the_valley", "melon", "moss_block",
    "mud", "muddy_mangrove_roots", "mycelium", "open_eyeblossom", "orange_tulip", "oxeye_daisy",
    "pale_moss_block", "pink_tulip", "podzol", "poppy", "pumpkin", "red_mushroom", "red_sand", "red_tulip",
    "rooted_dirt", "sand", "tnt", "torchflower", "warped_fungus", "warped_nylium", "warped_roots",
    "white_tulip", "wither_rose",
};

const char* const kValues71[] = {
    "cave_vines", "cave_vines_plant", "cobweb", "ladder", "scaffolding", "sweet_berry_bush", "twisting_vines",
    "twisting_vines_plant", "vine", "weeping_vines", "weeping_vines_plant",
};

const char* const kValues72[] = {
    "bedrock", "chest", "end_portal_frame", "reinforced_deepslate", "spawner", "trial_spawner", "vault",
};

const char* const kValues73[] = {
    "acacia_fence_gate", "bamboo_fence_gate", "birch_fence_gate", "cherry_fence_gate", "crimson_fence_gate",
    "dark_oak_fence_gate", "jungle_fence_gate", "mangrove_fence_gate", "oak_fence_gate", "pale_oak_fence_gate",
    "spruce_fence_gate", "warped_fence_gate",
};

const char* const kValues74[] = {
    "acacia_fence", "bamboo_fence", "birch_fence", "cherry_fence", "crimson_fence", "dark_oak_fence",
    "jungle_fence", "mangrove_fence", "nether_brick_fence", "oak_fence", "pale_oak_fence", "spruce_fence",
    "warped_fence",
};

const char* const kValues75[] = {
    "fire", "soul_fire",
};

const char* const kValues76[] = {
    "flower_pot", "potted_acacia_sapling", "potted_allium", "potted_azalea_bush", "potted_azure_bluet",
    "potted_bamboo", "potted_birch_sapling", "potted_blue_orchid", "potted_brown_mushroom", "potted_cactus",
    "potted_cherry_sapling", "potted_closed_eyeblossom", "potted_cornflower", "potted_crimson_fungus",
    "potted_crimson_roots", "potted_dandelion", "potted_dark_oak_sapling", "potted_dead_bush", "potted_fern",
    "potted_flowering_azalea_bush", "potted_jungle_sapling", "potted_lily_of_the_valley",
    "potted_mangrove_propagule", "potted_oak_sapling", "potted_open_eyeblossom", "potted_orange_tulip",
    "potted_oxeye_daisy", "potted_pale_oak_sapling", "potted_pink_tulip", "potted_poppy",
    "potted_red_mushroom", "potted_red_tulip", "potted_spruce_sapling", "potted_torchflower",
    "potted_warped_fungus", "potted_warped_roots", "potted_white_tulip", "potted_wither_rose",
};

const char* const kValues77[] = {
    "allium", "azure_bluet", "blue_orchid", "cactus_flower", "cherry_leaves", "chorus_flower",
    "closed_eyeblossom", "cornflower", "dandelion", "flowering_azalea", "flowering_azalea_leaves", "lilac",
    "lily_of_the_valley", "mangrove_propagule", "open_eyeblossom", "orange_tulip", "oxeye_daisy", "peony",
    "pink_petals", "pink_tulip", "pitcher_plant", "poppy", "red_tulip", "rose_bush", "spore_blossom",
    "sunflower", "torchflower", "white_tulip", "wildflowers", "wither_rose",
};

const char* const kValues78[] = {
    "coarse_dirt", "grass_block", "podzol", "snow", "snow_block",
};

const char* const kValues79[] = {
    "big_dripleaf", "lily_pad",
};

const char* const kValues80[] = {
    "grass_block", "mangrove_roots", "mud", "muddy_mangrove_roots",
};

const char* const kValues81[] = {
    "bedrock", "blue_ice", "ice", "lava", "packed_ice", "water",
};

const char* const kValues82[] = {
    "grass_block", "gravel", "packed_ice", "snow", "snow_block", "stone",
};

const char* const kValues83[] = {
    "deepslate_gold_ore", "gold_ore", "nether_gold_ore",
};

const char* const kValues84[] = {
    "barrel", "black_shulker_box", "blue_shulker_box", "brown_shulker_box", "chest", "copper_chest",
    "cyan_shulker_box", "deepslate_gold_ore", "ender_chest", "exposed_copper_chest", "gilded_blackstone",
    "gold_block", "gold_ore", "gray_shulker_box", "green_shulker_box", "light_blue_shulker_box",
    "light_gray_shulker_box", "lime_shulker_box", "magenta_shulker_box", "nether_gold_ore",
    "orange_shulker_box", "oxidized_copper_chest", "pink_shulker_box", "purple_shulker_box", "raw_gold_block",
    "red_shulker_box", "shulker_box", "trapped_chest", "waxed_copper_chest", "waxed_exposed_copper_chest",
    "waxed_oxidized_copper_chest", "waxed_weathered_copper_chest", "weathered_copper_chest",
    "white_shulker_box", "yellow_shulker_box",
};

const char* const kValues85[] = {
    "cactus", "fire", "magma_block", "pointed_dripstone", "sweet_berry_bush", "wither_rose",
};

const char* const kValues86[] = {
    "nether_portal", "potted_warped_fungus", "respawn_anchor", "warped_fungus",
};

const char* const kValues87[] = {
    "blue_ice", "frosted_ice", "ice", "packed_ice",
};

const char* const kValues88[] = {
    "barrier", "black_stained_glass", "blue_stained_glass", "brown_stained_glass", "cyan_stained_glass",
    "glass", "gray_stained_glass", "green_stained_glass", "light_blue_stained_glass",
    "light_gray_stained_glass", "lime_stained_glass", "magenta_stained_glass", "orange_stained_glass",
    "pink_stained_glass", "purple_stained_glass", "red_stained_glass", "tinted_glass", "white_stained_glass",
    "yellow_stained_glass",
};

const char* const kValues89[] = {
    "ancient_debris", "crying_obsidian", "deepslate_diamond_ore", "deepslate_emerald_ore",
    "deepslate_gold_ore", "deepslate_redstone_ore", "diamond_block", "diamond_ore", "emerald_block",
    "emerald_ore", "gold_block", "gold_ore", "netherite_block", "obsidian", "raw_gold_block", "redstone_ore",
    "respawn_anchor",
};

const char* const kValues90[] = {nullptr};

const char* const kValues91[] = {
    "ancient_debris", "chiseled_copper", "copper_block", "copper_bulb", "copper_chest", "copper_grate",
    "copper_ore", "copper_trapdoor", "crafter", "crying_obsidian", "cut_copper", "cut_copper_slab",
    "cut_copper_stairs", "deepslate_copper_ore", "deepslate_diamond_ore", "deepslate_emerald_ore",
    "deepslate_gold_ore", "deepslate_iron_ore", "deepslate_lapis_ore", "deepslate_redstone_ore",
    "diamond_block", "diamond_ore", "emerald_block", "emerald_ore", "exposed_chiseled_copper",
    "exposed_copper", "exposed_copper_bulb", "exposed_copper_chest", "exposed_copper_grate",
    "exposed_copper_trapdoor", "exposed_cut_copper", "exposed_cut_copper_slab", "exposed_cut_copper_stairs",
    "exposed_lightning_rod", "gold_block", "gold_ore", "iron_block", "iron_ore", "lapis_block", "lapis_ore",
    "lightning_rod", "netherite_block", "obsidian", "oxidized_chiseled_copper", "oxidized_copper",
    "oxidized_copper_bulb", "oxidized_copper_chest", "oxidized_copper_grate", "oxidized_copper_trapdoor",
    "oxidized_cut_copper", "oxidized_cut_copper_slab", "oxidized_cut_copper_stairs", "oxidized_lightning_rod",
    "raw_copper_block", "raw_gold_block", "raw_iron_block", "redstone_ore", "respawn_anchor",
    "waxed_chiseled_copper", "waxed_copper_block", "waxed_copper_bulb", "waxed_copper_chest",
    "waxed_copper_grate", "waxed_copper_trapdoor", "waxed_cut_copper", "waxed_cut_copper_slab",
    "waxed_cut_copper_stairs", "waxed_exposed_chiseled_copper", "waxed_exposed_copper",
    "waxed_exposed_copper_bulb", "waxed_exposed_copper_chest", "waxed_exposed_copper_grate",
    "waxed_exposed_copper_trapdoor", "waxed_exposed_cut_copper", "waxed_exposed_cut_copper_slab",
    "waxed_exposed_cut_copper_stairs", "waxed_exposed_lightning_rod", "waxed_lightning_rod",
    "waxed_oxidized_chiseled_copper", "waxed_oxidized_copper", "waxed_oxidized_copper_bulb",
    "waxed_oxidized_copper_chest", "waxed_oxidized_copper_grate", "waxed_oxidized_copper_trapdoor",
    "waxed_oxidized_cut_copper", "waxed_oxidized_cut_copper_slab", "waxed_oxidized_cut_copper_stairs",
    "waxed_oxidized_lightning_rod", "waxed_weathered_chiseled_copper", "waxed_weathered_copper",
    "waxed_weathered_copper_bulb", "waxed_weathered_copper_chest", "waxed_weathered_copper_grate",
    "waxed_weathered_copper_trapdoor", "waxed_weathered_cut_copper", "waxed_weathered_cut_copper_slab",
    "waxed_weathered_cut_copper_stairs", "waxed_weathered_lightning_rod", "weathered_chiseled_copper",
    "weathered_copper", "weathered_copper_bulb", "weathered_copper_chest", "weathered_copper_grate",
    "weathered_copper_trapdoor", "weathered_cut_copper", "weathered_cut_copper_slab",
    "weathered_cut_copper_stairs", "weathered_lightning_rod",
};

const char* const kValues92[] = {
    "ancient_debris", "crying_obsidian", "netherite_block", "obsidian", "respawn_anchor",
};

const char* const kValues93[] = {nullptr};

const char* const kValues94[] = {
    "ancient_debris", "crying_obsidian", "deepslate_diamond_ore", "deepslate_emerald_ore",
    "deepslate_gold_ore", "deepslate_redstone_ore", "diamond_block", "diamond_ore", "emerald_block",
    "emerald_ore", "gold_block", "gold_ore", "netherite_block", "obsidian", "raw_gold_block", "redstone_ore",
    "respawn_anchor",
};

const char* const kValues95[] = {
    "ancient_debris", "chiseled_copper", "copper_block", "copper_bulb", "copper_chest", "copper_grate",
    "copper_ore", "copper_trapdoor", "crafter", "crying_obsidian", "cut_copper", "cut_copper_slab",
    "cut_copper_stairs", "deepslate_copper_ore", "deepslate_diamond_ore", "deepslate_emerald_ore",
    "deepslate_gold_ore", "deepslate_iron_ore", "deepslate_lapis_ore", "deepslate_redstone_ore",
    "diamond_block", "diamond_ore", "emerald_block", "emerald_ore", "exposed_chiseled_copper",
    "exposed_copper", "exposed_copper_bulb", "exposed_copper_chest", "exposed_copper_grate",
    "exposed_copper_trapdoor", "exposed_cut_copper", "exposed_cut_copper_slab", "exposed_cut_copper_stairs",
    "exposed_lightning_rod", "gold_block", "gold_ore", "iron_block", "iron_ore", "lapis_block", "lapis_ore",
    "lightning_rod", "netherite_block", "obsidian", "oxidized_chiseled_copper", "oxidized_copper",
    "oxidized_copper_bulb", "oxidized_copper_chest", "oxidized_copper_grate", "oxidized_copper_trapdoor",
    "oxidized_cut_copper", "oxidized_cut_copper_slab", "oxidized_cut_copper_stairs", "oxidized_lightning_rod",
    "raw_copper_block", "raw_gold_block", "raw_iron_block", "redstone_ore", "respawn_anchor",
    "waxed_chiseled_copper", "waxed_copper_block", "waxed_copper_bulb", "waxed_copper_chest",
    "waxed_copper_grate", "waxed_copper_trapdoor", "waxed_cut_copper", "waxed_cut_copper_slab",
    "waxed_cut_copper_stairs", "waxed_exposed_chiseled_copper", "waxed_exposed_copper",
    "waxed_exposed_copper_bulb", "waxed_exposed_copper_chest", "waxed_exposed_copper_grate",
    "waxed_exposed_copper_trapdoor", "waxed_exposed_cut_copper", "waxed_exposed_cut_copper_slab",
    "waxed_exposed_cut_copper_stairs", "waxed_exposed_lightning_rod", "waxed_lightning_rod",
    "waxed_oxidized_chiseled_copper", "waxed_oxidized_copper", "waxed_oxidized_copper_bulb",
    "waxed_oxidized_copper_chest", "waxed_oxidized_copper_grate", "waxed_oxidized_copper_trapdoor",
    "waxed_oxidized_cut_copper", "waxed_oxidized_cut_copper_slab", "waxed_oxidized_cut_copper_stairs",
    "waxed_oxidized_lightning_rod", "waxed_weathered_chiseled_copper", "waxed_weathered_copper",
    "waxed_weathered_copper_bulb", "waxed_weathered_copper_chest", "waxed_weathered_copper_grate",
    "waxed_weathered_copper_trapdoor", "waxed_weathered_cut_copper", "waxed_weathered_cut_copper_slab",
    "waxed_weathered_cut_copper_stairs", "waxed_weathered_lightning_rod", "weathered_chiseled_copper",
    "weathered_copper", "weathered_copper_bulb", "weathered_copper_chest", "weathered_copper_grate",
    "weathered_copper_trapdoor", "weathered_cut_copper", "weathered_cut_copper_slab",
    "weathered_cut_copper_stairs", "weathered_lightning_rod",
};

const char* const kValues96[] = {
    "bedrock", "magma_block", "netherrack",
};

const char* const kValues97[] = {
    "magma_block", "netherrack",
};

const char* const kValues98[] = {
    "magma_block", "netherrack",
};

const char* const kValues99[] = {
    "glow_lichen", "leaf_litter", "lily_pad", "pink_petals", "powder_snow", "sculk_vein", "small_amethyst_bud",
    "wildflowers",
};

const char* const kValues100[] = {
    "end_gateway", "end_portal",
};

const char* const kValues101[] = {
    "deepslate_iron_ore", "iron_ore",
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
    "acacia_leaves", "acacia_log", "acacia_wood", "azalea_leaves", "bedrock", "birch_leaves", "birch_log",
    "birch_wood", "cherry_leaves", "cherry_log", "cherry_wood", "chest", "crimson_hyphae", "crimson_stem",
    "dark_oak_leaves", "dark_oak_log", "dark_oak_wood", "end_portal_frame", "flowering_azalea_leaves",
    "jungle_leaves", "jungle_log", "jungle_wood", "mangrove_leaves", "mangrove_log", "mangrove_wood",
    "oak_leaves", "oak_log", "oak_wood", "pale_oak_leaves", "pale_oak_log", "pale_oak_wood",
    "reinforced_deepslate", "spawner", "spruce_leaves", "spruce_log", "spruce_wood", "stripped_acacia_log",
    "stripped_acacia_wood", "stripped_birch_log", "stripped_birch_wood", "stripped_cherry_log",
    "stripped_cherry_wood", "stripped_crimson_hyphae", "stripped_crimson_stem", "stripped_dark_oak_log",
    "stripped_dark_oak_wood", "stripped_jungle_log", "stripped_jungle_wood", "stripped_mangrove_log",
    "stripped_mangrove_wood", "stripped_oak_log", "stripped_oak_wood", "stripped_pale_oak_log",
    "stripped_pale_oak_wood", "stripped_spruce_log", "stripped_spruce_wood", "stripped_warped_hyphae",
    "stripped_warped_stem", "trial_spawner", "vault", "warped_hyphae", "warped_stem",
};

const char* const kValues106[] = {
    "acacia_leaves", "azalea_leaves", "birch_leaves", "cherry_leaves", "dark_oak_leaves",
    "flowering_azalea_leaves", "jungle_leaves", "mangrove_leaves", "oak_leaves", "pale_oak_leaves",
    "spruce_leaves",
};

const char* const kValues107[] = {
    "exposed_lightning_rod", "lightning_rod", "oxidized_lightning_rod", "waxed_exposed_lightning_rod",
    "waxed_lightning_rod", "waxed_oxidized_lightning_rod", "waxed_weathered_lightning_rod",
    "weathered_lightning_rod",
};

const char* const kValues108[] = {
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

const char* const kValues109[] = {
    "acacia_log", "acacia_wood", "birch_log", "birch_wood", "cherry_log", "cherry_wood", "dark_oak_log",
    "dark_oak_wood", "jungle_log", "jungle_wood", "mangrove_log", "mangrove_wood", "oak_log", "oak_wood",
    "pale_oak_log", "pale_oak_wood", "spruce_log", "spruce_wood", "stripped_acacia_log",
    "stripped_acacia_wood", "stripped_birch_log", "stripped_birch_wood", "stripped_cherry_log",
    "stripped_cherry_wood", "stripped_dark_oak_log", "stripped_dark_oak_wood", "stripped_jungle_log",
    "stripped_jungle_wood", "stripped_mangrove_log", "stripped_mangrove_wood", "stripped_oak_log",
    "stripped_oak_wood", "stripped_pale_oak_log", "stripped_pale_oak_wood", "stripped_spruce_log",
    "stripped_spruce_wood",
};

const char* const kValues110[] = {
    "andesite", "cave_vines", "cave_vines_plant", "clay", "coarse_dirt", "deepslate", "diorite", "dirt",
    "granite", "grass_block", "gravel", "moss_block", "mud", "muddy_mangrove_roots", "mycelium",
    "pale_moss_block", "podzol", "rooted_dirt", "sand", "stone", "tuff",
};

const char* const kValues111[] = {
    "attached_melon_stem", "attached_pumpkin_stem", "beetroots", "carrots", "melon_stem", "pitcher_crop",
    "potatoes", "pumpkin_stem", "torchflower", "torchflower_crop", "wheat",
};

const char* const kValues112[] = {
    "mangrove_log", "mangrove_wood", "stripped_mangrove_log", "stripped_mangrove_wood",
};

const char* const kValues113[] = {
    "mangrove_leaves", "mangrove_log", "mangrove_propagule", "mangrove_roots", "moss_carpet", "mud",
    "muddy_mangrove_roots", "vine",
};

const char* const kValues114[] = {
    "mangrove_propagule", "mangrove_roots", "moss_carpet", "mud", "muddy_mangrove_roots", "snow", "vine",
};

const char* const kValues115[] = {
    "acacia_button", "acacia_door", "acacia_fence", "acacia_fence_gate", "acacia_hanging_sign", "acacia_log",
    "acacia_planks", "acacia_pressure_plate", "acacia_shelf", "acacia_sign", "acacia_slab", "acacia_stairs",
    "acacia_trapdoor", "acacia_wall_hanging_sign", "acacia_wall_sign", "acacia_wood", "bamboo", "bamboo_block",
    "bamboo_button", "bamboo_door", "bamboo_fence", "bamboo_fence_gate", "bamboo_hanging_sign",
    "bamboo_mosaic", "bamboo_mosaic_slab", "bamboo_mosaic_stairs", "bamboo_planks", "bamboo_pressure_plate",
    "bamboo_shelf", "bamboo_sign", "bamboo_slab", "bamboo_stairs", "bamboo_trapdoor",
    "bamboo_wall_hanging_sign", "bamboo_wall_sign", "barrel", "bee_nest", "beehive", "big_dripleaf",
    "big_dripleaf_stem", "birch_button", "birch_door", "birch_fence", "birch_fence_gate", "birch_hanging_sign",
    "birch_log", "birch_planks", "birch_pressure_plate", "birch_shelf", "birch_sign", "birch_slab",
    "birch_stairs", "birch_trapdoor", "birch_wall_hanging_sign", "birch_wall_sign", "birch_wood",
    "black_banner", "black_wall_banner", "blue_banner", "blue_wall_banner", "bookshelf", "brown_banner",
    "brown_mushroom_block", "brown_wall_banner", "campfire", "cartography_table", "carved_pumpkin",
    "cherry_button", "cherry_door", "cherry_fence", "cherry_fence_gate", "cherry_hanging_sign", "cherry_log",
    "cherry_planks", "cherry_pressure_plate", "cherry_shelf", "cherry_sign", "cherry_slab", "cherry_stairs",
    "cherry_trapdoor", "cherry_wall_hanging_sign", "cherry_wall_sign", "cherry_wood", "chest",
    "chiseled_bookshelf", "chorus_flower", "chorus_plant", "cocoa", "composter", "crafting_table",
    "creaking_heart", "crimson_button", "crimson_door", "crimson_fence", "crimson_fence_gate",
    "crimson_hanging_sign", "crimson_hyphae", "crimson_planks", "crimson_pressure_plate", "crimson_shelf",
    "crimson_sign", "crimson_slab", "crimson_stairs", "crimson_stem", "crimson_trapdoor",
    "crimson_wall_hanging_sign", "crimson_wall_sign", "cyan_banner", "cyan_wall_banner", "dark_oak_button",
    "dark_oak_door", "dark_oak_fence", "dark_oak_fence_gate", "dark_oak_hanging_sign", "dark_oak_log",
    "dark_oak_planks", "dark_oak_pressure_plate", "dark_oak_shelf", "dark_oak_sign", "dark_oak_slab",
    "dark_oak_stairs", "dark_oak_trapdoor", "dark_oak_wall_hanging_sign", "dark_oak_wall_sign",
    "dark_oak_wood", "daylight_detector", "fletching_table", "glow_lichen", "gray_banner", "gray_wall_banner",
    "green_banner", "green_wall_banner", "jack_o_lantern", "jukebox", "jungle_button", "jungle_door",
    "jungle_fence", "jungle_fence_gate", "jungle_hanging_sign", "jungle_log", "jungle_planks",
    "jungle_pressure_plate", "jungle_shelf", "jungle_sign", "jungle_slab", "jungle_stairs", "jungle_trapdoor",
    "jungle_wall_hanging_sign", "jungle_wall_sign", "jungle_wood", "ladder", "lectern", "light_blue_banner",
    "light_blue_wall_banner", "light_gray_banner", "light_gray_wall_banner", "lime_banner", "lime_wall_banner",
    "loom", "magenta_banner", "magenta_wall_banner", "mangrove_button", "mangrove_door", "mangrove_fence",
    "mangrove_fence_gate", "mangrove_hanging_sign", "mangrove_log", "mangrove_planks",
    "mangrove_pressure_plate", "mangrove_roots", "mangrove_shelf", "mangrove_sign", "mangrove_slab",
    "mangrove_stairs", "mangrove_trapdoor", "mangrove_wall_hanging_sign", "mangrove_wall_sign",
    "mangrove_wood", "melon", "mushroom_stem", "note_block", "oak_button", "oak_door", "oak_fence",
    "oak_fence_gate", "oak_hanging_sign", "oak_log", "oak_planks", "oak_pressure_plate", "oak_shelf",
    "oak_sign", "oak_slab", "oak_stairs", "oak_trapdoor", "oak_wall_hanging_sign", "oak_wall_sign", "oak_wood",
    "orange_banner", "orange_wall_banner", "pale_oak_button", "pale_oak_door", "pale_oak_fence",
    "pale_oak_fence_gate", "pale_oak_hanging_sign", "pale_oak_log", "pale_oak_planks",
    "pale_oak_pressure_plate", "pale_oak_shelf", "pale_oak_sign", "pale_oak_slab", "pale_oak_stairs",
    "pale_oak_trapdoor", "pale_oak_wall_hanging_sign", "pale_oak_wall_sign", "pale_oak_wood", "pink_banner",
    "pink_wall_banner", "pumpkin", "purple_banner", "purple_wall_banner", "red_banner", "red_mushroom_block",
    "red_wall_banner", "smithing_table", "soul_campfire", "spruce_button", "spruce_door", "spruce_fence",
    "spruce_fence_gate", "spruce_hanging_sign", "spruce_log", "spruce_planks", "spruce_pressure_plate",
    "spruce_shelf", "spruce_sign", "spruce_slab", "spruce_stairs", "spruce_trapdoor",
    "spruce_wall_hanging_sign", "spruce_wall_sign", "spruce_wood", "stripped_acacia_log",
    "stripped_acacia_wood", "stripped_bamboo_block", "stripped_birch_log", "stripped_birch_wood",
    "stripped_cherry_log", "stripped_cherry_wood", "stripped_crimson_hyphae", "stripped_crimson_stem",
    "stripped_dark_oak_log", "stripped_dark_oak_wood", "stripped_jungle_log", "stripped_jungle_wood",
    "stripped_mangrove_log", "stripped_mangrove_wood", "stripped_oak_log", "stripped_oak_wood",
    "stripped_pale_oak_log", "stripped_pale_oak_wood", "stripped_spruce_log", "stripped_spruce_wood",
    "stripped_warped_hyphae", "stripped_warped_stem", "trapped_chest", "vine", "warped_button", "warped_door",
    "warped_fence", "warped_fence_gate", "warped_hanging_sign", "warped_hyphae", "warped_planks",
    "warped_pressure_plate", "warped_shelf", "warped_sign", "warped_slab", "warped_stairs", "warped_stem",
    "warped_trapdoor", "warped_wall_hanging_sign", "warped_wall_sign", "white_banner", "white_wall_banner",
    "yellow_banner", "yellow_wall_banner",
};

const char* const kValues116[] = {
    "acacia_leaves", "azalea_leaves", "birch_leaves", "calibrated_sculk_sensor", "cherry_leaves",
    "dark_oak_leaves", "dried_kelp_block", "flowering_azalea_leaves", "hay_block", "jungle_leaves",
    "mangrove_leaves", "moss_block", "moss_carpet", "nether_wart_block", "oak_leaves", "pale_moss_block",
    "pale_moss_carpet", "pale_oak_leaves", "sculk", "sculk_catalyst", "sculk_sensor", "sculk_shrieker",
    "sculk_vein", "shroomlight", "sponge", "spruce_leaves", "target", "warped_wart_block", "wet_sponge",
};

const char* const kValues117[] = {
    "activator_rail", "amethyst_block", "amethyst_cluster", "ancient_debris", "andesite", "andesite_slab",
    "andesite_stairs", "andesite_wall", "anvil", "basalt", "bell", "black_concrete", "black_glazed_terracotta",
    "black_shulker_box", "black_terracotta", "blackstone", "blackstone_slab", "blackstone_stairs",
    "blackstone_wall", "blast_furnace", "blue_concrete", "blue_glazed_terracotta", "blue_ice",
    "blue_shulker_box", "blue_terracotta", "bone_block", "brain_coral_block", "brewing_stand", "brick_slab",
    "brick_stairs", "brick_wall", "bricks", "brown_concrete", "brown_glazed_terracotta", "brown_shulker_box",
    "brown_terracotta", "bubble_coral_block", "budding_amethyst", "calcite", "cauldron", "chipped_anvil",
    "chiseled_copper", "chiseled_deepslate", "chiseled_nether_bricks", "chiseled_polished_blackstone",
    "chiseled_quartz_block", "chiseled_red_sandstone", "chiseled_resin_bricks", "chiseled_sandstone",
    "chiseled_stone_bricks", "chiseled_tuff", "chiseled_tuff_bricks", "coal_block", "coal_ore",
    "cobbled_deepslate", "cobbled_deepslate_slab", "cobbled_deepslate_stairs", "cobbled_deepslate_wall",
    "cobblestone", "cobblestone_slab", "cobblestone_stairs", "cobblestone_wall", "conduit", "copper_bars",
    "copper_block", "copper_bulb", "copper_chain", "copper_chest", "copper_door", "copper_golem_statue",
    "copper_grate", "copper_lantern", "copper_ore", "copper_trapdoor", "cracked_deepslate_bricks",
    "cracked_deepslate_tiles", "cracked_nether_bricks", "cracked_polished_blackstone_bricks",
    "cracked_stone_bricks", "crafter", "crimson_nylium", "crying_obsidian", "cut_copper", "cut_copper_slab",
    "cut_copper_stairs", "cut_red_sandstone", "cut_red_sandstone_slab", "cut_sandstone", "cut_sandstone_slab",
    "cyan_concrete", "cyan_glazed_terracotta", "cyan_shulker_box", "cyan_terracotta", "damaged_anvil",
    "dark_prismarine", "dark_prismarine_slab", "dark_prismarine_stairs", "dead_brain_coral",
    "dead_brain_coral_block", "dead_brain_coral_fan", "dead_brain_coral_wall_fan", "dead_bubble_coral",
    "dead_bubble_coral_block", "dead_bubble_coral_fan", "dead_bubble_coral_wall_fan", "dead_fire_coral",
    "dead_fire_coral_block", "dead_fire_coral_fan", "dead_fire_coral_wall_fan", "dead_horn_coral",
    "dead_horn_coral_block", "dead_horn_coral_fan", "dead_horn_coral_wall_fan", "dead_tube_coral",
    "dead_tube_coral_block", "dead_tube_coral_fan", "dead_tube_coral_wall_fan", "deepslate",
    "deepslate_brick_slab", "deepslate_brick_stairs", "deepslate_brick_wall", "deepslate_bricks",
    "deepslate_coal_ore", "deepslate_copper_ore", "deepslate_diamond_ore", "deepslate_emerald_ore",
    "deepslate_gold_ore", "deepslate_iron_ore", "deepslate_lapis_ore", "deepslate_redstone_ore",
    "deepslate_tile_slab", "deepslate_tile_stairs", "deepslate_tile_wall", "deepslate_tiles", "detector_rail",
    "diamond_block", "diamond_ore", "diorite", "diorite_slab", "diorite_stairs", "diorite_wall", "dispenser",
    "dripstone_block", "dropper", "emerald_block", "emerald_ore", "enchanting_table", "end_stone",
    "end_stone_brick_slab", "end_stone_brick_stairs", "end_stone_brick_wall", "end_stone_bricks",
    "ender_chest", "exposed_chiseled_copper", "exposed_copper", "exposed_copper_bars", "exposed_copper_bulb",
    "exposed_copper_chain", "exposed_copper_chest", "exposed_copper_door", "exposed_copper_golem_statue",
    "exposed_copper_grate", "exposed_copper_lantern", "exposed_copper_trapdoor", "exposed_cut_copper",
    "exposed_cut_copper_slab", "exposed_cut_copper_stairs", "exposed_lightning_rod", "fire_coral_block",
    "furnace", "gilded_blackstone", "gold_block", "gold_ore", "granite", "granite_slab", "granite_stairs",
    "granite_wall", "gray_concrete", "gray_glazed_terracotta", "gray_shulker_box", "gray_terracotta",
    "green_concrete", "green_glazed_terracotta", "green_shulker_box", "green_terracotta", "grindstone",
    "heavy_core", "heavy_weighted_pressure_plate", "hopper", "horn_coral_block", "ice",
    "infested_chiseled_stone_bricks", "infested_cobblestone", "infested_cracked_stone_bricks",
    "infested_deepslate", "infested_mossy_stone_bricks", "infested_stone", "infested_stone_bricks",
    "iron_bars", "iron_block", "iron_chain", "iron_door", "iron_ore", "iron_trapdoor", "lantern",
    "lapis_block", "lapis_ore", "large_amethyst_bud", "lava_cauldron", "light_blue_concrete",
    "light_blue_glazed_terracotta", "light_blue_shulker_box", "light_blue_terracotta", "light_gray_concrete",
    "light_gray_glazed_terracotta", "light_gray_shulker_box", "light_gray_terracotta",
    "light_weighted_pressure_plate", "lightning_rod", "lime_concrete", "lime_glazed_terracotta",
    "lime_shulker_box", "lime_terracotta", "lodestone", "magenta_concrete", "magenta_glazed_terracotta",
    "magenta_shulker_box", "magenta_terracotta", "magma_block", "medium_amethyst_bud", "mossy_cobblestone",
    "mossy_cobblestone_slab", "mossy_cobblestone_stairs", "mossy_cobblestone_wall", "mossy_stone_brick_slab",
    "mossy_stone_brick_stairs", "mossy_stone_brick_wall", "mossy_stone_bricks", "mud_brick_slab",
    "mud_brick_stairs", "mud_brick_wall", "mud_bricks", "nether_brick_fence", "nether_brick_slab",
    "nether_brick_stairs", "nether_brick_wall", "nether_bricks", "nether_gold_ore", "nether_quartz_ore",
    "netherite_block", "netherrack", "observer", "obsidian", "orange_concrete", "orange_glazed_terracotta",
    "orange_shulker_box", "orange_terracotta", "oxidized_chiseled_copper", "oxidized_copper",
    "oxidized_copper_bars", "oxidized_copper_bulb", "oxidized_copper_chain", "oxidized_copper_chest",
    "oxidized_copper_door", "oxidized_copper_golem_statue", "oxidized_copper_grate", "oxidized_copper_lantern",
    "oxidized_copper_trapdoor", "oxidized_cut_copper", "oxidized_cut_copper_slab",
    "oxidized_cut_copper_stairs", "oxidized_lightning_rod", "packed_ice", "packed_mud", "petrified_oak_slab",
    "pink_concrete", "pink_glazed_terracotta", "pink_shulker_box", "pink_terracotta", "piston", "piston_head",
    "pointed_dripstone", "polished_andesite", "polished_andesite_slab", "polished_andesite_stairs",
    "polished_basalt", "polished_blackstone", "polished_blackstone_brick_slab",
    "polished_blackstone_brick_stairs", "polished_blackstone_brick_wall", "polished_blackstone_bricks",
    "polished_blackstone_button", "polished_blackstone_pressure_plate", "polished_blackstone_slab",
    "polished_blackstone_stairs", "polished_blackstone_wall", "polished_deepslate", "polished_deepslate_slab",
    "polished_deepslate_stairs", "polished_deepslate_wall", "polished_diorite", "polished_diorite_slab",
    "polished_diorite_stairs", "polished_granite", "polished_granite_slab", "polished_granite_stairs",
    "polished_tuff", "polished_tuff_slab", "polished_tuff_stairs", "polished_tuff_wall",
    "powder_snow_cauldron", "powered_rail", "prismarine", "prismarine_brick_slab", "prismarine_brick_stairs",
    "prismarine_bricks", "prismarine_slab", "prismarine_stairs", "prismarine_wall", "purple_concrete",
    "purple_glazed_terracotta", "purple_shulker_box", "purple_terracotta", "purpur_block", "purpur_pillar",
    "purpur_slab", "purpur_stairs", "quartz_block", "quartz_bricks", "quartz_pillar", "quartz_slab",
    "quartz_stairs", "rail", "raw_copper_block", "raw_gold_block", "raw_iron_block", "red_concrete",
    "red_glazed_terracotta", "red_nether_brick_slab", "red_nether_brick_stairs", "red_nether_brick_wall",
    "red_nether_bricks", "red_sandstone", "red_sandstone_slab", "red_sandstone_stairs", "red_sandstone_wall",
    "red_shulker_box", "red_terracotta", "redstone_block", "redstone_ore", "resin_brick_slab",
    "resin_brick_stairs", "resin_brick_wall", "resin_bricks", "respawn_anchor", "sandstone", "sandstone_slab",
    "sandstone_stairs", "sandstone_wall", "shulker_box", "small_amethyst_bud", "smoker", "smooth_basalt",
    "smooth_quartz", "smooth_quartz_slab", "smooth_quartz_stairs", "smooth_red_sandstone",
    "smooth_red_sandstone_slab", "smooth_red_sandstone_stairs", "smooth_sandstone", "smooth_sandstone_slab",
    "smooth_sandstone_stairs", "smooth_stone", "smooth_stone_slab", "soul_lantern", "spawner", "sticky_piston",
    "stone", "stone_brick_slab", "stone_brick_stairs", "stone_brick_wall", "stone_bricks", "stone_button",
    "stone_pressure_plate", "stone_slab", "stone_stairs", "stonecutter", "terracotta", "tube_coral_block",
    "tuff", "tuff_brick_slab", "tuff_brick_stairs", "tuff_brick_wall", "tuff_bricks", "tuff_slab",
    "tuff_stairs", "tuff_wall", "warped_nylium", "water_cauldron", "waxed_chiseled_copper",
    "waxed_copper_bars", "waxed_copper_block", "waxed_copper_bulb", "waxed_copper_chain", "waxed_copper_chest",
    "waxed_copper_door", "waxed_copper_golem_statue", "waxed_copper_grate", "waxed_copper_lantern",
    "waxed_copper_trapdoor", "waxed_cut_copper", "waxed_cut_copper_slab", "waxed_cut_copper_stairs",
    "waxed_exposed_chiseled_copper", "waxed_exposed_copper", "waxed_exposed_copper_bars",
    "waxed_exposed_copper_bulb", "waxed_exposed_copper_chain", "waxed_exposed_copper_chest",
    "waxed_exposed_copper_door", "waxed_exposed_copper_golem_statue", "waxed_exposed_copper_grate",
    "waxed_exposed_copper_lantern", "waxed_exposed_copper_trapdoor", "waxed_exposed_cut_copper",
    "waxed_exposed_cut_copper_slab", "waxed_exposed_cut_copper_stairs", "waxed_exposed_lightning_rod",
    "waxed_lightning_rod", "waxed_oxidized_chiseled_copper", "waxed_oxidized_copper",
    "waxed_oxidized_copper_bars", "waxed_oxidized_copper_bulb", "waxed_oxidized_copper_chain",
    "waxed_oxidized_copper_chest", "waxed_oxidized_copper_door", "waxed_oxidized_copper_golem_statue",
    "waxed_oxidized_copper_grate", "waxed_oxidized_copper_lantern", "waxed_oxidized_copper_trapdoor",
    "waxed_oxidized_cut_copper", "waxed_oxidized_cut_copper_slab", "waxed_oxidized_cut_copper_stairs",
    "waxed_oxidized_lightning_rod", "waxed_weathered_chiseled_copper", "waxed_weathered_copper",
    "waxed_weathered_copper_bars", "waxed_weathered_copper_bulb", "waxed_weathered_copper_chain",
    "waxed_weathered_copper_chest", "waxed_weathered_copper_door", "waxed_weathered_copper_golem_statue",
    "waxed_weathered_copper_grate", "waxed_weathered_copper_lantern", "waxed_weathered_copper_trapdoor",
    "waxed_weathered_cut_copper", "waxed_weathered_cut_copper_slab", "waxed_weathered_cut_copper_stairs",
    "waxed_weathered_lightning_rod", "weathered_chiseled_copper", "weathered_copper", "weathered_copper_bars",
    "weathered_copper_bulb", "weathered_copper_chain", "weathered_copper_chest", "weathered_copper_door",
    "weathered_copper_golem_statue", "weathered_copper_grate", "weathered_copper_lantern",
    "weathered_copper_trapdoor", "weathered_cut_copper", "weathered_cut_copper_slab",
    "weathered_cut_copper_stairs", "weathered_lightning_rod", "white_concrete", "white_glazed_terracotta",
    "white_shulker_box", "white_terracotta", "yellow_concrete", "yellow_glazed_terracotta",
    "yellow_shulker_box", "yellow_terracotta",
};

const char* const kValues118[] = {
    "black_concrete_powder", "blue_concrete_powder", "brown_concrete_powder", "clay", "coarse_dirt",
    "cyan_concrete_powder", "dirt", "dirt_path", "farmland", "grass_block", "gravel", "gray_concrete_powder",
    "green_concrete_powder", "light_blue_concrete_powder", "light_gray_concrete_powder",
    "lime_concrete_powder", "magenta_concrete_powder", "mud", "muddy_mangrove_roots", "mycelium",
    "orange_concrete_powder", "pink_concrete_powder", "podzol", "purple_concrete_powder",
    "red_concrete_powder", "red_sand", "rooted_dirt", "sand", "snow", "snow_block", "soul_sand", "soul_soil",
    "suspicious_gravel", "suspicious_sand", "white_concrete_powder", "yellow_concrete_powder",
};

const char* const kValues119[] = {
    "acacia_door", "bamboo_door", "birch_door", "cherry_door", "copper_door", "crimson_door", "dark_oak_door",
    "exposed_copper_door", "jungle_door", "mangrove_door", "oak_door", "oxidized_copper_door", "pale_oak_door",
    "spruce_door", "warped_door", "waxed_copper_door", "waxed_exposed_copper_door",
    "waxed_oxidized_copper_door", "waxed_weathered_copper_door", "weathered_copper_door",
};

const char* const kValues120[] = {
    "mycelium",
};

const char* const kValues121[] = {
    "andesite", "cave_vines", "cave_vines_plant", "coarse_dirt", "deepslate", "diorite", "dirt", "granite",
    "grass_block", "moss_block", "mud", "muddy_mangrove_roots", "mycelium", "pale_moss_block", "podzol",
    "rooted_dirt", "stone", "tuff",
};

const char* const kValues122[] = {
    "crimson_nylium", "mycelium", "podzol", "warped_nylium",
};

const char* const kValues123[] = {
    "ancient_debris", "crying_obsidian", "netherite_block", "obsidian", "respawn_anchor",
};

const char* const kValues124[] = {
    "deepslate_diamond_ore", "deepslate_emerald_ore", "deepslate_gold_ore", "deepslate_redstone_ore",
    "diamond_block", "diamond_ore", "emerald_block", "emerald_ore", "gold_block", "gold_ore", "raw_gold_block",
    "redstone_ore",
};

const char* const kValues125[] = {
    "chiseled_copper", "copper_block", "copper_bulb", "copper_chest", "copper_grate", "copper_ore",
    "copper_trapdoor", "crafter", "cut_copper", "cut_copper_slab", "cut_copper_stairs", "deepslate_copper_ore",
    "deepslate_iron_ore", "deepslate_lapis_ore", "exposed_chiseled_copper", "exposed_copper",
    "exposed_copper_bulb", "exposed_copper_chest", "exposed_copper_grate", "exposed_copper_trapdoor",
    "exposed_cut_copper", "exposed_cut_copper_slab", "exposed_cut_copper_stairs", "exposed_lightning_rod",
    "iron_block", "iron_ore", "lapis_block", "lapis_ore", "lightning_rod", "oxidized_chiseled_copper",
    "oxidized_copper", "oxidized_copper_bulb", "oxidized_copper_chest", "oxidized_copper_grate",
    "oxidized_copper_trapdoor", "oxidized_cut_copper", "oxidized_cut_copper_slab",
    "oxidized_cut_copper_stairs", "oxidized_lightning_rod", "raw_copper_block", "raw_iron_block",
    "waxed_chiseled_copper", "waxed_copper_block", "waxed_copper_bulb", "waxed_copper_chest",
    "waxed_copper_grate", "waxed_copper_trapdoor", "waxed_cut_copper", "waxed_cut_copper_slab",
    "waxed_cut_copper_stairs", "waxed_exposed_chiseled_copper", "waxed_exposed_copper",
    "waxed_exposed_copper_bulb", "waxed_exposed_copper_chest", "waxed_exposed_copper_grate",
    "waxed_exposed_copper_trapdoor", "waxed_exposed_cut_copper", "waxed_exposed_cut_copper_slab",
    "waxed_exposed_cut_copper_stairs", "waxed_exposed_lightning_rod", "waxed_lightning_rod",
    "waxed_oxidized_chiseled_copper", "waxed_oxidized_copper", "waxed_oxidized_copper_bulb",
    "waxed_oxidized_copper_chest", "waxed_oxidized_copper_grate", "waxed_oxidized_copper_trapdoor",
    "waxed_oxidized_cut_copper", "waxed_oxidized_cut_copper_slab", "waxed_oxidized_cut_copper_stairs",
    "waxed_oxidized_lightning_rod", "waxed_weathered_chiseled_copper", "waxed_weathered_copper",
    "waxed_weathered_copper_bulb", "waxed_weathered_copper_chest", "waxed_weathered_copper_grate",
    "waxed_weathered_copper_trapdoor", "waxed_weathered_cut_copper", "waxed_weathered_cut_copper_slab",
    "waxed_weathered_cut_copper_stairs", "waxed_weathered_lightning_rod", "weathered_chiseled_copper",
    "weathered_copper", "weathered_copper_bulb", "weathered_copper_chest", "weathered_copper_grate",
    "weathered_copper_trapdoor", "weathered_cut_copper", "weathered_cut_copper_slab",
    "weathered_cut_copper_stairs", "weathered_lightning_rod",
};

const char* const kValues126[] = {
    "andesite", "basalt", "blackstone", "coarse_dirt", "crimson_nylium", "deepslate", "diorite", "dirt",
    "granite", "grass_block", "moss_block", "mud", "muddy_mangrove_roots", "mycelium", "nether_wart_block",
    "netherrack", "pale_moss_block", "podzol", "rooted_dirt", "soul_sand", "soul_soil", "stone", "tuff",
    "warped_nylium", "warped_wart_block",
};

const char* const kValues127[] = {
    "crimson_nylium", "warped_nylium",
};

const char* const kValues128[] = {
    "oak_log", "oak_wood", "stripped_oak_log", "stripped_oak_wood",
};

const char* const kValues129[] = {
    "black_wool", "blue_wool", "brown_wool", "cyan_wool", "gray_wool", "green_wool", "light_blue_wool",
    "light_gray_wool", "lime_wool", "magenta_wool", "orange_wool", "pink_wool", "purple_wool", "red_wool",
    "white_wool", "yellow_wool",
};

const char* const kValues130[] = {
    "andesite", "black_terracotta", "blue_terracotta", "brown_terracotta", "calcite", "coarse_dirt",
    "copper_ore", "cyan_terracotta", "deepslate", "deepslate_copper_ore", "deepslate_iron_ore", "diorite",
    "dirt", "granite", "grass_block", "gravel", "gray_terracotta", "green_terracotta", "iron_ore",
    "light_blue_terracotta", "light_gray_terracotta", "lime_terracotta", "magenta_terracotta", "moss_block",
    "mud", "muddy_mangrove_roots", "mycelium", "orange_terracotta", "packed_ice", "pale_moss_block",
    "pink_terracotta", "podzol", "powder_snow", "purple_terracotta", "raw_copper_block", "raw_iron_block",
    "red_sand", "red_sandstone", "red_terracotta", "rooted_dirt", "sand", "sandstone", "snow", "snow_block",
    "stone", "suspicious_gravel", "suspicious_sand", "terracotta", "tuff", "water", "white_terracotta",
    "yellow_terracotta",
};

const char* const kValues131[] = {
    "acacia_log", "birch_log", "cherry_log", "dark_oak_log", "jungle_log", "mangrove_log", "oak_log",
    "pale_oak_log", "spruce_log",
};

const char* const kValues132[] = {
    "pale_oak_log", "pale_oak_wood", "stripped_pale_oak_log", "stripped_pale_oak_wood",
};

const char* const kValues133[] = {
    "acacia_leaves", "acacia_log", "acacia_wood", "air", "azalea_leaves", "birch_leaves", "birch_log",
    "birch_wood", "cherry_leaves", "cherry_log", "cherry_wood", "crimson_hyphae", "crimson_stem",
    "dark_oak_leaves", "dark_oak_log", "dark_oak_wood", "flowering_azalea_leaves", "grass_block",
    "jungle_leaves", "jungle_log", "jungle_wood", "mangrove_leaves", "mangrove_log", "mangrove_wood",
    "oak_leaves", "oak_log", "oak_wood", "pale_oak_leaves", "pale_oak_log", "pale_oak_wood", "spruce_leaves",
    "spruce_log", "spruce_wood", "stripped_acacia_log", "stripped_acacia_wood", "stripped_birch_log",
    "stripped_birch_wood", "stripped_cherry_log", "stripped_cherry_wood", "stripped_crimson_hyphae",
    "stripped_crimson_stem", "stripped_dark_oak_log", "stripped_dark_oak_wood", "stripped_jungle_log",
    "stripped_jungle_wood", "stripped_mangrove_log", "stripped_mangrove_wood", "stripped_oak_log",
    "stripped_oak_wood", "stripped_pale_oak_log", "stripped_pale_oak_wood", "stripped_spruce_log",
    "stripped_spruce_wood", "stripped_warped_hyphae", "stripped_warped_stem", "warped_hyphae", "warped_stem",
};

const char* const kValues134[] = {
    "soul_campfire", "soul_fire", "soul_lantern", "soul_torch", "soul_wall_torch",
};

const char* const kValues135[] = {
    "acacia_planks", "bamboo_planks", "birch_planks", "cherry_planks", "crimson_planks", "dark_oak_planks",
    "jungle_planks", "mangrove_planks", "oak_planks", "pale_oak_planks", "spruce_planks", "warped_planks",
};

const char* const kValues136[] = {
    "ice",
};

const char* const kValues137[] = {
    "end_gateway", "end_portal", "nether_portal",
};

const char* const kValues138[] = {
    "acacia_pressure_plate", "bamboo_pressure_plate", "birch_pressure_plate", "cherry_pressure_plate",
    "crimson_pressure_plate", "dark_oak_pressure_plate", "heavy_weighted_pressure_plate",
    "jungle_pressure_plate", "light_weighted_pressure_plate", "mangrove_pressure_plate", "oak_pressure_plate",
    "pale_oak_pressure_plate", "polished_blackstone_pressure_plate", "spruce_pressure_plate",
    "stone_pressure_plate", "warped_pressure_plate",
};

const char* const kValues139[] = {
    "activator_rail", "detector_rail", "powered_rail", "rail",
};

const char* const kValues140[] = {
    "grass_block", "sand", "snow", "snow_block",
};

const char* const kValues141[] = {
    "activator_rail", "detector_rail", "powered_rail", "rail",
};

const char* const kValues142[] = {
    "deepslate_redstone_ore", "redstone_ore",
};

const char* const kValues143[] = {
    "air", "bubble_column", "bush", "cave_air", "crimson_roots", "dead_bush", "fern", "fire", "glow_lichen",
    "hanging_roots", "large_fern", "lava", "leaf_litter", "light", "nether_sprouts", "resin_clump", "seagrass",
    "short_dry_grass", "short_grass", "snow", "soul_fire", "structure_void", "tall_dry_grass", "tall_grass",
    "tall_seagrass", "vine", "void_air", "warped_roots", "water",
};

const char* const kValues144[] = {
    "acacia_leaves", "allium", "azalea_leaves", "azure_bluet", "birch_leaves", "blue_orchid", "brown_mushroom",
    "brown_mushroom_block", "bush", "cherry_leaves", "closed_eyeblossom", "cornflower", "crimson_roots",
    "dandelion", "dark_oak_leaves", "dead_bush", "fern", "firefly_bush", "flowering_azalea_leaves",
    "glow_lichen", "hanging_roots", "jungle_leaves", "large_fern", "leaf_litter", "lilac",
    "lily_of_the_valley", "mangrove_leaves", "nether_sprouts", "oak_leaves", "open_eyeblossom", "orange_tulip",
    "oxeye_daisy", "pale_moss_carpet", "pale_oak_leaves", "peony", "pink_tulip", "pitcher_plant", "poppy",
    "red_mushroom", "red_mushroom_block", "red_tulip", "rose_bush", "seagrass", "short_dry_grass",
    "short_grass", "spruce_leaves", "sunflower", "tall_dry_grass", "tall_grass", "tall_seagrass",
    "torchflower", "vine", "warped_roots", "water", "white_tulip", "wither_rose",
};

const char* const kValues145[] = {
    "acacia_leaves", "allium", "azalea_leaves", "azure_bluet", "birch_leaves", "blue_orchid", "bush",
    "cherry_leaves", "closed_eyeblossom", "cornflower", "crimson_roots", "dandelion", "dark_oak_leaves",
    "dead_bush", "fern", "firefly_bush", "flowering_azalea_leaves", "glow_lichen", "hanging_roots",
    "jungle_leaves", "large_fern", "leaf_litter", "lilac", "lily_of_the_valley", "mangrove_leaves",
    "nether_sprouts", "oak_leaves", "open_eyeblossom", "orange_tulip", "oxeye_daisy", "pale_moss_carpet",
    "pale_oak_leaves", "peony", "pink_tulip", "pitcher_plant", "poppy", "red_tulip", "rose_bush", "seagrass",
    "short_dry_grass", "short_grass", "spruce_leaves", "sunflower", "tall_dry_grass", "tall_grass",
    "tall_seagrass", "torchflower", "vine", "warped_roots", "water", "white_tulip", "wither_rose",
};

const char* const kValues146[] = {
    "red_sand", "sand", "suspicious_sand",
};

const char* const kValues147[] = {
    "acacia_sapling", "azalea", "birch_sapling", "cherry_sapling", "dark_oak_sapling", "flowering_azalea",
    "jungle_sapling", "mangrove_propagule", "oak_sapling", "pale_oak_sapling", "spruce_sapling",
};

const char* const kValues148[] = {
    "andesite", "basalt", "black_terracotta", "blackstone", "blue_terracotta", "brown_terracotta", "calcite",
    "clay", "coarse_dirt", "crimson_nylium", "cyan_terracotta", "deepslate", "diorite", "dirt",
    "dripstone_block", "end_stone", "granite", "grass_block", "gravel", "gray_terracotta", "green_terracotta",
    "light_blue_terracotta", "light_gray_terracotta", "lime_terracotta", "magenta_terracotta", "moss_block",
    "mud", "muddy_mangrove_roots", "mycelium", "netherrack", "orange_terracotta", "pale_moss_block",
    "pink_terracotta", "podzol", "purple_terracotta", "red_sand", "red_sandstone", "red_terracotta",
    "rooted_dirt", "sand", "sandstone", "smooth_basalt", "soul_sand", "soul_soil", "stone", "terracotta",
    "tuff", "warped_nylium", "white_terracotta", "yellow_terracotta",
};

const char* const kValues149[] = {
    "andesite", "basalt", "black_terracotta", "blackstone", "blue_terracotta", "brown_terracotta", "calcite",
    "clay", "coarse_dirt", "cobbled_deepslate", "cracked_deepslate_bricks", "cracked_deepslate_tiles",
    "crimson_nylium", "cyan_terracotta", "deepslate", "deepslate_bricks", "deepslate_tiles", "diorite", "dirt",
    "dripstone_block", "end_stone", "granite", "grass_block", "gravel", "gray_terracotta", "green_terracotta",
    "light_blue_terracotta", "light_gray_terracotta", "lime_terracotta", "magenta_terracotta", "moss_block",
    "mud", "muddy_mangrove_roots", "mycelium", "netherrack", "orange_terracotta", "pale_moss_block",
    "pink_terracotta", "podzol", "polished_deepslate", "purple_terracotta", "red_sand", "red_sandstone",
    "red_terracotta", "rooted_dirt", "sand", "sandstone", "smooth_basalt", "soul_sand", "soul_soil", "stone",
    "terracotta", "tuff", "warped_nylium", "white_terracotta", "yellow_terracotta",
};

const char* const kValues150[] = {
    "black_shulker_box", "blue_shulker_box", "brown_shulker_box", "cyan_shulker_box", "gray_shulker_box",
    "green_shulker_box", "light_blue_shulker_box", "light_gray_shulker_box", "lime_shulker_box",
    "magenta_shulker_box", "orange_shulker_box", "pink_shulker_box", "purple_shulker_box", "red_shulker_box",
    "shulker_box", "white_shulker_box", "yellow_shulker_box",
};

const char* const kValues151[] = {
    "acacia_sign", "acacia_wall_sign", "bamboo_sign", "bamboo_wall_sign", "birch_sign", "birch_wall_sign",
    "cherry_sign", "cherry_wall_sign", "crimson_sign", "crimson_wall_sign", "dark_oak_sign",
    "dark_oak_wall_sign", "jungle_sign", "jungle_wall_sign", "mangrove_sign", "mangrove_wall_sign", "oak_sign",
    "oak_wall_sign", "pale_oak_sign", "pale_oak_wall_sign", "spruce_sign", "spruce_wall_sign", "warped_sign",
    "warped_wall_sign",
};

const char* const kValues152[] = {
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

const char* const kValues153[] = {
    "clay", "moss_block",
};

const char* const kValues154[] = {
    "allium", "azure_bluet", "blue_orchid", "closed_eyeblossom", "cornflower", "dandelion",
    "lily_of_the_valley", "open_eyeblossom", "orange_tulip", "oxeye_daisy", "pink_tulip", "poppy", "red_tulip",
    "torchflower", "white_tulip", "wither_rose",
};

const char* const kValues155[] = {
    "red_sand", "sand",
};

const char* const kValues156[] = {
    "acacia_log", "birch_log", "cherry_log", "coal_ore", "copper_ore", "dark_oak_log", "emerald_ore",
    "iron_ore", "jungle_log", "mangrove_log", "oak_log", "packed_ice", "pale_oak_log", "spruce_log", "stone",
};

const char* const kValues157[] = {
    "coarse_dirt", "dirt", "grass_block", "moss_block", "mud", "muddy_mangrove_roots", "pale_moss_block",
    "podzol", "rooted_dirt",
};

const char* const kValues158[] = {
    "moss_block",
};

const char* const kValues159[] = {
    "powder_snow", "snow", "snow_block",
};

const char* const kValues160[] = {
    "honey_block", "mud", "soul_sand",
};

const char* const kValues161[] = {
    "barrier", "ice", "packed_ice",
};

const char* const kValues162[] = {
    "soul_sand", "soul_soil",
};

const char* const kValues163[] = {
    "soul_sand", "soul_soil",
};

const char* const kValues164[] = {
    "spruce_log", "spruce_wood", "stripped_spruce_log", "stripped_spruce_wood",
};

const char* const kValues165[] = {
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

const char* const kValues166[] = {
    "acacia_sign", "bamboo_sign", "birch_sign", "cherry_sign", "crimson_sign", "dark_oak_sign", "jungle_sign",
    "mangrove_sign", "oak_sign", "pale_oak_sign", "spruce_sign", "warped_sign",
};

const char* const kValues167[] = {
    "chiseled_stone_bricks", "cracked_stone_bricks", "mossy_stone_bricks", "stone_bricks",
};

const char* const kValues168[] = {
    "polished_blackstone_button", "stone_button",
};

const char* const kValues169[] = {
    "andesite", "diorite", "granite", "stone",
};

const char* const kValues170[] = {
    "polished_blackstone_pressure_plate", "stone_pressure_plate",
};

const char* const kValues171[] = {
    "lava",
};

const char* const kValues172[] = {
    "acacia_leaves", "azalea_leaves", "big_dripleaf", "big_dripleaf_stem", "birch_leaves", "carved_pumpkin",
    "cherry_leaves", "chorus_flower", "chorus_plant", "cocoa", "dark_oak_leaves", "flowering_azalea_leaves",
    "glow_lichen", "jack_o_lantern", "jungle_leaves", "mangrove_leaves", "melon", "oak_leaves",
    "pale_oak_leaves", "pumpkin", "spruce_leaves", "vine",
};

const char* const kValues173[] = {
    "bamboo", "bamboo_sapling",
};

const char* const kValues174[] = {
    "black_terracotta", "blue_terracotta", "brown_terracotta", "cyan_terracotta", "gray_terracotta",
    "green_terracotta", "light_blue_terracotta", "light_gray_terracotta", "lime_terracotta",
    "magenta_terracotta", "orange_terracotta", "pink_terracotta", "purple_terracotta", "red_terracotta",
    "terracotta", "white_terracotta", "yellow_terracotta",
};

const char* const kValues175[] = {
    "gravel",
};

const char* const kValues176[] = {
    "acacia_trapdoor", "bamboo_trapdoor", "birch_trapdoor", "cherry_trapdoor", "copper_trapdoor",
    "crimson_trapdoor", "dark_oak_trapdoor", "exposed_copper_trapdoor", "iron_trapdoor", "jungle_trapdoor",
    "mangrove_trapdoor", "oak_trapdoor", "oxidized_copper_trapdoor", "pale_oak_trapdoor", "spruce_trapdoor",
    "warped_trapdoor", "waxed_copper_trapdoor", "waxed_exposed_copper_trapdoor",
    "waxed_oxidized_copper_trapdoor", "waxed_weathered_copper_trapdoor", "weathered_copper_trapdoor",
};

const char* const kValues177[] = {
    "black_terracotta", "blue_terracotta", "brown_terracotta", "cyan_terracotta", "gray_terracotta",
    "green_terracotta", "light_blue_terracotta", "light_gray_terracotta", "lime_terracotta",
    "magenta_terracotta", "orange_terracotta", "pink_terracotta", "purple_terracotta", "red_sand",
    "red_terracotta", "sand", "terracotta", "white_terracotta", "yellow_terracotta",
};

const char* const kValues178[] = {
    "red_sand", "sand",
};

const char* const kValues179[] = {
    "soul_sand", "soul_soil",
};

const char* const kValues180[] = {
    "brain_coral", "brain_coral_fan", "brain_coral_wall_fan", "bubble_coral", "bubble_coral_fan",
    "bubble_coral_wall_fan", "fire_coral", "fire_coral_fan", "fire_coral_wall_fan", "horn_coral",
    "horn_coral_fan", "horn_coral_wall_fan", "seagrass", "tube_coral", "tube_coral_fan", "tube_coral_wall_fan",
};

const char* const kValues181[] = {
    "acacia_fence_gate", "bamboo_fence_gate", "birch_fence_gate", "cherry_fence_gate", "crimson_fence_gate",
    "dark_oak_fence_gate", "jungle_fence_gate", "mangrove_fence_gate", "oak_fence_gate", "pale_oak_fence_gate",
    "spruce_fence_gate", "warped_fence_gate",
};

const char* const kValues182[] = {
    "grass_block", "podzol",
};

const char* const kValues183[] = {
    "amethyst_block",
};

const char* const kValues184[] = {
    "brain_coral_wall_fan", "bubble_coral_wall_fan", "fire_coral_wall_fan", "horn_coral_wall_fan",
    "tube_coral_wall_fan",
};

const char* const kValues185[] = {
    "acacia_wall_hanging_sign", "bamboo_wall_hanging_sign", "birch_wall_hanging_sign",
    "cherry_wall_hanging_sign", "crimson_wall_hanging_sign", "dark_oak_wall_hanging_sign",
    "jungle_wall_hanging_sign", "mangrove_wall_hanging_sign", "oak_wall_hanging_sign",
    "pale_oak_wall_hanging_sign", "spruce_wall_hanging_sign", "warped_wall_hanging_sign",
};

const char* const kValues186[] = {
    "acacia_pressure_plate", "acacia_sign", "acacia_wall_sign", "bamboo_pressure_plate", "bamboo_sign",
    "bamboo_wall_sign", "birch_pressure_plate", "birch_sign", "birch_wall_sign", "black_banner",
    "black_wall_banner", "blue_banner", "blue_wall_banner", "brown_banner", "brown_wall_banner",
    "cactus_flower", "cherry_pressure_plate", "cherry_sign", "cherry_wall_sign", "copper_torch",
    "crimson_pressure_plate", "crimson_sign", "crimson_wall_sign", "cyan_banner", "cyan_wall_banner",
    "dark_oak_pressure_plate", "dark_oak_sign", "dark_oak_wall_sign", "gray_banner", "gray_wall_banner",
    "green_banner", "green_wall_banner", "heavy_weighted_pressure_plate", "jungle_pressure_plate",
    "jungle_sign", "jungle_wall_sign", "light_blue_banner", "light_blue_wall_banner", "light_gray_banner",
    "light_gray_wall_banner", "light_weighted_pressure_plate", "lime_banner", "lime_wall_banner",
    "magenta_banner", "magenta_wall_banner", "mangrove_pressure_plate", "mangrove_sign", "mangrove_wall_sign",
    "oak_pressure_plate", "oak_sign", "oak_wall_sign", "orange_banner", "orange_wall_banner",
    "pale_oak_pressure_plate", "pale_oak_sign", "pale_oak_wall_sign", "pink_banner", "pink_wall_banner",
    "polished_blackstone_pressure_plate", "purple_banner", "purple_wall_banner", "red_banner",
    "red_wall_banner", "redstone_torch", "soul_torch", "spruce_pressure_plate", "spruce_sign",
    "spruce_wall_sign", "stone_pressure_plate", "torch", "tripwire", "warped_pressure_plate", "warped_sign",
    "warped_wall_sign", "white_banner", "white_wall_banner", "yellow_banner", "yellow_wall_banner",
};

const char* const kValues187[] = {
    "acacia_wall_sign", "bamboo_wall_sign", "birch_wall_sign", "cherry_wall_sign", "crimson_wall_sign",
    "dark_oak_wall_sign", "jungle_wall_sign", "mangrove_wall_sign", "oak_wall_sign", "pale_oak_wall_sign",
    "spruce_wall_sign", "warped_wall_sign",
};

const char* const kValues188[] = {
    "andesite_wall", "blackstone_wall", "brick_wall", "cobbled_deepslate_wall", "cobblestone_wall",
    "deepslate_brick_wall", "deepslate_tile_wall", "diorite_wall", "end_stone_brick_wall", "granite_wall",
    "mossy_cobblestone_wall", "mossy_stone_brick_wall", "mud_brick_wall", "nether_brick_wall",
    "polished_blackstone_brick_wall", "polished_blackstone_wall", "polished_deepslate_wall",
    "polished_tuff_wall", "prismarine_wall", "red_nether_brick_wall", "red_sandstone_wall", "resin_brick_wall",
    "sandstone_wall", "stone_brick_wall", "tuff_brick_wall", "tuff_wall",
};

const char* const kValues189[] = {
    "stripped_warped_hyphae", "stripped_warped_stem", "warped_hyphae", "warped_stem",
};

const char* const kValues190[] = {
    "nether_wart_block", "warped_wart_block",
};

const char* const kValues191[] = {
    "barrier", "bedrock", "chain_command_block", "command_block", "end_gateway", "end_portal",
    "end_portal_frame", "jigsaw", "light", "moving_piston", "reinforced_deepslate", "repeating_command_block",
    "structure_block", "test_block", "test_instance_block",
};

const char* const kValues192[] = {
    "soul_sand", "soul_soil",
};

const char* const kValues193[] = {
    "coarse_dirt", "grass_block", "podzol", "snow", "snow_block",
};

const char* const kValues194[] = {
    "acacia_button", "bamboo_button", "birch_button", "cherry_button", "crimson_button", "dark_oak_button",
    "jungle_button", "mangrove_button", "oak_button", "pale_oak_button", "spruce_button", "warped_button",
};

const char* const kValues195[] = {
    "acacia_door", "bamboo_door", "birch_door", "cherry_door", "crimson_door", "dark_oak_door", "jungle_door",
    "mangrove_door", "oak_door", "pale_oak_door", "spruce_door", "warped_door",
};

const char* const kValues196[] = {
    "acacia_fence", "bamboo_fence", "birch_fence", "cherry_fence", "crimson_fence", "dark_oak_fence",
    "jungle_fence", "mangrove_fence", "oak_fence", "pale_oak_fence", "spruce_fence", "warped_fence",
};

const char* const kValues197[] = {
    "acacia_pressure_plate", "bamboo_pressure_plate", "birch_pressure_plate", "cherry_pressure_plate",
    "crimson_pressure_plate", "dark_oak_pressure_plate", "jungle_pressure_plate", "mangrove_pressure_plate",
    "oak_pressure_plate", "pale_oak_pressure_plate", "spruce_pressure_plate", "warped_pressure_plate",
};

const char* const kValues198[] = {
    "acacia_shelf", "bamboo_shelf", "birch_shelf", "cherry_shelf", "crimson_shelf", "dark_oak_shelf",
    "jungle_shelf", "mangrove_shelf", "oak_shelf", "pale_oak_shelf", "spruce_shelf", "warped_shelf",
};

const char* const kValues199[] = {
    "acacia_slab", "bamboo_slab", "birch_slab", "cherry_slab", "crimson_slab", "dark_oak_slab", "jungle_slab",
    "mangrove_slab", "oak_slab", "pale_oak_slab", "spruce_slab", "warped_slab",
};

const char* const kValues200[] = {
    "acacia_stairs", "bamboo_stairs", "birch_stairs", "cherry_stairs", "crimson_stairs", "dark_oak_stairs",
    "jungle_stairs", "mangrove_stairs", "oak_stairs", "pale_oak_stairs", "spruce_stairs", "warped_stairs",
};

const char* const kValues201[] = {
    "acacia_trapdoor", "bamboo_trapdoor", "birch_trapdoor", "cherry_trapdoor", "crimson_trapdoor",
    "dark_oak_trapdoor", "jungle_trapdoor", "mangrove_trapdoor", "oak_trapdoor", "pale_oak_trapdoor",
    "spruce_trapdoor", "warped_trapdoor",
};

const char* const kValues202[] = {
    "black_wool", "blue_wool", "brown_wool", "cyan_wool", "gray_wool", "green_wool", "light_blue_wool",
    "light_gray_wool", "lime_wool", "magenta_wool", "orange_wool", "pink_wool", "purple_wool", "red_wool",
    "white_wool", "yellow_wool",
};

const char* const kValues203[] = {
    "black_carpet", "blue_carpet", "brown_carpet", "cyan_carpet", "gray_carpet", "green_carpet",
    "light_blue_carpet", "light_gray_carpet", "lime_carpet", "magenta_carpet", "orange_carpet", "pink_carpet",
    "purple_carpet", "red_carpet", "white_carpet", "yellow_carpet",
};

} // namespace

const VanillaBlockTagEntry* vanillaBlockTagTable(size_t& count) noexcept
{
    static const VanillaBlockTagEntry kTags[] = {
        {"acacia_logs", 4, kValues0},
        {"air", 3, kValues1},
        {"all_hanging_signs", 24, kValues2},
        {"all_signs", 48, kValues3},
        {"ancient_city_replaceable", 12, kValues4},
        {"animals_spawnable_on", 1, kValues5},
        {"anvil", 3, kValues6},
        {"armadillo_spawnable_on", 10, kValues7},
        {"axolotls_spawnable_on", 1, kValues8},
        {"azalea_grows_on", 32, kValues9},
        {"azalea_root_replaceable", 39, kValues10},
        {"badlands_terracotta", 7, kValues11},
        {"bamboo_blocks", 2, kValues12},
        {"bamboo_plantable_on", 17, kValues13},
        {"banners", 32, kValues14},
        {"bars", 9, kValues15},
        {"base_stone_nether", 3, kValues16},
        {"base_stone_overworld", 6, kValues17},
        {"bats_spawnable_on", 6, kValues18},
        {"beacon_base_blocks", 5, kValues19},
        {"beds", 16, kValues20},
        {"bee_attractive", 29, kValues21},
        {"bee_growables", 11, kValues22},
        {"beehives", 2, kValues23},
        {"big_dripleaf_placeable", 11, kValues24},
        {"birch_logs", 4, kValues25},
        {"blocks_wind_charge_explosions", 2, kValues26},
        {"buttons", 14, kValues27},
        {"camel_sand_step_sound_blocks", 19, kValues28},
        {"camels_spawnable_on", 3, kValues29},
        {"campfires", 2, kValues30},
        {"can_glide_through", 7, kValues31},
        {"candle_cakes", 17, kValues32},
        {"candles", 17, kValues33},
        {"cauldrons", 4, kValues34},
        {"cave_vines", 2, kValues35},
        {"ceiling_hanging_signs", 12, kValues36},
        {"chains", 9, kValues37},
        {"cherry_logs", 4, kValues38},
        {"climbable", 9, kValues39},
        {"coal_ores", 2, kValues40},
        {"combination_step_sound_blocks", 23, kValues41},
        {"completes_find_tree_tutorial", 57, kValues42},
        {"concrete_powder", 16, kValues43},
        {"convertable_to_mud", 3, kValues44},
        {"copper", 8, kValues45},
        {"copper_chests", 8, kValues46},
        {"copper_golem_statues", 8, kValues47},
        {"copper_ores", 2, kValues48},
        {"coral_blocks", 5, kValues49},
        {"coral_plants", 5, kValues50},
        {"corals", 10, kValues51},
        {"crimson_stems", 4, kValues52},
        {"crops", 8, kValues53},
        {"crystal_sound_blocks", 2, kValues54},
        {"dampens_vibrations", 32, kValues55},
        {"dark_oak_logs", 4, kValues56},
        {"deepslate_ore_replaceables", 2, kValues57},
        {"diamond_ores", 2, kValues58},
        {"dirt", 10, kValues59},
        {"does_not_block_hoppers", 2, kValues60},
        {"doors", 21, kValues61},
        {"dragon_immune", 19, kValues62},
        {"dragon_transparent", 3, kValues63},
        {"dripstone_replaceable_blocks", 6, kValues64},
        {"dry_vegetation_may_place_on", 31, kValues65},
        {"edible_for_sheep", 4, kValues66},
        {"emerald_ores", 2, kValues67},
        {"enchantment_power_provider", 1, kValues68},
        {"enchantment_power_transmitter", 29, kValues69},
        {"enderman_holdable", 44, kValues70},
        {"fall_damage_resetting", 11, kValues71},
        {"features_cannot_replace", 7, kValues72},
        {"fence_gates", 12, kValues73},
        {"fences", 13, kValues74},
        {"fire", 2, kValues75},
        {"flower_pots", 38, kValues76},
        {"flowers", 30, kValues77},
        {"foxes_spawnable_on", 5, kValues78},
        {"frog_prefer_jump_to", 2, kValues79},
        {"frogs_spawnable_on", 4, kValues80},
        {"geode_invalid_blocks", 6, kValues81},
        {"goats_spawnable_on", 6, kValues82},
        {"gold_ores", 3, kValues83},
        {"guarded_by_piglins", 35, kValues84},
        {"happy_ghast_avoids", 6, kValues85},
        {"hoglin_repellents", 4, kValues86},
        {"ice", 4, kValues87},
        {"impermeable", 19, kValues88},
        {"incorrect_for_copper_tool", 17, kValues89},
        {"incorrect_for_diamond_tool", 0, kValues90},
        {"incorrect_for_gold_tool", 108, kValues91},
        {"incorrect_for_iron_tool", 5, kValues92},
        {"incorrect_for_netherite_tool", 0, kValues93},
        {"incorrect_for_stone_tool", 17, kValues94},
        {"incorrect_for_wooden_tool", 108, kValues95},
        {"infiniburn_end", 3, kValues96},
        {"infiniburn_nether", 2, kValues97},
        {"infiniburn_overworld", 2, kValues98},
        {"inside_step_sound_blocks", 8, kValues99},
        {"invalid_spawn_inside", 2, kValues100},
        {"iron_ores", 2, kValues101},
        {"jungle_logs", 4, kValues102},
        {"lanterns", 10, kValues103},
        {"lapis_ores", 2, kValues104},
        {"lava_pool_stone_cannot_replace", 62, kValues105},
        {"leaves", 11, kValues106},
        {"lightning_rods", 8, kValues107},
        {"logs", 44, kValues108},
        {"logs_that_burn", 36, kValues109},
        {"lush_ground_replaceable", 21, kValues110},
        {"maintains_farmland", 11, kValues111},
        {"mangrove_logs", 4, kValues112},
        {"mangrove_logs_can_grow_through", 8, kValues113},
        {"mangrove_roots_can_grow_through", 7, kValues114},
        {"mineable/axe", 286, kValues115},
        {"mineable/hoe", 29, kValues116},
        {"mineable/pickaxe", 482, kValues117},
        {"mineable/shovel", 36, kValues118},
        {"mob_interactable_doors", 20, kValues119},
        {"mooshrooms_spawnable_on", 1, kValues120},
        {"moss_replaceable", 18, kValues121},
        {"mushroom_grow_block", 4, kValues122},
        {"needs_diamond_tool", 5, kValues123},
        {"needs_iron_tool", 12, kValues124},
        {"needs_stone_tool", 91, kValues125},
        {"nether_carver_replaceables", 25, kValues126},
        {"nylium", 2, kValues127},
        {"oak_logs", 4, kValues128},
        {"occludes_vibration_signals", 16, kValues129},
        {"overworld_carver_replaceables", 52, kValues130},
        {"overworld_natural_logs", 9, kValues131},
        {"pale_oak_logs", 4, kValues132},
        {"parrots_spawnable_on", 57, kValues133},
        {"piglin_repellents", 5, kValues134},
        {"planks", 12, kValues135},
        {"polar_bears_spawnable_on_alternate", 1, kValues136},
        {"portals", 3, kValues137},
        {"pressure_plates", 16, kValues138},
        {"prevent_mob_spawning_inside", 4, kValues139},
        {"rabbits_spawnable_on", 4, kValues140},
        {"rails", 4, kValues141},
        {"redstone_ores", 2, kValues142},
        {"replaceable", 29, kValues143},
        {"replaceable_by_mushrooms", 56, kValues144},
        {"replaceable_by_trees", 52, kValues145},
        {"sand", 3, kValues146},
        {"saplings", 11, kValues147},
        {"sculk_replaceable", 50, kValues148},
        {"sculk_replaceable_world_gen", 56, kValues149},
        {"shulker_boxes", 17, kValues150},
        {"signs", 24, kValues151},
        {"slabs", 62, kValues152},
        {"small_dripleaf_placeable", 2, kValues153},
        {"small_flowers", 16, kValues154},
        {"smelts_to_glass", 2, kValues155},
        {"snaps_goat_horn", 15, kValues156},
        {"sniffer_diggable_block", 9, kValues157},
        {"sniffer_egg_hatch_boost", 1, kValues158},
        {"snow", 3, kValues159},
        {"snow_layer_can_survive_on", 3, kValues160},
        {"snow_layer_cannot_survive_on", 3, kValues161},
        {"soul_fire_base_blocks", 2, kValues162},
        {"soul_speed_blocks", 2, kValues163},
        {"spruce_logs", 4, kValues164},
        {"stairs", 58, kValues165},
        {"standing_signs", 12, kValues166},
        {"stone_bricks", 4, kValues167},
        {"stone_buttons", 2, kValues168},
        {"stone_ore_replaceables", 4, kValues169},
        {"stone_pressure_plates", 2, kValues170},
        {"strider_warm_blocks", 1, kValues171},
        {"sword_efficient", 22, kValues172},
        {"sword_instantly_mines", 2, kValues173},
        {"terracotta", 17, kValues174},
        {"trail_ruins_replaceable", 1, kValues175},
        {"trapdoors", 21, kValues176},
        {"triggers_ambient_desert_dry_vegetation_block_sounds", 19, kValues177},
        {"triggers_ambient_desert_sand_block_sounds", 2, kValues178},
        {"triggers_ambient_dried_ghast_block_sounds", 2, kValues179},
        {"underwater_bonemeals", 16, kValues180},
        {"unstable_bottom_center", 12, kValues181},
        {"valid_spawn", 2, kValues182},
        {"vibration_resonators", 1, kValues183},
        {"wall_corals", 5, kValues184},
        {"wall_hanging_signs", 12, kValues185},
        {"wall_post_override", 78, kValues186},
        {"wall_signs", 12, kValues187},
        {"walls", 26, kValues188},
        {"warped_stems", 4, kValues189},
        {"wart_blocks", 2, kValues190},
        {"wither_immune", 15, kValues191},
        {"wither_summon_base_blocks", 2, kValues192},
        {"wolves_spawnable_on", 5, kValues193},
        {"wooden_buttons", 12, kValues194},
        {"wooden_doors", 12, kValues195},
        {"wooden_fences", 12, kValues196},
        {"wooden_pressure_plates", 12, kValues197},
        {"wooden_shelves", 12, kValues198},
        {"wooden_slabs", 12, kValues199},
        {"wooden_stairs", 12, kValues200},
        {"wooden_trapdoors", 12, kValues201},
        {"wool", 16, kValues202},
        {"wool_carpets", 16, kValues203},
    };
    count = sizeof(kTags) / sizeof(kTags[0]);
    return kTags;
}

} // namespace mc
