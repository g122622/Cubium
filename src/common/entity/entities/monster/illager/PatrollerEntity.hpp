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

#pragma once

#include "../MonsterEntity.hpp"
#include "common/core/Types.hpp"
#include "common/world/block/BlockPos.hpp"
#include <optional>

namespace mc {

/**
 * @brief 巡逻怪物公共基类
 *
 * 继承链: MonsterEntity -> PatrollerEntity -> AbstractRaiderEntity -> AbstractIllagerEntity
 *
 * 当前提供：
 * - 巡逻目标
 * - 巡逻队长标记
 * - 巡逻中状态
 * - 随机重置巡逻目标
 */
class PatrollerEntity : public MonsterEntity {
public:
    PatrollerEntity(EntityInstanceId id);
    ~PatrollerEntity() override = default;

    PatrollerEntity(const PatrollerEntity&) = delete;
    PatrollerEntity& operator=(const PatrollerEntity&) = delete;
    PatrollerEntity(PatrollerEntity&&) = delete;
    PatrollerEntity& operator=(PatrollerEntity&&) = delete;

    /// 本类继承链标识（parent = MonsterEntity::classInfo()）。见 Entity::classInfo()。
    // 透传层无自身同步字段，classInfo 仅作父链遍历节点。
    static const entity::EntityClassInfo& classInfo();

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
