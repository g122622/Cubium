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

#include "WitchEntity.hpp"
#include "../../../attribute/Attributes.hpp"
#include "../../../damage/DamageSource.hpp"
#include "../../../effect/EffectInstance.hpp"
#include "sound/SoundEvents.hpp"
#include "../../../../util/math/random/Random.hpp"

namespace mc {

WitchEntity::WitchEntity(LegacyEntityType type, EntityId id)
    : AbstractRaiderEntity(type, id)
{
    // 注册 AI 目标
    registerGoals();

    // 注册属性
    registerAttributes();
}

std::unique_ptr<Entity> WitchEntity::create(IWorld* /*world*/)
{
    return std::make_unique<WitchEntity>(LegacyEntityType::Unknown, 0);
}

// ========== 药水决策逻辑 ==========

bool WitchEntity::needsHealing() const
{
    // MC 1.16.5: 生命值低于最大值时需要治疗
    return health() < maxHealth();
}

bool WitchEntity::needsWaterBreathing() const
{
    // MC 1.16.5: 眼睛在水中且无水肺效果
    return areEyesInWater() && !hasEffect(entity::effect::EffectType::WaterBreathing);
}

bool WitchEntity::lastDamageSourceWasFire() const
{
    // MC 1.16.5: getLastDamageSource() != null && getLastDamageSource().isFireDamage()
    DamageSource* lastDamage = lastDamageSource();
    return lastDamage != nullptr && lastDamage->isFire();
}

bool WitchEntity::needsFireResistance() const
{
    // MC 1.16.5: 正在燃烧或最后一次受到火焰伤害
    // 且无抗火效果
    return (isOnFire() || lastDamageSourceWasFire()) && !hasEffect(entity::effect::EffectType::FireResistance);
}

bool WitchEntity::needsSwiftness() const
{
    // MC 1.16.5: 有攻击目标、无速度效果、距离超过11格
    const LivingEntity* target = attackTarget();
    if (target == nullptr) {
        return false;
    }

    if (hasEffect(entity::effect::EffectType::Speed)) {
        return false;
    }

    // 检查距离是否超过11格 (11^2 = 121)
    f64 dx = target->x() - x();
    f64 dy = target->y() - y();
    f64 dz = target->z() - z();
    f64 distSq = dx * dx + dy * dy + dz * dz;

    return distSq > SWIFTNESS_DISTANCE_SQ;
}

std::optional<entity::effect::EffectType> WitchEntity::decidePotionToDrink()
{
    math::Random rng = getRandom();

    // MC 1.16.5: 按优先级检查药水需求
    // 条件1：水肺药水 - 15%概率，眼睛在水中且无水肺效果
    if (rng.nextFloat() < WATER_BREATHING_CHANCE && needsWaterBreathing()) {
        return entity::effect::EffectType::WaterBreathing;
    }

    // 条件2：抗火药水 - 15%概率，正在燃烧或受火焰伤害且无抗火效果
    if (rng.nextFloat() < FIRE_RESISTANCE_CHANCE && needsFireResistance()) {
        return entity::effect::EffectType::FireResistance;
    }

    // 条件3：治疗药水 - 5%概率，生命值未满
    if (rng.nextFloat() < HEALING_CHANCE && needsHealing()) {
        return entity::effect::EffectType::InstantHealth;
    }

    // 条件4：速度药水 - 50%概率，有目标且无速度效果且距离超过11格
    if (rng.nextFloat() < SWIFTNESS_CHANCE && needsSwiftness()) {
        return entity::effect::EffectType::Speed;
    }

    return std::nullopt;
}

void WitchEntity::startDrinkingPotion(entity::effect::EffectType effectType)
{
    // 设置喝药水状态
    m_drinking = true;
    m_drinkTimer = DRINK_DURATION;
    m_currentPotionType = effectType;

    // MC 1.16.5: 播放喝药水音效
    // if (!this.isSilent()) {
    //     this.world.playSound((PlayerEntity)null, this.getPosX(), this.getPosY(), this.getPosZ(),
    //         SoundEvents.ENTITY_WITCH_DRINK, this.getSoundCategory(), 1.0F, 0.8F + this.rand.nextFloat() * 0.4F);
    // }
    if (!isSilent()) {
        math::Random rng = getRandom();
        f32 pitch = 0.8f + rng.nextFloat() * 0.4f;
        playSound(SoundEvents::ENTITY_WITCH_DRINK, 1.0f, pitch);
    }

    // MC 1.16.5: 应用移动速度减益 (-0.25)
    // 这需要在属性系统中添加修饰符
    // 目前先不实现，因为需要属性修饰符系统支持
}

void WitchEntity::finishDrinkingPotion()
{
    // MC 1.16.5: 清空喝药水状态
    m_drinking = false;
    m_drinkTimer = 0;

    // 应用喝药水的效果
    applyDrankPotionEffect(m_currentPotionType);

    // MC 1.16.5: 移除移动速度减益
    // 这需要在属性系统中移除修饰符
    // 目前先不实现
}

void WitchEntity::applyDrankPotionEffect(entity::effect::EffectType effectType)
{
    if (effectType == entity::effect::EffectType::InstantHealth) {
        // 瞬间治疗效果：直接恢复生命值
        // MC 1.16.5: 治疗药水 I 恢复 4 点生命值（2颗心）
        // 女巫不是亡灵生物，所以治疗效果正常
        heal(4.0f);
    } else {
        // 其他效果：添加到效果管理器
        // 持续时间：参考 MC 1.16.5 女巫喝的药水持续时间
        // - 水肺药水：3:00 (3600 ticks)
        // - 抗火药水：3:00 (3600 ticks)
        // - 速度药水：3:00 (3600 ticks)
        constexpr i32 POTION_DURATION = 3600; // 3分钟 = 3600 ticks

        entity::effect::EffectInstance effect(
            effectType,
            POTION_DURATION,
            0,    // amplifier = 0 (效果等级 I)
            false, // 非环境效果
            true,  // 显示粒子
            true   // 显示图标
        );

        addEffect(std::move(effect));
    }
}

// ========== 防御 ==========

f32 WitchEntity::applyMagicDamageReduction(DamageSource& source, f32 amount)
{
    // MC 1.16.5: 女巫对魔法伤害有 85% 减免
    // 且免疫自己造成的伤害
    if (source.getTrueSource() == this) {
        return 0.0f;
    }

    if (source.isMagic()) {
        return amount * 0.15f; // 只受 15% 伤害
    }

    return amount;
}

void WitchEntity::tick()
{
    AbstractRaiderEntity::tick();

    // 更新喝药水状态
    if (m_drinking && m_drinkTimer > 0) {
        m_drinkTimer--;
        if (m_drinkTimer <= 0) {
            // 喝完药水
            finishDrinkingPotion();
        }
    }

    // 更新攻击冷却
    if (m_attackCooldown > 0) {
        m_attackCooldown--;
    }

    // MC 1.16.5: 如果不在喝药水，检查是否需要喝药水
    if (!m_drinking && m_attackCooldown <= 0) {
        auto potionType = decidePotionToDrink();
        if (potionType.has_value()) {
            startDrinkingPotion(potionType.value());
            resetAttackCooldown();
        }
    }
}

void WitchEntity::registerGoals()
{
    // 调用父类方法
    AbstractRaiderEntity::registerGoals();

    // TODO: 女巫 AI 目标
    // - WitchAttackGoal: 药水攻击
    // - WitchDrinkPotionGoal: 喝药水（已通过 tick() 实现）
}

void WitchEntity::registerAttributes()
{
    // 调用父类方法
    AbstractRaiderEntity::registerAttributes();

    // 女巫的属性
    // 参考 MC 1.16.5 女巫属性
    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 26.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.25);
}

} // namespace mc
