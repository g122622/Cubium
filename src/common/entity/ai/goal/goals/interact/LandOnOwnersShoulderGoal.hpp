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
#include <string>

namespace mc {

// 前向声明
class ShoulderRidingEntity;
class Player;

namespace entity::ai::goal {

/**
 * @brief 落到主人肩膀上的 AI 目标
 *
 * 使可驯服的肩膀乘坐实体（如鹦鹉）飞到主人的肩膀上。
 *
 * 触发条件：
 * 1. 实体已驯服
 * 2. 实体未坐下
 * 3. 主人存在且不在旁观者模式
 * 4. 主人不在飞行（创造模式飞行）
 * 5. 主人不在水中
 * 6. 肩膀乘坐冷却已过（> 100 ticks）
 *
 * 当实体与主人的碰撞箱相交时，尝试坐到主人肩膀上。
 * 一旦成功坐到肩膀上，此目标不可被抢占（isPreemptible 返回 false）。
 */
class LandOnOwnersShoulderGoal : public Goal {
public:
    /**
     * @brief 构造函数
     * @param entity 肩膀乘坐实体（如鹦鹉）
     */
    explicit LandOnOwnersShoulderGoal(ShoulderRidingEntity* entity);

    ~LandOnOwnersShoulderGoal() override = default;

    /**
     * @brief 检查是否应该执行
     *
     * 条件：
     * - 实体已驯服
     * - 实体未坐下
     * - 主人存在且是有效玩家
     * - 主人不在旁观者模式
     * - 主人不在飞行
     * - 主人不在水中
     * - 肩膀乘坐冷却已过
     */
    [[nodiscard]] bool shouldExecute() override;

    /**
     * @brief 检查是否应该继续执行
     */
    [[nodiscard]] bool shouldContinueExecuting() override;

    /**
     * @brief 是否可被抢占
     * @return 如果已经在肩膀上返回 false，否则返回 true
     */
    [[nodiscard]] bool isPreemptible() const noexcept override;

    /**
     * @brief 开始执行时调用
     */
    void startExecuting() override;

    /**
     * @brief 每tick执行
     *
     * 检查实体是否与主人碰撞箱相交，如果是则尝试坐到肩膀上。
     */
    void tick() override;

    /**
     * @brief 获取目标名称
     */
    [[nodiscard]] std::string getTypeName() const override { return "LandOnOwnersShoulderGoal"; }

private:
    ShoulderRidingEntity* m_entity;
    Player* m_owner = nullptr;
    bool m_isSittingOnShoulder = false;
};

} // namespace entity::ai::goal
} // namespace mc
