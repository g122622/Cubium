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

#include "../../core/Types.hpp"
#include "AttackContext.hpp"

namespace mc {

// 前向声明
class Player;
class LivingEntity;
class Entity;

namespace item::enchant {
class KnockbackEnchantment; // 前向声明
}

namespace entity::combat {

/**
 * @brief 玩家攻击辅助类
 *
 * 提供玩家攻击相关的辅助函数，包括：
 * - 暴击判定
 * - 攻击冷却
 * - 击退计算
 * - 附魔伤害加成
 */
class PlayerAttackHelper {
public:
    // ========== 暴击判定 ==========

    /**
     * @brief 检查是否为暴击
     *
     * 暴击条件（必须全部满足）：
     * - 玩家正在下落（垂直速度 < 0，即 velocity.y < 0）
     * - 玩家不在地面
     * - 玩家不在水中
     * - 玩家不在梯子/藤蔓上
     * - 玩家没有失明效果
     * - 玩家没有骑乘
     *
     * @param player 玩家
     * @return 是否满足暴击条件
     */
    [[nodiscard]] static bool isCriticalHit(const Player& player);

    // ========== 伤害计算 ==========

    /**
     * @brief 计算攻击伤害
     *
     * 伤害计算流程：
     * 1. 获取武器基础伤害
     * 2. 应用攻击冷却影响（冷却不足时伤害降低）
     * 3. 应用力量药水加成
     * 4. 应用虚弱药水减益
     * 5. （在 AttackContext 中应用暴击加成和护甲减伤）
     *
     * @param player 玩家
     * @param baseDamage 基础伤害
     * @param cooldownProgress 攻击冷却进度 (0-1)
     * @return 计算后的伤害值
     */
    [[nodiscard]] static f32 calculateDamage(const Player& player, f32 baseDamage, f32 cooldownProgress);

    // ========== 击退计算 ==========

    /**
     * @brief 计算击退强度
     *
     * 击退计算：
     * - 基础击退 = 1.0（或武器的攻击击退属性）
     * - 疾跑加成 = +0.5
     * - 击退附魔加成 = level * 0.5（每级击退附魔）
     *
     * @param attacker 攻击者
     * @param target 目标
     * @param baseKnockback 基础击退强度
     * @param isSprinting 是否在疾跑
     * @param knockbackLevel 击退附魔等级
     * @return 击退强度
     */
    [[nodiscard]] static f32 calculateKnockback(const LivingEntity& attacker,
        const LivingEntity& target,
        f32 baseKnockback = 1.0f,
        bool isSprinting = false,
        i32 knockbackLevel = 0);

    /**
     * @brief 应用击退
     *
     * 击退处理：
     * - 击退方向：从攻击者指向目标
     * - 击退受击退抗性属性影响
     * - Y轴速度：min(0.4, 当前Y速度/2 + 击退强度) 如果在地面
     * - X/Z轴速度：当前速度/2 - 击退方向 * 击退强度
     *
     * @param target 目标
     * @param attacker 攻击者
     * @param strength 击退强度
     */
    static void applyKnockback(LivingEntity& target, const LivingEntity& attacker, f32 strength);

    /**
     * @brief 应用击退（从指定方向）
     *
     * @param target 目标
     * @param ratioX X方向比例
     * @param ratioZ Z方向比例
     * @param strength 击退强度
     */
    static void applyKnockback(LivingEntity& target, f64 ratioX, f64 ratioZ, f32 strength);

    // ========== 攻击冷却 ==========

    /**
     * @brief 应用攻击冷却影响
     *
     * MC 1.16.5: 冷却不足时伤害 = 原伤害 × cooldownProgress²
     * 只有当 cooldownProgress >= 0.9 时才造成完整伤害
     *
     * @param damage 原始伤害
     * @param cooldownProgress 攻击冷却进度 (0-1)
     * @return 调整后的伤害
     */
    [[nodiscard]] static f32 applyCooldown(f32 damage, f32 cooldownProgress);

