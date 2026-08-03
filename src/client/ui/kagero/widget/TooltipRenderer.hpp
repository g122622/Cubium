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

#include "Tooltip.hpp"
#include "client/ui/kagero/Types.hpp"
#include "client/ui/kagero/paint/PaintContext.hpp"
#include "common/core/Types.hpp"
#include <algorithm>
#include <cmath>
#include <utility>

namespace mc::client::ui::kagero::widget {

/**
 * @brief Tooltip 渲染工具
 *
 * 提供工具方法来渲染 Minecraft 风格的工具提示框。
 * 使用 PaintContext 的基本绘图原语绘制背景、边框和文本。
 *
 * 渲染风格：
 * - 背景：半透明深色 (0xF0100010)
 * - 边框：紫色 (0x505000FF)
 * - 文本：白色带阴影 (0xFFFFFFFF)
 * - 内边距：4px
 * - 鼠标偏移：12px
 *
 * 使用示例：
 * @code
 * TooltipRenderer::render(ctx, tooltip, mouseX, mouseY, canvasWidth, canvasHeight);
 * @endcode
 */
class TooltipRenderer {
public:
    /**
     * @brief Tooltip 渲染常量
     */
    static constexpr f32 PADDING = 4.0f;       ///< 内边距
    static constexpr f32 MOUSE_OFFSET = 12.0f; ///< 鼠标位置偏移
    static constexpr f32 MIN_POSITION = 4.0f;  ///< 最小位置（避免贴边）

    /**
     * @brief Tooltip 颜色常量，与 HudColors 保持一致
     */
    static constexpr u32 BACKGROUND_COLOR = 0xF0100010; ///< 半透明深色背景
    static constexpr u32 BORDER_COLOR = 0x505000FF;     ///< 紫色边框
    static constexpr u32 TEXT_COLOR = 0xFFFFFFFF;       ///< 白色文本

    /**
     * @brief 计算 Tooltip 所需的尺寸
     * @param ctx 绘图上下文（用于文本测量）
     * @param tooltip Tooltip 数据
     * @return Tooltip 尺寸（宽度, 高度），包含内边距
     */
    [[nodiscard]] static std::pair<f32, f32> measure(PaintContext& ctx, const Tooltip& tooltip)
    {
        if (tooltip.isEmpty()) {
            return {0.0f, 0.0f};
        }

        f32 maxTextWidth = 0.0f;
        const u32 fontHeight = ctx.getFontHeight();

        for (const auto& line : tooltip.lines()) {
            const f32 lineWidth = ctx.getTextWidth(line);
            if (lineWidth > maxTextWidth) {
                maxTextWidth = lineWidth;
            }
        }

        const f32 width = maxTextWidth + PADDING * 2.0f;
        const f32 height = static_cast<f32>(tooltip.lineCount()) * static_cast<f32>(fontHeight) + PADDING * 2.0f;

        return {width, height};
    }

    /**
     * @brief 计算 Tooltip 的渲染位置
     *
     * 默认在鼠标右下方偏移 MOUSE_OFFSET，超出边界时自动翻转：
     * - 超出屏幕右侧时翻转到鼠标左方
     * - 超出屏幕底部时翻转到鼠标上方
     * - 位置不低于 MIN_POSITION（避免贴边）
     *
     * @param mouseX 鼠标 X 坐标
     * @param mouseY 鼠标 Y 坐标
     * @param tooltipWidth Tooltip 宽度
     * @param tooltipHeight Tooltip 高度
     * @param canvasWidth 画布/屏幕宽度
     * @param canvasHeight 画布/屏幕高度
     * @return Tooltip 左上角位置 (x, y)
     */
    [[nodiscard]] static std::pair<f32, f32> positionTooltip(
        f32 mouseX, f32 mouseY, f32 tooltipWidth, f32 tooltipHeight, f32 canvasWidth, f32 canvasHeight)
    {
        f32 x = mouseX + MOUSE_OFFSET;
        f32 y = mouseY + MOUSE_OFFSET;

        // 超出右侧边界时翻转到鼠标左方
        if (x + tooltipWidth > canvasWidth) {
            x = mouseX - MOUSE_OFFSET - tooltipWidth;
        }

        // 超出底部边界时翻转到鼠标上方
        if (y + tooltipHeight > canvasHeight) {
            y = mouseY - MOUSE_OFFSET - tooltipHeight;
        }

        // 确保不贴边
        x = std::max(MIN_POSITION, x);
        y = std::max(MIN_POSITION, y);

        return {x, y};
    }

    /**
     * @brief 渲染 Tooltip
     *
     * 使用 PaintContext 绘制 Minecraft 风格的 Tooltip：
     * 1. 绘制半透明深色背景矩形
     * 2. 绘制紫色边框
     * 3. 逐行绘制白色文本
     *
     * @param ctx 绘图上下文
     * @param tooltip Tooltip 数据
     * @param mouseX 鼠标 X 坐标
     * @param mouseY 鼠标 Y 坐标
     * @param canvasWidth 画布/屏幕宽度（用于位置计算）
     * @param canvasHeight 画布/屏幕高度（用于位置计算）
     */
    static void render(
        PaintContext& ctx, const Tooltip& tooltip, f32 mouseX, f32 mouseY, f32 canvasWidth, f32 canvasHeight)
    {
        if (tooltip.isEmpty()) {
            return;
        }

        const auto [width, height] = measure(ctx, tooltip);
        if (width <= 0.0f || height <= 0.0f) {
            return;
        }

        const auto [x, y] = positionTooltip(mouseX, mouseY, width, height, canvasWidth, canvasHeight);

        // 绘制背景
        ctx.drawFilledRect(
            Rect(static_cast<i32>(x), static_cast<i32>(y), static_cast<i32>(width), static_cast<i32>(height)),
            BACKGROUND_COLOR);

        // 绘制边框
        ctx.drawBorder(
            Rect(static_cast<i32>(x), static_cast<i32>(y), static_cast<i32>(width), static_cast<i32>(height)),
            1.0f,
            BORDER_COLOR);

        // 逐行绘制文本
        const f32 textX = x + PADDING;
        f32 textY = y + PADDING;
        const u32 fontHeight = ctx.getFontHeight();

        for (const auto& line : tooltip.lines()) {
            ctx.drawText(line, static_cast<i32>(textX), static_cast<i32>(textY), TEXT_COLOR);
            textY += static_cast<f32>(fontHeight);
        }
    }
};

} // namespace mc::client::ui::kagero::widget
