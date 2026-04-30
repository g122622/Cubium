#include "AttackContext.hpp"
#include "../damage/DamageSource.hpp"
#include "../attribute/Attributes.hpp"
#include "../effect/EffectType.hpp"
#include "../core/LivingEntity.hpp"
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

f32 AttackContext::calculateFinalDamage() const {
    f32 damage = m_baseDamage;

    // ========== 1. 攻击者增益/减益 ==========
    if (m_attackerLiving && m_attackType == AttackType::Melee) {
        // 力量药水加成（每级 +3 伤害）
        const i32 strengthLevel = m_attackerLiving->getEffectLevel(entity::effect::EffectType::Strength);
        if (strengthLevel > 0) {
            damage += 3.0f * static_cast<f32>(strengthLevel);
        }

        // 虚弱药水减益（每级 -4 伤害）
        const i32 weaknessLevel = m_attackerLiving->getEffectLevel(entity::effect::EffectType::Weakness);
        if (weaknessLevel > 0) {
            damage -= 4.0f * static_cast<f32>(weaknessLevel);
            damage = std::max(0.0f, damage);
        }
    }

    // ========== 2. 暴击加成 ==========
    // MC 1.16.5: 暴击倍率在冷却影响之前应用
    if (m_critical) {
        damage *= m_criticalMultiplier;
    }

    // ========== 3. 攻击冷却影响 ==========
    // MC 1.16.5: 冷却不足时伤害 = 原伤害 × cooldownProgress²
    // 注意：只有冷却 >= 0.9 才能造成完整伤害
    if (m_cooldownProgress < 1.0f) {
        damage *= m_cooldownProgress * m_cooldownProgress;
    }

    // ========== 4. 目标护甲减伤 ==========
    if (m_target && !m_bypassArmor) {
        // 获取护甲值和护甲韧性
        f32 armor = static_cast<f32>(m_target->attributes().getValue(entity::attribute::Attributes::ARMOR));
        f32 armorToughness = static_cast<f32>(m_target->attributes().getValue(entity::attribute::Attributes::ARMOR_TOUGHNESS));

        // MC 1.16.5 护甲公式:
        // f = 2 + toughness / 4
        // g = clamp(armor - damage / f, armor * 0.2, 20)
        // final = damage * (1 - g / 25)
        const f32 protectionFactor = 2.0f + armorToughness / 4.0f;
        const f32 effectiveArmor = std::clamp(
            armor - damage / protectionFactor,
            armor * 0.2f,
            20.0f
        );
        damage *= (1.0f - effectiveArmor / 25.0f);

        // 抗性药水减伤（MC 1.16.5: 每级 -20%，最高 80%）
        const i32 resistanceLevel = m_target->getEffectLevel(entity::effect::EffectType::Resistance);
        if (resistanceLevel > 0) {
            damage *= std::max(0.0f, 1.0f - 0.2f * static_cast<f32>(resistanceLevel));
        }
    }

    // ========== 5. 附魔保护减伤 ==========
    // TODO: 附魔保护减伤（保护、火焰保护、摔落保护等）
    // 需要在目标装备上计算 EPF (Enchantment Protection Factor)

    // ========== 6. 吸收值处理（金苹果）==========
    // 吸收值在 LivingEntity::actuallyHurt() 中处理，这里不重复

    return std::max(0.0f, damage);
}

std::unique_ptr<DamageSource> AttackContext::createDamageSource() const {
    switch (m_attackType) {
        case AttackType::Melee:
            if (m_attacker) {
                return std::make_unique<EntityDamageSource>(
                    m_fireDamage ? DamageType::OnFire : DamageType::MobAttack,
                    m_attacker
                );
            }
            break;

        case AttackType::Ranged:
            if (m_attacker) {
                return std::make_unique<IndirectEntityDamageSource>(
                    DamageType::Arrow,
                    nullptr,  // 直接攻击者（箭矢等）
                    m_attacker
                );
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

} // namespace mc::entity::combat
