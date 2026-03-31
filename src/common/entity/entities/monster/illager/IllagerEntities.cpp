#include "IllagerEntities.hpp"
#include "../../../attribute/Attributes.hpp"
#include "../../../../item/core/ItemStack.hpp"
#include "../../../core/LivingEntity.hpp"

namespace mc {

// PillagerEntity
PillagerEntity::PillagerEntity(LegacyEntityType type, EntityId id)
    : AbstractRaiderEntity(type, id)
{
}

void PillagerEntity::attackEntityWithRangedAttack(LivingEntity* target, f32 charge) {
    // TODO: 实现弩攻击逻辑
    (void)target;
    (void)charge;
}

void PillagerEntity::onCrossbowLoadComplete(ItemStack& crossbow) {
    // TODO: 实现弩装填完成逻辑
    (void)crossbow;
}

void PillagerEntity::shootCrossbow(LivingEntity* target, ItemStack& crossbow, f32 charge) {
    // TODO: 实现弩射击逻辑
    (void)target;
    (void)crossbow;
    (void)charge;
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
