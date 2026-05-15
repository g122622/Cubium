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

#include "NetherEntities.hpp"
#include "../../../../sound/SoundEvents.hpp"
#include "../../../../util/math/MathUtils.hpp"
#include "../../../../world/IWorld.hpp"
#include "../../../attribute/Attributes.hpp"
#include "../../../core/EntityRegistry.hpp"
#include "../../../core/LivingEntity.hpp"
#include "../../../entities/projectile/AbstractFireballEntity.hpp"
#include <cmath>

namespace mc {

// GhastEntity
std::unique_ptr<Entity> GhastEntity::create(IWorld* world)
{
    return std::make_unique<GhastEntity>(LegacyEntityType::Ghast, EntityId(0));
}

GhastEntity::GhastEntity(LegacyEntityType type, EntityId id)
    : MonsterEntity(type, id)
{
    setBurnsInDaylight(false);
}

void GhastEntity::tick()
{
    MonsterEntity::tick();

    if (m_attackCooldown > 0) {
        m_attackCooldown--;
    }

    if (m_isCharging) {
        m_chargeTime++;
        if (m_chargeTime >= 20) { // 充能时间
            shootFireball();
            m_isCharging = false;
            m_chargeTime = 0;
            m_attackCooldown = 40; // 攻击冷却
        }
    }
}

void GhastEntity::shootFireball()
{
    // MC 1.16.5 GhastEntity.FireballAttackGoal.tick()
    // 在充能 20 ticks 后发射火球
    IWorld* worldPtr = world();
    LivingEntity* target = attackTarget();
    if (!worldPtr || !target || !target->isAlive()) {
        return;
    }

    // MC 1.16.5: 计算发射方向
    // d2, d3, d4 是从恶魂到目标的方向向量
    // vector3d = this.parentEntity.getLook(1.0F)
    // 发射位置 = 恶魂位置 + lookVector * 4.0

    // 获取恶魂的朝向向量（yaw 和 pitch 计算得出）
    const f32 yawRad = yaw() * math::DEG_TO_RAD;
    const f32 pitchRad = pitch() * math::DEG_TO_RAD;

    // 计算 look 向量
    const f32 lookX = -std::sin(yawRad) * std::cos(pitchRad);
    const f32 lookY = -std::sin(pitchRad);
    const f32 lookZ = std::cos(yawRad) * std::cos(pitchRad);

    // 火球发射位置：恶魂位置 + lookVector * 4.0
    const f32 fireballX = static_cast<f32>(x() + lookX * 4.0);
    const f32 fireballY = static_cast<f32>(y() + eyeHeight() + 0.5 + lookY * 4.0);
    const f32 fireballZ = static_cast<f32>(z() + lookZ * 4.0);

    // 计算到目标的方向向量
    // MC 1.16.5:
    // d2 = livingentity.getPosX() - (this.parentEntity.getPosX() + vector3d.x * 4.0D)
    // d3 = livingentity.getPosYHeight(0.5D) - (0.5D + this.parentEntity.getPosYHeight(0.5D))
    // d4 = livingentity.getPosZ() - (this.parentEntity.getPosZ() + vector3d.z * 4.0D)
    const f32 dx = static_cast<f32>(target->x() - fireballX);
    const f32 dy = static_cast<f32>(target->y() + target->eyeHeight() * 0.5 - (y() + eyeHeight() * 0.5 + 0.5));
    const f32 dz = static_cast<f32>(target->z() - fireballZ);

    // 创建火球实体
    auto fireball = std::make_unique<entity::FireballEntity>(LegacyEntityType::Fireball, EntityId(0));
    fireball->setShooter(this);
    fireball->setPosition(Vector3(fireballX, fireballY, fireballZ));

    // MC 1.16.5: 设置加速度方向
    // FireballEntity 构造函数中设置加速度
    // accelerationX/Y/Z 每tick累加到速度上
    fireball->setAcceleration(dx, dy, dz);

    // MC 1.16.5: 设置爆炸威力
    fireball->setExplosionPower(m_explosionPower);

    // 生成实体
    worldPtr->spawnEntity(std::move(fireball));

    // MC 1.16.5: 播放发射音效
    // world.playEvent((PlayerEntity)null, 1016, this.parentEntity.getPosition(), 0)
    playSound(SoundEvents::ENTITY_GHAST_SHOOT, 1.0f, 1.0f);
}

void GhastEntity::registerGoals()
{
    MonsterEntity::registerGoals();
    // TODO: 添加恶魂特有AI
}

void GhastEntity::registerAttributes()
{
    MonsterEntity::registerAttributes();

    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 10.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::FLYING_SPEED, 0.9);
}

