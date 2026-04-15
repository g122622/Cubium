#include "AbstractSkeletonEntity.hpp"

#include "../../../attribute/Attributes.hpp"

namespace mc {

AbstractSkeletonEntity::AbstractSkeletonEntity(LegacyEntityType type, EntityId id)
    : MonsterEntity(type, id)
{
}

void AbstractSkeletonEntity::attackEntityWithRangedAttack(LivingEntity* target, f32 charge)
{
    if (target == nullptr) {
        return;
    }

    m_chargingBow = false;
    m_attackTimer = 0;
    m_attackCooldown = ATTACK_COOLDOWN;
    (void)charge;
}

void AbstractSkeletonEntity::tick()
{
    MonsterEntity::tick();

    if (m_attackCooldown > 0) {
        --m_attackCooldown;
    }

    if (m_attackTimer > 0) {
        --m_attackTimer;
        m_chargingBow = true;
        if (m_attackTimer == 0) {
            m_chargingBow = false;
        }
    }
}

void AbstractSkeletonEntity::registerGoals()
{
    MonsterEntity::registerGoals();

    // TODO: 接入 1.16.5 弓箭手专用目标：
    // - RangedBowAttackGoal
    // - FleeSunGoal
    // - RestrictSunGoal
}

void AbstractSkeletonEntity::registerAttributes()
{
    MonsterEntity::registerAttributes();

    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 20.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.25);
    m_attributes.setBaseValue(entity::attribute::Attributes::ATTACK_DAMAGE, ARROW_DAMAGE);
}

} // namespace mc
