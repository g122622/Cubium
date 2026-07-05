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
#include "client/ui/kagero/widget/CheckboxWidget.hpp"
#include "client/ui/kagero/widget/SliderWidget.hpp"
#include "client/ui/kagero/widget/TextFieldWidget.hpp"
#include "common/input/KeyBinding.hpp"
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

    _registerClickEvents();
    _registerHoverEvents();
    _registerFocusEvents();
    _registerKeyEvents();
    _registerValueEvents();
    _registerDragEvents();
    _registerScrollEvents();

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

void BuiltinEvents::_registerClickEvents()
{
    // click事件
    m_handlers[event_names::CLICK] = [](widget::Widget* widget, const event::Event& event) {
        if (widget && widget->isActive() && widget->isVisible()) {
            if (auto* clickEvent = dynamic_cast<const event::MouseClickEvent*>(&event)) {
                widget->onClick(clickEvent->x(), clickEvent->y(), clickEvent->button(), clickEvent->mods());
            }
        }
    };
    m_eventTypes[event_names::CLICK] = event::EventType::MouseClick;

    // doubleClick事件
    m_handlers[event_names::DOUBLE_CLICK] = [](widget::Widget* widget, const event::Event& event) {
        if (widget && widget->isActive() && widget->isVisible()) {
            if (auto* clickEvent = dynamic_cast<const event::MouseClickEvent*>(&event)) {
                widget->onDoubleClick(clickEvent->x(), clickEvent->y(), clickEvent->button(), clickEvent->mods());
            }
        }
    };
    m_eventTypes[event_names::DOUBLE_CLICK] = event::EventType::MouseClick;

    // rightClick事件
    m_handlers[event_names::RIGHT_CLICK] = [](widget::Widget* widget, const event::Event& event) {
        if (widget && widget->isActive() && widget->isVisible()) {
            if (auto* clickEvent = dynamic_cast<const event::MouseClickEvent*>(&event)) {
                widget->onRightClick(clickEvent->x(), clickEvent->y(), clickEvent->mods());
            }
        }
    };
    m_eventTypes[event_names::RIGHT_CLICK] = event::EventType::MouseClick;

    // mouseDown事件
    m_handlers[event_names::MOUSE_DOWN] = [](widget::Widget* widget, const event::Event& event) {
        if (widget && widget->isActive() && widget->isVisible()) {
            if (auto* clickEvent = dynamic_cast<const event::MouseClickEvent*>(&event)) {
                widget->onClick(clickEvent->x(), clickEvent->y(), clickEvent->button(), clickEvent->mods());
            }
        }
    };
    m_eventTypes[event_names::MOUSE_DOWN] = event::EventType::MouseClick;

    // mouseUp事件
    m_handlers[event_names::MOUSE_UP] = [](widget::Widget* widget, const event::Event& event) {
        if (widget && widget->isActive() && widget->isVisible()) {
            if (auto* releaseEvent = dynamic_cast<const event::MouseReleaseEvent*>(&event)) {
                widget->onRelease(releaseEvent->x(), releaseEvent->y(), releaseEvent->button(), releaseEvent->mods());
            }
        }
    };
    m_eventTypes[event_names::MOUSE_UP] = event::EventType::MouseRelease;
}

void BuiltinEvents::_registerHoverEvents()
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

void BuiltinEvents::_registerFocusEvents()
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

void BuiltinEvents::_registerKeyEvents()
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

