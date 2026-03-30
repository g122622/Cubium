#include "AnimalEntity.hpp"
#include "../../../ai/goal/goals/SwimGoal.hpp"
#include "../../../ai/goal/goals/PanicGoal.hpp"
#include "../../../ai/goal/goals/BreedGoal.hpp"
#include "../../../ai/goal/goals/TemptGoal.hpp"
#include "../../../ai/goal/goals/FollowParentGoal.hpp"
#include "../../../ai/goal/goals/RandomWalkingGoal.hpp"
#include "../../../ai/goal/goals/LookAtGoal.hpp"

namespace mc {

AnimalEntity::AnimalEntity(LegacyEntityType type, EntityId id)
    : AgeableEntity(type, id)
{
}

bool AnimalEntity::isBreedingItem(const ItemStack& /*itemStack*/) const {
    // 默认实现：没有繁殖物品
    // 子类应该重写此方法
    return false;
}

bool AnimalEntity::canMateWith(const AnimalEntity& other) const {
    // 基本条件：
    // 1. 两个都是成体
    // 2. 两个都处于爱心状态
    // 3. 不是同一个实体
    // 4. 是同一种动物（子类可以重写来放宽此条件）

    if (this == &other) {
        return false;
    }

    if (isChild() || other.isChild()) {
        return false;
    }

    if (!isInLove() || !other.isInLove()) {
        return false;
    }

    // 检查是否是同一种动物
    // TODO: 比较 EntityType
    return true;
}

void AnimalEntity::tick() {
    AgeableEntity::tick();

    updateInLove();
}

void AnimalEntity::registerGoals() {
    // 调用父类方法
    AgeableEntity::registerGoals();

    // 基础动物 AI 目标
    // 优先级 0: 游泳（最高优先级）
    m_goalSelector.addGoal(0, new entity::ai::goal::SwimGoal(this));

    // 优先级 1: 恐慌逃跑（受到伤害时）
    m_goalSelector.addGoal(1, new entity::ai::goal::PanicGoal(this, 1.25));

    // 优先级 2: 繁殖（当处于爱心状态时）
    m_goalSelector.addGoal(2, new entity::ai::goal::BreedGoal(this, 1.0));

    // 优先级 3: 食物诱惑（当玩家手持食物时）
    // 子类需要设置食物检测谓词
    // m_goalSelector.addGoal(3, new entity::ai::goal::TemptGoal(this, 1.0, foodPredicate));

    // 优先级 4: 跟随父母（幼体行为）
    m_goalSelector.addGoal(4, new entity::ai::goal::FollowParentGoal(this, 1.0));

    // 优先级 5: 随机漫步
    m_goalSelector.addGoal(5, new entity::ai::goal::RandomWalkingGoal(this, 1.0));

    // 优先级 6: 看向玩家
    m_goalSelector.addGoal(6, new entity::ai::goal::LookAtGoal(this, 6.0f));

    // 优先级 7: 随机看向
    m_goalSelector.addGoal(7, new entity::ai::goal::LookRandomlyGoal(this));
}

void AnimalEntity::updateInLove() {
    if (m_inLoveTimer > 0) {
        --m_inLoveTimer;

        if (m_inLoveTimer == 0) {
            resetInLove();
        }
    }
}

void AnimalEntity::resetInLove() {
    m_inLoveTimer = 0;
    m_loveCause = 0;
}

} // namespace mc
