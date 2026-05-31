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

namespace mc::client::ui::kagero::event {

/**
 * @brief 值变化事件模板
 *
 * 用于滑块、复选框等具有可变值的组件
 *
 * @tparam T 值类型
 */
template <typename T>
class ValueChangeEvent : public Event {
public:
    ValueChangeEvent(const T& oldValue, const T& newValue, void* source = nullptr)
        : m_oldValue(oldValue)
        , m_newValue(newValue)
    {
        setTarget(source);
    }

    [[nodiscard]] EventType getType() const override { return EventType::ValueChange; }
    [[nodiscard]] const char* getName() const override { return "ValueChange"; }

    [[nodiscard]] const T& oldValue() const { return m_oldValue; }
    [[nodiscard]] const T& newValue() const { return m_newValue; }

private:
    T m_oldValue;
    T m_newValue;
};

/**
 * @brief 文本变化事件
 */
class TextChangeEvent : public Event {
public:
    TextChangeEvent(const std::string& oldText, const std::string& newText, void* source = nullptr)
        : m_oldText(oldText)
        , m_newText(newText)
    {
        setTarget(source);
    }

    [[nodiscard]] EventType getType() const override { return EventType::TextChange; }
    [[nodiscard]] const char* getName() const override { return "TextChange"; }

    [[nodiscard]] const std::string& oldText() const { return m_oldText; }
    [[nodiscard]] const std::string& newText() const { return m_newText; }

private:
    std::string m_oldText;
    std::string m_newText;
};

/**
 * @brief 按钮点击事件
 */
class ButtonClickEvent : public Event {
public:
    explicit ButtonClickEvent(void* button = nullptr, i32 buttonIndex = 0)
        : m_buttonIndex(buttonIndex)
    {
        setTarget(button);
    }

    [[nodiscard]] EventType getType() const override { return EventType::MouseClick; }
    [[nodiscard]] const char* getName() const override { return "ButtonClick"; }
    [[nodiscard]] bool bubbles() const override { return true; }

    [[nodiscard]] i32 buttonIndex() const { return m_buttonIndex; }

private:
    i32 m_buttonIndex;
};

/**
 * @brief 滑块值变化事件
 */
using SliderValueChangeEvent = ValueChangeEvent<f64>;

/**
 * @brief 复选框状态变化事件
 */
using CheckboxChangeEvent = ValueChangeEvent<bool>;

/**
 * @brief 选择事件
 *
 * 用于列表、下拉框等具有选择功能的组件
 */
class SelectionEvent : public Event {
public:
    SelectionEvent(i32 oldIndex, i32 newIndex, void* source = nullptr)
        : m_oldIndex(oldIndex)
        , m_newIndex(newIndex)
    {
        setTarget(source);
    }

    [[nodiscard]] EventType getType() const override { return EventType::ValueChange; }
    [[nodiscard]] const char* getName() const override { return "Selection"; }

    [[nodiscard]] i32 oldIndex() const { return m_oldIndex; }
    [[nodiscard]] i32 newIndex() const { return m_newIndex; }

    /**
     * @brief 检查是否有选择
     */
    [[nodiscard]] bool hasSelection() const { return m_newIndex >= 0; }

private:
    i32 m_oldIndex;
    i32 m_newIndex;
};

/**
 * @brief 多选事件
 */
class MultiSelectionEvent : public Event {
public:
    MultiSelectionEvent(const std::vector<i32>& oldIndices, const std::vector<i32>& newIndices, void* source = nullptr)
        : m_oldIndices(oldIndices)
        , m_newIndices(newIndices)
    {
        setTarget(source);
    }

    [[nodiscard]] EventType getType() const override { return EventType::ValueChange; }
    [[nodiscard]] const char* getName() const override { return "MultiSelection"; }

    [[nodiscard]] const std::vector<i32>& oldIndices() const { return m_oldIndices; }
    [[nodiscard]] const std::vector<i32>& newIndices() const { return m_newIndices; }

private:
    std::vector<i32> m_oldIndices;
    std::vector<i32> m_newIndices;
};

/**
 * @brief 物品槽点击事件
 */
class SlotClickEvent : public Event {
public:
    SlotClickEvent(i32 slotIndex, i32 button, bool shiftHeld, void* source = nullptr)
        : m_slotIndex(slotIndex)
        , m_button(button)
        , m_shiftHeld(shiftHeld)
    {
        setTarget(source);
    }

    [[nodiscard]] EventType getType() const override { return EventType::MouseClick; }
    [[nodiscard]] const char* getName() const override { return "SlotClick"; }

    [[nodiscard]] i32 slotIndex() const { return m_slotIndex; }
    [[nodiscard]] i32 button() const { return m_button; }
    [[nodiscard]] bool isShiftHeld() const { return m_shiftHeld; }

    /**
     * @brief 检查是否是左键点击
     */
    [[nodiscard]] bool isLeftClick() const { return m_button == 0; }

    /**
     * @brief 检查是否是右键点击
     */
    [[nodiscard]] bool isRightClick() const { return m_button == 1; }

private:
    i32 m_slotIndex;
    i32 m_button;
    bool m_shiftHeld;
};

/**
 * @brief 容器关闭事件
 */
class ContainerCloseEvent : public Event {
public:
    explicit ContainerCloseEvent(void* container = nullptr) { setTarget(container); }

    [[nodiscard]] EventType getType() const override { return EventType::Custom; }
    [[nodiscard]] const char* getName() const override { return "ContainerClose"; }
};

/**
 * @brief 表单提交事件
 */
class FormSubmitEvent : public Event {
public:
    explicit FormSubmitEvent(const std::string& formId = "")
        : m_formId(formId)
    {}

    [[nodiscard]] EventType getType() const override { return EventType::Custom; }
    [[nodiscard]] const char* getName() const override { return "FormSubmit"; }

    [[nodiscard]] const std::string& formId() const { return m_formId; }

private:
    std::string m_formId;
};

/**
 * @brief 拖拽开始事件
 */
class DragStartEvent : public Event {
public:
    DragStartEvent(i32 x, i32 y, void* source = nullptr)
        : m_x(x)
        , m_y(y)
    {
        setTarget(source);
    }

    [[nodiscard]] EventType getType() const override { return EventType::Custom; }
    [[nodiscard]] const char* getName() const override { return "DragStart"; }

    [[nodiscard]] i32 x() const { return m_x; }
    [[nodiscard]] i32 y() const { return m_y; }

private:
    i32 m_x;
    i32 m_y;
};

/**
 * @brief 拖拽结束事件
 */
class DragEndEvent : public Event {
public:
    DragEndEvent(i32 x, i32 y, bool dropped, void* source = nullptr)
        : m_x(x)
        , m_y(y)
        , m_dropped(dropped)
    {
        setTarget(source);
    }

    [[nodiscard]] EventType getType() const override { return EventType::Custom; }
    [[nodiscard]] const char* getName() const override { return "DragEnd"; }

    [[nodiscard]] i32 x() const { return m_x; }
    [[nodiscard]] i32 y() const { return m_y; }
    [[nodiscard]] bool wasDropped() const { return m_dropped; }

private:
    i32 m_x;
    i32 m_y;
    bool m_dropped;
};

} // namespace mc::client::ui::kagero::event
