#include "TameableEntity.hpp"
#include "../../../core/Entity.hpp"
#include "../../../ai/goal/GoalSelector.hpp"

namespace mc {

TameableEntity::TameableEntity(LegacyEntityType type, EntityId id)
    : AgeableEntity(type, id) {
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
    AgeableEntity::tick();
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
    AgeableEntity::registerGoals();
}

} // namespace mc
