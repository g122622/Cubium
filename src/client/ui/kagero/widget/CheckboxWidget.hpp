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
#include <functional>
#include <memory>
#include <string>
#include <utility>

#include "Widget.hpp"
#include "client/ui/Glyph.hpp"
#include "client/ui/kagero/Types.hpp"
#include "client/ui/kagero/paint/PaintContext.hpp"
#include "common/core/Types.hpp"

namespace mc::client::ui::kagero::widget {

/**
 * @brief 复选框组件
 *
 * 可选中/取消选中的复选框组件。
 *
 * 使用示例：
 * @code
 * auto checkbox = std::make_unique<CheckboxWidget>(
 *     "chk_option", 10, 10, "Enable Feature"
 * );
 * checkbox->setChecked(true);
 * checkbox->setOnChanged([](bool checked) {
 *     // 处理状态变化
 * });
 * @endcode
 */
class CheckboxWidget : public Widget {
public:
    /**
     * @brief 状态变化回调类型
     */
    using OnChangedCallback = std::function<void(bool)>;

    /**
     * @brief 默认构造函数
     */
    CheckboxWidget() = default;

    /**
     * @brief 构造函数
     * @param id 组件ID
     * @param x X坐标
     * @param y Y坐标
     * @param text 显示文本
     */
    CheckboxWidget(std::string id, i32 x, i32 y, std::string text)
        : Widget(std::move(id))
        , m_text(std::move(text))
    {
        setBounds(Rect(x, y, 20, 20)); // 默认尺寸
    }

    /**
     * @brief 构造函数（带尺寸）
     * @param id 组件ID
     * @param x X坐标
     * @param y Y坐标
     * @param width 宽度
     * @param height 高度
     * @param text 显示文本
     */
    CheckboxWidget(std::string id, i32 x, i32 y, i32 width, i32 height, std::string text)
        : Widget(std::move(id))
        , m_text(std::move(text))
    {
        setBounds(Rect(x, y, width, height));
    }

    // ==================== 生命周期 ====================

    void paint(PaintContext& ctx) override
    {
        if (!isVisible()) return;
        const Rect box{bounds().x, bounds().y, boxSize(), boxSize()};
        ctx.drawFilledRect(box, Colors::fromARGB(255, 32, 32, 32));
        ctx.drawBorder(box, 1.0f, Colors::fromARGB(255, 96, 96, 96));
        if (m_checked) {
            const Rect mark{box.x + 4, box.y + 4, box.width - 8, box.height - 8};
            ctx.drawFilledRect(mark, m_checkColor);
        }
    }

    // ==================== 事件处理 ====================

    bool onClick(i32 mouseX, i32 mouseY, i32 button, i32 mods) override
    {
        (void)mouseX;
        (void)mouseY;
        (void)mods;

        if (!isActive() || !isVisible()) return false;
        if (button != 0) return false;

        // 切换状态
        toggle();

        return true;
    }

    // ==================== 状态操作 ====================

    /**
     * @brief 设置选中状态
     */
    void setChecked(bool checked)
    {
        if (m_checked != checked) {
            m_checked = checked;
            if (m_onChanged) {
                m_onChanged(m_checked);
            }
        }
    }

    /**
     * @brief 获取选中状态
     */
    [[nodiscard]] bool isChecked() const { return m_checked; }

    /**
     * @brief 切换选中状态
     */
    void toggle() { setChecked(!m_checked); }

    // ==================== 显示属性 ====================

    /**
     * @brief 设置文本
     */
    void setText(const std::string& text) { m_text = text; }

    /**
     * @brief 获取文本
     */
    [[nodiscard]] const std::string& text() const { return m_text; }

    /**
     * @brief 设置状态变化回调
     */
    void setOnChanged(OnChangedCallback callback) { m_onChanged = std::move(callback); }

    /**
     * @brief 设置文本颜色
     */
    void setTextColor(u32 color) { m_textColor = color; }

    /**
     * @brief 获取文本颜色
     */
    [[nodiscard]] u32 textColor() const { return m_textColor; }

    /**
     * @brief 设置勾选颜色
     */
    void setCheckColor(u32 color) { m_checkColor = color; }

    /**
     * @brief 获取勾选颜色
     */
    [[nodiscard]] u32 checkColor() const { return m_checkColor; }

    /**
     * @brief 获取复选框框体大小
     */
    [[nodiscard]] i32 boxSize() const { return std::min(width(), height()); }

protected:
    bool m_checked = false;              ///< 选中状态
    std::string m_text;                  ///< 显示文本
    u32 m_textColor = Colors::WHITE;     ///< 文本颜色
    u32 m_checkColor = Colors::MC_WHITE; ///< 勾选颜色
    OnChangedCallback m_onChanged;       ///< 状态变化回调
};

} // namespace mc::client::ui::kagero::widget
