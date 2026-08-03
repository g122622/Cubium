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
#include <string>

namespace mc {

// 前向声明
class AnimalEntity;

namespace entity::ai::goal {

/**
 * @brief 跟随父母目标
 *
 * 幼体动物跟随成年动物。
 */
class FollowParentGoal : public Goal {
public:
    /**
     * @brief 构造函数
     * @param animal 幼体动物
     * @param speed 移动速度倍率
     */
    FollowParentGoal(AnimalEntity* animal, f64 speed);

    [[nodiscard]] bool shouldExecute() override;
    [[nodiscard]] bool shouldContinueExecuting() override;
    void startExecuting() override;
    void resetTask() override;
    void tick() override;

    [[nodiscard]] std::string getTypeName() const override { return "FollowParentGoal"; }

protected:
    /**
     * @brief 寻找附近的成年动物
     * @return 成年动物，如果没有则返回 nullptr
     */
    [[nodiscard]] AnimalEntity* findParent();

    AnimalEntity* m_childAnimal;
    f64 m_speed;
    AnimalEntity* m_parentAnimal = nullptr;
    i32 m_delayCounter = 0;
};

} // namespace entity::ai::goal
} // namespace mc
