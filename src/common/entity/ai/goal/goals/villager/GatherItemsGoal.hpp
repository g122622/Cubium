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
 * @brief 村民收集物品目标
 *
 * 村民收集地上的食物或种子等物品。
 */
class GatherItemsGoal : public Goal {
public:
    explicit GatherItemsGoal(VillagerEntity* villager);

    [[nodiscard]] bool shouldExecute() override;
    [[nodiscard]] bool shouldContinueExecuting() override;
    void startExecuting() override;
    void resetTask() override;
    void tick() override;

    [[nodiscard]] std::string getTypeName() const override { return "GatherItemsGoal"; }

private:
    /**
     * @brief 查找附近的物品
     */
    void _findNearbyItems();

    /**
     * @brief 移动到物品
     */
    void _moveToItem();

    /**
     * @brief 拾取物品
     */
    void _pickupItem();

private:
    VillagerEntity* m_villager;
    EntityInstanceId m_targetItem;
    static constexpr f32 PICKUP_RANGE = 32.0f;   // 搜索范围
    static constexpr f32 PICKUP_DISTANCE = 1.5f; // 拾取距离
};

} // namespace villager
} // namespace goal
} // namespace ai
} // namespace entity
} // namespace mc
