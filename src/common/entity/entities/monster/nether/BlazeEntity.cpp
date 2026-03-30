#include "BlazeEntity.hpp"
#include "../../../attribute/Attributes.hpp"

namespace mc {

BlazeEntity::BlazeEntity(LegacyEntityType type, EntityId id)
    : MonsterEntity(type, id)
{
    // 注册 AI 目标
    registerGoals();

    // 注册属性
    registerAttributes();
}

std::unique_ptr<Entity> BlazeEntity::create(IWorld* /*world*/) {
    return std::make_unique<BlazeEntity>(LegacyEntityType::Unknown, 0);
}

void BlazeEntity::attackEntityWithRangedAttack(LivingEntity* target, f32 charge) {
    // TODO: 发射火球
    (void)target;
    (void)charge;
}

void BlazeEntity::tick() {
    MonsterEntity::tick();

    // 更新攻击冷却
    if (m_attackCooldown > 0) {
        m_attackCooldown--;
    }

    // 更新发射状态
    if (m_charging) {
        if (m_fireballCount > 0) {
            m_fireballCount--;
            if (m_fireballCount <= 0) {
                m_charging = false;
                m_attackCooldown = ATTACK_COOLDOWN;
            }
        }
    }

    // 更新飞行状态
    // TODO: 检查是否在空中
}

void BlazeEntity::registerGoals() {
    // 调用父类方法
    MonsterEntity::registerGoals();

    // TODO: 烈焰人 AI 目标
    // - BlazeAttackGoal: 火球攻击
    // - BlazeFlyGoal: 飞行
}

void BlazeEntity::registerAttributes() {
    // 调用父类方法
    MonsterEntity::registerAttributes();

    // 烈焰人的属性
    // 参考 MC 1.16.5 烈焰人属性
    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 20.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.23);
    m_attributes.setBaseValue(entity::attribute::Attributes::ATTACK_DAMAGE, 6.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::FOLLOW_RANGE, 48.0);
}

} // namespace mc
