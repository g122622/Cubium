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
#include "../../../ai/goal/goals/attack/RangedAttackGoals.hpp"
#include "../../../attribute/Attributes.hpp"
#include "../../../damage/DamageSource.hpp"
#include "../../../effect/EffectInstance.hpp"
#include "../../../entities/projectile/ProjectileItemEntity.hpp"
#include "../../../interfaces/IRangedAttackMob.hpp"
#include "../../../../item/potion/PotionUtils.hpp"
#include "../../../../item/potion/Potions.hpp"
#include "../../../../item/Items.hpp"
#include "sound/SoundEvents.hpp"
#include "../../../../util/math/random/Random.hpp"
#include "../../../../util/math/MathUtils.hpp"
#include "../../../../world/IWorld.hpp"
#include <cmath>

namespace mc {

WitchEntity::WitchEntity(EntityId id)
    : AbstractRaiderEntity(id)
{
    // 注册 AI 目标
    registerGoals();

    // 注册属性
    registerAttributes();
}

std::unique_ptr<Entity> WitchEntity::create(IWorld* /*world*/)
{
    return std::make_unique<WitchEntity>(EntityId(0));
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

    // MC 1.16.5: 女巫 AI 目标
    // priority 1: 游泳目标（已在父类注册）
    // priority 2: 药水攻击
    m_goalSelector.addGoal(2, std::make_unique<entity::ai::goal::RangedAttackGoal>(
        this, 1.0, 60, 60, ATTACK_RADIUS));

    // priority 3: 随机行走（避开水）
    // priority 4: 看向玩家
    // priority 5: 随机看
    // 注: 喝药水逻辑已通过 tick() 实现
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

// ========== 远程攻击 (IRangedAttackMob) ==========

void WitchEntity::attackEntityWithRangedAttack(LivingEntity* target, f32 /*charge*/)
{
    // MC 1.16.5: 如果正在喝药水，不能投掷药水
    if (m_drinking) {
        return;
    }

    // 检查目标有效性
    if (target == nullptr || !target->isAlive()) {
        return;
    }

    // 选择药水类型
    entity::effect::EffectType potionType = selectAttackPotionType(target);

    // 投掷药水
    throwPotionAt(target, potionType);
}

entity::effect::EffectType WitchEntity::selectAttackPotionType(LivingEntity* target) const
{
    math::Random rng = getRandom();

    // MC 1.16.5: 检查目标是否是掠夺者同伴
    // 如果是，则使用治疗/再生药水
    const AbstractRaiderEntity* raiderTarget = dynamic_cast<const AbstractRaiderEntity*>(target);
    if (raiderTarget != nullptr) {
        // 对掠夺者同伴使用治疗或再生药水
        if (target->health() <= 4.0f) {
            // 生命值<=4时用治疗药水
            return entity::effect::EffectType::InstantHealth;
        } else {
            // 否则用再生药水
            return entity::effect::EffectType::Regeneration;
        }
    }

    // 计算到目标的距离
    f64 dx = target->x() - x();
    f64 dy = target->y() - y();
    f64 dz = target->z() - z();
    f64 distanceSq = dx * dx + dy * dy + dz * dz;
    f32 distance = static_cast<f32>(std::sqrt(distanceSq));

    // MC 1.16.5: 根据距离和目标状态选择药水类型
    // 距离>=8格且目标无缓慢效果 -> 缓慢药水
    if (distance >= FAR_RANGE_DISTANCE && !target->hasEffect(entity::effect::EffectType::Slowness)) {
        return entity::effect::EffectType::Slowness;
    }

    // 目标生命>=8且无中毒效果 -> 中毒药水
    if (target->health() >= 8.0f && !target->hasEffect(entity::effect::EffectType::Poison)) {
        return entity::effect::EffectType::Poison;
    }

    // 距离<=3格且无虚弱效果（25%概率）-> 虚弱药水
    if (distance <= CLOSE_RANGE_DISTANCE && !target->hasEffect(entity::effect::EffectType::Weakness)) {
        if (rng.nextFloat() < WEAKNESS_CHANCE) {
            return entity::effect::EffectType::Weakness;
        }
    }

    // 默认：伤害药水
    return entity::effect::EffectType::InstantDamage;
}

void WitchEntity::throwPotionAt(LivingEntity* target, entity::effect::EffectType potionType)
{
    IWorld* worldPtr = world();
    if (worldPtr == nullptr || target == nullptr) {
        return;
    }

    math::Random rng = getRandom();

    // 计算投掷方向（考虑目标运动）
    // MC 1.16.5: Vector3d vector3d = target.getMotion();
    // double d0 = target.getPosX() + vector3d.x - this.getPosX();
    // double d1 = target.getPosYEye() - 1.1 - this.getPosY();
    // double d2 = target.getPosZ() + vector3d.z - this.getPosZ();
    Vector3 targetMotion = target->velocity();
    f64 dx = target->x() + targetMotion.x - x();
    f64 dy = target->y() + target->eyeHeight() - 1.1 - y();
    f64 dz = target->z() + targetMotion.z - z();

    f32 horizontalDist = std::sqrt(static_cast<f32>(dx * dx + dz * dz));

    // 创建药水实体
    auto potion = std::make_unique<entity::PotionEntity>(EntityId(0));
    potion->setWorld(worldPtr);

    // 设置发射者
    potion->setShooter(this);

    // 设置位置（从眼睛高度发射）
    f32 posX = static_cast<f32>(x());
    f32 posY = static_cast<f32>(y() + eyeHeight() - 0.1f);
    f32 posZ = static_cast<f32>(z());
    potion->setPosition(Vector3(posX, posY, posZ));

    // 设置药水类型
    potion->setLingering(false);

    // 根据效果类型创建药水物品
    const potion::Potion* potionItem = nullptr;
    switch (potionType) {
        case entity::effect::EffectType::InstantHealth:
            potionItem = potion::Potions::HEALING;
            break;
        case entity::effect::EffectType::InstantDamage:
            potionItem = potion::Potions::HARMING;
            break;
        case entity::effect::EffectType::Poison:
            potionItem = potion::Potions::POISON;
            break;
        case entity::effect::EffectType::Slowness:
            potionItem = potion::Potions::SLOWNESS;
            break;
        case entity::effect::EffectType::Weakness:
            potionItem = potion::Potions::WEAKNESS;
            break;
        case entity::effect::EffectType::Regeneration:
            potionItem = potion::Potions::REGENERATION;
            break;
        default:
            // 默认使用伤害药水
            potionItem = potion::Potions::HARMING;
            break;
    }

    if (potionItem != nullptr) {
        ItemStack potionStack = potion::PotionUtils::createSplashPotionItem(potionItem);
        potion->setItemStack(potionStack);
    }

    // MC 1.16.5: 调整俯仰角
    // potionentity.rotationPitch -= -20.0F;
    potion->setRotation(yaw(), pitch() - 20.0f);

    // MC 1.16.5: 发射药水
    // potionentity.shoot(d0, d1 + (double)(f * 0.2F), d2, 0.75F, 8.0F);
    // 其中 f = MathHelper.sqrt(d0 * d0 + d2 * d2) 是水平距离
    f32 velocity = POTION_VELOCITY;
    f32 inaccuracy = POTION_INACCURACY;

    // 投掷方向添加额外高度补偿
    f64 adjustedY = dy + static_cast<f64>(horizontalDist * 0.2f);

    potion->shoot(static_cast<f32>(dx), static_cast<f32>(adjustedY), static_cast<f32>(dz),
                  velocity, inaccuracy);

    // 播放投掷音效
    // MC 1.16.5: this.world.playSound((PlayerEntity)null, this.getPosX(), this.getPosY(), this.getPosZ(),
    //            SoundEvents.ENTITY_WITCH_THROW, this.getSoundCategory(), 1.0F, 0.8F + this.rand.nextFloat() * 0.4F);
    if (!isSilent()) {
        f32 pitch = 0.8f + rng.nextFloat() * 0.4f;
        playSound(SoundEvents::ENTITY_WITCH_THROW, 1.0f, pitch);
    }

    // 生成实体
    worldPtr->spawnEntity(std::move(potion));
}

} // namespace mc
