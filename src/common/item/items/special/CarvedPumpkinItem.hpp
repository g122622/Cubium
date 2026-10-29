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
#include "common/item/attribute/ItemAttributeModifiers.hpp"
#include "common/item/items/block/BlockItem.hpp"

namespace mc {
namespace item::items {

/**
 * @brief 雕刻南瓜物品
 *
 * 与普通方块物品唯一的区别：戴在头上时隐藏导航点。原版由 `Waypoint.addHideAttribute`
 * 在 `Items.java` 里给雕刻南瓜与各类头颅统一加上
 * `waypoint_transmit_range × -100%` 的隐藏修饰符，其他玩家的定位条因此看不到穿戴者。
 *
 * 头颅走的是 `BlockItemRegistry` 的通用方块物品路径（没有单独的类），那条路径上同样
 * 按物品名判定；这里之所以单独成类，是因为雕刻南瓜在 `Items.cpp` 里以「方块物品」
 * 之外的名字注册成了普通 `BlockItem`，需要在注册处指定类型。
 */
class CarvedPumpkinItem : public BlockItem {
public:
    /**
     * @brief 构造雕刻南瓜物品
     * @param block 关联的方块（雕刻南瓜）
     * @param properties 物品属性
     */
    CarvedPumpkinItem(const Block& block, ItemProperties properties);

    ~CarvedPumpkinItem() override = default;

    /**
     * @brief 戴在头上时隐藏导航点
     *
     * 对齐 vanilla `Waypoint.WAYPOINT_TRANSMIT_RANGE_HIDE_MODIFIER`：只作用于头部槽位，
     * 修饰符 id 为 `minecraft:waypoint_transmit_range_hide`，运算为乘法总计 -1。
     */
    [[nodiscard]] item::ItemAttributeModifiers getAttributeModifiers(i32 equipmentSlot) const override;
};

} // namespace item::items
} // namespace mc