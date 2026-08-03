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

#include "AvoidHostileGoal.hpp"

#include "common/core/EnumSet.hpp"
#include "common/entity/ai/goal/GoalFlag.hpp"
#include "common/entity/core/EntityType.hpp"
#include "common/entity/core/EntityUtils.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/entities/villager/VillagerEntity.hpp"
#include "common/entity/interfaces/IMob.hpp"
#include "common/util/math/MathConstants.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"

#include <cmath>

namespace mc {
namespace entity {
namespace ai {
namespace goal {
namespace villager {

// ============================================================================
// AvoidHostileGoal - 村民逃避敌对目标
// ============================================================================

AvoidHostileGoal::AvoidHostileGoal(VillagerEntity* villager)
    : m_villager(villager)
    , m_hostileEntity(0)
{
    setMutexFlags(EnumSet<GoalFlag>{GoalFlag::Move});
}

bool AvoidHostileGoal::shouldExecute()
{
    if (!m_villager) return false;

    // 查找附近的敌对生物
    _findNearestHostile();
    return m_hostileEntity != 0;
}

bool AvoidHostileGoal::shouldContinueExecuting()
{
    if (!m_villager) return false;

    // 敌对生物消失或距离足够远
    if (m_hostileEntity == 0) return false;

    // 检查敌对生物是否仍然存在
    Entity* entity = m_villager->world() ? m_villager->world()->getEntity(m_hostileEntity) : nullptr;
    if (!entity) {
        m_hostileEntity = 0;
        return false;
    }

    LivingEntity* hostile = dynamic_cast<LivingEntity*>(entity);
    if (!hostile || !hostile->isAlive()) {
        m_hostileEntity = 0;
        return false;
    }

    // 检查距离，如果敌对生物已经足够远，停止逃跑
    f32 distSq = m_villager->distanceSqTo(*hostile);
    if (distSq > FLEE_DISTANCE * FLEE_DISTANCE * 4.0f) { // 超过逃跑距离的2倍
        m_hostileEntity = 0;
        return false;
    }

    return true;
}

void AvoidHostileGoal::startExecuting()
{
    _fleeFromHostile();
}

void AvoidHostileGoal::resetTask()
{
    m_hostileEntity = 0;
    m_fleeTarget = BlockPos::zero();

    if (m_villager) {
        m_villager->clearNavigation();
    }
}

void AvoidHostileGoal::tick()
{
    if (!m_villager || m_hostileEntity == 0) return;

    // 继续逃跑
    _fleeFromHostile();
}

void AvoidHostileGoal::_findNearestHostile()
{
    if (!m_villager || !m_villager->world()) {
        m_hostileEntity = 0;
        return;
    }

    m_hostileEntity = 0;

    // 使用 EntityUtils 查找最近的敌对生物
    // 村民逃离僵尸、掠夺者、劫掠兽、恼鬼等
    LivingEntity* hostile = EntityUtils::findClosestEntity<LivingEntity>(
        m_villager->world(), m_villager->position(), FLEE_RANGE, m_villager, [](LivingEntity* entity) {
            // 检查是否存活
            if (!entity || !entity->isAlive()) return false;

            // 使用 IMob 接口判断是否是敌对生物
            // IMob 是敌对生物的标记接口
            IMob* mob = dynamic_cast<IMob*>(entity);
            return mob != nullptr;
        });

    if (hostile) {
        m_hostileEntity = hostile->id();
    }
}

void AvoidHostileGoal::_fleeFromHostile()
{
    if (!m_villager || m_hostileEntity == 0) return;

    // 获取敌对生物位置
    Entity* entity = m_villager->world() ? m_villager->world()->getEntity(m_hostileEntity) : nullptr;
    if (!entity) {
        m_hostileEntity = 0;
        return;
    }

    LivingEntity* hostile = dynamic_cast<LivingEntity*>(entity);
    if (!hostile || !hostile->isAlive()) {
        m_hostileEntity = 0;
        return;
    }

    // 计算逃跑方向（远离敌对生物）
    f32 dx = m_villager->x() - hostile->x();
    f32 dz = m_villager->z() - hostile->z();

    // 归一化方向向量
    f32 dist = std::sqrt(dx * dx + dz * dz);
    if (dist < 0.001f) {
        // 距离太近，随机选择方向
        math::Random& rng = m_villager->getRandom();
        f32 angle = rng.nextFloat() * math::TWO_PI;
        dx = std::cos(angle);
        dz = std::sin(angle);
    } else {
        dx /= dist;
        dz /= dist;
    }

    // 计算目标位置（逃跑方向）
    f32 targetX = m_villager->x() + dx * FLEE_DISTANCE;
    f32 targetZ = m_villager->z() + dz * FLEE_DISTANCE;
    f32 targetY = m_villager->y();

    m_villager->tryMoveTo(targetX, targetY, targetZ, FLEE_SPEED);
}

} // namespace villager
} // namespace goal
} // namespace ai
} // namespace entity
} // namespace mc
