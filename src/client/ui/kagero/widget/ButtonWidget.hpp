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
 * @brief 按钮组件
 *
 * 标准按钮组件，支持：
 * - 文本显示
 * - 点击回调
 * - 悬停提示（通过 Widget 基类的 setTooltip/setTooltipDelay 设置）
 * - 禁用状态
 * - 三态渲染（正常、悬停、禁用）
 *
 * 使用示例：
 * @code
 * auto button = std::make_unique<ButtonWidget>(
 *     "btn_submit",
 *     100, 100, 200, 20,
 *     "Submit",
 *     [](ButtonWidget& btn) {
 *         // 处理点击
 *     }
 * );
 * // 设置 Tooltip
 * button->setTooltip("保存");
 * button->setTooltipDelay(500); // 悬停500ms后显示
 * @endcode
 */
class ButtonWidget : public Widget {
public:
    /**
     * @brief 按钮样式
     */
    struct Style {
        u32 normalColor = Colors::fromARGB(255, 60, 60, 60);          ///< 正常状态颜色
        u32 hoverColor = Colors::fromARGB(255, 80, 80, 80);           ///< 悬停状态颜色
        u32 disabledColor = Colors::fromARGB(255, 40, 40, 40);        ///< 禁用状态颜色
        u32 textColor = Colors::WHITE;                                ///< 文本颜色
        u32 disabledTextColor = Colors::fromARGB(255, 128, 128, 128); ///< 禁用文本颜色
        u32 borderColor = Colors::fromARGB(255, 100, 100, 100);       ///< 边框颜色
        u32 hoverBorderColor = Colors::fromARGB(255, 150, 150, 150);  ///< 悬停边框颜色
        i32 cornerRadius = 3;                                         ///< 圆角半径
        bool drawBorder = true;                                       ///< 是否绘制边框
    };

    /**
     * @brief 点击回调类型
     */
    using OnPressCallback = std::function<void(ButtonWidget&)>;

    /**
     * @brief 默认构造函数
     */
    ButtonWidget() = default;

    /**
     * @brief 构造函数
     * @param id 组件ID
     * @param x X坐标
     * @param y Y坐标
     * @param width 宽度
     * @param height 高度
     * @param text 按钮文本
     */
    ButtonWidget(std::string id, i32 x, i32 y, i32 width, i32 height, std::string text)
        : Widget(std::move(id))
        , m_text(std::move(text))
    {
        setBounds(Rect(x, y, width, height));
    }

    /**
     * @brief 构造函数（带回调）
     * @param id 组件ID
     * @param x X坐标
     * @param y Y坐标
     * @param width 宽度
     * @param height 高度
     * @param text 按钮文本
     * @param onPress 点击回调
     */
    ButtonWidget(std::string id, i32 x, i32 y, i32 width, i32 height, std::string text, OnPressCallback onPress)
        : Widget(std::move(id))
        , m_text(std::move(text))
        , m_onPress(std::move(onPress))
    {
        setBounds(Rect(x, y, width, height));
    }

    // ==================== 生命周期 ====================

    void paint(PaintContext& ctx) override
    {
        if (!isVisible()) return;
        ctx.drawFilledRect(bounds(), getBackgroundColor());
        if (m_style.drawBorder) {
            const u32 border = isHovered() ? m_style.hoverBorderColor : m_style.borderColor;
            ctx.drawBorder(bounds(), 1.0f, border);
        }
        ctx.drawTextCentered(m_text, bounds(), getTextColor());

        // 渲染 Tooltip（如果有且悬停）
        refreshTooltip(ctx, static_cast<f32>(ctx.canvas().width()), static_cast<f32>(ctx.canvas().height()));
    }

    // ==================== 事件处理 ====================

    bool onClick(i32 mouseX, i32 mouseY, i32 button, i32 mods) override
    {
        (void)mouseX;
        (void)mouseY;
        (void)mods;

        if (!isActive() || !isVisible()) return false;
        if (button != 0) return false; // 只响应左键

        // 播放点击音效
        playClickSound();

        // 调用回调
        if (m_onPress) {
            m_onPress(*this);
        }

        return true;
    }

