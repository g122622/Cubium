#include "CodEntity.hpp"

#include "../../../attribute/Attributes.hpp"

namespace mc {

CodEntity::CodEntity(LegacyEntityType type, EntityId id)
    : AbstractGroupFishEntity(type, id)
{
}

std::unique_ptr<Entity> CodEntity::create(IWorld* /*world*/)
{
    return std::make_unique<CodEntity>(LegacyEntityType::Unknown, 0);
}

void CodEntity::registerAttributes()
{
    AbstractGroupFishEntity::registerAttributes();
    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 3.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.3);
}

} // namespace mc
