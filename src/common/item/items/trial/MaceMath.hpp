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
 * THE SOFTWARE IS PROVIDED "AS IS", WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
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
namespace item {

/**
 * @brief 重锤下落攻击的纯计算函数和常量
 *
 * 此头文件不依赖实体系统，可独立包含和测试。
 * MaceItem 类通过调用这些函数实现下落攻击逻辑。
 */
namespace MaceMath {

/// 触发砸地攻击的最低下落距离
constexpr f32 SMASH_ATTACK_FALL_THRESHOLD = 1.5f;

/// 重砸判定阈值（超过此下落距离为重击，击退力翻倍）
constexpr f32 SMASH_ATTACK_HEAVY_THRESHOLD = 5.0f;

/// 击退范围（格）
constexpr f32 SMASH_ATTACK_KNOCKBACK_RADIUS = 3.5f;

/// 击退基础力度
constexpr f32 SMASH_ATTACK_KNOCKBACK_POWER = 0.7f;

/// 重锤基础攻击伤害
constexpr f32 DEFAULT_ATTACK_DAMAGE = 5.0f;

/// 重锤攻击速度修正
constexpr f32 DEFAULT_ATTACK_SPEED = -3.4f;

/// 重锤最大耐久度
constexpr i32 MAX_DURABILITY = 250;

/**
 * @brief 计算下落攻击的基础伤害加成（不含魔咒）
 *
 * 分段函数：
 * - fallDistance <= 0: 0
 * - fallDistance <= 3: 4.0 * fallDistance
 * - fallDistance <= 8: 12.0 + 2.0 * (fallDistance - 3.0)
 * - fallDistance > 8:  22.0 + 1.0 * (fallDistance - 8.0)
 *
 * @param fallDistance 下落距离
 * @return 基础伤害加成
 */
[[nodiscard]] inline f32 calculateSmashAttackDamage(f32 fallDistance) noexcept
{
    if (fallDistance <= 0.0f) {
        return 0.0f;
    }
    if (fallDistance <= 3.0f) {
        return 4.0f * fallDistance;
    }
    if (fallDistance <= 8.0f) {
        return 12.0f + 2.0f * (fallDistance - 3.0f);
    }
    return 22.0f + (fallDistance - 8.0f);
}

/**
 * @brief 检查是否满足砸地攻击的下落距离条件
 *
 * 仅检查下落距离，不检查鞘翅滑翔状态（需要实体上下文）。
 * 完整判断请使用 MaceItem::canSmashAttack()。
 *
 * @param fallDistance 下落距离
 * @return 是否满足下落距离条件
 */
[[nodiscard]] inline bool isSmashAttackFallDistance(f32 fallDistance) noexcept
{
    return fallDistance > SMASH_ATTACK_FALL_THRESHOLD;
}

/**
 * @brief 检查是否为重砸（下落距离超过重击阈值）
 *
 * @param fallDistance 下落距离
 * @return 是否为重砸
 */
[[nodiscard]] inline bool isHeavySmashAttack(f32 fallDistance) noexcept
{
    return fallDistance > SMASH_ATTACK_HEAVY_THRESHOLD;
}

/**
 * @brief 计算砸地攻击对某实体的击退力度
 *
 * @param distanceToTarget 与目标的水平距离
 * @param fallDistance 攻击者的下落距离
 * @param knockbackResistance 被击退实体的击退抗性（0~1）
 * @return 击退力度，<=0 表示无击退
 */
[[nodiscard]] inline f32 calculateSmashAttackKnockbackPower(
    f32 distanceToTarget, f32 fallDistance, f32 knockbackResistance) noexcept
{
    if (distanceToTarget >= SMASH_ATTACK_KNOCKBACK_RADIUS) {
        return 0.0f;
    }
    f32 power = (SMASH_ATTACK_KNOCKBACK_RADIUS - distanceToTarget) * SMASH_ATTACK_KNOCKBACK_POWER;
    if (isHeavySmashAttack(fallDistance)) {
        power *= 2.0f;
    }
    power *= (1.0f - knockbackResistance);
    return power > 0.0f ? power : 0.0f;
}

} // namespace MaceMath

} // namespace item
} // namespace mc
