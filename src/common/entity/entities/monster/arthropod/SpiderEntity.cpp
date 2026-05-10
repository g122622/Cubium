#include "SpiderEntity.hpp"
#include "../../../attribute/Attributes.hpp"
#include "../../../../world/IWorld.hpp"
#include "../../../../world/block/BlockPos.hpp"
#include <cmath>

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
    // MC 1.16.5: 蜘蛛只在黑暗中攻击
    // 参考 SpiderEntity.shouldAttack() 第94-101行
    // 光照等级 < 7 时攻击（亮度阈值 = 0.5 * 15 = 7.5）
    if (m_world != nullptr) {
        u8 lightLevel = m_world->getLightSubtracted(BlockPos(
            static_cast<i32>(std::floor(m_position.x)),
            static_cast<i32>(std::floor(m_position.y)),
            static_cast<i32>(std::floor(m_position.z))), 0);
        if (lightLevel < 7) {
            return MonsterEntity::shouldAttack(target);
        }
        // 在明亮处不攻击
        return false;
    }
    return MonsterEntity::shouldAttack(target);
}

void SpiderEntity::tick() {
    MonsterEntity::tick();

    // MC 1.16.5: 更新攀爬状态
    // 参考 SpiderEntity.tick() 第67-72行
    // 蜘蛛在碰到墙壁时可以攀爬
    m_climbing = collidedHorizontally();

    m_wasOnGround = onGround();
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
