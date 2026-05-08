#include "TextureMapper.hpp"
#include <algorithm>
#include <spdlog/spdlog.h>

namespace mc {
namespace resource {
namespace compat {

std::mutex TextureMapper::s_mutex;
TextureMapper* TextureMapper::s_instance = nullptr;

const TextureMapper& TextureMapper::instance() {
    std::lock_guard<std::mutex> lock(s_mutex);
    if (!s_instance) {
        s_instance = new TextureMapper();
    }
    return *s_instance;
}

TextureMapper::TextureMapper() {
    initializeMappings();
    spdlog::info("[TextureMapper] Initialized with {} texture name mappings",
                 m_modernToLegacy.size());
}

void TextureMapper::initializeMappings() {
    // =========================================================================
    // 原木纹理（顺序反转）
    // MC 1.12: log_oak, log_oak_top, log_spruce, ...
    // MC 1.13+: oak_log, oak_log_top, spruce_log, ...
    // =========================================================================
    static const std::vector<std::pair<std::string, std::string>> logMappings = {
        {"oak_log", "log_oak"},
        {"oak_log_top", "log_oak_top"},
        {"spruce_log", "log_spruce"},
        {"spruce_log_top", "log_spruce_top"},
        {"birch_log", "log_birch"},
        {"birch_log_top", "log_birch_top"},
        {"jungle_log", "log_jungle"},
        {"jungle_log_top", "log_jungle_top"},
        {"acacia_log", "log_acacia"},
        {"acacia_log_top", "log_acacia_top"},
        {"dark_oak_log", "log_big_oak"},
        {"dark_oak_log_top", "log_big_oak_top"},
    };

    for (const auto& [modern, legacy] : logMappings) {
        m_modernToLegacy[modern] = legacy;
        m_legacyToModern[legacy] = modern;
    }

    // =========================================================================
    // 树叶纹理（顺序反转）
    // MC 1.12: leaves_oak, leaves_spruce, ...
    // MC 1.13+: oak_leaves, spruce_leaves, ...
    // =========================================================================
    static const std::vector<std::pair<std::string, std::string>> leafMappings = {
        {"oak_leaves", "leaves_oak"},
        {"spruce_leaves", "leaves_spruce"},
        {"birch_leaves", "leaves_birch"},
        {"jungle_leaves", "leaves_jungle"},
        {"acacia_leaves", "leaves_acacia"},
        {"dark_oak_leaves", "leaves_big_oak"},
    };

    for (const auto& [modern, legacy] : leafMappings) {
        m_modernToLegacy[modern] = legacy;
        m_legacyToModern[legacy] = modern;
    }

    // =========================================================================
    // 树苗纹理
    // MC 1.12: sapling_oak, sapling_spruce, ...
    // MC 1.13+: oak_sapling, spruce_sapling, ...
    // =========================================================================
    static const std::vector<std::pair<std::string, std::string>> saplingMappings = {
        {"oak_sapling", "sapling_oak"},
        {"spruce_sapling", "sapling_spruce"},
        {"birch_sapling", "sapling_birch"},
        {"jungle_sapling", "sapling_jungle"},
        {"acacia_sapling", "sapling_acacia"},
        {"dark_oak_sapling", "sapling_roofed_oak"},
    };

    for (const auto& [modern, legacy] : saplingMappings) {
        m_modernToLegacy[modern] = legacy;
        m_legacyToModern[legacy] = modern;
    }

    // =========================================================================
    // 木板纹理（顺序反转）
    // MC 1.12: planks_oak, planks_spruce, ...
    // MC 1.13+: oak_planks, spruce_planks, ...
    // =========================================================================
    static const std::vector<std::pair<std::string, std::string>> planksMappings = {
        {"oak_planks", "planks_oak"},
        {"spruce_planks", "planks_spruce"},
        {"birch_planks", "planks_birch"},
        {"jungle_planks", "planks_jungle"},
        {"acacia_planks", "planks_acacia"},
        {"dark_oak_planks", "planks_big_oak"},
    };

    for (const auto& [modern, legacy] : planksMappings) {
        m_modernToLegacy[modern] = legacy;
        m_legacyToModern[legacy] = modern;
    }

    // =========================================================================
    // 羊毛纹理（顺序反转 + "colored" 后缀）
    // MC 1.12: wool_colored_white, wool_colored_orange, ...
    // MC 1.13+: white_wool, orange_wool, ...
    // 注意: silver -> light_gray
    // =========================================================================
    static const std::vector<std::pair<std::string, std::string>> woolMappings = {
        {"white_wool", "wool_colored_white"},
        {"orange_wool", "wool_colored_orange"},
        {"magenta_wool", "wool_colored_magenta"},
        {"light_blue_wool", "wool_colored_light_blue"},
        {"yellow_wool", "wool_colored_yellow"},
        {"lime_wool", "wool_colored_lime"},
        {"pink_wool", "wool_colored_pink"},
        {"gray_wool", "wool_colored_gray"},
        {"light_gray_wool", "wool_colored_silver"},  // silver -> light_gray
        {"cyan_wool", "wool_colored_cyan"},
        {"purple_wool", "wool_colored_purple"},
        {"blue_wool", "wool_colored_blue"},
        {"brown_wool", "wool_colored_brown"},
        {"green_wool", "wool_colored_green"},
        {"red_wool", "wool_colored_red"},
        {"black_wool", "wool_colored_black"},
    };

    for (const auto& [modern, legacy] : woolMappings) {
        m_modernToLegacy[modern] = legacy;
        m_legacyToModern[legacy] = modern;
    }

    // =========================================================================
    // 混凝土纹理（顺序反转）
    // MC 1.12: concrete_white, concrete_orange, ...
    // MC 1.13+: white_concrete, orange_concrete, ...
    // 注意: silver -> light_gray
    // =========================================================================
    static const std::vector<std::pair<std::string, std::string>> concreteMappings = {
        {"white_concrete", "concrete_white"},
        {"orange_concrete", "concrete_orange"},
        {"magenta_concrete", "concrete_magenta"},
        {"light_blue_concrete", "concrete_light_blue"},
        {"yellow_concrete", "concrete_yellow"},
        {"lime_concrete", "concrete_lime"},
        {"pink_concrete", "concrete_pink"},
        {"gray_concrete", "concrete_gray"},
        {"light_gray_concrete", "concrete_silver"},
        {"cyan_concrete", "concrete_cyan"},
        {"purple_concrete", "concrete_purple"},
        {"blue_concrete", "concrete_blue"},
        {"brown_concrete", "concrete_brown"},
        {"green_concrete", "concrete_green"},
        {"red_concrete", "concrete_red"},
        {"black_concrete", "concrete_black"},
    };

    for (const auto& [modern, legacy] : concreteMappings) {
        m_modernToLegacy[modern] = legacy;
        m_legacyToModern[legacy] = modern;
    }

    // =========================================================================
    // 混凝土粉末纹理
    // =========================================================================
    static const std::vector<std::pair<std::string, std::string>> concretePowderMappings = {
        {"white_concrete_powder", "concrete_powder_white"},
        {"orange_concrete_powder", "concrete_powder_orange"},
        {"magenta_concrete_powder", "concrete_powder_magenta"},
        {"light_blue_concrete_powder", "concrete_powder_light_blue"},
        {"yellow_concrete_powder", "concrete_powder_yellow"},
        {"lime_concrete_powder", "concrete_powder_lime"},
        {"pink_concrete_powder", "concrete_powder_pink"},
        {"gray_concrete_powder", "concrete_powder_gray"},
        {"light_gray_concrete_powder", "concrete_powder_silver"},
        {"cyan_concrete_powder", "concrete_powder_cyan"},
        {"purple_concrete_powder", "concrete_powder_purple"},
        {"blue_concrete_powder", "concrete_powder_blue"},
        {"brown_concrete_powder", "concrete_powder_brown"},
        {"green_concrete_powder", "concrete_powder_green"},
        {"red_concrete_powder", "concrete_powder_red"},
        {"black_concrete_powder", "concrete_powder_black"},
    };

    for (const auto& [modern, legacy] : concretePowderMappings) {
        m_modernToLegacy[modern] = legacy;
        m_legacyToModern[legacy] = modern;
    }

    // =========================================================================
    // 陶瓦纹理（stained -> color_terracotta）
    // MC 1.12: hardened_clay_stained_white, ...
    // MC 1.13+: white_terracotta, ...
    // =========================================================================
    static const std::vector<std::pair<std::string, std::string>> terracottaMappings = {
        {"white_terracotta", "hardened_clay_stained_white"},
        {"orange_terracotta", "hardened_clay_stained_orange"},
        {"magenta_terracotta", "hardened_clay_stained_magenta"},
        {"light_blue_terracotta", "hardened_clay_stained_light_blue"},
        {"yellow_terracotta", "hardened_clay_stained_yellow"},
        {"lime_terracotta", "hardened_clay_stained_lime"},
        {"pink_terracotta", "hardened_clay_stained_pink"},
        {"gray_terracotta", "hardened_clay_stained_gray"},
        {"light_gray_terracotta", "hardened_clay_stained_silver"},
        {"cyan_terracotta", "hardened_clay_stained_cyan"},
        {"purple_terracotta", "hardened_clay_stained_purple"},
        {"blue_terracotta", "hardened_clay_stained_blue"},
        {"brown_terracotta", "hardened_clay_stained_brown"},
        {"green_terracotta", "hardened_clay_stained_green"},
        {"red_terracotta", "hardened_clay_stained_red"},
        {"black_terracotta", "hardened_clay_stained_black"},
    };

    for (const auto& [modern, legacy] : terracottaMappings) {
        m_modernToLegacy[modern] = legacy;
        m_legacyToModern[legacy] = modern;
    }

    // =========================================================================
    // 釉面陶瓦纹理
    // =========================================================================
    static const std::vector<std::pair<std::string, std::string>> glazedTerracottaMappings = {
        {"white_glazed_terracotta", "glazed_terracotta_white"},
        {"orange_glazed_terracotta", "glazed_terracotta_orange"},
        {"magenta_glazed_terracotta", "glazed_terracotta_magenta"},
        {"light_blue_glazed_terracotta", "glazed_terracotta_light_blue"},
        {"yellow_glazed_terracotta", "glazed_terracotta_yellow"},
        {"lime_glazed_terracotta", "glazed_terracotta_lime"},
        {"pink_glazed_terracotta", "glazed_terracotta_pink"},
        {"gray_glazed_terracotta", "glazed_terracotta_gray"},
        {"light_gray_glazed_terracotta", "glazed_terracotta_silver"},
        {"cyan_glazed_terracotta", "glazed_terracotta_cyan"},
        {"purple_glazed_terracotta", "glazed_terracotta_purple"},
        {"blue_glazed_terracotta", "glazed_terracotta_blue"},
        {"brown_glazed_terracotta", "glazed_terracotta_brown"},
        {"green_glazed_terracotta", "glazed_terracotta_green"},
        {"red_glazed_terracotta", "glazed_terracotta_red"},
        {"black_glazed_terracotta", "glazed_terracotta_black"},
    };

    for (const auto& [modern, legacy] : glazedTerracottaMappings) {
        m_modernToLegacy[modern] = legacy;
        m_legacyToModern[legacy] = modern;
    }

    // =========================================================================
    // 石头变种（stone_ 前缀 vs 无前缀）
    // MC 1.12: stone_granite, stone_diorite, stone_andesite
    // MC 1.13+: granite, diorite, andesite
    // =========================================================================
    static const std::vector<std::pair<std::string, std::string>> stoneMappings = {
        {"granite", "stone_granite"},
        {"polished_granite", "stone_granite_smooth"},
        {"diorite", "stone_diorite"},
        {"polished_diorite", "stone_diorite_smooth"},
        {"andesite", "stone_andesite"},
        {"polished_andesite", "stone_andesite_smooth"},
    };

    for (const auto& [modern, legacy] : stoneMappings) {
        m_modernToLegacy[modern] = legacy;
        m_legacyToModern[legacy] = modern;
    }

    // =========================================================================
    // 草方块
    // MC 1.12: grass_top, grass_side, grass_side_overlay, grass_side_snowed
    // MC 1.13+: grass_block_top, grass_block_side, grass_block_side_overlay, grass_block_snow
    // =========================================================================
    static const std::vector<std::pair<std::string, std::string>> grassMappings = {
        {"grass_block_top", "grass_top"},
        {"grass_block_side", "grass_side"},
        {"grass_block_side_overlay", "grass_side_overlay"},
        {"grass_block_snow", "grass_side_snowed"},
    };

    for (const auto& [modern, legacy] : grassMappings) {
        m_modernToLegacy[modern] = legacy;
        m_legacyToModern[legacy] = modern;
    }

    // =========================================================================
    // 灰化土
    // =========================================================================
    static const std::vector<std::pair<std::string, std::string>> podzolMappings = {
        {"podzol_top", "dirt_podzol_top"},
        {"podzol_side", "dirt_podzol_side"},
    };

    for (const auto& [modern, legacy] : podzolMappings) {
        m_modernToLegacy[modern] = legacy;
        m_legacyToModern[legacy] = modern;
    }

    // =========================================================================
    // 砂岩
    // =========================================================================
    static const std::vector<std::pair<std::string, std::string>> sandstoneMappings = {
        {"sandstone_top", "sandstone_top"},
        {"sandstone_bottom", "sandstone_bottom"},
        {"sandstone_side", "sandstone_normal"},
        {"cut_sandstone", "sandstone_carved"},
        {"chiseled_sandstone", "sandstone_smooth"},
        {"red_sandstone_top", "red_sandstone_top"},
        {"red_sandstone_bottom", "red_sandstone_bottom"},
        {"red_sandstone_side", "red_sandstone_normal"},
        {"cut_red_sandstone", "red_sandstone_carved"},
        {"chiseled_red_sandstone", "red_sandstone_smooth"},
    };

    for (const auto& [modern, legacy] : sandstoneMappings) {
        m_modernToLegacy[modern] = legacy;
        m_legacyToModern[legacy] = modern;
    }

    // =========================================================================
    // 花
    // MC 1.12: flower_dandelion, flower_rose, flower_blue_orchid, ...
    // MC 1.13+: dandelion, poppy, blue_orchid, ...
    // 注意: rose -> poppy, houstonia -> azure_bluet
    // =========================================================================
    static const std::vector<std::pair<std::string, std::string>> flowerMappings = {
        {"dandelion", "flower_dandelion"},
        {"poppy", "flower_rose"},  // rose -> poppy
        {"blue_orchid", "flower_blue_orchid"},
        {"allium", "flower_allium"},
        {"azure_bluet", "flower_houstonia"},  // houstonia -> azure_bluet
        {"red_tulip", "flower_tulip_red"},
        {"orange_tulip", "flower_tulip_orange"},
        {"white_tulip", "flower_tulip_white"},
        {"pink_tulip", "flower_tulip_pink"},
        {"oxeye_daisy", "flower_oxeye_daisy"},
    };

    for (const auto& [modern, legacy] : flowerMappings) {
        m_modernToLegacy[modern] = legacy;
        m_legacyToModern[legacy] = modern;
    }

    // =========================================================================
    // 高草
    // =========================================================================
    static const std::vector<std::pair<std::string, std::string>> tallGrassMappings = {
        {"short_grass", "tallgrass"},
        {"tall_grass_top", "double_plant_grass_top"},
        {"tall_grass_bottom", "double_plant_grass_bottom"},
        {"fern", "fern"},  // fern 保持不变
        {"large_fern_top", "double_plant_fern_top"},
        {"large_fern_bottom", "double_plant_fern_bottom"},
    };

    for (const auto& [modern, legacy] : tallGrassMappings) {
        m_modernToLegacy[modern] = legacy;
        m_legacyToModern[legacy] = modern;
    }

    // =========================================================================
    // 门
    // =========================================================================
    static const std::vector<std::pair<std::string, std::string>> doorMappings = {
        {"oak_door_bottom", "door_wood_lower"},
        {"oak_door_top", "door_wood_upper"},
        {"spruce_door_bottom", "door_spruce_lower"},
        {"spruce_door_top", "door_spruce_upper"},
        {"birch_door_bottom", "door_birch_lower"},
        {"birch_door_top", "door_birch_upper"},
        {"jungle_door_bottom", "door_jungle_lower"},
        {"jungle_door_top", "door_jungle_upper"},
        {"acacia_door_bottom", "door_acacia_lower"},
        {"acacia_door_top", "door_acacia_upper"},
        {"dark_oak_door_bottom", "door_dark_oak_lower"},
        {"dark_oak_door_top", "door_dark_oak_upper"},
        {"iron_door_bottom", "door_iron_lower"},
        {"iron_door_top", "door_iron_upper"},
    };

    for (const auto& [modern, legacy] : doorMappings) {
        m_modernToLegacy[modern] = legacy;
        m_legacyToModern[legacy] = modern;
    }

    // =========================================================================
    // 床
    // =========================================================================
    static const std::vector<std::pair<std::string, std::string>> bedMappings = {
        {"white_bed", "bed_white"},
        {"orange_bed", "bed_orange"},
        {"magenta_bed", "bed_magenta"},
        {"light_blue_bed", "bed_light_blue"},
        {"yellow_bed", "bed_yellow"},
        {"lime_bed", "bed_lime"},
        {"pink_bed", "bed_pink"},
        {"gray_bed", "bed_gray"},
        {"light_gray_bed", "bed_silver"},
        {"cyan_bed", "bed_cyan"},
        {"purple_bed", "bed_purple"},
        {"blue_bed", "bed_blue"},
        {"brown_bed", "bed_brown"},
        {"green_bed", "bed_green"},
        {"red_bed", "bed_red"},
        {"black_bed", "bed_black"},
    };

    for (const auto& [modern, legacy] : bedMappings) {
        m_modernToLegacy[modern] = legacy;
        m_legacyToModern[legacy] = modern;
    }

    // =========================================================================
    // 潜影盒
    // =========================================================================
    static const std::vector<std::pair<std::string, std::string>> shulkerBoxMappings = {
        {"white_shulker_box", "shulker_top_white"},
        {"orange_shulker_box", "shulker_top_orange"},
        {"magenta_shulker_box", "shulker_top_magenta"},
        {"light_blue_shulker_box", "shulker_top_light_blue"},
        {"yellow_shulker_box", "shulker_top_yellow"},
        {"lime_shulker_box", "shulker_top_lime"},
        {"pink_shulker_box", "shulker_top_pink"},
        {"gray_shulker_box", "shulker_top_gray"},
        {"light_gray_shulker_box", "shulker_top_silver"},
        {"cyan_shulker_box", "shulker_top_cyan"},
        {"purple_shulker_box", "shulker_top_purple"},
        {"blue_shulker_box", "shulker_top_blue"},
        {"brown_shulker_box", "shulker_top_brown"},
        {"green_shulker_box", "shulker_top_green"},
        {"red_shulker_box", "shulker_top_red"},
        {"black_shulker_box", "shulker_top_black"},
    };

    for (const auto& [modern, legacy] : shulkerBoxMappings) {
        m_modernToLegacy[modern] = legacy;
        m_legacyToModern[legacy] = modern;
    }

    // =========================================================================
    // 杂项方块
    // =========================================================================
    static const std::vector<std::pair<std::string, std::string>> miscMappings = {
        {"bricks", "brick"},
        {"wet_sponge", "sponge_wet"},
        {"crafting_table_top", "crafting_table_top"},
        {"crafting_table_side", "crafting_table_side"},
        {"crafting_table_front", "crafting_table_front"},
        {"furnace_front", "furnace_front_off"},
        {"furnace_front_on", "furnace_front_on"},
        {"furnace_side", "furnace_side"},
        {"furnace_top", "furnace_top"},
        {"pumpkin_top", "pumpkin_top"},
        {"pumpkin_side", "pumpkin_side"},
        {"pumpkin_face", "pumpkin_face_off"},
        {"jack_o_lantern", "pumpkin_face_on"},
        {"melon_top", "melon_top"},
        {"melon_side", "melon_side"},
        {"farmland_moist", "farmland_wet"},
        {"farmland_dry", "farmland_dry"},
        {"mossy_cobblestone", "cobblestone_mossy"},
    };

    for (const auto& [modern, legacy] : miscMappings) {
        m_modernToLegacy[modern] = legacy;
        m_legacyToModern[legacy] = modern;
    }

    // =========================================================================
    // 刷怪笼
    // =========================================================================
    // spawner -> mob_spawner (注意: 纹理保持不变)

    // =========================================================================
    // 菌丝
    // =========================================================================
    // mycelium_top -> mycelium_top (相同)
    // mycelium_side -> mycelium_side (相同)

    // =========================================================================
    // 石砖
    // =========================================================================
    static const std::vector<std::pair<std::string, std::string>> stoneBrickMappings = {
        {"stone_bricks", "stonebrick"},
        {"mossy_stone_bricks", "stonebrick_mossy"},
        {"cracked_stone_bricks", "stonebrick_cracked"},
        {"chiseled_stone_bricks", "stonebrick_carved"},
    };

    for (const auto& [modern, legacy] : stoneBrickMappings) {
        m_modernToLegacy[modern] = legacy;
        m_legacyToModern[legacy] = modern;
    }

    // =========================================================================
    // 下界砖
    // =========================================================================
    static const std::vector<std::pair<std::string, std::string>> netherMappings = {
        {"red_nether_bricks", "red_nether_brick"},
    };

    for (const auto& [modern, legacy] : netherMappings) {
        m_modernToLegacy[modern] = legacy;
        m_legacyToModern[legacy] = modern;
    }

    // =========================================================================
    // 海晶石
    // =========================================================================
    static const std::vector<std::pair<std::string, std::string>> prismarineMappings = {
        {"prismarine", "prismarine_rough"},
        {"prismarine_bricks", "prismarine_bricks"},
        {"dark_prismarine", "prismarine_dark"},
    };

    for (const auto& [modern, legacy] : prismarineMappings) {
        m_modernToLegacy[modern] = legacy;
        m_legacyToModern[legacy] = modern;
    }

    // =========================================================================
    // 紫珀块
    // =========================================================================
    static const std::vector<std::pair<std::string, std::string>> purpurMappings = {
        {"purpur_block", "purpur_block"},
        {"purpur_pillar", "purpur_pillar"},
        {"purpur_pillar_top", "purpur_pillar_top"},
    };

    for (const auto& [modern, legacy] : purpurMappings) {
        m_modernToLegacy[modern] = legacy;
        m_legacyToModern[legacy] = modern;
    }

    // =========================================================================
    // 末地石
    // =========================================================================
    static const std::vector<std::pair<std::string, std::string>> endMappings = {
        {"end_stone", "end_stone"},
        {"end_stone_bricks", "end_bricks"},
    };

    for (const auto& [modern, legacy] : endMappings) {
        m_modernToLegacy[modern] = legacy;
        m_legacyToModern[legacy] = modern;
    }

    // =========================================================================
    // 蘑菇
    // =========================================================================
    static const std::vector<std::pair<std::string, std::string>> mushroomMappings = {
        {"brown_mushroom_block", "mushroom_block_skin_brown"},
        {"red_mushroom_block", "mushroom_block_skin_red"},
        {"mushroom_stem", "mushroom_block_skin_stem"},
        {"brown_mushroom", "mushroom_brown"},
        {"red_mushroom", "mushroom_red"},
    };

    for (const auto& [modern, legacy] : mushroomMappings) {
        m_modernToLegacy[modern] = legacy;
        m_legacyToModern[legacy] = modern;
    }

    // =========================================================================
    // 仙人掌
    // =========================================================================
    // cactus_top, cactus_side, cactus_bottom 保持不变

    // =========================================================================
    // 甘蔗
    // =========================================================================
    static const std::vector<std::pair<std::string, std::string>> reedMappings = {
        {"sugar_cane", "reeds"},
    };

    for (const auto& [modern, legacy] : reedMappings) {
        m_modernToLegacy[modern] = legacy;
        m_legacyToModern[legacy] = modern;
    }

    // =========================================================================
    // 地狱疣
    // =========================================================================
    // nether_wart_stage_0, stage_1, stage_2 保持不变

    // =========================================================================
    // 紫颂植物
    // =========================================================================
    // chorus_flower, chorus_plant 保持不变

    // =========================================================================
    // 甜菜根
    // =========================================================================
    // beetroots_stage_0, stage_1, stage_2, stage_3 保持不变

    // =========================================================================
    // 胡萝卜
    // =========================================================================
    // carrots_stage_0, stage_1, stage_2, stage_3 保持不变

    // =========================================================================
    // 马铃薯
    // =========================================================================
    // potatoes_stage_0, stage_1, stage_2, stage_3 保持不变

    // =========================================================================
    // 小麦
    // =========================================================================
    // wheat_stage_0 到 stage_7 保持不变

    // =========================================================================
    // 南瓜/西瓜茎
    // =========================================================================
    static const std::vector<std::pair<std::string, std::string>> stemMappings = {
        {"pumpkin_stem", "pumpkin_stem_connected"},
        {"attached_pumpkin_stem", "pumpkin_stem_disconnected"},
        {"melon_stem", "melon_stem_connected"},
        {"attached_melon_stem", "melon_stem_disconnected"},
    };

    for (const auto& [modern, legacy] : stemMappings) {
        m_modernToLegacy[modern] = legacy;
        m_legacyToModern[legacy] = modern;
    }

    // =========================================================================
    // 可可豆
    // =========================================================================
    // cocoa_stage_0, stage_1, stage_2 保持不变

    // =========================================================================
    // 藤蔓
    // =========================================================================
    // vine, vine_carried 保持不变

    // =========================================================================
    // 睡莲
    // =========================================================================
    static const std::vector<std::pair<std::string, std::string>> lilyPadMappings = {
        {"lily_pad", "waterlily"},
    };

    for (const auto& [modern, legacy] : lilyPadMappings) {
        m_modernToLegacy[modern] = legacy;
        m_legacyToModern[legacy] = modern;
    }

    // =========================================================================
    // 龙蛋
    // =========================================================================
    // dragon_egg 保持不变

    // =========================================================================
    // 命令方块
    // =========================================================================
    static const std::vector<std::pair<std::string, std::string>> commandBlockMappings = {
        {"command_block_front", "command_block_front"},
        {"command_block_back", "command_block_back"},
        {"command_block_side", "command_block_side"},
        {"chain_command_block_front", "chain_command_block_front"},
        {"chain_command_block_back", "chain_command_block_back"},
        {"chain_command_block_side", "chain_command_block_side"},
        {"repeating_command_block_front", "repeating_command_block_front"},
        {"repeating_command_block_back", "repeating_command_block_back"},
        {"repeating_command_block_side", "repeating_command_block_side"},
    };

    for (const auto& [modern, legacy] : commandBlockMappings) {
        m_modernToLegacy[modern] = legacy;
        m_legacyToModern[legacy] = modern;
    }

    // =========================================================================
    // 结构方块
    // =========================================================================
    // structure_block, structure_block_corner, structure_block_data 等保持不变

    // =========================================================================
    // 骨块
    // =========================================================================
    // bone_block_side, bone_block_top 保持不变

    // =========================================================================
    // 侦测器
    // =========================================================================
    static const std::vector<std::pair<std::string, std::string>> observerMappings = {
        {"observer_front", "observer_front"},
        {"observer_back", "observer_back"},
        {"observer_back_on", "observer_back_lit"},
        {"observer_side", "observer_side"},
        {"observer_top", "observer_top"},
    };

    for (const auto& [modern, legacy] : observerMappings) {
        m_modernToLegacy[modern] = legacy;
        m_legacyToModern[legacy] = modern;
    }

    // =========================================================================
    // 潜影
    // =========================================================================
    // shulker_top_white 等在上面的颜色映射中已处理

    // =========================================================================
    // 注意: 可根据需要在此添加更多映射
    // MC Java LegacyResourcePackWrapper 中完整的列表包含 854+ 个映射
    // =========================================================================
}

std::string TextureMapper::getLegacyName(std::string_view modernName) const {
    auto it = m_modernToLegacy.find(std::string(modernName));
    return it != m_modernToLegacy.end() ? it->second : std::string();
}

std::string TextureMapper::getModernName(std::string_view legacyName) const {
    auto it = m_legacyToModern.find(std::string(legacyName));
    return it != m_legacyToModern.end() ? it->second : std::string();
}

std::vector<std::string> TextureMapper::getNameVariants(std::string_view name) const {
    std::vector<std::string> variants;
    std::string nameStr(name);
    variants.push_back(nameStr);  // 始终包含原名称

    // 检查是否是具有旧版映射的现代名称
    std::string legacy = getLegacyName(name);
    if (!legacy.empty() && legacy != nameStr) {
        variants.push_back(legacy);
    }

    // 检查是否是具有现代映射的旧版名称
    std::string modern = getModernName(name);
    if (!modern.empty() && modern != nameStr) {
        variants.push_back(modern);
    }

    return variants;
}

bool TextureMapper::hasMapping(std::string_view name) const {
    std::string nameStr(name);
    return m_modernToLegacy.count(nameStr) > 0 || m_legacyToModern.count(nameStr) > 0;
}

std::string TextureMapper::toModernPath(std::string_view legacyPath) const {
    std::string path(legacyPath);

    // 将 textures/blocks/ 替换为 textures/block/
    const std::string oldPrefix = "textures/blocks/";
    const std::string newPrefix = "textures/block/";
    if (path.find(oldPrefix) == 0) {
        path = newPrefix + path.substr(oldPrefix.length());
    }

    // 将 textures/items/ 替换为 textures/item/
    const std::string oldItemPrefix = "textures/items/";
    const std::string newItemPrefix = "textures/item/";
    if (path.find(oldItemPrefix) == 0) {
        path = newItemPrefix + path.substr(oldItemPrefix.length());
    }

    // 提取名称并转换
    size_t lastSlash = path.find_last_of("/\\");
    size_t dotPos = path.find_last_of('.');
    if (lastSlash != std::string::npos && dotPos != std::string::npos && dotPos > lastSlash) {
        std::string dirPath = path.substr(0, lastSlash + 1);
        std::string baseName = path.substr(lastSlash + 1, dotPos - lastSlash - 1);
        std::string ext = path.substr(dotPos);

        std::string modernName = getModernName(baseName);
        if (!modernName.empty()) {
            path = dirPath + modernName + ext;
        }
    }

    return path;
}

std::string TextureMapper::toLegacyPath(std::string_view modernPath) const {
    std::string path(modernPath);

    // 将 textures/block/ 替换为 textures/blocks/
    const std::string newPrefix = "textures/block/";
    const std::string oldPrefix = "textures/blocks/";
    if (path.find(newPrefix) == 0) {
        path = oldPrefix + path.substr(newPrefix.length());
    }

    // 将 textures/item/ 替换为 textures/items/
    const std::string newItemPrefix = "textures/item/";
    const std::string oldItemPrefix = "textures/items/";
    if (path.find(newItemPrefix) == 0) {
        path = oldItemPrefix + path.substr(newItemPrefix.length());
    }

    // 提取名称并转换
    size_t lastSlash = path.find_last_of("/\\");
    size_t dotPos = path.find_last_of('.');
    if (lastSlash != std::string::npos && dotPos != std::string::npos && dotPos > lastSlash) {
        std::string dirPath = path.substr(0, lastSlash + 1);
        std::string baseName = path.substr(lastSlash + 1, dotPos - lastSlash - 1);
        std::string ext = path.substr(dotPos);

        std::string legacyName = getLegacyName(baseName);
        if (!legacyName.empty()) {
            path = dirPath + legacyName + ext;
        }
    }

    return path;
}

std::vector<std::string> TextureMapper::getPathVariants(std::string_view path) const {
    std::vector<std::string> variants;
    std::string pathStr(path);

    // 始终包含原始路径
    variants.push_back(pathStr);

    // 判断路径是否有扩展名
    size_t lastSlash = pathStr.find_last_of("/\\");
    size_t dotPos = pathStr.find_last_of('.');
    bool hasExtension = (dotPos != std::string::npos && lastSlash != std::string::npos && dotPos > lastSlash);

    std::string dirPath, baseName, ext;
    if (lastSlash != std::string::npos) {
        dirPath = pathStr.substr(0, lastSlash + 1);
        if (hasExtension) {
            baseName = pathStr.substr(lastSlash + 1, dotPos - lastSlash - 1);
            ext = pathStr.substr(dotPos);
        } else {
            baseName = pathStr.substr(lastSlash + 1);
            ext = "";
        }
    } else {
        dirPath = "";
        if (hasExtension) {
            baseName = pathStr.substr(0, dotPos);
            ext = pathStr.substr(dotPos);
        } else {
            baseName = pathStr;
            ext = "";
        }
    }

    // 获取名称变体
    auto nameVariants = getNameVariants(baseName);

    // 确定目录路径变体 (block vs blocks, item vs items)
    std::string modernDir = dirPath;
    std::string legacyDir = dirPath;

    if (dirPath.find("textures/block/") == 0) {
        legacyDir = "textures/blocks/" + dirPath.substr(15);
    } else if (dirPath.find("textures/blocks/") == 0) {
        modernDir = "textures/block/" + dirPath.substr(16);
    } else if (dirPath.find("textures/item/") == 0) {
        legacyDir = "textures/items/" + dirPath.substr(14);
    } else if (dirPath.find("textures/items/") == 0) {
        modernDir = "textures/item/" + dirPath.substr(15);
    }

    // 生成所有组合
    // 对于每个名称变体，尝试每个目录变体
    for (const auto& name : nameVariants) {
        // 使用原始目录
        std::string variant = dirPath + name + ext;
        if (std::find(variants.begin(), variants.end(), variant) == variants.end()) {
            variants.push_back(variant);
        }

        // 使用现代目录（如果不同）
        if (modernDir != dirPath) {
            variant = modernDir + name + ext;
            if (std::find(variants.begin(), variants.end(), variant) == variants.end()) {
                variants.push_back(variant);
            }
        }

        // 使用旧版目录（如果不同）
        if (legacyDir != dirPath) {
            variant = legacyDir + name + ext;
            if (std::find(variants.begin(), variants.end(), variant) == variants.end()) {
                variants.push_back(variant);
            }
        }
    }

    return variants;
}

} // namespace compat
} // namespace resource
} // namespace mc
