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
#include <string>

namespace mc::client::ui::kagero::event {

/**
 * @brief 焦点获得事件
 */
class FocusGainedEvent : public Event {
public:
    FocusGainedEvent() = default;

    [[nodiscard]] EventType getType() const override { return EventType::FocusGained; }
    [[nodiscard]] const char* getName() const override { return "FocusGained"; }
    [[nodiscard]] bool bubbles() const override { return false; }
};

/**
 * @brief 焦点失去事件
 */
class FocusLostEvent : public Event {
public:
    FocusLostEvent() = default;

    [[nodiscard]] EventType getType() const override { return EventType::FocusLost; }
    [[nodiscard]] const char* getName() const override { return "FocusLost"; }
    [[nodiscard]] bool bubbles() const override { return false; }
};

/**
 * @brief 组件初始化事件
 */
class WidgetInitEvent : public Event {
public:
    WidgetInitEvent() = default;

    [[nodiscard]] EventType getType() const override { return EventType::WidgetInit; }
    [[nodiscard]] const char* getName() const override { return "WidgetInit"; }
    [[nodiscard]] bool bubbles() const override { return false; }
};

/**
 * @brief 组件尺寸改变事件
 */
class WidgetResizeEvent : public Event {
public:
    WidgetResizeEvent(i32 oldWidth, i32 oldHeight, i32 newWidth, i32 newHeight)
        : m_oldWidth(oldWidth)
        , m_oldHeight(oldHeight)
        , m_newWidth(newWidth)
        , m_newHeight(newHeight)
    {}

    [[nodiscard]] EventType getType() const override { return EventType::WidgetResize; }
    [[nodiscard]] const char* getName() const override { return "WidgetResize"; }

    [[nodiscard]] i32 oldWidth() const { return m_oldWidth; }
    [[nodiscard]] i32 oldHeight() const { return m_oldHeight; }
    [[nodiscard]] i32 newWidth() const { return m_newWidth; }
    [[nodiscard]] i32 newHeight() const { return m_newHeight; }

private:
    i32 m_oldWidth;
    i32 m_oldHeight;
    i32 m_newWidth;
    i32 m_newHeight;
};

/**
 * @brief 组件显示事件
 *
 * 该事件会冒泡，可以从目标组件向上传播到父组件
 */
class WidgetShowEvent : public Event {
public:
    WidgetShowEvent() = default;

    [[nodiscard]] EventType getType() const override { return EventType::WidgetShow; }
    [[nodiscard]] const char* getName() const override { return "WidgetShow"; }
    [[nodiscard]] bool bubbles() const override { return true; }
};

/**
 * @brief 组件隐藏事件
 *
 * 该事件会冒泡，可以从目标组件向上传播到父组件
 */
class WidgetHideEvent : public Event {
public:
    WidgetHideEvent() = default;

    [[nodiscard]] EventType getType() const override { return EventType::WidgetHide; }
    [[nodiscard]] const char* getName() const override { return "WidgetHide"; }
    [[nodiscard]] bool bubbles() const override { return true; }
};

/**
 * @brief 组件启用事件
 *
 * 该事件会冒泡，可以从目标组件向上传播到父组件
 */
class WidgetEnableEvent : public Event {
public:
    WidgetEnableEvent() = default;

    [[nodiscard]] EventType getType() const override { return EventType::WidgetEnable; }
    [[nodiscard]] const char* getName() const override { return "WidgetEnable"; }
    [[nodiscard]] bool bubbles() const override { return true; }
};

/**
 * @brief 组件禁用事件
 *
 * 该事件会冒泡，可以从目标组件向上传播到父组件
 */
class WidgetDisableEvent : public Event {
public:
    WidgetDisableEvent() = default;

    [[nodiscard]] EventType getType() const override { return EventType::WidgetDisable; }
    [[nodiscard]] const char* getName() const override { return "WidgetDisable"; }
    [[nodiscard]] bool bubbles() const override { return true; }
};

/**
 * @brief 屏幕打开事件
 */
class ScreenOpenEvent : public Event {
public:
    explicit ScreenOpenEvent(const std::string& screenId)
        : m_screenId(screenId)
    {}

    [[nodiscard]] EventType getType() const override { return EventType::Custom; }
    [[nodiscard]] const char* getName() const override { return "ScreenOpen"; }

    [[nodiscard]] const std::string& screenId() const { return m_screenId; }

private:
    std::string m_screenId;
};

/**
 * @brief 屏幕关闭事件
 */
class ScreenCloseEvent : public Event {
public:
    explicit ScreenCloseEvent(const std::string& screenId)
        : m_screenId(screenId)
    {}

    [[nodiscard]] EventType getType() const override { return EventType::Custom; }
    [[nodiscard]] const char* getName() const override { return "ScreenClose"; }

    [[nodiscard]] const std::string& screenId() const { return m_screenId; }

private:
    std::string m_screenId;
};

/**
 * @brief 屏幕切换事件
 */
class ScreenChangeEvent : public Event {
public:
    ScreenChangeEvent(const std::string& fromScreen, const std::string& toScreen)
        : m_fromScreen(fromScreen)
        , m_toScreen(toScreen)
    {}

    [[nodiscard]] EventType getType() const override { return EventType::Custom; }
    [[nodiscard]] const char* getName() const override { return "ScreenChange"; }

    [[nodiscard]] const std::string& fromScreen() const { return m_fromScreen; }
    [[nodiscard]] const std::string& toScreen() const { return m_toScreen; }

private:
    std::string m_fromScreen;
    std::string m_toScreen;
};

} // namespace mc::client::ui::kagero::event
