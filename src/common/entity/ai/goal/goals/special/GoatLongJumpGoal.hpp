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

class GoatEntity;

namespace entity::ai::goal {

/**
 * @brief 山羊长跳目标（对齐 vanilla GoatAi 的 LONG_JUMP 活动）
 *
 * 合并 vanilla 的两个 Brain 行为：
 * - LongJumpToRandomPos：从当前位置周围 ±5 格（水平/垂直）的方块中尝试 20 次挑选落点，
 *   落点须脚下实心且寻路不可达（即"必须跳过去"）；用 65°/70°/75°/80° 的抛射角求初速度
 *   （v = JUMP_STRENGTH * 3.5714288），选中后原地蓄力 40 tick 再起跳；
 * - LongJumpMidJump：起跳后进入 LONG_JUMPING 姿态、丢弃摩擦力，落地时水平速度乘 0.1、
 *   播放落地音效并进入 600~1200 tick 冷却。
 *
 * 与 vanilla 的差异：vanilla 用 Brain 记忆驱动，Cubium 以 GoalSelector 等价实现，
 * 冷却与"跳跃中"状态保存在目标对象自身。
 */
class GoatLongJumpGoal : public Goal {
public:
    /// 落点挑选的最大尝试次数（vanilla FIND_JUMP_TRIES）
    static constexpr i32 FIND_JUMP_TRIES = 20;
    /// 蓄力时长（vanilla PREPARE_JUMP_DURATION）
    static constexpr i32 PREPARE_JUMP_DURATION = 40;
    /// 最短寻路距离判定（vanilla MIN_PATHFIND_DISTANCE_TO_VALID_JUMP）
    static constexpr i32 MIN_PATHFIND_DISTANCE_TO_VALID_JUMP = 8;
    /// 行为超时（vanilla LongJumpMidJump.TIME_OUT_DURATION）
    static constexpr i32 TIME_OUT_DURATION = 100;
    /// 搜索半径（vanilla GoatAi 传入的 maxLongJumpHeight / maxLongJumpWidth 均为 5）
    static constexpr i32 MAX_JUMP_RADIUS = 5;
    /// 初速度倍率（vanilla GoatAi 传入的 maxJumpVelocityMultiplier）
    static constexpr f32 MAX_JUMP_VELOCITY_MULTIPLIER = 3.5714288f;
    /// 冷却下限/上限（vanilla TIME_BETWEEN_LONG_JUMPS = UniformInt.of(600, 1200)）
    static constexpr i32 COOLDOWN_MIN_TICKS = 600;
    static constexpr i32 COOLDOWN_MAX_TICKS = 1200;
    /// 允许的抛射角（vanilla ALLOWED_ANGLES）
    static constexpr i32 ALLOWED_ANGLES[] = {65, 70, 75, 80};
    /// 起跳后速度整体缩放（vanilla LongJumpUtil 末尾 scale(0.95F)）
    static constexpr f32 JUMP_VECTOR_SCALE = 0.95f;
    /// 落地时水平速度衰减（vanilla LongJumpMidJump.stop：multiply(0.1F, 1.0, 0.1F)）
    static constexpr f32 LANDING_HORIZONTAL_DAMPING = 0.1f;

    explicit GoatLongJumpGoal(GoatEntity* goat);

    [[nodiscard]] bool shouldExecute() override;
    [[nodiscard]] bool shouldContinueExecuting() override;
    void startExecuting() override;
    void resetTask() override;
    void tick() override;

    [[nodiscard]] std::string getTypeName() const override
    {
        return "GoatLongJumpGoal";
    }

    /// 当前阶段（测试/诊断用）
    enum class Phase : u8 {
        Idle,
        Preparing, ///< 已选中落点，原地蓄力
        Jumping,   ///< 已在空中
    };

    [[nodiscard]] Phase phase() const noexcept
    {
        return m_phase;
    }

    /// 冷却剩余 tick
    [[nodiscard]] i32 cooldown() const noexcept
    {
        return m_cooldown;
    }

    /// 已选中的落点（无则返回无效坐标）
    [[nodiscard]] const BlockPos& chosenTarget() const noexcept
    {
        return m_target;
    }

    /// 计算从 from 到 target 的抛射初速度（对齐 vanilla LongJumpUtil.calculateJumpVectorForAngle）
    /// @param from 起跳实体位置（脚底中心）
    /// @param target 落点方块中心
    /// @param maxVelocity 初速度上限（JUMP_STRENGTH * 倍率）
    /// @param angleDeg 抛射角
    /// @return 初速度；无解时返回 std::nullopt
    [[nodiscard]] std::optional<Vector3> calculateJumpVector(
        const Vector3& from, const Vector3& target, f64 maxVelocity, i32 angleDeg) const;

private:
    /// 在周围搜索可跳落点并蓄力（对齐 LongJumpToRandomPos.start + tick 的 pickCandidate）
    void _pickCandidate();

    /// 落点是否可接受（脚下方块实心、非本格、寻路不可达）
    [[nodiscard]] bool _isAcceptableLandingSpot(const BlockPos& pos) const;

    /// 刷新冷却为 600~1200 tick
    void _setRandomCooldown();

    /// 播放 entity.goat.<suffix> 音效（起跳/落地）
    void _playEntitySound(const char* suffix) const;

    GoatEntity* m_goat;
    Phase m_phase = Phase::Idle;
    i32 m_cooldown = 0;
    i32 m_prepareTicks = 0;
    i32 m_airTicks = 0;
    BlockPos m_target;
    Vector3 m_jumpVector{0.0, 0.0, 0.0};
};

} // namespace entity::ai::goal
} // namespace mc
