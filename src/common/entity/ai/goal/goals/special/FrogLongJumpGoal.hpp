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

namespace mc {

class FrogEntity;

namespace entity::ai::goal {

/**
 * @brief 青蛙长跳目标（对齐 vanilla FrogAi 的 LONG_JUMP 活动）
 *
 * 合并 vanilla 的两个 Brain 行为：
 * - LongJumpToPreferredBlock（继承 LongJumpToRandomPos）：在周围 ±4 格水平、±2 格垂直范围内
 *   挑选落点；每次起跳有 50% 概率优先选择落点为睡莲/垂滴叶（#frog_prefer_jump_to）的候选；
 * - LongJumpMidJump：起跳后进入 LONG_JUMPING 姿态，落地时水平速度乘 0.1 并进入 100~140 tick 冷却。
 *
 * 落点约束（对齐 FrogAi.isAcceptableLandingSpot）：落点、其上方与下方三个位置均无流体；
 * 落点或其下方为偏好方块时直接接受，否则要求下方为实心方块。
 */
class FrogLongJumpGoal : public Goal {
public:
    /// 落点挑选尝试次数（vanilla FIND_JUMP_TRIES）
    static constexpr i32 FIND_JUMP_TRIES = 20;
    /// 蓄力时长（vanilla PREPARE_JUMP_DURATION）
    static constexpr i32 PREPARE_JUMP_DURATION = 40;
    /// 空中状态超时（vanilla LongJumpMidJump.TIME_OUT_DURATION）
    static constexpr i32 TIME_OUT_DURATION = 100;
    /// 搜索半径（vanilla MAX_LONG_JUMP_WIDTH = 4 / MAX_LONG_JUMP_HEIGHT = 2）
    static constexpr i32 MAX_JUMP_WIDTH = 4;
    static constexpr i32 MAX_JUMP_HEIGHT = 2;
    /// 初速度倍率（vanilla FrogAi 传入的 maxJumpVelocityMultiplier）
    static constexpr f32 MAX_JUMP_VELOCITY_MULTIPLIER = 3.5714288f;
    /// 冷却区间（vanilla TIME_BETWEEN_LONG_JUMPS = UniformInt.of(100, 140)）
    static constexpr i32 COOLDOWN_MIN_TICKS = 100;
    static constexpr i32 COOLDOWN_MAX_TICKS = 140;
    /// 优先选择偏好方块的概率（vanilla preferredBlocksChance = 0.5F）
    static constexpr f32 PREFERRED_BLOCK_CHANCE = 0.5f;
    /// 抛射角（vanilla ALLOWED_ANGLES）
    static constexpr i32 ALLOWED_ANGLES[] = {65, 70, 75, 80};
    /// 起跳后速度整体缩放（vanilla LongJumpUtil 末尾 scale(0.95F)）
    static constexpr f32 JUMP_VECTOR_SCALE = 0.95f;
    /// 落地水平速度衰减（vanilla LongJumpMidJump.stop）
    static constexpr f32 LANDING_HORIZONTAL_DAMPING = 0.1f;

    explicit FrogLongJumpGoal(FrogEntity* frog);

    [[nodiscard]] bool shouldExecute() override;
    [[nodiscard]] bool shouldContinueExecuting() override;
    void startExecuting() override;
    void resetTask() override;
    void tick() override;

    [[nodiscard]] std::string getTypeName() const override
    {
        return "FrogLongJumpGoal";
    }

    /// 当前阶段（测试/诊断用）
    enum class Phase : u8 {
        Idle,
        Preparing,
        Jumping,
    };

    [[nodiscard]] Phase phase() const noexcept
    {
        return m_phase;
    }

    [[nodiscard]] i32 cooldown() const noexcept
    {
        return m_cooldown;
    }

    [[nodiscard]] const BlockPos& chosenTarget() const noexcept
    {
        return m_target;
    }

    /// 计算抛射初速度（与山羊长跳共用同一套物理求解；实现见 GoatLongJumpGoal）
    [[nodiscard]] std::optional<Vector3> calculateJumpVector(
        const Vector3& from, const Vector3& target, f64 maxVelocity, i32 angleDeg) const;

private:
    void _pickCandidate();
    [[nodiscard]] bool _isAcceptableLandingSpot(const BlockPos& pos) const;
    [[nodiscard]] bool _isPreferredBlock(const BlockPos& pos) const;
    void _setRandomCooldown();
    void _playEntitySound(const char* suffix, f32 pitch) const;

    FrogEntity* m_frog;
    Phase m_phase = Phase::Idle;
    i32 m_cooldown = 0;
    i32 m_prepareTicks = 0;
    i32 m_airTicks = 0;
    BlockPos m_target;
    Vector3 m_jumpVector{0.0, 0.0, 0.0};
};

} // namespace entity::ai::goal
} // namespace mc