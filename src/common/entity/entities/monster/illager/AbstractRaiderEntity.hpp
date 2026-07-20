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

#include "PatrollerEntity.hpp"
#include "common/core/Types.hpp"
#include "common/entity/damage/DamageSource.hpp"

namespace mc {
namespace world::village::raid {
class Raid;
}

/**
 * @brief 袭击者抽象基类
 *
 * 为所有可参与村庄袭击的实体提供统一的 Raid 关联与庆祝状态管理。
 *
 * 继承链: MonsterEntity -> PatrollerEntity -> AbstractRaiderEntity
 */
class AbstractRaiderEntity : public PatrollerEntity {
public:
    /**
     * @brief 袭击者状态
     */
    enum class RaiderState : u8 {
        Neutral = 0,    // 中立
        Celebrating = 1 // 庆祝
    };

    /**
     * @brief 构造袭击者基类。
     *
     * @param type 实体类型。
     * @param id 实体 ID。
     */
    AbstractRaiderEntity(EntityInstanceId id);

    ~AbstractRaiderEntity() override = default;
    AbstractRaiderEntity(const AbstractRaiderEntity&) = delete;
    AbstractRaiderEntity& operator=(const AbstractRaiderEntity&) = delete;
    AbstractRaiderEntity(AbstractRaiderEntity&&) = delete;
    AbstractRaiderEntity& operator=(AbstractRaiderEntity&&) = delete;

    // ========== 状态系统 ==========

    /**
     * @brief 获取当前状态
     */
    [[nodiscard]] RaiderState getState() const { return m_state; }

    /**
     * @brief 设置状态
     */
    void setState(RaiderState state) { m_state = state; }

    /**
     * @brief 是否正在庆祝
     */
    [[nodiscard]] bool isCelebrating() const { return m_state == RaiderState::Celebrating; }

    // ========== 袭击系统 ==========

    /**
     * @brief 判断是否拥有袭击队长加成。
     */
    [[nodiscard]] bool hasRaidLeaderBonus() const { return m_hasLeaderBonus; }

    /**
     * @brief 设置袭击队长加成状态。
     *
     * @param bonus 是否拥有加成。
     */
    void setRaidLeaderBonus(bool bonus) { m_hasLeaderBonus = bonus; }

    /**
     * @brief 判断是否允许加入袭击。
     */
    [[nodiscard]] bool canJoinRaid() const { return m_canJoinRaid; }

    /**
     * @brief 设置是否允许加入袭击。
     *
     * @param canJoin 是否允许加入。
     */
    void setCanJoinRaid(bool canJoin) { m_canJoinRaid = canJoin; }

    /**
     * @brief 获取当前所属袭击。
     *
     * @return 当前 Raid 指针；若未加入则返回 `nullptr`。
     */
    [[nodiscard]] world::village::raid::Raid* getCurrentRaid() const { return m_raid; }

    /**
     * @brief 加入指定袭击。
     *
     * @param raid 目标袭击。
     * @param wave 当前波次。
     */
    void joinRaid(world::village::raid::Raid* raid, i32 wave);

    /**
     * @brief 离开当前袭击。
     */
    void leaveRaid();

    /**
     * @brief 获取当前所属波次。
     */
    [[nodiscard]] i32 getRaidWave() const { return m_wave; }

    /**
     * @brief 设置当前所属波次。
     *
     * @param wave 波次编号。
     */
    void setRaidWave(i32 wave) { m_wave = wave; }

    /**
     * @brief 获取庆祝剩余时间。
     */
    [[nodiscard]] i32 getCelebrationTime() const { return m_celebrationTime; }

    /**
     * @brief 进入庆祝状态。
     *
     * @note 仅更新本地状态，不会自动广播动画或音效。
     */
    void startCelebrating();

    /**
     * @brief 执行实体 tick。
     *
     * @note 会在常规巡逻者逻辑后更新 Raid 关联状态。
     */
    void tick() override;

    /**
     * @brief 处理死亡。
     *
     * @param cause 伤害来源。
     *
     * @warning 调用顺序很重要，必须先通知 Raid，再交给父类完成死亡流程。
     */
    void die(DamageSource& cause) override;

protected:
    RaiderState m_state = RaiderState::Neutral;
    bool m_hasLeaderBonus = false;
    bool m_canJoinRaid = true;
    i32 m_celebrationTime = 0;
    world::village::raid::Raid* m_raid = nullptr;
    i32 m_wave = 0;

    static constexpr i32 CELEBRATION_DURATION = 200;
};

} // namespace mc
