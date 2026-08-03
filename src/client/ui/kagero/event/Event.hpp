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

#include "client/ui/kagero/Types.hpp"
#include "common/core/Types.hpp"
#include <functional>

namespace mc::client::ui::kagero::event {

/**
 * @brief 事件类型枚举
 *
 * 定义所有事件类型的唯一标识符
 */
enum class EventType : u32 {
    // 输入事件
    MouseClick = 1,
    MouseRelease = 2,
    MouseDrag = 3,
    MouseScroll = 4,
    MouseMove = 5,
    MouseEnter = 6,
    MouseLeave = 7,

    KeyPress = 10,
    KeyRelease = 11,
    KeyRepeat = 12,
    CharInput = 13,

    // 焦点事件
    FocusGained = 20,
    FocusLost = 21,

    // 值变化事件
    ValueChange = 30,
    TextChange = 31,

    // 组件事件
    WidgetInit = 40,
    WidgetResize = 41,
    WidgetShow = 42,
    WidgetHide = 43,
    WidgetEnable = 44,
    WidgetDisable = 45,

    // 自定义事件
    Custom = 1000
};

/**
 * @brief 事件基类
 *
 * 所有事件类型的基类，提供事件的基本属性和方法。
 * 事件可以被取消（阻止传播）。
 *
 * 使用示例：
 * @code
 * class MyEvent : public Event {
 * public:
 *     EventType getType() const override { return EventType::Custom; }
 *     const char* getName() const override { return "MyEvent"; }
 *
 *     i32 myData;
 * };
 * @endcode
 */
class Event {
public:
    virtual ~Event() = default;

    /**
     * @brief 获取事件类型
     */
    [[nodiscard]] virtual EventType getType() const = 0;

    /**
     * @brief 获取事件名称
     */
    [[nodiscard]] virtual const char* getName() const = 0;

    /**
     * @brief 检查事件是否被取消
     */
    [[nodiscard]] bool isCancelled() const { return m_cancelled; }

    /**
     * @brief 取消事件
     *
     * 取消事件将阻止事件继续传播
     */
    void cancel() const { m_cancelled = true; }

    /**
     * @brief 获取事件时间戳
     */
    [[nodiscard]] u64 timestamp() const { return m_timestamp; }

    /**
     * @brief 设置事件时间戳
     */
    void setTimestamp(u64 ts) { m_timestamp = ts; }

    /**
     * @brief 检查是否是气泡事件
     *
     * 气泡事件会从目标组件向上传播到父组件
     */
    [[nodiscard]] virtual bool bubbles() const { return true; }

    /**
     * @brief 检查是否是可取消事件
     */
    [[nodiscard]] virtual bool isCancellable() const { return true; }

    /**
     * @brief 获取事件目标
     */
    [[nodiscard]] void* target() const { return m_target; }

    /**
     * @brief 设置事件目标
     */
    void setTarget(void* target) { m_target = target; }

    /**
     * @brief 获取事件当前目标
     */
    [[nodiscard]] void* currentTarget() const { return m_currentTarget; }

    /**
     * @brief 设置事件当前目标
     */
    void setCurrentTarget(void* target) { m_currentTarget = target; }

protected:
    mutable bool m_cancelled = false; // mutable 以允许在 const 上下文中取消
    u64 m_timestamp = 0;
    void* m_target = nullptr;
    void* m_currentTarget = nullptr;
};

/**
 * @brief 简单事件模板
 *
 * 用于快速创建携带数据的事件类型
 *
 * @tparam T 数据类型
 * @tparam Type 事件类型
 */
template <typename T, EventType Type>
class SimpleEvent : public Event {
public:
    explicit SimpleEvent(T data)
        : m_data(std::move(data))
    {}

    [[nodiscard]] EventType getType() const override { return Type; }

    [[nodiscard]] const char* getName() const override
    {
        static const char* name = []() {
            switch (Type) {
                case EventType::MouseClick:
                    return "MouseClick";
                case EventType::MouseRelease:
                    return "MouseRelease";
                case EventType::MouseDrag:
                    return "MouseDrag";
                case EventType::MouseScroll:
                    return "MouseScroll";
                case EventType::KeyPress:
                    return "KeyPress";
                case EventType::KeyRelease:
                    return "KeyRelease";
                case EventType::CharInput:
                    return "CharInput";
                case EventType::FocusGained:
                    return "FocusGained";
                case EventType::FocusLost:
                    return "FocusLost";
                case EventType::ValueChange:
                    return "ValueChange";
                case EventType::TextChange:
                    return "TextChange";
                default:
                    return "Unknown";
            }
        }();
        return name;
    }

    /**
     * @brief 获取事件数据
     */
    [[nodiscard]] const T& data() const { return m_data; }
    [[nodiscard]] T& data() { return m_data; }

private:
    T m_data;
};

/**
 * @brief 事件处理结果
 */
struct EventResult {
    bool handled = false;   ///< 是否被处理
    bool cancelled = false; ///< 是否被取消

    EventResult& setHandled(bool value)
    {
        handled = value;
        return *this;
    }

    EventResult& setCancelled(bool value)
    {
        cancelled = value;
        return *this;
    }
};

/**
 * @brief 事件处理器类型
 */
template <typename EventT>
using EventHandler = std::function<void(const EventT&)>;

/**
 * @brief 事件过滤器类型
 *
 * 返回true表示继续处理，false表示停止
 */
using EventFilter = std::function<bool(const Event&)>;

} // namespace mc::client::ui::kagero::event
