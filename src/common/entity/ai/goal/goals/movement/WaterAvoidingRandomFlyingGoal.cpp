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

#include "WaterAvoidingRandomFlyingGoal.hpp"
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
#include "common/util/math/MathConstants.hpp"
#include "common/util/math/MathUtils.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include <cmath>
#include <optional>

namespace mc::entity::ai::goal {

WaterAvoidingRandomFlyingGoal::WaterAvoidingRandomFlyingGoal(CreatureEntity* creature, f64 speed)
    : WaterAvoidingRandomFlyingGoal(creature, speed, 0.001f)
{}

WaterAvoidingRandomFlyingGoal::WaterAvoidingRandomFlyingGoal(CreatureEntity* creature, f64 speed, f32 chance)
    : Goal(EnumSet<GoalFlag>{GoalFlag::Move})
    , m_creature(creature)
    , m_speed(speed)
    , m_chance(chance)
{
    MC_ASSERT_RELEASE(creature != nullptr);
}

bool WaterAvoidingRandomFlyingGoal::shouldExecute()
{
    // 检查是否被骑乘
    if (m_creature->isBeingRidden()) {
        return false;
    }

    // 检查执行概率
    if (m_chance > 0.0f) {
        math::Random& rng = m_creature->getRandom();
        if (rng.nextFloat() >= m_chance) {
            return false;
        }
    }

    // 获取随机位置
    return getRandomPosition();
}

bool WaterAvoidingRandomFlyingGoal::shouldContinueExecuting()
{
    // 检查是否被骑乘
    if (m_creature->isBeingRidden()) {
        return false;
    }

    // 检查超时
    if (m_timeout <= 0) {
        return false;
    }

    // 检查导航路径
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

void WaterAvoidingRandomFlyingGoal::startExecuting()
{
    // 飞行目标使用导航器移动到目标位置
    if (auto* nav = m_creature->navigator()) {
        static_cast<void>(nav->moveTo(m_targetX, m_targetY, m_targetZ, m_speed));
    }
    m_timeout = MAX_TIMEOUT;
    m_isRunning = true;
}

void WaterAvoidingRandomFlyingGoal::resetTask()
{
    m_creature->clearNavigation();
    m_isRunning = false;
    m_timeout = 0;
}

void WaterAvoidingRandomFlyingGoal::tick()
{
    if (m_timeout > 0) {
        --m_timeout;
    }
}

bool WaterAvoidingRandomFlyingGoal::getRandomPosition()
{
    // 对应 MC 1.21.11 WaterAvoidingRandomFlyingGoal.getPosition()
    // 使用实体朝向作为飞行方向
    f32 yaw = m_creature->yaw() * math::DEG_TO_RAD;
    f64 dirX = -std::sin(static_cast<f64>(yaw));
    f64 dirZ = std::cos(static_cast<f64>(yaw));

    // 主策略：HoverRandomPos（在固体方块上方悬停）
    Vector3 targetPos;
    if (util::RandomPositionGenerator::findHoverPosition(
            m_creature, XZ_RANGE, Y_RANGE_HOVER, dirX, dirZ, math::HALF_PI, 3, 1, targetPos)) {
        if (!isInWaterOrLava(targetPos.x, targetPos.y, targetPos.z)) {
            m_targetX = targetPos.x;
            m_targetY = targetPos.y;
            m_targetZ = targetPos.z;
            return true;
        }
    }

    // 备选策略：AirAndWaterRandomPos（在空中选择位置）
    if (util::RandomPositionGenerator::findAirAndWaterPosition(
            m_creature, XZ_RANGE, Y_RANGE_FALLBACK, Y_OFFSET_FALLBACK, dirX, dirZ, math::HALF_PI, targetPos)) {
        if (!isInWaterOrLava(targetPos.x, targetPos.y, targetPos.z)) {
            m_targetX = targetPos.x;
            m_targetY = targetPos.y;
            m_targetZ = targetPos.z;
            return true;
        }
    }

    // 最终回退：使用简单的随机飞行目标搜索
    for (i32 attempt = 0; attempt < 10; ++attempt) {
        if (!util::RandomPositionGenerator::findRandomTargetBlock(
                m_creature, XZ_RANGE, Y_RANGE_FALLBACK, std::nullopt, targetPos)) {
            continue;
        }

        if (isInWaterOrLava(targetPos.x, targetPos.y, targetPos.z)) {
            continue;
        }

        m_targetX = targetPos.x;
        m_targetY = targetPos.y;
        m_targetZ = targetPos.z;
        return true;
    }

    return false;
}

bool WaterAvoidingRandomFlyingGoal::isInWaterOrLava(f64 x, f64 y, f64 z) const
{
    IWorld* world = m_creature->world();
    if (world == nullptr) {
        return false;
    }

    const i32 blockX = math::floorTo<i32>(x);
    const i32 blockY = math::floorTo<i32>(y);
    const i32 blockZ = math::floorTo<i32>(z);
    const BlockPos pos(blockX, blockY, blockZ);

    return world->isWaterAt(pos) || world->isLavaAt(pos);
}

} // namespace mc::entity::ai::goal
