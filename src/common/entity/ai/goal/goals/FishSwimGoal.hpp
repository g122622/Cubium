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

#include "RandomSwimmingGoal.hpp"
#include "common/core/Types.hpp"
#include <string>

namespace mc {

// 前向声明
class AbstractFishEntity;

namespace entity::ai::goal {

/**
 * @brief 鱼类游泳目标
 *
 * 继承自 RandomSwimmingGoal，添加了 canRandomSwim() 条件检查。
 * 对于群游鱼类，只有在没有群首时才会自主游泳，否则跟随群首。
 */
class FishSwimGoal : public RandomSwimmingGoal {
public:
    /**
     * @brief 构造函数
     * @param fish 鱼类实体
     */
    explicit FishSwimGoal(AbstractFishEntity* fish);

    /**
     * @brief 构造函数（自定义速度和概率）
     * @param fish 鱼类实体
     * @param speed 游泳速度倍率
     * @param chance 执行概率倒数
     */
    FishSwimGoal(AbstractFishEntity* fish, f64 speed, i32 chance);

    [[nodiscard]] bool shouldExecute() override;

    [[nodiscard]] std::string getTypeName() const override { return "FishSwimGoal"; }

private:
    AbstractFishEntity* m_fish;
};

} // namespace entity::ai::goal
} // namespace mc
