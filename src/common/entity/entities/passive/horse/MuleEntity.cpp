#include "MuleEntity.hpp"

#include "../../../attribute/Attributes.hpp"

namespace mc {

MuleEntity::MuleEntity(LegacyEntityType type, EntityId id)
    : AbstractChestedHorseEntity(type, id)
{
    setJumpStrength(0.5f);
}

std::unique_ptr<Entity> MuleEntity::create(IWorld* /*world*/)
{
    return std::make_unique<MuleEntity>(LegacyEntityType::Unknown, 0);
}

void MuleEntity::registerGoals()
{
    AbstractChestedHorseEntity::registerGoals();
    // TODO: 补齐骡专属 AI 目标。
}

void MuleEntity::registerAttributes()
{
    AbstractChestedHorseEntity::registerAttributes();
    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, m_horseHealth > 0 ? m_horseHealth : 20.0f);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, m_speed > 0 ? m_speed : 0.175f);
}

} // namespace mc
