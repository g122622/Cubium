#include "SkeletonEntity.hpp"

namespace mc {

SkeletonEntity::SkeletonEntity(LegacyEntityType type, EntityId id)
    : AbstractSkeletonEntity(type, id)
{
    registerGoals();
    registerAttributes();
}

std::unique_ptr<Entity> SkeletonEntity::create(IWorld* /*world*/)
{
    return std::make_unique<SkeletonEntity>(LegacyEntityType::Unknown, 0);
}

void SkeletonEntity::registerGoals()
{
    AbstractSkeletonEntity::registerGoals();
}

void SkeletonEntity::registerAttributes()
{
    AbstractSkeletonEntity::registerAttributes();
}

} // namespace mc
