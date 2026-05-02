#include "PatrollerEntity.hpp"

#include "../../../../util/assert/AssertAll.hpp"

namespace mc {

PatrollerEntity::PatrollerEntity(LegacyEntityType type, EntityId id)
    : MonsterEntity(type, id)
{
}

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
    auto random = getRandom();
    const BlockPos currentPos(position());
    setPatrolTarget(currentPos + BlockPos(-500 + random.nextInt(1000), 0, -500 + random.nextInt(1000)));
}

void PatrollerEntity::registerGoals()
{
    MonsterEntity::registerGoals();

    // TODO: 接入 1.16.5 PatrollerEntity.PatrolGoal 后，将巡逻移动目标注册到这里。
}

} // namespace mc
