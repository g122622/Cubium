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

#include "FlyGoal.hpp"

#include "common/core/EnumSet.hpp"
#include "common/entity/ai/controller/MovementController.hpp"
#include "common/entity/ai/goal/Goal.hpp"
#include "common/entity/ai/goal/GoalFlag.hpp"
#include "common/entity/ai/pathfinding/PathNavigator.hpp"
#include "common/entity/ai/util/RandomPositionGenerator.hpp"
#include "common/entity/core/CreatureEntity.hpp"
#include "common/entity/core/EntitySize.hpp"
#include "common/entity/core/EntityType.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/math/MathUtils.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/BlockPos.hpp"
#include <optional>

namespace mc::entity::ai::goal {

FlyGoal::FlyGoal(CreatureEntity* creature, f64 speed)
    : Goal(EnumSet<GoalFlag>{GoalFlag::Move})
    , m_creature(creature)
    , m_speed(speed)
{
    MC_ASSERT_RELEASE(creature != nullptr);
}

bool FlyGoal::shouldExecute()
{
    if (m_creature->isBeingRidden()) {
        return false;
    }

    // 概率检查
    math::Random& rng = m_creature->getRandom();
    if (rng.nextInt(EXECUTION_CHANCE) != 0) {
        return false;
    }

    return _generateFlightTarget();
}

bool FlyGoal::shouldContinueExecuting()
{
    // 检查超时
    if (m_timeout <= 0) {
        return false;
    }

    // 检查是否被骑乘
    if (m_creature->isBeingRidden()) {
        return false;
    }

    // 检查导航状态
    auto* nav = m_creature->navigator();
    if (nav && nav->hasPath() && !nav->isDone()) {
        return true;
    }

    // 检查移动控制器
    auto* moveCtrl = m_creature->moveController();
    if (moveCtrl && moveCtrl->isUpdating()) {
        return true;
    }

    return false;
}

void FlyGoal::startExecuting()
{
    auto* nav = m_creature->navigator();
    if (nav) {
        static_cast<void>(nav->moveTo(m_targetX, m_targetY, m_targetZ, m_speed));
    }
    m_timeout = MAX_FLIGHT_TIME;
}

void FlyGoal::resetTask()
{
    m_creature->clearNavigation();
    m_timeout = 0;
}

void FlyGoal::tick()
{
    if (m_timeout > 0) {
        --m_timeout;
    }
}

bool FlyGoal::_generateFlightTarget()
{
    IWorld* world = m_creature->world();
    if (world == nullptr) {
        return false;
    }

    // 使用RandomPositionGenerator的findRandomTargetBlock方法，
    // 该方法不要求位置可行走，适合飞行实体
    Vector3 targetPos;
    if (!util::RandomPositionGenerator::findRandomTargetBlock(
            m_creature, FLIGHT_XZ_RANGE, FLIGHT_Y_RANGE, std::nullopt, targetPos)) {
        return false;
    }

    // 检查目标位置是否在水或岩浆中，飞行实体应避开水面
    const i32 blockX = math::floorTo<i32>(targetPos.x);
    const i32 blockY = math::floorTo<i32>(targetPos.y);
    const i32 blockZ = math::floorTo<i32>(targetPos.z);
    const BlockPos pos(blockX, blockY, blockZ);

    if (world->isWaterAt(pos) || world->isLavaAt(pos)) {
        return false;
    }

    m_targetX = targetPos.x;
    m_targetY = targetPos.y;
    m_targetZ = targetPos.z;
    return true;
}

} // namespace mc::entity::ai::goal