    /**
     * @brief 检查攻击冷却是否足够
     *
     * @param cooldownProgress 攻击冷却进度 (0-1)
     * @param threshold 阈值（默认 0.9）
     * @return 是否可以造成完整伤害
     */
    [[nodiscard]] static bool isCooldownReady(f32 cooldownProgress, f32 threshold = 0.9f);

    /**
     * @brief 获取攻击冷却进度
     *
     * MC 1.16.5: cooldownProgress = ticksSinceLastAttack / (20 / attackSpeed)
     * 结果范围 [0, 1]
     *
     * @param ticksSinceLastAttack 自上次攻击以来的 tick 数
     * @param attackSpeed 攻击速度属性值
     * @return 冷却进度 (0-1)
     */
    [[nodiscard]] static f32 getCooldownProgress(i32 ticksSinceLastAttack, f32 attackSpeed);

    /**
     * @brief 将 ticks 转换为冷却进度
     *
     * @param ticksSinceLastAttack 自上次攻击以来的 tick 数
     * @param attackSpeed 攻击速度属性值
     * @return 冷却进度 (0-1)
     */
    [[nodiscard]] static f32 ticksToCooldownProgress(i32 ticksSinceLastAttack, f32 attackSpeed);

    // ========== 火焰附加 ==========

    /**
     * @brief 应用火焰附加
     *
     * MC 1.16.5: 火焰持续时间 = 80 × fireAspectLevel ticks（每级4秒）
     * 但如果目标已经有火焰，时间延长
     *
     * @param target 目标
     * @param fireAspectLevel 火焰附加等级
     * @return 是否成功应用
     */
    static bool applyFireAspect(LivingEntity& target, i32 fireAspectLevel);

    // ========== 横扫攻击 ==========

    /**
     * @brief 计算横扫攻击伤害比例
     *
     * MC 1.16.5: 横扫之刃附魔
     * - I: 50% 伤害传递
     * - II: 67% 伤害传递
     * - III: 75% 伤害传递
     *
     * @param sweepingLevel 横扫之刃等级 (0-3)
     * @return 伤害比例 (0.0-1.0)
     */
    [[nodiscard]] static f32 getSweepingDamageRatio(i32 sweepingLevel);

    // ========== 附魔伤害加成 ==========

    /**
     * @brief 计算附魔伤害加成
     *
     * 包括：锋利、亡灵杀手、节肢杀手
     *
     * @param weapon 武器物品堆
     * @param targetCreatureType 目标生物类型（用于亡灵杀手和节肢杀手）
     * @return 附加伤害值
     */
    [[nodiscard]] static f32 getEnchantmentDamageBonus(const ItemStack& weapon, CreatureAttribute targetCreatureType);

    // ========== 附魔回调 ==========

    /**
     * @brief 攻击后调用附魔回调
     *
     * 在攻击成功后调用，触发武器附魔的效果（如节肢杀手的缓慢效果）。
     * 参考 MC 1.16.5 EnchantmentHelper.applyArthropodEnchantmentDamage()
     *
     * @param attacker 攻击者
     * @param target 目标实体
     * @param weapon 攻击武器
     */
    static void applyEnchantmentEffects(LivingEntity& attacker, Entity& target, const ItemStack& weapon);

    // ========== 创建攻击上下文 ==========

    /**
     * @brief 创建攻击上下文
     *
     * @param player 攻击玩家
     * @param target 目标
     * @param cooldownProgress 攻击冷却进度
     * @return 配置好的攻击上下文
     */
    [[nodiscard]] static AttackContext createContext(Player& player, LivingEntity& target, f32 cooldownProgress);

private:
    // 常量
    static constexpr f32 CRITICAL_MULTIPLIER = 1.5f;    // 暴击伤害倍率
    static constexpr f32 SPRINT_KNOCKBACK_BONUS = 0.5f; // 疾跑击退加成
    static constexpr i32 FIRE_ASPECT_DURATION = 80;     // 火焰附加基础持续时间（4秒）
    static constexpr f32 MIN_COOLDOWN_THRESHOLD = 0.9f; // 最小冷却阈值
};

} // namespace entity::combat
} // namespace mc
