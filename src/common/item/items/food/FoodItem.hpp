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

#include "../../core/Item.hpp"
#include "../../core/UseAction.hpp"
#include "../../food/Food.hpp"
#include "common/core/Types.hpp"
#include "common/item/core/ActionResult.hpp"

namespace mc {

class Player;
class IWorld;

namespace item::items {

/**
 * @brief 食物物品基类
 *
 * 负责处理可食用物品的基础行为。
 */
class FoodItem : public Item {
public:
    /**
     * @brief 构造食物物品
     * @param food 食物属性
     * @param properties 物品属性
     */
    FoodItem(const food::Food* food, ItemProperties properties);

    /**
     * @brief 是否为食物
     */
    [[nodiscard]] bool isFood() const override { return m_food != nullptr; }

    /**
     * @brief 获取食物属性
     */
    [[nodiscard]] const food::Food* getFood() const override { return m_food; }

    /**
     * @brief 获取使用时长
     *
     * 快速食用：16 ticks (0.8秒)
     * 普通食用：32 ticks (1.6秒)
     */
    [[nodiscard]] i32 getUseDuration(const ItemStack& stack) const override;

    /**
     * @brief 获取使用动作
     *
     * 所有食物都返回 Eat 动作，
     * isMeat() 标记仅用于狼是否能食用。
     */
    [[nodiscard]] UseAction getUseAction(const ItemStack& stack) const override;

    /**
     * @brief 右键使用物品
     */
    ItemActionResult onItemRightClick(IWorld& world, Player& player, Hand hand) override;

    /**
     * @brief 使用完成（食用完成）
     *
     * 处理逻辑：
     * 1. 恢复饥饿值和饱和度
     * 2. 应用药水效果（带概率）
     * 3. 播放进食音效
     * 4. 播放打嗝音效（玩家专用）
     * 5. 减少物品数量（创造模式不减）
     * 6. 返回容器物品
     */
    ItemStack onItemUseFinish(ItemStack& stack, IWorld& world, Entity& entity) override;

    /**
     * @brief 是否可以食用
     *
     * 检查条件：
     * - 创造模式可以吃任何食物
     * - 金苹果等特殊食物可以在饱食时食用
     * - 其他食物需要饥饿值 < 20
     */
    [[nodiscard]] bool canEat(const ItemStack& stack, const Player& player) const override;

protected:
    const food::Food* m_food;
};

} // namespace item::items
} // namespace mc
