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

#include "common/entity/ai/goal/Goal.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/world/block/BlockPos.hpp"

#include "common/entity/core/EntityReference.hpp"

namespace mc {

class GoatEntity;
class LivingEntity;

namespace entity::ai::goal {

/**
 * @brief 山羊冲撞目标（对齐 vanilla GoatAi 的 RAM 活动：PrepareRamNearestTarget + RamTarget）
 *
 * 状态机：
 * 1. Prepare：冷却归零且找到 4~7 格内可冲撞目标（非山羊、非旁观者，预言走可达）时，
 *    以 1.25 倍速走到目标的起跑点；到达后低头蓄力 20 tick；
 * 2. Ram：沿起跑方向以 3.0 倍速冲过去，撞到目标按攻击伤害 + 击退力度结算
 *    （成年 2.5 / 幼年 1.0，护盾格挡后击退减半）；撞上 `#snaps_goat_horn` 方块掉落山羊角；
 * 3. 结算后进入冷却（尖叫山羊 100~300 tick，普通山羊 600~6000 tick）。
 *
 * 冲撞期间占用 Move 与 Look flag，并阻止带 Look 的其余 goal 干扰朝向。
 */
class GoatRamGoal : public Goal {
public:
    explicit GoatRamGoal(GoatEntity* goat);

    [[nodiscard]] bool shouldExecute() override;
    [[nodiscard]] bool shouldContinueExecuting() override;
    void startExecuting() override;
    void resetTask() override;
    void tick() override;

    [[nodiscard]] std::string getTypeName() const override
    {
        return "GoatRamGoal";
    }

    /// 当前阶段（测试/诊断用）
    enum class Phase : u8 {
        Idle,
        Preparing,
        Ramming,
    };

    [[nodiscard]] Phase phase() const noexcept
    {
        return m_phase;
    }

    /// 当前选中目标（无目标为 nullptr）
    [[nodiscard]] LivingEntity* target() const;

private:
    /// 为当前目标选择冲撞起跑点（对齐 vanilla PrepareRamNearestTarget.calculateRammingStartPosition）
    bool _chooseRamStartPosition();

    /// 起跑点是否可行走（脚下实心、身体两格为空气）
    [[nodiscard]] bool _isWalkableBlock(const BlockPos& pos) const;

    /// 结算命中（伤害 + 击退），对齐 vanilla RamTarget.tick 的命中分支
    void _finishRam();

    GoatEntity* m_goat;
    Phase m_phase = Phase::Idle;
    EntityReference<LivingEntity> m_target;
    BlockPos m_ramStartPos; // 起跑点
    BlockPos m_targetPos;   // 目标所在方块（用于判断目标是否移动）
    Vector3 m_ramDirection{0.0, 0.0, 0.0};
    i32 m_prepareTicks = 0; // 起跑点蓄力计时（达到 RAM_PREPARE_TIME 后开始冲撞）
    i32 m_timeoutTicks = 0; // 整体超时（对齐 Behavior 200 tick）
};

} // namespace entity::ai::goal
} // namespace mc
