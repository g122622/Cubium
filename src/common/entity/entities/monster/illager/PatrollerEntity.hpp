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

#include "common/core/Types.hpp"
#include "common/entity/core/EntityClassRegistry.hpp"
#include "common/entity/entities/monster/MonsterEntity.hpp"
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
    PatrollerEntity(EntityInstanceId id, ecs::EntityRegistry& registry);
    ~PatrollerEntity() override = default;

    PatrollerEntity(const PatrollerEntity&) = delete;
    PatrollerEntity& operator=(const PatrollerEntity&) = delete;
    PatrollerEntity(PatrollerEntity&&) = delete;
    PatrollerEntity& operator=(PatrollerEntity&&) = delete;

    /// 本类继承链标识（parent = MonsterEntity::classInfo()）。见 Entity::classInfo()。
    // 透传层无自身同步字段，classInfo 仅作父链遍历节点。
    static const entity::EntityClassInfo& classInfo();

    [[nodiscard]] virtual bool canBeLeader() const
    {
        return true;
    }

    [[nodiscard]] virtual bool canJoinPatrol() const
    {
        return true;
    }

    void setPatrolTarget(const BlockPos& patrolTarget);
    [[nodiscard]] const BlockPos& getPatrolTarget() const;

    [[nodiscard]] bool hasPatrolTarget() const
    {
        return m_patrolTarget.has_value();
    }

    void setLeader(bool isLeader);

    [[nodiscard]] bool isLeader() const
    {
        return m_isPatrolLeader;
    }

    /**
     * @brief 是否头戴不祥旗帜（vanilla `Raider.isCaptain()` 判据之一）。
     *
     * 原版把「戴不祥旗」与「巡逻队长标记」分开存放：{@code isPatrolLeader} 只说明它是
     * 巡逻队/袭击波次的领队，{@code isCaptain} 才同时要求头戴不祥旗帜。两者的旗都来自
     * `Raid#getOminousBannerInstance`，图案与颜色固定。
     */
    [[nodiscard]] bool wearsOminousBanner() const;

    /// 把一份不祥旗帜写进头盔槽（原版 `Raid.assignOminousBanner` 的后半段）。
    void equipOminousBanner();

    void resetPatrolTarget();

    [[nodiscard]] bool isPatrolling() const
    {
        return m_isPatrolling;
    }

    void setPatrolling(bool patrolling)
    {
        m_isPatrolling = patrolling;
    }

    void finalizeSpawn(IWorld& world,
        const entity::combat::DifficultyInstance& difficulty,
        world::spawn::SpawnReason spawnReason) override;

protected:
    void registerGoals() override;

    void addAdditionalSaveData(nbt::tags::compound_tag& tag) const override;
    Result<void> readAdditionalSaveData(const nbt::tags::compound_tag& tag) override;

private:
    std::optional<BlockPos> m_patrolTarget;
    bool m_isPatrolLeader = false;
    bool m_isPatrolling = false;
};

} // namespace mc
