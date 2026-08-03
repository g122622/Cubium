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

#include "Event.hpp"
#include "client/ui/kagero/Types.hpp"
#include "common/core/Types.hpp"
#include "common/util/text/Utf8.hpp"
#include <string>

namespace mc::client::ui::kagero::event {

/**
 * @brief 鼠标点击事件
 */
class MouseClickEvent : public Event {
public:
    MouseClickEvent(i32 x, i32 y, i32 button, i32 clicks, i32 mods = 0)
        : m_x(x)
        , m_y(y)
        , m_button(button)
        , m_clicks(clicks)
        , m_mods(mods)
    {}

    [[nodiscard]] EventType getType() const override { return EventType::MouseClick; }
    [[nodiscard]] const char* getName() const override { return "MouseClick"; }

    [[nodiscard]] i32 x() const { return m_x; }
    [[nodiscard]] i32 y() const { return m_y; }
    [[nodiscard]] i32 button() const { return m_button; }
    [[nodiscard]] i32 clicks() const { return m_clicks; }
    [[nodiscard]] i32 mods() const { return m_mods; }

    /**
     * @brief 检查是否是左键点击
     */
    [[nodiscard]] bool isLeftButton() const { return m_button == 0; }

    /**
     * @brief 检查是否是右键点击
     */
    [[nodiscard]] bool isRightButton() const { return m_button == 1; }

    /**
     * @brief 检查是否是双击
     */
    [[nodiscard]] bool isDoubleClick() const { return m_clicks == 2; }

private:
    i32 m_x;
    i32 m_y;
    i32 m_button;
    i32 m_clicks;
    i32 m_mods;
};

/**
 * @brief 鼠标释放事件
 */
class MouseReleaseEvent : public Event {
public:
    MouseReleaseEvent(i32 x, i32 y, i32 button, i32 mods = 0)
        : m_x(x)
        , m_y(y)
        , m_button(button)
        , m_mods(mods)
    {}

    [[nodiscard]] EventType getType() const override { return EventType::MouseRelease; }
    [[nodiscard]] const char* getName() const override { return "MouseRelease"; }

    [[nodiscard]] i32 x() const { return m_x; }
    [[nodiscard]] i32 y() const { return m_y; }
    [[nodiscard]] i32 button() const { return m_button; }
    [[nodiscard]] i32 mods() const { return m_mods; }

private:
    i32 m_x;
    i32 m_y;
    i32 m_button;
    i32 m_mods;
};

/**
 * @brief 鼠标拖动事件
 */
class MouseDragEvent : public Event {
public:
    MouseDragEvent(i32 x, i32 y, i32 deltaX, i32 deltaY, i32 button)
        : m_x(x)
        , m_y(y)
        , m_deltaX(deltaX)
        , m_deltaY(deltaY)
        , m_button(button)
    {}

    [[nodiscard]] EventType getType() const override { return EventType::MouseDrag; }
    [[nodiscard]] const char* getName() const override { return "MouseDrag"; }

    [[nodiscard]] i32 x() const { return m_x; }
    [[nodiscard]] i32 y() const { return m_y; }
    [[nodiscard]] i32 deltaX() const { return m_deltaX; }
    [[nodiscard]] i32 deltaY() const { return m_deltaY; }
    [[nodiscard]] i32 button() const { return m_button; }

private:
    i32 m_x;
    i32 m_y;
    i32 m_deltaX;
    i32 m_deltaY;
    i32 m_button;
};

/**
 * @brief 鼠标滚轮事件
 */
class MouseScrollEvent : public Event {
public:
    MouseScrollEvent(i32 x, i32 y, f64 deltaX, f64 deltaY)
        : m_x(x)
        , m_y(y)
        , m_deltaX(deltaX)
        , m_deltaY(deltaY)
    {}

    [[nodiscard]] EventType getType() const override { return EventType::MouseScroll; }
    [[nodiscard]] const char* getName() const override { return "MouseScroll"; }

    [[nodiscard]] i32 x() const { return m_x; }
    [[nodiscard]] i32 y() const { return m_y; }
    [[nodiscard]] f64 deltaX() const { return m_deltaX; }
    [[nodiscard]] f64 deltaY() const { return m_deltaY; }

    /**
     * @brief 检查是否是向上滚动
     */
    [[nodiscard]] bool isScrollUp() const { return m_deltaY > 0; }

    /**
     * @brief 检查是否是向下滚动
     */
    [[nodiscard]] bool isScrollDown() const { return m_deltaY < 0; }

private:
    i32 m_x;
    i32 m_y;
    f64 m_deltaX;
    f64 m_deltaY;
};

/**
 * @brief 鼠标移动事件
 */
class MouseMoveEvent : public Event {
public:
    MouseMoveEvent(i32 x, i32 y, i32 deltaX, i32 deltaY)
        : m_x(x)
        , m_y(y)
        , m_deltaX(deltaX)
        , m_deltaY(deltaY)
    {}

