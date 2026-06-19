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

#include "common/entity/core/LivingEntity.hpp"
#include "common/core/Constants.hpp"
#include "common/core/Types.hpp"
#include "common/entity/combat/CombatRules.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/entity/serialization/EntityNbtKeys.hpp"
#include "common/entity/serialization/NbtHelper.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/enchantment/EnchantmentHelper.hpp"
#include "common/mod/bedrock/addon/component/ItemComponentEvents.hpp"
#include "common/mod/bedrock/addon/component/ItemComponentRegistry.hpp"
#include "common/physics/PhysicsConstants.hpp"
#include "common/physics/PhysicsEngine.hpp"
#include "common/util/math/MathUtils.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockSoundType.hpp"
#include <algorithm>
#include <cmath>
#include <limits>

namespace mc {

// ============================================================================
// 常量
// ============================================================================

namespace {
// 数据参数
entity::DataParameter<i8> LIVING_FLAGS_PARAM{10};
entity::DataParameter<f32> HEALTH_PARAM{11};
entity::DataParameter<i32> POTION_EFFECTS_PARAM{12};
entity::DataParameter<i32> ARROW_COUNT_PARAM{13};

// 使用统一物理常量，避免重复定义
// 参考 physics::PhysicsConstants.hpp
using physics::DRAG_AIR;
using physics::DRAG_GROUND;
using physics::GRAVITY;
using physics::MOTION_THRESHOLD;
} // namespace

// ============================================================================
// 构造函数
// ============================================================================

LivingEntity::LivingEntity(EntityId id, IWorld* world)
    : Entity(id, world)
    , m_combatTracker(this)
{
    // 构造函数中设置 stepHeight = 0.6F
    setStepHeight(physics::STEP_HEIGHT);

    // 初始化装备槽
    for (auto& slot : m_equipment) {
        slot = ItemStack();
    }

    // 注册属性
    registerAttributes();
}

void LivingEntity::registerData()
{
    Entity::registerData();

    // 注册生物数据参数
    m_dataManager.registerParam(LIVING_FLAGS_PARAM, static_cast<i8>(0));
    m_dataManager.registerParam(HEALTH_PARAM, m_health);
    m_dataManager.registerParam(POTION_EFFECTS_PARAM, static_cast<i32>(0));
    m_dataManager.registerParam(ARROW_COUNT_PARAM, static_cast<i32>(0));
}

// ============================================================================
// 生命值
// ============================================================================

f32 LivingEntity::maxHealth() const
{
    return static_cast<f32>(m_attributes.getValue(entity::attribute::Attributes::MAX_HEALTH, 20.0));
}

void LivingEntity::setHealth(f32 health)
{
    f32 max = maxHealth();
    m_health = std::max(0.0f, std::min(health, max));
    m_dataManager.set(HEALTH_PARAM, m_health);
}

void LivingEntity::setAbsorptionAmount(f32 amount)
{
    // 与 MC 原版一致：吸收值限制在 [0, maxAbsorption] 范围内
    const f32 maxAbsorption = static_cast<f32>(getAttributeValue(entity::attribute::Attributes::MAX_ABSORPTION, 0.0));
    m_absorption = std::max(0.0f, std::min(amount, maxAbsorption));
}

void LivingEntity::heal(f32 amount)
{
    if (amount > 0.0f && !isDead()) {
        setHealth(m_health + amount);
    }
}

bool LivingEntity::hurt(DamageSource& source, f32 amount)
{
    // 1. 检查是否对伤害类型免疫
    if (isInvulnerableTo(source)) {
        return false;
    }

    // 2. 无敌帧逻辑
    // 如果 hurtResistantTime > 10，允许累积伤害
    if (m_hurtResistantTime > 10) {
        // 已经在无敌帧内，只承受差额伤害
        if (amount <= m_lastDamage) {
            return false; // 伤害不足
        }
        // 承受差额伤害
        actuallyHurt(source, amount - m_lastDamage);
        m_lastDamage = amount;
    } else {
        // 新的伤害，重置无敌帧
        m_lastDamage = amount;
        m_hurtResistantTime = MAX_HURT_RESISTANT_TIME;
        m_hurtTime = m_maxHurtTime;
        actuallyHurt(source, amount);
    }

    // 3. 战斗追踪器记录（需要在 actuallyHurt 中完成，因为那时才知道实际伤害）

    return true;
}

void LivingEntity::actuallyHurt(DamageSource& source, f32 amount)
{
    if (amount <= 0.0f) {
        return;
    }

    // 记录受伤前的生命值
    const f32 healthBefore = m_health;

    // 1. 盾牌格挡检查（子类可重写）
    if (canBlockDamageSource(source)) {
        damageShield(amount);
        return; // 格挡成功，不造成伤害
    }

    // 2. 护甲减伤（如果伤害不绕过护甲）
    if (!source.bypassesArmor()) {
        amount = applyArmorCalculations(source, amount);
        damageArmor(source, amount);
    }

    // 3. 药水效果和附魔保护减伤
    amount = applyPotionDamageCalculations(source, amount);

    // 4. 吸收值处理（金苹果额外生命）
    if (m_absorption > 0.0f) {
        const f32 absorbed = std::min(m_absorption, amount);
        setAbsorptionAmount(m_absorption - absorbed);
        amount -= absorbed;
    }

    if (amount <= 0.0f) {
        return; // 伤害被完全吸收
    }

    // 5. 实际扣血
    m_health -= amount;
    m_lastHealth = m_health;

    // 6. 记录到战斗追踪器
    m_combatTracker.trackDamage(source, m_lastHealth, amount);

    // 7. 记录伤害来源
    m_lastDamageSource = source.clone();

    // 8. 更新最近攻击者
    Entity* trueSource = source.getTrueSource();
    if (trueSource != nullptr && trueSource != this) {
        LivingEntity* attacker = dynamic_cast<LivingEntity*>(trueSource);
        if (attacker != nullptr) {
            setLastHurtBy(attacker);
        }
    }

    // 9. 触发荆棘附魔（对攻击者造成反伤）
    // 注意：荆棘伤害不触发无限循环，因为荆棘伤害的 isThornsDamage() 返回 true
    if (!source.isThornsDamage() && trueSource != nullptr && trueSource != this) {
        // 获取护甲槽位
        std::array<const ItemStack*, 4> armorSlots = {
            &getEquipment(EquipmentSlot::Head),  // 头盔
            &getEquipment(EquipmentSlot::Chest), // 胸甲
            &getEquipment(EquipmentSlot::Legs),  // 护腿
            &getEquipment(EquipmentSlot::Feet)   // 靴子
        };
        // 调用荆棘附魔回调
        item::enchant::EnchantmentHelper::applyThornsEnchantments(*this, *trueSource, armorSlots);
    }

    // 10. 更新战斗状态
    if (!m_inCombat) {
        m_inCombat = true;
        m_lastDamageTimestamp = ticksExisted();
        sendEnterCombat();
    }

    // 11. 死亡检查
    if (m_health <= 0.0f) {
        playDeathSound();
        die(source);
    } else {
        playHurtSound(source);
    }
}

bool LivingEntity::canBlockDamageSource(DamageSource& /*source*/) const
{
    // 默认返回 false，由 Player 子类重写实现盾牌格挡
    return false;
}

void LivingEntity::damageArmor(DamageSource& /*source*/, f32 /*amount*/)
{
    // 默认空实现，由 Player 子类重写
}

void LivingEntity::damageShield(f32 /*amount*/)
{
    // 默认空实现，由 Player 子类重写
}

f32 LivingEntity::applyArmorCalculations(DamageSource& source, f32 damage)
{
    if (source.bypassesArmor()) {
        return damage;
    }

    const f32 armor = static_cast<f32>(m_attributes.getValue(entity::attribute::Attributes::ARMOR, 0.0));
    const f32 toughness = static_cast<f32>(m_attributes.getValue(entity::attribute::Attributes::ARMOR_TOUGHNESS, 0.0));

    return entity::combat::CombatRules::getDamageAfterAbsorb(damage, armor, toughness);
}

f32 LivingEntity::applyPotionDamageCalculations(DamageSource& source, f32 damage)
{
    if (damage <= 0.0f) {
        return damage;
    }

    // 1. 抗性药水减伤
    // 注意：虚空伤害和特定伤害类型不受抗性药水影响
    if (!source.bypassesInvulnerability()) {
        const i32 resistanceLevel = getEffectLevel(entity::effect::EffectType::Resistance);
        if (resistanceLevel > 0) {
            damage = entity::combat::CombatRules::getDamageAfterResistance(damage, resistanceLevel);
        }
    }

    // 2. 附魔保护减伤
    // 遍历所有护甲槽位，计算保护附魔的 EPF 总和
    // 只有非 isDamageAbsolute 的伤害才受附魔保护影响
    if (!source.isDamageAbsolute()) {
        u32 damageTypeFlags = 0;
        if (source.isFire()) damageTypeFlags |= DamageFlags::FIRE;
        if (source.isFall()) damageTypeFlags |= DamageFlags::FALL;
        if (source.isExplosion()) damageTypeFlags |= DamageFlags::EXPLOSION;
        if (source.isProjectile()) damageTypeFlags |= DamageFlags::PROJECTILE;
        // 其他类型由全保护附魔处理 (ProtectionEnchantment::Type::All)

        auto armorSlots = getArmorSlots();
        i32 protectionEPF = item::enchant::EnchantmentHelper::getTotalArmorProtection(armorSlots, damageTypeFlags);
        if (protectionEPF > 0) {
            damage = entity::combat::CombatRules::getDamageAfterMagicAbsorb(damage, static_cast<f32>(protectionEPF));
        }
    }

    return damage;
}

f32 LivingEntity::computeFinalDamage(DamageSource& source, f32 damage)
{
    // 计算所有减伤后的最终伤害
    if (damage <= 0.0f || source.bypassesInvulnerability()) {
        return damage;
    }

    // 护甲减伤
    if (!source.bypassesArmor()) {
        damage = applyArmorCalculations(source, damage);
    }

    // 药水和附魔减伤
    damage = applyPotionDamageCalculations(source, damage);

    return damage;
}

void LivingEntity::die(DamageSource& /*cause*/)
{
    if (!isDead()) {
        return; // 已经死亡，避免重复执行
    }

    m_deathTime = 0;

    // 掉落经验
    dropExperience();
}

void LivingEntity::onKillCommand()
{
    // 使用虚空伤害杀死实体，这会触发完整的死亡流程
    auto damageSource = DamageSources::outOfWorld();
    hurt(damageSource, std::numeric_limits<f32>::max());
}

// ============================================================================
// 属性
// ============================================================================

void LivingEntity::registerAttributes()
{
    // 基础属性：所有生物实体都有
    m_attributes.registerAttribute(*entity::attribute::Attributes::maxHealth());
    m_attributes.registerAttribute(*entity::attribute::Attributes::knockbackResistance());
    m_attributes.registerAttribute(*entity::attribute::Attributes::movementSpeed());
    m_attributes.registerAttribute(*entity::attribute::Attributes::armor());
    m_attributes.registerAttribute(*entity::attribute::Attributes::armorToughness());
    m_attributes.registerAttribute(*entity::attribute::Attributes::maxAbsorption());

    // 注意：以下属性不在基类中注册：
    // - FOLLOW_RANGE: 由 MobEntity 设置默认值 16.0
    // - FLYING_SPEED: 由需要飞行的实体注册
    // - ATTACK_DAMAGE: 由 MonsterEntity 注册
    // - ATTACK_KNOCKBACK: 由需要攻击击退的实体注册
    // - LUCK: 由需要的实体注册
}

f64 LivingEntity::getAttributeValue(const std::string& name, f64 defaultValue) const
{
    return m_attributes.getValue(name, defaultValue);
}

void LivingEntity::setAttributeBaseValue(const std::string& name, f64 value)
{
    m_attributes.setBaseValue(name, value);
}

f32 LivingEntity::getSoundPitch() const
{
    math::Random random(static_cast<u64>(m_id) ^ (static_cast<u64>(m_ticksExisted) << 32));
    const f32 basePitch = isChild() ? 1.5f : 1.0f;
    return (random.nextFloat() - random.nextFloat()) * 0.2f + basePitch;
}

// ============================================================================
// 装备
// ============================================================================

const ItemStack& LivingEntity::getEquipment(EquipmentSlot slot) const
{
    size_t index = static_cast<size_t>(slot);
    if (index >= m_equipment.size()) {
        static ItemStack empty;
        return empty;
    }
    return m_equipment[index];
}

void LivingEntity::setEquipment(EquipmentSlot slot, const ItemStack& stack)
{
    size_t index = static_cast<size_t>(slot);
    if (index < m_equipment.size()) {
        m_equipment[index] = stack;
    }
}

// ============================================================================
// 受伤无敌帧
// ============================================================================

bool LivingEntity::isInvulnerableTo(DamageSource& source) const
{
    // 1. 检查实体是否处于无敌状态
    if (Entity::isInvulnerable()) {
        // 虚空伤害可以绕过无敌
        return !source.bypassesInvulnerability();
    }

    // 2. 检查无敌帧
    // 当 hurtResistantTime > 0 时，大部分伤害被阻挡
    // 但虚空伤害可以绕过
    if (m_hurtResistantTime > 0 && !source.bypassesInvulnerability()) {
        return true;
    }

    return false;
}

void LivingEntity::playHurtSound(DamageSource& source)
{
    auto soundEvent = getHurtSound(source);
    if (soundEvent.has_value()) {
        playSound(*soundEvent, getSoundVolume(), getSoundPitch());
    }
}

void LivingEntity::playDeathSound()
{
    auto soundEvent = getDeathSound();
    if (soundEvent.has_value()) {
        playSound(*soundEvent, getSoundVolume(), getSoundPitch());
    }
}

std::optional<ResourceLocation> LivingEntity::getHurtSound(DamageSource& /*source*/) const
{
    return makeSoundEventId("hurt");
}

std::optional<ResourceLocation> LivingEntity::getDeathSound() const
{
    return makeSoundEventId("death");
}

// ============================================================================
// 刻更新
// ============================================================================

void LivingEntity::tick()
{
    Entity::tick();

    // 保存上一帧渲染属性
    m_prevLimbSwing = m_limbSwing;
    m_prevLimbSwingAmount = m_limbSwingAmount;
    m_prevSwingProgress = m_swingProgress;
    m_prevRenderYawOffset = m_renderYawOffset;
    m_prevRotationYawHead = m_rotationYawHead;

    // 更新效果
    m_effectManager.tick(*this);

    // 更新无敌帧计时器
    if (m_hurtResistantTime > 0) {
        m_hurtResistantTime--;
    }

    // 更新受伤动画计时器
    if (m_hurtTime > 0) {
        m_hurtTime--;
    }

    // 更新攻击动画
    if (m_swingInProgress) {
        ++m_swingProgressInt;
        const i32 swingEnd = getArmSwingAnimationEnd();
        if (m_swingProgressInt >= swingEnd) {
            m_swingProgressInt = 0;
            m_swingInProgress = false;
        }
        m_swingProgress = static_cast<f32>(m_swingProgressInt) / static_cast<f32>(swingEnd);
    } else {
        m_swingProgress = 0.0f;
        m_swingProgressInt = 0;
    }

    // 更新步态动画
    updateAnimation();

    // 执行 AI 步进（物理更新）
    aiStep();

    // 更新生命值
    tickHealth();

    // 更新空气供应和溺水
    updateAirSupply();

    // 更新箭矢自动脱落
    tickArrows();

    // 更新死亡
    if (isDead()) {
        tickDeath();
    }

    // 重置战斗状态
    if (m_inCombat && ticksExisted() - m_lastDamageTimestamp > CombatTracker::COMBAT_TIMEOUT) {
        m_inCombat = false;
        sendEndCombat();
    }

    // 更新激流攻击状态
    updateSpinAttack();
}

void LivingEntity::syncMetadataFromDataManager()
{
    Entity::syncMetadataFromDataManager();
    m_health = m_dataManager.get<f32>(HEALTH_PARAM);
    m_lastHealth = m_health;
    m_arrowCount = m_dataManager.get<i32>(ARROW_COUNT_PARAM);
}

void LivingEntity::updateAnimation()
{
    // 计算移动距离
    f32 dx = x() - prevX();
    f32 dz = z() - prevZ();
    f32 distance = std::sqrt(dx * dx + dz * dz);

    // 更新步态动画
    m_prevLimbSwingAmount = m_limbSwingAmount;
    m_limbSwingAmount += (distance - m_limbSwingAmount) * 0.4f;

    // 如果在移动，增加步态周期
    if (distance > 0.001f) {
        m_limbSwing += std::min(distance, 1.0f);
    }

    // 更新移动距离
    m_prevMovedDistance = m_movedDistance;
    m_movedDistance = distance;
}

i32 LivingEntity::getArmSwingAnimationEnd() const
{
    // 默认 6 tick，急迫效果减少，挖掘疲劳增加
    i32 base = 6;

    // 检查急迫效果（加快挥动）
    const i32 hasteLevel = getEffectLevel(entity::effect::EffectType::Haste);
    if (hasteLevel > 0) {
        base -= 1 + hasteLevel;
    }

    // 检查挖掘疲劳效果（减慢挥动）
    const i32 fatigueLevel = getEffectLevel(entity::effect::EffectType::MiningFatigue);
    if (fatigueLevel > 0) {
        base += (1 + fatigueLevel) * 2;
    }

    // 确保最小值为 1
    return std::max(base, 1);
}

void LivingEntity::updateTravelAnimation(bool includeVertical)
{
    // 在 travel() 结束时调用，更新肢体摆动动画

    // 保存上一帧的 limbSwingAmount
    m_prevLimbSwingAmount = m_limbSwingAmount;

    // 计算位移
    f64 dx = static_cast<f64>(x() - prevX());
    f64 dy = includeVertical ? static_cast<f64>(y() - prevY()) : 0.0;
    f64 dz = static_cast<f64>(z() - prevZ());

    // 计算移动距离并乘以 4（用于动画速度）
    f32 distance = static_cast<f32>(std::sqrt(dx * dx + dy * dy + dz * dz)) * 4.0f;

    // 限制最大值为 1.0
    if (distance > 1.0f) {
        distance = 1.0f;
    }

    // 平滑插值 limbSwingAmount
    m_limbSwingAmount += (distance - m_limbSwingAmount) * 0.4f;

    // 增加 limbSwing 周期计数
    m_limbSwing += m_limbSwingAmount;
}

void LivingEntity::tickHealth()
{
    // 自然回血逻辑
    // 生命恢复效果每 50/(level+1) tick 治疗 1 点生命
    // 和平模式下每秒恢复 1 点生命

    // 检查生命恢复效果
    const i32 regenLevel = getEffectLevel(entity::effect::EffectType::Regeneration);
    if (regenLevel > 0 && m_health < maxHealth()) {
        // 生命恢复 tick 计数器
        m_regenTickCounter++;
        const i32 regenInterval = 50 / (regenLevel + 1);
        if (m_regenTickCounter >= regenInterval) {
            m_regenTickCounter = 0;
            heal(1.0f);
        }
    } else {
        m_regenTickCounter = 0;
    }

    // 更新属性缓存
    for (auto& [name, instance] : m_attributes.allInstances()) {
        if (instance->isDirty()) {
            (void)instance->getValue(); // 重新计算并缓存，故意丢弃返回值
        }
    }
}

void LivingEntity::tickDeath()
{
    m_deathTime++;

    // 死亡动画（20 ticks = 1 秒）
    if (m_deathTime >= 20) {
        remove(); // 移除实体
    }
}

// ============================================================================
// 摔落伤害
// ============================================================================

void LivingEntity::handleFallDamage(f32 distance, f32 damageMultiplier)
{
    // 默认使用普通摔落伤害来源
    causeFallDamage(distance, damageMultiplier, DamageSources::fall());
}

void LivingEntity::causeFallDamage(f32 distance, f32 damageMultiplier, const DamageSource& source)
{
    // MC 1.21.11: LivingEntity.causeFallDamage 先调用 super.causeFallDamage（传播给乘客）
    // 参考: net.minecraft.world.entity.LivingEntity.causeFallDamage → super.causeFallDamage
    Entity::causeFallDamage(distance, damageMultiplier, source);

    // 缓降效果免疫摔落伤害
    if (hasEffect(entity::effect::EffectType::SlowFalling)) {
        return;
    }

    // 跳跃增强药水减少摔落距离
    const i32 jumpBoostLevel = getEffectLevel(entity::effect::EffectType::JumpBoost);
    f32 effectiveDistance = distance - static_cast<f32>(jumpBoostLevel);

    // 计算摔落伤害
    // 摔落 > 3 格才开始受伤，每格 1 点伤害
    if (effectiveDistance > 3.0f) {
        f32 damage = (effectiveDistance - 3.0f) * damageMultiplier;

        // 计算摔落保护附魔减伤
        // 只有 source.isFall() 为 true 时才应用摔落保护附魔
        if (source.isFall()) {
            std::array<const ItemStack*, 4> armorSlots = {&getEquipment(EquipmentSlot::Head),
                &getEquipment(EquipmentSlot::Chest),
                &getEquipment(EquipmentSlot::Legs),
                &getEquipment(EquipmentSlot::Feet)};
            i32 fallProtectionEPF =
                item::enchant::EnchantmentHelper::getTotalArmorProtection(armorSlots, DamageFlags::FALL);
            if (fallProtectionEPF > 0) {
                damage =
                    entity::combat::CombatRules::getDamageAfterMagicAbsorb(damage, static_cast<f32>(fallProtectionEPF));
            }
        }

        if (damage > 0.0f) {
            // 克隆伤害来源以获得可变引用
            auto sourceClone = source.clone();
            hurt(*sourceClone, damage);
        }
    }

    // 播放摔落音效
    playFallSound(distance);
}

// ============================================================================
// 摔落音效
// ============================================================================

std::optional<ResourceLocation> LivingEntity::getFallSound(i32 /*fallHeight*/) const
{
    // 基类默认不返回摔落音效
    // 子类（Player, MonsterEntity 等）可以重写
    return std::nullopt;
}

void LivingEntity::playFallSound(f32 distance)
{
    if (m_world == nullptr || isSilent()) {
        return;
    }

    // 播放实体的摔落音效
    i32 fallHeight = static_cast<i32>(distance);
    auto fallSound = getFallSound(fallHeight);
    if (fallSound.has_value()) {
        playSound(*fallSound, 1.0f, 1.0f);
    }

    // 播放脚下方块的摔落音效
    BlockPos landPos(static_cast<i32>(std::floor(m_position.x)),
        static_cast<i32>(std::floor(m_position.y - 0.2f)), // 脚底位置
        static_cast<i32>(std::floor(m_position.z)));
    const BlockState* landState = m_world->getBlockState(landPos);
    if (landState != nullptr && !landState->isAir()) {
        const BlockSoundType& soundType = landState->getSoundType();
        playSound(soundType.getFallSound(), soundType.getVolume() * 0.5f, soundType.getPitch() * 0.75f);
    }
}

// ============================================================================
// AI移动（travel方法）
// ============================================================================

void LivingEntity::jump()
{
    // 执行跳跃
    f32 jumpPower = m_jumpUpwardsMotion;

    // 跳跃增强药水效果
    // 每级增加 0.1 跳跃力
    const i32 jumpBoostLevel = getEffectLevel(entity::effect::EffectType::JumpBoost);
    if (jumpBoostLevel > 0) {
        jumpPower += static_cast<f32>(jumpBoostLevel) * 0.1f;
    }

    // 设置垂直速度
    m_velocity.y = jumpPower;

    // 冲刺跳跃
    // 如果正在冲刺，添加额外的向前动量
    if (hasFlag(EntityFlags::Sprinting)) {
        // 获取朝向方向的水平分量
        f32 yawRad = yaw() * math::DEG_TO_RAD;
        f32 forwardX = -std::sin(yawRad) * 0.2f;
        f32 forwardZ = std::cos(yawRad) * 0.2f;
        m_velocity.x += forwardX;
        m_velocity.z += forwardZ;
    }

    m_onGround = false;
}

void LivingEntity::aiStep()
{
    // 处理跳跃
    if (m_isJumping) {
        // 在地面时执行跳跃
        if (m_onGround && m_jumpTicks == 0) {
            jump();
            m_jumpTicks = 10; // 跳跃冷却
        }
    } else {
        m_jumpTicks = 0;
    }

    // 更新跳跃冷却
    if (m_jumpTicks > 0) {
        m_jumpTicks--;
    }

    // 执行 travel（物理移动）
    // 注意：aiStep() 中不对输入值应用阻力
    // 阻力是在 travel() 中应用到速度上的
    travel(m_moveStrafing, 0.0f, m_moveForward);
}

void LivingEntity::travel(f32 strafing, f32 vertical, f32 forward)
{
    // 正确的物理顺序：
    // 1. 计算移动因子
    // 2. moveRelative(): 速度 += 输入 * 移动因子
    // 3. 应用重力（或攀爬）
    // 4. 执行移动（碰撞检测）
    // 5. 应用摩擦/阻力（基于滑度）
    // 6. 重置过小速度

    // 检查是否在梯子上
    bool onLadder = isOnLadder();

    // 获取移动速度属性
    f32 moveSpeed = static_cast<f32>(getAttributeValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.2));

