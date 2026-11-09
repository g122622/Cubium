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

namespace mc {

// 前向声明
class Player;
class LivingEntity;
class ItemStack;

namespace entity::combat {

/**
 * @brief 玩家攻击辅助类
 *
 * 只保留**真正被攻击管线调用**的两项能力：
 * - 暴击判定（`Player::attack` 使用）
 * - 附魔伤害加成汇总（`Player::attack` 与 `MobEntity::doHurtTarget` 使用）
 *
 * 伤害的其余环节各有权威实现，不要在本类里再写一份：
 * - 攻击冷却系数、暴击倍率、击退强度：`Player::attack` 内联
 * - 护甲/韧性/破甲减伤：`CombatRules::getDamageAfterAbsorb`
 * - 抗性药水与附魔保护：`LivingEntity::applyPotionDamageCalculations`
 * - 火焰附加：`Player::attack` 调用 `target.setRemainingFireTicks`
 */
class PlayerAttackHelper {
public:
    /**
     * @brief 判断一次攻击是否构成暴击
     *
     * 条件（全部满足）：
     * - 玩家在下落（fallDistance > 0）
     * - 玩家不在梯子/藤蔓上
     * - 玩家没有失明效果
     * - 玩家没有骑乘
     *
     * @param player 玩家
     * @return 是否满足暴击条件
     */
    [[nodiscard]] static bool isCriticalHit(const Player& player);

    /**
     * @brief 计算附魔伤害加成
     *
     * 包括：锋利、亡灵杀手、节肢杀手（通过 EnchantmentHelper::getTotalDamageBonus 委托各附魔
     * 的 getDamageBonus 虚函数汇总）。亡灵杀手/节肢杀手的目标判定用 EntityTypeTags 标签
     * （SENSITIVE_TO_SMITE / SENSITIVE_TO_BANE_OF_ARTHROPODS）。
     *
     * @param weapon 武器物品堆
     * @param target 受击目标实体（ nullptr 时亡灵/节肢杀手无目标判定返 0，锋利不受影响）
     * @return 附加伤害值
     */
    [[nodiscard]] static f32 getEnchantmentDamageBonus(const ItemStack& weapon, const LivingEntity* target);
};

} // namespace entity::combat
} // namespace mc