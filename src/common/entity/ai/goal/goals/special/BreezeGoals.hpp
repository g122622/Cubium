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
 * IMPLIED, INCLUDING NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#pragma once

#include "common/core/Types.hpp"
#include "common/entity/ai/goal/Goal.hpp"
#include "common/util/math/Vector3.hpp"
#include <optional>
#include <string>

namespace mc {

class BreezeEntity;
class LivingEntity;

namespace entity::ai::goal {

// ============================================================================
// BreezeShootGoal
// ============================================================================

/**
 * @brief 旋风人风弹射击目标
 *
 * 旋风人向攻击目标投掷风弹的行为。对应 MC Java 版 BreezeAi 中的 Shoot 行为。
 *
 * 行为流程：
 * 1. 充能阶段（15 ticks）：旋风人吸气动画，看向目标
 * 2. 发射阶段：创建风弹弹射物，播放发射音效
 * 3. 恢复阶段（4 ticks）：发射后短暂恢复
 * 4. 冷却阶段（10 ticks）：发射完毕后进入冷却
 *
 * 启动条件：
 * - 存在攻击目标
 * - 射击冷却已过
 * - 不在充能/恢复状态
 * - 射击许可已获得（由 Slide/LongJump 结束时设置）
 * - 站立姿态
 * - 目标在攻击范围内（< 16 格）
 *
 * 射击精度受难度影响：简单难度下精度低（inaccuracy=5），困难难度下精度高（inaccuracy=-3）
 */
class BreezeShootGoal : public Goal {
public:
    explicit BreezeShootGoal(BreezeEntity* breeze);

    [[nodiscard]] bool shouldExecute() override;
    [[nodiscard]] bool shouldContinueExecuting() override;
    void startExecuting() override;
    void resetTask() override;
    void tick() override;

    [[nodiscard]] std::string getTypeName() const override { return "BreezeShootGoal"; }

private:
    BreezeEntity* m_breeze;
    LivingEntity* m_target = nullptr;
    i32 m_shootCooldown = 0;   // 射击冷却计时器
    bool m_isCharging = false; // 是否正在充能
    bool m_hasFired = false;   // 是否已经发射
    i32 m_chargeTime = 0;      // 充能计时器
    i32 m_recoverTime = 0;     // 恢复计时器

    // MC 原版常量
    static constexpr i32 CHARGE_TICKS = 15;           // 充能时间
    static constexpr i32 RECOVER_TICKS = 4;           // 恢复时间
    static constexpr i32 SHOOT_COOLDOWN_TICKS = 10;   // 射击冷却时间
    static constexpr f64 ATTACK_RANGE_MAX_SQ = 256.0; // 最大攻击距离平方（16格）
    static constexpr f32 PROJECTILE_SPEED = 0.7f;     // 弹射物速度
};

// ============================================================================
// BreezeLongJumpGoal
// ============================================================================

/**
 * @brief 旋风人长跳目标
 *
 * 旋风人向目标后方长跳的行为。对应 MC Java 版 BreezeAi 中的 LongJump 行为。
 *
 * 行为流程：
 * 1. 吸气阶段（10 ticks）：旋风人蓄力动画，看向跳转目标位置
 * 2. 跳跃阶段：计算跳跃向量，施加速度，播放跳跃音效
 * 3. 着陆阶段：检测着陆，播放着陆音效，设置射击许可
 *
 * 启动条件：
 * - 存在攻击目标
 * - 长跳冷却已过
 * - 没有射击许可（避免与射击冲突）
 * - 在地面上或水中
 * - 目标不在近距离（> 4 格）
 * - 能够跳跃（头顶有空间，不在蜂蜜上）
 * - 找到有效的跳跃目标位置
 *
 * 跳跃后设置射击许可（100 ticks），使 BreezeShootGoal 可以激活。
 */
class BreezeLongJumpGoal : public Goal {
public:
    explicit BreezeLongJumpGoal(BreezeEntity* breeze);

    [[nodiscard]] bool shouldExecute() override;
    [[nodiscard]] bool shouldContinueExecuting() override;
    void startExecuting() override;
    void resetTask() override;
    void tick() override;

    [[nodiscard]] std::string getTypeName() const override { return "BreezeLongJumpGoal"; }

private:
    /**
     * @brief 检查旋风人当前位置是否可以跳跃
     *
     * 头顶需要有至少4格空气或水，且不能站在蜂蜜上。
     */
    [[nodiscard]] bool _canJumpFromCurrentPosition() const;

    /**
     * @brief 在目标身后随机选择一个跳跃目标点
     * @return 目标位置，如果找不到有效位置则返回 nullopt
     */
    [[nodiscard]] std::optional<Vector3> _findJumpTargetBehindAttackTarget() const;

    /**
     * @brief 计算指定角度的跳跃向量
     * @param angle 仰角（度）
     * @return 跳跃向量，如果无法到达则返回 nullopt
     */
    [[nodiscard]] std::optional<Vector3> _calculateJumpVector(f32 angle) const;

    /**
     * @brief 从允许的角度中随机选择最优跳跃向量
     * @return 跳跃向量，如果没有有效角度则返回 nullopt
     */
    [[nodiscard]] std::optional<Vector3> _calculateOptimalJumpVector() const;

    BreezeEntity* m_breeze;
    LivingEntity* m_target = nullptr;
    bool m_isInhaling = false; // 是否正在吸气
    bool m_isJumping = false;  // 是否正在跳跃中
    i32 m_inhaleTime = 0;      // 吸气计时器
    i32 m_jumpCooldown = 0;    // 跳跃冷却
    Vector3 m_jumpTargetPos;   // 跳跃目标位置

