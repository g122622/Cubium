#include "AttackContext.hpp"
#include "../damage/DamageSource.hpp"
#include "../attribute/Attributes.hpp"
#include "../core/LivingEntity.hpp"
#include <cmath>

namespace mc::entity::combat {

AttackContext::AttackContext(Entity* attacker, LivingEntity* target)
    : m_attacker(attacker)
    , m_target(target)
{
    // attacker 可能为空（环境伤害）
}

f32 AttackContext::calculateFinalDamage() const {
    f32 damage = m_baseDamage;

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
        f32 armor = m_target->attributes().getValue(entity::attribute::Attributes::ARMOR);
        f32 armorToughness = m_target->attributes().getValue(entity::attribute::Attributes::ARMOR_TOUGHNESS);

        // MC 1.16.5 护甲减伤公式:
        // damageReduction = armor / (5 + armor / 4)
        // 但不能超过 armor / 25（最大 80% 减伤）
        // 同时受护甲韧性影响，减少护甲穿透效果

        // 计算护甲减伤比例
        f32 armorReduction = armor / 25.0f;  // 基础减伤：每点护甲减伤 4%
        armorReduction = std::min(armorReduction, 0.8f);  // 最大 80% 减伤

        // 应用韧性对高伤害的减伤保护
        // 当伤害高于护甲韧性时，护甲效果会降低
        // damageAfterArmor = damage * (1 - armorReduction * (1 - max(0, damage - armorToughness / 4) / damage))

        f32 effectiveReduction = armorReduction;

        // 计算实际减伤
        damage *= (1.0f - effectiveReduction);
    }

    // TODO: 附魔加成（锋利、亡灵杀手、节肢杀手）
    // TODO: 药水效果加成（力量、虚弱）
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
