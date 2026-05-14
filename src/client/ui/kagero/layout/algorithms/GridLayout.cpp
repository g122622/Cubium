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

#include "GridLayout.hpp"

namespace mc::client::ui::kagero::layout {

void GridLayout::setColumns(i32 columns)
{
    m_config.columns = std::max(1, columns);
}

i32 GridLayout::columns() const
{
    return m_config.columns;
}

void GridLayout::setRows(i32 rows)
{
    m_config.rows = std::max(0, rows);
}

i32 GridLayout::rows() const
{
    return m_config.rows;
}

void GridLayout::setColumnGap(i32 gap)
{
    m_config.columnGap = std::max(0, gap);
}

i32 GridLayout::columnGap() const
{
    return m_config.columnGap;
}

void GridLayout::setRowGap(i32 gap)
{
    m_config.rowGap = std::max(0, gap);
}

i32 GridLayout::rowGap() const
{
    return m_config.rowGap;
}

void GridLayout::setConfig(const GridConfig& config)
{
    m_config = config;
    m_config.columns = std::max(1, m_config.columns);
    m_config.rows = std::max(0, m_config.rows);
    m_config.columnGap = std::max(0, m_config.columnGap);
    m_config.rowGap = std::max(0, m_config.rowGap);
}

const GridConfig& GridLayout::config() const
{
    return m_config;
}

std::vector<LayoutResult> GridLayout::compute(
    const Rect& containerBounds, const std::vector<WidgetLayoutAdaptor*>& children)
{
    std::vector<LayoutResult> results;
    results.resize(children.size());
    if (children.empty()) {
        return results;
    }

    const i32 colCount = std::max(1, m_config.columns);
    const i32 rowCount = std::max(1, resolveRows(static_cast<i32>(children.size())));

    const i32 totalGapX = (colCount - 1) * m_config.columnGap;
    const i32 totalGapY = (rowCount - 1) * m_config.rowGap;
    const i32 cellWidth = std::max(1, (containerBounds.width - totalGapX) / colCount);
    const i32 cellHeight = std::max(1, (containerBounds.height - totalGapY) / rowCount);

    i32 autoIndex = 0;
    for (size_t i = 0; i < children.size(); ++i) {
        auto* child = children[i];
        if (child == nullptr || !child->constraints().enabled) {
            results[i] = LayoutResult(containerBounds.x, containerBounds.y, 0, 0);
            continue;
        }

        const GridItem& grid = child->gridItem();
        i32 col = grid.column;
        i32 row = grid.row;

        // 自动放置：如果 column 或 row 为负数，自动计算位置
        if (m_config.autoPlacement && (col < 0 || row < 0)) {
            col = autoIndex % colCount;
            row = autoIndex / colCount;
            ++autoIndex;
        } else {
            // 手动指定位置时，确保 col 和 row 非负
            col = std::max(0, col);
            row = std::max(0, row);
        }

        // 限制在网格范围内
        col = std::min(col, colCount - 1);

        const i32 spanCols = std::max(1, std::min(grid.columnSpan, colCount - col));
        const i32 spanRows = std::max(1, grid.rowSpan);

        const i32 x = containerBounds.x + col * (cellWidth + m_config.columnGap);
        const i32 y = containerBounds.y + row * (cellHeight + m_config.rowGap);

        const i32 width = spanCols * cellWidth + (spanCols - 1) * m_config.columnGap;
        const i32 height = spanRows * cellHeight + (spanRows - 1) * m_config.rowGap;

        results[i] = LayoutResult(x, y, width, height);
    }

    return results;
}

i32 GridLayout::resolveRows(i32 childCount) const
{
    if (m_config.rows > 0) {
        return m_config.rows;
    }
    return (childCount + m_config.columns - 1) / m_config.columns;
}

} // namespace mc::client::ui::kagero::layout
