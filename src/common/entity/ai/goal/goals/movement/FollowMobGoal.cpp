/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software
 * is furnished to do so, subject to the following conditions:
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

#include "FollowMobGoal.hpp"
#include "../../../../../util/assert/AssertMacros.hpp"
#include "../../../../core/EntityUtils.hpp"
#include "../../../../core/LivingEntity.hpp"
#include "../../../../core/MobEntity.hpp"
#include "../../../pathfinding/PathNavigator.hpp"
#include "common/core/EnumSet.hpp"
#include "common/entity/ai/goal/Goal.hpp"
#include "common/entity/ai/goal/GoalFlag.hpp"
#include "common/entity/core/EntitySize.hpp"
#include "common/entity/core/EntityType.hpp"

namespace mc::entity::ai::goal {

FollowMobGoal::FollowMobGoal(MobEntity* mob, f64 speed, f32 minDistance, f32 maxDistance)
    : Goal(EnumSet<GoalFlag>{GoalFlag::Move})
    , m_mob(mob)
    , m_speed(speed)
    , m_minDistance(minDistance)
    , m_maxDistance(maxDistance)
{
    MC_ASSERT_RELEASE(mob != nullptr);
}

bool FollowMobGoal::shouldExecute()
{
    // 寻找附近的生物
    m_targetMob = _findNearbyMob();
    return m_targetMob != nullptr;
}

bool FollowMobGoal::shouldContinueExecuting()
{
    if (m_targetMob == nullptr) {
        return false;
    }

    // 目标死亡或移除
    if (!m_targetMob->isAlive()) {
        return false;
    }

    // 距离检查：在最大距离内且超过最小距离
    f64 distSq = m_mob->distanceSqTo(*m_targetMob);
    f32 maxDistSq = m_maxDistance * m_maxDistance;
    f32 minDistSq = m_minDistance * m_minDistance;

    return distSq <= static_cast<f64>(maxDistSq) && distSq >= static_cast<f64>(minDistSq);
}

void FollowMobGoal::startExecuting()
{
    m_delayCounter = 0;
}

void FollowMobGoal::resetTask()
{
    m_targetMob = nullptr;
    m_mob->clearNavigation();
}

void FollowMobGoal::tick()
{
    if (m_targetMob == nullptr) {
        return;
    }

    // 看向目标生物
    m_mob->lookAt(*m_targetMob);

    // 定期重新计算路径
    if (--m_delayCounter <= 0) {
        m_delayCounter = PATH_RECALC_INTERVAL;

        // 移动到目标生物位置
        if (auto* nav = m_mob->navigator()) {
            static_cast<void>(nav->moveTo(*m_targetMob, m_speed));
        }
    }
}

LivingEntity* FollowMobGoal::_findNearbyMob()
{
    if (m_mob->world() == nullptr) {
        return nullptr;
    }

    // 在 maxDistance 范围内寻找其他生物
    // 鹦鹉会跟随附近的任何 LivingEntity（不包括自己）
    return EntityUtils::findClosestEntity<LivingEntity>(m_mob->world(),
        m_mob->position(),
        m_maxDistance,
        m_mob, // 排除自己
        [](LivingEntity* entity) {
            // 只要活着就行
            return entity->isAlive();
        });
}

} // namespace mc::entity::ai::goal
