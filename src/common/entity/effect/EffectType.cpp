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

#include "EffectType.hpp"
#include <algorithm>
#include <unordered_map>

namespace mc {
namespace entity {
namespace effect {

// ============================================================================
// 效果名称映射
// ============================================================================

namespace {

/// 效果资源名称映射（不含命名空间）
const std::unordered_map<std::string, EffectType> s_effectResourceNameMap = {
    {"speed", EffectType::Speed},
    {"slowness", EffectType::Slowness},
    {"haste", EffectType::Haste},
    {"mining_fatigue", EffectType::MiningFatigue},
    {"strength", EffectType::Strength},
    {"instant_health", EffectType::InstantHealth},
    {"instant_damage", EffectType::InstantDamage},
    {"jump_boost", EffectType::JumpBoost},
    {"nausea", EffectType::Nausea},
    {"regeneration", EffectType::Regeneration},
    {"resistance", EffectType::Resistance},
    {"fire_resistance", EffectType::FireResistance},
    {"water_breathing", EffectType::WaterBreathing},
    {"invisibility", EffectType::Invisibility},
    {"blindness", EffectType::Blindness},
    {"night_vision", EffectType::NightVision},
    {"hunger", EffectType::Hunger},
    {"weakness", EffectType::Weakness},
    {"poison", EffectType::Poison},
    {"wither", EffectType::Wither},
    {"health_boost", EffectType::HealthBoost},
    {"absorption", EffectType::Absorption},
    {"saturation", EffectType::Saturation},
    {"glowing", EffectType::Glowing},
    {"levitation", EffectType::Levitation},
    {"luck", EffectType::Luck},
    {"bad_luck", EffectType::BadLuck},
    {"slow_falling", EffectType::SlowFalling},
    {"conduit_power", EffectType::ConduitPower},
    {"dolphins_grace", EffectType::DolphinsGrace},
    {"bad_omen", EffectType::BadOmen},
    {"hero_of_the_village", EffectType::HeroOfTheVillage},
    {"trial_omen", EffectType::TrialOmen},
    {"wind_charged", EffectType::WindCharged},
    {"raid_omen", EffectType::RaidOmen},
    {"darkness", EffectType::Darkness},
    {"weaving", EffectType::Weaving},
    {"oozing", EffectType::Oozing},
    {"infested", EffectType::Infested},
    {"breath_of_the_nautilus", EffectType::BreathOfTheNautilus},
};

/// 效果类型到资源名称的映射
const char* s_effectResourceNames[] = {
    "",                       // 0 - 无效
    "speed",                  // 1
    "slowness",               // 2
    "haste",                  // 3
    "mining_fatigue",         // 4
    "strength",               // 5
    "instant_health",         // 6
    "instant_damage",         // 7
    "jump_boost",             // 8
    "nausea",                 // 9
    "regeneration",           // 10
    "resistance",             // 11
    "fire_resistance",        // 12
    "water_breathing",        // 13
    "invisibility",           // 14
    "blindness",              // 15
    "night_vision",           // 16
    "hunger",                 // 17
    "weakness",               // 18
    "poison",                 // 19
    "wither",                 // 20
    "health_boost",           // 21
    "absorption",             // 22
    "saturation",             // 23
    "glowing",                // 24
    "levitation",             // 25
    "luck",                   // 26
    "bad_luck",               // 27
    "slow_falling",           // 28
    "conduit_power",          // 29
    "dolphins_grace",         // 30
    "bad_omen",               // 31
    "hero_of_the_village",    // 32
    "trial_omen",             // 33
    "wind_charged",           // 34
    "raid_omen",              // 35
    "darkness",               // 36
    "weaving",                // 37
    "oozing",                 // 38
    "infested",               // 39
    "breath_of_the_nautilus", // 40
};

/// 效果类型数量（不包括 0）
constexpr i32 EFFECT_COUNT = 40;

} // namespace

// ============================================================================
// 显示名称
// ============================================================================

const char* getEffectName(EffectType type) noexcept
{
    switch (type) {
        case EffectType::Speed:
            return "Speed";
        case EffectType::Slowness:
            return "Slowness";
        case EffectType::Haste:
            return "Haste";
        case EffectType::MiningFatigue:
            return "Mining Fatigue";
        case EffectType::Strength:
            return "Strength";
        case EffectType::InstantHealth:
            return "Instant Health";
        case EffectType::InstantDamage:
            return "Instant Damage";
        case EffectType::JumpBoost:
            return "Jump Boost";
        case EffectType::Nausea:
            return "Nausea";
        case EffectType::Regeneration:
            return "Regeneration";
        case EffectType::Resistance:
            return "Resistance";
        case EffectType::FireResistance:
            return "Fire Resistance";
        case EffectType::WaterBreathing:
            return "Water Breathing";
        case EffectType::Invisibility:
            return "Invisibility";
        case EffectType::Blindness:
            return "Blindness";
        case EffectType::NightVision:
            return "Night Vision";
        case EffectType::Hunger:
            return "Hunger";
        case EffectType::Weakness:
            return "Weakness";
        case EffectType::Poison:
            return "Poison";
        case EffectType::Wither:
            return "Wither";
        case EffectType::HealthBoost:
            return "Health Boost";
        case EffectType::Absorption:
            return "Absorption";
        case EffectType::Saturation:
            return "Saturation";
        case EffectType::Glowing:
            return "Glowing";
        case EffectType::Levitation:
            return "Levitation";
        case EffectType::Luck:
            return "Luck";
        case EffectType::BadLuck:
            return "Bad Luck";
        case EffectType::SlowFalling:
            return "Slow Falling";
        case EffectType::ConduitPower:
            return "Conduit Power";
        case EffectType::DolphinsGrace:
            return "Dolphin's Grace";
        case EffectType::BadOmen:
            return "Bad Omen";
        case EffectType::HeroOfTheVillage:
            return "Hero of the Village";
        case EffectType::TrialOmen:
            return "Trial Omen";
        case EffectType::WindCharged:
            return "Wind Charged";
        case EffectType::RaidOmen:
            return "Raid Omen";
        case EffectType::Darkness:
            return "Darkness";
        case EffectType::Weaving:
            return "Weaving";
        case EffectType::Oozing:
            return "Oozing";
        case EffectType::Infested:
            return "Infested";
        case EffectType::BreathOfTheNautilus:
            return "Breath of the Nautilus";
        default:
            return "Unknown";
    }
}

bool isBeneficialEffect(EffectType type) noexcept
{
    switch (type) {
        case EffectType::Speed:
        case EffectType::Haste:
        case EffectType::Strength:
        case EffectType::InstantHealth:
        case EffectType::JumpBoost:
        case EffectType::Regeneration:
        case EffectType::Resistance:
        case EffectType::FireResistance:
        case EffectType::WaterBreathing:
        case EffectType::Invisibility:
        case EffectType::NightVision:
        case EffectType::HealthBoost:
        case EffectType::Absorption:
        case EffectType::Saturation:
        case EffectType::Luck:
        case EffectType::SlowFalling:
        case EffectType::ConduitPower:
        case EffectType::DolphinsGrace:
        case EffectType::HeroOfTheVillage:
            return true;
        case EffectType::WindCharged:
            return true;
        case EffectType::Darkness:
        default:
            return false;
    }
}

u32 getEffectColor(EffectType type) noexcept
{
    switch (type) {
        case EffectType::Speed:
            return 0x7CAFC6;
        case EffectType::Slowness:
            return 0x5A6C81;
        case EffectType::Haste:
            return 0xD9C043;
        case EffectType::MiningFatigue:
            return 0x4A7210;
        case EffectType::Strength:
            return 0x932423;
        case EffectType::InstantHealth:
            return 0xF82423;
        case EffectType::InstantDamage:
            return 0x430A09;
        case EffectType::JumpBoost:
            return 0x22FF4C;
        case EffectType::Nausea:
            return 0xC31C4D;
        case EffectType::Regeneration:
            return 0xCD5CAB;
        case EffectType::Resistance:
            return 0x99453A;
        case EffectType::FireResistance:
            return 0xE49A3A;
        case EffectType::WaterBreathing:
            return 0x2E5299;
        case EffectType::Invisibility:
            return 0x7F8392;
        case EffectType::Blindness:
            return 0x1F1F23;
        case EffectType::NightVision:
            return 0x1F1FA1;
        case EffectType::Hunger:
            return 0x587653;
        case EffectType::Weakness:
            return 0x484D48;
        case EffectType::Poison:
            return 0x4E9331;
        case EffectType::Wither:
            return 0x352A27;
        case EffectType::HealthBoost:
            return 0xF87D23;
        case EffectType::Absorption:
            return 0x2552A5;
        case EffectType::Saturation:
            return 0xF82423;
        case EffectType::Glowing:
            return 0x94A061;
        case EffectType::Levitation:
            return 0xCEFFFF;
        case EffectType::Luck:
            return 0x339900;
        case EffectType::BadLuck:
            return 0xC0A44D;
        case EffectType::SlowFalling:
            return 0xFFFBF1;
        case EffectType::ConduitPower:
            return 0x1DC2D1;
        case EffectType::DolphinsGrace:
            return 0x8894C6;
        case EffectType::BadOmen:
            return 0x0B0B0B;
        case EffectType::HeroOfTheVillage:
            return 0x44FF44;
        case EffectType::TrialOmen:
            return 0x2D6A4F; // 深绿色 - 试炼之兆
        case EffectType::WindCharged:
            return 0x77FFD4; // 浅青色 - 风充能
        case EffectType::RaidOmen:
            return 0x5B0B0B; // 深红色 - 袭击之兆
        case EffectType::Darkness:
            return 0x1F1F21; // 深灰色 - 黑暗
        case EffectType::Weaving:
            return 0x4A3B2A; // 蛛网棕 - 盘绕
        case EffectType::Oozing:
            return 0x4E6B3A; // 史莱姆绿 - 渗浆
        case EffectType::Infested:
            return 0x5A3A2A; // 寄生褐 - 寄生
        case EffectType::BreathOfTheNautilus:
            return 0x2D6A8C; // 鹦鹉螺蓝 - 鹦鹉螺之力
        default:
            return 0xFFFFFF;
    }
}

// ============================================================================
// ID 转换函数
// ============================================================================

std::optional<EffectType> getEffectById(i32 id) noexcept
{
    if (id < 1 || id > EFFECT_COUNT) {
        return std::nullopt;
    }
    return static_cast<EffectType>(id);
}

std::optional<EffectType> getEffectByResourceLocation(const ResourceLocation& id) noexcept
{
    // 尝试直接匹配路径（不含命名空间）
    std::string path = id.path();
    std::transform(path.begin(), path.end(), path.begin(), [](char c) {
        return static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    });

    auto it = s_effectResourceNameMap.find(path);
    if (it != s_effectResourceNameMap.end()) {
        return it->second;
    }

    // 尝试下划线转换（处理 "nightVision" -> "night_vision" 等）
    // Minecraft 使用 snake_case 格式
    return std::nullopt;
}

ResourceLocation getEffectResourceLocation(EffectType type) noexcept
{
    const char* name = getEffectResourceName(type);
    return ResourceLocation("minecraft", name);
}

const char* getEffectResourceName(EffectType type) noexcept
{
    const i32 id = static_cast<i32>(type);
    if (id < 1 || id > EFFECT_COUNT) {
        return "unknown";
    }
    return s_effectResourceNames[id];
}

bool isInstantEffect(EffectType type) noexcept
{
    // 瞬间效果包括：瞬间治疗、瞬间伤害、饱和
    return type == EffectType::InstantHealth || type == EffectType::InstantDamage || type == EffectType::Saturation;
}

} // namespace effect
} // namespace entity
} // namespace mc
