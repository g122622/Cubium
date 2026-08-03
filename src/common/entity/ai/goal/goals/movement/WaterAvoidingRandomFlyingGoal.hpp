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
#include "common/util/math/Vector3.hpp"
#include <string>

namespace mc {

// 前向声明
class CreatureEntity;

namespace entity::ai::goal {

/**
 * @brief 避水随机飞行目标
 *
 * 类似于 RandomWalkingGoal，但用于飞行实体，且会避开水域。
 * 飞行实体会选择一个随机的空中位置并飞行过去。
 *
 * 与 WaterAvoidingRandomWalkingGoal 的区别：
 * - 飞行目标不依赖地面导航，直接设置目标位置
 * - 飞行目标可以在三维空间中选择目标点
 * - 飞行目标使用飞行移动控制器
 */
class WaterAvoidingRandomFlyingGoal : public Goal {
public:
    /**
     * @brief 构造函数
     * @param creature 拥有此目标的生物
     * @param speed 飞行速度倍率
     */
    WaterAvoidingRandomFlyingGoal(CreatureEntity* creature, f64 speed);

    ~WaterAvoidingRandomFlyingGoal() override = default;

    /**
     * @brief 构造函数（带执行概率）
     * @param creature 拥有此目标的生物
     * @param speed 飞行速度倍率
     * @param chance 执行概率（0.0-1.0）
     */
    WaterAvoidingRandomFlyingGoal(CreatureEntity* creature, f64 speed, f32 chance);

    [[nodiscard]] bool shouldExecute() override;
    [[nodiscard]] bool shouldContinueExecuting() override;
    void startExecuting() override;
    void resetTask() override;
    void tick() override;

    [[nodiscard]] std::string getTypeName() const override { return "WaterAvoidingRandomFlyingGoal"; }

protected:
    /**
     * @brief 获取随机飞行目标位置
     * @return 是否找到有效位置
     */
    [[nodiscard]] bool getRandomPosition();

    /**
     * @brief 检查位置是否在水或岩浆中
     */
    [[nodiscard]] bool isInWaterOrLava(f64 x, f64 y, f64 z) const;

    CreatureEntity* m_creature;
    f64 m_speed;
    f32 m_chance;
    f64 m_targetX = 0.0;
    f64 m_targetY = 0.0;
    f64 m_targetZ = 0.0;
    i32 m_timeout = 0;
    bool m_isRunning = false;

    // 常量
    static constexpr i32 MAX_TIMEOUT = 600;      // 最大飞行时间（30秒）
    static constexpr i32 XZ_RANGE = 8;           // 水平搜索范围
    static constexpr i32 Y_RANGE_HOVER = 7;      ///< HoverRandomPos 垂直搜索范围
    static constexpr i32 Y_RANGE_FALLBACK = 4;   ///< AirAndWaterRandomPos 垂直搜索范围（备选策略）
    static constexpr i32 Y_OFFSET_FALLBACK = -2; ///< AirAndWaterRandomPos Y轴偏移（备选策略）
};

} // namespace entity::ai::goal
} // namespace mc
