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

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <functional>
#include <memory>
#include <string>
#include <utility>

#include "Widget.hpp"
#include "client/ui/Glyph.hpp"
#include "client/ui/kagero/Types.hpp"
#include "client/ui/kagero/paint/PaintContext.hpp"
#include "common/core/Types.hpp"
#include "common/input/KeyBinding.hpp"

namespace mc::client::ui::kagero::widget {

/**
 * @brief 滑块组件
 *
 * 支持拖动调整值的滑块组件。
 *
 * 使用示例：
 * @code
 * auto slider = std::make_unique<SliderWidget>(
 *     "sldr_volume", 10, 10, 200, 20,
 *     0.0, 100.0, 50.0
 * );
 * slider->setOnValueChanged([](f64 value) {
 *     // 处理值变化
 * });
 * @endcode
 */
class SliderWidget : public Widget {
public:
    /**
     * @brief 值变化回调类型
     */
    using OnValueChangedCallback = std::function<void(f64)>;

    /**
     * @brief 格式化回调类型
     */
    using FormatCallback = std::function<std::string(f64)>;

    /**
     * @brief 默认构造函数
     */
    SliderWidget() = default;

    /**
     * @brief 构造函数
     * @param id 组件ID
     * @param x X坐标
     * @param y Y坐标
     * @param width 宽度
     * @param height 高度
     * @param minVal 最小值
     * @param maxVal 最大值
     * @param value 当前值
     */
    SliderWidget(std::string id, i32 x, i32 y, i32 width, i32 height, f64 minVal, f64 maxVal, f64 value)
        : Widget(std::move(id))
        , m_minValue(minVal)
        , m_maxValue(maxVal)
        , m_value(minVal) // 先设为最小值，后面再用 setValue 设置
        , m_stepSize(0.0)
    {
        setBounds(Rect(x, y, width, height));
        m_value = clampValue(value); // 现在可以安全调用 clampValue
    }

    // ==================== 生命周期 ====================

    void paint(PaintContext& ctx) override
    {
        if (!isVisible()) return;

        // 绘制滑块轨道背景
        ctx.drawFilledRect(bounds(), Colors::fromARGB(255, 45, 45, 45));
        ctx.drawBorder(bounds(), 1.0f, Colors::fromARGB(255, 90, 90, 90));

        // 计算滑块手柄位置和大小
        const i32 knobSize = std::max(8, height() - 4);
        const i32 range = std::max(1, width() - knobSize - 4);
        const i32 knobX = x() + 2 + static_cast<i32>(getRatio() * range);
        const Rect knob{knobX, y() + 2, knobSize, std::max(4, height() - 4)};
        ctx.drawFilledRect(knob, Colors::fromARGB(255, 200, 200, 200));

        // 根据 m_showValue 绘制显示文本
        if (m_showValue) {
            std::string text = displayText();
            if (!text.empty()) {
                f32 textWidth = ctx.getTextWidth(text);
                i32 textX = x() + (width() - static_cast<i32>(textWidth)) / 2;
                i32 textY = y() + (height() - static_cast<i32>(ctx.getFontHeight())) / 2;
                ctx.drawText(text, textX, textY, Colors::WHITE);
            }
        }
    }

    // ==================== 事件处理 ====================

    bool onClick(i32 mouseX, i32 mouseY, i32 button, i32 mods) override
    {
        (void)mods;
        if (!isActive() || !isVisible()) return false;
        if (button != 0) return false; // 只响应左键

        // 设置焦点
        setFocused(true);

        // 开始拖动
        m_dragging = true;
        setValueFromMouse(mouseX);

        return true;
    }

    bool onRelease(i32 mouseX, i32 mouseY, i32 button, i32 mods) override
    {
        (void)mouseX;
        (void)mouseY;
        (void)mods;

        if (button != 0) return false;

        if (m_dragging) {
            m_dragging = false;
            // 触发最终值变化（拖拽结束的最终回调由 onRelease 统一触发，
            // 与原设计一致；onDragEnd 不重复触发，避免双重回调）
            if (m_onValueChanged) {
                m_onValueChanged(m_value);
            }
            return true;
        }

        return false;
    }

