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
namespace entity {
class VillagerEntity;

namespace ai {
namespace goal {
namespace villager {

/**
 * @brief 村民看向实体目标
 *
 * 村民随机看向附近的实体（村民、玩家、猫等）。
 */
class LookAtEntitiesGoal : public Goal {
public:
    /**
     * @brief 构造函数
     * @param villager 村民实体
     */
    explicit LookAtEntitiesGoal(VillagerEntity* villager);

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

    [[nodiscard]] std::string getTypeName() const override { return "LookAtEntitiesGoal"; }

private:
    /**
     * @brief 选择目标类型
     *
     * 根据权重随机选择看向的目标类型：
     * 猫(8)、村民(2)、玩家(2)、生物(1)，总计13。
     */
    void _selectTargetType();

private:
    VillagerEntity* m_villager;
    EntityInstanceId m_lookTargetId;
    i32 m_lookTime = 0;

    enum class TargetType : u8 { Villager, Player, Cat, Creature };
    TargetType m_targetType = TargetType::Villager;

    static constexpr f32 LOOK_RANGE = 8.0f;   // 看向距离
    static constexpr f32 LOOK_CHANCE = 0.02f; // 触发概率
    static constexpr i32 LOOK_MIN_TIME = 40;  // 最小看向时间
    static constexpr i32 LOOK_MAX_TIME = 80;  // 最大看向时间
};

} // namespace villager
} // namespace goal
} // namespace ai
} // namespace entity
} // namespace mc
