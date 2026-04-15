#include "SalmonEntity.hpp"

#include "../../../attribute/Attributes.hpp"

namespace mc {

SalmonEntity::SalmonEntity(LegacyEntityType type, EntityId id)
    : AbstractGroupFishEntity(type, id)
{
}

std::unique_ptr<Entity> SalmonEntity::create(IWorld* /*world*/)
{
    return std::make_unique<SalmonEntity>(LegacyEntityType::Unknown, 0);
}

void SalmonEntity::registerAttributes()
{
    AbstractGroupFishEntity::registerAttributes();
    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 3.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.35);
}

} // namespace mc
