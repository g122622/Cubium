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

#include "../paint/PaintContext.hpp"
#include "ScrollableWidget.hpp"
#include "Widget.hpp"
#include "client/ui/Glyph.hpp"
#include "client/ui/kagero/Types.hpp"
#include "common/core/Types.hpp"
#include <algorithm>
#include <cstddef>
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

// 前向声明 Value 类型
namespace mc::client::ui::kagero::tpl::binder {
class Value;
}

namespace mc::client::ui::kagero::widget {

/**
 * @brief 列表项接口
 *
 * 列表中的单个项目
 */
class IListItem {
public:
    virtual ~IListItem() = default;

    /**
     * @brief 获取项目高度
     */
    [[nodiscard]] virtual i32 getHeight() const = 0;

    /**
     * @brief 绘制项目
     * @param ctx 绘图上下文
     * @param x X坐标
     * @param y Y坐标
     * @param width 宽度
     * @param selected 是否选中
     * @param hovered 是否悬停
     */
    virtual void paintItem(PaintContext& ctx, i32 x, i32 y, i32 width, bool selected, bool hovered) = 0;

    /**
     * @brief 点击项目
     */
    virtual void onClick(i32 mouseX, i32 mouseY, i32 button)
    {
        (void)mouseX;
        (void)mouseY;
        (void)button;
    }

    /**
     * @brief 双击项目
     */
    virtual void onDoubleClick(i32 mouseX, i32 mouseY)
    {
        (void)mouseX;
        (void)mouseY;
    }
};

/**
 * @brief 列表组件
 *
 * 显示可滚动列表的组件，支持单选和多选。
 *
 * 使用示例：
 * @code
 * auto list = std::make_unique<ListWidget>("list", 10, 10, 200, 300);
 * list->setOnSelect([](size_t index, IListItem* item) {
 *     // 处理选择
 * });
 * list->addItem(std::make_unique<MyListItem>("Item 1"));
 * @endcode
 */
class ListWidget : public ScrollableWidget {
public:
    /**
     * @brief 选择模式
     */
    enum class SelectionMode : u8 {
        None,    ///< 不可选择
        Single,  ///< 单选
        Multiple ///< 多选
    };

    /**
     * @brief 选择回调类型
     */
    using OnSelectCallback = std::function<void(size_t, IListItem*)>;

    /**
     * @brief 双击回调类型
     */
    using OnDoubleClickCallback = std::function<void(size_t, IListItem*)>;

    /**
     * @brief 默认构造函数
     *
     * 定义在 .cpp 中：m_dataSource 为前向声明的 Value 的 unique_ptr，
     * 其默认构造（虽不分配）仍需 Value 完整类型以满足编译器的潜在销毁路径检查。
     */
    ListWidget();

    /**
     * @brief 析构函数
     *
     * 声明为虚函数并在 .cpp 中定义，以便安全释放 m_dataSource（其类型为
     * 前向声明的 tpl::binder::Value 的 std::unique_ptr）。
     */
    ~ListWidget() override;

    // 禁止拷贝（继承自 Widget 的删除语义）
    ListWidget(const ListWidget&) = delete;
    ListWidget& operator=(const ListWidget&) = delete;

    // 允许移动（unique_ptr 成员支持移动语义）
    // 定义在 .cpp 中，原因同上：需要 Value 完整类型
    ListWidget(ListWidget&&) noexcept;
    ListWidget& operator=(ListWidget&&) noexcept;

    /**
     * @brief 构造函数（仅ID）
     * @param id 组件ID
     *
     * 定义在 .cpp 中：由于 m_dataSource 类型为前向声明的 Value 的 unique_ptr，
     * 编译器需要在构造异常路径上能销毁 unique_ptr，因此要求 Value 完整可见。
     */
    explicit ListWidget(std::string id);

    /**
     * @brief 构造函数
     * @param id 组件ID
     * @param x X坐标
     * @param y Y坐标
     * @param width 宽度
     * @param height 高度
     */
    ListWidget(std::string id, i32 x, i32 y, i32 width, i32 height);

    // ==================== 生命周期 ====================

    void init() override
    {
        ScrollableWidget::init();
        updateContentHeight();
    }

    void tick(f32 dt) override { ScrollableWidget::tick(dt); }

