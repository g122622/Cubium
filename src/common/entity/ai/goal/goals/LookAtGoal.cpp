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

#include "LookAtGoal.hpp"
#include "../../../../util/math/random/Random.hpp"
#include "../../../core/EntityUtils.hpp"
#include "../../../core/LivingEntity.hpp"
#include "../../../core/MobEntity.hpp"
#include "../../controller/LookController.hpp"
#include "common/core/EnumSet.hpp"
#include "common/entity/ai/goal/GoalFlag.hpp"
#include "common/entity/core/EntitySize.hpp"
#include "common/entity/core/EntityType.hpp"
#include "common/util/math/MathConstants.hpp"
#include <cmath>
#include <utility>

namespace mc::entity::ai::goal {

// ==================== LookAtGoal ====================

LookAtGoal::LookAtGoal(MobEntity* mob, f32 maxDistance)
    : LookAtGoal(mob, maxDistance, DEFAULT_LOOK_CHANCE)
{}

LookAtGoal::LookAtGoal(MobEntity* mob, f32 maxDistance, f32 chance)
    : m_mob(mob)
    , m_maxDistance(maxDistance)
    , m_chance(chance)
{
    setMutexFlags(EnumSet<GoalFlag>{GoalFlag::Look});
}

LookAtGoal::LookAtGoal(MobEntity* mob, f32 maxDistance, f32 chance, EntityFilter filter)
    : m_mob(mob)
    , m_filter(std::move(filter))
    , m_maxDistance(maxDistance)
    , m_chance(chance)
{
    setMutexFlags(EnumSet<GoalFlag>{GoalFlag::Look});
}

bool LookAtGoal::shouldExecute()
{
    if (!m_mob) return false;

    // 检查概率
    math::Random& rng = m_mob->getRandom();
    if (rng.nextFloat() >= m_chance) {
        return false;
    }

    // 首先检查攻击目标
    if (m_mob->attackTarget() != nullptr) {
        m_lookTarget = m_mob->attackTarget();
        return true;
    }

    // 寻找最近的实体
    m_lookTarget = findTarget();
    return m_lookTarget != nullptr;
}

bool LookAtGoal::shouldContinueExecuting()
{
    if (!m_lookTarget) return false;

    // 检查目标是否存活
    if (!m_lookTarget->isAlive()) return false;

    // 使用距离平方比较
    f64 distSq = m_mob->distanceSqTo(*m_lookTarget);
    f64 maxDistSq = static_cast<f64>(m_maxDistance) * static_cast<f64>(m_maxDistance);

    if (distSq > maxDistSq) {
        return false;
    }

    // 检查剩余时间
    return m_lookTime > 0;
}

void LookAtGoal::startExecuting()
{
    if (!m_mob) return;

    // 设置看向时间 (40 + random.nextInt(40))
    math::Random& rng = m_mob->getRandom();
    m_lookTime = LOOK_AT_MIN_TIME + rng.nextInt(LOOK_AT_MAX_TIME - LOOK_AT_MIN_TIME);
}

void LookAtGoal::resetTask()
{
    m_lookTarget = nullptr;
}

void LookAtGoal::tick()
{
    if (!m_mob || !m_lookTarget) return;

    // 使用 LookController 看向目标眼睛位置
    if (auto* lookCtrl = m_mob->lookController()) {
        f64 eyeY = m_lookTarget->y() + m_lookTarget->eyeHeight();
        lookCtrl->setLookPosition(m_lookTarget->x(), eyeY, m_lookTarget->z());
    }

    --m_lookTime;
}

LivingEntity* LookAtGoal::findTarget()
{
    if (!m_mob || !m_mob->world()) return nullptr;

    // 查找最近的 LivingEntity
    // 使用 boundingBox.grow(maxDistance, 3.0D, maxDistance) 扩展范围
    // 直接使用 findClosestEntity 单次遍历
    f64 maxDistSq = static_cast<f64>(m_maxDistance) * static_cast<f64>(m_maxDistance);

    return EntityUtils::findClosestEntity<LivingEntity>(m_mob->world(),
        m_mob->position(),
        m_maxDistance + 3.0f, // boundingBox.grow(maxDistance, 3.0D, maxDistance)
        m_mob,
        [this, maxDistSq](LivingEntity* entity) {
            // 检查是否在最大距离内
            if (m_mob->distanceSqTo(*entity) > maxDistSq) return false;

            // 执行自定义过滤条件
            if (m_filter && !m_filter(entity)) return false;

            return true;
        });
}

// ==================== LookRandomlyGoal ====================

LookRandomlyGoal::LookRandomlyGoal(MobEntity* mob)
    : m_mob(mob)
{
    // 需要同时锁定移动和看向标志
    setMutexFlags(EnumSet<GoalFlag>{GoalFlag::Move, GoalFlag::Look});
}

bool LookRandomlyGoal::shouldExecute()
{
    if (!m_mob) return false;

    // 默认概率执行 (2%)
    math::Random& rng = m_mob->getRandom();
    return rng.nextFloat() < RANDOM_LOOK_CHANCE;
}

bool LookRandomlyGoal::shouldContinueExecuting()
{
    // 检查是否还有剩余时间
    return m_idleTime >= 0;
}

void LookRandomlyGoal::startExecuting()
{
    if (!m_mob) return;

    // 选择随机方向
    math::Random& rng = m_mob->getRandom();
    f64 angle = math::TWO_PI * rng.nextDouble();

    m_lookX = std::cos(angle);
    m_lookZ = std::sin(angle);

    // 设置看向持续时间
    m_idleTime = RANDOM_LOOK_MIN_TIME + rng.nextInt(RANDOM_LOOK_MAX_TIME - RANDOM_LOOK_MIN_TIME);
}

void LookRandomlyGoal::resetTask()
{
    m_idleTime = 0;
}

void LookRandomlyGoal::tick()
{
    if (!m_mob) return;

    --m_idleTime;

    // 设置看向位置（当前位置 + 随机方向向量）
    if (auto* lookCtrl = m_mob->lookController()) {
        f64 eyeY = m_mob->y() + m_mob->eyeHeight();
        lookCtrl->setLookPosition(m_mob->x() + m_lookX, eyeY, m_mob->z() + m_lookZ);
    }
}

} // namespace mc::entity::ai::goal
