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

#include "PatrollerEntity.hpp"

#include "common/core/Types.hpp"
#include "common/entity/ai/goal/goals/special/PatrolGoals.hpp"
#include "common/entity/core/EntityClassRegistry.hpp"
#include "common/entity/entities/monster/MonsterEntity.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/world/block/BlockPos.hpp"
#include <memory>

namespace mc {

// ============================================================================
// 继承链标识（parent = MonsterEntity::classInfo()）。透传层无自身同步字段，
// classInfo 仅作父链遍历节点。
// ============================================================================
const entity::EntityClassInfo& PatrollerEntity::classInfo()
{
    static const entity::EntityClassInfo s_classInfo{"PatrollerEntity", &MonsterEntity::classInfo()};
    return s_classInfo;
}

PatrollerEntity::PatrollerEntity(EntityInstanceId id, ecs::EntityRegistry& registry)
    : MonsterEntity(id, registry)
{}

void PatrollerEntity::setPatrolTarget(const BlockPos& patrolTarget)
{
    m_patrolTarget = patrolTarget;
    m_isPatrolling = true;
}

const BlockPos& PatrollerEntity::getPatrolTarget() const
{
    MC_ASSERT_RELEASE(m_patrolTarget.has_value());
    return *m_patrolTarget;
}

void PatrollerEntity::setLeader(bool isLeader)
{
    m_isPatrolLeader = isLeader;
    m_isPatrolling = true;
}

void PatrollerEntity::resetPatrolTarget()
{
    auto& random = getRandom();
    const BlockPos currentPos(position());
    setPatrolTarget(currentPos + BlockPos(-500 + random.nextInt(1000), 0, -500 + random.nextInt(1000)));
}

void PatrollerEntity::registerGoals()
{
    MonsterEntity::registerGoals();

    // 队员速度 0.7，队长速度 0.595（队长较慢以便队员跟随）
    m_goalSelector.addGoal(4, std::make_unique<entity::ai::goal::PatrolGoal>(this, 0.7, 0.595));
}

} // namespace mc
