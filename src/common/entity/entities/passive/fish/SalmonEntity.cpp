#include "SalmonEntity.hpp"
#include "../../../attribute/Attributes.hpp"

namespace mc {

SalmonEntity::SalmonEntity(LegacyEntityType type, EntityId id)
    : AbstractFishEntity(type, id)
{
    // 设置群居参数
    setSchoolingRange(5.0f);
}

std::unique_ptr<Entity> SalmonEntity::create(IWorld* /*world*/) {
    return std::make_unique<SalmonEntity>(LegacyEntityType::Unknown, 0);
}

void SalmonEntity::registerAttributes() {
    // 调用父类方法
    AbstractFishEntity::registerAttributes();

    // 鲑鱼的属性
    // 参考 MC 1.16.5 鲑鱼属性
    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 3.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.35);
}

} // namespace mc
