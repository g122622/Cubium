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

namespace mc {
namespace entity {
class VillagerEntity;

namespace ai {
namespace goal {
namespace villager {

/**
 * @brief 村民聚集目标
 *
 * 村民在聚集活动期间与其他村民互动。
 * 包括流言传播和物品分享。
 */
class CongregateGoal : public Goal {
public:
    /**
     * @brief 构造函数
     * @param villager 村民实体
     */
    explicit CongregateGoal(VillagerEntity* villager);

    /**
     * @brief 检查是否应该开始执行
     */
    [[nodiscard]] bool shouldExecute() override;

    /**
     * @brief 检查是否应该继续执行
     */
    [[nodiscard]] bool shouldContinueExecuting() override;

    /**
     * @brief 开始执行
     */
    void startExecuting() override;

    /**
     * @brief 重置任务
     */
    void resetTask() override;

    /**
     * @brief 每tick更新
     */
    void tick() override;

    [[nodiscard]] std::string getTypeName() const override { return "CongregateGoal"; }

private:
    /**
     * @brief 查找交互目标
     */
    void _findInteractionTarget();

    /**
     * @brief 传播流言
     */
    void _spreadGossip();

    /**
     * @brief 分享物品（农民分享食物）
     */
    void _shareItems();

private:
    VillagerEntity* m_villager;
    EntityInstanceId m_targetVillagerId;
    i32 m_interactCooldown = 0;
    static constexpr i32 INTERACTION_DURATION = 100;  // 交互持续时间
    static constexpr f32 INTERACTION_DISTANCE = 5.0f; // 交互距离
};

} // namespace villager
} // namespace goal
} // namespace ai
} // namespace entity
} // namespace mc
