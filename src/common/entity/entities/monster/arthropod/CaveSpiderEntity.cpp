#include "CaveSpiderEntity.hpp"
#include "../../../attribute/Attributes.hpp"

namespace mc {

CaveSpiderEntity::CaveSpiderEntity(LegacyEntityType type, EntityId id)
    : SpiderEntity(type, id)
{
    // 注册属性
    registerAttributes();
}

std::unique_ptr<Entity> CaveSpiderEntity::create(IWorld* /*world*/)
{
    return std::make_unique<CaveSpiderEntity>(LegacyEntityType::Unknown, 0);
}

void CaveSpiderEntity::registerAttributes()
{
    // 调用父类方法
    SpiderEntity::registerAttributes();

    // 洞穴蜘蛛的属性
    // 参考 MC 1.16.5 洞穴蜘蛛属性
    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 12.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.3);
    m_attributes.setBaseValue(entity::attribute::Attributes::ATTACK_DAMAGE, 2.0);
}

} // namespace mc
