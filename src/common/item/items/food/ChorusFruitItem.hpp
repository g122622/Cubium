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

#include "FoodItem.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/food/Food.hpp"

namespace mc {
namespace item::items {

/**
 * @brief 紫颂果物品
 *
 * 紫颂果是一种特殊食物，食用后随机传送到附近位置。
 *
 * 特性：
 * 1. 食用后随机传送（最大16次尝试）
 * 2. 传送范围：以玩家为中心，水平方向±8格，垂直方向±8格
 * 3. 播放传送音效
 * 4. 冷却时间20 ticks（1秒）
 */
class ChorusFruitItem : public FoodItem {
public:
    /**
     * @brief 构造紫颂果
     * @param food 食物属性
     * @param properties 物品属性
     */
    ChorusFruitItem(const food::Food* food, ItemProperties properties);

    /**
     * @brief 使用完成
     *
     * 覆盖父类方法以实现：
     * 1. 随机传送
     * 2. 播放传送音效
     * 3. 设置冷却时间
     *
     * @param stack 物品堆
     * @param world 世界引用
     * @param entity 使用的实体
     * @return 使用后的物品堆
     */
    ItemStack onItemUseFinish(ItemStack& stack, IWorld& world, Entity& entity) override;
};

} // namespace item::items
} // namespace mc
