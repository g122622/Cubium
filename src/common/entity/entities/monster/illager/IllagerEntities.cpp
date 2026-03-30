#include "IllagerEntities.hpp"
#include "../../../attribute/Attributes.hpp"

namespace mc {

// AbstractIllagerEntity
AbstractIllagerEntity::AbstractIllagerEntity(LegacyEntityType type, EntityId id)
    : MonsterEntity(type, id)
{
}

void AbstractIllagerEntity::registerGoals() {
    MonsterEntity::registerGoals();
    // TODO: 添加灾厄村民基础AI
}

// AbstractRaiderEntity
AbstractRaiderEntity::AbstractRaiderEntity(LegacyEntityType type, EntityId id)
    : AbstractIllagerEntity(type, id)
{
}

void AbstractRaiderEntity::registerGoals() {
    AbstractIllagerEntity::registerGoals();
    // TODO: 添加袭击者基础AI
}

// PillagerEntity
PillagerEntity::PillagerEntity(LegacyEntityType type, EntityId id)
    : AbstractRaiderEntity(type, id)
{
}

void PillagerEntity::registerGoals() {
    AbstractRaiderEntity::registerGoals();
    // TODO: 添加掠夺者特有AI（弩攻击等）
}

void PillagerEntity::registerAttributes() {
    AbstractRaiderEntity::registerAttributes();

    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 24.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.35);
    m_attributes.setBaseValue(entity::attribute::Attributes::ATTACK_DAMAGE, 5.0);
}

// VindicatorEntity
VindicatorEntity::VindicatorEntity(LegacyEntityType type, EntityId id)
    : AbstractRaiderEntity(type, id)
{
}

void VindicatorEntity::registerGoals() {
    AbstractRaiderEntity::registerGoals();
    // TODO: 添加卫道士特有AI（斧攻击等）
}

void VindicatorEntity::registerAttributes() {
    AbstractRaiderEntity::registerAttributes();

    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 24.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.35);
    m_attributes.setBaseValue(entity::attribute::Attributes::ATTACK_DAMAGE, 9.0); // 铁斧伤害
}

} // namespace mc