    bool onRelease(i32 mouseX, i32 mouseY, i32 button, i32 mods) override
    {
        (void)mouseX;
        (void)mouseY;
        (void)button;
        (void)mods;
        return isActive() && isVisible();
    }

    // ==================== 属性设置 ====================

    /**
     * @brief 设置按钮文本
     */
    void setText(const std::string& text) { m_text = text; }

    /**
     * @brief 获取按钮文本
     */
    [[nodiscard]] const std::string& text() const { return m_text; }

    /**
     * @brief 设置点击回调
     */
    void setOnPress(OnPressCallback callback) { m_onPress = std::move(callback); }

    /**
     * @brief 设置样式
     */
    void setStyle(const Style& style) { m_style = style; }

    /**
     * @brief 获取样式
     */
    [[nodiscard]] const Style& style() const { return m_style; }

    /**
     * @brief 获取可变样式
     */
    [[nodiscard]] Style& style() { return m_style; }

    /**
     * @brief 获取当前渲染状态（0=禁用，1=正常，2=悬停）
     */
    [[nodiscard]] i32 getRenderState() const
    {
        if (!isActive()) return 0;
        if (isHovered()) return 2;
        return 1;
    }

    /**
     * @brief 获取当前背景颜色
     */
    [[nodiscard]] u32 getBackgroundColor() const
    {
        switch (getRenderState()) {
            case 0:
                return m_style.disabledColor;
            case 2:
                return m_style.hoverColor;
            default:
                return m_style.normalColor;
        }
    }

    /**
     * @brief 获取当前文本颜色
     */
    [[nodiscard]] u32 getTextColor() const { return isActive() ? m_style.textColor : m_style.disabledTextColor; }

protected:
    /**
     * @brief 播放点击音效
     *
     * 子类可重写以自定义音效。
     * 默认播放 minecraft:ui.button.click，音量 0.25。
     */
    virtual void playClickSound() { Widget::playUiSound("minecraft:ui.button.click"); }

    std::string m_text;        ///< 按钮文本
    OnPressCallback m_onPress; ///< 点击回调
    Style m_style;             ///< 按钮样式
};

/**
 * @brief 图片按钮组件
 *
 * 使用自定义图片作为按钮外观
 */
class ImageButtonWidget : public ButtonWidget {
public:
    /**
     * @brief 构造函数
     * @param id 组件ID
     * @param x X坐标
     * @param y Y坐标
     * @param width 宽度
     * @param height 高度
     * @param u 纹理U坐标
     * @param v 纹理V坐标
     * @param hoveredVU 悬停时的U偏移
     * @param texturePath 纹理路径
     */
    ImageButtonWidget(
        std::string id, i32 x, i32 y, i32 width, i32 height, i32 u, i32 v, i32 hoveredVU, std::string texturePath)
        : ButtonWidget(std::move(id), x, y, width, height, "")
        , m_u(u)
        , m_v(v)
        , m_hoveredU(hoveredVU)
        , m_texturePath(std::move(texturePath))
    {}

    void paint(PaintContext& ctx) override
    {
        if (!isVisible()) return;
        (void)ctx;
        // TODO: 实际渲染逻辑
        // 绑定纹理并渲染

        // 渲染 Tooltip（如果有且悬停）
        refreshTooltip(ctx, static_cast<f32>(ctx.canvas().width()), static_cast<f32>(ctx.canvas().height()));
    }

    /**
     * @brief 设置纹理坐标
     */
    void setTextureCoords(i32 u, i32 v, i32 hoveredU)
    {
        m_u = u;
        m_v = v;
        m_hoveredU = hoveredU;
    }

    /**
     * @brief 设置纹理路径
     */
    void setTexturePath(std::string path) { m_texturePath = std::move(path); }

private:
    i32 m_u = 0;               ///< 纹理U坐标
    i32 m_v = 0;               ///< 纹理V坐标
    i32 m_hoveredU = 0;        ///< 悬停时的U偏移
    std::string m_texturePath; ///< 纹理路径
};

} // namespace mc::client::ui::kagero::widget
