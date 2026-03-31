#include "DonkeyEntity.hpp"
#include "MuleEntity.hpp"
#include "../../../attribute/Attributes.hpp"
#include "../../../core/EntityRegistry.hpp"
#include "../../../../item/core/ItemStack.hpp"
#include <memory>

namespace mc {

DonkeyEntity::DonkeyEntity(LegacyEntityType type, EntityId id)
    : AbstractHorseEntity(type, id)
{
    // 驴的跳跃强度较低
    setJumpStrength(0.5f);
}

std::unique_ptr<Entity> DonkeyEntity::create(IWorld* /*world*/) {
    return std::make_unique<DonkeyEntity>(LegacyEntityType::Unknown, 0);
}

bool DonkeyEntity::isBreedingItem(const ItemStack& itemStack) const {
    // TODO: 检查是否是金苹果或金胡萝卜
    (void)itemStack;
    return false;
}

std::unique_ptr<AnimalEntity> DonkeyEntity::spawnBaby(AnimalEntity& partner) {
    // TODO: 创建后代
    // 如果与马繁殖，产生骡
    // 如果与驴繁殖，产生驴
    // auto baby = std::make_unique<MuleEntity>(LegacyEntityType::Unknown, 0);
    // baby->setChild(true);
    // return baby;
    (void)partner;
    return nullptr;
}

void DonkeyEntity::registerGoals() {
    AbstractHorseEntity::registerGoals();
    // TODO: 驴特有 AI 目标
}

void DonkeyEntity::registerAttributes() {
    AbstractHorseEntity::registerAttributes();

    // 驴的属性比马稍低
    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, m_horseHealth > 0 ? m_horseHealth : 15.0f);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, m_speed > 0 ? m_speed : 0.175f);
}

} // namespace mc
