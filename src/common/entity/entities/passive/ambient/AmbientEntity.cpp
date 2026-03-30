#include "AmbientEntity.hpp"
#include "../../attribute/Attributes.hpp"

namespace mc {

AmbientEntity::AmbientEntity(LegacyEntityType type, EntityId id)
    : MobEntity(type, id)
{
    // 注册属性
    registerAttributes();
}

void AmbientEntity::registerAttributes() {
    // 调用父类方法
    MobEntity::registerAttributes();

    // 环境生物的基础属性
    // 通常不需要设置额外属性
}

} // namespace mc
