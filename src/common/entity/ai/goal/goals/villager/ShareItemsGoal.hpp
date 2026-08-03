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
 * @brief 村民分享物品目标
 *
 * 农民分享食物给其他村民。
 */
class ShareItemsGoal : public Goal {
public:
    /**
     * @brief 构造函数
     * @param villager 村民实体
     */
    explicit ShareItemsGoal(VillagerEntity* villager);

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

    [[nodiscard]] std::string getTypeName() const override { return "ShareItemsGoal"; }

private:
    /**
     * @brief 检查是否有多余的食物可以分享
     */
    [[nodiscard]] bool _canAbandonItems() const;

    /**
     * @brief 检查目标是否需要食物
     * @param target 目标村民
     * @return 目标村民的食物点数是否低于需求阈值
     */
    [[nodiscard]] bool _targetNeedsFoodForTarget(VillagerEntity* target) const;

    /**
     * @brief 分享食物给目标
     */
    void _shareFoodWithTarget();

private:
    VillagerEntity* m_villager;
    EntityInstanceId m_targetVillagerId;
    i32 m_shareCooldown = 0;
    static constexpr f32 SHARE_DISTANCE = 2.0f; // 分享距离
    static constexpr i32 SHARE_COOLDOWN = 200;  // 分享冷却
};

} // namespace villager
} // namespace goal
} // namespace ai
} // namespace entity
} // namespace mc
