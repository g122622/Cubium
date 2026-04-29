#pragma once

#include "../../core/Types.hpp"

namespace mc {
namespace entity::combat {

/**
 * @brief 战斗规则工具类
 *
 * 提供伤害计算相关的公式和工具方法。
 * 所有方法都是静态的，不需要实例化。
 *
 * 参考 MC 1.16.5 net.minecraft.util.CombatRules
 */
class CombatRules {
public:
    /**
     * @brief 计算护甲减伤后的伤害
     *
     * MC 1.16.5 公式:
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
     * @brief 计算附魔保护减伤后的伤害
     *
     * MC 1.16.5 公式:
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
     * MC 1.16.5 公式:
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
    static constexpr f32 ARMOR_MAX_EFFECTIVE = 20.0f;    // 有效护甲上限
    static constexpr f32 ARMOR_MIN_RATIO = 0.2f;         // 护甲最小比例
    static constexpr f32 ARMOR_DIVISOR = 25.0f;          // 护甲减伤除数
    static constexpr f32 TOUGHNESS_FACTOR = 4.0f;        // 韧性因子
    static constexpr f32 TOUGHNESS_BASE = 2.0f;          // 韧性基数
    static constexpr f32 EPF_MAX = 20.0f;                // EPF 上限
    static constexpr f32 RESISTANCE_FACTOR = 0.2f;       // 抗性因子
    static constexpr i32 RESISTANCE_MAX_LEVEL = 5;       // 抗性最大等级
};

} // namespace entity::combat
} // namespace mc
