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
#include "client/ui/Glyph.hpp"
#include "client/ui/kagero/Types.hpp"
#include "common/core/Types.hpp"
#include "common/input/KeyBinding.hpp"
#include <algorithm>
#include <functional>
#include <memory>
#include <string>
#include <utility>
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
 * 提供水平和垂直滚动功能的容器，可以包含其他组件。
 * 支持垂直滚动条（右侧）和水平滚动条（底部），以及对应的鼠标拖拽、
 * 滚轮和键盘交互。
 *
 * 使用示例：
 * @code
 * auto scrollable = std::make_unique<ScrollableWidget>("scroll", 10, 10, 200, 300);
 * scrollable->setContentSize(800, 1000);
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
        ctx.translate(-static_cast<f32>(m_scrollX), -static_cast<f32>(m_scrollY));
        paintChildren(ctx);
        ctx.restore();

        // 绘制垂直滚动条
        if (m_showScrollbar && m_contentHeight > visibleHeight()) {
            paintScrollbar(ctx);
        }

        // 绘制水平滚动条
        if (m_showHorizontalScrollbar && m_contentWidth > visibleWidth()) {
            paintHorizontalScrollbar(ctx);
        }
    }

    // ==================== 事件处理 ====================

    bool onClick(i32 mouseX, i32 mouseY, i32 button, i32 mods) override
    {
        (void)mods;
        if (!isActive() || !isVisible()) return false;

        // 检查是否点击垂直滚动条
        if (m_showScrollbar && isOnScrollbar(mouseX, mouseY)) {
            m_draggingScrollbar = true;
            return true;
        }

        // 检查是否点击水平滚动条
        if (m_showHorizontalScrollbar && isOnHorizontalScrollbar(mouseX, mouseY)) {
            m_draggingHorizontalScrollbar = true;
            return true;
        }

        // 调整坐标并传递给子组件
        i32 adjustedX = mouseX + m_scrollX;
        i32 adjustedY = mouseY + m_scrollY;
        return handleClickInChildren(adjustedX, adjustedY, button, mods);
    }

    bool onRelease(i32 mouseX, i32 mouseY, i32 button, i32 mods) override
    {
        (void)mouseX;
        (void)mouseY;
        (void)mods;
        if (button != 0) return false;

        if (m_draggingScrollbar) {
            m_draggingScrollbar = false;
            return true;
        }

        if (m_draggingHorizontalScrollbar) {
            m_draggingHorizontalScrollbar = false;
            return true;
        }

        i32 adjustedX = mouseX + m_scrollX;
        i32 adjustedY = mouseY + m_scrollY;
        return handleReleaseInChildren(adjustedX, adjustedY, button, mods);
    }

    bool onDoubleClick(i32 mouseX, i32 mouseY, i32 button, i32 mods) override
    {
        if (!isActive() || !isVisible()) return false;

        i32 adjustedX = mouseX + m_scrollX;
        i32 adjustedY = mouseY + m_scrollY;
        return handleDoubleClickInChildren(adjustedX, adjustedY, button, mods);
    }

    bool onRightClick(i32 mouseX, i32 mouseY, i32 mods) override
    {
        if (!isActive() || !isVisible()) return false;

        i32 adjustedX = mouseX + m_scrollX;
        i32 adjustedY = mouseY + m_scrollY;
        return handleRightClickInChildren(adjustedX, adjustedY, mods);
    }

    bool onDrag(i32 mouseX, i32 mouseY, i32 deltaX, i32 deltaY, i32 button) override
    {
        (void)button;
        if (m_draggingScrollbar) {
            // 垂直滚动条拖动
            i32 visibleH = visibleHeight();
            i32 maxScrollY = std::max(1, m_contentHeight - visibleH);
            i32 trackHeight = visibleH - scrollbarHeight();
            if (trackHeight <= 0) trackHeight = 1;
            f64 scrollRatio = static_cast<f64>(deltaY) / trackHeight;
            m_scrollY += static_cast<i32>(scrollRatio * maxScrollY);
            clampScroll();
            return true;
        }

        if (m_draggingHorizontalScrollbar) {
            // 水平滚动条拖动
            i32 visibleW = visibleWidth();
            i32 maxScrollX = std::max(1, m_contentWidth - visibleW);
            i32 trackWidth = visibleW - scrollbarWidthForHorizontal();
            if (trackWidth <= 0) trackWidth = 1;
            f64 scrollRatio = static_cast<f64>(deltaX) / trackWidth;
            m_scrollX += static_cast<i32>(scrollRatio * maxScrollX);
            clampScroll();
            return true;
        }

        i32 adjustedX = mouseX + m_scrollX;
        i32 adjustedY = mouseY + m_scrollY;
        for (auto& child : m_children) {
            if (child->isVisible() && child->isActive() && child->contains(adjustedX, adjustedY)) {
                if (child->onDrag(adjustedX, adjustedY, deltaX, deltaY, button)) {
                    return true;
                }
            }
        }
        return false;
    }

    bool onDragStart(i32 mouseX, i32 mouseY, i32 button, i32 mods) override
    {
        (void)mods;
        if (!isActive() || !isVisible()) return false;
        if (button != 0) return false; // 仅左键触发拖拽

        // 滚动条拖拽状态已由 onClick 设置（m_draggingScrollbar/m_draggingHorizontalScrollbar），
        // onDragStart 仅负责将事件转发给命中的子组件，让子组件也能感知拖拽开始。
        // 若点击落在滚动条区域，则不向子组件分发。
        if (m_draggingScrollbar || m_draggingHorizontalScrollbar) {
            return true;
        }

        // 调整坐标并传递给子组件
        i32 adjustedX = mouseX + m_scrollX;
        i32 adjustedY = mouseY + m_scrollY;
        return handleDragStartInChildren(adjustedX, adjustedY, button, mods);
    }

    bool onDragEnd(i32 mouseX, i32 mouseY, i32 button, bool dropped) override
    {
        if (button != 0) return false;

        // 滚动条拖拽：onDragEnd 仅作为通知，状态清理由 onRelease 统一完成
        // （避免 onDragEnd→onRelease 双重清理导致 onRelease 误将事件传递给子组件）。
        if (m_draggingScrollbar || m_draggingHorizontalScrollbar) {
            return true;
        }

        // 非滚动条拖拽：调整坐标并传递给子组件
        i32 adjustedX = mouseX + m_scrollX;
        i32 adjustedY = mouseY + m_scrollY;
        return handleDragEndInChildren(adjustedX, adjustedY, button, dropped);
    }

    bool onScroll(i32 mouseX, i32 mouseY, f64 delta) override
    {
        if (!isActive() || !isVisible()) return false;

        // 检测Shift键是否按下：Shift+滚轮进行水平滚动
        if (hasShiftModifier()) {
            // 水平滚动
            m_scrollX -= static_cast<i32>(delta * m_scrollSpeed);
            clampScroll();

            if (m_onScrollCallback) {
                m_onScrollCallback(mouseX, mouseY, -delta, 0.0);
            }
        } else {
            // 垂直滚动
            m_scrollY -= static_cast<i32>(delta * m_scrollSpeed);
            clampScroll();

            if (m_onScrollCallback) {
                m_onScrollCallback(mouseX, mouseY, 0.0, -delta);
            }
        }

        return true;
    }

    bool onKey(i32 key, i32 scanCode, i32 action, i32 mods) override
    {
        if (!isActive() || !isVisible()) return false;

        // 追踪Shift键状态（用于Shift+滚轮水平滚动）
        // 仅更新状态，不消费事件，以便Shift键正常传递给子组件和其他处理器
        if (key == Keys::LeftShift || key == Keys::RightShift) {
            m_shiftHeld =
                (action == static_cast<i32>(KeyAction::Press) || action == static_cast<i32>(KeyAction::Repeat));
            // Shift键释放时重置状态
            if (action == static_cast<i32>(KeyAction::Release)) {
                m_shiftHeld = false;
            }
            // 不消费Shift事件，让它继续传播
        }

        if (!isFocused()) return false;

        // 处理方向键滚动
        if (action == static_cast<i32>(KeyAction::Press) || action == static_cast<i32>(KeyAction::Repeat)) {
            switch (key) {
                case Keys::Down:
                    scrollBy(20);
                    return true;

                case Keys::Up:
                    scrollBy(-20);
                    return true;

                case Keys::PageDown:
                    scrollBy(m_bounds.height - m_padding.vertical());
                    return true;

                case Keys::PageUp:
                    scrollBy(-(m_bounds.height - m_padding.vertical()));
                    return true;

                case Keys::Right:
                    scrollByX(20);
                    return true;

                case Keys::Left:
                    scrollByX(-20);
                    return true;

                case Keys::Home:
                    if (hasMod(static_cast<KeyMods>(mods), KeyMods::Shift)) {
                        scrollToLeft();
                    } else {
                        scrollToTop();
                    }
                    return true;

                case Keys::End:
                    if (hasMod(static_cast<KeyMods>(mods), KeyMods::Shift)) {
                        scrollToRight();
                    } else {
                        scrollToBottom();
                    }
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
     * @brief 垂直滚动指定距离
     */
    void scrollBy(i32 delta)
    {
        m_scrollY += delta;
        clampScroll();
    }

    /**
     * @brief 水平滚动指定距离
     */
    void scrollByX(i32 delta)
    {
        m_scrollX += delta;
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
        i32 visH = visibleHeight();
        m_scrollY = std::max(0, m_contentHeight - visH);
    }

    /**
     * @brief 滚动到最左侧
     */
    void scrollToLeft() { m_scrollX = 0; }

    /**
     * @brief 滚动到最右侧
     */
    void scrollToRight()
    {
        i32 visW = visibleWidth();
        m_scrollX = std::max(0, m_contentWidth - visW);
    }

    /**
     * @brief 滚动到指定垂直位置
     */
    void scrollTo(i32 y)
    {
        m_scrollY = y;
        clampScroll();
    }

    /**
     * @brief 滚动到使指定位置可见
     * @param y 内容中的Y坐标
     * @param height 目标区域的高度
     */
    void scrollIntoView(i32 y, i32 height)
    {
        i32 visH = visibleHeight();
        i32 viewTop = m_scrollY;
        i32 viewBottom = m_scrollY + visH;

        if (y < viewTop) {
            m_scrollY = y;
        } else if (y + height > viewBottom) {
            m_scrollY = y + height - visH;
        }
        clampScroll();
    }

    /**
     * @brief 滚动到使指定组件可见（垂直方向）
     * @param child 子组件指针
     */
    void scrollIntoView(Widget* child)
    {
        if (child == nullptr) return;

        // 获取子组件在内容中的位置
        i32 childTop = child->y() - m_bounds.y;

        scrollIntoView(childTop, child->height());
    }

    /**
     * @brief 水平滚动到使指定位置可见
     * @param x 内容中的X坐标
     * @param width 目标区域的宽度
     */
    void scrollXIntoView(i32 x, i32 width)
    {
        i32 visW = visibleWidth();
        i32 viewLeft = m_scrollX;
        i32 viewRight = m_scrollX + visW;

        if (x < viewLeft) {
            m_scrollX = x;
        } else if (x + width > viewRight) {
            m_scrollX = x + width - visW;
        }
        clampScroll();
    }

    // ==================== 显示属性 ====================

    /**
     * @brief 设置是否显示垂直滚动条
     */
    void setShowScrollbar(bool show) { m_showScrollbar = show; }

    /**
     * @brief 是否显示垂直滚动条
     */
    [[nodiscard]] bool showScrollbar() const { return m_showScrollbar; }

    /**
     * @brief 设置是否显示水平滚动条
     */
    void setShowHorizontalScrollbar(bool show) { m_showHorizontalScrollbar = show; }

    /**
     * @brief 是否显示水平滚动条
     */
    [[nodiscard]] bool showHorizontalScrollbar() const { return m_showHorizontalScrollbar; }

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
     *
     * 当水平滚动条可见时，需要减去水平滚动条占用的底部空间。
     */
    [[nodiscard]] i32 visibleHeight() const
    {
        i32 h = m_bounds.height - m_padding.vertical();
        // 如果水平滚动条可见，需要减去水平滚动条高度
        if (m_showHorizontalScrollbar && m_contentWidth > visibleWidthRaw()) {
            h -= m_scrollbarWidth;
        }
        return h;
    }

    /**
     * @brief 获取可见区域宽度
     *
     * 当垂直滚动条可见时，需要减去垂直滚动条占用的右侧空间。
     */
    [[nodiscard]] i32 visibleWidth() const
    {
        i32 w = m_bounds.width - m_padding.horizontal();
        // 如果垂直滚动条可见，需要减去垂直滚动条宽度
        if (m_showScrollbar && m_contentHeight > visibleHeightRaw()) {
            w -= m_scrollbarWidth;
        }
        return w;
    }

    /**
     * @brief 计算垂直滚动比例（0.0-1.0）
     */
    [[nodiscard]] f64 scrollRatio() const
    {
        i32 maxScroll = m_contentHeight - visibleHeight();
        if (maxScroll <= 0) return 0.0;
        return static_cast<f64>(m_scrollY) / maxScroll;
    }

    /**
     * @brief 计算水平滚动比例（0.0-1.0）
     */
    [[nodiscard]] f64 horizontalScrollRatio() const
    {
        i32 maxScroll = m_contentWidth - visibleWidth();
        if (maxScroll <= 0) return 0.0;
        return static_cast<f64>(m_scrollX) / maxScroll;
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
     * @brief 检查是否在垂直滚动条上
     */
    [[nodiscard]] bool isOnScrollbar(i32 mouseX, i32 mouseY) const
    {
        if (!m_showScrollbar) return false;

        // 当水平滚动条可见时，垂直滚动条底部不覆盖水平滚动条区域
        i32 scrollbarBottom = m_bounds.bottom();
        if (isHorizontalScrollbarVisible()) {
            scrollbarBottom -= m_scrollbarWidth;
        }

        i32 scrollbarX = m_bounds.right() - m_scrollbarWidth;
        return mouseX >= scrollbarX && mouseX < m_bounds.right() && mouseY >= m_bounds.y && mouseY < scrollbarBottom;
    }

    /**
     * @brief 检查是否在水平滚动条上
     */
    [[nodiscard]] bool isOnHorizontalScrollbar(i32 mouseX, i32 mouseY) const
    {
        if (!m_showHorizontalScrollbar) return false;
        if (!isHorizontalScrollbarVisible()) return false;

        // 当垂直滚动条可见时，水平滚动条右侧不覆盖垂直滚动条区域
        i32 scrollbarRight = m_bounds.right();
        if (isVerticalScrollbarVisible()) {
            scrollbarRight -= m_scrollbarWidth;
        }

        i32 scrollbarY = m_bounds.bottom() - m_scrollbarWidth;
        return mouseX >= m_bounds.x && mouseX < scrollbarRight && mouseY >= scrollbarY && mouseY < m_bounds.bottom();
    }

    /**
     * @brief 检查垂直滚动条是否实际可见
     */
    [[nodiscard]] bool isVerticalScrollbarVisible() const
    {
        return m_showScrollbar && m_contentHeight > visibleHeightRaw();
    }

    /**
     * @brief 检查水平滚动条是否实际可见
     */
    [[nodiscard]] bool isHorizontalScrollbarVisible() const
    {
        return m_showHorizontalScrollbar && m_contentWidth > visibleWidthRaw();
    }

    /**
     * @brief 不考虑水平滚动条的可见高度（用于判断滚动条可见性）
     */
    [[nodiscard]] i32 visibleHeightRaw() const { return m_bounds.height - m_padding.vertical(); }

    /**
     * @brief 不考虑垂直滚动条的可见宽度（用于判断滚动条可见性）
     */
    [[nodiscard]] i32 visibleWidthRaw() const { return m_bounds.width - m_padding.horizontal(); }

    /**
     * @brief 计算垂直滚动条滑块高度
     */
    [[nodiscard]] i32 scrollbarHeight() const
    {
        i32 visibleH = visibleHeight();
        if (m_contentHeight <= 0) return 20;
        f64 ratio = static_cast<f64>(visibleH) / m_contentHeight;
        return std::max(20, static_cast<i32>(ratio * visibleH));
    }

    /**
     * @brief 计算水平滚动条滑块宽度
     */
    [[nodiscard]] i32 scrollbarWidthForHorizontal() const
    {
        i32 visibleW = visibleWidth();
        if (m_contentWidth <= 0) return 20;
        f64 ratio = static_cast<f64>(visibleW) / m_contentWidth;
        return std::max(20, static_cast<i32>(ratio * visibleW));
    }

    /**
     * @brief 检测当前是否按下了Shift修饰键
     *
     * 注意：ScrollableWidget::onScroll 不直接接收修饰键信息，
     * 此方法使用平台相关的状态查询。如果无法获取，默认返回false。
     */
    [[nodiscard]] bool hasShiftModifier() const { return m_shiftHeld; }

    /**
     * @brief 绘制垂直滚动条
     */
    void paintScrollbar(PaintContext& ctx)
    {
        i32 visibleH = visibleHeight();
        i32 maxScroll = m_contentHeight - visibleH;
        if (maxScroll <= 0) return;

        // 计算滑块高度和位置
        i32 thumbHeight = scrollbarHeight();
        i32 trackHeight = visibleH - thumbHeight;
        i32 thumbY = m_bounds.y + static_cast<i32>(scrollRatio() * trackHeight);

        // 垂直滚动条轨道不覆盖水平滚动条区域
        i32 trackFullHeight = m_bounds.height;
        if (isHorizontalScrollbarVisible()) {
            trackFullHeight -= m_scrollbarWidth;
        }

        // 绘制轨道（右侧）
        Rect track{m_bounds.right() - m_scrollbarWidth, m_bounds.y, m_scrollbarWidth, trackFullHeight};
        ctx.drawFilledRect(track, Colors::fromARGB(128, 40, 40, 40));

        // 绘制滑块
        Rect thumb{m_bounds.right() - m_scrollbarWidth, thumbY, m_scrollbarWidth, thumbHeight};
        ctx.drawFilledRect(thumb, Colors::fromARGB(200, 120, 120, 120));
    }

    /**
     * @brief 绘制水平滚动条
     */
    void paintHorizontalScrollbar(PaintContext& ctx)
    {
        i32 visibleW = visibleWidth();
        i32 maxScroll = m_contentWidth - visibleW;
        if (maxScroll <= 0) return;

        // 计算滑块宽度和位置
        i32 thumbWidth = scrollbarWidthForHorizontal();
        i32 trackWidth = visibleW - thumbWidth;
        i32 thumbX = m_bounds.x + static_cast<i32>(horizontalScrollRatio() * trackWidth);

        // 水平滚动条轨道不覆盖垂直滚动条区域
        i32 trackFullWidth = m_bounds.width;
        if (isVerticalScrollbarVisible()) {
            trackFullWidth -= m_scrollbarWidth;
        }

        // 绘制轨道（底部）
        Rect track{m_bounds.x, m_bounds.bottom() - m_scrollbarWidth, trackFullWidth, m_scrollbarWidth};
        ctx.drawFilledRect(track, Colors::fromARGB(128, 40, 40, 40));

        // 绘制滑块
        Rect thumb{thumbX, m_bounds.bottom() - m_scrollbarWidth, thumbWidth, m_scrollbarWidth};
        ctx.drawFilledRect(thumb, Colors::fromARGB(200, 120, 120, 120));
    }

    // 内容尺寸
    i32 m_contentWidth = 0;  ///< 内容宽度
    i32 m_contentHeight = 0; ///< 内容高度

    // 滚动位置
    i32 m_scrollX = 0; ///< 水平滚动位置
    i32 m_scrollY = 0; ///< 垂直滚动位置

    // 滚动条
    bool m_showScrollbar = true;           ///< 是否显示垂直滚动条
    bool m_showHorizontalScrollbar = true; ///< 是否显示水平滚动条
    i32 m_scrollbarWidth = 6;              ///< 滚动条宽度（垂直）/ 高度（水平）
    f64 m_scrollSpeed = 20.0;              ///< 滚动速度

    // 状态
    bool m_draggingScrollbar = false;           ///< 是否正在拖动垂直滚动条
    bool m_draggingHorizontalScrollbar = false; ///< 是否正在拖动水平滚动条
    bool m_shiftHeld = false;                   ///< Shift键是否按下（用于Shift+滚轮水平滚动）

    // 回调
    ScrollCallback m_onScrollCallback; ///< 滚动事件回调
};

} // namespace mc::client::ui::kagero::widget
