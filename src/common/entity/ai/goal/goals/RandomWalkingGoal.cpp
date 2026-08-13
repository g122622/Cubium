/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "RandomWalkingGoal.hpp"
#include "../../../../util/math/random/Random.hpp"
#include "../../../core/CreatureEntity.hpp"
#include "../../../core/MobEntity.hpp"
#include "../../pathfinding/PathNavigator.hpp"
#include "../../util/RandomPositionGenerator.hpp"
#include "../GoalConstants.hpp"
#include "common/core/EnumSet.hpp"
#include "common/entity/ai/goal/Goal.hpp"
#include "common/entity/ai/goal/GoalFlag.hpp"
#include "common/entity/core/DataParameter.hpp"
#include "common/entity/core/EntitySize.hpp"

namespace mc::entity::ai::goal {

using namespace constants;

RandomWalkingGoal::RandomWalkingGoal(CreatureEntity* creature, f64 speed)
    : RandomWalkingGoal(creature, speed, 120, true)
{}

RandomWalkingGoal::RandomWalkingGoal(CreatureEntity* creature, f64 speed, i32 chance)
    : RandomWalkingGoal(creature, speed, chance, true)
{}

RandomWalkingGoal::RandomWalkingGoal(CreatureEntity* creature, f64 speed, i32 chance, bool checkIdleTime)
    : m_creature(creature)
    , m_speed(speed)
    , m_executionChance(chance)
    , m_checkIdleTime(checkIdleTime)
{
    setMutexFlags(EnumSet<GoalFlag>{GoalFlag::Move});
}

bool RandomWalkingGoal::shouldExecute()
{
    // creature 为 null 时安全返回 false（与 WaterAvoidingRandomWalkingGoal 保持一致）
    if (!m_creature) return false;

    // 检查是否被骑乘
    if (m_creature->isBeingRidden()) return false;

    // 如果不需要强制更新，检查概率
    if (!m_forceUpdate) {
        // 检查空闲时间（如果 m_checkIdleTime 为 true 且空闲时间 >= 100 则不执行）
        if (m_checkIdleTime && m_creature->idleTime() >= 100) return false;

        // 检查执行概率。对齐 vanilla RandomStrollGoal.canUse：
        // nextInt(reducedTickDelay(interval))，interval 默认 120。
        // reducedTickDelay 把门槛减半补偿 GoalSelector 半 tick 评估。
        math::Random& rng = m_creature->getRandom();
        if (rng.nextInt(reducedTickDelay(m_executionChance)) != 0) return false;
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

bool RandomWalkingGoal::shouldContinueExecuting()
{
    // creature 为 null 时安全返回 false（与 WaterAvoidingRandomWalkingGoal 保持一致）
    if (!m_creature) return false;

    // 检查是否被骑乘
    if (m_creature->isBeingRidden()) return false;

    // 检查超时
    if (m_timeoutCounter <= 0) return false;

    // 继续执行直到路径完成
    auto* nav = m_creature->navigator();
    return nav && !nav->noPath();
}

void RandomWalkingGoal::startExecuting()
{
    // creature 为 null 时直接返回（与 WaterAvoidingRandomWalkingGoal 保持一致）
    if (!m_creature) return;

    // 使用 navigator.tryMoveToXYZ
    if (auto* nav = m_creature->navigator()) {
        static_cast<void>(nav->moveTo(m_targetX, m_targetY, m_targetZ, m_speed));
    }
    // 初始化超时计数器
    m_timeoutCounter = MAX_WALK_TIME;
}

void RandomWalkingGoal::resetTask()
{
    // creature 为 null 时直接返回（与 WaterAvoidingRandomWalkingGoal 保持一致）
    if (!m_creature) return;

    // 清除路径
    m_creature->clearNavigation();
    // 调用父类 resetTask
    Goal::resetTask();
}

void RandomWalkingGoal::tick()
{
    // 减少超时计数器
    if (m_timeoutCounter > 0) {
        --m_timeoutCounter;
    }
}

bool RandomWalkingGoal::getRandomPosition(Vector3& outPos)
{
    // creature 为 null 时安全返回 false（与 WaterAvoidingRandomWalkingGoal 保持一致）
    if (!m_creature) return false;

    // 使用 RandomPositionGenerator.findRandomTarget(creature, 10, 7)
    // xzRange=10, yRange=7 是默认参数
    return util::RandomPositionGenerator::findRandomTarget(m_creature,
        RANDOM_WALK_RANGE, // 10
        7,                 // 垂直范围
        outPos);
}

} // namespace mc::entity::ai::goal
