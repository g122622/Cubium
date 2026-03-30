#include "WitchEntity.hpp"
#include "../../../attribute/Attributes.hpp"

namespace mc {

WitchEntity::WitchEntity(LegacyEntityType type, EntityId id)
    : MonsterEntity(type, id)
{
    // 注册 AI 目标
    registerGoals();

    // 注册属性
    registerAttributes();
}

std::unique_ptr<Entity> WitchEntity::create(IWorld* /*world*/) {
    return std::make_unique<WitchEntity>(LegacyEntityType::Unknown, 0);
}

bool WitchEntity::needsHealing() const {
    f32 currentHealth = static_cast<f32>(m_attributes.getValue(entity::attribute::Attributes::MAX_HEALTH));
    f32 maxHealth = static_cast<f32>(m_attributes.getBaseValue(entity::attribute::Attributes::MAX_HEALTH));
    return currentHealth < maxHealth * HEAL_THRESHOLD;
}

void WitchEntity::tryDrinkHealingPotion() {
    if (!m_drinking && needsHealing()) {
        m_drinking = true;
        m_drinkTimer = DRINK_DURATION;
    }
}

void WitchEntity::tick() {
    MonsterEntity::tick();

    // 更新喝药水状态
    if (m_drinking && m_drinkTimer > 0) {
        m_drinkTimer--;
        if (m_drinkTimer <= 0) {
            // 喝完药水，恢复生命值
            // TODO: 应用治疗效果
            m_drinking = false;
        }
    }

    // 更新攻击冷却
    if (m_attackCooldown > 0) {
        m_attackCooldown--;
    }

    // 检查是否需要治疗
    if (needsHealing() && !m_drinking && m_attackCooldown <= 0) {
        tryDrinkHealingPotion();
    }
}

void WitchEntity::registerGoals() {
    // 调用父类方法
    MonsterEntity::registerGoals();

    // TODO: 女巫 AI 目标
    // - WitchAttackGoal: 药水攻击
    // - WitchDrinkPotionGoal: 喝药水
}

void WitchEntity::registerAttributes() {
    // 调用父类方法
    MonsterEntity::registerAttributes();

    // 女巫的属性
    // 参考 MC 1.16.5 女巫属性
    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 26.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.25);
}

} // namespace mc
