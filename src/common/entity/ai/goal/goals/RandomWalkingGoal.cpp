#include "RandomWalkingGoal.hpp"
#include "../../../core/MobEntity.hpp"
#include "../../../core/CreatureEntity.hpp"
#include "../GoalConstants.hpp"
#include "../../pathfinding/PathNavigator.hpp"
#include "../../controller/MovementController.hpp"
#include "../../../../util/math/random/Random.hpp"
#include <cmath>

namespace mc::entity::ai::goal {

using namespace constants;

RandomWalkingGoal::RandomWalkingGoal(CreatureEntity* creature, f64 speed)
    : RandomWalkingGoal(creature, speed, 120, true)
{
}

RandomWalkingGoal::RandomWalkingGoal(CreatureEntity* creature, f64 speed, i32 chance)
    : RandomWalkingGoal(creature, speed, chance, true)
{
}

RandomWalkingGoal::RandomWalkingGoal(CreatureEntity* creature, f64 speed, i32 chance, bool checkIdleTime)
    : m_creature(creature)
    , m_speed(speed)
    , m_executionChance(chance)
    , m_checkIdleTime(checkIdleTime)
{
    setMutexFlags(EnumSet<GoalFlag>{GoalFlag::Move});
}

bool RandomWalkingGoal::shouldExecute() {
    if (!m_creature) return false;

    // 检查是否被骑乘
    if (m_creature->isBeingRidden()) return false;

    // 如果不需要强制更新，检查概率
    if (!m_forceUpdate) {
        // MC 1.16.5: 检查空闲时间（如果 m_checkIdleTime 为 true 且空闲时间 >= 100 则不执行）
        if (m_checkIdleTime && m_creature->idleTime() >= 100) return false;

        // 检查执行概率
        math::Random rng = m_creature->getRandom();
        if (rng.nextInt(m_executionChance) != 0) return false;
    }

    // 获取随机位置
    Vector3 targetPos;
    if (getRandomPosition(targetPos)) {
        m_targetX = targetPos.x;
        m_targetY = targetPos.y;
        m_targetZ = targetPos.z;
        m_forceUpdate = false;
        return true;
    }

    return false;
}

bool RandomWalkingGoal::shouldContinueExecuting() {
    if (!m_creature) return false;

    // 检查是否被骑乘
    if (m_creature->isBeingRidden()) return false;

    // 如果有导航路径且未完成，继续执行
    auto* nav = m_creature->navigator();
    if (nav && nav->hasPath() && !nav->isDone()) {
        return m_timeoutCounter > 0;
    }

    // 如果没有路径，检查移动控制器是否仍在更新
    // MovementController的MoveTo状态表示仍在移动
    auto* moveCtrl = m_creature->moveController();
    if (moveCtrl && moveCtrl->isUpdating()) {
        return m_timeoutCounter > 0;
    }

    return m_timeoutCounter > 0;
}

void RandomWalkingGoal::startExecuting() {
    if (m_creature) {
        m_creature->tryMoveTo(m_targetX, m_targetY, m_targetZ, m_speed);
        m_timeoutCounter = MAX_WALK_TIME;
    }
}

void RandomWalkingGoal::resetTask() {
    if (m_creature) {
        m_creature->clearNavigation();
    }
    m_timeoutCounter = 0;
}

void RandomWalkingGoal::tick() {
    if (m_timeoutCounter > 0) {
        m_timeoutCounter--;
    }
}

bool RandomWalkingGoal::getRandomPosition(Vector3& outPos) {
    if (!m_creature) return false;

    // 使用实体的随机数生成器
    math::Random rng = m_creature->getRandom();

    f32 x = m_creature->x() + (rng.nextFloat() * 2.0f - 1.0f) * RANDOM_WALK_RANGE;
    f32 z = m_creature->z() + (rng.nextFloat() * 2.0f - 1.0f) * RANDOM_WALK_RANGE;

    // Y坐标需要找到地面
    // 简化实现：使用当前位置
    f32 y = m_creature->y();

    outPos = Vector3(x, y, z);
    return true;
}

} // namespace mc::entity::ai::goal
