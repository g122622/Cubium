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
#include <string>

namespace mc {

// 前向声明
class MobEntity;

namespace entity::ai::goal {

/**
 * @brief 游泳目标
 *
 * 当实体在水中或岩浆中时，尝试向上游动。
 */
class SwimGoal : public Goal {
public:
    /**
     * @brief 构造函数
     * @param mob 拥有此目标的生物
     */
    explicit SwimGoal(MobEntity* mob);

    [[nodiscard]] bool shouldExecute() override;
    void tick() override;

    // 对齐 vanilla FloatGoal.requiresUpdateEveryTick()=true：
    // 该 goal 每 tick 评估，tick 内 0.8 跳跃概率用裸值（不经 adjustedTickDelay）。
    [[nodiscard]] bool requiresUpdateEveryTick() const override { return true; }

    [[nodiscard]] std::string getTypeName() const override { return "SwimGoal"; }

private:
    MobEntity* m_mob;
};

} // namespace entity::ai::goal
} // namespace mc
