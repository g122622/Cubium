#include "SkeletonEntity.hpp"
#include "../../../attribute/Attributes.hpp"

namespace mc {

SkeletonEntity::SkeletonEntity(LegacyEntityType type, EntityId id)
    : MonsterEntity(type, id)
{
    // 注册 AI 目标
    registerGoals();

    // 注册属性
    registerAttributes();
}

std::unique_ptr<Entity> SkeletonEntity::create(IWorld* /*world*/) {
    return std::make_unique<SkeletonEntity>(LegacyEntityType::Unknown, 0);
}

void SkeletonEntity::attackEntityWithRangedAttack(LivingEntity* target, f32 charge) {
    // TODO: 发射箭矢
    // 发射一支箭，伤害基于拉弓程度
    (void)target;
    (void)charge;
}

void SkeletonEntity::tick() {
    MonsterEntity::tick();

    // 更新攻击冷却
    if (m_attackCooldown > 0) {
        m_attackCooldown--;
    }

    // 更新攻击计时器
    if (m_attackTimer > 0) {
        m_attackTimer--;
        m_chargingBow = true;
        if (m_attackTimer <= 0) {
            m_chargingBow = false;
        }
    }
}

void SkeletonEntity::registerGoals() {
    // 调用父类方法
    MonsterEntity::registerGoals();

    // TODO: 骷髅 AI 目标
    // - RangedBowAttackGoal: 弓箭攻击
    // - FleeSunGoal: 避开阳光
    // - RestrictSunGoal: 限制阳光
}

void SkeletonEntity::registerAttributes() {
    // 调用父类方法
    MonsterEntity::registerAttributes();

    // 骷髅的属性
    // 参考 MC 1.16.5 骷髅属性
    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 20.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.25);
    m_attributes.setBaseValue(entity::attribute::Attributes::ATTACK_DAMAGE, 2.0);
}

} // namespace mc