    bool onMouseMove(i32 mouseX, i32 mouseY) override
    {
        // 调用父类方法以保持鼠标状态更新（当前 ScrollableWidget 未覆写，
        // 但保留调用以确保未来扩展时不遗漏）
        bool handled = ScrollableWidget::onMouseMove(mouseX, mouseY);

        if (!isActive() || !isVisible()) return handled;

        i32 newIndex = getIndexAt(mouseX, mouseY);
        if (newIndex != m_hoveredIndex) {
            m_hoveredIndex = newIndex;
        }
        return newIndex >= 0 || handled;
    }

    void paint(PaintContext& ctx) override
    {
        if (!isVisible()) return;

        // 绘制背景
        ctx.drawFilledRect(bounds(), Colors::fromARGB(255, 18, 18, 18));
        ctx.drawBorder(bounds(), 1.0f, Colors::fromARGB(255, 80, 80, 80));

        // 计算可见区域
        i32 contentX = m_bounds.x + m_padding.left;
        i32 contentWidth = visibleWidth();

        // 绘制可见项目
        i32 currentY = m_bounds.y + m_padding.top - m_scrollY;

        ctx.save();
        ctx.translate(0, -static_cast<f32>(m_scrollY));

        for (size_t i = 0; i < m_items.size(); ++i) {
            auto& item = m_items[i];
            i32 itemHeight = item->getHeight();

            // 检查是否在可见区域内
            if (currentY + itemHeight >= m_bounds.y && currentY < m_bounds.bottom()) {
                bool selected = (m_selectedIndex == static_cast<i32>(i));
                bool hovered = (m_hoveredIndex == static_cast<i32>(i));
                item->paintItem(ctx, contentX, currentY, contentWidth, selected, hovered);
            }

            currentY += itemHeight;
        }

        ctx.restore();

        // 绘制滚动条
        if (m_showScrollbar && m_contentHeight > visibleHeight()) {
            paintScrollbar(ctx);
        }
    }

    // ==================== 事件处理 ====================

    bool onClick(i32 mouseX, i32 mouseY, i32 button, i32 mods) override
    {
        (void)mods;
        if (!isActive() || !isVisible()) return false;

        // 首先检查滚动条（垂直或水平）
        if ((m_showScrollbar && isOnScrollbar(mouseX, mouseY)) ||
            (m_showHorizontalScrollbar && isOnHorizontalScrollbar(mouseX, mouseY))) {
            return ScrollableWidget::onClick(mouseX, mouseY, button, mods);
        }

        // 获取点击的项目索引
        i32 index = getIndexAt(mouseX, mouseY);
        if (index >= 0) {
            selectItem(index);

            // 处理点击
            auto& item = m_items[index];
            item->onClick(mouseX, mouseY - getItemY(index), button);

            return true;
        }

        return false;
    }

    bool onRelease(i32 mouseX, i32 mouseY, i32 button, i32 mods) override
    {
        (void)mods;
        // 释放事件委托给父类处理滚动条等
        return ScrollableWidget::onRelease(mouseX, mouseY, button, mods);
    }

    /**
     * @brief 双击事件处理
     *
     * 由KageroEngine在检测到双击时调用（250ms内同一Widget、同一按钮）。
     * ListWidget在双击时触发列表项的onDoubleClick回调和m_onDoubleClick。
     */
    bool onDoubleClick(i32 mouseX, i32 mouseY, i32 button, i32 mods) override
    {
        (void)mods;
        if (!isActive() || !isVisible()) return false;
        if (button != 0) return false; // 仅左键双击

        i32 index = getIndexAt(mouseX, mouseY);
        if (index >= 0) {
            auto& item = m_items[index];
            item->onDoubleClick(mouseX, mouseY - getItemY(index));

            if (m_onDoubleClick) {
                m_onDoubleClick(index, item.get());
            }

            // 调用基类实现以触发模板回调
            Widget::onDoubleClick(mouseX, mouseY, button, mods);
            return true;
        }
        return false;
    }

    // ==================== 项目操作 ====================

    /**
     * @brief 添加项目
     */
    void addItem(std::unique_ptr<IListItem> item)
    {
        m_items.push_back(std::move(item));
        updateContentHeight();
    }

    /**
     * @brief 插入项目
     */
    void insertItem(size_t index, std::unique_ptr<IListItem> item)
    {
        if (index <= m_items.size()) {
            m_items.insert(m_items.begin() + static_cast<i32>(index), std::move(item));
            updateContentHeight();
        }
    }

    /**
     * @brief 移除项目
     */
    void removeItem(size_t index)
    {
        if (index < m_items.size()) {
            m_items.erase(m_items.begin() + static_cast<i32>(index));

            // 更新选中索引
            if (m_selectedIndex == static_cast<i32>(index)) {
                m_selectedIndex = -1;
            } else if (m_selectedIndex > static_cast<i32>(index)) {
                --m_selectedIndex;
            }

            updateContentHeight();
        }
    }

