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
#include "IWidgetContainer.hpp"
#include "Widget.hpp"
#include <functional>
#include <memory>
#include <vector>

namespace mc::client::ui::kagero::widget {

/**
 * @brief 滚动回调类型
 *
 * 参数: x, y, deltaX, deltaY
 */
using ScrollCallback = std::function<void(i32, i32, f64, f64)>;

/**
 * @brief 可滚动容器组件
 *
 * 提供滚动功能的容器，可以包含其他组件。
 *
 * 参考MC 1.16.5 AbstractList.java实现
 *
 * 使用示例：
 * @code
 * auto scrollable = std::make_unique<ScrollableWidget>("scroll", 10, 10, 200, 300);
 * scrollable->setContentHeight(1000);
 * scrollable->addWidget(std::make_unique<TextWidget>("item1", 0, 0, 180, 30, "Item 1"));
 * @endcode
 */
class ScrollableWidget : public Widget, public WidgetContainerMixin<ScrollableWidget> {
public:
    using WidgetContainerMixin<ScrollableWidget>::addWidget;
    using WidgetContainerMixin<ScrollableWidget>::widgets;
    using WidgetContainerMixin<ScrollableWidget>::findWidgetById;
    using WidgetContainerMixin<ScrollableWidget>::getWidgetAt;

    /**
     * @brief 默认构造函数
     */
    ScrollableWidget() = default;

    /**
     * @brief 构造函数
     * @param id 组件ID
     * @param x X坐标
     * @param y Y坐标
     * @param width 宽度
     * @param height 高度
     */
    ScrollableWidget(std::string id, i32 x, i32 y, i32 width, i32 height)
        : Widget(std::move(id))
    {
        setBounds(Rect(x, y, width, height));
    }

    // ==================== 生命周期 ====================

    void init() override
    {
        for (auto& child : m_children) {
            child->init();
        }
    }

    void tick(f32 dt) override
    {
        if (!isVisible() || !isActive()) return;
        tickChildren(dt);
    }

    void paint(PaintContext& ctx) override
    {
        if (!isVisible()) return;
        ctx.drawFilledRect(bounds(), Colors::fromARGB(255, 20, 20, 20));
        ctx.drawBorder(bounds(), 1.0f, Colors::fromARGB(255, 70, 70, 70));

        // 绘制子组件（带滚动偏移）
        ctx.save();
        ctx.translate(0, -static_cast<f32>(m_scrollY));
        paintChildren(ctx);
        ctx.restore();

        // 绘制滚动条
        if (m_showScrollbar && m_contentHeight > visibleHeight()) {
            paintScrollbar(ctx);
        }
    }

    // ==================== 事件处理 ====================

    bool onClick(i32 mouseX, i32 mouseY, i32 button) override
    {
        if (!isActive() || !isVisible()) return false;

        // 检查是否点击滚动条
        if (m_showScrollbar && isOnScrollbar(mouseX, mouseY)) {
            m_draggingScrollbar = true;
            m_lastMouseY = mouseY;
            return true;
        }

        // 调整Y坐标并传递给子组件
        i32 adjustedY = mouseY + m_scrollY;
        return handleClickInChildren(mouseX, adjustedY, button);
    }

    bool onRelease(i32 mouseX, i32 mouseY, i32 button) override
    {
        (void)mouseX;
        (void)mouseY;

        if (button != 0) return false;

        if (m_draggingScrollbar) {
            m_draggingScrollbar = false;
            return true;
        }

        i32 adjustedY = mouseY + m_scrollY;
        return handleReleaseInChildren(mouseX, adjustedY, button);
    }

    bool onDrag(i32 mouseX, i32 mouseY, i32 deltaX, i32 deltaY) override
    {
        (void)deltaX;

        if (m_draggingScrollbar) {
            // 滚动条拖动
            i32 visibleHeight = m_bounds.height - m_padding.vertical();
            f64 scrollRatio = static_cast<f64>(deltaY) / visibleHeight;
            m_scrollY += static_cast<i32>(scrollRatio * m_contentHeight);
            clampScroll();
            return true;
        }

        i32 adjustedY = mouseY + m_scrollY;
        for (auto& child : m_children) {
            if (child->isVisible() && child->isActive() && child->contains(mouseX, adjustedY)) {
                if (child->onDrag(mouseX, adjustedY, deltaX, deltaY)) {
                    return true;
                }
            }
        }
        return false;
    }

    bool onScroll(i32 mouseX, i32 mouseY, f64 delta) override
    {
        (void)mouseX;
        (void)mouseY;

        if (!isActive() || !isVisible()) return false;

        // 滚动内容
        m_scrollY -= static_cast<i32>(delta * m_scrollSpeed);
        clampScroll();

        // 调用用户回调
        if (m_onScrollCallback) {
            m_onScrollCallback(mouseX, mouseY, 0.0, delta);
        }

        return true;
    }

