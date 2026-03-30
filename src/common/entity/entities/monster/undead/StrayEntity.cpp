#include "StrayEntity.hpp"
#include "../../../attribute/Attributes.hpp"

namespace mc {

StrayEntity::StrayEntity(LegacyEntityType type, EntityId id)
    : SkeletonEntity(type, id)
{
    // 注册属性
    registerAttributes();
}

std::unique_ptr<Entity> StrayEntity::create(IWorld* /*world*/) {
    return std::make_unique<StrayEntity>(LegacyEntityType::Unknown, 0);
}

void StrayEntity::registerAttributes() {
    // 调用父类方法
    SkeletonEntity::registerAttributes();

    // 流浪者的属性与骷髅相同
}

} // namespace mc
