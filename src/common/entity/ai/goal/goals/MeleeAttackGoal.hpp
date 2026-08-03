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

#include "../../../../core/Types.hpp"
#include "../Goal.hpp"
#include "../GoalConstants.hpp"
#include <string>

namespace mc {

// 前向声明
class CreatureEntity;
class LivingEntity;

namespace entity::ai::goal {

/**
 * @brief 近战攻击目标
 *
 * 使生物攻击目标实体。
 */
class MeleeAttackGoal : public Goal {
public:
    /**
     * @brief 构造函数
     * @param creature 生物实体
     * @param speed 移动速度倍率
     * @param useLongMemory 是否使用长期记忆（目标丢失后继续追踪）
     */
    MeleeAttackGoal(CreatureEntity* creature, f64 speed, bool useLongMemory = false);

    [[nodiscard]] bool shouldExecute() override;
    [[nodiscard]] bool shouldContinueExecuting() override;
    void startExecuting() override;
    void resetTask() override;
    void tick() override;

    [[nodiscard]] std::string getTypeName() const noexcept override { return "MeleeAttackGoal"; }

protected:
    /**
     * @brief 检查是否可以攻击目标
     * @param target 目标实体
     * @return 是否可以攻击
     */
    [[nodiscard]] bool _canAttack(LivingEntity* target) const;

    /**
     * @brief 检查并执行攻击
     * @param target 目标实体
     * @param distToEnemySqr 到敌人的距离平方
     */
    virtual void checkAndPerformAttack(LivingEntity* target, f64 distToEnemySqr);

    /**
     * @brief 执行攻击
     * @param target 目标实体
     */
    void _attackTarget(LivingEntity* target);

    /**
     * @brief 计算攻击距离平方
     */
    [[nodiscard]] virtual f32 getAttackReachSqr(LivingEntity* target) const;

    CreatureEntity* m_creature;
    f64 m_speed;
    bool m_useLongMemory;
    LivingEntity* m_attackTarget = nullptr;
    i32 m_attackCooldown = 0;
    i32 m_pathRecalculateTimer = 0;
    f64 m_targetX = 0.0;
    f64 m_targetY = 0.0;
    f64 m_targetZ = 0.0;
    i32 m_failedPathFindingPenalty = 0;
    bool m_canPenalize = false; // 路径失败惩罚开关
    u32 m_lastCheckTime = 0;    // 游戏时间节流

    static constexpr i32 ATTACK_COOLDOWN_TICKS = 20;   // 攻击冷却（ticks）
    static constexpr f32 STOP_ATTACK_DISTANCE = 32.0f; // 停止追踪距离
};

} // namespace entity::ai::goal
} // namespace mc
