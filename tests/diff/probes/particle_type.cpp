/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of
 * this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction,
 * including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell

 * * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the
 * following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies
 * or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR
 * PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES
 * OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

/**
 * @brief 差分能力：粒子类型的协议 ID 与注册表 ID 的对齐（`ParticleTypeId`）。
 *
 * `list`：原版 115
 * 个粒子类型的注册表 ID，**按协议 ID 升序**，形如 `0|angry_villager`
 * `run` ：同上一行 → { "result": "<Cubium
 * 在该索引的枚举名>" } 或 `"OUT_OF_RANGE"`
 *
 * ## 为什么这个能力重要
 *
 * `ParticleTypeId`
 * 的枚举值**直接就是网络协议 ID**——服务端发的
 * `ClientboundLevelParticlesPacket`
 * 里带的就是这个数值，客户端按下标查表。
 * 因此**索引只要错一位，后面所有粒子都会错位**，表现为「放烟花却出末影粒子」

 * * 这类完全不讲道理的现象，且不会报任何错误。
 *
 * ## 注意：Cubium 有内部扩展类型
 *
 * Cubium 在 0..114
 * 之外还定义了若干**本项目内部扩展**的粒子（`Breaking`、
 * `Barrier`、`Light` 等，索引 115
 * 起）。这些不发给原版客户端，
 * 由 `PROTOCOL_PARTICLE_TYPE_COUNT = 115` 界定范围。
 * 本能力只比对
 * 0..114，扩展部分不参与比对。
 */

#include "common/core/Types.hpp"
#include "common/particle/ParticleTypes.hpp"
#include "probe.hpp"
#include <cstdio>
#include <cstdlib>
#include <string>
#include <string_view>
#include <nlohmann/json.hpp>

