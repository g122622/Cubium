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

#include "PlayerAttackHelper.hpp"
#include "../../item/core/ItemStack.hpp"
#include "../../item/enchantment/EnchantmentHelper.hpp"
#include "../../item/enchantment/enchantments/AllEnchantments.hpp"
#include "../../item/enchantment/enchantments/weapon/KnockbackEnchantment.hpp"
#include "../../util/math/MathUtils.hpp"
#include "../core/LivingEntity.hpp"
#include "../effect/EffectType.hpp"
#include "../entities/player/Player.hpp"
#include "AttackContext.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/DataParameter.hpp"

namespace mc::entity::combat {

// ========== 暴击判定 ==========

bool PlayerAttackHelper::isCriticalHit(const Player& player)
{
    // 暴击条件：
    // 1. 玩家正在下落（fallDistance > 0）
    // 2. 玩家不在地面
    // 3. 玩家不在水中
    // 4. 玩家不在梯子/藤蔓上
    // 5. 玩家没有失明效果
    // 6. 玩家没有骑乘
    // 7. 玩家不在疾跑（暴击后疾跑会被取消）

    // 条件 1: fallDistance > 0
    if (player.fallDistance() <= 0.0f) {
        return false;
    }

    // 条件 2: 不在地面
    if (player.isOnGround()) {
        return false;
    }

    // 条件 3: 不在水中
    if (player.isInWater()) {
        return false;
    }

    // 条件 4: 不在梯子/藤蔓上
    if (player.isOnLadder()) {
        return false;
    }

    // 条件 5: 没有失明效果
    if (player.hasEffect(effect::EffectType::Blindness)) {
        return false;
    }

    // 条件 6: 没有骑乘
    if (player.isRiding()) {
        return false;
    }

    // 条件 7: 不在疾跑（暴击时不能疾跑）
    // 注意：疾跑击退和暴击是互斥的
    if (player.isSprinting()) {
        return false;
    }

    return true;
}

// ========== 伤害计算 ==========

f32 PlayerAttackHelper::calculateDamage(const Player& player, f32 baseDamage, f32 cooldownProgress)
{
    f32 damage = baseDamage;

    // 冷却公式始终应用：damage * (0.2 + progress² * 0.8)
    // 注意：这不是仅在冷却不足时应用，而是始终应用！
    // 当 progress = 1.0 时，伤害为 100%；当 progress = 0 时，伤害为 20%
    damage = applyCooldown(damage, cooldownProgress);

    // 力量/虚弱药水效果已通过属性修改器系统自动应用到 ATTACK_DAMAGE 属性值中，
    // 不需要在此手动计算。参见 EffectAttributeModifiers 中 Strength(+3.0/级) 和 Weakness(-4.0/级) 的 Addition 操作。

    (void)player;
    return damage;
}

// ========== 攻击冷却 ==========

f32 PlayerAttackHelper::applyCooldown(f32 damage, f32 cooldownProgress)
{
    // 冷却公式：damage * (0.2 + progress² * 0.8)
    // 即使冷却为 0，伤害也有 20%，而不是 0%
    // 冷却为 1 时，伤害为 100%
    // 这是一个二次函数：damage * (0.2 + progress² * 0.8)
    return damage * (0.2f + cooldownProgress * cooldownProgress * 0.8f);
}

bool PlayerAttackHelper::isCooldownReady(f32 cooldownProgress, f32 threshold)
{
    return cooldownProgress >= threshold;
}

f32 PlayerAttackHelper::getCooldownProgress(i32 ticksSinceLastAttack, f32 attackSpeed)
{
    // cooldownProgress = ticksSinceLastAttack / (20 / attackSpeed)
    // 攻击间隔 = 20 / attackSpeed tick
    if (attackSpeed <= 0.0f) {
        return 0.0f;
    }

    f32 cooldownTime = 20.0f / attackSpeed;
    f32 progress = static_cast<f32>(ticksSinceLastAttack) / cooldownTime;

    return math::clamp(progress, 0.0f, 1.0f);
}

f32 PlayerAttackHelper::ticksToCooldownProgress(i32 ticksSinceLastAttack, f32 attackSpeed)
{
    return getCooldownProgress(ticksSinceLastAttack, attackSpeed);
}

// ========== 火焰附加 ==========

bool PlayerAttackHelper::applyFireAspect(LivingEntity& target, i32 fireAspectLevel)
{
    if (fireAspectLevel <= 0) {
        return false;
    }

    // 火焰持续时间 = 80 * fireAspectLevel ticks (每级4秒)
    i32 duration = FIRE_ASPECT_DURATION * fireAspectLevel;

    // 设置目标着火
    target.igniteForTicks(duration);

    return true;
}

// ========== 横扫攻击 ==========

f32 PlayerAttackHelper::getSweepingDamageRatio(i32 sweepingLevel)
{
    // 横扫之刃附魔：
    // I: 50%, II: 67%, III: 75%
    // 公式: 1 - 1/(level + 1)
    if (sweepingLevel <= 0) {
        return 0.0f;
    }
    return 1.0f - (1.0f / static_cast<f32>(sweepingLevel + 1));
}

// ========== 附魔伤害加成 ==========

f32 PlayerAttackHelper::getEnchantmentDamageBonus(const ItemStack& weapon, const LivingEntity* target)
{
    if (weapon.isEmpty()) {
        return 0.0f;
    }

    // 委托 EnchantmentHelper::getTotalDamageBonus 汇总武器上所有附魔的 getDamageBonus 虚函数。
    // 锋利（DamageEnchantment::Type::All）对所有生物 +0.5+level*0.5，与 target 无关；
    // 亡灵杀手（Type::Undead）/节肢杀手（Type::Arthropods）各自用 EntityTypeTags 标签
    // （SENSITIVE_TO_SMITE / SENSITIVE_TO_BANE_OF_ARTHROPODS）判定 target，命中则 +level*2.5。
    // 标签判定覆盖全部亡灵/节肢成员，包括 zombie_horse/zombie_nautilus 等枚举未覆盖的实体。
    // 穿刺（ImpalingEnchantment）属 Trident 类型附魔，与 Weapon 类型互斥，近战武器不会携带，无干扰。
    return item::enchant::EnchantmentHelper::getTotalDamageBonus(weapon, target);
}

// ========== 创建攻击上下文 ==========

AttackContext PlayerAttackHelper::createContext(Player& player, LivingEntity& target, f32 cooldownProgress)
{
    // TODO: 此 AttackContext 体系目前为死代码——全仓无业务调用方（Player::attack 与 MobEntity::doHurtTarget
    //   均走各自内联击退计算 + LivingEntity::getKnockback，不经此 createContext）。且此处击退强度语义偏离
    //   vanilla：默认 1.0 + sprint 0.5 + getKnockbackBonus(每级1.0) 直接累加，未走 vanilla 的
    //   getKnockback(target,source)=(ATTACK_KNOCKBACK+附魔)/2.0 路径。若未来启用此体系，须重写击退强度
    //   计算为 getKnockback(target) + (sprint?0.5:0) 对齐 vanilla Player.java:988（同 Player::attack 修复）。
    AttackContext context(static_cast<Entity*>(&player), &target);

    // 设置攻击冷却
    context.setCooldownProgress(cooldownProgress);

    // 检查暴击
    if (isCriticalHit(player) && isCooldownReady(cooldownProgress)) {
        context.setCritical(true);
        context.setCriticalMultiplier(CRITICAL_MULTIPLIER);
    }

    // 设置击退
    context.setKnockback(true);
    context.setKnockbackStrength(1.0f);

    // 检查是否疾跑
    if (player.isSprinting()) {
        context.setKnockbackStrength(1.0f + SPRINT_KNOCKBACK_BONUS);
    }

    // 检查武器附魔
    const ItemStack& mainHand = player.getHeldItem(Hand::MainHand);
    if (!mainHand.isEmpty()) {
        // 计算附魔伤害加成（锋利、亡灵杀手、节肢杀手，委托 getTotalDamageBonus 标签判定）
        f32 enchantBonus = getEnchantmentDamageBonus(mainHand, &target);
        context.setEnchantDamageBonus(enchantBonus);

        // 火焰附加
        i32 fireAspectLevel = item::enchant::EnchantmentHelper::getEnchantmentLevel(
            mainHand, &item::enchant::AllEnchantments::FIRE_ASPECT);
        if (fireAspectLevel > 0) {
            context.setFireDamage(true);
            context.setFireDuration(FIRE_ASPECT_DURATION * fireAspectLevel);
        }

        // 击退附魔
        i32 knockbackLevel =
            item::enchant::EnchantmentHelper::getEnchantmentLevel(mainHand, &item::enchant::AllEnchantments::KNOCKBACK);
        if (knockbackLevel > 0) {
            context.setKnockbackStrength(context.getKnockbackStrength() +
                item::enchant::KnockbackEnchantment::getKnockbackBonus(knockbackLevel));
        }
    }

    return context;
}

} // namespace mc::entity::combat
