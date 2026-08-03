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
 * THE SOFTWARE IS PROVIDED " IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#pragma once

#include "OnAStickItem.hpp"
#include "common/core/Types.hpp"
#include "common/item/core/Item.hpp"

namespace mc {
namespace item {

/**
 * @brief 胡萝卜钓竿
 *
 * 用于控制骑乘的猪的物品。
 *
 * 特性：
 * - 耐久度：25
 * - 每次加速消耗：7 耐久度
 * - 目标实体：猪 (minecraft:pig)
 * - 耐久度耗尽后转换为钓鱼竿
 *
 * 用法：
 * 1. 给猪装备鞍
 * 2. 右键骑上猪
 * 3. 手持胡萝卜钓竿，右键加速
 */
class CarrotOnAStickItem : public OnAStickItem {
public:
    /**
     * @brief 构造函数
     * @param properties 物品属性
     */
    explicit CarrotOnAStickItem(const ItemProperties& properties);

    ~CarrotOnAStickItem() noexcept override = default;

    // MC 常量
    static constexpr i32 MAX_DAMAGE = 25;     ///< 最大耐久度
    static constexpr i32 DURABILITY_COST = 7; ///< 每次加速消耗的耐久度
};

/**
 * @brief 诡异菌钓竿
 *
 * 用于控制骑乘的炽足兽的物品。
 *
 * 特性：
 * - 耐久度：100
 * - 每次加速消耗：1 耐久度
 * - 目标实体：炽足兽 (minecraft:strider)
 * - 耐久度耗尽后转换为钓鱼竿
 *
 * 用法：
 * 1. 给炽足兽装备鞍
 * 2. 右键骑上炽足兽
 * 3. 手持诡异菌钓竿，右键加速
 */
class WarpedFungusOnAStickItem : public OnAStickItem {
public:
    /**
     * @brief 构造函数
     * @param properties 物品属性
     */
    explicit WarpedFungusOnAStickItem(const ItemProperties& properties);

    ~WarpedFungusOnAStickItem() noexcept override = default;

    // MC 常量
    static constexpr i32 MAX_DAMAGE = 100;    ///< 最大耐久度
    static constexpr i32 DURABILITY_COST = 1; ///< 每次加速消耗的耐久度
};

} // namespace item
} // namespace mc
