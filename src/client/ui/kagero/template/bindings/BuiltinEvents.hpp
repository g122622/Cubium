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

#include "client/ui/kagero/event/Event.hpp"
#include "client/ui/kagero/event/EventBus.hpp"
#include "client/ui/kagero/event/InputEvents.hpp"
#include "client/ui/kagero/event/UIEvents.hpp"
#include "client/ui/kagero/event/WidgetEvents.hpp"
#include "client/ui/kagero/widget/Widget.hpp"
#include "common/core/Types.hpp"
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

namespace mc::client::ui::kagero::tpl::bindings {

/**
 * @brief 事件处理器类型
 */
using EventHandler = std::function<void(widget::Widget*, const event::Event&)>;

/**
 * @brief 内置事件注册
 *
 * 管理所有内置事件类型的处理器注册和分发。
 */
class BuiltinEvents {
public:
    /**
     * @brief 获取单例实例
     */
    static BuiltinEvents& instance();

    /**
     * @brief 初始化所有内置事件
     */
    void initialize();

    /**
     * @brief 注册事件处理器
     *
     * @param eventName 事件名（如 "click", "hover"）
     * @param handler 处理函数
     */
    void registerHandler(const std::string& eventName, EventHandler handler);

    /**
     * @brief 处理事件
     *
     * @param widget 目标Widget
     * @param eventName 事件名
     * @param event 事件对象
     * @return 是否处理成功
     */
    bool handle(widget::Widget* widget, const std::string& eventName, const event::Event& event);

    /**
     * @brief 检查事件名是否注册
     */
    [[nodiscard]] bool hasEvent(const std::string& eventName) const;

    /**
     * @brief 获取所有注册的事件名
     */
    [[nodiscard]] std::vector<std::string> registeredEvents() const;

    /**
     * @brief 获取事件类型
     *
     * @param eventName 事件名
     * @return 事件类型，未知事件返回 EventType::Custom
     */
    [[nodiscard]] event::EventType getEventType(const std::string& eventName) const;

private:
    BuiltinEvents() = default;
    ~BuiltinEvents() = default;

    // 禁止拷贝
    BuiltinEvents(const BuiltinEvents&) = delete;
    BuiltinEvents& operator=(const BuiltinEvents&) = delete;

    void _registerClickEvents();
    void _registerHoverEvents();
    void _registerFocusEvents();
    void _registerKeyEvents();
    void _registerValueEvents();
    void _registerDragEvents();
    void _registerScrollEvents();

    std::unordered_map<std::string, EventHandler> m_handlers;
    std::unordered_map<std::string, event::EventType> m_eventTypes;
    bool m_initialized = false;
};

/**
 * @brief 事件名称常量
 */
namespace event_names {
// 鼠标事件
constexpr const char* CLICK = "click";
constexpr const char* DOUBLE_CLICK = "doubleClick";
constexpr const char* RIGHT_CLICK = "rightClick";
constexpr const char* MOUSE_DOWN = "mouseDown";
constexpr const char* MOUSE_UP = "mouseUp";
constexpr const char* MOUSE_ENTER = "mouseEnter";
constexpr const char* MOUSE_LEAVE = "mouseLeave";
constexpr const char* MOUSE_MOVE = "mouseMove";
constexpr const char* DRAG = "drag";
constexpr const char* DRAG_START = "dragStart";
constexpr const char* DRAG_END = "dragEnd";
constexpr const char* SCROLL = "scroll";

// 键盘事件
constexpr const char* KEY_DOWN = "keyDown";
constexpr const char* KEY_UP = "keyUp";
constexpr const char* KEY_PRESS = "keyPress";
constexpr const char* CHAR_INPUT = "charInput";

// 焦点事件
constexpr const char* FOCUS = "focus";
constexpr const char* BLUR = "blur";

// 值变化事件
constexpr const char* CHANGE = "change";
constexpr const char* INPUT = "input";

// 生命周期事件
constexpr const char* INIT = "init";
constexpr const char* SHOW = "show";
constexpr const char* HIDE = "hide";
constexpr const char* RESIZE = "resize";

// 特殊事件
constexpr const char* SLOT_CLICK = "slotClick";
constexpr const char* SLOT_HOVER = "slotHover";
constexpr const char* SELECTION_CHANGE = "selectionChange";
} // namespace event_names

/**
 * @brief 事件辅助工具
 */
namespace event_utils {

/**
 * @brief 从事件名推断事件类型
 */
[[nodiscard]] event::EventType inferEventType(const std::string& eventName);

/**
 * @brief 创建鼠标点击事件
 */
[[nodiscard]] event::MouseClickEvent createClickEvent(i32 x, i32 y, i32 button, i32 clicks, i32 mods = 0);

/**
 * @brief 创建鼠标释放事件
 */
[[nodiscard]] event::MouseReleaseEvent createReleaseEvent(i32 x, i32 y, i32 button, i32 mods = 0);

/**
 * @brief 创建鼠标拖动事件
 */
[[nodiscard]] event::MouseDragEvent createDragEvent(i32 x, i32 y, i32 deltaX, i32 deltaY, i32 button);

/**
 * @brief 创建拖拽开始事件
 *
 * @param x 屏幕 X 坐标
 * @param y 屏幕 Y 坐标
 * @param button 触发拖拽的鼠标按钮（0=左键，1=右键，2=中键）
 * @param mods 拖拽开始时的修饰键状态（KeyMods 位掩码）
 */
[[nodiscard]] event::DragStartEvent createDragStartEvent(i32 x, i32 y, i32 button, i32 mods = 0);

/**
 * @brief 创建拖拽结束事件
 *
 * @param x 屏幕 X 坐标
 * @param y 屏幕 Y 坐标
 * @param button 触发拖拽的鼠标按钮（0=左键，1=右键，2=中键）
 * @param dropped 是否被外部取消（焦点丢失等）
 */
[[nodiscard]] event::DragEndEvent createDragEndEvent(i32 x, i32 y, i32 button, bool dropped = false);

/**
 * @brief 创建鼠标滚轮事件
 */
[[nodiscard]] event::MouseScrollEvent createScrollEvent(i32 x, i32 y, f64 deltaX, f64 deltaY);

/**
 * @brief 创建键盘事件
 */
[[nodiscard]] event::KeyEvent createKeyEvent(i32 key, i32 scanCode, i32 action, i32 mods);

/**
 * @brief 创建字符输入事件
 */
[[nodiscard]] event::CharInputEvent createCharInputEvent(u32 codePoint);

/**
 * @brief 创建值变化事件
 */
template <typename T>
[[nodiscard]] event::ValueChangeEvent<T> createValueChangeEvent(const T& oldValue, const T& newValue);

/**
 * @brief 从字符串解析键码
 */
[[nodiscard]] i32 parseKeyCode(const std::string& keyName);

/**
 * @brief 从字符串解析鼠标按钮
 */
[[nodiscard]] i32 parseMouseButton(const std::string& buttonName);

/**
 * @brief 从字符串解析修饰键
 */
[[nodiscard]] i32 parseKeyMods(const std::string& mods);

} // namespace event_utils

/**
 * @brief 初始化内置事件处理器
 *
 * 调用 BuiltinEvents::instance().initialize()
 */
inline void registerBuiltinEvents()
{
    BuiltinEvents::instance().initialize();
}

} // namespace mc::client::ui::kagero::tpl::bindings
