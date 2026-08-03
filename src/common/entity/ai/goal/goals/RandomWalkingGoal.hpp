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
 * @brief 随机漫步目标
 *
 * 使生物随机选择一个方向并移动过去。
 */
class RandomWalkingGoal : public Goal {
public:
    /**
     * @brief 构造函数
     * @param creature 拥有此目标的生物
     * @param speed 移动速度倍率
     */
    RandomWalkingGoal(CreatureEntity* creature, f64 speed);

    /**
     * @brief 构造函数（带执行概率）
     * @param creature 拥有此目标的生物
     * @param speed 移动速度倍率
     * @param chance 执行概率倒数（1/chance 的概率执行）
     */
    RandomWalkingGoal(CreatureEntity* creature, f64 speed, i32 chance);

    /**
     * @brief 构造函数（带执行概率和空闲时间检查）
     * @param creature 拥有此目标的生物
     * @param speed 移动速度倍率
     * @param chance 执行概率倒数（1/chance 的概率执行）
     * @param checkIdleTime 是否检查空闲时间（如果空闲时间>=100则不执行）
     */
    RandomWalkingGoal(CreatureEntity* creature, f64 speed, i32 chance, bool checkIdleTime);

    [[nodiscard]] bool shouldExecute() override;
    [[nodiscard]] bool shouldContinueExecuting() override;
    void startExecuting() override;
    void resetTask() override;
    void tick() override;

    /**
     * @brief 强制下次执行
     */
    void makeUpdate() { m_forceUpdate = true; }

    /**
     * @brief 设置执行概率倒数
     */
    void setExecutionChance(i32 chance) { m_executionChance = chance; }

    [[nodiscard]] std::string getTypeName() const noexcept override { return "RandomWalkingGoal"; }

protected:
    /**
     * @brief 获取随机目标位置
     * @param outPos 输出位置
     * @return 是否找到有效位置
     */
    [[nodiscard]] virtual bool getRandomPosition(Vector3& outPos);

    CreatureEntity* m_creature;
    f64 m_speed;
    f64 m_targetX = 0.0;
    f64 m_targetY = 0.0;
    f64 m_targetZ = 0.0;
    i32 m_executionChance;
    i32 m_timeoutCounter = 0; // 超时计数器，最大漫步时间
    bool m_forceUpdate = false;
    bool m_checkIdleTime; // 是否检查空闲时间
};

} // namespace entity::ai::goal
} // namespace mc