    // 获取脚下方块的滑度
    f32 slipperiness = 0.6f; // 默认滑度
    if (m_onGround && m_world != nullptr) {
        BlockPos blockPos(static_cast<i32>(std::floor(m_position.x)),
            static_cast<i32>(std::floor(m_boundingBox.minY - 0.001f)),
            static_cast<i32>(std::floor(m_position.z)));
        const BlockState* blockState = m_world->getBlockState(blockPos);
        if (blockState != nullptr) {
            slipperiness = blockState->getBlock().getSlipperiness(*blockState, m_world, &blockPos, this);
        }
    }

    // 根据是否在地面选择不同的移动因子
    f32 moveFactor;
    if (m_onGround) {
        // 地面移动：使用滑度计算
        moveFactor = moveSpeed * 0.21600002f / (slipperiness * slipperiness * slipperiness);
    } else {
        // 空中移动：使用跳跃移动因子
        moveFactor = m_jumpMovementFactor;
    }

    // 1. 计算移动向量并添加到速度
    if (strafing != 0.0f || forward != 0.0f) {
        f32 length = std::sqrt(strafing * strafing + forward * forward);
        if (length < 1.0E-7f) {
            length = 1.0E-7f;
        }

        // 归一化并应用速度
        f32 normalizedStrafe = strafing / length * moveFactor;
        f32 normalizedForward = forward / length * moveFactor;

        // 根据偏航角计算实际移动方向
        f32 yawRad = m_yaw * math::DEG_TO_RAD;
        f32 sinYaw = std::sin(yawRad);
        f32 cosYaw = std::cos(yawRad);

        // moveRelative 公式
        f32 moveX = normalizedStrafe * cosYaw - normalizedForward * sinYaw;
        f32 moveZ = normalizedForward * cosYaw + normalizedStrafe * sinYaw;

        // 添加到速度（累加，不是替换）
        m_velocity.x += moveX;
        m_velocity.z += moveZ;
    }

