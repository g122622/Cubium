#include "CombatRules.hpp"
#include <algorithm>

namespace mc::entity::combat {

f32 CombatRules::getDamageAfterAbsorb(f32 damage, f32 totalArmor, f32 toughness)
{
    if (damage <= 0.0f) {
        return 0.0f;
    }

    // MC 1.16.5 CombatRules.getDamageAfterAbsorb()
    // f = 2 + toughness / 4
    // g = clamp(armor - damage / f, armor * 0.2, 20)
    // final = damage * (1 - g / 25)

    const f32 protectionFactor = TOUGHNESS_BASE + toughness / TOUGHNESS_FACTOR;

    const f32 minArmor = totalArmor * ARMOR_MIN_RATIO;
    const f32 effectiveArmor = std::clamp(totalArmor - damage / protectionFactor, minArmor, ARMOR_MAX_EFFECTIVE);

    // 减伤比例 = effectiveArmor / 25
    // 最终伤害 = damage * (1 - 减伤比例)
    return damage * (1.0f - effectiveArmor / ARMOR_DIVISOR);
}

f32 CombatRules::getDamageAfterMagicAbsorb(f32 damage, f32 enchantmentProtectionFactor)
{
    if (damage <= 0.0f) {
        return 0.0f;
    }

    // MC 1.16.5 CombatRules.getDamageAfterMagicAbsorb()
    // f = clamp(epf, 0, 20)
    // final = damage * (1 - f / 25)

    const f32 clampedEPF = std::clamp(enchantmentProtectionFactor, 0.0f, EPF_MAX);

    // 减伤比例 = EPF / 25
    // 最大减伤 80% (当 EPF = 20 时)
    return damage * (1.0f - clampedEPF / ARMOR_DIVISOR);
}

f32 CombatRules::getDamageAfterResistance(f32 damage, i32 resistanceLevel)
{
    if (damage <= 0.0f || resistanceLevel <= 0) {
        return damage;
    }

    // MC 1.16.5 抗性药水减伤
    // final = damage * max(0, 1 - level * 0.2)
    // 抗性 V = 80% 减伤

    const i32 effectiveLevel = std::min(resistanceLevel, RESISTANCE_MAX_LEVEL);
    const f32 reduction = static_cast<f32>(effectiveLevel) * RESISTANCE_FACTOR;

    return damage * std::max(0.0f, 1.0f - reduction);
}

std::pair<f32, f32> CombatRules::applyAbsorption(f32 damage, f32 absorption)
{
    if (damage <= 0.0f || absorption <= 0.0f) {
        return {0.0f, damage};
    }

    // 吸收值优先消耗
    const f32 absorbed = std::min(absorption, damage);
    const f32 remaining = damage - absorbed;

    return {absorbed, remaining};
}

} // namespace mc::entity::combat
