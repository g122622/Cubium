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
#include "common/world/block/BlockPos.hpp"
#include <string>

namespace mc {

// 前向声明
class CreatureEntity;

namespace entity::ai::goal {

/**
 * @brief 恐慌逃跑目标
 *
 * 当实体受到攻击或着火时，随机逃跑。
 */
class PanicGoal : public Goal {
public:
    /**
     * @brief 构造函数
     * @param creature 拥有此目标的生物
     * @param speed 逃跑速度倍率
     */
    PanicGoal(CreatureEntity* creature, f64 speed);

    [[nodiscard]] bool shouldExecute() override;
    [[nodiscard]] bool shouldContinueExecuting() override;
    void startExecuting() override;
    void resetTask() override;
    void tick() override;

    /**
     * @brief 检查是否正在逃跑
     */
    [[nodiscard]] bool isRunning() const noexcept { return m_running; }

    [[nodiscard]] std::string getTypeName() const override { return "PanicGoal"; }

protected:
    /**
     * @brief 获取最近的水源位置（着火时）
     *
     * 遍历立方体区域找最近的水方块。
     *
     * @param horizontalRange 水平搜索范围
     * @param verticalRange 垂直搜索范围
     * @return 水源方块位置，如果没有则返回 (0, 0, 0)
     */
    [[nodiscard]] BlockPos _getRandomWaterPosition(i32 horizontalRange, i32 verticalRange);

private:
    /**
     * @brief 寻找随机逃跑位置
     * @return 是否找到有效位置
     */
    [[nodiscard]] bool _findRandomPosition();

    CreatureEntity* m_creature;
    f64 m_speed;
    f32 m_targetX = 0.0f;
    f32 m_targetY = 0.0f;
    f32 m_targetZ = 0.0f;
    bool m_running = false;
};

} // namespace entity::ai::goal
} // namespace mc
