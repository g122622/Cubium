#include "RandomWalkingGoal.hpp"
#include "../../../core/MobEntity.hpp"
#include "../../../core/CreatureEntity.hpp"
#include "../GoalConstants.hpp"
#include "../../pathfinding/PathNavigator.hpp"
#include "../../controller/MovementController.hpp"
#include "../../util/RandomPositionGenerator.hpp"
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

    // MC 1.16.5: 检查是否被骑乘
    if (m_creature->isBeingRidden()) return false;

    // 如果不需要强制更新，检查概率
    if (!m_forceUpdate) {
        // MC 1.16.5: 检查空闲时间（如果 m_checkIdleTime 为 true 且空闲时间 >= 100 则不执行）
        if (m_checkIdleTime && m_creature->idleTime() >= 100) return false;

        // MC 1.16.5: 检查执行概率
        math::Random rng = m_creature->getRandom();
        if (rng.nextInt(m_executionChance) != 0) return false;
    }

    // MC 1.16.5: 使用 RandomPositionGenerator.findRandomTarget(creature, 10, 7)
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

    // MC 1.16.5: 检查是否被骑乘
    if (m_creature->isBeingRidden()) return false;

    // MC 1.16.5: 继续执行直到路径完成
    // return !this.creature.getNavigator().noPath() && !this.creature.isBeingRidden();
    auto* nav = m_creature->navigator();
    if (nav) {
        return !nav->noPath();
    }

    return false;
}

void RandomWalkingGoal::startExecuting() {
    if (m_creature) {
        // MC 1.16.5: 使用 navigator.tryMoveToXYZ
        if (auto* nav = m_creature->navigator()) {
            nav->moveTo(m_targetX, m_targetY, m_targetZ, m_speed);
        }
    }
}

void RandomWalkingGoal::resetTask() {
    if (m_creature) {
        // MC 1.16.5: 清除路径
        m_creature->clearNavigation();
    }
    // 调用父类 resetTask
    Goal::resetTask();
}

void RandomWalkingGoal::tick() {
    // MC 1.16.5: RandomWalkingGoal 没有 tick 实现
}

bool RandomWalkingGoal::getRandomPosition(Vector3& outPos) {
    if (!m_creature) return false;

    // MC 1.16.5: 使用 RandomPositionGenerator.findRandomTarget(creature, 10, 7)
    // xzRange=10, yRange=7 是默认参数
    return util::RandomPositionGenerator::findRandomTarget(
        m_creature,
        RANDOM_WALK_RANGE,  // 10
        7,                   // 垂直范围
        outPos
    );
}

} // namespace mc::entity::ai::goal
