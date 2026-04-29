#include "LivingEntity.hpp"
#include "../../core/Constants.hpp"
#include "../../util/math/MathUtils.hpp"
#include "../../util/math/random/Random.hpp"
#include "../../physics/PhysicsConstants.hpp"
#include "../../physics/PhysicsEngine.hpp"
#include <cmath>

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
    using physics::GRAVITY;
    using physics::DRAG_AIR;
    using physics::DRAG_GROUND;
    using physics::MOTION_THRESHOLD;
}

// ============================================================================
// 构造函数
// ============================================================================

LivingEntity::LivingEntity(LegacyEntityType type, EntityId id, IWorld* world)
    : Entity(type, id, world)
    , m_combatTracker(this)
{
    // 初始化装备槽
    for (auto& slot : m_equipment) {
        slot = ItemStack();
    }

    // 注册属性
    registerAttributes();
}

void LivingEntity::registerData() {
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

f32 LivingEntity::maxHealth() const {
    return static_cast<f32>(m_attributes.getValue(entity::attribute::Attributes::MAX_HEALTH, 20.0));
}

void LivingEntity::setHealth(f32 health) {
    f32 max = maxHealth();
    m_health = std::max(0.0f, std::min(health, max));
    m_dataManager.set(HEALTH_PARAM, m_health);
}

void LivingEntity::heal(f32 amount) {
    if (amount > 0.0f && !isDead()) {
        setHealth(m_health + amount);
    }
}

bool LivingEntity::hurt(DamageSource& source, f32 amount) {
    if (m_hurtTime > 0) {
        return false;  // 无敌帧期间
    }

    // 吸收值优先
    if (m_absorption > 0.0f) {
        f32 absorbed = std::min(m_absorption, amount);
        m_absorption -= absorbed;
        amount -= absorbed;
    }

    if (amount > 0.0f) {
        m_health -= amount;
        m_lastHealth = m_health;
        m_hurtTime = m_maxHurtTime;
        m_lastDamage = amount;

        // 记录伤害到战斗追踪器
        m_combatTracker.recordDamage(source.clone(), amount, ticksExisted());

        // 保存伤害来源（用于死亡消息）
        m_lastDamageSource = source.clone();

        // MC 1.16.5: 如果伤害来源是 LivingEntity，记录到 m_lastHurtBy
        // 参考 LivingEntity.damageEntity()
        Entity* trueSource = source.getTrueSource();
        if (trueSource != nullptr && trueSource != this) {
            // 尝试转换为 LivingEntity
            LivingEntity* attacker = dynamic_cast<LivingEntity*>(trueSource);
            if (attacker != nullptr) {
                setLastHurtBy(attacker);
            }
        }

        if (m_health <= 0.0f) {
            playDeathSound();
            die(source);
        } else {
            playHurtSound(source);
        }

        return true;
    }

    return false;
}

void LivingEntity::die(DamageSource& /*cause*/) {
    if (!isDead()) {
        return;  // 已经死亡，避免重复执行
    }

    m_deathTime = 0;

    // 掉落经验
    dropExperience();
}

// ============================================================================
// 属性
// ============================================================================

void LivingEntity::registerAttributes() {
    m_attributes.registerAttribute(*entity::attribute::Attributes::maxHealth());
    m_attributes.registerAttribute(*entity::attribute::Attributes::followRange());
    m_attributes.registerAttribute(*entity::attribute::Attributes::knockbackResistance());
    m_attributes.registerAttribute(*entity::attribute::Attributes::movementSpeed());
    m_attributes.registerAttribute(*entity::attribute::Attributes::flyingSpeed());
    m_attributes.registerAttribute(*entity::attribute::Attributes::attackDamage());
    m_attributes.registerAttribute(*entity::attribute::Attributes::attackKnockback());
    m_attributes.registerAttribute(*entity::attribute::Attributes::armor());
    m_attributes.registerAttribute(*entity::attribute::Attributes::armorToughness());
}

f64 LivingEntity::getAttributeValue(const String& name, f64 defaultValue) const {
    return m_attributes.getValue(name, defaultValue);
}

void LivingEntity::setAttributeBaseValue(const String& name, f64 value) {
    m_attributes.setBaseValue(name, value);
}

f32 LivingEntity::getSoundPitch() const {
    math::Random random(static_cast<u64>(m_id) ^ (static_cast<u64>(m_ticksExisted) << 32));
    const f32 basePitch = isChild() ? 1.5f : 1.0f;
    return (random.nextFloat() - random.nextFloat()) * 0.2f + basePitch;
}

// ============================================================================
// 装备
// ============================================================================

const ItemStack& LivingEntity::getEquipment(EquipmentSlot slot) const {
    size_t index = static_cast<size_t>(slot);
    if (index >= m_equipment.size()) {
        static ItemStack empty;
        return empty;
    }
    return m_equipment[index];
}

void LivingEntity::setEquipment(EquipmentSlot slot, const ItemStack& stack) {
    size_t index = static_cast<size_t>(slot);
    if (index < m_equipment.size()) {
        m_equipment[index] = stack;
    }
}

// ============================================================================
// 受伤无敌帧
// ============================================================================

bool LivingEntity::isInvulnerableTo(DamageSource& /*source*/) const {
    return m_hurtTime > 0;
}

void LivingEntity::playHurtSound(DamageSource& source) {
    auto soundEvent = getHurtSound(source);
    if (soundEvent.has_value()) {
        playSound(*soundEvent, getSoundVolume(), getSoundPitch());
    }
}

void LivingEntity::playDeathSound() {
    auto soundEvent = getDeathSound();
    if (soundEvent.has_value()) {
        playSound(*soundEvent, getSoundVolume(), getSoundPitch());
    }
}

std::optional<ResourceLocation> LivingEntity::getHurtSound(DamageSource& /*source*/) const {
    return makeSoundEventId("hurt");
}

std::optional<ResourceLocation> LivingEntity::getDeathSound() const {
    return makeSoundEventId("death");
}

// ============================================================================
// 刻更新
// ============================================================================

void LivingEntity::tick() {
    Entity::tick();

    // 保存上一帧渲染属性
    m_prevLimbSwing = m_limbSwing;
    m_prevLimbSwingAmount = m_limbSwingAmount;
    m_prevSwingProgress = m_swingProgress;
    m_prevRenderYawOffset = m_renderYawOffset;
    m_prevRotationYawHead = m_rotationYawHead;

    // 更新效果
    m_effectManager.tick(*this);

    // 更新受伤无敌帧
    if (m_hurtTime > 0) {
        m_hurtTime--;
    }

    // 更新攻击动画
    if (m_swingInProgress) {
        m_swingProgressInt++;
        if (m_swingProgressInt >= 6) {
            m_swingProgressInt = 0;
            m_swingInProgress = false;
        }
        m_swingProgress = static_cast<f32>(m_swingProgressInt) / 6.0f;
    } else {
        m_swingProgress = 0.0f;
        m_swingProgressInt = 0;
    }

    // 更新步态动画
    updateAnimation();

    // 更新生命值
    tickHealth();

    // 更新死亡
    if (isDead()) {
        tickDeath();
    }
}

void LivingEntity::syncMetadataFromDataManager() {
    Entity::syncMetadataFromDataManager();
    m_health = m_dataManager.get<f32>(HEALTH_PARAM);
    m_lastHealth = m_health;
}

void LivingEntity::updateAnimation() {
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

void LivingEntity::tickHealth() {
    // 自然回血（和平模式或生命恢复效果）
    // TODO: 实现生命恢复效果检查

    // 更新属性缓存
    for (auto& [name, instance] : m_attributes.allInstances()) {
        if (instance->isDirty()) {
            (void)instance->getValue();  // 重新计算并缓存，故意丢弃返回值
        }
    }
}

void LivingEntity::tickDeath() {
    m_deathTime++;

    // 死亡动画（20 ticks = 1 秒）
    if (m_deathTime >= 20) {
        remove();  // 移除实体
    }
}

// ============================================================================
// 摔落伤害
// ============================================================================

void LivingEntity::handleFallDamage(f32 distance, f32 damageMultiplier) {
    // 计算摔落伤害
    // MC 规则：摔落 > 3 格才开始受伤，每格 1 点伤害
    if (distance > 3.0f) {
        f32 damage = (distance - 3.0f) * damageMultiplier;

        // 考虑摔落保护效果
        // TODO: 实现摔落保护附魔

        if (damage > 0.0f) {
            // 创建摔落伤害来源
            EnvironmentalDamage source = DamageSources::fall();
            hurt(source, damage);
        }
    }
}

// ============================================================================
// AI移动（travel方法）
// ============================================================================

void LivingEntity::jump() {
    // 执行跳跃
    // 参考 MC LivingEntity.jump()
    f32 jumpPower = m_jumpUpwardsMotion;

    // TODO: 检查跳跃增强药水效果

    // 设置垂直速度
    m_velocity.y = jumpPower;

    // 如果正在冲刺，添加额外的向前动量
    // TODO: 实现冲刺跳跃

    m_onGround = false;
}

void LivingEntity::aiStep() {
    // 处理跳跃
    // 参考 MC LivingEntity.livingTick() 中的跳跃处理
    if (m_isJumping) {
        // 简化实现：在地面时执行跳跃
        if (m_onGround && m_jumpTicks == 0) {
            jump();
            m_jumpTicks = 10;  // 跳跃冷却
        }
    } else {
        m_jumpTicks = 0;
    }

    // 更新跳跃冷却
    if (m_jumpTicks > 0) {
        m_jumpTicks--;
    }

    // 应用移动阻力
    // 参考 MC LivingEntity.livingTick() / aiStep()
    m_moveStrafing *= DRAG_AIR;
    m_moveForward *= DRAG_AIR;

    // 执行travel（物理移动）
    travel(m_moveStrafing, 0.0f, m_moveForward);
}

void LivingEntity::travel(f32 strafing, f32 vertical, f32 forward) {
    // 参考 MC 1.16.5 LivingEntity.travel()
    // 核心物理移动逻辑

    // 获取移动速度属性
    f32 moveSpeed = static_cast<f32>(getAttributeValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.2));

    // 根据是否在地面选择不同的移动因子
    f32 moveFactor;
    if (m_onGround) {
        // 地面移动：使用滑度计算
        // MC 公式: speed * (0.21600002F / (slipperiness^3))
        // 默认滑度 0.6 -> 0.21600002 / 0.216 = 1.0
        // 简化：直接使用 moveSpeed
        moveFactor = moveSpeed * 0.21600002f / (0.6f * 0.6f * 0.6f);
    } else {
        // 空中移动：使用跳跃移动因子
        moveFactor = m_jumpMovementFactor;
    }

    // 计算移动向量（根据实体朝向）
    // 参考 MC Entity.moveRelative()
    if (strafing != 0.0f || forward != 0.0f) {
        // 计算归一化后的移动向量
        f32 length = std::sqrt(strafing * strafing + forward * forward);
        if (length < 1.0E-7f) {
            return;
        }

        // 归一化并应用速度
        f32 normalizedStrafe = strafing / length * moveFactor;
        f32 normalizedForward = forward / length * moveFactor;

        // 根据偏航角计算实际移动方向
        // MC坐标系: yaw=0 看向 +Z, yaw=90 看向 -X (左手坐标系)
        f32 yawRad = m_yaw * math::DEG_TO_RAD;
        f32 sinYaw = std::sin(yawRad);
        f32 cosYaw = std::cos(yawRad);

        // MC的moveRelative公式:
        // moveX = strafe * cosYaw - forward * sinYaw
        // moveZ = forward * cosYaw + strafe * sinYaw
        f32 moveX = normalizedStrafe * cosYaw - normalizedForward * sinYaw;
        f32 moveZ = normalizedForward * cosYaw + normalizedStrafe * sinYaw;

        // 添加到速度
        m_velocity.x += moveX;
        m_velocity.z += moveZ;
    }

    // 应用重力
    if (!m_onGround && !hasNoGravity()) {
        m_velocity.y -= GRAVITY;
    }

    // 应用空气阻力
    m_velocity.x *= DRAG_AIR;
    m_velocity.y *= DRAG_AIR;
    m_velocity.z *= DRAG_AIR;

    // 如果在地面，停止Y方向速度
    if (m_onGround && m_velocity.y < 0.0f) {
        m_velocity.y = 0.0f;
    }

    // 重置过小的速度
    if (std::abs(m_velocity.x) < MOTION_THRESHOLD) m_velocity.x = 0.0f;
    if (std::abs(m_velocity.y) < MOTION_THRESHOLD) m_velocity.y = 0.0f;
    if (std::abs(m_velocity.z) < MOTION_THRESHOLD) m_velocity.z = 0.0f;

    // 执行碰撞移动
    if (m_velocity.x != 0.0f || m_velocity.y != 0.0f || m_velocity.z != 0.0f) {
        moveWithCollision(m_velocity.x, m_velocity.y, m_velocity.z);
    }

    // 更新地面摩擦
    if (m_onGround) {
        m_velocity.x *= DRAG_GROUND;
        m_velocity.z *= DRAG_GROUND;
    }
}

// ============================================================================
// 效果系统
// ============================================================================

bool LivingEntity::addEffect(entity::effect::EffectInstance effect) {
    return m_effectManager.addEffect(std::move(effect), *this);
}

void LivingEntity::removeEffect(entity::effect::EffectType type) {
    m_effectManager.removeEffect(type, *this);
}

void LivingEntity::removeAllEffects() {
    m_effectManager.removeAllEffects(*this);
}

bool LivingEntity::hasEffect(entity::effect::EffectType type) const {
    return m_effectManager.hasEffect(type);
}

const entity::effect::EffectInstance* LivingEntity::getEffect(entity::effect::EffectType type) const {
    return m_effectManager.getEffect(type);
}

i32 LivingEntity::getEffectLevel(entity::effect::EffectType type) const {
    return m_effectManager.getEffectLevel(type);
}

// ============================================================================
// 受伤追踪（Target Goals 使用）
// ============================================================================

void LivingEntity::setLastHurtBy(LivingEntity* attacker) {
    // MC 1.16.5: LivingEntity.setLastHurtBy()
    m_lastHurtBy = attacker;
    m_lastHurtByTimestamp = ticksExisted();
}

void LivingEntity::setLastHurtTarget(LivingEntity* target) {
    // MC 1.16.5: LivingEntity.setLastHurtTarget()
    m_lastHurtTarget = target;
    m_lastHurtTargetTimestamp = ticksExisted();
}

// ============================================================================
// 击退
// ============================================================================

void LivingEntity::applyKnockback(f32 strength, f64 ratioX, f64 ratioZ) {
    // MC 1.16.5: LivingEntity.applyKnockback()
    // 击退强度会被击退抗性降低
    strength = static_cast<f32>(static_cast<f64>(strength) *
        (1.0 - getAttributeValue(entity::attribute::Attributes::KNOCKBACK_RESISTANCE, 0.0)));

    if (strength <= 0.0f) {
        return;  // 击退被完全抗性抵消
    }

    // 归一化方向向量
    f64 length = std::sqrt(ratioX * ratioX + ratioZ * ratioZ);
    if (length < 1.0E-7) {
        return;  // 零向量，不应用击退
    }

    ratioX /= length;
    ratioZ /= length;

    // 计算击退速度
    // MC 1.16.5: this.setMotion(vec3d.x / 2.0D - vec3d1.x, ...
    // 击退会减少当前水平速度的一半，然后加上击退向量
    f64 knockbackX = ratioX * static_cast<f64>(strength);
    f64 knockbackZ = ratioZ * static_cast<f64>(strength);

    // Y轴速度
    // MC 1.16.5: onGround ? Math.min(0.4D, vec3d.y / 2.0D + (double)strength) : vec3d.y
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

    // 设置为空中状态（MC 1.16.5: isAirBorne = true）
    m_onGround = false;
}

void LivingEntity::applyKnockbackFrom(LivingEntity* attacker, f32 strength) {
    if (attacker == nullptr) {
        return;
    }

    // MC 1.16.5: 从攻击者位置计算击退方向
    // ratioX = attacker.x - this.x (然后取反)
    // ratioZ = attacker.z - this.z (然后取反)
    f64 ratioX = static_cast<f64>(attacker->position().x - m_position.x);
    f64 ratioZ = static_cast<f64>(attacker->position().z - m_position.z);

    applyKnockback(strength, -ratioX, -ratioZ);
}

} // namespace mc
