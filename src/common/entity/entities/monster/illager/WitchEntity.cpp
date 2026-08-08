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

#include "common/core/Types.hpp"
#include "common/entity/ai/goal/goals/attack/RangedAttackGoals.hpp"
#include "common/entity/ai/goal/goals/target/TargetGoals.hpp"
#include "common/entity/attribute/AttributeModifier.hpp"
#include "common/entity/attribute/Attributes.hpp"
#include "common/entity/core/EntityClassRegistry.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/entity/effect/EffectInstance.hpp"
#include "common/entity/effect/EffectType.hpp"
#include "common/entity/entities/monster/illager/AbstractRaiderEntity.hpp"
#include "common/entity/entities/projectile/ProjectileItemEntity.hpp"
#include "common/entity/interfaces/IRangedAttackMob.hpp"
#include "common/item/potion/Potion.hpp"
#include "common/item/potion/PotionUtils.hpp"
#include "common/item/potion/Potions.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include <cmath>
#include <memory>
#include <optional>
#include <utility>

namespace mc {

// ============================================================================
// 继承链标识（parent = AbstractRaiderEntity::classInfo()）。透传层无自身同步字段，
// classInfo 仅作父链遍历节点。
// ============================================================================
const entity::EntityClassInfo& WitchEntity::classInfo()
{
    static const entity::EntityClassInfo s_classInfo{"WitchEntity", &AbstractRaiderEntity::classInfo()};
    return s_classInfo;
}

WitchEntity::WitchEntity(EntityInstanceId id, ecs::EntityRegistry& registry)
    : AbstractRaiderEntity(id, registry)
{
    // 注册 AI 目标
    registerGoals();

    // 注册属性
    registerAttributes();
}

std::unique_ptr<Entity> WitchEntity::create(IWorld* /*world*/, ecs::EntityRegistry& registry)
{
    return std::make_unique<WitchEntity>(EntityInstanceId(0), registry);
}

// ========== 药水决策逻辑 ==========

bool WitchEntity::_needsHealing() const
{
    // 生命值低于最大值时需要治疗
    return health() < maxHealth();
}

bool WitchEntity::_needsWaterBreathing() const
{
    // 眼睛在水中且无水肺效果
    return areEyesInWater() && !hasEffect(entity::effect::EffectType::WaterBreathing);
}

bool WitchEntity::_lastDamageSourceWasFire() const
{
    DamageSource* lastDamage = lastDamageSource();
    return lastDamage != nullptr && lastDamage->isFire();
}

bool WitchEntity::_needsFireResistance() const
{
    // 正在燃烧或最后一次受到火焰伤害，且无抗火效果
    return (isOnFire() || _lastDamageSourceWasFire()) && !hasEffect(entity::effect::EffectType::FireResistance);
}

bool WitchEntity::_needsSwiftness() const
{
    // 有攻击目标、无速度效果、距离超过11格
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

std::optional<entity::effect::EffectType> WitchEntity::_decidePotionToDrink()
{
    math::Random& rng = getRandom();

    // 按优先级检查药水需求
    // 条件1：水肺药水 - 15%概率，眼睛在水中且无水肺效果
    if (rng.nextFloat() < WATER_BREATHING_CHANCE && _needsWaterBreathing()) {
        return entity::effect::EffectType::WaterBreathing;
    }

    // 条件2：抗火药水 - 15%概率，正在燃烧或受火焰伤害且无抗火效果
    if (rng.nextFloat() < FIRE_RESISTANCE_CHANCE && _needsFireResistance()) {
        return entity::effect::EffectType::FireResistance;
    }

    // 条件3：治疗药水 - 5%概率，生命值未满
    if (rng.nextFloat() < HEALING_CHANCE && _needsHealing()) {
        return entity::effect::EffectType::InstantHealth;
    }

    // 条件4：速度药水 - 50%概率，有目标且无速度效果且距离超过11格
    if (rng.nextFloat() < SWIFTNESS_CHANCE && _needsSwiftness()) {
        return entity::effect::EffectType::Speed;
    }

    return std::nullopt;
}

void WitchEntity::_startDrinkingPotion(entity::effect::EffectType effectType)
{
    // 设置喝药水状态
    m_drinking = true;
    m_drinkTimer = DRINK_DURATION;
    m_currentPotionType = effectType;

    // 播放喝药水音效
    if (!isSilent()) {
        math::Random& rng = getRandom();
        f32 pitch = 0.8f + rng.nextFloat() * 0.4f;
        playSound(SoundEvents::ENTITY_WITCH_DRINK, 1.0f, pitch);
    }

    // 应用移动速度减益 (-0.25 Addition 操作)
    // MC 1.21: 女巫喝药水时移动速度减少 0.25，使用 ADD_VALUE (Addition) 操作
    // 女巫基础移动速度为 0.25，减去 0.25 后变为 0，即喝药水时完全停止移动
    entity::attribute::AttributeModifier speedPenalty(
        DRINKING_SPEED_PENALTY_UUID, "Drinking speed penalty", -0.25, entity::attribute::Operation::Addition);
    m_attributes.addModifier(entity::attribute::Attributes::MOVEMENT_SPEED, speedPenalty);
}

void WitchEntity::_finishDrinkingPotion()
{
    // 清空喝药水状态
    m_drinking = false;
    m_drinkTimer = 0;

    // 应用喝药水的效果
    _applyDrankPotionEffect(m_currentPotionType);

    // 移除移动速度减益
    m_attributes.removeModifier(entity::attribute::Attributes::MOVEMENT_SPEED, DRINKING_SPEED_PENALTY_UUID);
}

void WitchEntity::_applyDrankPotionEffect(entity::effect::EffectType effectType)
{
    if (effectType == entity::effect::EffectType::InstantHealth) {
        // 瞬间治疗效果：直接恢复生命值
        // 治疗药水 I 恢复 4 点生命值（2颗心）
        // 女巫不是亡灵生物，所以治疗效果正常
        heal(4.0f);
    } else {
        // 其他效果：添加到效果管理器
        // 持续时间：3分钟 = 3600 ticks
        constexpr i32 POTION_DURATION = 3600;

        entity::effect::EffectInstance effect(effectType,
            POTION_DURATION,
            0,     // amplifier = 0 (效果等级 I)
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
    // 女巫对魔法伤害有 85% 减免，且免疫自己造成的伤害
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
            _finishDrinkingPotion();
        }
    }

    // 更新攻击冷却
    if (m_attackCooldown > 0) {
        m_attackCooldown--;
    }

    // 如果不在喝药水，检查是否需要喝药水
    if (!m_drinking && m_attackCooldown <= 0) {
        auto potionType = _decidePotionToDrink();
        if (potionType.has_value()) {
            _startDrinkingPotion(potionType.value());
            resetAttackCooldown();
        }
    }
}

void WitchEntity::registerGoals()
{
    // 调用父类方法
    AbstractRaiderEntity::registerGoals();

    // MC 原版: HurtByTargetGoal(this, Raider.class) — 女巫不会反击其他灾厄村民
    // 父类 MonsterEntity 注册了 HurtByTargetGoal(this, false)，需要替换为带排斥的版本
    m_targetSelector.addGoal(
        1, std::make_unique<entity::ai::goal::HurtByTargetGoal>(this, false, [](const LivingEntity* attacker) -> bool {
            return dynamic_cast<const AbstractRaiderEntity*>(attacker) != nullptr;
        }));

    // 女巫 AI 目标
    // priority 1: 游泳目标（已在父类注册）
    // priority 2: 药水攻击
    m_goalSelector.addGoal(2, std::make_unique<entity::ai::goal::RangedAttackGoal>(this, 1.0, 60, 60, ATTACK_RADIUS));

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
    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 26.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.25);
}

// ========== 远程攻击 (IRangedAttackMob) ==========

void WitchEntity::attackEntityWithRangedAttack(LivingEntity* target, f32 /*charge*/)
{
    // 如果正在喝药水，不能投掷药水
    if (m_drinking) {
        return;
    }

    // 检查目标有效性
    if (target == nullptr || !target->isAlive()) {
        return;
    }

    // 选择药水类型
    entity::effect::EffectType potionType = _selectAttackPotionType(target);

    // 投掷药水
    _throwPotionAt(target, potionType);
}

entity::effect::EffectType WitchEntity::_selectAttackPotionType(LivingEntity* target) const
{
    math::Random& rng = getRandom();

    // 检查目标是否是掠夺者同伴
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

    // 根据距离和目标状态选择药水类型
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

void WitchEntity::_throwPotionAt(LivingEntity* target, entity::effect::EffectType potionType)
{
    IWorld* worldPtr = world();
    if (worldPtr == nullptr || target == nullptr) {
        return;
    }

    math::Random& rng = getRandom();

    // 计算投掷方向（考虑目标运动）
    Vector3 targetMotion = target->velocity();
    f64 dx = target->x() + targetMotion.x - x();
    f64 dy = target->y() + target->eyeHeight() - 1.1 - y();
    f64 dz = target->z() + targetMotion.z - z();

    f32 horizontalDist = std::sqrt(static_cast<f32>(dx * dx + dz * dz));

    // 创建药水实体
    // ECS 迁移：实体构造需要 registry 句柄（worldPtr 已判空，此处 registry 必非空）
    auto* registry = worldPtr->entityRegistry();
    if (registry == nullptr) {
        return;
    }
    auto potion = std::make_unique<entity::PotionEntity>(EntityInstanceId(0), *registry);
    potion->setWorld(worldPtr);

    // 设置发射者
    potion->setShooter(this);

    // 设置位置（从眼睛高度发射）
    f32 posX = static_cast<f32>(x());
    f32 posY = static_cast<f32>(getEyeY() - 0.1f);
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

    // 调整俯仰角
    potion->setRotation(yaw(), pitch() - 20.0f);

    // 发射药水
    f32 velocity = POTION_VELOCITY;
    f32 inaccuracy = POTION_INACCURACY;

    // 投掷方向添加额外高度补偿
    f64 adjustedY = dy + static_cast<f64>(horizontalDist * 0.2f);

    potion->shoot(static_cast<f32>(dx), static_cast<f32>(adjustedY), static_cast<f32>(dz), velocity, inaccuracy);

    // 播放投掷音效
    if (!isSilent()) {
        f32 pitch = 0.8f + rng.nextFloat() * 0.4f;
        playSound(SoundEvents::ENTITY_WITCH_THROW, 1.0f, pitch);
    }

    // 生成实体
    worldPtr->spawnEntity(std::move(potion));
}

} // namespace mc
