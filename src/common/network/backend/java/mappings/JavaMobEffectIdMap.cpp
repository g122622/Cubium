/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including limitation the rights
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

#include "common/network/backend/java/mappings/JavaMobEffectIdMap.hpp"

#include "common/entity/effect/EffectType.hpp"
#include "common/resource/ResourceLocation.hpp"

#include <spdlog/spdlog.h>

#include <string>

namespace mc::network::backend::java {

namespace {

/// vanilla 1.21.11 MobEffects.java 静态字段声明顺序（wire id 权威源，0-based）。
/// 每项 = {vanilla registry id, vanilla name, 项目 EffectType}。
/// 项目 EffectType 数字值与 vanilla id 存在错位（darkness/wind_charged/raid_omen），
/// 故按资源名而非枚举数字建立对应关系。vanilla unluck(26) 对应项目 BadLuck。
struct VanillaEffectEntry {
    u32 vanillaId;
    const char* vanillaName;
    ::mc::entity::effect::EffectType type;
};

constexpr VanillaEffectEntry kVanillaEffects[] = {
    {0, "minecraft:speed", ::mc::entity::effect::EffectType::Speed},
    {1, "minecraft:slowness", ::mc::entity::effect::EffectType::Slowness},
    {2, "minecraft:haste", ::mc::entity::effect::EffectType::Haste},
    {3, "minecraft:mining_fatigue", ::mc::entity::effect::EffectType::MiningFatigue},
    {4, "minecraft:strength", ::mc::entity::effect::EffectType::Strength},
    {5, "minecraft:instant_health", ::mc::entity::effect::EffectType::InstantHealth},
    {6, "minecraft:instant_damage", ::mc::entity::effect::EffectType::InstantDamage},
    {7, "minecraft:jump_boost", ::mc::entity::effect::EffectType::JumpBoost},
    {8, "minecraft:nausea", ::mc::entity::effect::EffectType::Nausea},
    {9, "minecraft:regeneration", ::mc::entity::effect::EffectType::Regeneration},
    {10, "minecraft:resistance", ::mc::entity::effect::EffectType::Resistance},
    {11, "minecraft:fire_resistance", ::mc::entity::effect::EffectType::FireResistance},
    {12, "minecraft:water_breathing", ::mc::entity::effect::EffectType::WaterBreathing},
    {13, "minecraft:invisibility", ::mc::entity::effect::EffectType::Invisibility},
    {14, "minecraft:blindness", ::mc::entity::effect::EffectType::Blindness},
    {15, "minecraft:night_vision", ::mc::entity::effect::EffectType::NightVision},
    {16, "minecraft:hunger", ::mc::entity::effect::EffectType::Hunger},
    {17, "minecraft:weakness", ::mc::entity::effect::EffectType::Weakness},
    {18, "minecraft:poison", ::mc::entity::effect::EffectType::Poison},
    {19, "minecraft:wither", ::mc::entity::effect::EffectType::Wither},
    {20, "minecraft:health_boost", ::mc::entity::effect::EffectType::HealthBoost},
    {21, "minecraft:absorption", ::mc::entity::effect::EffectType::Absorption},
    {22, "minecraft:saturation", ::mc::entity::effect::EffectType::Saturation},
    {23, "minecraft:glowing", ::mc::entity::effect::EffectType::Glowing},
    {24, "minecraft:levitation", ::mc::entity::effect::EffectType::Levitation},
    {25, "minecraft:luck", ::mc::entity::effect::EffectType::Luck},
    {26, "minecraft:unluck", ::mc::entity::effect::EffectType::BadLuck},
    {27, "minecraft:slow_falling", ::mc::entity::effect::EffectType::SlowFalling},
    {28, "minecraft:conduit_power", ::mc::entity::effect::EffectType::ConduitPower},
    {29, "minecraft:dolphins_grace", ::mc::entity::effect::EffectType::DolphinsGrace},
    {30, "minecraft:bad_omen", ::mc::entity::effect::EffectType::BadOmen},
    {31, "minecraft:hero_of_the_village", ::mc::entity::effect::EffectType::HeroOfTheVillage},
    {32, "minecraft:darkness", ::mc::entity::effect::EffectType::Darkness},
    {33, "minecraft:trial_omen", ::mc::entity::effect::EffectType::TrialOmen},
    {34, "minecraft:raid_omen", ::mc::entity::effect::EffectType::RaidOmen},
    {35, "minecraft:wind_charged", ::mc::entity::effect::EffectType::WindCharged},
    {36, "minecraft:weaving", ::mc::entity::effect::EffectType::Weaving},
    {37, "minecraft:oozing", ::mc::entity::effect::EffectType::Oozing},
    {38, "minecraft:infested", ::mc::entity::effect::EffectType::Infested},
    {39, "minecraft:breath_of_the_nautilus", ::mc::entity::effect::EffectType::BreathOfTheNautilus},
};

constexpr size_t kVanillaEffectCount = std::size(kVanillaEffects);

} // namespace

// ============================================================================
// 单例
// ============================================================================

JavaMobEffectIdMap& JavaMobEffectIdMap::instance()
{
    static JavaMobEffectIdMap s_instance;
    return s_instance;
}

// ============================================================================
// 公开接口
// ============================================================================

Result<void> JavaMobEffectIdMap::initialize()
{
    m_initialized = false;
    m_nameToJava.clear();
    m_javaToInternal.clear();

    for (size_t i = 0; i < kVanillaEffectCount; ++i) {
        const auto& entry = kVanillaEffects[i];
        m_nameToJava[entry.vanillaName] = entry.vanillaId;
        m_javaToInternal[entry.vanillaId] = entry.type;
    }

    spdlog::info("JavaMobEffectIdMap: matched {} mob effects", m_javaToInternal.size());

    m_initialized = true;
    return {};
}

u32 JavaMobEffectIdMap::toJavaRegistryId(::mc::entity::effect::EffectType type) const
{
    if (!m_initialized) {
        // 防御：漏初始化时自动建表（幂等），避免全发 id 0。
        (void)const_cast<JavaMobEffectIdMap*>(this)->initialize();
    }
    // 按项目 EffectType 反查 vanilla name（用项目资源名映射，注意 unluck/bad_luck 别名）。
    const ::mc::ResourceLocation loc = ::mc::entity::effect::getEffectResourceLocation(type);
    const std::string key = loc.toString();
    // 项目 BadLuck 的资源名是 bad_luck，vanilla 注册名是 unluck，需别名。
    static const std::string kBadLuckProject = "minecraft:bad_luck";
    static const std::string kBadLuckVanilla = "minecraft:unluck";
    const std::string& effective = (key == kBadLuckProject) ? kBadLuckVanilla : key;
    if (const auto it = m_nameToJava.find(effective); it != m_nameToJava.end()) {
        return it->second;
    }
    spdlog::warn("JavaMobEffectIdMap: toJavaRegistryId miss for effect {}", key);
    return 0; // speed 兜底
}

::mc::entity::effect::EffectType JavaMobEffectIdMap::fromJavaRegistryId(u32 javaRegistryId) const
{
    if (!m_initialized) {
        return ::mc::entity::effect::EffectType::Speed;
    }
    if (const auto it = m_javaToInternal.find(javaRegistryId); it != m_javaToInternal.end()) {
        return it->second;
    }
    spdlog::warn("JavaMobEffectIdMap: fromJavaRegistryId miss for javaRegistryId={}", javaRegistryId);
    return ::mc::entity::effect::EffectType::Speed; // speed 兜底
}

} // namespace mc::network::backend::java
