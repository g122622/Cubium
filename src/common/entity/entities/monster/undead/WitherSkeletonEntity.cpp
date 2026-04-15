#include "WitherSkeletonEntity.hpp"

#include "../../../attribute/Attributes.hpp"

namespace mc {

WitherSkeletonEntity::WitherSkeletonEntity(LegacyEntityType type, EntityId id)
    : AbstractSkeletonEntity(type, id)
{
    registerGoals();
    registerAttributes();
}

std::unique_ptr<Entity> WitherSkeletonEntity::create(IWorld* /*world*/)
{
    return std::make_unique<WitherSkeletonEntity>(LegacyEntityType::Unknown, 0);
}

void WitherSkeletonEntity::registerGoals()
{
    AbstractSkeletonEntity::registerGoals();

    // TODO: 切到 1.16.5 近战目标集，替换弓箭手行为。
}

void WitherSkeletonEntity::registerAttributes()
{
    AbstractSkeletonEntity::registerAttributes();

    m_attributes.setBaseValue(entity::attribute::Attributes::ATTACK_DAMAGE, 4.0);
}

} // namespace mc
