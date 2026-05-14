#include "HuskEntity.hpp"
#include "../../../attribute/Attributes.hpp"

namespace mc {

HuskEntity::HuskEntity(LegacyEntityType type, EntityId id)
    : ZombieEntity(type, id)
{
    // 注册属性
    registerAttributes();
}

std::unique_ptr<Entity> HuskEntity::create(IWorld* /*world*/)
{
    return std::make_unique<HuskEntity>(LegacyEntityType::Unknown, 0);
}

void HuskEntity::registerAttributes()
{
    // 调用父类方法
    ZombieEntity::registerAttributes();

    // 尸壳的属性与僵尸相同
}

} // namespace mc
