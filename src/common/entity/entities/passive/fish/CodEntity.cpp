#include "CodEntity.hpp"
#include "../../../attribute/Attributes.hpp"

namespace mc {

CodEntity::CodEntity(LegacyEntityType type, EntityId id)
    : AbstractFishEntity(type, id)
{
    // 设置群居参数
    setSchoolingRange(5.0f);
}

std::unique_ptr<Entity> CodEntity::create(IWorld* /*world*/) {
    return std::make_unique<CodEntity>(LegacyEntityType::Unknown, 0);
}

void CodEntity::registerAttributes() {
    // 调用父类方法
    AbstractFishEntity::registerAttributes();

    // 鳕鱼的属性
    // 参考 MC 1.16.5 鳕鱼属性
    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 3.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.3);
}

} // namespace mc