    /**
     * @brief 清空所有项目
     */
    void clearItems()
    {
        m_items.clear();
        m_selectedIndex = -1;
        m_hoveredIndex = -1;
        updateContentHeight();
        scrollToTop();
    }

    /**
     * @brief 获取项目数量
     */
    [[nodiscard]] size_t itemCount() const { return m_items.size(); }

    /**
     * @brief 获取项目
     */
    [[nodiscard]] IListItem* getItem(size_t index)
    {
        if (index < m_items.size()) {
            return m_items[index].get();
        }
        return nullptr;
    }

    /**
     * @brief 获取项目（const版本）
     */
    [[nodiscard]] const IListItem* getItem(size_t index) const
    {
        if (index < m_items.size()) {
            return m_items[index].get();
        }
        return nullptr;
    }

    // ==================== 选择操作 ====================

    /**
     * @brief 选择项目
     */
    void selectItem(size_t index)
    {
        if (m_selectionMode == SelectionMode::None) return;
        if (index >= m_items.size()) return;

        i32 oldIndex = m_selectedIndex;
        i32 newIndex = static_cast<i32>(index);

        if (m_selectionMode == SelectionMode::Multiple) {
            // 多选模式：切换选择状态
            auto it = std::find(m_selectedIndices.begin(), m_selectedIndices.end(), newIndex);
            if (it != m_selectedIndices.end()) {
                m_selectedIndices.erase(it);
            } else {
                m_selectedIndices.push_back(newIndex);
            }
            m_selectedIndex = newIndex;
        } else {
            // 单选模式
            if (m_selectedIndex != newIndex) {
                m_selectedIndex = newIndex;
                m_selectedIndices.clear();
                m_selectedIndices.push_back(newIndex);
            }
        }

        // 触发回调
        if (oldIndex != newIndex && m_onSelectionChanged) {
            m_onSelectionChanged(oldIndex, newIndex);
        }
        if (m_onSelect) {
            m_onSelect(index, m_items[index].get());
        }
    }

    /**
     * @brief 清除选择
     */
    void clearSelection()
    {
        m_selectedIndex = -1;
        m_selectedIndices.clear();
    }

    /**
     * @brief 获取选中索引
     */
    [[nodiscard]] i32 selectedIndex() const { return m_selectedIndex; }

    /**
     * @brief 获取选中项目
     */
    [[nodiscard]] IListItem* selectedItem()
    {
        if (m_selectedIndex >= 0 && m_selectedIndex < static_cast<i32>(m_items.size())) {
            return m_items[m_selectedIndex].get();
        }
        return nullptr;
    }

    /**
     * @brief 设置多选模式
     *
     * 多选模式下，用户可以通过Ctrl+点击选择多个项目
     */
    void setMultiSelect(bool multiSelect)
    {
        m_selectionMode = multiSelect ? SelectionMode::Multiple : SelectionMode::Single;
        if (!multiSelect && m_selectedIndices.size() > 1) {
            // 保留第一个选择
            if (!m_selectedIndices.empty()) {
                m_selectedIndex = m_selectedIndices[0];
                m_selectedIndices.clear();
                m_selectedIndices.push_back(m_selectedIndex);
            }
        }
    }

    /**
     * @brief 是否启用多选
     */
    [[nodiscard]] bool isMultiSelect() const { return m_selectionMode == SelectionMode::Multiple; }

    /**
     * @brief 设置选中的索引列表
     */
    void setSelectedIndices(const std::vector<i32>& indices)
    {
        if (m_selectionMode == SelectionMode::None) return;

        m_selectedIndices.clear();
        for (i32 idx : indices) {
            if (idx >= 0 && idx < static_cast<i32>(m_items.size())) {
                m_selectedIndices.push_back(idx);
            }
        }

        // 更新单选索引为第一个选中项
        m_selectedIndex = m_selectedIndices.empty() ? -1 : m_selectedIndices[0];
    }

    /**
     * @brief 获取选中的索引列表
     */
    [[nodiscard]] const std::vector<i32>& selectedIndices() const { return m_selectedIndices; }

    /**
     * @brief 检查指定索引是否被选中
     */
    [[nodiscard]] bool isSelected(i32 index) const
    {
        return std::find(m_selectedIndices.begin(), m_selectedIndices.end(), index) != m_selectedIndices.end();
    }

