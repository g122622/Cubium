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

#include "AvoidEntityGoal.hpp"
#include "common/core/EnumSet.hpp"
#include "common/entity/ai/goal/GoalConstants.hpp"
#include "common/entity/ai/goal/GoalFlag.hpp"
#include "common/entity/ai/pathfinding/PathNavigator.hpp"
#include "common/entity/ai/util/RandomPositionGenerator.hpp"
#include "common/entity/core/EntitySize.hpp"
#include "common/entity/core/EntityType.hpp"
#include "common/entity/core/EntityUtils.hpp"
#include "common/entity/core/MobEntity.hpp"
#include <utility>

namespace mc::entity::ai::goal {

using namespace constants;

AvoidEntityGoal::AvoidEntityGoal(CreatureEntity* creature, f32 avoidDistance, f64 farSpeed, f64 nearSpeed)
    : m_creature(creature)
    , m_avoidDistance(avoidDistance)
    , m_farSpeed(farSpeed)
    , m_nearSpeed(nearSpeed)
    , m_predicate(nullptr)
{
    setMutexFlags(EnumSet<GoalFlag>{GoalFlag::Move});
}

AvoidEntityGoal::AvoidEntityGoal(
    CreatureEntity* creature, f32 avoidDistance, f64 farSpeed, f64 nearSpeed, EntityPredicate predicate)
    : m_creature(creature)
    , m_avoidDistance(avoidDistance)
    , m_farSpeed(farSpeed)
    , m_nearSpeed(nearSpeed)
    , m_predicate(std::move(predicate))
{
    setMutexFlags(EnumSet<GoalFlag>{GoalFlag::Move});
}

bool AvoidEntityGoal::shouldExecute()
{
    if (!m_creature) return false;

    // 寻找要避开的实体
    m_avoidTarget = _findEntityToAvoid();
    if (!m_avoidTarget) {
        return false;
    }

    // 寻找远离目标的位置
    if (!_findEscapePosition()) {
        return false;
    }

    // 检查路径是否存在
    auto* nav = m_creature->navigator();
    if (!nav) {
        return false;
    }

    // 尝试找到到逃跑位置的路径
    return nav->moveTo(m_escapeX, m_escapeY, m_escapeZ, 0.0);
}

bool AvoidEntityGoal::shouldContinueExecuting()
{
    if (!m_creature) return false;

    // 【UAF 防护】被躲避的目标已被 discard/移除（isAlive()==false）时立即放手。
    // vanilla AvoidEntityGoal.canContinueToUse() 仅检查 `!this.pathNav.isDone()`，
    // 在 Java 中安全（GC 保证引用有效性）；但 Cubium 采用 graveyard 延迟析构
    // （EntityManager.cpp:345 才真正释放），本 goal 缓存的 m_avoidTarget 裸指针
    // 会指向"已标记移除但尚未析构"的对象，而 tick() 会解引用它。
    // 因此此处必须显式检查，这是 C++ 移植相对 vanilla 的额外要求，
    // 而非与 vanilla 的行为分歧。契约见 EntityManager.cpp:380-381。
    if (m_avoidTarget != nullptr && !m_avoidTarget->isAlive()) {
        return false;
    }

    // 继续执行直到路径完成
    auto* nav = m_creature->navigator();
    if (nav && nav->noPath()) {
        return false;
    }

    return true;
}

void AvoidEntityGoal::startExecuting()
{
    if (m_creature) {
        if (auto* nav = m_creature->navigator()) {
            static_cast<void>(nav->moveTo(m_escapeX, m_escapeY, m_escapeZ, m_farSpeed));
        }
    }
}

void AvoidEntityGoal::resetTask()
{
    m_avoidTarget = nullptr;
    if (m_creature) {
        m_creature->clearNavigation();
    }
}

void AvoidEntityGoal::tick()
{
    if (!m_creature || !m_avoidTarget) return;

    // 【UAF 防护】被躲避的目标已被 discard/移除时立即清空裸指针。
    // 与 shouldContinueExecuting() 中的守卫构成双保险：Phase 1 通常已在
    // Phase 3 之前调用 resetTask() 清空 m_avoidTarget，但同一 Phase 3 循环中
    // 先执行的兄弟 goal 可能在本次 tick 内移除该目标，此时本 goal 的 Phase 1
    // 已经跑过，只有此处再检查一次才安全。
    // CI 实测崩溃点：heap-use-after-free @ Entity.hpp:409 mc::Entity::distanceSqTo(...)
    //   <- AvoidEntityGoal::tick() AvoidEntityGoal.cpp:121。
    if (!m_avoidTarget->isAlive()) {
        m_avoidTarget = nullptr;
        return;
    }

    // 根据距离调整速度，阈值是 49.0D (7*7)
    f64 distSq = m_creature->distanceSqTo(*m_avoidTarget);

    if (auto* nav = m_creature->navigator()) {
        if (distSq < AVOID_NEAR_DISTANCE_SQ) {
            // 近距离使用近距速度（更快）
            nav->setSpeed(m_nearSpeed);
        } else {
            // 远距离使用远距速度
            nav->setSpeed(m_farSpeed);
        }
    }
}

LivingEntity* AvoidEntityGoal::_findEntityToAvoid()
{
    if (!m_creature || !m_creature->world()) return nullptr;

    // 在避开距离内搜索，垂直扩展 3.0D
    constexpr f32 verticalExpand = 3.0f;

    return EntityUtils::findClosestEntity<LivingEntity>(
        m_creature->world(), m_creature->position(), m_avoidDistance + verticalExpand, m_creature, m_predicate);
}

bool AvoidEntityGoal::_findEscapePosition()
{
    if (!m_creature || !m_avoidTarget) return false;

    // 使用 RandomPositionGenerator.findRandomTargetBlockAwayFrom
    Vector3 avoidPos(m_avoidTarget->x(), m_avoidTarget->y(), m_avoidTarget->z());
    Vector3 escapePos;

    if (util::RandomPositionGenerator::findRandomTargetBlockAwayFrom(
            m_creature, ESCAPE_HORIZONTAL_RANGE, ESCAPE_VERTICAL_RANGE, avoidPos, escapePos)) {

        // 检查逃跑位置是否比当前位置更远离目标
        if (!_isEscapePositionValid(escapePos)) {
            return false;
        }

        m_escapeX = escapePos.x;
        m_escapeY = escapePos.y;
        m_escapeZ = escapePos.z;
        return true;
    }

    return false;
}

bool AvoidEntityGoal::_isEscapePositionValid(const Vector3& escapePos) const
{
    if (!m_avoidTarget || !m_creature) return false;

    // 检查逃跑位置到目标的距离是否大于当前位置到目标的距离
    f64 distToEscapePos = m_avoidTarget->distanceSqTo(escapePos.x, escapePos.y, escapePos.z);
    f64 distToCurrentPos = m_avoidTarget->distanceSqTo(*m_creature);

    // 逃跑位置必须比当前位置更远离目标
    return distToEscapePos >= distToCurrentPos;
}

} // namespace mc::entity::ai::goal
