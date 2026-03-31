#include "AbstractFireballEntity.hpp"
#include "../../core/LivingEntity.hpp"
#include "../../damage/DamageSource.hpp"
#include "../../../world/IWorld.hpp"
#include <cmath>

namespace mc {
namespace entity {

// ============================================================================
// AbstractFireballEntity
// ============================================================================

AbstractFireballEntity::AbstractFireballEntity(LegacyEntityType type, EntityId id)
    : ProjectileEntity(type, id)
{
    m_noGravity = true;  // 火球默认不受重力影响
}

void AbstractFireballEntity::tick() {
    // 调用父类tick
    ProjectileEntity::tick();

    // 火球使用加速度持续加速
    m_velocity.x += m_accelerationX;
    m_velocity.y += m_accelerationY;
    m_velocity.z += m_accelerationZ;

    // 限制最大速度
    f32 speed = std::sqrt(m_velocity.x * m_velocity.x +
                          m_velocity.y * m_velocity.y +
                          m_velocity.z * m_velocity.z);
    f32 maxSpeed = 10.0f;  // 最大速度
    if (speed > maxSpeed) {
        f32 ratio = maxSpeed / speed;
        m_velocity.x *= ratio;
        m_velocity.y *= ratio;
        m_velocity.z *= ratio;
    }

    // 火球粒子效果
    // TODO: 生成火焰粒子
}

// ============================================================================
// FireballEntity
// ============================================================================

FireballEntity::FireballEntity(LegacyEntityType type, EntityId id)
    : AbstractFireballEntity(type, id)
{
    m_damage = 6.0f;
}

std::unique_ptr<Entity> FireballEntity::create(IWorld* /*world*/) {
    return std::make_unique<FireballEntity>(LegacyEntityType::Unknown, 0);
}

void FireballEntity::onEntityHit(const RayTraceResult& result) {
    if (!result.hitEntity) {
        return;
    }

    // 造成伤害
    mc::Entity* shooter = getShooter();
    std::unique_ptr<DamageSource> damageSource;
    if (shooter) {
        damageSource = std::make_unique<IndirectEntityDamageSource>(
            DamageType::Fireball, shooter, this, false);
    } else {
        damageSource = std::make_unique<IndirectEntityDamageSource>(
            DamageType::Fireball, this, this, false);
    }

    // TODO: target->attackEntityFrom(*damageSource, 6.0f);

    // 点燃目标
    result.hitEntity->setFire(5);

    // 爆炸
    // TODO: world->createExplosion(this, x(), y(), z(), m_explosionPower, ...);

    remove();
}

void FireballEntity::onBlockHit(const RayTraceResult& result) {
    // 爆炸
    // TODO: world->createExplosion(this, x(), y(), z(), m_explosionPower, ...);
    remove();
}

// ============================================================================
// SmallFireballEntity
// ============================================================================

SmallFireballEntity::SmallFireballEntity(LegacyEntityType type, EntityId id)
    : AbstractFireballEntity(type, id)
{
    m_damage = 5.0f;
}

std::unique_ptr<Entity> SmallFireballEntity::create(IWorld* /*world*/) {
    return std::make_unique<SmallFireballEntity>(LegacyEntityType::Unknown, 0);
}

void SmallFireballEntity::onEntityHit(const RayTraceResult& result) {
    if (!result.hitEntity) {
        return;
    }

    // 造成伤害
    mc::Entity* shooter = getShooter();
    std::unique_ptr<DamageSource> damageSource;
    if (shooter) {
        damageSource = std::make_unique<IndirectEntityDamageSource>(
            DamageType::Fireball, shooter, this, false);
    } else {
        damageSource = std::make_unique<IndirectEntityDamageSource>(
            DamageType::Fireball, this, this, false);
    }

    // TODO: target->attackEntityFrom(*damageSource, 5.0f);

    // 点燃目标
    if (!result.hitEntity->isOnFire()) {
        result.hitEntity->setFire(5);
    }

    remove();
}

void SmallFireballEntity::onBlockHit(const RayTraceResult& /*result*/) {
    // 小火球不爆炸，直接消失
    // TODO: 可能点燃方块
    remove();
}

// ============================================================================
// DragonFireballEntity
// ============================================================================

DragonFireballEntity::DragonFireballEntity(LegacyEntityType type, EntityId id)
    : AbstractFireballEntity(type, id)
{
    m_damage = 12.0f;
}

std::unique_ptr<Entity> DragonFireballEntity::create(IWorld* /*world*/) {
    return std::make_unique<DragonFireballEntity>(LegacyEntityType::Unknown, 0);
}

void DragonFireballEntity::onEntityHit(const RayTraceResult& result) {
    if (!result.hitEntity) {
        return;
    }

    // 造成伤害
    // TODO: target->attackEntityFrom(DamageSources::dragonBreath(this), 12.0f);

    // 生成龙息区域效果云
    // TODO: 生成 AreaEffectCloudEntity

    remove();
}

void DragonFireballEntity::onBlockHit(const RayTraceResult& /*result*/) {
    // 生成龙息区域效果云
    // TODO: 生成 AreaEffectCloudEntity
    remove();
}

// ============================================================================
// WitherSkullEntity
// ============================================================================

WitherSkullEntity::WitherSkullEntity(LegacyEntityType type, EntityId id)
    : AbstractFireballEntity(type, id)
{
    m_damage = 8.0f;
}

std::unique_ptr<Entity> WitherSkullEntity::create(IWorld* /*world*/) {
    return std::make_unique<WitherSkullEntity>(LegacyEntityType::Unknown, 0);
}

void WitherSkullEntity::onEntityHit(const RayTraceResult& result) {
    if (!result.hitEntity) {
        return;
    }

    // 造成伤害（凋零效果）
    mc::Entity* shooter = getShooter();
    std::unique_ptr<DamageSource> damageSource;
    if (shooter) {
        damageSource = std::make_unique<IndirectEntityDamageSource>(
            DamageType::Magic, shooter, this, false);
    } else {
        damageSource = std::make_unique<IndirectEntityDamageSource>(
            DamageType::Magic, this, this, false);
    }

    // TODO: target->attackEntityFrom(*damageSource, 8.0f);

    // 凋零效果
    // if (target instanceof LivingEntity) {
    //     ((LivingEntity)target).addEffect(new WitherEffect(10 * 20, 1));
    // }

    // 爆炸
    // TODO: world->createExplosion(this, x(), y(), z(), m_blue ? 1.0f : 0.0f, ...);

    remove();
}

void WitherSkullEntity::onBlockHit(const RayTraceResult& /*result*/) {
    // 蓝色凋灵之首会破坏方块
    // TODO: world->createExplosion(this, x(), y(), z(), m_blue ? 1.0f : 0.0f, ...);
    remove();
}

} // namespace entity
} // namespace mc
