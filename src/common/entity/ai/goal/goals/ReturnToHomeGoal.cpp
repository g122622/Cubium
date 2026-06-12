/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to Use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software be
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

#include "ReturnToHomeGoal.hpp"

#include "common/entity/ai/pathfinding/PathNavigator.hpp"
#include "common/entity/ai/util/RandomPositionGenerator.hpp"
#include "common/entity/core/CreatureEntity.hpp"
#include "common/entity/core/MobEntity.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/math/MathUtils.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"

namespace mc::entity::ai::goal {

ReturnToHomeGoal::ReturnToHomeGoal(CreatureEntity* creature, f64 speed, f32 homeRadius)
    : Goal(EnumSet<GoalFlag>{GoalFlag::Move})
    , m_creature(creature)
    , m_speed(speed)
    , m_homeRadius(homeRadius)
{
    MC_ASSERT_RELEASE(creature != nullptr);
}

bool ReturnToHomeGoal::shouldExecute()
{
    if (m_creature->isBeingRidden()) {
        return false;
    }

    // 只在家园区系统启用且生物离开家园范围时执行
    auto* mob = dynamic_cast<MobEntity*>(m_creature);
    if (mob == nullptr) {
        return false;
    }

    if (!mob->hasHome()) {
        return false;
    }

    // 检查是否在家园范围内
    if (mob->isWithinHomeDistanceCurrentPosition()) {
        return false;
    }

    // 生成朝向家园位置的随机目标
    Vector3 targetPos;
    if (!util::RandomPositionGenerator::findRandomTargetTowards(m_creature,
            HOME_XZ_RANGE,
            HOME_Y_RANGE,
            Vector3(static_cast<f32>(mob->homePosition().x),
                static_cast<f32>(mob->homePosition().y),
                static_cast<f32>(mob->homePosition().z)),
            targetPos)) {
        return false;
    }

    m_targetX = targetPos.x;
    m_targetY = targetPos.y;
    m_targetZ = targetPos.z;
    return true;
}

bool ReturnToHomeGoal::shouldContinueExecuting()
{
    auto* mob = dynamic_cast<MobEntity*>(m_creature);
    if (mob == nullptr) {
        return false;
    }

    // 已经回到家园范围内，停止
    if (mob->isWithinHomeDistanceCurrentPosition()) {
        return false;
    }

    // 导航仍在进行中
    auto* nav = m_creature->navigator();
    if (nav && nav->hasPath() && !nav->isDone()) {
        return true;
    }

    return false;
}

void ReturnToHomeGoal::startExecuting()
{
    static_cast<void>(m_creature->navigator()->moveTo(m_targetX, m_targetY, m_targetZ, m_speed));
    m_pathRecalcTimer = 0;
}

void ReturnToHomeGoal::resetTask()
{
    m_creature->clearNavigation();
    m_pathRecalcTimer = 0;
}

void ReturnToHomeGoal::tick()
{
    if (m_pathRecalcTimer > 0) {
        --m_pathRecalcTimer;
        return;
    }

    // 定期重新计算路径
    _recalculatePath();
    m_pathRecalcTimer = PATH_RECALC_INTERVAL;
}

void ReturnToHomeGoal::_recalculatePath()
{
    auto* mob = dynamic_cast<MobEntity*>(m_creature);
    if (mob == nullptr || !mob->hasHome()) {
        return;
    }

    // 生成朝向家园位置的新路径
    Vector3 targetPos;
    if (util::RandomPositionGenerator::findRandomTargetTowards(m_creature,
            HOME_XZ_RANGE,
            HOME_Y_RANGE,
            Vector3(static_cast<f32>(mob->homePosition().x),
                static_cast<f32>(mob->homePosition().y),
                static_cast<f32>(mob->homePosition().z)),
            targetPos)) {
        m_targetX = targetPos.x;
        m_targetY = targetPos.y;
        m_targetZ = targetPos.z;
        static_cast<void>(m_creature->navigator()->moveTo(m_targetX, m_targetY, m_targetZ, m_speed));
    }
}

} // namespace mc::entity::ai::goal
