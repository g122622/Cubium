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

#include "../core/LayoutResult.hpp"
#include "../integration/WidgetLayoutAdaptor.hpp"
#include "client/ui/kagero/Types.hpp"
#include <vector>

namespace mc::client::ui::kagero::layout {

/**
 * @brief Anchor布局算法
 *
 * 基于锚点约束的布局算法，支持子元素相对于容器边缘的定位。
 * 支持的定位方式包括：边缘锚定、拉伸填充、居中对齐、百分比定位。
 */
class AnchorLayout {
public:
    /**
     * @brief 计算所有子元素的布局结果
     *
     * @param containerBounds 容器边界
     * @param children 子元素适配器列表
     * @return 每个子元素的布局结果
     */
    [[nodiscard]] std::vector<LayoutResult> compute(
        const Rect& containerBounds, const std::vector<WidgetLayoutAdaptor*>& children) const;

private:
    /**
     * @brief 计算单个子元素的边界矩形
     *
     * @param containerBounds 容器边界
     * @param child 子元素适配器
     * @return 子元素的边界矩形
     */
    [[nodiscard]] Rect _computeChildBounds(const Rect& containerBounds, const WidgetLayoutAdaptor& child) const;
};

} // namespace mc::client::ui::kagero::layout