    // 2. 应用重力或攀爬物理
    if (onLadder) {
        // 在梯子上时的特殊物理
        // 水平速度限制为 0.15，重力被抵消

        // 限制水平速度
        f32 horizontalSpeed = std::sqrt(m_velocity.x * m_velocity.x + m_velocity.z * m_velocity.z);
        constexpr f32 LADDER_MAX_SPEED = 0.15f;
        if (horizontalSpeed > LADDER_MAX_SPEED) {
            f32 scale = LADDER_MAX_SPEED / horizontalSpeed;
            m_velocity.x *= scale;
            m_velocity.z *= scale;
        }

        // 梯子上的垂直移动
        // 向上爬：Y速度正（输入控制）
        // 向下滑：Y速度负（重力控制，但被限制）
        // 静止：Y速度趋近于 0（缓慢滑落）

        // 如果在移动（按住前进键），允许向上爬
        // 否则应用轻微重力使其缓慢下滑
        if (forward > 0.0f) {
            // 向上攀爬
            m_velocity.y = physics::LADDER_CLIMB_SPEED;
        } else if (forward < 0.0f) {
            // 向下滑落（比正常下落慢）
            m_velocity.y = -physics::LADDER_SLIDE_SPEED;
        } else {
            // 不按键时，缓慢滑落
            if (m_velocity.y < -physics::LADDER_SPEED_MAX) {
                m_velocity.y = -physics::LADDER_SPEED_MAX;
            }
        }

        // 不应用正常重力（梯子上重力已被处理）
    } else if (!hasNoGravity()) {
        // 缓降效果处理
        f32 gravity = GRAVITY;

        if (hasEffect(entity::effect::EffectType::SlowFalling)) {
            // 缓降效果下重力大幅降低
            gravity = physics::SLOW_FALLING_GRAVITY;
            // 同时重置摔落距离
            m_fallDistance = 0.0f;
        }

        // 应用重力
        m_velocity.y -= gravity;
    }

