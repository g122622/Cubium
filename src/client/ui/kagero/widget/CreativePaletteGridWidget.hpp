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

#include "client/ui/Glyph.hpp"
#include "client/ui/kagero/Types.hpp"
#include "client/ui/kagero/paint/PaintContext.hpp"
#include "client/ui/kagero/widget/Widget.hpp"
#include "common/entity/inventory/CreativeInventory.hpp"
#include "common/item/core/ItemStack.hpp"
#include "core/Types.hpp"
#include <algorithm>
#include <cstddef>
#include <functional>
#include <string>
#include <utility>
#include <vector>
#include <GLFW/glfw3.h>

namespace mc::client::ui::kagero::widget {

/**
 * @brief 创造模式物品调色板网格（带可见行剔除）
 *
 * 承载创造物品池（全部 CreativeInventoryEntry），按 9 列网格渲染，仅绘制
 * 当前可见的若干行（行剔除），并自带 clip 裁剪防止边缘行溢出。滚轮按整行
 * 步进（与原版创造屏一致），鼠标命中复刻 CreativeScreen::_getPaletteIndexAt
 * 的坐标→索引计算（含槽位间距 2px 排除）。
 *
 * 设计取舍：不把每个格子做成子 SlotWidget（创造池可达上千项，会构造上千
 * 组件）。改为直接持有 entries 引用，paint 时按可见行遍历、通过宿主注入的
 * ItemPaintCallback 画图标。点击事件经 indexAt 转成 palette 索引后回调上抛，
 * 由宿主 CreativeScreen 决定如何取物（对齐原版容器协议）。
 *
 * 数据源生命周期由宿主持有（setData 仅存裸指针/引用），宿主在重建物品池或
 * 切换搜索过滤后调用 refresh() 通知本组件重算行数并夹紧滚动。
 */
class CreativePaletteGridWidget : public Widget {
public:
    /// 物品图标绘制回调（与 SlotWidget::ItemPaintCallback 同签名语义）
    using ItemPaintCallback = std::function<void(const mc::ItemStack& item, i32 x, i32 y, i32 size)>;
    /// 点击命中回调：(paletteEntryIndex, visibleIndex, button, shiftHeld)
    using PaletteClickCallback =
        std::function<void(i32 paletteEntryIndex, i32 visibleIndex, i32 button, bool shiftHeld)>;

    static constexpr i32 COLUMNS = 9;
    static constexpr i32 SLOT_SIZE = 16;
    static constexpr i32 SLOT_SPACING = 18;

    CreativePaletteGridWidget() = default;

    /**
     * @brief 构造函数
     * @param id 组件 ID
     * @param x 相对父坐标 X
     * @param y 相对父坐标 Y
     * @param visibleRows 可见行数（决定组件高度 = visibleRows * SLOT_SPACING）
     */
    CreativePaletteGridWidget(std::string id, i32 x, i32 y, i32 visibleRows)
        : Widget(std::move(id))
        , m_visibleRows(visibleRows > 0 ? visibleRows : 1)
    {
        setBounds(Rect(x, y, COLUMNS * SLOT_SPACING, m_visibleRows * SLOT_SPACING));
    }

    // ==================== 数据与外观 ====================

    /**
     * @brief 设置可见条目列表（已按搜索过滤后的 entries 子集）
     *
     * @param entries 宿主持有的完整物品池（构造时全量；搜索过滤由宿主在
     *                visibleIndices 中体现）
     * @param visibleIndices 当前可见的 entries 下标（已过滤、已排序）
     *
     * 宿主在搜索文本变化或物品池重建后调用本方法 + refresh()。
     */
    void setData(const std::vector<mc::CreativeInventoryEntry>* entries, const std::vector<i32>* visibleIndices)
    {
        m_entries = entries;
        m_visibleIndices = visibleIndices;
    }

    void setItemPaintCallback(ItemPaintCallback callback) { m_itemPaintCallback = std::move(callback); }

