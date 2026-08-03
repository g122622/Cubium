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
namespace entity {
class VillagerEntity;

namespace ai {
namespace goal {
namespace villager {

/**
 * @brief 村民逃避敌对目标
 *
 * 村民逃离僵尸、掠夺者等敌对生物。
 */
class AvoidHostileGoal : public Goal {
public:
    explicit AvoidHostileGoal(VillagerEntity* villager);
    [[nodiscard]] bool shouldExecute() override;
    [[nodiscard]] bool shouldContinueExecuting() override;
    void startExecuting() override;
    void resetTask() override;
    void tick() override;
    [[nodiscard]] std::string getTypeName() const override { return "AvoidHostileGoal"; }

private:
    /**
     * @brief 查找最近的敌对生物
     */
    void _findNearestHostile();

    /**
     * @brief 计算逃跑方向
     */
    void _fleeFromHostile();

private:
    VillagerEntity* m_villager;
    EntityInstanceId m_hostileEntity;
    BlockPos m_fleeTarget;
    static constexpr f32 FLEE_RANGE = 8.0f;     // 敌对生物触发距离
    static constexpr f32 FLEE_DISTANCE = 16.0f; // 逃跑距离
    static constexpr f32 FLEE_SPEED = 0.6f;     // 逃跑速度倍率
};

} // namespace villager
} // namespace goal
} // namespace ai
} // namespace entity
} // namespace mc
