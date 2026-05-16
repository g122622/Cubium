/*
* Copyright (c) 2026 Guo Yi
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software being
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
#include "../../../../../util/assert/AssertMacros.hpp"
#include "../../../../../util/math/random/Random.hpp"
#include "../../../../../util/math/MathConstants.hpp"
#include "../../../../../util/math/MathUtils.hpp"
#include "../../../../../world/IWorld.hpp"
#include "../../../../core/CreatureEntity.hpp"
#include "../../../controller/MovementController.hpp"
#include "../../../pathfinding/PathNavigator.hpp"
#include "../../../util/RandomPositionGenerator.hpp"
#include <cmath>
#include <optional>

namespace mc::entity::ai::goal {

WaterAvoidingRandomFlyingGoal::WaterAvoidingRandomFlyingGoal(CreatureEntity* creature, f64 speed)
    : WaterAvoidingRandomFlyingGoal(creature, speed, 0.001f)
{
}

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
    if (m_creature == nullptr) {
        return false;
    }

    // MC 1.16.5: 检查是否被骑乘
    if (m_creature->isBeingRidden()) {
        return false;
    }

    // 检查执行概率
    if (m_chance > 0.0f) {
        math::Random rng = m_creature->getRandom();
        if (rng.nextFloat() >= m_chance) {
            return false;
        }
    }

    // 获取随机位置
    return getRandomPosition();
}

bool WaterAvoidingRandomFlyingGoal::shouldContinueExecuting()
{
    if (m_creature == nullptr) {
        return false;
    }

    // MC 1.16.5: 检查是否被骑乘
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
    if (m_creature == nullptr) {
        return;
    }

    // MC 1.16.5: 飞行目标使用导航器移动到目标位置
    if (auto* nav = m_creature->navigator()) {
        static_cast<void>(nav->moveTo(m_targetX, m_targetY, m_targetZ, m_speed));
    }
    m_timeout = MAX_TIMEOUT;
    m_isRunning = true;
}

void WaterAvoidingRandomFlyingGoal::resetTask()
{
    if (m_creature != nullptr) {
        m_creature->clearNavigation();
    }
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
    if (m_creature == nullptr) {
        return false;
    }

    // MC 1.16.5: WaterAvoidingRandomFlyingGoal.getRandomPosition()
    // 首先获取一个随机位置，然后检查是否在水中
    // 使用 RandomPositionGenerator.findRandomTargetBlock(creature, 8, 4, null)

    // 尝试多次找到不在水中的位置
    for (i32 attempt = 0; attempt < 10; ++attempt) {
        // 使用 RandomPositionGenerator 获取随机位置
        Vector3 targetPos;
        if (!util::RandomPositionGenerator::findRandomTargetBlock(m_creature, XZ_RANGE, Y_RANGE, std::nullopt, targetPos)) {
            continue;
        }

        // 检查位置是否在水或岩浆中
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
    if (m_creature == nullptr || m_creature->world() == nullptr) {
        return false;
    }

    IWorld* world = m_creature->world();
    const i32 blockX = math::floorTo<i32>(x);
    const i32 blockY = math::floorTo<i32>(y);
    const i32 blockZ = math::floorTo<i32>(z);
    const BlockPos pos(blockX, blockY, blockZ);

    return world->isWaterAt(pos) || world->isLavaAt(pos);
}

} // namespace mc::entity::ai::goal
