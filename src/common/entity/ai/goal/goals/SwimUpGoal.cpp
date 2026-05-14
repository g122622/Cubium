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

#include "SwimUpGoal.hpp"
#include "../../../../world/IWorld.hpp"
#include "../../../../world/block/Block.hpp"
#include "../../../../world/block/BlockPos.hpp"
#include "../../../../world/block/Material.hpp"
#include "../../../core/CreatureEntity.hpp"
#include "../../../core/Entity.hpp"
#include "../../../core/MobEntity.hpp"
#include "../../pathfinding/PathNavigator.hpp"

namespace mc::entity::ai::goal {

SwimUpGoal::SwimUpGoal(CreatureEntity* creature, f64 speed, i32 targetY)
    : Goal(EnumSet<GoalFlag>{GoalFlag::Move})
    , m_creature(creature)
    , m_speed(speed)
    , m_targetY(targetY)
    , m_originalTargetY(targetY)
{
    MC_ASSERT(creature != nullptr);
}

bool SwimUpGoal::shouldExecute()
{
    if (m_creature == nullptr) {
        return false;
    }

    // 只在水中执行
    if (!m_creature->isInWater()) {
        return false;
    }

    // 如果已经设置了目标Y且已到达，不执行
    if (m_originalTargetY >= 0) {
        if (m_creature->y() >= static_cast<f64>(m_originalTargetY)) {
            return false;
        }
    }

    // 如果目标Y未设置，自动检测水面
    if (m_originalTargetY < 0) {
        IWorld* world = m_creature->world();
        if (world == nullptr) {
            return false;
        }

        BlockPos pos(
            static_cast<i32>(m_creature->x()), static_cast<i32>(m_creature->y()), static_cast<i32>(m_creature->z()));
        // 向上搜索水面
        for (i32 dy = 0; dy <= 10; ++dy) {
            BlockPos checkPos(pos.x, pos.y + dy, pos.z);
            const BlockState* state = world->getBlockState(checkPos);
            if (state == nullptr || !state->getMaterial().isLiquid()) {
                // 找到水面
                m_targetY = pos.y + dy;
                return true;
            }
        }
        return false;
    }

    m_targetY = m_originalTargetY;
    return true;
}

bool SwimUpGoal::shouldContinueExecuting()
{
    if (m_creature == nullptr) {
        return false;
    }

    // 必须仍在水中
    if (!m_creature->isInWater()) {
        return false;
    }

    // 超时检查
    if (m_timeoutCounter <= 0) {
        return false;
    }

    // 如果已到达目标，停止
    if (hasReachedTarget()) {
        return false;
    }

    return m_active;
}

void SwimUpGoal::startExecuting()
{
    if (m_creature == nullptr) {
        return;
    }

    m_active = true;
    m_timeoutCounter = 400; // 最大游泳时间（20秒）

    // 移动到目标高度
    f64 currentX = m_creature->x();
    f64 currentZ = m_creature->z();
    m_creature->tryMoveTo(currentX, static_cast<f64>(m_targetY), currentZ, m_speed);
}

void SwimUpGoal::resetTask()
{
    m_active = false;
    m_timeoutCounter = 0;
    m_targetY = m_originalTargetY;
}

void SwimUpGoal::tick()
{
    if (m_creature == nullptr) {
        return;
    }

    m_timeoutCounter--;

    // 向上游动
    // MC 1.16.5: 在水中向上移动
    if (m_creature->isInWater() && !hasReachedTarget()) {
        // 添加向上的速度
        Vector3 vel = m_creature->velocity();
        vel.y = static_cast<f32>(m_speed) * 0.1f; // 向上的推力
        m_creature->setVelocity(vel);
    }
}

bool SwimUpGoal::hasReachedTarget() const
{
    if (m_creature == nullptr) {
        return true;
    }

    return m_creature->y() >= static_cast<f64>(m_targetY);
}

} // namespace mc::entity::ai::goal
