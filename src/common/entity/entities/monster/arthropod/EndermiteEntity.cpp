#include "EndermiteEntity.hpp"
#include "../../../attribute/Attributes.hpp"
#include "../../../core/EntityRegistry.hpp"
#include "../../../../world/IWorld.hpp"

namespace mc {

// EndermiteEntity
std::unique_ptr<Entity> EndermiteEntity::create(IWorld* world) {
    return std::make_unique<EndermiteEntity>(LegacyEntityType::Endermite, EntityId(0));
}

EndermiteEntity::EndermiteEntity(LegacyEntityType type, EntityId id)
    : MonsterEntity(type, id)
{
    m_burnsInDaylight = false;
}

void EndermiteEntity::tick() {
    MonsterEntity::tick();

    // 末影螨会在一定时间后消失
    if (!m_persistent) {
        m_lifetime++;
        if (m_lifetime > 2400) { // 120秒
            // TODO: 在正确实现后调用 discard()
        }
    }
}

void EndermiteEntity::registerGoals() {
    MonsterEntity::registerGoals();
    // TODO: 添加末影螨特有AI
}

void EndermiteEntity::registerAttributes() {
    MonsterEntity::registerAttributes();

    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 8.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.25);
    m_attributes.setBaseValue(entity::attribute::Attributes::ATTACK_DAMAGE, 2.0);
}

// SilverfishEntity
std::unique_ptr<Entity> SilverfishEntity::create(IWorld* world) {
    return std::make_unique<SilverfishEntity>(LegacyEntityType::Silverfish, EntityId(0));
}

SilverfishEntity::SilverfishEntity(LegacyEntityType type, EntityId id)
    : MonsterEntity(type, id)
{
}

void SilverfishEntity::registerGoals() {
    MonsterEntity::registerGoals();
    // TODO: 添加蠹虫特有AI
}

void SilverfishEntity::registerAttributes() {
    MonsterEntity::registerAttributes();

    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 8.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.25);
    m_attributes.setBaseValue(entity::attribute::Attributes::ATTACK_DAMAGE, 1.0);
}

} // namespace mc
