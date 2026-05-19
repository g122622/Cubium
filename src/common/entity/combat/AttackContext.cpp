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

#include "AttackContext.hpp"
#include "../../item/enchantment/EnchantmentHelper.hpp"
#include "../attribute/Attributes.hpp"
#include "../core/LivingEntity.hpp"
#include "../damage/DamageSource.hpp"
#include "../effect/EffectType.hpp"
#include "CombatRules.hpp"
#include <algorithm>
#include <cmath>

namespace mc::entity::combat {

AttackContext::AttackContext(Entity* attacker, LivingEntity* target)
    : m_attacker(attacker)
    , m_target(target)
{
    // attacker 可能为空（环境伤害）
    m_attackerLiving = dynamic_cast<LivingEntity*>(attacker);
}

f32 AttackContext::calculateFinalDamage() const
{
    f32 baseDamage = m_baseDamage;
    f32 enchantDamage = 0.0f; // 附魔伤害单独计算

    // ========== 1. 攻击者增益/减益（应用到基础伤害） ==========
    if (m_attackerLiving && m_attackType == AttackType::Melee) {
        // 力量药水加成（每级 +3 伤害）
        const i32 strengthLevel = m_attackerLiving->getEffectLevel(entity::effect::EffectType::Strength);
        if (strengthLevel > 0) {
            baseDamage += 3.0f * static_cast<f32>(strengthLevel);
        }

        // 虚弱药水减益（每级 -4 伤害）
        const i32 weaknessLevel = m_attackerLiving->getEffectLevel(entity::effect::EffectType::Weakness);
        if (weaknessLevel > 0) {
            baseDamage -= 4.0f * static_cast<f32>(weaknessLevel);
            baseDamage = std::max(0.0f, baseDamage);
        }
    }

    // ========== 2. 附魔伤害加成（从外部传入） ==========
    // 附魔伤害（锋利、亡灵杀手、节肢杀手）需要单独计算
    // 这应该在创建AttackContext时设置
    enchantDamage = m_enchantDamageBonus;

    // ========== 3. 攻击冷却影响（MC 1.16.5 关键逻辑） ==========
    // 基础伤害 × 二次冷却系数（quadratic）
    // 附魔伤害 × 线性冷却系数（linear）- 这是MC 1.16.5的关键差异！
    // 参考：PlayerEntity.attack() 中 f = f * (0.2 + f2*f2 * 0.8) 和 f1 = f1 * f2
    f32 quadraticCooldown = 0.2f + m_cooldownProgress * m_cooldownProgress * 0.8f;
    f32 linearCooldown = m_cooldownProgress;
    baseDamage *= quadraticCooldown;
    enchantDamage *= linearCooldown;

    // ========== 4. 暴击加成（只对基础伤害） ==========
    // MC 1.16.5: 暴击倍率仅应用于基础伤害，不影响附魔伤害
    // 参考：PlayerEntity.attack() 中 if (flag2) { f *= hitResult.getDamageModifier(); } 然后才是 f = f + f1
    if (m_critical) {
        baseDamage *= m_criticalMultiplier;
    }

    // ========== 5. 合并基础伤害和附魔伤害 ==========
    // MC 1.16.5: f = f + f1（基础伤害 + 附魔伤害）
    f32 damage = baseDamage + enchantDamage;

    // ========== 6. 目标护甲减伤 ==========
    if (m_target && !m_bypassArmor) {
        // 获取护甲值和护甲韧性
        f32 armor = static_cast<f32>(m_target->attributes().getValue(entity::attribute::Attributes::ARMOR));
        f32 armorToughness =
            static_cast<f32>(m_target->attributes().getValue(entity::attribute::Attributes::ARMOR_TOUGHNESS));

        // MC 1.16.5 护甲公式:
        // f = 2 + toughness / 4
        // g = clamp(armor - damage / f, armor * 0.2, 20)
        // final = damage * (1 - g / 25)
        const f32 protectionFactor = 2.0f + armorToughness / 4.0f;
        const f32 effectiveArmor = std::clamp(armor - damage / protectionFactor, armor * 0.2f, 20.0f);
        damage *= (1.0f - effectiveArmor / 25.0f);

        // 抗性药水减伤（MC 1.16.5: 每级 -20%，最高 80%）
        const i32 resistanceLevel = m_target->getEffectLevel(entity::effect::EffectType::Resistance);
        if (resistanceLevel > 0) {
            damage *= std::max(0.0f, 1.0f - 0.2f * static_cast<f32>(resistanceLevel));
        }
    }

    // ========== 7. 附魔保护减伤 ==========
    // MC 1.16.5: 计算护甲附魔的 EPF (Enchantment Protection Factor)
    // 参考: LivingEntity.applyPotionDamageCalculations() 中调用 EnchantmentHelper.getEnchantmentModifierDamage()
    if (m_target && m_damageFlags != 0) {
        // 获取护甲槽位
        auto armorSlots = m_target->getArmorSlots();

        // 计算 EPF 总和
        i32 totalEPF = item::enchant::EnchantmentHelper::getTotalArmorProtection(armorSlots, m_damageFlags);

        if (totalEPF > 0) {
            // 使用 CombatRules 计算附魔保护减伤
            // EPF 上限为 20，对应 80% 减伤
            damage = CombatRules::getDamageAfterMagicAbsorb(damage, static_cast<f32>(totalEPF));
        }
    }

    // ========== 8. 吸收值处理（金苹果）==========
    // 吸收值在 LivingEntity::actuallyHurt() 中处理，这里不重复

    return std::max(0.0f, damage);
}

std::unique_ptr<DamageSource> AttackContext::createDamageSource() const
{
    switch (m_attackType) {
        case AttackType::Melee:
            if (m_attacker) {
                return std::make_unique<EntityDamageSource>(
                    m_fireDamage ? DamageType::OnFire : DamageType::MobAttack, m_attacker);
            }
            break;

        case AttackType::Ranged:
            if (m_attacker) {
                return std::make_unique<IndirectEntityDamageSource>(DamageType::Arrow,
                    nullptr, // 直接攻击者（箭矢等）
                    m_attacker);
            }
            break;

        case AttackType::Magic:
            return std::make_unique<EnvironmentalDamage>(DamageType::Magic);

        case AttackType::Explosion:
            return std::make_unique<EnvironmentalDamage>(DamageType::Explosion);

        case AttackType::Thorns:
            return std::make_unique<EnvironmentalDamage>(DamageType::Thorns);
    }

    // 默认返回通用伤害
    return std::make_unique<EnvironmentalDamage>(DamageType::Generic);
}

void AttackContext::setDamageFlagsFromSource(const DamageSource& source)
{
    m_damageFlags = 0;

    // 根据伤害来源设置对应的标志位
    // 参考 MC 1.16.5 ProtectionEnchantment.calcModifierDamage()
    if (source.isFire()) {
        m_damageFlags |= DamageFlags::FIRE;
    }
    if (source.isFall()) {
        m_damageFlags |= DamageFlags::FALL;
    }
    if (source.isExplosion()) {
        m_damageFlags |= DamageFlags::EXPLOSION;
    }
    if (source.isProjectile()) {
        m_damageFlags |= DamageFlags::PROJECTILE;
    }

    // 如果没有任何特殊标志，则认为是通用伤害
    // 保护附魔对通用伤害有效（Type::All），EPF = level
    // 这里不需要设置任何标志，因为 ProtectionEnchantment::Type::All 对所有伤害都有效
}

} // namespace mc::entity::combat
