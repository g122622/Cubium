#include "SpiderEntity.hpp"
#include "../../../attribute/Attributes.hpp"
#include "../../../world/World.hpp"

namespace mc {

SpiderEntity::SpiderEntity(LegacyEntityType type, EntityId id)
    : MonsterEntity(type, id)
{
    // 注册 AI 目标
    registerGoals();

    // 注册属性
    registerAttributes();
}

std::unique_ptr<Entity> SpiderEntity::create(IWorld* /*world*/) {
    return std::make_unique<SpiderEntity>(LegacyEntityType::Unknown, 0);
}

bool SpiderEntity::shouldAttack(LivingEntity* target) const {
    // 蜘蛛只在黑暗中攻击
    // TODO: 检查光照等级
    // return world()->getLightLevel(position()) < 7;
    return MonsterEntity::shouldAttack(target);
}

void SpiderEntity::tick() {
    MonsterEntity::tick();

    // 更新攀爬状态
    // TODO: 检查是否贴着墙壁
    // if (isCollidedHorizontally) {
    //     m_climbing = true;
    // } else {
    //     m_climbing = false;
    // }

    m_wasOnGround = isOnGround();
}

void SpiderEntity::registerGoals() {
    // 调用父类方法
    MonsterEntity::registerGoals();

    // TODO: 蜘蛛 AI 目标
    // - SpiderAttackGoal: 近战攻击
    // - SpiderTargetGoal: 目标选择
    // - LeapAtTargetGoal: 跳向目标
}

void SpiderEntity::registerAttributes() {
    // 调用父类方法
    MonsterEntity::registerAttributes();

    // 蜘蛛的属性
    // 参考 MC 1.16.5 蜘蛛属性
    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 16.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.3);
    m_attributes.setBaseValue(entity::attribute::Attributes::ATTACK_DAMAGE, 2.0);
}

} // namespace mc