    // 3. 执行碰撞移动
    // 注意：moveWithCollision() 内部会根据碰撞结果重置速度
    if (m_velocity.x != 0.0f || m_velocity.y != 0.0f || m_velocity.z != 0.0f) {
        moveWithCollision(m_velocity.x, m_velocity.y, m_velocity.z);
    }

    // 4. 应用摩擦/阻力（在移动后）
    if (m_onGround) {
        // 地面摩擦 = slipperiness * 0.91
        f32 groundFriction = slipperiness * 0.91f;
        m_velocity.x *= groundFriction;
        m_velocity.z *= groundFriction;
    } else if (isInWater()) {
        // 水中阻力
        f32 waterDrag = physics::DRAG_WATER;

        // 海豚的恩惠效果：大幅降低水中阻力
        if (hasEffect(entity::effect::EffectType::DolphinsGrace)) {
            waterDrag = physics::DOLPHINS_GRACE_WATER_DRAG;
        }

        // 深度守卫附魔减少水中阻力影响
        const ItemStack& boots = getEquipment(EquipmentSlot::Feet);
        if (!boots.isEmpty()) {
            i32 depthStriderLevel =
                item::enchant::EnchantmentHelper::getEnchantmentLevel(boots, "minecraft:depth_strider");
            if (depthStriderLevel > 0) {
                // 每级减少阻力差值的 1/3
                // 阻力从 0.8 提升到 max 0.546
                f32 dragReduction = static_cast<f32>(depthStriderLevel) * physics::DEPTH_STRIDER_SPEED_BONUS;
                waterDrag = std::min(waterDrag + dragReduction * (1.0f - waterDrag), physics::DEPTH_STRIDER_MAX_DRAG);
            }
        }

        m_velocity.x *= waterDrag;
        m_velocity.y *= waterDrag * 0.8f; // 垂直阻力略大
        m_velocity.z *= waterDrag;
    } else if (isInLava()) {
        // 岩浆阻力
        m_velocity.x *= physics::DRAG_LAVA;
        m_velocity.y *= physics::DRAG_LAVA * 0.8f;
        m_velocity.z *= physics::DRAG_LAVA;
    } else if (!onLadder) {
        // 空气阻力（不在梯子上）
        m_velocity.x *= DRAG_AIR;
        m_velocity.y *= DRAG_AIR;
        m_velocity.z *= DRAG_AIR;
    } else {
        // 梯子上的阻力
        m_velocity.x *= DRAG_GROUND;
        m_velocity.z *= DRAG_GROUND;
    }

