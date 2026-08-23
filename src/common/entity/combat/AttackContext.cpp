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
#include "../../item/enchantment/enchantments/mace/BreachEnchantment.hpp"
#include "../attribute/Attributes.hpp"
#include "../core/LivingEntity.hpp"
#include "../damage/DamageSource.hpp"
#include "CombatRules.hpp"
#include "common/entity/core/DataParameter.hpp"
#include "common/entity/effect/EffectType.hpp"
#include <algorithm>
#include <memory>

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

    // 力量/虚弱药水效果已通过属性修改器系统自动应用到 ATTACK_DAMAGE 属性值中，
    // baseDamage 已包含这些修改，不需要在此手动计算。
    // 参见 EffectAttributeModifiers 中 Strength(+3.0/级) 和 Weakness(-4.0/级) 的 Addition 操作。

    // ========== 2. 附魔伤害加成（从外部传入） ==========
    // 附魔伤害（锋利、亡灵杀手、节肢杀手）需要单独计算
    // 这应该在创建AttackContext时设置
    enchantDamage = m_enchantDamageBonus;

    // ========== 3. 攻击冷却影响 ==========
    // 基础伤害 × 二次冷却系数
    // 附魔伤害 × 线性冷却系数
    f32 quadraticCooldown = 0.2f + m_cooldownProgress * m_cooldownProgress * 0.8f;
    f32 linearCooldown = m_cooldownProgress;
    baseDamage *= quadraticCooldown;
    enchantDamage *= linearCooldown;

    // ========== 4. 暴击加成（只对基础伤害） ==========
    if (m_critical) {
        baseDamage *= m_criticalMultiplier;
    }

    // ========== 5. 合并基础伤害和附魔伤害 ==========
    f32 damage = baseDamage + enchantDamage;

    // ========== 6. 目标护甲减伤 ==========
    if (m_target && !m_bypassArmor) {
        // 获取护甲值和护甲韧性
        f32 armor = static_cast<f32>(m_target->attributes().getValue(entity::attribute::Attributes::ARMOR));
        f32 armorToughness =
            static_cast<f32>(m_target->attributes().getValue(entity::attribute::Attributes::ARMOR_TOUGHNESS));

        // 护甲公式:
        // protectionFactor = 2 + toughness / 4
        // effectiveArmor = clamp(armor - damage / protectionFactor, armor * 0.2, 20)
        // armorRatio = effectiveArmor / 25
        // final = damage * (1 - armorRatio)
        const f32 protectionFactor = 2.0f + armorToughness / 4.0f;
        f32 effectiveArmor = std::clamp(armor - damage / protectionFactor, armor * 0.2f, 20.0f);
        f32 armorRatio = effectiveArmor / 25.0f;

        // 破甲附魔(Breach)：修改护甲有效率
        // 每级 -0.15，降低护甲减伤效果
        if (m_weapon != nullptr && !m_weapon->isEmpty()) {
            i32 breachLevel = item::enchant::EnchantmentHelper::getBreachLevel(*m_weapon);
            if (breachLevel > 0) {
                f32 breachModifier = item::enchant::BreachEnchantment::getArmorEffectivenessModifier(breachLevel);
                armorRatio = std::clamp(armorRatio + breachModifier, 0.0f, 1.0f);
            }
        }

        damage *= (1.0f - armorRatio);

        // 抗性药水减伤（每级 -20%，最高 80%）
        // TODO: AttackContext::calculateFinalDamage 当前未被主伤害管线调用（玩家近战走 Player::attack
        //   → target.hurt → LivingEntity::applyPotionDamageCalculations，后者已查 BYPASSES_RESISTANCE
        //   门控）。本路径为死代码，接入主路径时须同步补 !source.is(BYPASSES_RESISTANCE) 门控，
        //   否则 OutOfWorld/GenericKill 伤害会被错误减免（与 applyPotionDamageCalculations 行为分叉）。
        const i32 resistanceLevel = m_target->getEffectLevel(entity::effect::EffectType::Resistance);
        if (resistanceLevel > 0) {
            damage *= std::max(0.0f, 1.0f - 0.2f * static_cast<f32>(resistanceLevel));
        }
    }

    // ========== 7. 附魔保护减伤 ==========
    // 计算护甲附魔的 EPF (Enchantment Protection Factor)
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