    bool onDrag(i32 mouseX, i32 mouseY, i32 deltaX, i32 deltaY, i32 button) override
    {
        (void)deltaX;
        (void)deltaY;
        (void)button;

        if (!m_dragging) return false;

        setValueFromMouse(mouseX);
        return true;
    }

    bool onDragStart(i32 mouseX, i32 mouseY, i32 button, i32 mods) override
    {
        (void)mouseX;
        (void)mouseY;
        (void)mods;
        // onClick 已经设置了 m_dragging 并设置了焦点，onDragStart 仅确认拖拽按钮
        if (button != 0) return false;
        return m_dragging;
    }

    bool onDragEnd(i32 mouseX, i32 mouseY, i32 button, bool dropped) override
    {
        (void)mouseX;
        (void)mouseY;
        (void)dropped;
        // onDragEnd 仅作为拖拽结束通知，不清理 m_dragging，也不触发回调。
        // 状态清理与最终回调由 onRelease 统一完成（与原设计一致，
        // 避免 onDragEnd→onRelease 双重触发回调）。
        if (button != 0) return false;
        return m_dragging;
    }

    bool onScroll(i32 mouseX, i32 mouseY, f64 delta) override
    {
        (void)mouseX;
        (void)mouseY;

        if (!isActive() || !isVisible()) return false;

        // 滚轮调整值
        f64 step = (m_stepSize > 0) ? m_stepSize : (m_maxValue - m_minValue) / 20.0;
        setValue(m_value + delta * step);

        return true;
    }

    bool onKey(i32 key, i32 scanCode, i32 action, i32 mods) override
    {
        (void)scanCode;
        (void)mods;

        if (!isActive() || !isVisible() || !isFocused()) return false;
        if (action != static_cast<i32>(KeyAction::Press) && action != static_cast<i32>(KeyAction::Repeat)) return false;

        f64 step = (m_stepSize > 0) ? m_stepSize : (m_maxValue - m_minValue) / 20.0;

        switch (key) {
            case Keys::Right:
                setValue(m_value + step);
                return true;

            case Keys::Left:
                setValue(m_value - step);
                return true;

            default:
                return false;
        }
    }

    // ==================== 值操作 ====================

    /**
     * @brief 设置值
     */
    virtual void setValue(f64 value)
    {
        f64 newValue = clampValue(value);
        if (m_value != newValue) {
            m_value = newValue;
            if (m_onValueChanged) {
                m_onValueChanged(m_value);
            }
        }
    }

    /**
     * @brief 获取值
     */
    [[nodiscard]] f64 value() const { return m_value; }

    /**
     * @brief 设置最小值
     */
    void setMinValue(f64 minVal)
    {
        m_minValue = minVal;
        setValue(m_value); // 重新约束
    }

    /**
     * @brief 获取最小值
     */
    [[nodiscard]] f64 minValue() const { return m_minValue; }

    /**
     * @brief 设置最大值
     */
    void setMaxValue(f64 maxVal)
    {
        m_maxValue = maxVal;
        setValue(m_value); // 重新约束
    }

    /**
     * @brief 获取最大值
     */
    [[nodiscard]] f64 maxValue() const { return m_maxValue; }

    /**
     * @brief 设置值范围
     */
    void setRange(f64 minVal, f64 maxVal)
    {
        m_minValue = minVal;
        m_maxValue = maxVal;
        setValue(m_value); // 重新约束
    }

    /**
     * @brief 设置步长
     */
    void setStepSize(f64 step) { m_stepSize = step; }

    /**
     * @brief 获取步长
     */
    [[nodiscard]] f64 stepSize() const { return m_stepSize; }

    /**
     * @brief 获取滑块位置比例（0.0-1.0）
     */
    [[nodiscard]] f64 getRatio() const
    {
        if (m_maxValue <= m_minValue) return 0.0;
        return (m_value - m_minValue) / (m_maxValue - m_minValue);
    }

