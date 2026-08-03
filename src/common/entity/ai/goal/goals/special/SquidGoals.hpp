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
#include "common/entity/ai/goal/GoalFlag.hpp"
#include <string>

namespace mc {

// 前向声明
class SquidEntity;
class LivingEntity;

namespace entity::ai::goal {

/**
 * @brief 鱿鱼随机移动目标
 *
 * 鱿鱼在水中随机游动的主要行为。
 *
 * 执行条件:
 * - 始终可以执行（shouldExecute 始终返回 true）
 *
 * tick 行为:
 * - 如果空闲时间超过 100 tick，停止移动（设置移动向量为零）
 * - 否则以 1/50 概率，或不在水中，或没有移动向量时，生成新的随机移动向量
 * - 随机向量计算：
 *   - 角度：随机 [0, 2π)
 *   - X = cos(角度) * 0.2
 *   - Y = -0.1 + random * 0.2 (范围 [-0.1, 0.1])
 *   - Z = sin(角度) * 0.2
 *
 * 关键常量:
 * - IDLE_THRESHOLD = 100 tick（空闲阈值）
 * - RANDOM_CHANCE = 50（执行概率倒数）
 * - HORIZONTAL_SPEED = 0.2f（水平移动向量大小）
 * - VERTICAL_MIN = -0.1f（垂直移动向量最小值）
 * - VERTICAL_RANGE = 0.2f（垂直移动向量范围）
 */
class SquidMoveRandomGoal : public Goal {
public:
    /**
     * @brief 构造函数
     * @param squid 鱿鱼实体
     */
    explicit SquidMoveRandomGoal(SquidEntity* squid);

    ~SquidMoveRandomGoal() override = default;

    [[nodiscard]] bool shouldExecute() override;
    void tick() override;

    [[nodiscard]] std::string getTypeName() const override { return "SquidMoveRandomGoal"; }

private:
    SquidEntity* m_squid;

    // 行为常量
    static constexpr i32 IDLE_THRESHOLD = 100;    // 空闲tick阈值
    static constexpr i32 RANDOM_CHANCE = 50;      // 1/50概率触发新方向
    static constexpr f32 HORIZONTAL_SPEED = 0.2f; // 水平移动向量大小
    static constexpr f32 VERTICAL_MIN = -0.1f;    // 垂直移动向量最小值
    static constexpr f32 VERTICAL_RANGE = 0.2f;   // 垂直移动向量范围
};

/**
 * @brief 鱿鱼逃跑目标
 *
 * 当鱿鱼受到攻击时，向相反方向逃跑并产生气泡粒子。
 * 参考 MC 1.21.11 Squid.SquidFleeGoal 实现。
 *
 * 执行条件:
 * - 鱿鱼必须在水中
 * - 必须有复仇目标（攻击者）
 * - 复仇目标距离必须小于 10 格（距离平方 < 100）
 *
 * tick 行为:
 * - 计算远离敌人的方向向量
 * - 检查逃跑目标位置的方块和流体状态：仅当目标位置是水或空气时才逃跑
 * - 根据距离调整逃跑速度：
 *   - 基础速度 = 3.0
 *   - 距离 > 5 格时：速度 = 3.0 - (距离 - 5) / 5
 * - 如果目标位置是空气，移除 Y 分量避免跳出水面
 * - 设置移动向量（除以 20 转换为每 tick 速度）
 * - 每 10 tick 的第 5 tick 产生气泡粒子
 *
 * 关键常量:
 * - FLEE_DISTANCE_SQ = 100.0D（触发逃跑的距离平方阈值）
 * - BASE_FLEE_SPEED = 3.0f（基础逃跑速度）
 * - DISTANCE_THRESHOLD = 5.0（速度衰减开始距离）
 * - SPEED_SCALE = 20.0f（速度缩放因子）
 * - BUBBLE_INTERVAL = 10（气泡粒子产生间隔）
 * - BUBBLE_OFFSET = 5（气泡粒子产生偏移）
 */
class SquidFleeGoal : public Goal {
public:
    /**
     * @brief 构造函数
     * @param squid 鱿鱼实体
     */
    explicit SquidFleeGoal(SquidEntity* squid);

    ~SquidFleeGoal() override = default;

    [[nodiscard]] bool shouldExecute() override;
    void startExecuting() override;
    void tick() override;

    [[nodiscard]] std::string getTypeName() const override { return "SquidFleeGoal"; }

private:
    SquidEntity* m_squid;
    LivingEntity* m_fleeTarget = nullptr;
    i32 m_tickCounter = 0;

    // 行为常量
    static constexpr f64 FLEE_DISTANCE_SQ = 100.0; // 触发逃跑的距离平方阈值 (10^2)
    static constexpr f32 BASE_FLEE_SPEED = 3.0f;   // 基础逃跑速度
    static constexpr f64 DISTANCE_THRESHOLD = 5.0; // 速度衰减开始距离
    static constexpr f32 SPEED_SCALE = 20.0f;      // 速度缩放因子
    static constexpr i32 BUBBLE_INTERVAL = 10;     // 气泡粒子产生间隔
    static constexpr i32 BUBBLE_OFFSET = 5;        // 气泡粒子产生偏移
};

} // namespace entity::ai::goal
} // namespace mc