namespace mc::diff {

namespace {

using nlohmann::json;

/// Cubium 的 `ParticleTypeId` 在协议范围内的枚举名（索引 = 协议 ID）。
struct ParticleEntry {
    int id;
    const char* name;
};

const ParticleEntry CUBIUM_PARTICLES[] = {
    {0, "AngryVillager"},
    {1, "Block"},
    {2, "BlockMarker"},
    {3, "Bubble"},
    {4, "Cloud"},
    {5, "CopperFireFlame"},
    {6, "Crit"},
    {7, "DamageIndicator"},
    {8, "DragonBreath"},
    {9, "DrippingLava"},
    {10, "FallingLava"},
    {11, "LandingLava"},
    {12, "DrippingWater"},
    {13, "FallingWater"},
    {14, "Dust"},
    {15, "DustColorTransition"},
    {16, "Spell"},
    {17, "ElderGuardian"},
    {18, "EnchantedHit"},
    {19, "Enchant"},
    {20, "EndRod"},
    {21, "EntityEffect"},
    {22, "HugeExplosion"},
    {23, "Explosion"},
    {24, "Gust"},
    {25, "SmallGust"},
    {26, "GustEmitterLarge"},
    {27, "GustEmitterSmall"},
    {28, "SonicBoom"},
    {29, "FallingDust"},
    {30, "Firework"},
    {31, "Fishing"},
    {32, "Flame"},
    {33, "Infested"},
    {34, "CherryLeaves"},
    {35, "PaleOakLeaves"},
    {36, "TintedLeaves"},
    {37, "SculkSoul"},
    {38, "SculkCharge"},
    {39, "SculkChargePop"},
    {40, "SoulFireFlame"},
    {41, "Soul"},
    {42, "Flash"},
    {43, "HappyVillager"},
    {44, "Composter"},
    {45, "Heart"},
    {46, "InstantSpell"},
    {47, "Item"},
    {48, "Vibration"},
    {49, "Trail"},
    {50, "ItemSlime"},
    {51, "ItemCobweb"},
    {52, "ItemSnowball"},
    {53, "LargeSmoke"},
    {54, "Lava"},
    {55, "Mycelium"},
    {56, "Note"},
    {57, "Poof"},
    {58, "Portal"},
    {59, "Rain"},
    {60, "Smoke"},
    {61, "WhiteSmoke"},
    {62, "Sneeze"},
    {63, "Spit"},
    {64, "SquidInk"},
    {65, "SweepAttack"},
    {66, "TotemOfUndying"},
    {67, "Underwater"},
    {68, "Splash"},
    {69, "Witch"},
    {70, "BubblePop"},
    {71, "CurrentDown"},
    {72, "BubbleColumnUp"},
    {73, "Nautilus"},
    {74, "Dolphin"},
    {75, "CampfireCozy"},
    {76, "CampfireSignal"},
    {77, "DrippingHoney"},
    {78, "FallingHoney"},
    {79, "LandingHoney"},
    {80, "FallingNectar"},
    {81, "FallingSporeBlossom"},
    {82, "Ash"},
    {83, "CrimsonSpore"},
    {84, "WarpedSpore"},
    {85, "SporeBlossomAir"},
    {86, "DrippingObsidianTear"},
    {87, "FallingObsidianTear"},
    {88, "LandingObsidianTear"},
    {89, "ReversePortal"},
    {90, "WhiteAsh"},
    {91, "SmallFlame"},
    {92, "Snowflake"},
    {93, "DrippingDripstoneLava"},
    {94, "FallingDripstoneLava"},
    {95, "DrippingDripstoneWater"},
    {96, "FallingDripstoneWater"},
    {97, "GlowSquidInk"},
    {98, "Glow"},
    {99, "WaxOn"},
    {100, "WaxOff"},
    {101, "ElectricSpark"},
    {102, "Scrape"},
    {103, "Shriek"},
    {104, "EggCrack"},
    {105, "DustPlume"},
    {106, "TrialSpawnerDetection"},
    {107, "TrialSpawnerDetectionOminous"},
    {108, "VaultConnection"},
    {109, "DustPillar"},
    {110, "OminousSpawning"},
    {111, "RaidOmen"},
    {112, "TrialOmen"},
    {113, "BlockCrumble"},
    {114, "Firefly"},
    {115, "Breaking"},
    {116, "Barrier"},
    {117, "Light"},
    {118, "Redstone"},
    {119, "LargeExplosion"},
    {120, "ItemPickup"},
    {121, "DrippingCherryLeaves"},
    {122, "FallingCherryLeaves"},
    {123, "LandingCherryLeaves"},
    {124, "Count"},
};

constexpr int CUBIUM_PARTICLE_COUNT = static_cast<int>(sizeof(CUBIUM_PARTICLES) / sizeof(CUBIUM_PARTICLES[0]));

/**
 * @brief 把 Cubium 的驼峰枚举名转成原版的注册表 ID（snake_case）。
 *
 * `AngryVillager` →
 * `angry_villager`、`BlockMarker` → `block_marker`。
 * 与 oracle 输出的注册表 ID 对齐后才能直接字符串比较。

 */
std::string toRegistryName(std::string_view camel)
{
    std::string out;
    out.reserve(camel.size() + 8);
    for (std::size_t i = 0; i < camel.size(); ++i) {
        const char ch = camel[i];
        if (ch >= 'A' && ch <= 'Z') {
            if (i != 0) {
                out.push_back('_');
            }
            out.push_back(static_cast<char>(ch - 'A' + 'a'));
        } else {
            out.push_back(ch);
        }
    }
    return out;
}

/// 原版 115 个粒子类型，按协议 ID 升序。
const char* const VANILLA_PARTICLES[] = {
    "0|angry_villager",
    "1|block",
    "2|block_marker",
    "3|bubble",
    "4|cloud",
    "5|copper_fire_flame",
    "6|crit",
    "7|damage_indicator",
    "8|dragon_breath",
    "9|dripping_lava",
    "10|falling_lava",
    "11|landing_lava",
    "12|dripping_water",
    "13|falling_water",
    "14|dust",
    "15|dust_color_transition",
    "16|effect",
    "17|elder_guardian",
    "18|enchanted_hit",
    "19|enchant",
    "20|end_rod",
    "21|entity_effect",
    "22|explosion_emitter",
    "23|explosion",
    "24|gust",
    "25|small_gust",
    "26|gust_emitter_large",
    "27|gust_emitter_small",
    "28|sonic_boom",
    "29|falling_dust",
    "30|firework",
    "31|fishing",
    "32|flame",
    "33|infested",
    "34|cherry_leaves",
    "35|pale_oak_leaves",
    "36|tinted_leaves",
    "37|sculk_soul",
    "38|sculk_charge",
    "39|sculk_charge_pop",
    "40|soul_fire_flame",
    "41|soul",
    "42|flash",
    "43|happy_villager",
    "44|composter",
    "45|heart",
    "46|instant_effect",
    "47|item",
    "48|vibration",
    "49|trail",
    "50|item_slime",
    "51|item_cobweb",
    "52|item_snowball",
    "53|large_smoke",
    "54|lava",
    "55|mycelium",
    "56|note",
    "57|poof",
    "58|portal",
    "59|rain",
    "60|smoke",
    "61|white_smoke",
    "62|sneeze",
    "63|spit",
    "64|squid_ink",
    "65|sweep_attack",
    "66|totem_of_undying",
    "67|underwater",
    "68|splash",
    "69|witch",
    "70|bubble_pop",
    "71|current_down",
    "72|bubble_column_up",
    "73|nautilus",
    "74|dolphin",
    "75|campfire_cosy_smoke",
    "76|campfire_signal_smoke",
    "77|dripping_honey",
    "78|falling_honey",
    "79|landing_honey",
    "80|falling_nectar",
    "81|falling_spore_blossom",
    "82|ash",
    "83|crimson_spore",
    "84|warped_spore",
    "85|spore_blossom_air",
    "86|dripping_obsidian_tear",
    "87|falling_obsidian_tear",
    "88|landing_obsidian_tear",
    "89|reverse_portal",
    "90|white_ash",
    "91|small_flame",
    "92|snowflake",
    "93|dripping_dripstone_lava",
    "94|falling_dripstone_lava",
    "95|dripping_dripstone_water",
    "96|falling_dripstone_water",
    "97|glow_squid_ink",
    "98|glow",
    "99|wax_on",
    "100|wax_off",
    "101|electric_spark",
    "102|scrape",
    "103|shriek",
    "104|egg_crack",
    "105|dust_plume",
    "106|trial_spawner_detection",
    "107|trial_spawner_detection_ominous",
    "108|vault_connection",
    "109|dust_pillar",
    "110|ominous_spawning",
    "111|raid_omen",
    "112|trial_omen",
    "113|block_crumble",
    "114|firefly",
};

void writeList(FILE* out)
{
    const std::size_t count = sizeof(VANILLA_PARTICLES) / sizeof(VANILLA_PARTICLES[0]);
    for (std::size_t i = 0; i < count; ++i) {
        std::fprintf(out, "%zu|%s\n", i, VANILLA_PARTICLES[i]);
    }
}

std::string runLine(std::string_view inputLine)
{
    const std::string line = json::parse(inputLine).at("unit").get<std::string>();
    const size_t bar = line.find('|');
    if (bar == std::string::npos) {
        std::fprintf(stderr, "particle_type: 单元应为 <id>|<name>，实际：%s\n", line.c_str());
        std::exit(1);
    }
    const int id = std::atoi(line.substr(0, bar).c_str());

    for (int i = 0; i < CUBIUM_PARTICLE_COUNT; ++i) {
        if (CUBIUM_PARTICLES[i].id == id) {
            return json{{"unit", line}, {"result", toRegistryName(CUBIUM_PARTICLES[i].name)}}.dump();
        }
    }
    return json{{"unit", line}, {"result", "OUT_OF_RANGE"}}.dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE("particle_type", &writeList, &runLine)

} // namespace mc::diff
