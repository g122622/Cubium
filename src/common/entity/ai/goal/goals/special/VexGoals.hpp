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

#include "../../Goal.hpp"
#include "../../GoalFlag.hpp"
#include "../target/TargetGoals.hpp"
#include "core/Types.hpp"
#include "util/math/Vector3.hpp"
#include <string>

namespace mc {

// Forward declarations
class VexEntity;
class LivingEntity;
class IWorld;

namespace entity::ai::goal {

/**
 * @brief 恼鬼冲锋攻击目标
 *
 * 恼鬼特有的冲锋攻击AI。恼鬼会飞向目标的眼睛位置进行攻击。
 * 与普通近战攻击不同，恼鬼直接飞向目标而非寻路。
 */
class VexChargeAttackGoal : public Goal {
public:
    /**
     * @brief 构造函数
     * @param vex 拥有此目标的恼鬼实体
     */
    explicit VexChargeAttackGoal(VexEntity* vex);

    ~VexChargeAttackGoal() override = default;

    [[nodiscard]] bool shouldExecute() override;
    [[nodiscard]] bool shouldContinueExecuting() override;
    void startExecuting() override;
    void resetTask() override;
    void tick() override;

    [[nodiscard]] std::string getTypeName() const override { return "VexChargeAttackGoal"; }

private:
    /**
     * @brief 检查并执行攻击
     * @param target 目标实体
     * @param distSq 到目标的距离平方
     */
    void _checkAndPerformAttack(LivingEntity* target, f64 distSq);

    VexEntity* m_vex;
    LivingEntity* m_attackTarget = nullptr;
    i32 m_attackCooldown = 0;

    // 常量
    static constexpr f64 MIN_CHARGE_DISTANCE_SQ = 4.0; // 最小冲锋距离（2格的平方）
    static constexpr f64 STOP_CHASE_DISTANCE_SQ = 9.0; // 停止追击距离（3格的平方）
    static constexpr i32 ATTACK_COOLDOWN_TICKS = 20;   // 攻击冷却（1秒）
    static constexpr i32 CHARGE_PROBABILITY = 7;       // 冲锋概率倒数（1/7 ≈ 14%）
};

/**
 * @brief 恼鬼随机飞行目标
 *
 * 恼鬼在绑定点周围随机飞行。
 */
class VexMoveRandomGoal : public Goal {
public:
    /**
     * @brief 构造函数
     * @param vex 拥有此目标的恼鬼实体
     */
    explicit VexMoveRandomGoal(VexEntity* vex);

    ~VexMoveRandomGoal() override = default;

    [[nodiscard]] bool shouldExecute() override;
    [[nodiscard]] bool shouldContinueExecuting() override;
    void tick() override;

    [[nodiscard]] std::string getTypeName() const override { return "VexMoveRandomGoal"; }

private:
    VexEntity* m_vex;

    // 常量
    static constexpr i32 RANDOM_PROBABILITY = 7; // 随机移动概率倒数（1/7 ≈ 14%）
    static constexpr f32 WANDER_SPEED = 0.25f;   // 漫游速度
    static constexpr i32 WANDER_RANGE_X = 7;     // X轴漫游范围（±7格）
    static constexpr i32 WANDER_RANGE_Y = 5;     // Y轴漫游范围（±5格）
    static constexpr i32 WANDER_RANGE_Z = 7;     // Z轴漫游范围（±7格）
};

/**
 * @brief 恼鬼复制主人目标
 *
 * 当恼鬼的主人（唤魔者）有攻击目标时，恼鬼也会攻击该目标。
 */
class VexCopyOwnerTargetGoal : public TargetGoal {
public:
    /**
     * @brief 构造函数
     * @param vex 拥有此目标的恼鬼实体
     */
    explicit VexCopyOwnerTargetGoal(VexEntity* vex);

    ~VexCopyOwnerTargetGoal() override = default;

    [[nodiscard]] bool shouldExecute() override;
    void startExecuting() override;

    [[nodiscard]] std::string getTypeName() const override { return "VexCopyOwnerTargetGoal"; }

private:
    VexEntity* m_vex;
};

} // namespace entity::ai::goal
} // namespace mc
