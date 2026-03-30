#include "IronGolemEntity.hpp"
#include "../../../attribute/Attributes.hpp"

namespace mc {

IronGolemEntity::IronGolemEntity(LegacyEntityType type, EntityId id)
    : GolemEntity(type, id)
{
    // 注册 AI 目标
    registerGoals();

    // 注册属性
    registerAttributes();
}

std::unique_ptr<Entity> IronGolemEntity::create(IWorld* /*world*/) {
    return std::make_unique<IronGolemEntity>(LegacyEntityType::Unknown, 0);
}

void IronGolemEntity::tick() {
    GolemEntity::tick();

    // 更新攻击动画
    if (m_attackTimer > 0) {
        m_attackTimer--;
        m_armsRaised = true;
        if (m_attackTimer <= 0) {
            m_armsRaised = false;
        }
    }

    // 更新攻击冷却
    if (m_attackCooldown > 0) {
        m_attackCooldown--;
    }
}

void IronGolemEntity::registerGoals() {
    // 调用父类方法
    GolemEntity::registerGoals();

    // TODO: 铁傀儡 AI 目标
    // - IronGolemAttackGoal: 攻击敌对目标
    // - IronGolemDefendVillageGoal: 保护村庄
    // - IronGolemLookAtVillagerGoal: 看向村民
    // - IronGolemOfferFlowerGoal: 给村民送花
}

void IronGolemEntity::registerAttributes() {
    // 调用父类方法
    GolemEntity::registerAttributes();

    // 铁傀儡的属性
    // 参考 MC 1.16.5 铁傀儡属性
    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 100.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.25);
    m_attributes.setBaseValue(entity::attribute::Attributes::KNOCKBACK_RESISTANCE, 1.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::ATTACK_DAMAGE, ATTACK_DAMAGE);
}

} // namespace mc
