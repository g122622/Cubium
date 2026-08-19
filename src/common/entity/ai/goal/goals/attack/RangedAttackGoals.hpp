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
#include "common/entity/ai/goal/Goal.hpp"
#include <string>

namespace mc {

// Forward declarations
class LivingEntity;
class MobEntity;

namespace entity::ai::goal {

/**
 * @brief 远程攻击目标
 *
 * 使实体能够在一定距离外进行远程攻击。
 * 适用于骷髅、烈焰人等使用远程攻击的怪物。
 */
class RangedAttackGoal : public Goal {
public:
    /**
     * @brief 构造函数
     * @param mob 拥有此目标的生物
     * @param speed 移动速度倍率
     * @param attackIntervalMin 最小攻击间隔（ticks）
     * @param attackIntervalMax 最大攻击间隔（ticks）
     * @param attackRadius 攻击半径
     */
    RangedAttackGoal(MobEntity* mob, f64 speed, i32 attackIntervalMin, i32 attackIntervalMax, f32 attackRadius);

    ~RangedAttackGoal() noexcept override = default;

    [[nodiscard]] bool shouldExecute() override;
    [[nodiscard]] bool shouldContinueExecuting() override;
    void startExecuting() override;
    void resetTask() override;
    void tick() override;

    [[nodiscard]] std::string getTypeName() const override { return "RangedAttackGoal"; }

protected:
    /**
     * @brief 执行远程攻击
     * @param target 目标实体
     * @param charge 蓄力程度（0.0-1.0）
     */
    virtual void performAttack(LivingEntity* target, f32 charge);

    MobEntity* m_mob;
    LivingEntity* m_target = nullptr;
    f64 m_speed;
    i32 m_attackIntervalMin;
    i32 m_attackIntervalMax;
    f32 m_attackRadius;
    f32 m_maxAttackDistanceSq; // 缓存的平方距离
    i32 m_attackTime = -1;     // 初始值为 -1
    i32 m_seenTime = 0;        // 能看到目标的时间

    static constexpr i32 MIN_SEEN_TIME = 5; // 停止移动前需要看到的tick数（对齐 vanilla RangedAttackGoal.seeTime>=5）
};

/**
 * @brief 弓箭攻击目标
 *
 * 使用弓进行远程攻击。
 * 需要实体实现IRangedAttackMob接口。
 */
class RangedBowAttackGoal : public RangedAttackGoal {
public:
    /**
     * @brief 构造函数
     * @param mob 拥有此目标的生物
     * @param speed 移动速度倍率
     * @param attackIntervalMin 最小攻击间隔
     * @param attackIntervalMax 最大攻击间隔
     */
    RangedBowAttackGoal(MobEntity* mob, f64 speed, i32 attackIntervalMin, i32 attackIntervalMax);

    ~RangedBowAttackGoal() noexcept override = default;

    [[nodiscard]] bool shouldExecute() override;
    void startExecuting() override;
    void resetTask() override;
    void tick() override;

    [[nodiscard]] std::string getTypeName() const override { return "RangedBowAttackGoal"; }

    /**
     * @brief 设置最小攻击间隔
     *
     * 允许在目标创建后动态调整最小攻击间隔，用于根据游戏难度
     * 调整骷髅的射击频率（困难难度射击更快）。
     *
     * 对应 MC 原版 RangedBowAttackGoal.setMinAttackInterval()。
     *
     * @param interval 新的最小攻击间隔（ticks）
     */
    void setMinAttackInterval(i32 interval)
    {
        m_attackIntervalMin = interval;
        // 保护 max>=min 不变量：performAttack 用 nextInt(max-min+1) 计算冷却，
        // 若新 min > 构造时硬编码的 max（如沼骸 min=50/70 > 默认 max=40），
        // max-min+1 会变为非正数触发 nextInt 的 MC_ASSERT_RELEASE(bound>0) 断言。
        // 此时把 max 抬到 min，使 nextInt(1) 返回 0（冷却=min，确定性）。
        if (m_attackIntervalMax < m_attackIntervalMin) {
            m_attackIntervalMax = m_attackIntervalMin;
        }
    }

protected:
    void performAttack(LivingEntity* target, f32 charge) override;

private:
    bool m_strafingClockwise = false; // 是否顺时针走位
    bool m_strafingBackwards = false; // 是否向后走位
    i32 m_strafingTime = -1;          // 走位时间计数器

    static constexpr i32 STRAFE_THRESHOLD = 20; // 走位方向变化阈值
};

/**
 * @brief 弩攻击目标状态枚举
 */
enum class CrossbowState : u8 {
    Uncharged,    // 未装填
    Charging,     // 装填中
    Charged,      // 已装填
    ReadyToAttack // 准备攻击
};

/**
 * @brief 弩远程攻击目标
 *
 * 使用弩进行远程攻击的AI目标。
 * 需要实体实现ICrossbowUser接口。
 *
 * 状态机:
 * - Uncharged: 等待进入攻击范围
 * - Charging: 装填弩（25 ticks基础时间）
 * - Charged: 装填完成，等待发射
 * - ReadyToAttack: 发射弩箭
 */
class RangedCrossbowAttackGoal : public Goal {
public:
    /**
     * @brief 构造函数
     * @param mob 拥有此目标的生物
     * @param speed 移动速度倍率
     * @param attackRadius 攻击半径
     */
    RangedCrossbowAttackGoal(MobEntity* mob, f64 speed, f32 attackRadius);

    ~RangedCrossbowAttackGoal() noexcept override = default;

    [[nodiscard]] bool shouldExecute() override;
    [[nodiscard]] bool shouldContinueExecuting() override;
    void startExecuting() override;
    void resetTask() override;
    void tick() override;

    [[nodiscard]] std::string getTypeName() const override { return "RangedCrossbowAttackGoal"; }

private:
    /**
     * @brief 检查实体是否持有弩
     */
    [[nodiscard]] bool _isHoldingCrossbow() const;

    /**
     * @brief 检查视线并更新可见时间
     */
    void _updateSeenTime();

    /**
     * @brief 处理未装填状态
     */
    void _handleUnchargedState();

    /**
     * @brief 处理装填中状态
     */
    void _handleChargingState();

    /**
     * @brief 处理已装填状态
     */
    void _handleChargedState();

    /**
     * @brief 处理准备攻击状态
     */
    void _handleReadyToAttackState();

    MobEntity* m_mob;
    LivingEntity* m_target = nullptr;
    f64 m_speed;
    f32 m_attackRadius;
    f32 m_attackRadiusSq;

    CrossbowState m_crossbowState = CrossbowState::Uncharged;
    i32 m_seenTime = 0;     // 能看到目标的时间
    i32 m_chargeTime = 0;   // 装填计时器
    i32 m_cooldownTime = 0; // 装填后等待时间
    i32 m_moveCooldown = 0; // 移动冷却

    static constexpr i32 MIN_SEEN_TIME = 5;        // 最小可见时间才开始攻击
    static constexpr i32 CHARGED_WAIT_MIN = 20;    // 装填后最小等待时间
    static constexpr i32 CHARGED_WAIT_MAX = 40;    // 装填后最大等待时间
    static constexpr i32 MOVE_COOLDOWN_MIN = 20;   // 移动冷却最小值
    static constexpr i32 MOVE_COOLDOWN_MAX = 40;   // 移动冷却最大值
    static constexpr f32 ARROW_VELOCITY = 3.15f;   // 箭矢速度
    static constexpr f32 FIREWORK_VELOCITY = 1.6f; // 烟花速度
};

} // namespace entity::ai::goal
} // namespace mc
