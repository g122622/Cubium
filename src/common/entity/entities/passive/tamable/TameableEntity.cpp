#include "TameableEntity.hpp"
#include "../../../core/Entity.hpp"
#include "../../../ai/goal/GoalSelector.hpp"
#include "../../../attribute/Attributes.hpp"

namespace mc {

TameableEntity::TameableEntity(LegacyEntityType type, EntityId id)
    : AnimalEntity(type, id) {
    // 注册属性
    registerAttributes();
}

void TameableEntity::setTamed(bool tamed) {
    if (m_tamed != tamed) {
        m_tamed = tamed;
        onTamed(tamed);
    }
}

void TameableEntity::setSitting(bool sitting) {
    if (m_sitting != sitting) {
        m_sitting = sitting;
        // 坐下时停止移动
        if (sitting) {
            clearNavigation();
        }
    }
}

void TameableEntity::setAttackTarget(LivingEntity* target) {
    m_attackTarget = target;
    if (target != nullptr) {
        setAngerTime(MAX_ANGER_TIME);
    }
}

void TameableEntity::setRevengeTarget(LivingEntity* target) {
    if (target != nullptr) {
        m_revengeTargetId = target->id();
        setAngerTime(MAX_ANGER_TIME);
    } else {
        m_revengeTargetId = std::nullopt;
    }
}

void TameableEntity::setAngry(bool angry) {
    if (angry) {
        setAngerTime(MAX_ANGER_TIME);
    } else {
        setAngerTime(0);
        m_attackTarget = nullptr;
        m_revengeTargetId = std::nullopt;
    }
}

void TameableEntity::tick() {
    AnimalEntity::tick();
    updateAnger();
}

void TameableEntity::updateAnger() {
    if (m_angerTime > 0) {
        --m_angerTime;
        if (m_angerTime <= 0) {
            // 愤怒时间结束，清除攻击目标
            m_attackTarget = nullptr;
            m_revengeTargetId = std::nullopt;
        }
    }
}

void TameableEntity::registerGoals() {
    // 基础目标由子类添加
    // 子类应该调用此方法然后添加自己的目标
    AnimalEntity::registerGoals();
}

void TameableEntity::registerAttributes() {
    // 调用父类方法
    AnimalEntity::registerAttributes();

    // 驯服动物的基础属性（子类可以覆盖）
    // 参考 MC 1.16.5 TameableEntity
    // 大多数驯服动物的属性由子类设置
}

} // namespace mc
