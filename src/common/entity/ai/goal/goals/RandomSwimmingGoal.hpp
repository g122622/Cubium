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
#include "../../../../util/math/Vector3.hpp"
#include "../Goal.hpp"
#include <string>

namespace mc {

// 前向声明
class CreatureEntity;

namespace entity::ai::goal {

/**
 * @brief 水下随机游泳目标
 *
 * 使水生生物在水中随机选择方向游泳。
 * 类似于 RandomWalkingGoal，但专门针对水下环境。
 */
class RandomSwimmingGoal : public Goal {
public:
    /**
     * @brief 构造函数
     * @param creature 拥有此目标的生物
     * @param speed 游泳速度倍率
     */
    RandomSwimmingGoal(CreatureEntity* creature, f64 speed);

    /**
     * @brief 构造函数（带执行概率）
     * @param creature 拥有此目标的生物
     * @param speed 游泳速度倍率
     * @param chance 执行概率倒数（1/chance 的概率执行）
     */
    RandomSwimmingGoal(CreatureEntity* creature, f64 speed, i32 chance);

    [[nodiscard]] bool shouldExecute() override;
    [[nodiscard]] bool shouldContinueExecuting() override;
    void startExecuting() override;
    void resetTask() override;
    void tick() override;

    /**
     * @brief 强制下次执行
     */
    void makeUpdate() noexcept { m_forceUpdate = true; }

    /**
     * @brief 设置执行概率倒数
     */
    void setExecutionChance(i32 chance) noexcept { m_executionChance = chance; }

    [[nodiscard]] std::string getTypeName() const override { return "RandomSwimmingGoal"; }

protected:
    /**
     * @brief 获取随机游泳目标位置
     * @param outPos 输出位置
     * @return 是否找到有效位置
     */
    [[nodiscard]] virtual bool getRandomSwimPosition(Vector3& outPos);

    CreatureEntity* m_creature;
    f64 m_speed;
    f64 m_targetX = 0.0;
    f64 m_targetY = 0.0;
    f64 m_targetZ = 0.0;
    i32 m_executionChance;
    i32 m_timeoutCounter = 0;
    bool m_forceUpdate = false;
};

} // namespace entity::ai::goal
} // namespace mc
