#include "AbstractFishEntity.hpp"

#include "../../../attribute/Attributes.hpp"

namespace mc {

AbstractFishEntity::AbstractFishEntity(LegacyEntityType type, EntityId id)
    : WaterMobEntity(type, id)
{
    setMaxAirSupply(MAX_AIR_SUPPLY);
    setAirSupply(MAX_AIR_SUPPLY);

    registerGoals();
    registerAttributes();
}

void AbstractFishEntity::tick()
{
    WaterMobEntity::tick();
    updateSwimming();
    updateFlopping();
}

void AbstractFishEntity::registerGoals()
{
    // TODO: 对齐 1.16.5 的 PanicGoal、AvoidEntityGoal 和 RandomSwimmingGoal。
}

void AbstractFishEntity::registerAttributes()
{
    WaterMobEntity::registerAttributes();
    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 3.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.3);
}

void AbstractFishEntity::updateSwimming()
{
    if (isInWater()) {
        m_swimming = true;
        m_flopping = false;
        return;
    }

    m_swimming = false;
    m_flopping = true;
}

void AbstractFishEntity::updateFlopping()
{
    if (isInWater()) {
        m_flopTimer = 0;
        m_flopping = false;
        return;
    }

    ++m_flopTimer;
    if (m_flopTimer >= 100) {
        // TODO: 对齐 1.16.5 的离水跳动逻辑和声音触发。
        m_flopTimer = 0;
    }
}

} // namespace mc