    [[nodiscard]] EventType getType() const override { return EventType::MouseMove; }
    [[nodiscard]] const char* getName() const override { return "MouseMove"; }

    [[nodiscard]] i32 x() const { return m_x; }
    [[nodiscard]] i32 y() const { return m_y; }
    [[nodiscard]] i32 deltaX() const { return m_deltaX; }
    [[nodiscard]] i32 deltaY() const { return m_deltaY; }

private:
    i32 m_x;
    i32 m_y;
    i32 m_deltaX;
    i32 m_deltaY;
};

/**
 * @brief 鼠标进入事件
 */
class MouseEnterEvent : public Event {
public:
    explicit MouseEnterEvent(i32 x, i32 y)
        : m_x(x)
        , m_y(y)
    {}

    [[nodiscard]] EventType getType() const override { return EventType::MouseEnter; }
    [[nodiscard]] const char* getName() const override { return "MouseEnter"; }

    [[nodiscard]] i32 x() const { return m_x; }
    [[nodiscard]] i32 y() const { return m_y; }

private:
    i32 m_x;
    i32 m_y;
};

/**
 * @brief 鼠标离开事件
 */
class MouseLeaveEvent : public Event {
public:
    explicit MouseLeaveEvent(i32 x, i32 y)
        : m_x(x)
        , m_y(y)
    {}

    [[nodiscard]] EventType getType() const override { return EventType::MouseLeave; }
    [[nodiscard]] const char* getName() const override { return "MouseLeave"; }

    [[nodiscard]] i32 x() const { return m_x; }
    [[nodiscard]] i32 y() const { return m_y; }

private:
    i32 m_x;
    i32 m_y;
};

/**
 * @brief 键盘按键事件
 */
class KeyEvent : public Event {
public:
    KeyEvent(i32 key, i32 scanCode, i32 action, i32 mods)
        : m_key(key)
        , m_scanCode(scanCode)
        , m_action(action)
        , m_mods(mods)
    {}

    [[nodiscard]] EventType getType() const override
    {
        switch (static_cast<KeyAction>(m_action)) {
            case KeyAction::Release:
                return EventType::KeyRelease;
            case KeyAction::Press:
                return EventType::KeyPress;
            case KeyAction::Repeat:
                return EventType::KeyRepeat;
            default:
                return EventType::KeyPress;
        }
    }

    [[nodiscard]] const char* getName() const override
    {
        switch (static_cast<KeyAction>(m_action)) {
            case KeyAction::Release:
                return "KeyRelease";
            case KeyAction::Press:
                return "KeyPress";
            case KeyAction::Repeat:
                return "KeyRepeat";
            default:
                return "Key";
        }
    }

    [[nodiscard]] i32 key() const { return m_key; }
    [[nodiscard]] i32 scanCode() const { return m_scanCode; }
    [[nodiscard]] i32 action() const { return m_action; }
    [[nodiscard]] i32 mods() const { return m_mods; }

    /**
     * @brief 检查是否按下
     */
    [[nodiscard]] bool isPressed() const { return static_cast<KeyAction>(m_action) == KeyAction::Press; }

    /**
     * @brief 检查是否释放
     */
    [[nodiscard]] bool isReleased() const { return static_cast<KeyAction>(m_action) == KeyAction::Release; }

    /**
     * @brief 检查是否重复
     */
    [[nodiscard]] bool isRepeat() const { return static_cast<KeyAction>(m_action) == KeyAction::Repeat; }

    /**
     * @brief 检查修饰键
     */
    [[nodiscard]] bool hasShift() const { return hasMod(static_cast<KeyMods>(m_mods), KeyMods::Shift); }
    [[nodiscard]] bool hasControl() const { return hasMod(static_cast<KeyMods>(m_mods), KeyMods::Control); }
    [[nodiscard]] bool hasAlt() const { return hasMod(static_cast<KeyMods>(m_mods), KeyMods::Alt); }
    [[nodiscard]] bool hasSuper() const { return hasMod(static_cast<KeyMods>(m_mods), KeyMods::Super); }

private:
    i32 m_key;
    i32 m_scanCode;
    i32 m_action;
    i32 m_mods;
};

/**
 * @brief 字符输入事件
 */
class CharInputEvent : public Event {
public:
    explicit CharInputEvent(u32 codePoint)
        : m_codePoint(codePoint)
    {}

    [[nodiscard]] EventType getType() const override { return EventType::CharInput; }
    [[nodiscard]] const char* getName() const override { return "CharInput"; }

    [[nodiscard]] u32 codePoint() const { return m_codePoint; }

    /**
     * @brief 获取UTF-8字符串
     */
    [[nodiscard]] std::string toUtf8() const { return util::text::utf8Encode(m_codePoint); }

private:
    u32 m_codePoint;
};

} // namespace mc::client::ui::kagero::event
