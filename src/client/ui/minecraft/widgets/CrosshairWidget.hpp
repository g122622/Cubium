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

#include "client/ui/kagero/paint/PaintContext.hpp"
#include "client/ui/kagero/widget/Widget.hpp"
#include "common/core/Types.hpp"

namespace mc::client::ui::minecraft::widgets {

/**
 * @brief 准星Widget
 *
 * 在屏幕中心渲染十字准星，用于第一人称视角瞄准。
 */
class CrosshairWidget : public kagero::widget::Widget {
public:
    CrosshairWidget();
    ~CrosshairWidget() override = default;

    /**
     * @brief 绘制准星
     */
    void paint(kagero::widget::PaintContext& ctx) override;

    /**
     * @brief 设置准星颜色（ARGB格式）
     */
    void setColor(u32 color) { m_color = color; }

    /**
     * @brief 设置准星大小（十字线长度）
     */
    void setSize(f32 size) { m_size = size; }

    /**
     * @brief 设置准星线宽
     */
    void setThickness(f32 thickness) { m_thickness = thickness; }

    /**
     * @brief 获取准星颜色
     */
    [[nodiscard]] u32 color() const { return m_color; }

    /**
     * @brief 获取准星大小
     */
    [[nodiscard]] f32 size() const { return m_size; }

    /**
     * @brief 获取准星线宽
     */
    [[nodiscard]] f32 thickness() const { return m_thickness; }

private:
    u32 m_color = 0xFFFFFFFF; ///< 白色，完全不透明
    f32 m_size = 10.0f;       ///< 十字线长度
    f32 m_thickness = 1.0f;   ///< 线宽
};

} // namespace mc::client::ui::minecraft::widgets