    /**
     * @brief 设置选择模式
     */
    void setSelectionMode(SelectionMode mode)
    {
        m_selectionMode = mode;
        if (mode == SelectionMode::None) {
            clearSelection();
        }
    }

    /**
     * @brief 获取选择模式
     */
    [[nodiscard]] SelectionMode selectionMode() const { return m_selectionMode; }

    // ==================== 回调设置 ====================

    /**
     * @brief 设置选择回调
     */
    void setOnSelect(OnSelectCallback callback) { m_onSelect = std::move(callback); }

    /**
     * @brief 设置选择变化回调（与文档一致）
     *
     * 当选择从一项变为另一项时触发
     * @param callback 回调函数，参数为(旧索引, 新索引)
     */
    void setOnSelectionChanged(std::function<void(i32, i32)> callback) { m_onSelectionChanged = std::move(callback); }

    /**
     * @brief 设置双击回调
     */
    void setOnDoubleClick(OnDoubleClickCallback callback) { m_onDoubleClick = std::move(callback); }

    /**
     * @brief 设置项目高度（固定高度模式）
     */
    void setItemHeight(i32 height)
    {
        m_fixedItemHeight = height;
        updateContentHeight();
    }

    /**
     * @brief 获取项目高度
     */
    [[nodiscard]] i32 itemHeight() const { return m_fixedItemHeight; }

    /**
     * @brief 获取当前悬停项索引
     *
     * 返回鼠标当前悬停的列表项索引，无悬停时返回 -1。
     */
    [[nodiscard]] i32 hoveredIndex() const { return m_hoveredIndex; }

    // ==================== 数据绑定 ====================

    /**
     * @brief 项目工厂类型
     *
     * 用于从 Value 数据创建列表项
     */
    using ItemFactory = std::function<std::unique_ptr<IListItem>(
        const ::mc::client::ui::kagero::tpl::binder::Value& data, size_t index)>;

    /**
     * @brief 从 Value 数组设置列表项
     *
     * 用于模板绑定 bind:items。调用后会将 array 缓存为内部数据源，
     * 后续可通过 refreshItems() 使用该缓存重新创建列表项（例如在
     * 替换 ItemFactory 之后）。
     * @param array 包含列表数据的 Value 数组
     */
    void setItemsFromValue(const ::mc::client::ui::kagero::tpl::binder::Value& array);

    /**
     * @brief 设置项目工厂
     *
     * 用于自定义从 Value 创建 IListItem 的方式
     * @param factory 工厂函数
     */
    void setItemFactory(ItemFactory factory) { m_itemFactory = std::move(factory); }

    /**
     * @brief 获取项目工厂
     */
    [[nodiscard]] const ItemFactory& itemFactory() const { return m_itemFactory; }

    /**
     * @brief 设置数据变更回调
     *
     * 当列表数据更新时调用（包括 setItemsFromValue 与 refreshItems 重建后）
     */
    void setOnItemsChanged(std::function<void()> callback) { m_onItemsChanged = std::move(callback); }

    /**
     * @brief 刷新列表项
     *
     * 使用最近一次 setItemsFromValue 缓存的数据源重新创建列表项。
     * 适用场景：
     * - 替换 ItemFactory 后重建列表（无需重新提供 Value 数组）
     * - 强制重建以响应外部状态变化（模板绑定路径另有 tick 级自动刷新，此处仅用于直接 C++ 调用）
     *
     * 若从未调用过 setItemsFromValue，则该方法为空操作。
     * 刷新会尽可能保留当前选中索引（重新校验范围），并触发 m_onItemsChanged 回调。
     */
    void refreshItems();

    /**
     * @brief 检查是否存在已缓存的数据源
     *
     * @return 是否存在可通过 refreshItems 重建的数据源
     */
    [[nodiscard]] bool hasCachedDataSource() const { return m_dataSource != nullptr; }

protected:
    /**
     * @brief 更新内容高度
     */
    void updateContentHeight()
    {
        if (m_fixedItemHeight > 0) {
            setContentHeight(static_cast<i32>(m_items.size()) * m_fixedItemHeight);
        } else {
            i32 totalHeight = 0;
            for (const auto& item : m_items) {
                totalHeight += item->getHeight();
            }
            setContentHeight(totalHeight);
        }
    }

