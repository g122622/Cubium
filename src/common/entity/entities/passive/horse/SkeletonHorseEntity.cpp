#include "SkeletonHorseEntity.hpp"
#include "../../../attribute/Attributes.hpp"
#include "../../../core/EntityRegistry.hpp"
#include <memory>

namespace mc {

SkeletonHorseEntity::SkeletonHorseEntity(LegacyEntityType type, EntityId id)
    : AbstractHorseEntity(type, id)
{
    // 骷髅马默认已驯服
    setTame(true);
    // 设置跳跃强度
    setJumpStrength(1.0f);
}

std::unique_ptr<Entity> SkeletonHorseEntity::create(IWorld* /*world*/) {
    return std::make_unique<SkeletonHorseEntity>(LegacyEntityType::Unknown, 0);
}

bool SkeletonHorseEntity::canBeRiddenBy(Player* player) const {
    // 骷髅马不需要驯服即可骑乘
    if (m_rider != nullptr && m_rider != player) {
        return false;
    }
    return true;
}

void SkeletonHorseEntity::tick() {
    AbstractHorseEntity::tick();

    // 检查阳光燃烧
    if (shouldBurnInDaylight()) {
        // TODO: 实现阳光燃烧逻辑
    }
}

void SkeletonHorseEntity::registerGoals() {
    AbstractHorseEntity::registerGoals();
    // 骷髅马没有额外 AI
}

void SkeletonHorseEntity::registerAttributes() {
    AbstractHorseEntity::registerAttributes();

    // 骷髅马的属性
    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 15.0f);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.2f);
}

} // namespace mc
