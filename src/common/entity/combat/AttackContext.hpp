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
#include "../../entity/damage/DamageSource.hpp"
#include <memory>

namespace mc {

// 前向声明
class Entity;
class LivingEntity;
class Player;
class ItemStack;

namespace entity::combat {

/**
 * @brief 攻击类型
 *
 * 参考 MC 1.16.5 攻击类型
 */
enum class AttackType : u8 {
    Melee,     // 近战攻击
    Ranged,    // 远程攻击
    Magic,     // 魔法攻击
    Explosion, // 爆炸攻击
    Thorns     // 荆棘反伤
};

/**
 * @brief 攻击上下文
 *
 * 包含攻击相关的所有信息，用于计算伤害和效果。
 *
 * 参考 MC 1.16.5 攻击相关逻辑
 */
class AttackContext {
public:
    /**
     * @brief 构造函数
     * @param attacker 攻击者（可能为空，如环境伤害）
     * @param target 目标实体
     */
    AttackContext(Entity* attacker, LivingEntity* target);
    ~AttackContext() = default;

    // ========== 攻击者信息 ==========

    [[nodiscard]] Entity* getAttacker() const { return m_attacker; }
    [[nodiscard]] Player* getAttackerAsPlayer() const { return m_attackerPlayer; }
    [[nodiscard]] LivingEntity* getAttackerAsLiving() const { return m_attackerLiving; }
    [[nodiscard]] const ItemStack* getWeapon() const { return m_weapon; }

    // ========== 目标信息 ==========

    [[nodiscard]] LivingEntity* getTarget() const { return m_target; }

    // ========== 攻击属性 ==========

    [[nodiscard]] f32 getBaseDamage() const { return m_baseDamage; }
    void setBaseDamage(f32 damage) { m_baseDamage = damage; }

    [[nodiscard]] f32 getEnchantDamageBonus() const { return m_enchantDamageBonus; }
    void setEnchantDamageBonus(f32 bonus) { m_enchantDamageBonus = bonus; }

    [[nodiscard]] AttackType getAttackType() const { return m_attackType; }
    void setAttackType(AttackType type) { m_attackType = type; }

    // ========== 攻击修饰 ==========

    [[nodiscard]] bool isCritical() const { return m_critical; }
    void setCritical(bool critical) { m_critical = critical; }

    [[nodiscard]] f32 getCriticalMultiplier() const { return m_criticalMultiplier; }
    void setCriticalMultiplier(f32 multiplier) { m_criticalMultiplier = multiplier; }

    [[nodiscard]] bool causesKnockback() const { return m_knockback; }
    void setKnockback(bool knockback) { m_knockback = knockback; }

    [[nodiscard]] f32 getKnockbackStrength() const { return m_knockbackStrength; }
    void setKnockbackStrength(f32 strength) { m_knockbackStrength = strength; }

    [[nodiscard]] bool causesFireDamage() const { return m_fireDamage; }
    void setFireDamage(bool fire) { m_fireDamage = fire; }

    [[nodiscard]] i32 getFireDuration() const { return m_fireDuration; }
    void setFireDuration(i32 duration) { m_fireDuration = duration; }

    // ========== 伤害计算 ==========

    [[nodiscard]] f32 calculateFinalDamage() const;
    [[nodiscard]] std::unique_ptr<DamageSource> createDamageSource() const;

    // ========== 攻击冷却 ==========

    [[nodiscard]] f32 getCooldownProgress() const { return m_cooldownProgress; }
    void setCooldownProgress(f32 progress) { m_cooldownProgress = progress; }

    // ========== 护甲穿透 ==========

    [[nodiscard]] bool bypassesArmor() const { return m_bypassArmor; }
    void setBypassArmor(bool bypass) { m_bypassArmor = bypass; }

    // ========== 伤害类型标志位（用于保护附魔计算） ==========

    /**
     * @brief 获取伤害类型标志位
     *
     * 用于保护附魔计算，由 DamageFlags 定义。
     */
    [[nodiscard]] u32 getDamageFlags() const { return m_damageFlags; }

    /**
     * @brief 设置伤害类型标志位
     * @param flags 伤害类型标志位
     */
    void setDamageFlags(u32 flags) { m_damageFlags = flags; }

    /**
     * @brief 从伤害来源提取伤害类型标志位
     * @param source 伤害来源
     */
    void setDamageFlagsFromSource(const DamageSource& source);

private:
    Entity* m_attacker = nullptr;
    Player* m_attackerPlayer = nullptr;
    LivingEntity* m_attackerLiving = nullptr;
    LivingEntity* m_target = nullptr;
    const ItemStack* m_weapon = nullptr;

    f32 m_baseDamage = 1.0f;
    f32 m_enchantDamageBonus = 0.0f; // 附魔伤害加成（锋利、亡灵杀手、节肢杀手）
    AttackType m_attackType = AttackType::Melee;

    bool m_critical = false;
    f32 m_criticalMultiplier = 1.5f;
    bool m_knockback = true;
    f32 m_knockbackStrength = 1.0f;
    bool m_fireDamage = false;
    i32 m_fireDuration = 0;
    bool m_bypassArmor = false;

    f32 m_cooldownProgress = 1.0f;
    u32 m_damageFlags = 0; // 伤害类型标志位，用于保护附魔计算
};

} // namespace entity::combat
} // namespace mc
