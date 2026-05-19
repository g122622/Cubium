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
#include "../../../../util/math/MathUtils.hpp"
#include "../../../../util/math/random/Random.hpp"
#include "../../../../world/IWorld.hpp"
#include "../../../core/CreatureEntity.hpp"
#include "../../../core/Entity.hpp"
#include "../../../core/EntityUtils.hpp"
#include "../../../core/LivingEntity.hpp"
#include "../../../core/MobEntity.hpp"
#include "../../pathfinding/PathNavigator.hpp"
#include "../../util/RandomPositionGenerator.hpp"
#include "../GoalConstants.hpp"
#include <cmath>

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

    // MC 1.16.5: 寻找要避开的实体
    m_avoidTarget = findEntityToAvoid();
    if (!m_avoidTarget) {
        return false;
    }

    // MC 1.16.5: 使用 RandomPositionGenerator.findRandomTargetBlockAwayFrom
    // 寻找远离目标的位置
    if (!findEscapePosition()) {
        return false;
    }

    // MC 1.16.5: 检查路径是否存在
    // Path path = this.navigation.getPathToPos(vector3d.x, vector3d.y, vector3d.z, 0);
    // return this.path != null;
    auto* nav = m_creature->navigator();
    if (!nav) {
        return false;
    }

    // 尝试找到到逃跑位置的路径
    // MC 1.16.5: getPathToPos(vector3d.x, vector3d.y, vector3d.z, 0)
    // 注意：这里我们使用 moveTo 来尝试计算路径，但不实际移动
    // 如果路径计算成功则返回 true
    return nav->moveTo(m_escapeX, m_escapeY, m_escapeZ, 0.0);
}

bool AvoidEntityGoal::shouldContinueExecuting()
{
    if (!m_creature) return false;

    // MC 1.16.5: 继续执行直到路径完成
    auto* nav = m_creature->navigator();
    if (nav && nav->noPath()) {
        return false;
    }

    return true;
}

void AvoidEntityGoal::startExecuting()
{
    if (m_creature) {
        // MC 1.16.5: 设置路径到逃跑位置
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

    // MC 1.16.5: 根据距离调整速度，阈值是 49.0D (7*7)
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

LivingEntity* AvoidEntityGoal::findEntityToAvoid()
{
    if (!m_creature || !m_creature->world()) return nullptr;

    // MC 1.16.5: 在避开距离内搜索，垂直扩展 3.0D
    // 使用 EntityPredicate 进行搜索
    f32 verticalExpand = 3.0f;

    return EntityUtils::findClosestEntity<LivingEntity>(m_creature->world(),
        m_creature->position(),
        m_avoidDistance + verticalExpand, // 包含垂直扩展
        m_creature,
        m_predicate);
}

bool AvoidEntityGoal::findEscapePosition()
{
    if (!m_creature || !m_avoidTarget) return false;

    // MC 1.16.5: 使用 RandomPositionGenerator.findRandomTargetBlockAwayFrom(entity, 16, 7,
    // avoidTarget.getPositionVec())
    Vector3 avoidPos(m_avoidTarget->x(), m_avoidTarget->y(), m_avoidTarget->z());
    Vector3 escapePos;

    if (util::RandomPositionGenerator::findRandomTargetBlockAwayFrom(m_creature,
            ESCAPE_HORIZONTAL_RANGE, // 16
            ESCAPE_VERTICAL_RANGE,   // 7
            avoidPos,
            escapePos)) {

        // MC 1.16.5: 检查逃跑位置是否比当前位置更远离目标
        // if (this.avoidTarget.getDistanceSq(vector3d.x, vector3d.y, vector3d.z) <
        // this.avoidTarget.getDistanceSq(this.entity))
        //     return false;
        if (!isEscapePositionValid(escapePos)) {
            return false;
        }

        m_escapeX = escapePos.x;
        m_escapeY = escapePos.y;
        m_escapeZ = escapePos.z;
        return true;
    }

    return false;
}

bool AvoidEntityGoal::isEscapePositionValid(const Vector3& escapePos) const
{
    if (!m_avoidTarget || !m_creature) return false;

    // MC 1.16.5: 检查逃跑位置到目标的距离是否大于当前位置到目标的距离
    // if (this.avoidTarget.getDistanceSq(vector3d) < this.avoidTarget.getDistanceSq(this.entity))
    //     return false;
    f64 distToEscapePos = m_avoidTarget->distanceSqTo(escapePos.x, escapePos.y, escapePos.z);
    f64 distToCurrentPos = m_avoidTarget->distanceSqTo(*m_creature);

    // 逃跑位置必须比当前位置更远离目标
    return distToEscapePos >= distToCurrentPos;
}

} // namespace mc::entity::ai::goal
