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

#pragma once

#include "common/core/Types.hpp"
#include <utility>

namespace mc {
namespace entity::combat {

/**
 * @brief 战斗规则工具类
 *
 * 提供伤害计算相关的公式和工具方法。
 * 所有方法都是静态的，不需要实例化。
 */
class CombatRules {
public:
    /**
     * @brief 计算护甲减伤后的伤害
     *
     * 公式:
     * f = 2 + toughness / 4
     * g = clamp(armor - damage / f, armor * 0.2, 20)
     * final = damage * (1 - g / 25)
     *
     * 护甲减伤上限为 80%（当 effectiveArmor = 20 时）
     *
     * @param damage 原始伤害
     * @param totalArmor 总护甲值 (0-20，超过20也会被clamp)
     * @param toughness 护甲韧性 (0-20)
     * @return 减伤后的伤害
     */
    [[nodiscard]] static f32 getDamageAfterAbsorb(f32 damage, f32 totalArmor, f32 toughness);

    /**
     * @brief 计算护甲减伤后的伤害（含破甲 Breach 附魔修正）
     *
     * 对齐 vanilla CombatRules.getDamageAfterArmor（CombatRules.java:16-30）：
     *   f  = 2 + toughness / 4
     *   f1 = clamp(armor - damage / f, armor * 0.2, 20)   // effectiveArmor
     *   f2 = f1 / 25                                       // armorRatio
     *   f3 = clamp(f2 + breachModifier, 0, 1)              // Breach 修正（每级 -0.15）
     *   final = damage * (1 - f3)
     *
     * Breach 修正作用于 armorRatio（非 effectiveArmor），结果 clamp 到 [0, 1]。
     * breachLevel <= 0 时等价于三参数重载（无修正）。
     *
     * @param damage 原始伤害
     * @param totalArmor 总护甲值
     * @param toughness 护甲韧性
     * @param breachLevel 攻击者武器的破甲附魔等级（0 表示无破甲）
     * @return 减伤后的伤害
     */
    [[nodiscard]] static f32 getDamageAfterAbsorb(f32 damage, f32 totalArmor, f32 toughness, i32 breachLevel);

    /**
     * @brief 计算附魔保护减伤后的伤害
     *
     * 公式:
     * f = clamp(epf, 0, 20)
     * final = damage * (1 - f / 25)
     *
     * 附魔保护减伤上限为 80%（当 EPF = 20 时）
     *
     * @param damage 原始伤害
     * @param enchantmentProtectionFactor 附魔保护因子总和
     * @return 减伤后的伤害
     */
    [[nodiscard]] static f32 getDamageAfterMagicAbsorb(f32 damage, f32 enchantmentProtectionFactor);

    /**
     * @brief 计算抗性药水减伤后的伤害
     *
     * 公式:
     * final = damage * max(0, 1 - level * 0.2)
     *
     * 抗性药水减伤上限为 80%（抗性 V）
     *
     * @param damage 原始伤害
     * @param resistanceLevel 抗性药水等级 (0-5)
     * @return 减伤后的伤害
     */
    [[nodiscard]] static f32 getDamageAfterResistance(f32 damage, i32 resistanceLevel);

    /**
     * @brief 计算吸收值消耗后的实际伤害
     *
     * @param damage 原始伤害
     * @param absorption 当前的吸收值
     * @return pair<吸收值消耗, 剩余伤害>
     */
    [[nodiscard]] static std::pair<f32, f32> applyAbsorption(f32 damage, f32 absorption);

private:
    CombatRules() = delete;
    ~CombatRules() = delete;

    // 常量
    static constexpr f32 ARMOR_MAX_EFFECTIVE = 20.0f; // 有效护甲上限
    static constexpr f32 ARMOR_MIN_RATIO = 0.2f;      // 护甲最小比例
    static constexpr f32 ARMOR_DIVISOR = 25.0f;       // 护甲减伤除数
    static constexpr f32 TOUGHNESS_FACTOR = 4.0f;     // 韧性因子
    static constexpr f32 TOUGHNESS_BASE = 2.0f;       // 韧性基数
    static constexpr f32 EPF_MAX = 20.0f;             // EPF 上限
    static constexpr f32 RESISTANCE_FACTOR = 0.2f;    // 抗性因子
    static constexpr i32 RESISTANCE_MAX_LEVEL = 5;    // 抗性最大等级
};

} // namespace entity::combat
} // namespace mc
