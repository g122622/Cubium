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

#include "client/ui/kagero/Types.hpp"
#include "client/ui/kagero/layout/core/LayoutResult.hpp"
#include "client/ui/kagero/layout/integration/WidgetLayoutAdaptor.hpp"
#include "common/core/Types.hpp"
#include <vector>

namespace mc::client::ui::kagero::layout {

/**
 * @brief 网格布局配置
 */
struct GridConfig {
    i32 columns = 1;           ///< 列数，最小为1
    i32 rows = 0;              ///< 行数，0表示自动根据子项数量推算
    i32 columnGap = 0;         ///< 列间距（像素）
    i32 rowGap = 0;            ///< 行间距（像素）
    bool autoPlacement = true; ///< 是否自动放置未指定位置的子项
};

/**
 * @brief Grid布局算法
 *
 * 将子项按照行列网格排列，支持自动放置和手动指定位置、
 * 跨行跨列、行列间距等特性。
 */
class GridLayout {
public:
    void setColumns(i32 columns);
    [[nodiscard]] i32 columns() const;

    void setRows(i32 rows);
    [[nodiscard]] i32 rows() const;

    void setColumnGap(i32 gap);
    [[nodiscard]] i32 columnGap() const;

    void setRowGap(i32 gap);
    [[nodiscard]] i32 rowGap() const;

    void setConfig(const GridConfig& config);
    [[nodiscard]] const GridConfig& config() const;

    /**
     * @brief 计算所有子项在网格中的布局结果
     *
     * @param containerBounds 容器边界矩形
     * @param children 子项适配器列表
     * @return 每个子项对应的布局结果，与children一一对应
     */
    [[nodiscard]] std::vector<LayoutResult> compute(
        const Rect& containerBounds, const std::vector<WidgetLayoutAdaptor*>& children);

private:
    /** 根据子项数量和列数推算行数 */
    [[nodiscard]] i32 _resolveRows(i32 childCount) const;

    GridConfig m_config;
};

} // namespace mc::client::ui::kagero::layout
