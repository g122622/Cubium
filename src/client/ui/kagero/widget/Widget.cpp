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

#include "Widget.hpp"
#include "TooltipRenderer.hpp"
#include "common/core/Types.hpp"
#include <chrono>

namespace mc::client::ui::kagero::widget {

void Widget::refreshTooltip(PaintContext& ctx, f32 canvasWidth, f32 canvasHeight)
{
    if (m_tooltip.isEmpty()) {
        m_tooltipWasDisplayed = false;
        return;
    }

    const bool shouldShow = isHovered();
    if (shouldShow != m_tooltipWasDisplayed) {
        if (shouldShow) {
            m_tooltipDisplayStartTime = std::chrono::steady_clock::now();
        }
        m_tooltipWasDisplayed = shouldShow;
    }

    if (!shouldShow) {
        return;
    }

    // 检查延迟
    if (m_tooltipDelayMs > 0) {
        const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - m_tooltipDisplayStartTime)
                                 .count();
        if (elapsed < m_tooltipDelayMs) {
            return;
        }
    }

    // 渲染 Tooltip（使用 updateHover 中记录的鼠标位置）
    TooltipRenderer::render(
        ctx, m_tooltip, static_cast<f32>(m_lastMouseX), static_cast<f32>(m_lastMouseY), canvasWidth, canvasHeight);
}

} // namespace mc::client::ui::kagero::widget
