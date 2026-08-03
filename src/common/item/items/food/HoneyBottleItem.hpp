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
#include "common/entity/effect/EffectType.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/core/UseAction.hpp"
#include "common/item/food/Food.hpp"
#include "common/item/items/food/FoodItem.hpp"

namespace mc {
namespace item::items {

/**
 * @brief 蜂蜜瓶物品
 *
 * 蜂蜜瓶是一种特殊的食物，具有以下特性：
 * 1. 食用后清除中毒效果
 * 2. 返回玻璃瓶
 * 3. 使用时间40 ticks（比普通食物慢）
 */
class HoneyBottleItem : public FoodItem {
public:
    /**
     * @brief 构造蜂蜜瓶
     * @param food 食物属性
     * @param properties 物品属性
     */
    HoneyBottleItem(const food::Food* food, ItemProperties properties);

    /**
     * @brief 使用完成
     *
     * 覆盖父类方法以实现：
     * 1. 清除中毒效果
     * 2. 返回玻璃瓶
     *
     * @param stack 物品堆
     * @param world 世界引用
     * @param entity 使用的实体
     * @return 使用后的物品堆
     */
    ItemStack onItemUseFinish(ItemStack& stack, IWorld& world, Entity& entity) override;

    /**
     * @brief 获取使用时长
     *
     * 蜂蜜瓶使用时间为40 ticks（2秒）。
     *
     * @param stack 物品堆
     * @return 使用时长（ticks）
     */
    [[nodiscard]] i32 getUseDuration(const ItemStack& stack) const override;

    /**
     * @brief 获取使用动作
     *
     * 蜂蜜瓶返回 Drink 动作。
     *
     * @param stack 物品堆
     * @return 使用动作
     */
    [[nodiscard]] UseAction getUseAction(const ItemStack& stack) const override;
};

} // namespace item::items
} // namespace mc