    // 5. 重置过小的速度
    if (std::abs(m_velocity.x) < MOTION_THRESHOLD) m_velocity.x = 0.0f;
    if (std::abs(m_velocity.y) < MOTION_THRESHOLD) m_velocity.y = 0.0f;
    if (std::abs(m_velocity.z) < MOTION_THRESHOLD) m_velocity.z = 0.0f;
}

// ============================================================================
// 效果系统
// ============================================================================

bool LivingEntity::addEffect(entity::effect::EffectInstance effect)
{
    return m_effectManager.addEffect(std::move(effect), *this);
}

void LivingEntity::removeEffect(entity::effect::EffectType type)
{
    m_effectManager.removeEffect(type, *this);
}

void LivingEntity::removeAllEffects()
{
    m_effectManager.removeAllEffects(*this);
}

bool LivingEntity::hasEffect(entity::effect::EffectType type) const
{
    return m_effectManager.hasEffect(type);
}

const entity::effect::EffectInstance* LivingEntity::getEffect(entity::effect::EffectType type) const
{
    return m_effectManager.getEffect(type);
}

i32 LivingEntity::getEffectLevel(entity::effect::EffectType type) const
{
    return m_effectManager.getEffectLevel(type);
}

// ============================================================================
// 箭矢计数
// ============================================================================

