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

#include "TameableEntity.hpp"
#include "common/core/Types.hpp"

namespace mc {

/**
 * @brief 可停在玩家肩膀上的可驯服实体
 *
 * 负责肩膀乘坐冷却和肩膀挂靠状态。
 */
class ShoulderRidingEntity : public TameableEntity {
public:
    /**
     * @brief 构造肩膀乘坐实体
     * @param id 实体 ID
     */
    ShoulderRidingEntity(EntityInstanceId id)
        : TameableEntity(id)
    {}

    ~ShoulderRidingEntity() override = default;

    ShoulderRidingEntity(const ShoulderRidingEntity&) = delete;
    ShoulderRidingEntity& operator=(const ShoulderRidingEntity&) = delete;
    ShoulderRidingEntity(ShoulderRidingEntity&&) = delete;
    ShoulderRidingEntity& operator=(ShoulderRidingEntity&&) = delete;

    /**
     * @brief 当前是否挂在玩家肩膀上
     */
    [[nodiscard]] bool isOnShoulder() const noexcept { return m_onShoulder; }

    /**
     * @brief 获取肩膀所属玩家 ID
     */
    [[nodiscard]] u64 getShoulderPlayerId() const noexcept { return m_shoulderPlayerId; }

    /**
     * @brief 当前是否满足停到肩膀上的冷却条件
     */
    [[nodiscard]] bool canSitOnShoulder() const noexcept { return m_rideCooldownCounter > 100; }

    /**
     * @brief 挂到玩家肩膀上
     */
    bool mountShoulder(u64 playerId) noexcept
    {
        if (!isTamed() || isSitting() || !canSitOnShoulder()) {
            return false;
        }

        m_onShoulder = true;
        m_shoulderPlayerId = playerId;
        return true;
    }

    /**
     * @brief 从肩膀上离开
     */
    void dismountShoulder() noexcept
    {
        m_onShoulder = false;
        m_shoulderPlayerId = 0;
        m_rideCooldownCounter = 0;
    }

    void tick() override
    {
        ++m_rideCooldownCounter;
        TameableEntity::tick();
    }

private:
    i32 m_rideCooldownCounter = 0;
    bool m_onShoulder = false;
    u64 m_shoulderPlayerId = 0;
};

} // namespace mc