void BuiltinEvents::_registerValueEvents()
{
    // change事件：值变化通知
    //
    // 值变化事件由具体 Widget 在内部状态变化时主动发出（例如 CheckboxWidget 在
    // onClick 中触发 m_onChanged、SliderWidget 在 setValue 中触发 m_onValueChanged、
    // TextFieldWidget 在 onChar/onKey 中触发 m_onTextChanged）。
    //
    // 模板系统通过 TemplateInstance::registerDefaultEventBinders 中的 "change" 绑定器
    // 将这些回调桥接到模板回调（ctx.invokeCallback），这是实际生效的路径。
    //
    // 此处注册的 handler 仅用于 BuiltinEvents::handle() 直接分发场景（当前未被调用），
    // 按 Widget 类型做最小分发示意，不重复注册回调以避免覆盖模板绑定。
    m_handlers[event_names::CHANGE] = [](widget::Widget* widget, const event::Event& event) {
        if (!widget || !widget->isActive() || !widget->isVisible()) {
            return;
        }
        (void)event;
        // 值变化回调已由 TemplateInstance 的 change 绑定器通过 setOnChanged/
        // setOnValueChanged/setTextChangedCallback 注册，此处仅作为类型分发占位。
        if (dynamic_cast<widget::CheckboxWidget*>(widget) != nullptr) {
            // CheckboxWidget：值变化由 onClick 内部触发
        } else if (dynamic_cast<widget::SliderWidget*>(widget) != nullptr) {
            // SliderWidget：值变化由 setValue/onDrag 内部触发
        } else if (dynamic_cast<widget::TextFieldWidget*>(widget) != nullptr) {
            // TextFieldWidget：值变化由 onChar/onKey 内部触发
        }
    };
    m_eventTypes[event_names::CHANGE] = event::EventType::ValueChange;

    // input事件：文本输入通知（仅 TextFieldWidget）
    //
    // 与 change 事件类似，文本输入回调由 TemplateInstance 的 "input" 绑定器通过
    // setTextChangedCallback 注册，此处仅作为事件分发占位。
    m_handlers[event_names::INPUT] = [](widget::Widget* widget, const event::Event& event) {
        if (!widget || !widget->isActive() || !widget->isVisible()) {
            return;
        }
        (void)event;
        if (dynamic_cast<widget::TextFieldWidget*>(widget) != nullptr) {
            // TextFieldWidget：文本输入由 onChar 内部触发
        }
    };
    m_eventTypes[event_names::INPUT] = event::EventType::TextChange;
}

void BuiltinEvents::_registerDragEvents()
{
    // drag事件
    m_handlers[event_names::DRAG] = [](widget::Widget* widget, const event::Event& event) {
        if (widget && widget->isActive() && widget->isVisible()) {
            if (auto* dragEvent = dynamic_cast<const event::MouseDragEvent*>(&event)) {
                widget->onDrag(
                    dragEvent->x(), dragEvent->y(), dragEvent->deltaX(), dragEvent->deltaY(), dragEvent->button());
            }
        }
    };
    m_eventTypes[event_names::DRAG] = event::EventType::MouseDrag;

    // dragStart事件：拖拽开始，调用 Widget::onDragStart
    // 与 KageroEngine::handleClick 中的 onDragStart 调用对应。DragStartEvent 当前
    // 仅携带 x/y，button 与 mods 通过默认值传入（直接分发场景下需要事件对象扩展字段，
    // 当前 KageroEngine 走同步调用路径，不经过此 handler）。
    m_handlers[event_names::DRAG_START] = [](widget::Widget* widget, const event::Event& event) {
        if (widget && widget->isActive() && widget->isVisible()) {
            if (auto* dragEvent = dynamic_cast<const event::DragStartEvent*>(&event)) {
                widget->onDragStart(dragEvent->x(), dragEvent->y(), /*button=*/0, /*mods=*/0);
            }
        }
    };
    m_eventTypes[event_names::DRAG_START] = event::EventType::Custom;

    // dragEnd事件：拖拽结束，调用 Widget::onDragEnd
    // 与 KageroEngine::handleRelease 中的 onDragEnd 调用对应。DragEndEvent 携带
    // wasDropped 标志，button 通过默认值传入。
    m_handlers[event_names::DRAG_END] = [](widget::Widget* widget, const event::Event& event) {
        if (widget && widget->isActive() && widget->isVisible()) {
            if (auto* dragEvent = dynamic_cast<const event::DragEndEvent*>(&event)) {
                widget->onDragEnd(dragEvent->x(), dragEvent->y(), /*button=*/0, dragEvent->wasDropped());
            }
        }
    };
    m_eventTypes[event_names::DRAG_END] = event::EventType::Custom;
}

void BuiltinEvents::_registerScrollEvents()
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

event::MouseClickEvent createClickEvent(i32 x, i32 y, i32 button, i32 clicks, i32 mods)
{
    return event::MouseClickEvent(x, y, button, clicks, mods);
}