// MagmaCubeEntity
std::unique_ptr<Entity> MagmaCubeEntity::create(IWorld* world)
{
    return std::make_unique<MagmaCubeEntity>(LegacyEntityType::MagmaCube, EntityId(0));
}

MagmaCubeEntity::MagmaCubeEntity(LegacyEntityType type, EntityId id)
    : MonsterEntity(type, id)
{}

void MagmaCubeEntity::setSize(i32 size)
{
    m_size = size;
    // TODO: 更新碰撞箱和属性
}

void MagmaCubeEntity::registerGoals()
{
    MonsterEntity::registerGoals();
    // TODO: 添加岩浆怪特有AI
}

void MagmaCubeEntity::registerAttributes()
{
    MonsterEntity::registerAttributes();

    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 16.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.2);
    m_attributes.setBaseValue(entity::attribute::Attributes::ATTACK_DAMAGE, 4.0);
}

// AbstractPiglinEntity
AbstractPiglinEntity::AbstractPiglinEntity(LegacyEntityType type, EntityId id)
    : MonsterEntity(type, id)
{
    setBurnsInDaylight(false);
}

void AbstractPiglinEntity::registerGoals()
{
    MonsterEntity::registerGoals();
    // TODO: 添加猪灵基础AI
}

// PiglinEntity
std::unique_ptr<Entity> PiglinEntity::create(IWorld* world)
{
    return std::make_unique<PiglinEntity>(LegacyEntityType::Piglin, EntityId(0));
}

PiglinEntity::PiglinEntity(LegacyEntityType type, EntityId id)
    : AbstractPiglinEntity(type, id)
{}

void PiglinEntity::attackEntityWithRangedAttack(LivingEntity* target, f32 charge)
{
    // TODO: 实现弩攻击逻辑
    (void)target;
    (void)charge;
}

void PiglinEntity::onCrossbowLoadComplete(ItemStack& crossbow)
{
    // TODO: 实现弩装填完成逻辑
    (void)crossbow;
}

void PiglinEntity::shootCrossbow(LivingEntity* target, ItemStack& crossbow, f32 charge)
{
    // TODO: 实现弩射击逻辑
    (void)target;
    (void)crossbow;
    (void)charge;
}

void PiglinEntity::registerGoals()
{
    AbstractPiglinEntity::registerGoals();
    // TODO: 添加猪灵特有AI
}

void PiglinEntity::registerAttributes()
{
    AbstractPiglinEntity::registerAttributes();

    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 16.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.35);
    m_attributes.setBaseValue(entity::attribute::Attributes::ATTACK_DAMAGE, 5.0);
}

// PiglinBruteEntity
std::unique_ptr<Entity> PiglinBruteEntity::create(IWorld* world)
{
    return std::make_unique<PiglinBruteEntity>(LegacyEntityType::PiglinBrute, EntityId(0));
}

PiglinBruteEntity::PiglinBruteEntity(LegacyEntityType type, EntityId id)
    : AbstractPiglinEntity(type, id)
{}

void PiglinBruteEntity::registerGoals()
{
    AbstractPiglinEntity::registerGoals();
    // TODO: 添加猪灵蛮兵特有AI
}

void PiglinBruteEntity::registerAttributes()
{
    AbstractPiglinEntity::registerAttributes();

    // MC 1.16.5 PiglinBruteEntity 属性
    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 50.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.35);
    m_attributes.setBaseValue(entity::attribute::Attributes::ATTACK_DAMAGE, 7.0); // MC 1.16.5: 基础伤害（金斧额外 +4）
}

// ZombifiedPiglinEntity
ZombifiedPiglinEntity::ZombifiedPiglinEntity(LegacyEntityType type, EntityId id)
    : MonsterEntity(type, id)
{
    setBurnsInDaylight(false);
}

