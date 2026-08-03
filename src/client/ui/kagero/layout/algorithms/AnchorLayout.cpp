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

#include "AnchorLayout.hpp"
#include "client/ui/kagero/Types.hpp"
#include "client/ui/kagero/layout/core/LayoutResult.hpp"
#include "client/ui/kagero/layout/integration/WidgetLayoutAdaptor.hpp"
#include "common/core/Types.hpp"
#include <algorithm>
#include <vector>

namespace mc::client::ui::kagero::layout {

std::vector<LayoutResult> AnchorLayout::compute(
    const Rect& containerBounds, const std::vector<WidgetLayoutAdaptor*>& children) const
{
    std::vector<LayoutResult> results;
    results.reserve(children.size());

    for (const auto* child : children) {
        // 跳过未启用的子元素，返回零尺寸布局结果
        if (!child->constraints().enabled) {
            results.emplace_back(containerBounds.x, containerBounds.y, 0, 0);
            continue;
        }

        results.emplace_back(_computeChildBounds(containerBounds, *child));
    }

    return results;
}

Rect AnchorLayout::_computeChildBounds(const Rect& containerBounds, const WidgetLayoutAdaptor& child) const
{
    const auto& c = child.constraints();
    const auto& anchor = c.anchor;

    // 确定子元素尺寸：优先使用期望尺寸，否则使用当前尺寸
    i32 width = c.preferredWidth > 0 ? c.preferredWidth : std::max(1, child.currentSize().width);
    i32 height = c.preferredHeight > 0 ? c.preferredHeight : std::max(1, child.currentSize().height);

    i32 x = containerBounds.x;
    i32 y = containerBounds.y;

    // 水平方向定位
    if (anchor.isStretchHorizontal()) {
        // 拉伸模式：左右锚定，宽度填充剩余空间
        x = containerBounds.x + anchor.left.value_or(0);
        width = std::max(1, containerBounds.width - anchor.left.value_or(0) - anchor.right.value_or(0));
    } else if (anchor.centerX) {
        // 水平居中
        x = containerBounds.x + (containerBounds.width - width) / 2;
    } else if (anchor.left.has_value()) {
        // 锚定左边缘
        x = containerBounds.x + anchor.left.value();
    } else if (anchor.right.has_value()) {
        // 锚定右边缘
        x = containerBounds.right() - anchor.right.value() - width;
    }

    // 垂直方向定位
    if (anchor.isStretchVertical()) {
        // 拉伸模式：上下锚定，高度填充剩余空间
        y = containerBounds.y + anchor.top.value_or(0);
        height = std::max(1, containerBounds.height - anchor.top.value_or(0) - anchor.bottom.value_or(0));
    } else if (anchor.centerY) {
        // 垂直居中
        y = containerBounds.y + (containerBounds.height - height) / 2;
    } else if (anchor.top.has_value()) {
        // 锚定上边缘
        y = containerBounds.y + anchor.top.value();
    } else if (anchor.bottom.has_value()) {
        // 锚定下边缘
        y = containerBounds.bottom() - anchor.bottom.value() - height;
    }

    // 百分比定位（覆盖绝对定位结果）
    if (anchor.leftPercent.has_value()) {
        x = containerBounds.x + static_cast<i32>(containerBounds.width * anchor.leftPercent.value());
    }
    if (anchor.topPercent.has_value()) {
        y = containerBounds.y + static_cast<i32>(containerBounds.height * anchor.topPercent.value());
    }

    // 应用偏移量
    x += anchor.offsetX;
    y += anchor.offsetY;

    return Rect{x, y, width, height};
}

} // namespace mc::client::ui::kagero::layout
