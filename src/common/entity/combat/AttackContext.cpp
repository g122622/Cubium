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

    // 攻击者增益/减益
    if (m_attackerLiving && m_attackType == AttackType::Melee) {
        const i32 strengthLevel = m_attackerLiving->getEffectLevel(entity::effect::EffectType::Strength);
        if (strengthLevel > 0) {
            damage += 3.0f * static_cast<f32>(strengthLevel);
        }

        const i32 weaknessLevel = m_attackerLiving->getEffectLevel(entity::effect::EffectType::Weakness);
        if (weaknessLevel > 0) {
            damage -= 4.0f * static_cast<f32>(weaknessLevel);
        }
    }

    // 暴击加成
    if (m_critical) {
        damage *= m_criticalMultiplier;
    }

    // 攻击冷却影响（玩家攻击）
    if (m_cooldownProgress < 1.0f) {
        // 冷却不足时伤害降低
        damage *= m_cooldownProgress;
    }

    // 护甲减伤（如果目标存在且伤害不绕过护甲）
    if (m_target) {
        // 获取护甲值和护甲韧性
        f32 armor = static_cast<f32>(m_target->attributes().getValue(entity::attribute::Attributes::ARMOR));
        f32 armorToughness = static_cast<f32>(m_target->attributes().getValue(entity::attribute::Attributes::ARMOR_TOUGHNESS));

        // MC 1.16.5 ��甲公式:
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

        const i32 resistanceLevel = m_target->getEffectLevel(entity::effect::EffectType::Resistance);
        if (resistanceLevel > 0) {
            damage *= std::max(0.0f, 1.0f - 0.2f * static_cast<f32>(resistanceLevel));
        }
    }

    // TODO: 附魔加成（锋利、亡灵杀手、节肢杀手）
    // TODO: 附魔保护（保护、火焰保护、摔落保护等）

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
