/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to Use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software be
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
 * @brief 飞行目标
 *
 * 控制飞行生物的随机飞行行为。用于鹦鹉、蝙蝠等飞行生物。
 * 在三维空间中选择随机空中位置并导航过去，避开水面和岩浆。
 *
 * 与WaterAvoidingRandomFlyingGoal的区别：
 * FlyGoal使用概率触发机制（默认1/120每tick），而WaterAvoidingRandomFlyingGoal使用更小的概率。
 * FlyGoal适用于需要更频繁飞行的生物。
 *
 * TODO: 当前无实体直接使用此目标。BatEntity使用自定义的BatRandomFlyGoal，
 * ParrotEntity使用WaterAvoidingRandomFlyingGoal，PhantomEntity使用自定义目标。
 * 此目标作为通用飞行行为备用，待需要通用飞行AI的实体实现后接入。
 */
class FlyGoal : public Goal {
public:
    /**
     * @brief 构造函数
     * @param creature 拥有此目标的飞行生物
     * @param speed 飞行速度倍率
     */
    FlyGoal(CreatureEntity* creature, f64 speed);

    ~FlyGoal() override = default;

    [[nodiscard]] bool shouldExecute() override;
    [[nodiscard]] bool shouldContinueExecuting() override;
    void startExecuting() override;
    void resetTask() override;
    void tick() override;

    [[nodiscard]] std::string getTypeName() const override { return "FlyGoal"; }

protected:
    /**
     * @brief 生成随机飞行目标位置
     * @return 是否找到有效位置
     */
    [[nodiscard]] bool _generateFlightTarget();

    CreatureEntity* m_creature;
    f64 m_speed;
    f64 m_targetX = 0.0;
    f64 m_targetY = 0.0;
    f64 m_targetZ = 0.0;
    i32 m_timeout = 0;

    // 飞行范围常量
    static constexpr i32 FLIGHT_XZ_RANGE = 10;   // 水平飞行范围
    static constexpr i32 FLIGHT_Y_RANGE = 7;     // 垂直飞行范围
    static constexpr i32 MAX_FLIGHT_TIME = 600;  // 最大飞行时间（tick）
    static constexpr i32 EXECUTION_CHANCE = 120; // 执行概率倒数
};

} // namespace entity::ai::goal
} // namespace mc
