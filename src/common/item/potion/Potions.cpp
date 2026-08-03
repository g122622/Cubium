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

#include "Potions.hpp"
#include "common/entity/effect/EffectInstance.hpp"
#include "common/entity/effect/EffectType.hpp"
#include "common/item/potion/Potion.hpp"
#include "common/item/potion/PotionRegistry.hpp"
#include "common/resource/ResourceLocation.hpp"
#include <utility>

namespace mc {
namespace potion {

// ========== 静态成员初始化 ==========

bool Potions::s_initialized = false;

const Potion* Potions::EMPTY = nullptr;
const Potion* Potions::WATER = nullptr;
const Potion* Potions::MUNDANE = nullptr;
const Potion* Potions::THICK = nullptr;
const Potion* Potions::AWKWARD = nullptr;

const Potion* Potions::NIGHT_VISION = nullptr;
const Potion* Potions::LONG_NIGHT_VISION = nullptr;

const Potion* Potions::INVISIBILITY = nullptr;
const Potion* Potions::LONG_INVISIBILITY = nullptr;

const Potion* Potions::LEAPING = nullptr;
const Potion* Potions::LONG_LEAPING = nullptr;
const Potion* Potions::STRONG_LEAPING = nullptr;

const Potion* Potions::FIRE_RESISTANCE = nullptr;
const Potion* Potions::LONG_FIRE_RESISTANCE = nullptr;

const Potion* Potions::SWIFTNESS = nullptr;
const Potion* Potions::LONG_SWIFTNESS = nullptr;
const Potion* Potions::STRONG_SWIFTNESS = nullptr;

const Potion* Potions::SLOWNESS = nullptr;
const Potion* Potions::LONG_SLOWNESS = nullptr;
const Potion* Potions::STRONG_SLOWNESS = nullptr;

const Potion* Potions::TURTLE_MASTER = nullptr;
const Potion* Potions::LONG_TURTLE_MASTER = nullptr;
const Potion* Potions::STRONG_TURTLE_MASTER = nullptr;

const Potion* Potions::WATER_BREATHING = nullptr;
const Potion* Potions::LONG_WATER_BREATHING = nullptr;

const Potion* Potions::HEALING = nullptr;
const Potion* Potions::STRONG_HEALING = nullptr;

const Potion* Potions::HARMING = nullptr;
const Potion* Potions::STRONG_HARMING = nullptr;

const Potion* Potions::POISON = nullptr;
const Potion* Potions::LONG_POISON = nullptr;
const Potion* Potions::STRONG_POISON = nullptr;

const Potion* Potions::REGENERATION = nullptr;
const Potion* Potions::LONG_REGENERATION = nullptr;
const Potion* Potions::STRONG_REGENERATION = nullptr;

const Potion* Potions::STRENGTH = nullptr;
const Potion* Potions::LONG_STRENGTH = nullptr;
const Potion* Potions::STRONG_STRENGTH = nullptr;

const Potion* Potions::WEAKNESS = nullptr;
const Potion* Potions::LONG_WEAKNESS = nullptr;

const Potion* Potions::LUCK = nullptr;

const Potion* Potions::SLOW_FALLING = nullptr;
const Potion* Potions::LONG_SLOW_FALLING = nullptr;

// ========== 辅助函数 ==========

const Potion* Potions::_registerPotion(const char* name, Potion potion)
{
    auto& registry = PotionRegistry::instance();
    return registry.registerPotion(ResourceLocation("minecraft", name), std::move(potion));
}

// ========== 初始化 ==========

void Potions::initialize() noexcept
{
    if (s_initialized) {
        return;
    }
    s_initialized = true;

    auto& registry = PotionRegistry::instance();

    // 基础药水（无效果）
    EMPTY = _registerPotion("empty", Potion());
    WATER = _registerPotion("water", Potion());
    MUNDANE = _registerPotion("mundane", Potion());
    THICK = _registerPotion("thick", Potion());
    AWKWARD = _registerPotion("awkward", Potion());

    // 夜视药水
    // 夜视 (3:00 = 3600 tick)
    NIGHT_VISION = _registerPotion(
        "night_vision", Potion("", {entity::effect::EffectInstance(entity::effect::EffectType::NightVision, 3600)}));
    // 夜视延长 (8:00 = 9600 tick)
    LONG_NIGHT_VISION = _registerPotion("long_night_vision",
        Potion("night_vision", {entity::effect::EffectInstance(entity::effect::EffectType::NightVision, 9600)}));

    // 隐身药水
    INVISIBILITY = _registerPotion(
        "invisibility", Potion("", {entity::effect::EffectInstance(entity::effect::EffectType::Invisibility, 3600)}));
    LONG_INVISIBILITY = _registerPotion("long_invisibility",
        Potion("invisibility", {entity::effect::EffectInstance(entity::effect::EffectType::Invisibility, 9600)}));

    // 跳跃提升药水
    LEAPING = _registerPotion(
        "leaping", Potion("", {entity::effect::EffectInstance(entity::effect::EffectType::JumpBoost, 3600)}));
    LONG_LEAPING = _registerPotion("long_leaping",
        Potion("leaping", {entity::effect::EffectInstance(entity::effect::EffectType::JumpBoost, 9600)}));
    STRONG_LEAPING = _registerPotion("strong_leaping",
        Potion("leaping", {entity::effect::EffectInstance(entity::effect::EffectType::JumpBoost, 1800, 1)}));

    // 防火药水
    FIRE_RESISTANCE = _registerPotion("fire_resistance",
        Potion("", {entity::effect::EffectInstance(entity::effect::EffectType::FireResistance, 3600)}));
    LONG_FIRE_RESISTANCE = _registerPotion("long_fire_resistance",
        Potion("fire_resistance", {entity::effect::EffectInstance(entity::effect::EffectType::FireResistance, 9600)}));

    // 速度药水
    SWIFTNESS = _registerPotion(
        "swiftness", Potion("", {entity::effect::EffectInstance(entity::effect::EffectType::Speed, 3600)}));
    LONG_SWIFTNESS = _registerPotion("long_swiftness",
        Potion("swiftness", {entity::effect::EffectInstance(entity::effect::EffectType::Speed, 9600)}));
    STRONG_SWIFTNESS = _registerPotion("strong_swiftness",
        Potion("swiftness", {entity::effect::EffectInstance(entity::effect::EffectType::Speed, 1800, 1)}));

    // 缓慢药水
    SLOWNESS = _registerPotion(
        "slowness", Potion("", {entity::effect::EffectInstance(entity::effect::EffectType::Slowness, 1800)}));
    LONG_SLOWNESS = _registerPotion("long_slowness",
        Potion("slowness", {entity::effect::EffectInstance(entity::effect::EffectType::Slowness, 4800)}));
    STRONG_SLOWNESS = _registerPotion("strong_slowness",
        Potion("slowness", {entity::effect::EffectInstance(entity::effect::EffectType::Slowness, 400, 3)}));

    // 海龟大师药水 (缓慢 IV + 抗性提升 III)
    TURTLE_MASTER = _registerPotion("turtle_master",
        Potion("turtle_master",
            {entity::effect::EffectInstance(entity::effect::EffectType::Slowness, 400, 3),
                entity::effect::EffectInstance(entity::effect::EffectType::Resistance, 400, 2)}));
    LONG_TURTLE_MASTER = _registerPotion("long_turtle_master",
        Potion("turtle_master",
            {entity::effect::EffectInstance(entity::effect::EffectType::Slowness, 800, 3),
                entity::effect::EffectInstance(entity::effect::EffectType::Resistance, 800, 2)}));
    STRONG_TURTLE_MASTER = _registerPotion("strong_turtle_master",
        Potion("turtle_master",
            {entity::effect::EffectInstance(entity::effect::EffectType::Slowness, 400, 5),
                entity::effect::EffectInstance(entity::effect::EffectType::Resistance, 400, 3)}));

    // 水下呼吸药水
    WATER_BREATHING = _registerPotion("water_breathing",
        Potion("", {entity::effect::EffectInstance(entity::effect::EffectType::WaterBreathing, 3600)}));
    LONG_WATER_BREATHING = _registerPotion("long_water_breathing",
        Potion("water_breathing", {entity::effect::EffectInstance(entity::effect::EffectType::WaterBreathing, 9600)}));

    // 瞬间治疗药水
    HEALING = _registerPotion(
        "healing", Potion("", {entity::effect::EffectInstance(entity::effect::EffectType::InstantHealth, 1)}));
    STRONG_HEALING = _registerPotion("strong_healing",
        Potion("healing", {entity::effect::EffectInstance(entity::effect::EffectType::InstantHealth, 1, 1)}));

    // 瞬间伤害药水
    HARMING = _registerPotion(
        "harming", Potion("", {entity::effect::EffectInstance(entity::effect::EffectType::InstantDamage, 1)}));
    STRONG_HARMING = _registerPotion("strong_harming",
        Potion("harming", {entity::effect::EffectInstance(entity::effect::EffectType::InstantDamage, 1, 1)}));

    // 中毒药水
    POISON = _registerPotion(
        "poison", Potion("", {entity::effect::EffectInstance(entity::effect::EffectType::Poison, 900)}));
    LONG_POISON = _registerPotion(
        "long_poison", Potion("poison", {entity::effect::EffectInstance(entity::effect::EffectType::Poison, 1800)}));
    STRONG_POISON = _registerPotion("strong_poison",
        Potion("poison", {entity::effect::EffectInstance(entity::effect::EffectType::Poison, 432, 1)}));

    // 生命恢复药水
    REGENERATION = _registerPotion(
        "regeneration", Potion("", {entity::effect::EffectInstance(entity::effect::EffectType::Regeneration, 900)}));
    LONG_REGENERATION = _registerPotion("long_regeneration",
        Potion("regeneration", {entity::effect::EffectInstance(entity::effect::EffectType::Regeneration, 1800)}));
    STRONG_REGENERATION = _registerPotion("strong_regeneration",
        Potion("regeneration", {entity::effect::EffectInstance(entity::effect::EffectType::Regeneration, 450, 1)}));

    // 力量药水
    STRENGTH = _registerPotion(
        "strength", Potion("", {entity::effect::EffectInstance(entity::effect::EffectType::Strength, 3600)}));
    LONG_STRENGTH = _registerPotion("long_strength",
        Potion("strength", {entity::effect::EffectInstance(entity::effect::EffectType::Strength, 9600)}));
    STRONG_STRENGTH = _registerPotion("strong_strength",
        Potion("strength", {entity::effect::EffectInstance(entity::effect::EffectType::Strength, 1800, 1)}));

    // 虚弱药水
    WEAKNESS = _registerPotion(
        "weakness", Potion("", {entity::effect::EffectInstance(entity::effect::EffectType::Weakness, 1800)}));
    LONG_WEAKNESS = _registerPotion("long_weakness",
        Potion("weakness", {entity::effect::EffectInstance(entity::effect::EffectType::Weakness, 4800)}));

    // 幸运药水
    LUCK = _registerPotion(
        "luck", Potion("luck", {entity::effect::EffectInstance(entity::effect::EffectType::Luck, 6000)}));

    // 缓降药水
    SLOW_FALLING = _registerPotion(
        "slow_falling", Potion("", {entity::effect::EffectInstance(entity::effect::EffectType::SlowFalling, 1800)}));
    LONG_SLOW_FALLING = _registerPotion("long_slow_falling",
        Potion("slow_falling", {entity::effect::EffectInstance(entity::effect::EffectType::SlowFalling, 4800)}));

    // 同步注册表指针
    PotionRegistry::EMPTY = EMPTY;
    PotionRegistry::WATER = WATER;
    PotionRegistry::MUNDANE = MUNDANE;
    PotionRegistry::THICK = THICK;
    PotionRegistry::AWKWARD = AWKWARD;

    PotionRegistry::NIGHT_VISION = NIGHT_VISION;
    PotionRegistry::LONG_NIGHT_VISION = LONG_NIGHT_VISION;

    PotionRegistry::INVISIBILITY = INVISIBILITY;
    PotionRegistry::LONG_INVISIBILITY = LONG_INVISIBILITY;

    PotionRegistry::LEAPING = LEAPING;
    PotionRegistry::LONG_LEAPING = LONG_LEAPING;
    PotionRegistry::STRONG_LEAPING = STRONG_LEAPING;

    PotionRegistry::FIRE_RESISTANCE = FIRE_RESISTANCE;
    PotionRegistry::LONG_FIRE_RESISTANCE = LONG_FIRE_RESISTANCE;

    PotionRegistry::SWIFTNESS = SWIFTNESS;
    PotionRegistry::LONG_SWIFTNESS = LONG_SWIFTNESS;
    PotionRegistry::STRONG_SWIFTNESS = STRONG_SWIFTNESS;

    PotionRegistry::SLOWNESS = SLOWNESS;
    PotionRegistry::LONG_SLOWNESS = LONG_SLOWNESS;
    PotionRegistry::STRONG_SLOWNESS = STRONG_SLOWNESS;

    PotionRegistry::TURTLE_MASTER = TURTLE_MASTER;
    PotionRegistry::LONG_TURTLE_MASTER = LONG_TURTLE_MASTER;
    PotionRegistry::STRONG_TURTLE_MASTER = STRONG_TURTLE_MASTER;

    PotionRegistry::WATER_BREATHING = WATER_BREATHING;
    PotionRegistry::LONG_WATER_BREATHING = LONG_WATER_BREATHING;

    PotionRegistry::HEALING = HEALING;
    PotionRegistry::STRONG_HEALING = STRONG_HEALING;

    PotionRegistry::HARMING = HARMING;
    PotionRegistry::STRONG_HARMING = STRONG_HARMING;

    PotionRegistry::POISON = POISON;
    PotionRegistry::LONG_POISON = LONG_POISON;
    PotionRegistry::STRONG_POISON = STRONG_POISON;

    PotionRegistry::REGENERATION = REGENERATION;
    PotionRegistry::LONG_REGENERATION = LONG_REGENERATION;
    PotionRegistry::STRONG_REGENERATION = STRONG_REGENERATION;

    PotionRegistry::STRENGTH = STRENGTH;
    PotionRegistry::LONG_STRENGTH = LONG_STRENGTH;
    PotionRegistry::STRONG_STRENGTH = STRONG_STRENGTH;

    PotionRegistry::WEAKNESS = WEAKNESS;
    PotionRegistry::LONG_WEAKNESS = LONG_WEAKNESS;

    PotionRegistry::LUCK = LUCK;

    PotionRegistry::SLOW_FALLING = SLOW_FALLING;
    PotionRegistry::LONG_SLOW_FALLING = LONG_SLOW_FALLING;
}

} // namespace potion
} // namespace mc