void LivingEntity::setArrowCountInEntity(i32 count)
{
    m_arrowCount = std::max(0, count);
    m_dataManager.set(ARROW_COUNT_PARAM, m_arrowCount);
}

void LivingEntity::tickArrows()
{
    // 箭矢自动脱落逻辑
    // 仅在服务端执行
    if (world() == nullptr || world()->isClientSide()) {
        return;
    }

    if (m_arrowCount > 0) {
        // 如果计时器未启动，初始化计时器
        // 公式: 20 * (30 - arrowCount) ticks
        // 箭矢越多，脱落越快
        if (m_arrowHitTimer <= 0) {
            m_arrowHitTimer = 20 * (30 - m_arrowCount);
        }

        --m_arrowHitTimer;
        if (m_arrowHitTimer <= 0) {
            // 计时器归零，减少一支箭
            setArrowCountInEntity(m_arrowCount - 1);
        }
    }
}

// ============================================================================
// 攻击附魔回调
// ============================================================================

void LivingEntity::onAttackEntity(Entity& target)
{
    // 获取主手武器上的附魔，触发 onEntityDamaged 回调
    const ItemStack& mainHand = getMainHandItem();
    if (!mainHand.isEmpty()) {
        item::enchant::EnchantmentHelper::applyArthropodEnchantmentDamage(*this, target, mainHand);
    }
}

// ============================================================================
// 受伤追踪（Target Goals 使用）
// ============================================================================

void LivingEntity::setLastHurtBy(LivingEntity* attacker)
{
    m_lastHurtBy = attacker;
    m_lastHurtByTimestamp = ticksExisted();
}

void LivingEntity::setLastHurtTarget(LivingEntity* target)
{
    m_lastHurtTarget = target;
    m_lastHurtTargetTimestamp = ticksExisted();
}

// ============================================================================
// 击退
// ============================================================================

void LivingEntity::applyKnockback(f32 strength, f64 ratioX, f64 ratioZ)
{
    // 击退强度会被击退抗性降低
    strength = static_cast<f32>(static_cast<f64>(strength) *
        (1.0 - getAttributeValue(entity::attribute::Attributes::KNOCKBACK_RESISTANCE, 0.0)));

    if (strength <= 0.0f) {
        return; // 击退被完全抗性抵消
    }

    // 归一化方向向量
    f64 length = std::sqrt(ratioX * ratioX + ratioZ * ratioZ);
    if (length < 1.0E-7) {
        return; // 零向量，不应用击退
    }

    ratioX /= length;
    ratioZ /= length;

    // 计算击退速度
    // 击退会减少当前水平速度的一半，然后加上击退向量
    f64 knockbackX = ratioX * static_cast<f64>(strength);
    f64 knockbackZ = ratioZ * static_cast<f64>(strength);

    // Y轴速度
    f64 newVelocityY;
    if (m_onGround) {
        // 在地面时：Y速度 = min(0.4, 当前Y速度/2 + 击退强度)
        newVelocityY = std::min(0.4, static_cast<f64>(m_velocity.y) / 2.0 + static_cast<f64>(strength));
    } else {
        // 在空中时：保持当前Y速度
        newVelocityY = static_cast<f64>(m_velocity.y);
    }

    // 设置新速度
    // X轴：当前速度的一半减去击退向量
    // Z轴：当前速度的一半减去击退向量
    m_velocity.x = static_cast<f32>(static_cast<f64>(m_velocity.x) / 2.0 - knockbackX);
    m_velocity.y = static_cast<f32>(newVelocityY);
    m_velocity.z = static_cast<f32>(static_cast<f64>(m_velocity.z) / 2.0 - knockbackZ);

    // 设置为空中状态
    m_onGround = false;
}

void LivingEntity::applyKnockbackFrom(LivingEntity* attacker, f32 strength)
{
    if (attacker == nullptr) {
        return;
    }

    // 从攻击者位置计算击退方向
    f64 ratioX = static_cast<f64>(attacker->position().x - m_position.x);
    f64 ratioZ = static_cast<f64>(attacker->position().z - m_position.z);

    applyKnockback(strength, ratioX, ratioZ);
}

// ============================================================================
// 物品使用
// ============================================================================

void LivingEntity::setActiveHand(Hand hand)
{
    ItemStack heldItem = getEquipment(hand == Hand::MainHand ? EquipmentSlot::MainHand : EquipmentSlot::OffHand);

    if (heldItem.isEmpty()) {
        return;
    }

    i32 useDuration = heldItem.getItem()->getUseDuration(heldItem);
    if (useDuration <= 0) {
        return;
    }

    m_activeHand = hand;
    m_activeItem = heldItem;
    m_activeItemUseCount = useDuration;
}

void LivingEntity::stopActiveHand()
{
    if (!isUsingItem()) {
        return;
    }

    // 调用物品的 onPlayerStoppedUsing
    // 注意：const_cast 是安全的，因为 Items 在注册后是不可变的
    const Item* item = m_activeItem.getItem();
    if (!m_activeItem.isEmpty() && item != nullptr) {
        ItemStack stackCopy = m_activeItem;
        const_cast<Item*>(item)->onPlayerStoppedUsing(stackCopy, *m_world, *this, m_activeItemUseCount);
    }

    // 重置状态
    m_activeItem = ItemStack();
    m_activeItemUseCount = 0;
}

