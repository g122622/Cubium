#include "GuardianEntity.hpp"
#include "../../../attribute/Attributes.hpp"
#include "../../../damage/DamageSource.hpp"
#include "../../../../sound/SoundEvents.hpp"

namespace mc {

GuardianEntity::GuardianEntity(LegacyEntityType type, EntityId id)
    : MonsterEntity(type, id)
{
    // 注册 AI 目标
    registerGoals();

    // 注册属性
    registerAttributes();
}

std::unique_ptr<Entity> GuardianEntity::create(IWorld* /*world*/) {
    return std::make_unique<GuardianEntity>(LegacyEntityType::Unknown, 0);
}

bool GuardianEntity::isInWater() const {
    // TODO: 检查是否在水中
    return false;
}

void GuardianEntity::tick() {
    MonsterEntity::tick();

    // 更新激光充能
    if (m_laserCharging && m_laserChargeTime > 0) {
        m_laserChargeTime--;
        if (m_laserChargeTime <= 0) {
            // 发射激光
            // TODO: 对目标造成伤害
            m_laserCharging = false;
        }
    }

    // 更新尖刺动画
    m_spikeTimer++;
    if (m_spikeTimer >= 40) {
        m_spikeTimer = 0;
        m_spikesRetracted = !m_spikesRetracted;
    }
}

void GuardianEntity::registerGoals() {
    // 调用父类方法
    MonsterEntity::registerGoals();

    // TODO: 守卫者 AI 目标
    // - GuardianAttackGoal: 激光攻击
    // - GuardianMoveGoal: 移动
}

void GuardianEntity::registerAttributes() {
    // 调用父类方法
    MonsterEntity::registerAttributes();

    // 守卫者的属性
    // 参考 MC 1.16.5 守卫者属性
    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 30.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.3);
    m_attributes.setBaseValue(entity::attribute::Attributes::ATTACK_DAMAGE, LASER_DAMAGE);
    m_attributes.setBaseValue(entity::attribute::Attributes::FOLLOW_RANGE, 16.0);
}

std::optional<ResourceLocation> GuardianEntity::getAmbientSound() const {
    // MC 1.16.5: 在水中和陆地使用不同音效
    if (isInWater()) {
        return SoundEvents::ENTITY_GUARDIAN_AMBIENT;
    }
    return SoundEvents::ENTITY_GUARDIAN_AMBIENT_LAND;
}

std::optional<ResourceLocation> GuardianEntity::getHurtSound(DamageSource& /*source*/) const {
    // MC 1.16.5: 在水中和陆地使用不同音效
    if (isInWater()) {
        return SoundEvents::ENTITY_GUARDIAN_HURT;
    }
    return SoundEvents::ENTITY_GUARDIAN_HURT_LAND;
}

std::optional<ResourceLocation> GuardianEntity::getDeathSound() const {
    // MC 1.16.5: 在水中和陆地使用不同音效
    if (isInWater()) {
        return SoundEvents::ENTITY_GUARDIAN_DEATH;
    }
    return SoundEvents::ENTITY_GUARDIAN_DEATH_LAND;
}

void GuardianEntity::playLaserSound() {
    playSound(SoundEvents::ENTITY_GUARDIAN_ATTACK, 1.0f, 1.0f);
}

} // namespace mc