    bool onKey(i32 key, i32 scanCode, i32 action, i32 mods) override
    {
        if (!isActive() || !isVisible() || !isFocused()) return false;

        // 处理上下键滚动
        if (action == 1 || action == 2) {
            switch (key) {
                case 264: // GLFW_KEY_DOWN
                    scrollBy(20);
                    return true;

                case 265: // GLFW_KEY_UP
                    scrollBy(-20);
                    return true;

                case 266: // GLFW_KEY_PAGE_DOWN
                    scrollBy(m_bounds.height - m_padding.vertical());
                    return true;

                case 267: // GLFW_KEY_PAGE_UP
                    scrollBy(-(m_bounds.height - m_padding.vertical()));
                    return true;

                default:
                    break;
            }
        }

        // 传递给子组件
        for (auto& child : m_children) {
            if (child->isVisible() && child->isActive() && child->isFocused()) {
                if (child->onKey(key, scanCode, action, mods)) {
                    return true;
                }
            }
        }

        return false;
    }

    // ==================== 滚动操作 ====================

    /**
     * @brief 设置内容宽度
     */
    void setContentWidth(i32 width)
    {
        m_contentWidth = width;
        clampScroll();
    }

    /**
     * @brief 获取内容宽度
     */
    [[nodiscard]] i32 contentWidth() const { return m_contentWidth; }

    /**
     * @brief 设置内容高度
     */
    void setContentHeight(i32 height)
    {
        m_contentHeight = height;
        clampScroll();
    }

    /**
     * @brief 获取内容高度
     */
    [[nodiscard]] i32 contentHeight() const { return m_contentHeight; }

    /**
     * @brief 设置内容尺寸
     */
    void setContentSize(i32 width, i32 height)
    {
        m_contentWidth = width;
        m_contentHeight = height;
        clampScroll();
    }

    /**
     * @brief 设置水平滚动位置
     */
    void setScrollX(i32 scrollX)
    {
        m_scrollX = scrollX;
        clampScroll();
    }

    /**
     * @brief 获取水平滚动位置
     */
    [[nodiscard]] i32 scrollX() const { return m_scrollX; }

    /**
     * @brief 设置垂直滚动位置
     */
    void setScrollY(i32 scrollY)
    {
        m_scrollY = scrollY;
        clampScroll();
    }

    /**
     * @brief 获取垂直滚动位置
     */
    [[nodiscard]] i32 scrollY() const { return m_scrollY; }

    /**
     * @brief 滚动指定距离
     */
    void scrollBy(i32 delta)
    {
        m_scrollY += delta;
        clampScroll();
    }

    /**
     * @brief 滚动到顶部
     */
    void scrollToTop() { m_scrollY = 0; }

    /**
     * @brief 滚动到底部
     */
    void scrollToBottom()
    {
        i32 visibleHeight = m_bounds.height - m_padding.vertical();
        m_scrollY = std::max(0, m_contentHeight - visibleHeight);
    }

    /**
     * @brief 滚动到指定位置
     */
    void scrollTo(i32 y)
    {
        m_scrollY = y;
        clampScroll();
    }

    /**
     * @brief 滚动到使指定位置可见
     */
    void scrollIntoView(i32 y, i32 height = 0)
    {
        i32 visibleHeight = m_bounds.height - m_padding.vertical();
        i32 viewTop = m_scrollY;
        i32 viewBottom = m_scrollY + visibleHeight;

        if (y < viewTop) {
            m_scrollY = y;
        } else if (y + height > viewBottom) {
            m_scrollY = y + height - visibleHeight;
        }
        clampScroll();
    }

    /**
     * @brief 滚动到使指定组件可见
     * @param child 子组件指针
     */
    void scrollIntoView(Widget* child)
    {
        if (child == nullptr) return;

        // 获取子组件在内容中的位置
        i32 childTop = child->y() - m_bounds.y;
        i32 childBottom = childTop + child->height();

        scrollIntoView(childTop, child->height());
    }

    /**
     * @brief 水平滚动指定距离
     */
    void scrollByX(i32 delta)
    {
        m_scrollX += delta;
        clampScroll();
    }

    // ==================== 显示属性 ====================

    /**
     * @brief 设置是否显示滚动条
     */
    void setShowScrollbar(bool show) { m_showScrollbar = show; }

    /**
     * @brief 是否显示滚动条
     */
    [[nodiscard]] bool showScrollbar() const { return m_showScrollbar; }

    /**
     * @brief 设置滚动速度
     */
    void setScrollSpeed(f64 speed) { m_scrollSpeed = speed; }

    /**
     * @brief 获取滚动速度
     */
    [[nodiscard]] f64 scrollSpeed() const { return m_scrollSpeed; }