    /**
     * @brief 从比例设置值
     */
    void setFromRatio(f64 ratio)
    {
        ratio = std::max(0.0, std::min(1.0, ratio));
        setValue(m_minValue + ratio * (m_maxValue - m_minValue));
    }

    // ==================== 显示文本 ====================

    /**
     * @brief 设置显示文本
     */
    void setDisplayText(const std::string& text) { m_displayText = text; }

    /**
     * @brief 获取显示文本
     */
    [[nodiscard]] std::string displayText() const
    {
        if (m_formatCallback) {
            return m_formatCallback(m_value);
        }
        if (!m_displayText.empty()) {
            return m_displayText;
        }
        return formatValue(m_value);
    }

    /**
     * @brief 设置格式化回调
     */
    void setFormatCallback(FormatCallback callback) { m_formatCallback = std::move(callback); }

    /**
     * @brief 设置值变化回调
     */
    void setOnValueChanged(OnValueChangedCallback callback) { m_onValueChanged = std::move(callback); }

    /**
     * @brief 设置是否显示值
     */
    void setShowValue(bool show) { m_showValue = show; }

    /**
     * @brief 是否显示值
     */
    [[nodiscard]] bool showValue() const { return m_showValue; }

    /**
     * @brief 获取滑块手柄位置（像素）
     */
    [[nodiscard]] i32 getHandlePosition() const
    {
        i32 sliderWidth = width() - 8; // 减去手柄宽度
        return x() + static_cast<i32>(getRatio() * sliderWidth);
    }

    /**
     * @brief 检查是否正在拖动
     */
    [[nodiscard]] bool isDragging() const { return m_dragging; }

protected:
    /**
     * @brief 从鼠标位置设置值
     */
    void setValueFromMouse(i32 mouseX)
    {
        i32 sliderX = x() + 4; // 手柄宽度/2
        i32 sliderWidth = width() - 8;

        f64 ratio = static_cast<f64>(mouseX - sliderX) / sliderWidth;
        ratio = std::max(0.0, std::min(1.0, ratio));

        f64 newValue = m_minValue + ratio * (m_maxValue - m_minValue);

        // 应用步长
        if (m_stepSize > 0) {
            newValue = std::round(newValue / m_stepSize) * m_stepSize;
        }

        setValue(newValue);
    }

    /**
     * @brief 约束值到有效范围
     */
    [[nodiscard]] f64 clampValue(f64 value) const
    {
        if (m_stepSize > 0) {
            value = std::round(value / m_stepSize) * m_stepSize;
        }
        return std::max(m_minValue, std::min(m_maxValue, value));
    }

    /**
     * @brief 格式化值显示
     */
    [[nodiscard]] virtual std::string formatValue(f64 val) const
    {
        // 默认显示整数或两位小数
        if (m_stepSize >= 1.0) {
            return std::to_string(static_cast<i32>(val));
        } else {
            std::array<char, 32> buf{};
            std::snprintf(buf.data(), buf.size(), "%.2f", val);
            return std::string(buf.data());
        }
    }

    // 值
    f64 m_minValue = 0.0;
    f64 m_maxValue = 100.0;
    f64 m_value = 0.0;
    f64 m_stepSize = 0.0;

    // 显示
    std::string m_displayText;
    bool m_showValue = true;
    FormatCallback m_formatCallback;

    // 状态
    bool m_dragging = false;

    // 回调
    OnValueChangedCallback m_onValueChanged;
};

/**
 * @brief 整数滑块组件
 */
class IntSliderWidget : public SliderWidget {
public:
    using SliderWidget::SliderWidget;

    void setValue(f64 value) override { SliderWidget::setValue(std::round(value)); }

    [[nodiscard]] i32 intValue() const { return static_cast<i32>(value()); }

    void setIntValue(i32 val) { setValue(static_cast<f64>(val)); }

protected:
    [[nodiscard]] std::string formatValue(f64 val) const override { return std::to_string(static_cast<i32>(val)); }
};

} // namespace mc::client::ui::kagero::widget