    void setPaletteClickCallback(PaletteClickCallback callback) { m_paletteClickCallback = std::move(callback); }

    void setVisibleRows(i32 rows)
    {
        m_visibleRows = rows > 0 ? rows : 1;
        setBounds(Rect(bounds().x, bounds().y, COLUMNS * SLOT_SPACING, m_visibleRows * SLOT_SPACING));
        clampScrollRows();
    }

    [[nodiscard]] i32 scrollRows() const { return m_scrollRows; }
    [[nodiscard]] i32 maxScrollRows() const { return _computeMaxScrollRows(); }

    /** @brief 滚动到顶部（搜索文本变化时调用） */
    void scrollToTop() { m_scrollRows = 0; }

    /**
     * @brief 重建后重算行数并夹紧滚动（搜索过滤/物品池变化后调用）
     */
    void refresh() { clampScrollRows(); }

    // ==================== 生命周期 ====================

    void paint(PaintContext& ctx) override
    {
        if (!isVisible()) {
            return;
        }

        const Rect viewport = bounds();

        // 背景
        ctx.drawFilledRect(viewport, Colors::fromARGB(255, 21, 25, 30));

        // 裁剪：仅可见区域，防止边缘行图标溢出
        ctx.pushClip(viewport);

        const i32 visibleCount = visibleEntryCount();
        for (i32 row = 0; row < m_visibleRows; ++row) {
            for (i32 col = 0; col < COLUMNS; ++col) {
                const i32 visibleIndex = (m_scrollRows + row) * COLUMNS + col;
                const i32 cellX = viewport.x + col * SLOT_SPACING;
                const i32 cellY = viewport.y + row * SLOT_SPACING;

                if (visibleIndex < visibleCount) {
                    // 有物品的格子
                    ctx.drawFilledRect(Rect(cellX, cellY, SLOT_SIZE, SLOT_SIZE), Colors::fromARGB(255, 60, 70, 84));
                    ctx.drawBorder(Rect(cellX, cellY, SLOT_SIZE, SLOT_SIZE), 1.0f, Colors::fromARGB(255, 21, 26, 32));

                    const mc::ItemStack* stack = entryStackAt(visibleIndex);
                    if (stack != nullptr && !stack->isEmpty() && m_itemPaintCallback) {
                        m_itemPaintCallback(*stack, cellX, cellY, SLOT_SIZE);
                    }
                } else {
                    // 空格子
                    ctx.drawFilledRect(Rect(cellX, cellY, SLOT_SIZE, SLOT_SIZE), Colors::fromARGB(255, 42, 47, 55));
                    ctx.drawBorder(Rect(cellX, cellY, SLOT_SIZE, SLOT_SIZE), 1.0f, Colors::fromARGB(255, 16, 20, 26));
                }
            }
        }

        ctx.popClip();

        // 悬停高亮（命中有效格子时）
        if (isHovered() && m_lastHoverVisibleIndex >= 0 && m_lastHoverVisibleIndex < visibleCount) {
            const i32 localRow = (m_lastHoverVisibleIndex / COLUMNS) - m_scrollRows;
            const i32 localCol = m_lastHoverVisibleIndex % COLUMNS;
            if (localRow >= 0 && localRow < m_visibleRows) {
                const i32 hx = viewport.x + localCol * SLOT_SPACING;
                const i32 hy = viewport.y + localRow * SLOT_SPACING;
                ctx.drawBorder(Rect(hx, hy, SLOT_SIZE, SLOT_SIZE), 1.0f, Colors::fromARGB(255, 77, 163, 255));
            }
        }
    }

    bool onScroll(i32 mouseX, i32 mouseY, f64 delta) override
    {
        (void)mouseX;
        (void)mouseY;
        const i32 maxRows = _computeMaxScrollRows();
        if (maxRows <= 0) {
            return true;
        }
        const i32 step = delta > 0.0 ? -1 : 1;
        m_scrollRows = std::clamp(m_scrollRows + step, 0, maxRows);
        return true;
    }

