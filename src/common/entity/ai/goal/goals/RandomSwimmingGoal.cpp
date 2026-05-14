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

#include "RandomSwimmingGoal.hpp"
#include "../../../../util/assert/AssertAll.hpp"
#include "../../../../util/math/random/Random.hpp"
#include "../../../../world/IWorld.hpp"
#include "../../../../world/block/Block.hpp"
#include "../../../../world/block/BlockPos.hpp"
#include "../../../../world/block/Material.hpp"
#include "../../../core/CreatureEntity.hpp"
#include "../../../core/Entity.hpp"
#include "../../../core/MobEntity.hpp"
#include "../../pathfinding/PathNavigator.hpp"

namespace mc::entity::ai::goal {

RandomSwimmingGoal::RandomSwimmingGoal(CreatureEntity* creature, f64 speed)
    : Goal(EnumSet<GoalFlag>{GoalFlag::Move})
    , m_creature(creature)
    , m_speed(speed)
    , m_executionChance(120)
{
    MC_ASSERT_RELEASE(creature != nullptr);
}

RandomSwimmingGoal::RandomSwimmingGoal(CreatureEntity* creature, f64 speed, i32 chance)
    : Goal(EnumSet<GoalFlag>{GoalFlag::Move})
    , m_creature(creature)
    , m_speed(speed)
    , m_executionChance(chance)
{
    MC_ASSERT_RELEASE(creature != nullptr);
}

bool RandomSwimmingGoal::shouldExecute()
{
    if (m_creature == nullptr) {
        return false;
    }

    // 只在水中执行
    if (!m_creature->isInWater()) {
        return false;
    }

    // 随机概率执行
    if (m_executionChance > 0 && m_creature->getRandom().nextInt(m_executionChance) != 0) {
        return false;
    }

    // 获取随机游泳位置
    Vector3 targetPos;
    if (getRandomSwimPosition(targetPos)) {
        m_targetX = targetPos.x;
        m_targetY = targetPos.y;
        m_targetZ = targetPos.z;
        return true;
    }

    return false;
}

bool RandomSwimmingGoal::shouldContinueExecuting()
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

    // 检查是否有导航器和有效路径
    auto* mob = dynamic_cast<MobEntity*>(m_creature);
    if (mob == nullptr) {
        return false;
    }
    return mob->navigator() != nullptr && mob->navigator()->hasPath();
}

void RandomSwimmingGoal::startExecuting()
{
    if (m_creature == nullptr) {
        return;
    }

    m_creature->tryMoveTo(m_targetX, m_targetY, m_targetZ, m_speed);
    m_timeoutCounter = 600; // 最大游泳时间（30秒）
}

void RandomSwimmingGoal::resetTask()
{
    m_timeoutCounter = 0;
    m_forceUpdate = false;
}

void RandomSwimmingGoal::tick()
{
    if (m_creature == nullptr) {
        return;
    }

    m_timeoutCounter--;

    // 如果到达目标或超时，尝试新的目标
    if (m_timeoutCounter > 0 && m_forceUpdate) {
        Vector3 targetPos;
        if (getRandomSwimPosition(targetPos)) {
            m_targetX = targetPos.x;
            m_targetY = targetPos.y;
            m_targetZ = targetPos.z;
            m_creature->tryMoveTo(m_targetX, m_targetY, m_targetZ, m_speed);
        }
        m_forceUpdate = false;
    }
}

bool RandomSwimmingGoal::getRandomSwimPosition(Vector3& outPos)
{
    if (m_creature == nullptr || m_creature->world() == nullptr) {
        return false;
    }

    // MC 1.16.5: 使用 RandomPositionGenerator.findRandomTargetBlockInWater
    // 简化实现：在当前实体附近随机选择水中的位置
    math::Random rng = m_creature->getRandom();

    f64 currentX = m_creature->x();
    f64 currentY = m_creature->y();
    f64 currentZ = m_creature->z();

    // 在实体周围 10 格范围内随机搜索
    for (i32 attempt = 0; attempt < 10; ++attempt) {
        i32 dx = rng.nextInt(20) - 10;
        i32 dy = rng.nextInt(20) - 10;
        i32 dz = rng.nextInt(20) - 10;

        f64 targetX = currentX + static_cast<f64>(dx);
        f64 targetY = currentY + static_cast<f64>(dy);
        f64 targetZ = currentZ + static_cast<f64>(dz);

        // 检查目标位置是否在水中
        BlockPos targetBlockPos(static_cast<i32>(targetX), static_cast<i32>(targetY), static_cast<i32>(targetZ));
        if (m_creature->world()->getBlockState(targetBlockPos)->getMaterial().isLiquid()) {
            outPos = Vector3(static_cast<f32>(targetX), static_cast<f32>(targetY), static_cast<f32>(targetZ));
            return true;
        }
    }

    return false;
}

} // namespace mc::entity::ai::goal