event::MouseReleaseEvent createReleaseEvent(i32 x, i32 y, i32 button, i32 mods)
{
    return event::MouseReleaseEvent(x, y, button, mods);
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
    static const std::unordered_map<std::string, i32> keyMap = {
        {"unknown", Keys::Unknown},
        {"space", Keys::Space},
        {"apostrophe", Keys::Apostrophe},
        {"comma", Keys::Comma},
        {"minus", Keys::Minus},
        {"period", Keys::Period},
        {"slash", Keys::Slash},
        {"semicolon", Keys::Semicolon},
        {"equal", Keys::Equal},
        {"enter", Keys::Enter},
        {"tab", Keys::Tab},
        {"backspace", Keys::Backspace},
        {"insert", Keys::Insert},
        {"delete", Keys::Delete},
        {"right", Keys::Right},
        {"left", Keys::Left},
        {"down", Keys::Down},
        {"up", Keys::Up},
        {"page_up", Keys::PageUp},
        {"page_down", Keys::PageDown},
        {"home", Keys::Home},
        {"end", Keys::End},
        {"caps_lock", Keys::CapsLock},
        {"scroll_lock", Keys::ScrollLock},
        {"num_lock", Keys::NumLock},
        {"print_screen", Keys::PrintScreen},
        {"pause", Keys::Pause},
        {"escape", Keys::Escape},
        {"f1", Keys::F1},
        {"f2", Keys::F2},
        {"f3", Keys::F3},
        {"f4", Keys::F4},
        {"f5", Keys::F5},
        {"f6", Keys::F6},
        {"f7", Keys::F7},
        {"f8", Keys::F8},
        {"f9", Keys::F9},
        {"f10", Keys::F10},
        {"f11", Keys::F11},
        {"f12", Keys::F12},
        {"f13", Keys::F13},
        {"f14", Keys::F14},
        {"f15", Keys::F15},
        {"f16", Keys::F16},
        {"f17", Keys::F17},
        {"f18", Keys::F18},
        {"f19", Keys::F19},
        {"f20", Keys::F20},
        {"f21", Keys::F21},
        {"f22", Keys::F22},
        {"f23", Keys::F23},
        {"f24", Keys::F24},
        {"f25", Keys::F25},
        {"kp_0", Keys::KP_0},
        {"kp_1", Keys::KP_1},
        {"kp_2", Keys::KP_2},
        {"kp_3", Keys::KP_3},
        {"kp_4", Keys::KP_4},
        {"kp_5", Keys::KP_5},
        {"kp_6", Keys::KP_6},
        {"kp_7", Keys::KP_7},
        {"kp_8", Keys::KP_8},
        {"kp_9", Keys::KP_9},
        {"kp_decimal", Keys::KP_Decimal},
        {"kp_divide", Keys::KP_Divide},
        {"kp_multiply", Keys::KP_Multiply},
        {"kp_subtract", Keys::KP_Subtract},
        {"kp_add", Keys::KP_Add},
        {"kp_enter", Keys::KP_Enter},
        {"kp_equal", Keys::KP_Equal},
        {"left_shift", Keys::LeftShift},
        {"left_control", Keys::LeftControl},
        {"left_alt", Keys::LeftAlt},
        {"left_super", Keys::LeftSuper},
        {"right_shift", Keys::RightShift},
        {"right_control", Keys::RightControl},
        {"right_alt", Keys::RightAlt},
        {"right_super", Keys::RightSuper},
        {"left_bracket", Keys::LeftBracket},
        {"backslash", Keys::Backslash},
        {"right_bracket", Keys::RightBracket},
        {"grave_accent", Keys::GraveAccent},
        {"world_1", Keys::World1},
        {"world_2", Keys::World2},
        {"menu", Keys::Menu},
    };

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

    return Keys::Unknown;
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

    if (lower.find("shift") != std::string::npos) result |= static_cast<i32>(KeyMods::Shift);
    if (lower.find("ctrl") != std::string::npos || lower.find("control") != std::string::npos)
        result |= static_cast<i32>(KeyMods::Control);
    if (lower.find("alt") != std::string::npos) result |= static_cast<i32>(KeyMods::Alt);
    if (lower.find("super") != std::string::npos || lower.find("meta") != std::string::npos)
        result |= static_cast<i32>(KeyMods::Super);
    if (lower.find("caps") != std::string::npos) result |= static_cast<i32>(KeyMods::CapsLock);
    if (lower.find("num") != std::string::npos) result |= static_cast<i32>(KeyMods::NumLock);

    return result;
}

} // namespace event_utils

} // namespace mc::client::ui::kagero::tpl::bindings
