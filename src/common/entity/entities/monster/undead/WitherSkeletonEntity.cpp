#include "WitherSkeletonEntity.hpp"
#include "../../../attribute/Attributes.hpp"

namespace mc {

WitherSkeletonEntity::WitherSkeletonEntity(LegacyEntityType type, EntityId id)
    : SkeletonEntity(type, id)
{
    // 注册 AI 目标
    registerGoals();

    // 注册属性
    registerAttributes();
}

std::unique_ptr<Entity> WitherSkeletonEntity::create(IWorld* /*world*/) {
    return std::make_unique<WitherSkeletonEntity>(LegacyEntityType::Unknown, 0);
}

void WitherSkeletonEntity::registerGoals() {
    // 调用父类方法
    SkeletonEntity::registerGoals();

    // TODO: 凋灵骷髅使用近战攻击而不是远程攻击
    // 移除远程攻击目标，添加近战攻击目标
}

void WitherSkeletonEntity::registerAttributes() {
    // 调用父类方法
    SkeletonEntity::registerAttributes();

    // 凋灵骷髅的属性
    // 参考 MC 1.16.5 凋灵骷髅属性
    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 20.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.25);
    m_attributes.setBaseValue(entity::attribute::Attributes::ATTACK_DAMAGE, 4.0); // 石剑 + 凋灵效果
}

} // namespace mc
