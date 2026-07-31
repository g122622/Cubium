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

#include "common/world/entity/JavaEntityTypeIdMap.hpp"

#include <spdlog/spdlog.h>

#include <array>
#include <string>
#include <string_view>
#include <utility>

namespace mc {

// ============================================================================
// 单例
// ============================================================================

JavaEntityTypeIdMap& JavaEntityTypeIdMap::instance()
{
    static JavaEntityTypeIdMap s_instance;
    return s_instance;
}

// ============================================================================
// vanilla 1.21.11 entity_type 注册顺序（EntityType.java 静态 register("name",...) 调用顺序）
// registry id = 下标。提取自 D:/Minecraft/MC研究/Minecraft1.21.11源码/net/minecraft/world/entity/
// EntityType.java。该注册表未由本项目 RegistryDataBuilder 同步，真 Java 客户端用内置 core 包，
// id 即此顺序。共 157 条（id 0..156）。
// ============================================================================
static constexpr std::array<std::string_view, 157> kVanillaEntityTypeNames = {
    "minecraft:acacia_boat",            // 0
    "minecraft:acacia_chest_boat",      // 1
    "minecraft:allay",                  // 2
    "minecraft:area_effect_cloud",      // 3
    "minecraft:armadillo",              // 4
    "minecraft:armor_stand",            // 5
    "minecraft:arrow",                  // 6
    "minecraft:axolotl",                // 7
    "minecraft:bamboo_chest_raft",      // 8
    "minecraft:bamboo_raft",            // 9
    "minecraft:bat",                    // 10
    "minecraft:bee",                    // 11
    "minecraft:birch_boat",             // 12
    "minecraft:birch_chest_boat",       // 13
    "minecraft:blaze",                  // 14
    "minecraft:block_display",          // 15
    "minecraft:bogged",                 // 16
    "minecraft:breeze",                 // 17
    "minecraft:breeze_wind_charge",     // 18
    "minecraft:camel",                  // 19
    "minecraft:camel_husk",             // 20
    "minecraft:cat",                    // 21
    "minecraft:cave_spider",            // 22
    "minecraft:cherry_boat",            // 23
    "minecraft:cherry_chest_boat",      // 24
    "minecraft:chest_minecart",         // 25
    "minecraft:chicken",                // 26
    "minecraft:cod",                    // 27
    "minecraft:copper_golem",           // 28
    "minecraft:command_block_minecart", // 29
    "minecraft:cow",                    // 30
    "minecraft:creaking",               // 31
    "minecraft:creeper",                // 32
    "minecraft:dark_oak_boat",          // 33
    "minecraft:dark_oak_chest_boat",    // 34
    "minecraft:dolphin",                // 35
    "minecraft:donkey",                 // 36
    "minecraft:dragon_fireball",        // 37
    "minecraft:drowned",                // 38
    "minecraft:egg",                    // 39
    "minecraft:elder_guardian",         // 40
    "minecraft:enderman",               // 41
    "minecraft:endermite",              // 42
    "minecraft:ender_dragon",           // 43
    "minecraft:ender_pearl",            // 44
    "minecraft:end_crystal",            // 45
    "minecraft:evoker",                 // 46
    "minecraft:evoker_fangs",           // 47
    "minecraft:experience_bottle",      // 48
    "minecraft:experience_orb",         // 49
    "minecraft:eye_of_ender",           // 50
    "minecraft:falling_block",          // 51
    "minecraft:fireball",               // 52
    "minecraft:firework_rocket",        // 53
    "minecraft:fox",                    // 54
    "minecraft:frog",                   // 55
    "minecraft:furnace_minecart",       // 56
    "minecraft:ghast",                  // 57
    "minecraft:happy_ghast",            // 58
    "minecraft:giant",                  // 59
    "minecraft:glow_item_frame",        // 60
    "minecraft:glow_squid",             // 61
    "minecraft:goat",                   // 62
    "minecraft:guardian",               // 63
    "minecraft:hoglin",                 // 64
    "minecraft:hopper_minecart",        // 65
    "minecraft:horse",                  // 66
    "minecraft:husk",                   // 67
    "minecraft:illusioner",             // 68
    "minecraft:interaction",            // 69
    "minecraft:iron_golem",             // 70
    "minecraft:item",                   // 71
    "minecraft:item_display",           // 72
    "minecraft:item_frame",             // 73
    "minecraft:jungle_boat",            // 74
    "minecraft:jungle_chest_boat",      // 75
    "minecraft:leash_knot",             // 76
    "minecraft:lightning_bolt",         // 77
    "minecraft:llama",                  // 78
    "minecraft:llama_spit",             // 79
    "minecraft:magma_cube",             // 80
    "minecraft:mangrove_boat",          // 81
    "minecraft:mangrove_chest_boat",    // 82
    "minecraft:mannequin",              // 83
    "minecraft:marker",                 // 84
    "minecraft:minecart",               // 85
    "minecraft:mooshroom",              // 86
    "minecraft:mule",                   // 87
    "minecraft:nautilus",               // 88
    "minecraft:oak_boat",               // 89
    "minecraft:oak_chest_boat",         // 90
    "minecraft:ocelot",                 // 91
    "minecraft:ominous_item_spawner",   // 92
    "minecraft:painting",               // 93
    "minecraft:pale_oak_boat",          // 94
    "minecraft:pale_oak_chest_boat",    // 95
    "minecraft:panda",                  // 96
    "minecraft:parched",                // 97
    "minecraft:parrot",                 // 98
    "minecraft:phantom",                // 99
    "minecraft:pig",                    // 100
    "minecraft:piglin",                 // 101
    "minecraft:piglin_brute",           // 102
    "minecraft:pillager",               // 103
    "minecraft:polar_bear",             // 104
    "minecraft:splash_potion",          // 105
    "minecraft:lingering_potion",       // 106
    "minecraft:pufferfish",             // 107
    "minecraft:rabbit",                 // 108
    "minecraft:ravager",                // 109
    "minecraft:salmon",                 // 110
    "minecraft:sheep",                  // 111
    "minecraft:shulker",                // 112
    "minecraft:shulker_bullet",         // 113
    "minecraft:silverfish",             // 114
    "minecraft:skeleton",               // 115
    "minecraft:skeleton_horse",         // 116
    "minecraft:slime",                  // 117
    "minecraft:small_fireball",         // 118
    "minecraft:sniffer",                // 119
    "minecraft:snowball",               // 120
    "minecraft:snow_golem",             // 121
    "minecraft:spawner_minecart",       // 122
    "minecraft:spectral_arrow",         // 123
    "minecraft:spider",                 // 124
    "minecraft:spruce_boat",            // 125
    "minecraft:spruce_chest_boat",      // 126
    "minecraft:squid",                  // 127
    "minecraft:stray",                  // 128
    "minecraft:strider",                // 129
    "minecraft:tadpole",                // 130
    "minecraft:text_display",           // 131
    "minecraft:tnt",                    // 132
    "minecraft:tnt_minecart",           // 133
    "minecraft:trader_llama",           // 134
    "minecraft:trident",                // 135
    "minecraft:tropical_fish",          // 136
    "minecraft:turtle",                 // 137
    "minecraft:vex",                    // 138
    "minecraft:villager",               // 139
    "minecraft:vindicator",             // 140
    "minecraft:wandering_trader",       // 141
    "minecraft:warden",                 // 142
    "minecraft:wind_charge",            // 143
    "minecraft:witch",                  // 144
    "minecraft:wither",                 // 145
    "minecraft:wither_skeleton",        // 146
    "minecraft:wither_skull",           // 147
    "minecraft:wolf",                   // 148
    "minecraft:zoglin",                 // 149
    "minecraft:zombie",                 // 150
    "minecraft:zombie_horse",           // 151
    "minecraft:zombie_nautilus",        // 152
    "minecraft:zombie_villager",        // 153
    "minecraft:zombified_piglin",       // 154
    "minecraft:player",                 // 155
    "minecraft:fishing_bobber",         // 156
};

// ============================================================================
// 项目特有键别名 → 选定的 vanilla id
// vanilla 1.21.11 无这些泛型实体类型，项目用单一类型对应 vanilla 的多个/不同类型，
// 选一个最贴近的 vanilla id。船类（boat/chest_boat）不在此处理，由 BoatEntity/ChestBoatEntity
// 的 getJavaEntityTypeId() override 按木种拼变体名查主表。
// ============================================================================
static constexpr std::array<std::pair<std::string_view, std::string_view>, 2> kAliases = {{
    // 项目 minecraft:potion 对应 vanilla 投掷药水（splash_potion=105）；
    // lingering_potion(106) 项目未单独建模，统一按 splash。
    {"minecraft:potion", "minecraft:splash_potion"},
    // 项目 minecraft:spear 是非 vanilla 实体（1.16 combat test 长矛），最贴近的 vanilla
    // 可回收投掷物是 trident(135)（同为 throw + loyalty 返回语义）。
    {"minecraft:spear", "minecraft:trident"},
}};

// ============================================================================
// 公开接口
// ============================================================================

Result<void> JavaEntityTypeIdMap::initialize()
{
    m_initialized = false;
    m_nameToId.clear();
    m_idToName.clear();

    m_nameToId.reserve(kVanillaEntityTypeNames.size());
    m_idToName.reserve(kVanillaEntityTypeNames.size());
    for (u32 i = 0; i < kVanillaEntityTypeNames.size(); ++i) {
        m_nameToId[std::string(kVanillaEntityTypeNames[i])] = i;
        m_idToName[i] = std::string(kVanillaEntityTypeNames[i]);
    }

    spdlog::info("JavaEntityTypeIdMap: initialized with {} vanilla entity types, {} aliases",
        kVanillaEntityTypeNames.size(),
        kAliases.size());

    m_initialized = true;
    return {};
}

u32 JavaEntityTypeIdMap::toJavaRegistryId(std::string_view name) const
{
    if (!m_initialized) {
        // 防御：漏初始化时自动建表（幂等），避免全发 id 0。
        (void)const_cast<JavaEntityTypeIdMap*>(this)->initialize();
    }

    // 先查别名表（项目特有键 → vanilla 键）
    for (const auto& [projKey, vanillaKey] : kAliases) {
        if (name == projKey) {
            if (const auto it = m_nameToId.find(std::string(vanillaKey)); it != m_nameToId.end()) {
                return it->second;
            }
            break;
        }
    }

    if (const auto it = m_nameToId.find(std::string(name)); it != m_nameToId.end()) {
        return it->second;
    }

    spdlog::warn("JavaEntityTypeIdMap: toJavaRegistryId miss for entity type {}", name);
    return 0; // acacia_boat 兜底
}

std::string_view JavaEntityTypeIdMap::fromJavaRegistryId(u32 javaRegistryId) const
{
    if (!m_initialized) {
        return {};
    }
    if (const auto it = m_idToName.find(javaRegistryId); it != m_idToName.end()) {
        return it->second;
    }
    spdlog::warn("JavaEntityTypeIdMap: fromJavaRegistryId miss for javaRegistryId={}", javaRegistryId);
    return {};
}

} // namespace mc
