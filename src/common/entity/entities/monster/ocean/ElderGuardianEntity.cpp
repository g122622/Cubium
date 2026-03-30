#include "ElderGuardianEntity.hpp"
#include "../../../attribute/Attributes.hpp"

namespace mc {

ElderGuardianEntity::ElderGuardianEntity(LegacyEntityType type, EntityId id)
    : GuardianEntity(type, id)
{
    // 注册属性
    registerAttributes();
}

std::unique_ptr<Entity> ElderGuardianEntity::create(IWorld* /*world*/) {
    return std::make_unique<ElderGuardianEntity>(LegacyEntityType::Unknown, 0);
}

void ElderGuardianEntity::tick() {
    GuardianEntity::tick();

    // 更新挖掘疲劳
    m_fatigueTimer++;
    if (m_fatigueTimer >= FATIGUE_INTERVAL) {
        m_fatigueTimer = 0;
        // TODO: 给附近的玩家挖掘疲劳效果
    }
}

void ElderGuardianEntity::registerAttributes() {
    // 调用父类方法
    GuardianEntity::registerAttributes();

    // 远古守卫者的属性
    // 参考 MC 1.16.5 远古守卫者属性
    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 80.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.3);
    m_attributes.setBaseValue(entity::attribute::Attributes::ATTACK_DAMAGE, 5.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::FOLLOW_RANGE, 16.0);
}

} // namespace mc
