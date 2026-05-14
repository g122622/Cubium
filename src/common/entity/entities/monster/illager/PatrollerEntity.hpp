#pragma once

#include "../../../../core/Types.hpp"
#include "../../../../world/block/BlockPos.hpp"
#include "../MonsterEntity.hpp"
#include <optional>

namespace mc {

/**
 * @brief 巡逻怪物公共基类
 *
 * 对齐 1.16.5 `PatrollerEntity` 的基础巡逻状态层。
 *
 * 继承链: MonsterEntity -> PatrollerEntity -> AbstractRaiderEntity -> AbstractIllagerEntity
 *
 * 当前提供：
 * - 巡逻目标
 * - 巡逻队长标记
 * - 巡逻中状态
 * - 随机重置巡逻目标
 *
 * 参考 MC 1.16.5 PatrollerEntity
 */
class PatrollerEntity : public MonsterEntity {
public:
    PatrollerEntity(LegacyEntityType type, EntityId id);
    ~PatrollerEntity() override = default;

    PatrollerEntity(const PatrollerEntity&) = delete;
    PatrollerEntity& operator=(const PatrollerEntity&) = delete;
    PatrollerEntity(PatrollerEntity&&) = default;
    PatrollerEntity& operator=(PatrollerEntity&&) = default;

    [[nodiscard]] virtual bool canBeLeader() const { return true; }
    [[nodiscard]] virtual bool canJoinPatrol() const { return true; }

    void setPatrolTarget(const BlockPos& patrolTarget);
    [[nodiscard]] const BlockPos& getPatrolTarget() const;
    [[nodiscard]] bool hasPatrolTarget() const { return m_patrolTarget.has_value(); }

    void setLeader(bool isLeader);
    [[nodiscard]] bool isLeader() const { return m_isPatrolLeader; }

    void resetPatrolTarget();

    [[nodiscard]] bool isPatrolling() const { return m_isPatrolling; }
    void setPatrolling(bool patrolling) { m_isPatrolling = patrolling; }

protected:
    void registerGoals() override;

private:
    std::optional<BlockPos> m_patrolTarget;
    bool m_isPatrolLeader = false;
    bool m_isPatrolling = false;
};

} // namespace mc
