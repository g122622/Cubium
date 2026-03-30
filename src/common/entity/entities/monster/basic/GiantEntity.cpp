#include "GiantEntity.hpp"
#include "../../../attribute/Attributes.hpp"
#include "../../../core/EntityRegistry.hpp"
#include <memory>

namespace mc {

GiantEntity::GiantEntity(LegacyEntityType type, EntityId id)
    : MonsterEntity(type, id)
{
    // 巨人体型巨大 - 通过 width()/height() 设置
}

std::unique_ptr<Entity> GiantEntity::create(IWorld* /*world*/) {
    return std::make_unique<GiantEntity>(LegacyEntityType::Unknown, 0);
}

void GiantEntity::tick() {
    MonsterEntity::tick();

    // 巨人没有特殊的tick逻辑
    // 原版巨人没有AI
}

void GiantEntity::registerGoals() {
    // 原版巨人没有AI目标
    // 不调用 MonsterEntity::registerGoals()
}

void GiantEntity::registerAttributes() {
    MonsterEntity::registerAttributes();

    // 巨人属性
    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 100.0f);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.5f);
    m_attributes.setBaseValue(entity::attribute::Attributes::ATTACK_DAMAGE, 50.0f);
    m_attributes.setBaseValue(entity::attribute::Attributes::FOLLOW_RANGE, 40.0f);
}

} // namespace mc