    void updateHover(i32 mouseX, i32 mouseY) override
    {
        Widget::updateHover(mouseX, mouseY);
        m_lastHoverVisibleIndex = indexAt(mouseX, mouseY);
    }

    bool onClick(i32 mouseX, i32 mouseY, i32 button, i32 mods) override
    {
        if (!isActive() || !isVisible()) {
            return false;
        }
        const i32 visibleIndex = indexAt(mouseX, mouseY);
        if (visibleIndex < 0) {
            return false;
        }
        if (m_paletteClickCallback) {
            const i32 paletteEntryIndex = entryIndexOf(visibleIndex);
            const bool shiftHeld = (mods & GLFW_MOD_SHIFT) != 0;
            m_paletteClickCallback(paletteEntryIndex, visibleIndex, button, shiftHeld);
        }
        return true;
    }

    /**
     * @brief 屏幕坐标 → 可见条目索引（未命中或落在间距返回 -1）
     *
     * 复刻 CreativeScreen::_getPaletteIndexAt 的坐标计算，含 2px 间距排除。
     */
    [[nodiscard]] i32 indexAt(i32 mouseX, i32 mouseY) const
    {
        const Rect viewport = bounds();
        const i32 localX = mouseX - viewport.x;
        const i32 localY = mouseY - viewport.y;
        if (localX < 0 || localY < 0) {
            return -1;
        }
        const i32 col = localX / SLOT_SPACING;
        const i32 row = localY / SLOT_SPACING;
        if (col < 0 || col >= COLUMNS || row < 0 || row >= m_visibleRows) {
            return -1;
        }
        // 落在槽位间距区域则不算命中
        if (localX % SLOT_SPACING >= SLOT_SIZE || localY % SLOT_SPACING >= SLOT_SIZE) {
            return -1;
        }
        const i32 visibleIndex = (m_scrollRows + row) * COLUMNS + col;
        if (visibleIndex >= visibleEntryCount()) {
            return -1;
        }
        return visibleIndex;
    }

private:
    [[nodiscard]] i32 visibleEntryCount() const
    {
        return (m_visibleIndices != nullptr) ? static_cast<i32>(m_visibleIndices->size()) : 0;
    }

    [[nodiscard]] i32 entryIndexOf(i32 visibleIndex) const
    {
        if (m_visibleIndices == nullptr || visibleIndex < 0 ||
            visibleIndex >= static_cast<i32>(m_visibleIndices->size())) {
            return -1;
        }
        return (*m_visibleIndices)[static_cast<std::size_t>(visibleIndex)];
    }

    [[nodiscard]] const mc::ItemStack* entryStackAt(i32 visibleIndex) const
    {
        if (m_entries == nullptr) {
            return nullptr;
        }
        const i32 entryIndex = entryIndexOf(visibleIndex);
        if (entryIndex < 0 || entryIndex >= static_cast<i32>(m_entries->size())) {
            return nullptr;
        }
        return &(*m_entries)[static_cast<std::size_t>(entryIndex)].stack;
    }

    [[nodiscard]] i32 _computeMaxScrollRows() const
    {
        const i32 count = visibleEntryCount();
        const i32 totalRows = (count + COLUMNS - 1) / COLUMNS;
        return std::max(0, totalRows - m_visibleRows);
    }

    void clampScrollRows() { m_scrollRows = std::clamp(m_scrollRows, 0, _computeMaxScrollRows()); }

    const std::vector<mc::CreativeInventoryEntry>* m_entries = nullptr;
    const std::vector<i32>* m_visibleIndices = nullptr;

    ItemPaintCallback m_itemPaintCallback;
    PaletteClickCallback m_paletteClickCallback;

    i32 m_visibleRows = 5;
    i32 m_scrollRows = 0;
    i32 m_lastHoverVisibleIndex = -1;
};

} // namespace mc::client::ui::kagero::widget
