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

#include "Widget.hpp"
#include "common/core/Types.hpp"
#include "common/util/assert/AssertAll.hpp"
#include <algorithm>
#include <cstddef>
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace mc::client::ui::kagero::widget {

/**
 * @brief Widget容器接口
 *
 * 定义可以包含子组件的容器行为。
 * 实现 IWidgetContainer 的类可以管理一组子组件。
 *
 * 使用示例：
 * @code
 * class Panel : public Widget, public IWidgetContainer {
 * public:
 *     void addWidget(Widget::Ptr widget) override {
 *         widget->setParent(this);
 *         widget->init();
 *         m_children.push_back(std::move(widget));
 *     }
 *
 *     void paint(PaintContext& ctx) override {
 *         paintChildren(ctx);
 *     }
 * };
 * @endcode
 */
class IWidgetContainer {
public:
    virtual ~IWidgetContainer() = default;

    /**
     * @brief 添加子组件
     * @param widget 子组件
     */
    virtual void addWidget(Widget::Ptr widget) = 0;

    /**
     * @brief 移除子组件
     * @param widget 子组件指针
     */
    virtual void removeWidget(Widget* widget) = 0;

    /**
     * @brief 通过ID移除子组件
     * @param id 组件ID
     * @return 如果找到并移除返回true
     */
    virtual bool removeWidgetById(const std::string& id) = 0;

    /**
     * @brief 清空所有子组件
     */
    virtual void clearWidgets() = 0;

    /**
     * @brief 获取所有子组件
     */
    [[nodiscard]] virtual const std::vector<Widget::Ptr>& widgets() const = 0;

    /**
     * @brief 获取子组件数量
     */
    [[nodiscard]] virtual size_t widgetCount() const = 0;

    /**
     * @brief 通过ID查找子组件
     * @param id 组件ID
     * @return 组件指针，如果未找到返回nullptr
     */
    [[nodiscard]] virtual Widget* findWidgetById(const std::string& id) = 0;

    /**
     * @brief 通过ID查找子组件（const版本）
     */
    [[nodiscard]] virtual const Widget* findWidgetById(const std::string& id) const = 0;

    /**
     * @brief 获取指定位置的组件
     * @param x X坐标
     * @param y Y坐标
     * @return 组件指针，如果未找到返回nullptr
     *
     * 返回最上层（最后添加）的可见且激活的组件
     */
    [[nodiscard]] virtual Widget* getWidgetAt(i32 x, i32 y) = 0;

    /**
     * @brief 获取指定位置的组件（const版本）
     */
    [[nodiscard]] virtual const Widget* getWidgetAt(i32 x, i32 y) const = 0;

    /**
     * @brief 将子组件提升到顶层
     * @param widget 子组件指针
     */
    virtual void bringToFront(Widget* widget) = 0;

    /**
     * @brief 将子组件降低到底层
     * @param widget 子组件指针
     */
    virtual void sendToBack(Widget* widget) = 0;

    /**
     * @brief 遍历所有子组件
     * @param callback 回调函数
     */
    virtual void forEachWidget(const std::function<void(Widget&)>& callback) = 0;

    /**
     * @brief 遍历所有子组件（const版本）
     */
    virtual void forEachWidget(const std::function<void(const Widget&)>& callback) const = 0;
};

/**
 * @brief Widget容器混入类
 *
 * 提供IWidgetContainer的默认实现，可以混入到其他类中。
 * 使用CRTP模式。
 *
 * @tparam Derived 派生类类型
 *
 * 使用示例：
 * @code
 * class Panel : public Widget, public WidgetContainerMixin<Panel> {
 * public:
 *     using WidgetContainerMixin<Panel>::addWidget;
 *     using WidgetContainerMixin<Panel>::widgets;
 *
 *     void paint(PaintContext& ctx) override {
 *         paintChildren(ctx);
 *     }
 * };
 * @endcode
 */
template <typename Derived>
// TODO: Derived 模板参数目前未被使用（伪 CRTP），未来如果需要静态多态或访问派生类类型信息时再启用，
//       否则应移除该模板参数以减少编译膨胀
class WidgetContainerMixin : public IWidgetContainer {
public:
    WidgetContainerMixin() = default;
    ~WidgetContainerMixin() override = default;

    // 禁止拷贝（因为 m_children 包含 unique_ptr）
    WidgetContainerMixin(const WidgetContainerMixin&) = delete;
    WidgetContainerMixin& operator=(const WidgetContainerMixin&) = delete;

    // 允许移动
    WidgetContainerMixin(WidgetContainerMixin&&) = default;
    WidgetContainerMixin& operator=(WidgetContainerMixin&&) = default;

    void addWidget(Widget::Ptr widget) override
    {
        MC_ASSERT_RELEASE(widget != nullptr);
        widget->setParent(this);
        widget->init();
        m_children.push_back(std::move(widget));
    }

    /**
     * @brief 添加子组件（别名，与文档一致）
     * @param widget 子组件
     */
    void addChild(Widget::Ptr widget) { addWidget(std::move(widget)); }

    void removeWidget(Widget* widget) override
    {
        MC_ASSERT_RELEASE(widget != nullptr);

        // 清除焦点引用，防止野指针
        if (m_focusedWidget == widget) {
            m_focusedWidget = nullptr;
        }

        auto it = std::find_if(
            m_children.begin(), m_children.end(), [widget](const Widget::Ptr& ptr) { return ptr.get() == widget; });

        if (it != m_children.end()) {
            (*it)->setParent(nullptr);
            m_children.erase(it);
        }
    }

    /**
     * @brief 移除子组件（别名，与文档一致）
     * @param id 组件ID
     * @return 是否成功移除
     */
    bool removeChild(const std::string& id) { return removeWidgetById(id); }

    bool removeWidgetById(const std::string& id) override
    {
        auto it = std::find_if(
            m_children.begin(), m_children.end(), [&id](const Widget::Ptr& ptr) { return ptr->id() == id; });

        if (it != m_children.end()) {
            // 清除焦点引用，防止野指针
            if (m_focusedWidget == it->get()) {
                m_focusedWidget = nullptr;
            }
            (*it)->setParent(nullptr);
            m_children.erase(it);
            return true;
        }
        return false;
    }

    void clearWidgets() override
    {
        for (auto& child : m_children) {
            child->setParent(nullptr);
        }
        m_children.clear();
    }

    /**
     * @brief 清空所有子组件（别名，与文档一致）
     */
    void clearChildren() { clearWidgets(); }

    [[nodiscard]] const std::vector<Widget::Ptr>& widgets() const override { return m_children; }

    /**
     * @brief 获取子组件数量（别名，与文档一致）
     */
    [[nodiscard]] size_t childCount() const { return m_children.size(); }

    [[nodiscard]] size_t widgetCount() const override { return m_children.size(); }

    [[nodiscard]] Widget* findWidgetById(const std::string& id) override
    {
        auto it = std::find_if(
            m_children.begin(), m_children.end(), [&id](const Widget::Ptr& ptr) { return ptr->id() == id; });

        return (it != m_children.end()) ? it->get() : nullptr;
    }

    /**
     * @brief 通过ID查找子组件（别名，与文档一致）
     */
    [[nodiscard]] Widget* findChild(const std::string& id) { return findWidgetById(id); }

    [[nodiscard]] const Widget* findWidgetById(const std::string& id) const override
    {
        auto it = std::find_if(
            m_children.begin(), m_children.end(), [&id](const Widget::Ptr& ptr) { return ptr->id() == id; });

        return (it != m_children.end()) ? it->get() : nullptr;
    }

    [[nodiscard]] const Widget* findChild(const std::string& id) const { return findWidgetById(id); }

    [[nodiscard]] Widget* getWidgetAt(i32 x, i32 y) override
    {
        // 从后往前遍历（后添加的在上面）
        for (auto it = m_children.rbegin(); it != m_children.rend(); ++it) {
            Widget* widget = it->get();
            if (widget->isVisible() && widget->isActive() && widget->contains(x, y)) {
                return widget;
            }
        }
        return nullptr;
    }

    [[nodiscard]] const Widget* getWidgetAt(i32 x, i32 y) const override
    {
        for (auto it = m_children.rbegin(); it != m_children.rend(); ++it) {
            const Widget* widget = it->get();
            if (widget->isVisible() && widget->isActive() && widget->contains(x, y)) {
                return widget;
            }
        }
        return nullptr;
    }

    void bringToFront(Widget* widget) override
    {
        MC_ASSERT_RELEASE(widget != nullptr);

        auto it = std::find_if(
            m_children.begin(), m_children.end(), [widget](const Widget::Ptr& ptr) { return ptr.get() == widget; });

        // 由于 getWidgetAt 从后往前遍历，"最前面"应该是列表末尾
        if (it != m_children.end() && it != m_children.end() - 1) {
            auto ptr = std::move(*it);
            m_children.erase(it);
            m_children.push_back(std::move(ptr));
        }
    }

    void sendToBack(Widget* widget) override
    {
        MC_ASSERT_RELEASE(widget != nullptr);

        auto it = std::find_if(
            m_children.begin(), m_children.end(), [widget](const Widget::Ptr& ptr) { return ptr.get() == widget; });

        // 由于 getWidgetAt 从后往前遍历，"最后面"应该是列表开头
        if (it != m_children.end() && it != m_children.begin()) {
            auto ptr = std::move(*it);
            m_children.erase(it);
            m_children.insert(m_children.begin(), std::move(ptr));
        }
    }

    void forEachWidget(const std::function<void(Widget&)>& callback) override
    {
        for (auto& child : m_children) {
            callback(*child);
        }
    }

    void forEachWidget(const std::function<void(const Widget&)>& callback) const override
    {
        for (const auto& child : m_children) {
            callback(*child);
        }
    }

    // ========== 公共焦点管理接口 ==========

    /**
     * @brief 设置焦点组件
     * @param widget 要聚焦的组件（nullptr清除焦点）
     */
    void setFocusedWidget(Widget* widget)
    {
        if (m_focusedWidget != widget) {
            if (m_focusedWidget != nullptr) {
                m_focusedWidget->setFocused(false);
                m_focusedWidget->onFocusLost();
            }
            m_focusedWidget = widget;
            if (widget != nullptr) {
                widget->setFocused(true);
                widget->onFocusGained();
            }
        }
    }

    /**
     * @brief 获取当前焦点组件
     */
    [[nodiscard]] Widget* getFocusedWidget() { return m_focusedWidget; }
    [[nodiscard]] const Widget* getFocusedWidget() const { return m_focusedWidget; }

    /**
     * @brief 清除焦点
     */
    void clearFocus() { setFocusedWidget(nullptr); }

    /**
     * @brief 将焦点移动到下一个可聚焦的子组件（Tab导航）
     * @return 如果成功移动焦点返回true
     */
    bool focusNext()
    {
        // 如果没有焦点，聚焦第一个可聚焦的组件
        if (m_focusedWidget == nullptr) {
            for (auto& child : m_children) {
                if (child->isVisible() && child->isActive()) {
                    setFocusedWidget(child.get());
                    return true;
                }
            }
            return false;
        }

        // 找到当前焦点组件的索引，然后找下一个
        for (size_t i = 0; i < m_children.size(); ++i) {
            if (m_children[i].get() == m_focusedWidget) {
                for (size_t j = i + 1; j < m_children.size(); ++j) {
                    if (m_children[j]->isVisible() && m_children[j]->isActive()) {
                        setFocusedWidget(m_children[j].get());
                        return true;
                    }
                }
                break;
            }
        }
        return false;
    }

    /**
     * @brief 将焦点移动到上一个可聚焦的子组件（Shift+Tab导航）
     * @return 如果成功移动焦点返回true
     */
    bool focusPrevious()
    {
        // 如果没有焦点，聚焦最后一个可聚焦的组件
        if (m_focusedWidget == nullptr) {
            for (auto it = m_children.rbegin(); it != m_children.rend(); ++it) {
                if ((*it)->isVisible() && (*it)->isActive()) {
                    setFocusedWidget(it->get());
                    return true;
                }
            }
            return false;
        }

        // 找到当前焦点组件的索引，然后找上一个
        for (size_t i = 0; i < m_children.size(); ++i) {
            if (m_children[i].get() == m_focusedWidget) {
                for (size_t j = i; j > 0; --j) {
                    if (m_children[j - 1]->isVisible() && m_children[j - 1]->isActive()) {
                        setFocusedWidget(m_children[j - 1].get());
                        return true;
                    }
                }
                break;
            }
        }
        return false;
    }

protected:
    /**
     * @brief 绘制所有子组件
     * @param ctx 绘图上下文
     */
    void paintChildren(PaintContext& ctx)
    {
        for (auto& child : m_children) {
            if (child->isVisible()) {
                child->paint(ctx);
            }
        }
    }

    /**
     * @brief 更新所有子组件
     * @param dt 增量时间
     */
    void tickChildren(f32 dt)
    {
        for (auto& child : m_children) {
            if (child->isVisible() && child->isActive()) {
                child->tick(dt);
            }
        }
    }

    /**
     * @brief 处理子组件的点击事件
     * @param mouseX 鼠标X坐标
     * @param mouseY 鼠标Y坐标
     * @param button 鼠标按钮
     * @param mods 修饰键位掩码 (GLFW_MOD_SHIFT, GLFW_MOD_CONTROL 等)
     * @return 如果有组件处理了事件返回true
     */
    bool handleClickInChildren(i32 mouseX, i32 mouseY, i32 button, i32 mods)
    {
        Widget* widget = getWidgetAt(mouseX, mouseY);
        if (widget != nullptr) {
            // 点击时自动设置焦点
            setFocusedWidget(widget);
            return widget->onClick(mouseX, mouseY, button, mods);
        }
        // 点击空白区域清除焦点
        setFocusedWidget(nullptr);
        return false;
    }

    /**
     * @brief 处理子组件的释放事件
     * @param mouseX 鼠标X坐标
     * @param mouseY 鼠标Y坐标
     * @param button 鼠标按钮
     * @param mods 修饰键位掩码 (GLFW_MOD_SHIFT, GLFW_MOD_CONTROL 等)
     * @return 如果有组件处理了事件返回true
     */
    bool handleReleaseInChildren(i32 mouseX, i32 mouseY, i32 button, i32 mods)
    {
        Widget* widget = getWidgetAt(mouseX, mouseY);
        if (widget != nullptr) {
            return widget->onRelease(mouseX, mouseY, button, mods);
        }
        return false;
    }

    /**
     * @brief 处理子组件的双击事件
     * @param mouseX 鼠标X坐标
     * @param mouseY 鼠标Y坐标
     * @param button 鼠标按钮
     * @param mods 修饰键位掩码
     * @return 如果有组件处理了事件返回true
     */
    bool handleDoubleClickInChildren(i32 mouseX, i32 mouseY, i32 button, i32 mods)
    {
        Widget* widget = getWidgetAt(mouseX, mouseY);
        if (widget != nullptr) {
            return widget->onDoubleClick(mouseX, mouseY, button, mods);
        }
        return false;
    }

    /**
     * @brief 处理子组件的右键点击事件
     * @param mouseX 鼠标X坐标
     * @param mouseY 鼠标Y坐标
     * @param mods 修饰键位掩码
     * @return 如果有组件处理了事件返回true
     */
    bool handleRightClickInChildren(i32 mouseX, i32 mouseY, i32 mods)
    {
        Widget* widget = getWidgetAt(mouseX, mouseY);
        if (widget != nullptr) {
            return widget->onRightClick(mouseX, mouseY, mods);
        }
        return false;
    }

    /**
     * @brief 处理子组件的拖动事件
     * @param mouseX 鼠标X坐标
     * @param mouseY 鼠标Y坐标
     * @param deltaX X轴移动量
     * @param deltaY Y轴移动量
     * @param button 触发拖动的鼠标按钮
     * @return 如果有组件处理了事件返回true
     */
    bool handleDragInChildren(i32 mouseX, i32 mouseY, i32 deltaX, i32 deltaY, i32 button)
    {
        // 拖动事件发送到悬停的组件
        for (auto it = m_children.rbegin(); it != m_children.rend(); ++it) {
            Widget* widget = it->get();
            if (widget->isVisible() && widget->isActive() && widget->isHovered()) {
                if (widget->onDrag(mouseX, mouseY, deltaX, deltaY, button)) {
                    return true;
                }
            }
        }
        return false;
    }

    /**
     * @brief 处理子组件的拖拽开始事件
     *
     * 将 onDragStart 分发到命中的子组件（与 handleClickInChildren 一致，
     * 通过 getWidgetAt 查找命中目标，自动设置焦点）。
     *
     * @param mouseX 鼠标X坐标
     * @param mouseY 鼠标Y坐标
     * @param button 触发拖拽的鼠标按钮
     * @param mods 修饰键位掩码
     * @return 如果有组件处理了事件返回true
     */
    bool handleDragStartInChildren(i32 mouseX, i32 mouseY, i32 button, i32 mods)
    {
        Widget* widget = getWidgetAt(mouseX, mouseY);
        if (widget != nullptr) {
            return widget->onDragStart(mouseX, mouseY, button, mods);
        }
        return false;
    }

    /**
     * @brief 处理子组件的拖拽结束事件
     *
     * 将 onDragEnd 分发到命中的子组件。若已知拖拽目标（draggingWidget），
     * 优先分发到该组件，否则按命中位置查找。
     *
     * @param mouseX 鼠标X坐标
     * @param mouseY 鼠标Y坐标
     * @param button 结束拖拽的鼠标按钮
     * @param dropped 是否被丢弃（焦点丢失/外部取消）
     * @param draggingWidget 当前拖拽中的子组件（可选，nullptr 时按命中位置查找）
     * @return 如果有组件处理了事件返回true
     */
    bool handleDragEndInChildren(i32 mouseX, i32 mouseY, i32 button, bool dropped, Widget* draggingWidget = nullptr)
    {
        if (draggingWidget != nullptr) {
            // 优先分发到已知的拖拽目标，避免拖拽过程中鼠标移出组件区域后丢失事件
            if (draggingWidget->isVisible() && draggingWidget->isActive()) {
                return draggingWidget->onDragEnd(mouseX, mouseY, button, dropped);
            }
            return false;
        }
        Widget* widget = getWidgetAt(mouseX, mouseY);
        if (widget != nullptr) {
            return widget->onDragEnd(mouseX, mouseY, button, dropped);
        }
        return false;
    }

    /**
     * @brief 处理子组件的滚轮事件
     * @param mouseX 鼠标X坐标
     * @param mouseY 鼠标Y坐标
     * @param delta 滚动量
     * @return 如果有组件处理了事件返回true
     */
    bool handleScrollInChildren(i32 mouseX, i32 mouseY, f64 delta)
    {
        Widget* widget = getWidgetAt(mouseX, mouseY);
        if (widget != nullptr) {
            return widget->onScroll(mouseX, mouseY, delta);
        }
        return false;
    }

    /**
     * @brief 处理子组件的键盘事件（发送到聚焦的组件）
     * @param key 键码
     * @param scanCode 扫描码
     * @param action 动作（按下/释放/重复）
     * @param mods 修饰键
     * @return 如果有组件处理了事件返回true
     */
    bool handleKeyInChildren(i32 key, i32 scanCode, i32 action, i32 mods)
    {
        if (m_focusedWidget != nullptr && m_focusedWidget->isVisible() && m_focusedWidget->isActive()) {
            return m_focusedWidget->onKey(key, scanCode, action, mods);
        }
        return false;
    }

    /**
     * @brief 处理子组件的字符输入事件（发送到聚焦的组件）
     * @param codePoint Unicode码点
     * @return 如果有组件处理了事件返回true
     */
    bool handleCharInChildren(u32 codePoint)
    {
        if (m_focusedWidget != nullptr && m_focusedWidget->isVisible() && m_focusedWidget->isActive()) {
            return m_focusedWidget->onChar(codePoint);
        }
        return false;
    }

    std::vector<Widget::Ptr> m_children;

private:
    Widget* m_focusedWidget = nullptr;
};

} // namespace mc::client::ui::kagero::widget