    /**
     * @brief 设置滚动回调
     * @param callback 回调函数，参数为 (x, y, deltaX, deltaY)
     */
    void setOnScroll(ScrollCallback callback) { m_onScrollCallback = std::move(callback); }

    /**
     * @brief 清除滚动回调
     */
    void clearOnScroll() { m_onScrollCallback = nullptr; }

    /**
     * @brief 设置滚动条宽度
     */
    void setScrollbarWidth(i32 width) { m_scrollbarWidth = width; }

    /**
     * @brief 获取滚动条宽度
     */
    [[nodiscard]] i32 scrollbarWidth() const { return m_scrollbarWidth; }

    /**
     * @brief 获取可见区域高度
     */
    [[nodiscard]] i32 visibleHeight() const { return m_bounds.height - m_padding.vertical(); }

    /**
     * @brief 获取可见区域宽度
     */
    [[nodiscard]] i32 visibleWidth() const
    {
        return m_bounds.width - m_padding.horizontal() - (m_showScrollbar ? m_scrollbarWidth : 0);
    }

    /**
     * @brief 计算滚动比例（0.0-1.0）
     */
    [[nodiscard]] f64 scrollRatio() const
    {
        i32 maxScroll = m_contentHeight - visibleHeight();
        if (maxScroll <= 0) return 0.0;
        return static_cast<f64>(m_scrollY) / maxScroll;
    }

protected:
    /**
     * @brief 限制滚动范围
     */
    void clampScroll()
    {
        i32 maxScrollY = std::max(0, m_contentHeight - visibleHeight());
        m_scrollY = std::max(0, std::min(m_scrollY, maxScrollY));
        i32 maxScrollX = std::max(0, m_contentWidth - visibleWidth());
        m_scrollX = std::max(0, std::min(m_scrollX, maxScrollX));
    }

    /**
     * @brief 检查是否在滚动条上
     */
    [[nodiscard]] bool isOnScrollbar(i32 mouseX, i32 mouseY) const
    {
        if (!m_showScrollbar) return false;

        i32 scrollbarX = m_bounds.right() - m_scrollbarWidth;
        return mouseX >= scrollbarX && mouseX < m_bounds.right() && mouseY >= m_bounds.y && mouseY < m_bounds.bottom();
    }

    /**
     * @brief 绘制滚动条
     */
    void paintScrollbar(PaintContext& ctx)
    {
        i32 visibleH = visibleHeight();
        i32 maxScroll = m_contentHeight - visibleH;
        if (maxScroll <= 0) return;

        // 计算滚动条高度和位置
        f64 ratio = static_cast<f64>(visibleH) / m_contentHeight;
        i32 scrollbarHeight = std::max(20, static_cast<i32>(ratio * visibleH));
        i32 scrollbarY = m_bounds.y + static_cast<i32>(scrollRatio() * (visibleH - scrollbarHeight));

        // 绘制滚动条轨道
        Rect track{m_bounds.right() - m_scrollbarWidth, m_bounds.y, m_scrollbarWidth, m_bounds.height};
        ctx.drawFilledRect(track, Colors::fromARGB(128, 40, 40, 40));

        // 绘制滚动条滑块
        Rect thumb{m_bounds.right() - m_scrollbarWidth, scrollbarY, m_scrollbarWidth, scrollbarHeight};
        ctx.drawFilledRect(thumb, Colors::fromARGB(200, 120, 120, 120));
    }

    // 处理子组件拖动
    bool handleDragInChildren(i32 mouseX, i32 mouseY, i32 deltaX, i32 deltaY)
    {
        Widget* widget = getWidgetAt(mouseX, mouseY);
        if (widget != nullptr) {
            return widget->onDrag(mouseX, mouseY, deltaX, deltaY);
        }
        return false;
    }

    // 内容尺寸
    i32 m_contentWidth = 0;  ///< 内容宽度
    i32 m_contentHeight = 0; ///< 内容高度

    // 滚动位置
    i32 m_scrollX = 0; ///< 水平滚动位置
    i32 m_scrollY = 0; ///< 垂直滚动位置

    // 滚动条
    bool m_showScrollbar = true;           ///< 是否显示滚动条
    bool m_showHorizontalScrollbar = true; ///< 是否显示水平滚动条
    i32 m_scrollbarWidth = 6;              ///< 滚动条宽度
    f64 m_scrollSpeed = 20.0;              ///< 滚动速度

    // 状态
    bool m_draggingScrollbar = false; ///< 是否正在拖动滚动条
    i32 m_lastMouseX = 0;             ///< 上次鼠标X位置
    i32 m_lastMouseY = 0;             ///< 上次鼠标Y位置

    // 回调
    ScrollCallback m_onScrollCallback; ///< 滚动事件回调
};

} // namespace mc::client::ui::kagero::widget
