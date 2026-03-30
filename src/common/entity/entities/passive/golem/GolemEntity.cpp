#include "GolemEntity.hpp"
#include "../../../attribute/Attributes.hpp"

namespace mc {

GolemEntity::GolemEntity(LegacyEntityType type, EntityId id)
    : MobEntity(type, id)
{
    // 注册属性
    registerAttributes();
}

void GolemEntity::setRevengeTarget(LivingEntity* target) {
    m_attackTarget = target;
    if (target) {
        m_angerTime = MAX_ANGER_TIME;
    }
}

void GolemEntity::setAngry(bool angry) {
    if (angry) {
        m_angerTime = MAX_ANGER_TIME;
    } else {
        m_angerTime = 0;
        m_attackTarget = nullptr;
    }
}

void GolemEntity::tick() {
    MobEntity::tick();

    // 更新愤怒状态
    updateAnger();
}

void GolemEntity::registerAttributes() {
    // 调用父类方法
    MobEntity::registerAttributes();

    // 傀儡的基础属性
    // 子类会覆盖具体值
}

void GolemEntity::updateAnger() {
    if (m_angerTime > 0) {
        m_angerTime--;
        if (m_angerTime <= 0) {
            m_attackTarget = nullptr;
        }
    }
}

} // namespace mc
