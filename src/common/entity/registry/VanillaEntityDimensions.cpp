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

// 本文件由 scripts/baking/gen_entity_dimensions.py 生成，请勿手改。
// 数据来源：原版 1.21.11 服务端 jar 的 EntityType#getDimensions()（见
// tests/diff/java/dev/horizonlab/diff/EntityDimensionsExport.java）。

#include "common/entity/core/EntitySize.hpp"

#include <charconv>
#include <string_view>
#include <unordered_map>

namespace mc::entity {

namespace {

/// 一个实体类型的碰撞箱尺寸。
struct VanillaSize {
    f32 width;
    f32 height;
    f32 eyeHeight;
    bool fixed;
};

/// 实体名 → 原版碰撞箱尺寸。
const std::unordered_map<std::string_view, VanillaSize>& sizeTable()
{
    static const std::unordered_map<std::string_view, VanillaSize> table = {

        {"minecraft:acacia_boat", {1.375f, 0.5625f, 0.5625f, false}},
        {"minecraft:acacia_chest_boat", {1.375f, 0.5625f, 0.5625f, false}},
        {"minecraft:allay", {0.35f, 0.6f, 0.36f, false}},
        {"minecraft:area_effect_cloud", {6.0f, 0.5f, 0.425f, false}},
        {"minecraft:armadillo", {0.7f, 0.65f, 0.26f, false}},
        {"minecraft:armor_stand", {0.5f, 1.975f, 1.7775f, false}},
        {"minecraft:arrow", {0.5f, 0.5f, 0.13f, false}},
        {"minecraft:axolotl", {0.75f, 0.42f, 0.2751f, false}},
        {"minecraft:bamboo_chest_raft", {1.375f, 0.5625f, 0.5625f, false}},
        {"minecraft:bamboo_raft", {1.375f, 0.5625f, 0.5625f, false}},
        {"minecraft:bat", {0.5f, 0.9f, 0.45f, false}},
        {"minecraft:bee", {0.7f, 0.6f, 0.3f, false}},
        {"minecraft:birch_boat", {1.375f, 0.5625f, 0.5625f, false}},
        {"minecraft:birch_chest_boat", {1.375f, 0.5625f, 0.5625f, false}},
        {"minecraft:blaze", {0.6f, 1.8f, 1.53f, false}},
        {"minecraft:block_display", {0.0f, 0.0f, 0.0f, false}},
        {"minecraft:bogged", {0.6f, 1.99f, 1.74f, false}},
        {"minecraft:breeze", {0.6f, 1.77f, 1.3452f, false}},
        {"minecraft:breeze_wind_charge", {0.3125f, 0.3125f, 0.0f, false}},
        {"minecraft:camel", {1.7f, 2.375f, 2.275f, false}},
        {"minecraft:camel_husk", {1.7f, 2.375f, 2.275f, false}},
        {"minecraft:cat", {0.6f, 0.7f, 0.35f, false}},
        {"minecraft:cave_spider", {0.7f, 0.5f, 0.45f, false}},
        {"minecraft:cherry_boat", {1.375f, 0.5625f, 0.5625f, false}},
        {"minecraft:cherry_chest_boat", {1.375f, 0.5625f, 0.5625f, false}},
        {"minecraft:chest_minecart", {0.98f, 0.7f, 0.595f, false}},
        {"minecraft:chicken", {0.4f, 0.7f, 0.644f, false}},
        {"minecraft:cod", {0.5f, 0.3f, 0.195f, false}},
        {"minecraft:command_block_minecart", {0.98f, 0.7f, 0.595f, false}},
        {"minecraft:copper_golem", {0.49f, 0.98f, 0.8125f, false}},
        {"minecraft:cow", {0.9f, 1.4f, 1.3f, false}},
        {"minecraft:creaking", {0.9f, 2.7f, 2.3f, false}},
        {"minecraft:creeper", {0.6f, 1.7f, 1.445f, false}},
        {"minecraft:dark_oak_boat", {1.375f, 0.5625f, 0.5625f, false}},
        {"minecraft:dark_oak_chest_boat", {1.375f, 0.5625f, 0.5625f, false}},
        {"minecraft:dolphin", {0.9f, 0.6f, 0.3f, false}},
        {"minecraft:donkey", {1.3964844f, 1.5f, 1.425f, false}},
        {"minecraft:dragon_fireball", {1.0f, 1.0f, 0.85f, false}},
        {"minecraft:drowned", {0.6f, 1.95f, 1.74f, false}},
        {"minecraft:egg", {0.25f, 0.25f, 0.2125f, false}},
        {"minecraft:elder_guardian", {1.9975f, 1.9975f, 0.99875f, false}},
        {"minecraft:end_crystal", {2.0f, 2.0f, 1.7f, false}},
        {"minecraft:ender_dragon", {16.0f, 8.0f, 6.8f, false}},
        {"minecraft:ender_pearl", {0.25f, 0.25f, 0.2125f, false}},
        {"minecraft:enderman", {0.6f, 2.9f, 2.55f, false}},
        {"minecraft:endermite", {0.4f, 0.3f, 0.13f, false}},
        {"minecraft:evoker", {0.6f, 1.95f, 1.6575f, false}},
        {"minecraft:evoker_fangs", {0.5f, 0.8f, 0.68f, false}},
        {"minecraft:experience_bottle", {0.25f, 0.25f, 0.2125f, false}},
        {"minecraft:experience_orb", {0.5f, 0.5f, 0.425f, false}},
        {"minecraft:eye_of_ender", {0.25f, 0.25f, 0.2125f, false}},
        {"minecraft:falling_block", {0.98f, 0.98f, 0.83300006f, false}},
        {"minecraft:fireball", {1.0f, 1.0f, 0.85f, false}},
        {"minecraft:firework_rocket", {0.25f, 0.25f, 0.2125f, false}},
        {"minecraft:fishing_bobber", {0.25f, 0.25f, 0.2125f, false}},
        {"minecraft:fox", {0.6f, 0.7f, 0.4f, false}},
        {"minecraft:frog", {0.5f, 0.5f, 0.425f, false}},
        {"minecraft:furnace_minecart", {0.98f, 0.7f, 0.595f, false}},
        {"minecraft:ghast", {4.0f, 4.0f, 2.6f, false}},
        {"minecraft:giant", {3.6f, 12.0f, 10.44f, false}},
        {"minecraft:glow_item_frame", {0.5f, 0.5f, 0.0f, false}},
        {"minecraft:glow_squid", {0.8f, 0.8f, 0.4f, false}},
        {"minecraft:goat", {0.9f, 1.3f, 1.105f, false}},
        {"minecraft:guardian", {0.85f, 0.85f, 0.425f, false}},
        {"minecraft:happy_ghast", {4.0f, 4.0f, 2.6f, false}},
        {"minecraft:hoglin", {1.3964844f, 1.4f, 1.19f, false}},
        {"minecraft:hopper_minecart", {0.98f, 0.7f, 0.595f, false}},
        {"minecraft:horse", {1.3964844f, 1.6f, 1.52f, false}},
        {"minecraft:husk", {0.6f, 1.95f, 1.74f, false}},
        {"minecraft:illusioner", {0.6f, 1.95f, 1.6575f, false}},
        {"minecraft:interaction", {0.0f, 0.0f, 0.0f, false}},
        {"minecraft:iron_golem", {1.4f, 2.7f, 2.295f, false}},
        {"minecraft:item", {0.25f, 0.25f, 0.2125f, false}},
        {"minecraft:item_display", {0.0f, 0.0f, 0.0f, false}},
        {"minecraft:item_frame", {0.5f, 0.5f, 0.0f, false}},
        {"minecraft:jungle_boat", {1.375f, 0.5625f, 0.5625f, false}},
        {"minecraft:jungle_chest_boat", {1.375f, 0.5625f, 0.5625f, false}},
        {"minecraft:leash_knot", {0.375f, 0.5f, 0.0625f, false}},
        {"minecraft:lightning_bolt", {0.0f, 0.0f, 0.0f, false}},
        {"minecraft:lingering_potion", {0.25f, 0.25f, 0.2125f, false}},
        {"minecraft:llama", {0.9f, 1.87f, 1.7765f, false}},
        {"minecraft:llama_spit", {0.25f, 0.25f, 0.2125f, false}},
        {"minecraft:magma_cube", {0.52f, 0.52f, 0.325f, false}},
        {"minecraft:mangrove_boat", {1.375f, 0.5625f, 0.5625f, false}},
        {"minecraft:mangrove_chest_boat", {1.375f, 0.5625f, 0.5625f, false}},
        {"minecraft:mannequin", {0.6f, 1.8f, 1.62f, false}},
        {"minecraft:marker", {0.0f, 0.0f, 0.0f, false}},
        {"minecraft:minecart", {0.98f, 0.7f, 0.595f, false}},
        {"minecraft:mooshroom", {0.9f, 1.4f, 1.3f, false}},
        {"minecraft:mule", {1.3964844f, 1.6f, 1.52f, false}},
        {"minecraft:nautilus", {0.875f, 0.95f, 0.2751f, false}},
        {"minecraft:oak_boat", {1.375f, 0.5625f, 0.5625f, false}},
        {"minecraft:oak_chest_boat", {1.375f, 0.5625f, 0.5625f, false}},
        {"minecraft:ocelot", {0.6f, 0.7f, 0.595f, false}},
        {"minecraft:ominous_item_spawner", {0.25f, 0.25f, 0.2125f, false}},
        {"minecraft:painting", {0.5f, 0.5f, 0.425f, false}},
        {"minecraft:pale_oak_boat", {1.375f, 0.5625f, 0.5625f, false}},
        {"minecraft:pale_oak_chest_boat", {1.375f, 0.5625f, 0.5625f, false}},
        {"minecraft:panda", {1.3f, 1.25f, 1.0625f, false}},
        {"minecraft:parched", {0.6f, 1.99f, 1.74f, false}},
        {"minecraft:parrot", {0.5f, 0.9f, 0.54f, false}},
        {"minecraft:phantom", {0.9f, 0.5f, 0.175f, false}},
        {"minecraft:pig", {0.9f, 0.9f, 0.765f, false}},
        {"minecraft:piglin", {0.6f, 1.95f, 1.79f, false}},
        {"minecraft:piglin_brute", {0.6f, 1.95f, 1.79f, false}},
        {"minecraft:pillager", {0.6f, 1.95f, 1.6575f, false}},
        {"minecraft:player", {0.6f, 1.8f, 1.62f, false}},
        {"minecraft:polar_bear", {1.4f, 1.4f, 1.19f, false}},
        {"minecraft:pufferfish", {0.7f, 0.7f, 0.455f, false}},
        {"minecraft:rabbit", {0.4f, 0.5f, 0.425f, false}},
        {"minecraft:ravager", {1.95f, 2.2f, 1.8700001f, false}},
        {"minecraft:salmon", {0.7f, 0.4f, 0.26f, false}},
        {"minecraft:sheep", {0.9f, 1.3f, 1.235f, false}},
        {"minecraft:shulker", {1.0f, 1.0f, 0.5f, false}},
        {"minecraft:shulker_bullet", {0.3125f, 0.3125f, 0.265625f, false}},
        {"minecraft:silverfish", {0.4f, 0.3f, 0.13f, false}},
        {"minecraft:skeleton", {0.6f, 1.99f, 1.74f, false}},
        {"minecraft:skeleton_horse", {1.3964844f, 1.6f, 1.52f, false}},
        {"minecraft:slime", {0.52f, 0.52f, 0.325f, false}},
        {"minecraft:small_fireball", {0.3125f, 0.3125f, 0.265625f, false}},
        {"minecraft:sniffer", {1.9f, 1.75f, 1.05f, false}},
        {"minecraft:snow_golem", {0.7f, 1.9f, 1.7f, false}},
        {"minecraft:snowball", {0.25f, 0.25f, 0.2125f, false}},
        {"minecraft:spawner_minecart", {0.98f, 0.7f, 0.595f, false}},
        {"minecraft:spectral_arrow", {0.5f, 0.5f, 0.13f, false}},
        {"minecraft:spider", {1.4f, 0.9f, 0.65f, false}},
        {"minecraft:splash_potion", {0.25f, 0.25f, 0.2125f, false}},
        {"minecraft:spruce_boat", {1.375f, 0.5625f, 0.5625f, false}},
        {"minecraft:spruce_chest_boat", {1.375f, 0.5625f, 0.5625f, false}},
        {"minecraft:squid", {0.8f, 0.8f, 0.4f, false}},
        {"minecraft:stray", {0.6f, 1.99f, 1.74f, false}},
        {"minecraft:strider", {0.9f, 1.7f, 1.445f, false}},
        {"minecraft:tadpole", {0.4f, 0.3f, 0.19500001f, false}},
        {"minecraft:text_display", {0.0f, 0.0f, 0.0f, false}},
        {"minecraft:tnt", {0.98f, 0.98f, 0.15f, false}},
        {"minecraft:tnt_minecart", {0.98f, 0.7f, 0.595f, false}},
        {"minecraft:trader_llama", {0.9f, 1.87f, 1.7765f, false}},
        {"minecraft:trident", {0.5f, 0.5f, 0.13f, false}},
        {"minecraft:tropical_fish", {0.5f, 0.4f, 0.26f, false}},
        {"minecraft:turtle", {1.2f, 0.4f, 0.34f, false}},
        {"minecraft:vex", {0.4f, 0.8f, 0.51875f, false}},
        {"minecraft:villager", {0.6f, 1.95f, 1.62f, false}},
        {"minecraft:vindicator", {0.6f, 1.95f, 1.6575f, false}},
        {"minecraft:wandering_trader", {0.6f, 1.95f, 1.62f, false}},
        {"minecraft:warden", {0.9f, 2.9f, 2.4650002f, false}},
        {"minecraft:wind_charge", {0.3125f, 0.3125f, 0.0f, false}},
        {"minecraft:witch", {0.6f, 1.95f, 1.62f, false}},
        {"minecraft:wither", {0.9f, 3.5f, 2.9750001f, false}},
        {"minecraft:wither_skeleton", {0.7f, 2.4f, 2.1f, false}},
        {"minecraft:wither_skull", {0.3125f, 0.3125f, 0.265625f, false}},
        {"minecraft:wolf", {0.6f, 0.85f, 0.68f, false}},
        {"minecraft:zoglin", {1.3964844f, 1.4f, 1.19f, false}},
        {"minecraft:zombie", {0.6f, 1.95f, 1.74f, false}},
        {"minecraft:zombie_horse", {1.3964844f, 1.6f, 1.52f, false}},
        {"minecraft:zombie_nautilus", {0.875f, 0.95f, 0.2751f, false}},
        {"minecraft:zombie_villager", {0.6f, 1.95f, 1.74f, false}},
        {"minecraft:zombified_piglin", {0.6f, 1.95f, 1.79f, false}},
    };
    return table;
}

} // namespace

std::optional<EntitySize> vanillaEntitySize(std::string_view entityName)
{
    const auto& table = sizeTable();
    const auto found = table.find(entityName);
    if (found == table.end()) {
        return std::nullopt;
    }
    const VanillaSize& size = found->second;
    return EntitySize(size.width, size.height, size.eyeHeight, size.fixed);
}

std::optional<f32> vanillaEyeHeight(std::string_view entityName)
{
    const auto& table = sizeTable();
    const auto found = table.find(entityName);
    if (found == table.end()) {
        return std::nullopt;
    }
    return found->second.eyeHeight;
}

} // namespace mc::entity