void ZombifiedPiglinEntity::tick()
{
    MonsterEntity::tick();

    if (m_angerTime > 0) {
        m_angerTime--;
        if (m_angerTime <= 0) {
            m_angry = false;
        }
    }
}

void ZombifiedPiglinEntity::registerGoals()
{
    MonsterEntity::registerGoals();
    // TODO: 添加僵尸猪灵特有AI
}

void ZombifiedPiglinEntity::registerAttributes()
{
    MonsterEntity::registerAttributes();

    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 20.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.23);
    m_attributes.setBaseValue(entity::attribute::Attributes::ATTACK_DAMAGE, 5.0);
}

// HoglinEntity
std::unique_ptr<Entity> HoglinEntity::create(IWorld* world)
{
    return std::make_unique<HoglinEntity>(LegacyEntityType::Hoglin, EntityId(0));
}

HoglinEntity::HoglinEntity(LegacyEntityType type, EntityId id)
    : MonsterEntity(type, id)
{
    setBurnsInDaylight(false);
    registerAttributes();
}

void HoglinEntity::tick()
{
    MonsterEntity::tick();

    if (m_attackCooldown > 0) {
        m_attackCooldown--;
    }

    if (m_attackAnimationTicks > 0) {
        m_attackAnimationTicks--;
    }
}

bool HoglinEntity::attackLivingTarget(LivingEntity& target)
{
    if (m_attackCooldown > 0) {
        return false;
    }

    m_attackCooldown = 20;
    m_attackAnimationTicks = 10;
    return entity::IFlinging::attackWithFling(*this, target, m_isBaby);
}

void HoglinEntity::registerGoals()
{
    MonsterEntity::registerGoals();
    // TODO: 添加疣猪兽特有AI
}

void HoglinEntity::registerAttributes()
{
    MonsterEntity::registerAttributes();

    // 注册攻击属性（MonsterEntity 不自动注册这些）
    m_attributes.registerAttribute(*entity::attribute::Attributes::attackDamage());
    m_attributes.registerAttribute(*entity::attribute::Attributes::attackKnockback());

    // MC 1.16.5 HoglinEntity 属性
    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 40.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.3);
    m_attributes.setBaseValue(entity::attribute::Attributes::KNOCKBACK_RESISTANCE, 0.6);
    m_attributes.setBaseValue(entity::attribute::Attributes::ATTACK_KNOCKBACK, 1.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::ATTACK_DAMAGE, 6.0); // MC 1.16.5: 成年疣猪兽基础伤害
}

// ZoglinEntity
std::unique_ptr<Entity> ZoglinEntity::create(IWorld* world)
{
    return std::make_unique<ZoglinEntity>(LegacyEntityType::Zoglin, EntityId(0));
}

ZoglinEntity::ZoglinEntity(LegacyEntityType type, EntityId id)
    : MonsterEntity(type, id)
{
    registerAttributes();
}

void ZoglinEntity::tick()
{
    MonsterEntity::tick();

    if (m_attackAnimationTicks > 0) {
        m_attackAnimationTicks--;
    }
}

bool ZoglinEntity::attackLivingTarget(LivingEntity& target)
{
    m_attackAnimationTicks = 10;
    return entity::IFlinging::attackWithFling(*this, target, m_isBaby);
}

void ZoglinEntity::registerGoals()
{
    MonsterEntity::registerGoals();
    // TODO: 添加僵尸疣兽特有AI
}

void ZoglinEntity::registerAttributes()
{
    MonsterEntity::registerAttributes();

    // 注册攻击属性（MonsterEntity 不自动注册这些）
    m_attributes.registerAttribute(*entity::attribute::Attributes::attackDamage());
    m_attributes.registerAttribute(*entity::attribute::Attributes::attackKnockback());

    // MC 1.16.5 ZoglinEntity 属性
    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 40.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.3);
    m_attributes.setBaseValue(entity::attribute::Attributes::KNOCKBACK_RESISTANCE, 0.6);
    m_attributes.setBaseValue(entity::attribute::Attributes::ATTACK_KNOCKBACK, 1.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::ATTACK_DAMAGE, 6.0); // MC 1.16.5: 成年僵尸疣兽基础伤害
}

} // namespace mc
