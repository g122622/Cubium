#include "StrayEntity.hpp"

namespace mc {

StrayEntity::StrayEntity(LegacyEntityType type, EntityId id)
    : AbstractSkeletonEntity(type, id)
{
    registerGoals();
    registerAttributes();
}

std::unique_ptr<Entity> StrayEntity::create(IWorld* /*world*/)
{
    return std::make_unique<StrayEntity>(LegacyEntityType::Unknown, 0);
}

void StrayEntity::registerAttributes()
{
    AbstractSkeletonEntity::registerAttributes();
}

} // namespace mc
