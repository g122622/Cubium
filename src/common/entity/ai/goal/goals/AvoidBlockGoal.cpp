/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
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

#include "AvoidBlockGoal.hpp"

#include "common/core/EnumSet.hpp"
#include "common/entity/ai/goal/Goal.hpp"
#include "common/entity/ai/goal/GoalFlag.hpp"
#include "common/entity/ai/pathfinding/PathNavigator.hpp"
#include "common/entity/ai/util/RandomPositionGenerator.hpp"
#include "common/entity/core/CreatureEntity.hpp"
#include "common/entity/core/EntitySize.hpp"
#include "common/entity/core/EntityType.hpp"
#include "common/entity/core/MobEntity.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/math/MathUtils.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/BlockTags.hpp"
#include <limits>
#include <utility>

namespace mc::entity::ai::goal {

AvoidBlockGoal::AvoidBlockGoal(
    CreatureEntity* creature, const BlockTag& tag, f64 speed, i32 horizontalRange, i32 verticalRange)
    : Goal(EnumSet<GoalFlag>{GoalFlag::Move})
    , m_creature(creature)
    , m_tag(tag)
    , m_speed(speed)
    , m_horizontalRange(horizontalRange)
    , m_verticalRange(verticalRange)
    , m_validator(nullptr)
{
    MC_ASSERT_RELEASE(creature != nullptr);
}

AvoidBlockGoal::AvoidBlockGoal(CreatureEntity* creature,
    const BlockTag& tag,
    f64 speed,
    i32 horizontalRange,
    i32 verticalRange,
    BlockValidator validator)
    : Goal(EnumSet<GoalFlag>{GoalFlag::Move})
    , m_creature(creature)
    , m_tag(tag)
    , m_speed(speed)
    , m_horizontalRange(horizontalRange)
    , m_verticalRange(verticalRange)
    , m_validator(std::move(validator))
{
    MC_ASSERT_RELEASE(creature != nullptr);
}

bool AvoidBlockGoal::shouldExecute()
{
    if (!m_creature) return false;

    // 寻找附近的排斥方块
    if (!_findNearestRepellent()) {
        return false;
    }

    // 寻找远离排斥方块的位置
    if (!_findEscapePosition()) {
        return false;
    }

    // 仅检查导航器可用性，不启动导航
    // 实际的路径计算和导航在 startExecuting() 中统一启动
    auto* nav = m_creature->navigator();
    return nav != nullptr;
}

bool AvoidBlockGoal::shouldContinueExecuting()
{
    if (!m_creature) return false;

    // 检查是否仍在排斥方块附近
    // 如果已经远离排斥方块，不再需要继续逃跑
    IWorld* worldPtr = m_creature->world();
    if (worldPtr != nullptr) {
        const f64 dx = m_creature->x() - static_cast<f64>(m_nearestRepellentPos.x);
        const f64 dy = m_creature->y() - static_cast<f64>(m_nearestRepellentPos.y);
        const f64 dz = m_creature->z() - static_cast<f64>(m_nearestRepellentPos.z);
        const f64 distSq = dx * dx + dy * dy + dz * dz;

        // 对应 MC 1.21.11 SetWalkTargetAwayFrom 的 desiredDistance 参数
        // 疣猪兽/猪灵的排斥物检测范围是 8 格，逃跑至超出检测范围即安全
        // 使用 (horizontalRange * 1.5)^2 作为安全距离平方阈值
        const f64 safeDist = static_cast<f64>(m_horizontalRange) * 1.5;
        const f64 safeDistanceSq = safeDist * safeDist;
        if (distSq > safeDistanceSq) {
            return false;
        }
    }

    // 继续执行直到路径完成
    auto* nav = m_creature->navigator();
    if (nav && nav->noPath()) {
        return false;
    }

    return true;
}

void AvoidBlockGoal::startExecuting()
{
    if (m_creature) {
        if (auto* nav = m_creature->navigator()) {
            // 以指定速度启动导航到逃跑位置
            // 如果路径计算失败（无法到达），shouldContinueExecuting 会在下一 tick 检测到
            static_cast<void>(nav->moveTo(m_escapeX, m_escapeY, m_escapeZ, m_speed));
        }
    }
}

void AvoidBlockGoal::resetTask()
{
    if (m_creature) {
        m_creature->clearNavigation();
    }
}

void AvoidBlockGoal::tick()
{
    if (!m_creature) return;

    // 持续导航到逃跑位置
    // PathNavigator 的 tick 由 MobEntity::tick 调用，无需手动调用
    // 但需要检测是否需要重新计算逃跑位置
    // 如果仍然靠近排斥方块且导航已完成，尝试重新逃跑
    auto* nav = m_creature->navigator();
    if (nav && nav->noPath()) {
        // 导航完成但仍在排斥方块附近，尝试重新逃跑
        if (_findNearestRepellent() && _findEscapePosition()) {
            static_cast<void>(nav->moveTo(m_escapeX, m_escapeY, m_escapeZ, m_speed));
        }
    }
}

bool AvoidBlockGoal::_findNearestRepellent()
{
    IWorld* worldPtr = m_creature ? m_creature->world() : nullptr;
    if (worldPtr == nullptr) {
        return false;
    }

    // 以生物当前位置为中心搜索排斥方块
    // 搜索范围与 MC 原版 PiglinSpecificSensor / HoglinSpecificSensor 的检测范围一致
    const i32 cx = math::floorTo<i32>(m_creature->x());
    const i32 cy = math::floorTo<i32>(m_creature->y());
    const i32 cz = math::floorTo<i32>(m_creature->z());

    // 记录最近的排斥方块位置
    i32 nearestDistSq = std::numeric_limits<i32>::max();
    bool found = false;

    for (i32 dx = -m_horizontalRange; dx <= m_horizontalRange; ++dx) {
        for (i32 dy = -m_verticalRange; dy <= m_verticalRange; ++dy) {
            for (i32 dz = -m_horizontalRange; dz <= m_horizontalRange; ++dz) {
                const BlockPos checkPos(cx + dx, cy + dy, cz + dz);
                const BlockState* state = worldPtr->getBlockState(checkPos);
                if (state == nullptr || !m_tag.contains(*state)) {
                    continue;
                }

                // 如果有验证函数，执行额外验证
                if (m_validator && !m_validator(*state)) {
                    continue;
                }

                // 计算距离的平方，保留最近的位置
                const i32 distSq = dx * dx + dy * dy + dz * dz;
                if (distSq < nearestDistSq) {
                    nearestDistSq = distSq;
                    m_nearestRepellentPos = checkPos;
                    found = true;
                }
            }
        }
    }

    return found;
}

bool AvoidBlockGoal::_findEscapePosition()
{
    if (!m_creature) return false;

    // 使用 RandomPositionGenerator 找到远离排斥方块的位置
    // 对应 MC 1.21.11 的 LandRandomPos.getPosAway(mob, 16, 7, repellantPos)
    Vector3 avoidPos(static_cast<f64>(m_nearestRepellentPos.x) + 0.5,
        static_cast<f64>(m_nearestRepellentPos.y),
        static_cast<f64>(m_nearestRepellentPos.z) + 0.5);
    Vector3 escapePos;

    if (util::RandomPositionGenerator::findRandomTargetBlockAwayFrom(
            m_creature, ESCAPE_HORIZONTAL_RANGE, ESCAPE_VERTICAL_RANGE, avoidPos, escapePos)) {

        // 验证逃跑位置比当前位置更远离排斥方块
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

bool AvoidBlockGoal::_isEscapePositionValid(const Vector3& escapePos) const
{
    if (!m_creature) return false;

    // 计算排斥方块到逃跑位置的距离
    const f64 avoidToEscapeX = escapePos.x - static_cast<f64>(m_nearestRepellentPos.x);
    const f64 avoidToEscapeY = escapePos.y - static_cast<f64>(m_nearestRepellentPos.y);
    const f64 avoidToEscapeZ = escapePos.z - static_cast<f64>(m_nearestRepellentPos.z);
    const f64 distAvoidToEscape =
        avoidToEscapeX * avoidToEscapeX + avoidToEscapeY * avoidToEscapeY + avoidToEscapeZ * avoidToEscapeZ;

    // 计算排斥方块到当前位置的距离
    const f64 avoidToCreatureX = m_creature->x() - static_cast<f64>(m_nearestRepellentPos.x);
    const f64 avoidToCreatureY = m_creature->y() - static_cast<f64>(m_nearestRepellentPos.y);
    const f64 avoidToCreatureZ = m_creature->z() - static_cast<f64>(m_nearestRepellentPos.z);
    const f64 distAvoidToCreature =
        avoidToCreatureX * avoidToCreatureX + avoidToCreatureY * avoidToCreatureY + avoidToCreatureZ * avoidToCreatureZ;

    // 逃跑位置必须比当前位置更远离排斥方块
    return distAvoidToEscape >= distAvoidToCreature;
}

} // namespace mc::entity::ai::goal
