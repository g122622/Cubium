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

#include "BuiltinEvents.hpp"
#include <algorithm>
#include <cctype>

namespace mc::client::ui::kagero::tpl::bindings {

// ========== BuiltinEvents实现 ==========

BuiltinEvents& BuiltinEvents::instance()
{
    static BuiltinEvents instance;
    return instance;
}

void BuiltinEvents::initialize()
{
    if (m_initialized) return;

    registerClickEvents();
    registerHoverEvents();
    registerFocusEvents();
    registerKeyEvents();
    registerValueEvents();
    registerDragEvents();
    registerScrollEvents();

    m_initialized = true;
}

void BuiltinEvents::registerHandler(const std::string& eventName, EventHandler handler)
{
    m_handlers[eventName] = std::move(handler);
}

bool BuiltinEvents::handle(widget::Widget* widget, const std::string& eventName, const event::Event& event)
{
    auto it = m_handlers.find(eventName);
    if (it == m_handlers.end()) {
        return false;
    }

    it->second(widget, event);
    return true;
}

bool BuiltinEvents::hasEvent(const std::string& eventName) const
{
    return m_handlers.find(eventName) != m_handlers.end();
}

std::vector<std::string> BuiltinEvents::registeredEvents() const
{
    std::vector<std::string> events;
    events.reserve(m_handlers.size());
    for (const auto& [name, handler] : m_handlers) {
        events.push_back(name);
    }
    return events;
}

event::EventType BuiltinEvents::getEventType(const std::string& eventName) const
{
    auto it = m_eventTypes.find(eventName);
    return it != m_eventTypes.end() ? it->second : event::EventType::Custom;
}

void BuiltinEvents::registerClickEvents()
{
    // click事件
    m_handlers[event_names::CLICK] = [](widget::Widget* widget, const event::Event& event) {
        if (widget && widget->isActive() && widget->isVisible()) {
            if (auto* clickEvent = dynamic_cast<const event::MouseClickEvent*>(&event)) {
                widget->onClick(clickEvent->x(), clickEvent->y(), clickEvent->button());
            }
        }
    };
    m_eventTypes[event_names::CLICK] = event::EventType::MouseClick;

    // doubleClick事件
    m_handlers[event_names::DOUBLE_CLICK] = [](widget::Widget* widget, const event::Event& event) {
        if (widget && widget->isActive() && widget->isVisible()) {
            if (auto* clickEvent = dynamic_cast<const event::MouseClickEvent*>(&event)) {
                widget->onClick(clickEvent->x(), clickEvent->y(), clickEvent->button());
            }
        }
    };
    m_eventTypes[event_names::DOUBLE_CLICK] = event::EventType::MouseClick;

    // rightClick事件
    m_handlers[event_names::RIGHT_CLICK] = [](widget::Widget* widget, const event::Event& event) {
        if (widget && widget->isActive() && widget->isVisible()) {
            if (auto* clickEvent = dynamic_cast<const event::MouseClickEvent*>(&event)) {
                widget->onClick(clickEvent->x(), clickEvent->y(), 1); // 右键
            }
        }
    };
    m_eventTypes[event_names::RIGHT_CLICK] = event::EventType::MouseClick;

    // mouseDown事件
    m_handlers[event_names::MOUSE_DOWN] = [](widget::Widget* widget, const event::Event& event) {
        if (widget && widget->isActive() && widget->isVisible()) {
            if (auto* clickEvent = dynamic_cast<const event::MouseClickEvent*>(&event)) {
                widget->onClick(clickEvent->x(), clickEvent->y(), clickEvent->button());
            }
        }
    };
    m_eventTypes[event_names::MOUSE_DOWN] = event::EventType::MouseClick;

    // mouseUp事件
    m_handlers[event_names::MOUSE_UP] = [](widget::Widget* widget, const event::Event& event) {
        if (widget && widget->isActive() && widget->isVisible()) {
            if (auto* releaseEvent = dynamic_cast<const event::MouseReleaseEvent*>(&event)) {
                widget->onRelease(releaseEvent->x(), releaseEvent->y(), releaseEvent->button());
            }
        }
    };
    m_eventTypes[event_names::MOUSE_UP] = event::EventType::MouseRelease;
}

void BuiltinEvents::registerHoverEvents()
{
    // mouseEnter事件
    m_handlers[event_names::MOUSE_ENTER] = [](widget::Widget* widget, const event::Event&) {
        if (widget) {
            widget->setHovered(true);
            widget->onMouseEnter();
        }
    };
    m_eventTypes[event_names::MOUSE_ENTER] = event::EventType::MouseEnter;

    // mouseLeave事件
    m_handlers[event_names::MOUSE_LEAVE] = [](widget::Widget* widget, const event::Event&) {
        if (widget) {
            widget->setHovered(false);
            widget->onMouseLeave();
        }
    };
    m_eventTypes[event_names::MOUSE_LEAVE] = event::EventType::MouseLeave;

    // mouseMove事件
    m_handlers[event_names::MOUSE_MOVE] = [](widget::Widget* widget, const event::Event& event) {
        if (widget && widget->isActive() && widget->isVisible()) {
            if (auto* moveEvent = dynamic_cast<const event::MouseMoveEvent*>(&event)) {
                widget->updateHover(moveEvent->x(), moveEvent->y());
            }
        }
    };
    m_eventTypes[event_names::MOUSE_MOVE] = event::EventType::MouseMove;
}

void BuiltinEvents::registerFocusEvents()
{
    // focus事件
    m_handlers[event_names::FOCUS] = [](widget::Widget* widget, const event::Event&) {
        if (widget) {
            widget->setFocused(true);
            widget->onFocusGained();
        }
    };
    m_eventTypes[event_names::FOCUS] = event::EventType::FocusGained;

    // blur事件
    m_handlers[event_names::BLUR] = [](widget::Widget* widget, const event::Event&) {
        if (widget) {
            widget->setFocused(false);
            widget->onFocusLost();
        }
    };
    m_eventTypes[event_names::BLUR] = event::EventType::FocusLost;
}

void BuiltinEvents::registerKeyEvents()
{
    // keyDown事件
    m_handlers[event_names::KEY_DOWN] = [](widget::Widget* widget, const event::Event& event) {
        if (widget && widget->isActive() && widget->isVisible() && widget->isFocused()) {
            if (auto* keyEvent = dynamic_cast<const event::KeyEvent*>(&event)) {
                if (keyEvent->isPressed()) {
                    widget->onKey(keyEvent->key(), keyEvent->scanCode(), keyEvent->action(), keyEvent->mods());
                }
            }
        }
    };
    m_eventTypes[event_names::KEY_DOWN] = event::EventType::KeyPress;

    // keyUp事件
    m_handlers[event_names::KEY_UP] = [](widget::Widget* widget, const event::Event& event) {
        if (widget && widget->isActive() && widget->isVisible() && widget->isFocused()) {
            if (auto* keyEvent = dynamic_cast<const event::KeyEvent*>(&event)) {
                if (keyEvent->isReleased()) {
                    widget->onKey(keyEvent->key(), keyEvent->scanCode(), keyEvent->action(), keyEvent->mods());
                }
            }
        }
    };
    m_eventTypes[event_names::KEY_UP] = event::EventType::KeyRelease;

    // keyPress事件
    m_handlers[event_names::KEY_PRESS] = [](widget::Widget* widget, const event::Event& event) {
        if (widget && widget->isActive() && widget->isVisible() && widget->isFocused()) {
            if (auto* keyEvent = dynamic_cast<const event::KeyEvent*>(&event)) {
                widget->onKey(keyEvent->key(), keyEvent->scanCode(), keyEvent->action(), keyEvent->mods());
            }
        }
    };
    m_eventTypes[event_names::KEY_PRESS] = event::EventType::KeyPress;

    // charInput事件
    m_handlers[event_names::CHAR_INPUT] = [](widget::Widget* widget, const event::Event& event) {
        if (widget && widget->isActive() && widget->isVisible() && widget->isFocused()) {
            if (auto* charEvent = dynamic_cast<const event::CharInputEvent*>(&event)) {
                widget->onChar(charEvent->codePoint());
            }
        }
    };
    m_eventTypes[event_names::CHAR_INPUT] = event::EventType::CharInput;
}

void BuiltinEvents::registerValueEvents()
{
    // change事件
    m_handlers[event_names::CHANGE] = [](widget::Widget* widget, const event::Event&) {
        // 值变化处理，由具体Widget实现
        (void)widget;
    };
    m_eventTypes[event_names::CHANGE] = event::EventType::ValueChange;

    // input事件
    m_handlers[event_names::INPUT] = [](widget::Widget* widget, const event::Event&) {
        // 输入处理，由具体Widget实现
        (void)widget;
    };
    m_eventTypes[event_names::INPUT] = event::EventType::TextChange;
}

void BuiltinEvents::registerDragEvents()
{
    // drag事件
    m_handlers[event_names::DRAG] = [](widget::Widget* widget, const event::Event& event) {
        if (widget && widget->isActive() && widget->isVisible()) {
            if (auto* dragEvent = dynamic_cast<const event::MouseDragEvent*>(&event)) {
                widget->onDrag(dragEvent->x(), dragEvent->y(), dragEvent->deltaX(), dragEvent->deltaY());
            }
        }
    };
    m_eventTypes[event_names::DRAG] = event::EventType::MouseDrag;

    // dragStart事件
    m_handlers[event_names::DRAG_START] = [](widget::Widget* widget, const event::Event& event) {
        if (widget && widget->isActive() && widget->isVisible()) {
            if (auto* dragEvent = dynamic_cast<const event::DragStartEvent*>(&event)) {
                (void)dragEvent;
                // 开始拖拽处理
            }
        }
    };
    m_eventTypes[event_names::DRAG_START] = event::EventType::Custom;

    // dragEnd事件
    m_handlers[event_names::DRAG_END] = [](widget::Widget* widget, const event::Event& event) {
        if (widget && widget->isActive() && widget->isVisible()) {
            if (auto* dragEvent = dynamic_cast<const event::DragEndEvent*>(&event)) {
                (void)dragEvent;
                // 结束拖拽处理
            }
        }
    };
    m_eventTypes[event_names::DRAG_END] = event::EventType::Custom;
}

void BuiltinEvents::registerScrollEvents()
{
    // scroll事件
    m_handlers[event_names::SCROLL] = [](widget::Widget* widget, const event::Event& event) {
        if (widget && widget->isActive() && widget->isVisible()) {
            if (auto* scrollEvent = dynamic_cast<const event::MouseScrollEvent*>(&event)) {
                widget->onScroll(scrollEvent->x(), scrollEvent->y(), scrollEvent->deltaY());
            }
        }
    };
    m_eventTypes[event_names::SCROLL] = event::EventType::MouseScroll;
}

// ========== event_utils实现 ==========

namespace event_utils {

event::EventType inferEventType(const std::string& eventName)
{
    static const std::unordered_map<std::string, event::EventType> eventTypeMap = {
        {event_names::CLICK, event::EventType::MouseClick},
        {event_names::DOUBLE_CLICK, event::EventType::MouseClick},
        {event_names::RIGHT_CLICK, event::EventType::MouseClick},
        {event_names::MOUSE_DOWN, event::EventType::MouseClick},
        {event_names::MOUSE_UP, event::EventType::MouseRelease},
        {event_names::MOUSE_ENTER, event::EventType::MouseEnter},
        {event_names::MOUSE_LEAVE, event::EventType::MouseLeave},
        {event_names::MOUSE_MOVE, event::EventType::MouseMove},
        {event_names::DRAG, event::EventType::MouseDrag},
        {event_names::DRAG_START, event::EventType::Custom},
        {event_names::DRAG_END, event::EventType::Custom},
        {event_names::SCROLL, event::EventType::MouseScroll},
        {event_names::KEY_DOWN, event::EventType::KeyPress},
        {event_names::KEY_UP, event::EventType::KeyRelease},
        {event_names::KEY_PRESS, event::EventType::KeyPress},
        {event_names::CHAR_INPUT, event::EventType::CharInput},
        {event_names::FOCUS, event::EventType::FocusGained},
        {event_names::BLUR, event::EventType::FocusLost},
        {event_names::CHANGE, event::EventType::ValueChange},
        {event_names::INPUT, event::EventType::TextChange},
        {event_names::INIT, event::EventType::WidgetInit},
        {event_names::SHOW, event::EventType::WidgetShow},
        {event_names::HIDE, event::EventType::WidgetHide},
        {event_names::RESIZE, event::EventType::WidgetResize},
        {event_names::SLOT_CLICK, event::EventType::MouseClick},
        {event_names::SELECTION_CHANGE, event::EventType::ValueChange}};

    auto it = eventTypeMap.find(eventName);
    return it != eventTypeMap.end() ? it->second : event::EventType::Custom;
}

event::MouseClickEvent createClickEvent(i32 x, i32 y, i32 button, i32 clicks)
{
    return event::MouseClickEvent(x, y, button, clicks);
}

event::MouseReleaseEvent createReleaseEvent(i32 x, i32 y, i32 button)
{
    return event::MouseReleaseEvent(x, y, button);
}

event::MouseDragEvent createDragEvent(i32 x, i32 y, i32 deltaX, i32 deltaY, i32 button)
{
    return event::MouseDragEvent(x, y, deltaX, deltaY, button);
}

event::MouseScrollEvent createScrollEvent(i32 x, i32 y, f64 deltaX, f64 deltaY)
{
    return event::MouseScrollEvent(x, y, deltaX, deltaY);
}

event::KeyEvent createKeyEvent(i32 key, i32 scanCode, i32 action, i32 mods)
{
    return event::KeyEvent(key, scanCode, action, mods);
}

event::CharInputEvent createCharInputEvent(u32 codePoint)
{
    return event::CharInputEvent(codePoint);
}

template <typename T>
event::ValueChangeEvent<T> createValueChangeEvent(const T& oldValue, const T& newValue)
{
    return event::ValueChangeEvent<T>(oldValue, newValue);
}

// 显式实例化常用类型
template event::ValueChangeEvent<i32> createValueChangeEvent(const i32&, const i32&);
template event::ValueChangeEvent<f32> createValueChangeEvent(const f32&, const f32&);
template event::ValueChangeEvent<bool> createValueChangeEvent(const bool&, const bool&);
template event::ValueChangeEvent<std::string> createValueChangeEvent(const std::string&, const std::string&);

i32 parseKeyCode(const std::string& keyName)
{
    // 简化版本，只支持常用键
    static const std::unordered_map<std::string, i32> keyMap = {{"unknown", 0},
        {"space", 32},
        {"enter", 257},
        {"tab", 258},
        {"backspace", 259},
        {"insert", 260},
        {"delete", 261},
        {"right", 262},
        {"left", 263},
        {"down", 264},
        {"up", 265},
        {"page_up", 266},
        {"page_down", 267},
        {"home", 268},
        {"end", 269},
        {"caps_lock", 280},
        {"scroll_lock", 281},
        {"num_lock", 282},
        {"print_screen", 283},
        {"pause", 284},
        {"f1", 290},
        {"f2", 291},
        {"f3", 292},
        {"f4", 293},
        {"f5", 294},
        {"f6", 295},
        {"f7", 296},
        {"f8", 297},
        {"f9", 298},
        {"f10", 299},
        {"f11", 300},
        {"f12", 301},
        {"kp_0", 320},
        {"kp_1", 321},
        {"kp_2", 322},
        {"kp_3", 323},
        {"kp_4", 324},
        {"kp_5", 325},
        {"kp_6", 326},
        {"kp_7", 327},
        {"kp_8", 328},
        {"kp_9", 329},
        {"kp_decimal", 330},
        {"kp_divide", 331},
        {"kp_multiply", 332},
        {"kp_subtract", 333},
        {"kp_add", 334},
        {"kp_enter", 335},
        {"kp_equal", 336},
        {"left_shift", 340},
        {"left_control", 341},
        {"left_alt", 342},
        {"left_super", 343},
        {"right_shift", 344},
        {"right_control", 345},
        {"right_alt", 346},
        {"right_super", 347}};

    std::string lower = keyName;
    std::transform(
        lower.begin(), lower.end(), lower.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

    auto it = keyMap.find(lower);
    if (it != keyMap.end()) {
        return it->second;
    }

    // 单字符键
    if (keyName.size() == 1) {
        char c = static_cast<char>(std::toupper(static_cast<unsigned char>(keyName[0])));
        if (c >= 'A' && c <= 'Z') {
            return static_cast<i32>(c);
        }
        if (c >= '0' && c <= '9') {
            return static_cast<i32>(c);
        }
    }

    return 0; // 未知键
}

i32 parseMouseButton(const std::string& buttonName)
{
    std::string lower = buttonName;
    std::transform(
        lower.begin(), lower.end(), lower.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

    if (lower == "left" || lower == "0") return 0;
    if (lower == "right" || lower == "1") return 1;
    if (lower == "middle" || lower == "2") return 2;
    if (lower == "button4" || lower == "3") return 3;
    if (lower == "button5" || lower == "4") return 4;

    return 0;
}

i32 parseKeyMods(const std::string& mods)
{
    i32 result = 0;

    // 解析修饰键字符串，格式如 "shift+ctrl" 或 "alt"
    std::string lower = mods;
    std::transform(
        lower.begin(), lower.end(), lower.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

    if (lower.find("shift") != std::string::npos) result |= 0x01;
    if (lower.find("ctrl") != std::string::npos || lower.find("control") != std::string::npos) result |= 0x02;
    if (lower.find("alt") != std::string::npos) result |= 0x04;
    if (lower.find("super") != std::string::npos || lower.find("meta") != std::string::npos) result |= 0x08;
    if (lower.find("caps") != std::string::npos) result |= 0x10;
    if (lower.find("num") != std::string::npos) result |= 0x20;

    return result;
}

} // namespace event_utils

} // namespace mc::client::ui::kagero::tpl::bindings
