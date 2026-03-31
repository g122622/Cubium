#include "PhantomEntity.hpp"
#include "../../../attribute/Attributes.hpp"
#include "../../../core/EntityRegistry.hpp"
#include "../../../../world/IWorld.hpp"

namespace mc {

std::unique_ptr<Entity> PhantomEntity::create(IWorld* world) {
    return std::make_unique<PhantomEntity>(LegacyEntityType::Phantom, EntityId(0));
}

PhantomEntity::PhantomEntity(LegacyEntityType type, EntityId id)
    : MonsterEntity(type, id)
{
    // 幻翼不燃烧在阳光下
    setBurnsInDaylight(false);
}

void PhantomEntity::tick() {
    MonsterEntity::tick();

    // 更新攻击冷却
    if (m_attackCooldown > 0) {
        m_attackCooldown--;
    }

    // 更新飞行状态
    updateFlight();
}

void PhantomEntity::registerGoals() {
    MonsterEntity::registerGoals();

    // TODO: 添加幻翼特有的AI目标
    // - PhantomAttackGoal
    // - PhantomSweepAttackGoal
}

void PhantomEntity::registerAttributes() {
    MonsterEntity::registerAttributes();

    // 幻翼属性
    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 20.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.0); // 幻翼飞行，不使用地面速度
    m_attributes.setBaseValue(entity::attribute::Attributes::ATTACK_DAMAGE, 6.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::FLYING_SPEED, 0.9);
}

void PhantomEntity::updateFlight() {
    // 更新环绕角度
    if (m_attackPhase == AttackPhase::CIRCLE) {
        m_circleAngle += 0.1f;
        if (m_circleAngle > 6.28f) { // 2 * PI
            m_circleAngle -= 6.28f;
        }
    }
}

} // namespace mc