void LivingEntity::updateActiveItem()
{
    if (!isUsingItem()) {
        return;
    }

    // 递减使用计时器
    m_activeItemUseCount--;

    // 检查是否完成使用
    if (m_activeItemUseCount <= 0) {
        // 使用完成
        const Item* item = m_activeItem.getItem();
        if (!m_activeItem.isEmpty() && item != nullptr) {
            // 注意：const_cast 是安全的，因为 Items 在注册后是不可变的
            ItemStack result = const_cast<Item*>(item)->onItemUseFinish(m_activeItem, *m_world, *this);

            // 派发自定义物品组件回调 - onCompleteUse
            auto& itemCompReg = mc::mod::bedrock::addon::ItemComponentRegistry::instance();
            std::string itemTypeId = item->itemLocation().toString();
            if (itemCompReg.hasCompleteUseCallback(itemTypeId)) {
                mc::mod::bedrock::addon::ItemComponentCompleteUseEvent compEvent;
                compEvent.itemTypeId = itemTypeId;
                compEvent.sourceId = id();
                compEvent.useDuration = item->getUseDuration(m_activeItem);
                compEvent.itemStackAmount = m_activeItem.getCount();
                itemCompReg.dispatchCompleteUse(itemTypeId, compEvent);
            }

            // 更新装备槽
            setEquipment(m_activeHand == Hand::MainHand ? EquipmentSlot::MainHand : EquipmentSlot::OffHand, result);
        }
        m_activeItem = ItemStack();
        m_activeItemUseCount = 0;
    }
}

// ============================================================================
// 空气供应和溺水
// ============================================================================

i32 LivingEntity::decreaseAirSupply(i32 currentAir)
{
    // 水下呼吸附魔有概率不消耗空气
    const ItemStack& helmet = getEquipment(EquipmentSlot::Head);
    i32 respirationLevel = item::enchant::EnchantmentHelper::getRespirationLevel(helmet);

    if (respirationLevel > 0 && m_world != nullptr) {
        // 水下呼吸附魔: 每级有 level/(level+1) 的概率不消耗空气
        // I级: 50%, II级: 66.7%, III级: 75%
        math::Random& random = m_world->getRandom();
        if (random.nextInt(respirationLevel + 1) > 0) {
            return currentAir; // 附魔生效，不消耗空气
        }
    }

    return currentAir - 1;
}

i32 LivingEntity::determineNextAir(i32 currentAir) const
{
    // 每tick恢复4点空气
    return std::min(currentAir + 4, maxAir());
}

void LivingEntity::updateAirSupply()
{
    if (!isAlive()) {
        return;
    }

    bool inWater = isInWater();
    bool inLava = isInLava();

    // 检查是否能水下呼吸
    // - 水下呼吸效果 (WaterBreathing)
    // - 潮涌能量效果 (ConduitPower)
    // - 亡灵生物天生可以水下呼吸
    bool canBreathe = canBreatheUnderwater() || hasEffect(entity::effect::EffectType::WaterBreathing) ||
        hasEffect(entity::effect::EffectType::ConduitPower);

    if ((inWater || inLava) && !canBreathe) {
        // 在水或岩浆中且不能呼吸
        i32 newAir = decreaseAirSupply(air());
        setAir(newAir);

        // 空气耗尽到 -20 时触发溺水伤害
        if (air() <= -20) {
            setAir(0);

            // 溺水伤害计时器
            m_drownDamageTimer++;
            if (m_drownDamageTimer >= physics::DROWN_DAMAGE_INTERVAL) {
                m_drownDamageTimer = 0;

                // 造成溺水伤害
                EnvironmentalDamage drownSource = DamageSources::drown();
                hurt(drownSource, physics::DROWN_DAMAGE_AMOUNT);
            }
        }
    } else {
        // 不在水中或可以呼吸，恢复空气
        i32 newAir = determineNextAir(air());
        setAir(newAir);
        m_drownDamageTimer = 0;
    }
}

// ============================================================================
// 三叉戟激流攻击
// ============================================================================

bool LivingEntity::isSpinAttacking() const
{
    // 检查 LIVING_FLAGS 的第2位（0x04）
    // LIVING_FLAGS 位定义：
    // - 位 0 (0x01): 是否正在使用物品 (isHandActive)
    // - 位 1 (0x02): 使用的手（0=主手，1=副手）
    // - 位 2 (0x04): 是否正在旋转攻击（三叉戟激流）
    i8 flags = m_dataManager.get<i8>(LIVING_FLAGS_PARAM);
    return (flags & 0x04) != 0;
}

void LivingEntity::startSpinAttack(i32 duration)
{
    // 设置旋转攻击持续时间和标志
    m_spinAttackDuration = duration;

    // 设置 LIVING_FLAGS 的第2位（0x04）
    // 只在服务端设置，客户端通过数据参数同步
    if (m_world == nullptr || !m_world->isClientSide()) {
        i8 flags = m_dataManager.get<i8>(LIVING_FLAGS_PARAM);
        flags |= 0x04;
        m_dataManager.set(LIVING_FLAGS_PARAM, flags);
    }
}

void LivingEntity::stopSpinAttack()
{
    // 清除旋转攻击状态
    m_spinAttackDuration = 0;

    // 清除 LIVING_FLAGS 的第2位（0x04）
    // 只在服务端设置，客户端通过数据参数同步
    if (m_world == nullptr || !m_world->isClientSide()) {
        i8 flags = m_dataManager.get<i8>(LIVING_FLAGS_PARAM);
        flags &= ~0x04;
        m_dataManager.set(LIVING_FLAGS_PARAM, flags);
    }
}

void LivingEntity::updateSpinAttack()
{
    // 每tick递减持续时间，归零时停止
    if (m_spinAttackDuration > 0) {
        m_spinAttackDuration--;

        // 检查是否结束
        if (m_spinAttackDuration <= 0) {
            stopSpinAttack();
        } else if (isWet()) {
            // 在水中或雨中时，激流攻击会有额外的上升速度
            // 注意：这部分在 Player 的 travel() 中更详细实现
        }
    }
}

// ============================================================================
// 药水效果和摔落免疫
// ============================================================================

bool LivingEntity::isPotionApplicable(const entity::effect::EffectInstance& effect) const
{
    // 默认实现：所有效果都可应用
    // 子类可重写此方法来免疫某些效果（如凋灵免疫凋零效果）
    MC_UNUSED(effect);
    return true;
}

bool LivingEntity::onLivingFall(f32 distance, f32 damageMultiplier)
{
    // 默认实现：处理摔落伤害
    // 子类可重写此方法来免疫摔落伤害（如凋灵、末影龙）
    MC_UNUSED(distance);
    MC_UNUSED(damageMultiplier);
    return true;
}

// ============================================================================
// NBT 序列化
// ============================================================================

