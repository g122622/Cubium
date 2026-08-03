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

#include "CombatRules.hpp"
#include "common/core/Types.hpp"
#include <algorithm>
#include <utility>

namespace mc::entity::combat {

f32 CombatRules::getDamageAfterAbsorb(f32 damage, f32 totalArmor, f32 toughness)
{
    if (damage <= 0.0f) {
        return 0.0f;
    }

    // 计算护甲减伤公式：
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

    // 附魔保护减伤公式：
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

    // 抗性药水减伤公式：
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