    // MC 原版常量
    static constexpr i32 INHALE_TICKS = 10;                  // 吸气时间
    static constexpr i32 JUMP_COOLDOWN_TICKS = 10;           // 正常跳跃冷却
    static constexpr i32 JUMP_COOLDOWN_HURT_TICKS = 2;       // 受伤后跳跃冷却
    static constexpr i32 SHOOT_PERMIT_TICKS = 100;           // 跳跃后射击许可持续时间
    static constexpr f32 JUMP_VELOCITY_SCALE = 0.058333334f; // MC 原版跳跃速度系数
    static constexpr f64 TOO_CLOSE_RANGE = 4.0;              // 太近无法跳跃的距离
    static constexpr f64 BEHIND_TARGET_MIN = 4.0;            // 目标身后最小距离
    static constexpr f64 BEHIND_TARGET_MAX = 8.0;            // 目标身后最大距离

    // 允许的跳跃角度（度），MC 原版使用 [40, 55, 60, 75, 80] 随机排列
    static constexpr f32 ALLOWED_ANGLES[] = {40.0f, 55.0f, 60.0f, 75.0f, 80.0f};
    static constexpr i32 ALLOWED_ANGLES_COUNT = 5;
};

// ============================================================================
// BreezeSlideGoal
// ============================================================================

/**
 * @brief 旋风人滑行目标
 *
 * 旋风人在地面上滑行移动的行为。对应 MC Java 版 BreezeAi 中的 Slide 行为。
 *
 * 行为逻辑：
 * - 如果在目标内圈（< 4格水平，< 10格垂直）：向远离目标的方向逃跑
 * - 如果不在内圈：
 *   - 50% 概率：跳到目标身后
 *   - 50% 概率：跳到中圈（4-8格距离）
 * - 设置行走目标，由 MovementController 处理滑行移动
 * - 滑行结束后（行走目标清除），设置射击许可
 *
 * 启动条件：
 * - 存在攻击目标
 * - 没有行走目标（未在移动中）
 * - 长跳冷却已过
 * - 没有射击许可（避免与射击冲突）
 * - 在地面上
 * - 不在水中
 * - 站立姿态
 */
class BreezeSlideGoal : public Goal {
public:
    explicit BreezeSlideGoal(BreezeEntity* breeze);

    [[nodiscard]] bool shouldExecute() override;
    [[nodiscard]] bool shouldContinueExecuting() override;
    void startExecuting() override;
    void resetTask() override;

    [[nodiscard]] std::string getTypeName() const override { return "BreezeSlideGoal"; }

private:
    /**
     * @brief 检查目标是否在内圈范围内
     *
     * 内圈定义：水平距离 < 4 格且垂直距离 < 10 格
     */
    [[nodiscard]] bool _isWithinInnerCircle(const Vector3& targetPos) const;

    /**
     * @brief 在目标身后随机选择一个位置
     * @return 目标位置
     */
    [[nodiscard]] Vector3 _randomPointBehindTarget() const;

    /**
     * @brief 在中圈范围内随机选择一个位置
     *
     * 中圈定义：旋风人到目标方向上 4-8 格距离
     * @return 目标位置
     */
    [[nodiscard]] Vector3 _randomPointInMiddleCircle() const;

    BreezeEntity* m_breeze;
    LivingEntity* m_target = nullptr;

    // MC 原版常量
    static constexpr f64 INNER_CIRCLE_HORIZONTAL = 4.0; // 内圈水平距离
    static constexpr f64 INNER_CIRCLE_VERTICAL = 10.0;  // 内圈垂直距离
    static constexpr f64 BEHIND_TARGET_MIN = 4.0;       // 目标身后最小距离
    static constexpr f64 BEHIND_TARGET_MAX = 8.0;       // 目标身后最大距离
    static constexpr f64 MIDDLE_CIRCLE_MIN = 4.0;       // 中圈最小距离
    static constexpr f64 MIDDLE_CIRCLE_MAX = 8.0;       // 中圈最大距离
    static constexpr f32 SLIDE_SPEED = 0.6f;            // 滑行速度
    static constexpr f32 SLIDE_CLOSE_ENOUGH = 1.0f;     // 接近目标的判定距离
    static constexpr i32 SHOOT_PERMIT_TICKS = 60;       // 滑行后射击许可持续时间
};

// ============================================================================
// BreezeShootWhenStuckGoal
// ============================================================================

/**
 * @brief 旋风人卡住时紧急射击目标
 *
 * 当旋风人处于困境（在水中、骑乘其他实体、或受到飘浮效果）时，
 * 设置射击许可以触发风弹攻击。对应 MC Java 版 BreezeAi 中的 ShootWhenStuck 行为。
 *
 * 这是一个一次性目标：启动后立即设置射击许可然后结束。
 *
 * 启动条件：
 * - 存在攻击目标
 * - 不在吸气/跳跃状态
 * - 没有跳跃目标
 * - 没有行走目标
 * - 没有射击许可
 * - 处于困境（在水中、骑乘实体、或受到飘浮效果）
 */
class BreezeShootWhenStuckGoal : public Goal {
public:
    explicit BreezeShootWhenStuckGoal(BreezeEntity* breeze);

    [[nodiscard]] bool shouldExecute() override;
    [[nodiscard]] bool shouldContinueExecuting() override;
    void startExecuting() override;

    [[nodiscard]] std::string getTypeName() const override { return "BreezeShootWhenStuckGoal"; }

private:
    BreezeEntity* m_breeze;

    // 射击许可持续时间
    static constexpr i32 SHOOT_PERMIT_TICKS = 60;
};

} // namespace entity::ai::goal
} // namespace mc