void LivingEntity::addAdditionalSaveData(nbt::tags::compound_tag& tag) const
{
    using namespace mc::entity::serialization;

    // 先调用基类实现
    Entity::addAdditionalSaveData(tag);

    // Health (f32)
    tag.put(nbt_keys::HEALTH, m_health);

    // AbsorptionAmount (f32)
    tag.put(nbt_keys::ABSORPTION_AMOUNT, m_absorption);

    // HurtTime (i16)
    tag.put(nbt_keys::HURT_TIME, static_cast<i16>(m_hurtTime));

    // DeathTime (i16)
    tag.put(nbt_keys::DEATH_TIME, static_cast<i16>(m_deathTime));

    // HurtByTimestamp (i32)
    tag.put(nbt_keys::HURT_BY_TIMESTAMP, m_lastDamageTimestamp);

    // FallFlying (byte) - 鞘翅飞行状态
    tag.put(nbt_keys::FALL_FLYING, static_cast<i8>(isElytraFlying() ? 1 : 0));

    // ActiveEffects - 药水效果列表
    const auto& effects = m_effectManager.getAllEffects();
    if (!effects.empty()) {
        auto effectsList = std::make_unique<nbt::tags::compound_list_tag>();
        for (const auto& effect : effects) {
            nbt::tags::compound_tag effectTag;
            effect.toNbt(effectTag);
            effectsList->value.push_back(std::move(effectTag));
        }
        tag.value.emplace(nbt_keys::ACTIVE_EFFECTS, std::move(effectsList));
    }

    // Attributes - 属性列表
    nbt_helper::writeAttributeMap(tag, nbt_keys::ATTRIBUTES, m_attributes);

    // HandItems - 手持物品
    // ArmorItems - 装备物品
    auto handItems = std::make_unique<nbt::tags::compound_list_tag>();
    {
        nbt::tags::compound_tag mainHandTag;
        getMainHandItem().toNbt(mainHandTag);
        handItems->value.push_back(std::move(mainHandTag));
    }
    {
        nbt::tags::compound_tag offHandTag;
        getOffHandItem().toNbt(offHandTag);
        handItems->value.push_back(std::move(offHandTag));
    }
    tag.value.emplace(nbt_keys::HAND_ITEMS, std::move(handItems));

    auto armorItems = std::make_unique<nbt::tags::compound_list_tag>();
    for (EquipmentSlot slot : {EquipmentSlot::Feet, EquipmentSlot::Legs, EquipmentSlot::Chest, EquipmentSlot::Head}) {
        nbt::tags::compound_tag armorTag;
        getEquipment(slot).toNbt(armorTag);
        armorItems->value.push_back(std::move(armorTag));
    }
    tag.value.emplace(nbt_keys::ARMOR_ITEMS, std::move(armorItems));
}

Result<void> LivingEntity::readAdditionalSaveData(const nbt::tags::compound_tag& tag)
{
    using namespace mc::entity::serialization;

    // 先调用基类实现
    MC_TRY(Entity::readAdditionalSaveData(tag));

    // Health (f32)
    if (auto val = nbt_helper::tryGetFloat(tag, nbt_keys::HEALTH)) {
        m_health = *val;
        m_lastHealth = m_health;
    }

    // AbsorptionAmount (f32)
    if (auto val = nbt_helper::tryGetFloat(tag, nbt_keys::ABSORPTION_AMOUNT)) {
        setAbsorptionAmount(*val);
    }

    // HurtTime (i16)
    if (auto val = nbt_helper::tryGetShort(tag, nbt_keys::HURT_TIME)) {
        m_hurtTime = *val;
    }

    // DeathTime (i16)
    if (auto val = nbt_helper::tryGetShort(tag, nbt_keys::DEATH_TIME)) {
        m_deathTime = *val;
    }

    // HurtByTimestamp (i32)
    if (auto val = nbt_helper::tryGetInt(tag, nbt_keys::HURT_BY_TIMESTAMP)) {
        m_lastDamageTimestamp = *val;
    }

    // FallFlying (byte)
    if (auto val = nbt_helper::tryGetBool(tag, nbt_keys::FALL_FLYING)) {
        if (*val) {
            addFlag(EntityFlags::FallFlying);
        } else {
            removeFlag(EntityFlags::FallFlying);
        }
    }

    // ActiveEffects - 药水效果列表
    if (auto* effectsList = nbt_helper::tryGetList(tag, nbt_keys::ACTIVE_EFFECTS)) {
        if (effectsList->element_id() == nbt::TagId::Compound) {
            auto& compoundList = dynamic_cast<const nbt::tags::compound_list_tag&>(*effectsList);
            m_effectManager.getAllEffects().clear();
            for (const auto& effectTag : compoundList.value) {
                m_effectManager.getAllEffects().push_back(entity::effect::EffectInstance::fromNbt(effectTag));
            }
        }
    }

    // Attributes - 属性列表
    nbt_helper::readAttributeMap(tag, nbt_keys::ATTRIBUTES, m_attributes);

    // HandItems / ArmorItems
    if (auto* handItems = nbt_helper::tryGetList(tag, nbt_keys::HAND_ITEMS)) {
        if (handItems->element_id() == nbt::TagId::Compound) {
            auto& compoundList = dynamic_cast<const nbt::tags::compound_list_tag&>(*handItems);
            if (!compoundList.value.empty()) {
                auto mainHandResult = ItemStack::fromNbt(compoundList.value[0]);
                if (mainHandResult.success()) {
                    setMainHandItem(mainHandResult.value());
                }
            }
            if (compoundList.value.size() > 1) {
                auto offHandResult = ItemStack::fromNbt(compoundList.value[1]);
                if (offHandResult.success()) {
                    setOffHandItem(offHandResult.value());
                }
            }
        }
    }

    if (auto* armorItems = nbt_helper::tryGetList(tag, nbt_keys::ARMOR_ITEMS)) {
        if (armorItems->element_id() == nbt::TagId::Compound) {
            auto& compoundList = dynamic_cast<const nbt::tags::compound_list_tag&>(*armorItems);
            constexpr std::array<EquipmentSlot, 4> armorOrder = {
                EquipmentSlot::Feet, EquipmentSlot::Legs, EquipmentSlot::Chest, EquipmentSlot::Head};
            for (size_t i = 0; i < armorOrder.size() && i < compoundList.value.size(); ++i) {
                auto armorResult = ItemStack::fromNbt(compoundList.value[i]);
                if (armorResult.success()) {
                    setEquipment(armorOrder[i], armorResult.value());
                }
            }
        }
    }

    return Result<void>::ok();
}

} // namespace mc
