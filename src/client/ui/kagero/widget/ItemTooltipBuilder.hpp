/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to furnished to do so, subject to the following
 * conditions:
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

#include "client/ui/kagero/widget/Tooltip.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/world/IWorld.hpp"
#include <string>

namespace mc::client::ui::kagero::widget {

/**
 * @brief 物品 Tooltip 行构建器
 *
 * 把一个 ItemStack 转成多行 Tooltip 文本，对应 MC 1.21.11
 * ItemStack#getTooltipLines（displayName → Count → Durability →
 * Item#appendHoverText）。
 *
 * 仅构建文本行（Tooltip 数据），渲染交给 kagero 的 TooltipRenderer /
 * Widget::refreshTooltip。收纳袋等需要图像 tooltip 的物品暂未在此处处理。
 */
class ItemTooltipBuilder {
public:
    /**
     * @brief 构建物品 Tooltip
     * @param stack 物品堆（空堆返回空 Tooltip）
     * @param world 世界指针（可为 null，对应 MC 的 EMPTY TooltipContext）
     * @return 多行 Tooltip
     */
    [[nodiscard]] static Tooltip build(const mc::ItemStack& stack, mc::IWorld* world);
};

} // namespace mc::client::ui::kagero::widget