    /**
     * @brief 获取指定位置的项索引
     */
    [[nodiscard]] i32 getIndexAt(i32 mouseX, i32 mouseY) const
    {
        if (mouseX < m_bounds.x || mouseX >= m_bounds.right()) return -1;
        if (mouseY < m_bounds.y || mouseY >= m_bounds.bottom()) return -1;

        i32 relativeY = mouseY - m_bounds.y + m_scrollY - m_padding.top;

        if (m_fixedItemHeight > 0) {
            return relativeY / m_fixedItemHeight;
        } else {
            i32 currentY = 0;
            for (size_t i = 0; i < m_items.size(); ++i) {
                currentY += m_items[i]->getHeight();
                if (relativeY < currentY) {
                    return static_cast<i32>(i);
                }
            }
        }

        return -1;
    }

    /**
     * @brief 获取指定项的Y位置
     */
    [[nodiscard]] i32 getItemY(size_t index) const
    {
        if (m_fixedItemHeight > 0) {
            return static_cast<i32>(index) * m_fixedItemHeight;
        } else {
            i32 y = 0;
            for (size_t i = 0; i < index && i < m_items.size(); ++i) {
                y += m_items[i]->getHeight();
            }
            return y;
        }
    }

    /**
     * @brief 从给定的 Value 数组重建列表项
     *
     * 内部公共重建逻辑：清空当前列表项，按 array 中的元素依次调用 m_itemFactory
     * （未设置工厂时回退为 TextListItem）构造新项。不处理数据源缓存与选中状态保留，
     * 这些由调用方（setItemsFromValue / refreshItems）负责。
     *
     * @param array 已校验为 Array 类型的数据源
     */
    void _rebuildItemsFromArray(const ::mc::client::ui::kagero::tpl::binder::Value& array);

    // 项目
    std::vector<std::unique_ptr<IListItem>> m_items; ///< 列表项
    i32 m_fixedItemHeight = 20;                      ///< 固定项高度（0表示使用项目自己的高度）

    // 选择
    SelectionMode m_selectionMode = SelectionMode::Single; ///< 选择模式
    i32 m_selectedIndex = -1;                              ///< 选中索引（单选模式）
    std::vector<i32> m_selectedIndices;                    ///< 选中索引列表（多选模式）
    i32 m_hoveredIndex = -1;                               ///< 悬停索引

    // 回调
    OnSelectCallback m_onSelect;                        ///< 选择回调
    OnDoubleClickCallback m_onDoubleClick;              ///< 双击回调
    std::function<void(i32, i32)> m_onSelectionChanged; ///< 选择变化回调

    // 数据绑定
    ItemFactory m_itemFactory;              ///< 项目工厂
    std::function<void()> m_onItemsChanged; ///< 数据变更回调
    /// 缓存的数据源（最近一次 setItemsFromValue 的入参），用于 refreshItems 重建。
    /// 使用 unique_ptr 以避免在头文件中包含完整的 BindingContext.hpp；
    /// 析构由 ~ListWidget()（定义于 .cpp，此时 Value 已完整）负责。
    std::unique_ptr<::mc::client::ui::kagero::tpl::binder::Value> m_dataSource;
};

/**
 * @brief 简单文本列表项
 */
class TextListItem : public IListItem {
public:
    TextListItem(std::string text, i32 height = 20)
        : m_text(std::move(text))
        , m_height(height)
    {}

    [[nodiscard]] i32 getHeight() const override { return m_height; }

    void paintItem(PaintContext& ctx, i32 x, i32 y, i32 width, bool selected, bool hovered) override
    {
        // 先画背景，再画文本，保证选中态和悬停态可见
        if (selected) {
            Rect bg{x, y, width, m_height};
            ctx.drawFilledRect(bg, m_selectedColor);
        } else if (hovered) {
            Rect bg{x, y, width, m_height};
            ctx.drawFilledRect(bg, m_hoveredColor);
        }

        const i32 textY = y + (m_height - static_cast<i32>(ctx.getFontHeight())) / 2;
        ctx.drawText(m_text, x + 4, textY, m_textColor);
    }

    void setText(const std::string& text) { m_text = text; }
    [[nodiscard]] const std::string& text() const { return m_text; }

    void setTextColor(u32 color) { m_textColor = color; }
    [[nodiscard]] u32 textColor() const { return m_textColor; }

    void setSelectedColor(u32 color) { m_selectedColor = color; }
    void setHoveredColor(u32 color) { m_hoveredColor = color; }

private:
    std::string m_text;
    i32 m_height = 20;
    u32 m_textColor = Colors::WHITE;
    u32 m_selectedColor = Colors::fromARGB(128, 0, 0, 255);
    u32 m_hoveredColor = Colors::fromARGB(64, 255, 255, 255);
};

} // namespace mc::client::ui::kagero::widget
