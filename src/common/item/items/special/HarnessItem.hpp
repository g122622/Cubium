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

#include "common/item/core/Item.hpp"
#include "common/util/color/DyeColor.hpp"

namespace mc {
namespace item::items {

/**
 * @brief 欢乐诡鬼装备物品（16色变体）
 *
 * 用于装备欢乐诡鬼（HappyGhast）的装饰性装备，共 16 种颜色变体。
 * 每种颜色对应一个独立的 Item 实例，颜色为物品固有属性（非 NBT 染色）。
 *
 * 特性：
 * - 无护甲值、无耐久度、不可附魔
 * - 可通过合成（皮革+玻璃+对应颜色羊毛）或染色配方获得
 * - 装备交互（itemInteractionForEntity）与剪刀剪下逻辑由 HappyGhastEntity 实现后集成
 *
 * 参考: net.minecraft.world.item.equipment.Equippable#harness(DyeColor)
 */
class HarnessItem : public Item {
public:
    /**
     * @brief 构造函数
     * @param properties 物品属性（maxStackSize=1）
     * @param color 染料颜色（决定变体）
     */
    HarnessItem(ItemProperties properties, DyeColor color);

    /**
     * @brief 获取装备颜色
     * @return 染料颜色
     */
    [[nodiscard]] DyeColor getColor() const { return m_color; }

private:
    DyeColor m_color;
};

} // namespace item::items
} // namespace mc
