#include "MonsterEntity.hpp"
#include "../../core/MobEntity.hpp"
#include "../../ai/goal/GoalSelector.hpp"
#include "../../ai/goal/goals/SwimGoal.hpp"
#include "../../ai/goal/goals/target/TargetGoals.hpp"
#include "../../attribute/Attributes.hpp"

namespace mc {

MonsterEntity::MonsterEntity(LegacyEntityType type, EntityId id)
    : CreatureEntity(type, id)
{
    // 注册 AI 目标
    registerGoals();
}

bool MonsterEntity::isInDaylight() const {
    // TODO: 检查是否在阳光下
    // return world() && world()->isDaytime() && world()->canSeeSky(position());
    return false;
}

bool MonsterEntity::shouldAttack(LivingEntity* target) const {
    // 默认实现：攻击所有活着的生物
    return target != nullptr && target->isAlive();
}

void MonsterEntity::tick() {
    CreatureEntity::tick();

    // 处理阳光燃烧
    handleDaylightBurning();
}

void MonsterEntity::registerGoals() {
    // 调用父类方法
    CreatureEntity::registerGoals();

    // 敌对生物基础 AI
    // 优先级 0: 游泳（最高优先级）
    m_goalSelector.addGoal(0, new entity::ai::goal::SwimGoal(this));

    // 目标选择器
    // 优先级 1: 被攻击后反击
    m_targetSelector.addGoal(1, new entity::ai::goal::HurtByTargetGoal(this, false));

    // 注意: 具体的攻击目标选择需要子类添加
    // 例如僵尸会添加 NearestAttackableTargetGoal<Player>
}

void MonsterEntity::handleDaylightBurning() {
    if (m_burnsInDaylight && isInDaylight()) {
        // 在阳光下燃烧
        m_burnTime++;
        if (m_burnTime >= 20) {  // 1秒后开始燃烧
            // TODO: 造成火焰伤害
            // damage(DamageSource::ON_FIRE, 1.0f);
        }
    } else {
        m_burnTime = 0;
    }
}

} // namespace mc
