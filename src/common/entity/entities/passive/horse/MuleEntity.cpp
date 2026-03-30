#include "MuleEntity.hpp"
#include "../../../attribute/Attributes.hpp"
#include "../../../core/EntityRegistry.hpp"
#include <memory>

namespace mc {

MuleEntity::MuleEntity(LegacyEntityType type, EntityId id)
    : AbstractHorseEntity(type, id)
{
    // 骡的跳跃强度适中
    setJumpStrength(0.5f);
}

std::unique_ptr<Entity> MuleEntity::create(IWorld* /*world*/) {
    return std::make_unique<MuleEntity>(LegacyEntityType::Unknown, 0);
}

void MuleEntity::registerGoals() {
    AbstractHorseEntity::registerGoals();
    // TODO: 骡特有 AI 目标
}

void MuleEntity::registerAttributes() {
    AbstractHorseEntity::registerAttributes();

    // 骡的属性介于马和驴之间
    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, m_health > 0 ? m_health : 20.0f);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, m_speed > 0 ? m_speed : 0.175f);
}

} // namespace mc
