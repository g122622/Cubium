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

#include "TargetInfoWidget.hpp"
#include "client/ui/kagero/Types.hpp"
#include "client/ui/kagero/paint/PaintContext.hpp"
#include "client/ui/kagero/widget/Widget.hpp"
#include "client/ui/minecraft/targetinfo/TargetInfo.hpp"
#include "common/core/Types.hpp"

#include <algorithm>
#include <utility>

namespace mc::client::ui::minecraft::targetinfo {

namespace {

constexpr i32 PANEL_TOP_MARGIN = 18;
constexpr i32 PANEL_SIDE_MARGIN = 8;
constexpr i32 PANEL_LEFT_PADDING = 10;
constexpr i32 PANEL_RIGHT_PADDING = 10;
constexpr i32 PANEL_TOP_PADDING = 8;
constexpr i32 PANEL_BOTTOM_PADDING = 8;
constexpr i32 PANEL_LINE_GAP = 2;
constexpr i32 PANEL_TITLE_GAP = 4;
constexpr i32 PANEL_ACCENT_WIDTH = 4;
constexpr u32 BACKGROUND_COLOR = 0xDA141414;
constexpr u32 BORDER_COLOR = 0xFF4A4A4A;
constexpr u32 TITLE_COLOR = 0xFFFFFFFF;
constexpr u32 DETAIL_COLOR = 0xFFD4D4D4;
constexpr u32 SHADOW_COLOR = 0xFF000000;

} // namespace

TargetInfoWidget::TargetInfoWidget()
    : Widget("target_info")
    , m_targetInfo(TargetInfoSnapshot::none())
{
    setVisible(true);
    setActive(true);
}

void TargetInfoWidget::setTargetInfo(TargetInfoSnapshot targetInfo)
{
    m_targetInfo = std::move(targetInfo);
}

void TargetInfoWidget::paint(kagero::widget::PaintContext& ctx)
{
    if (!isVisible() || !m_targetInfo.hasTarget()) {
        return;
    }

    const auto& details = m_targetInfo.details();
    const i32 fontHeight = static_cast<i32>(ctx.getFontHeight());
    const i32 lineHeight = fontHeight + PANEL_LINE_GAP;
    const i32 titleHeight = fontHeight;

    f32 contentWidth = ctx.getTextWidth(m_targetInfo.title());
    for (const auto& line : details) {
        contentWidth = std::max(contentWidth, ctx.getTextWidth(line));
    }

    const i32 panelWidth =
        static_cast<i32>(contentWidth) + PANEL_LEFT_PADDING + PANEL_RIGHT_PADDING + PANEL_ACCENT_WIDTH;
    const i32 panelHeight = PANEL_TOP_PADDING + titleHeight + PANEL_BOTTOM_PADDING +
        (details.empty() ? 0 : PANEL_TITLE_GAP + static_cast<i32>(details.size()) * lineHeight);

    const i32 screenWidth = static_cast<i32>(width());
    const i32 panelX = std::clamp((screenWidth - panelWidth) / 2,
        PANEL_SIDE_MARGIN,
        std::max(PANEL_SIDE_MARGIN, screenWidth - panelWidth - PANEL_SIDE_MARGIN));
    const i32 panelY = PANEL_TOP_MARGIN;

    const kagero::Rect panelBounds(panelX, panelY, panelWidth, panelHeight);
    ctx.drawRoundedRect(panelBounds, 4.0f, BACKGROUND_COLOR);
    ctx.drawBorder(panelBounds, 1.0f, BORDER_COLOR);
    ctx.drawFilledRect(
        kagero::Rect(panelX, panelY + 1, PANEL_ACCENT_WIDTH, panelHeight - 2), m_targetInfo.accentColor());

    const i32 textX = panelX + PANEL_ACCENT_WIDTH + PANEL_LEFT_PADDING;
    i32 textY = panelY + PANEL_TOP_PADDING;

    ctx.drawText(m_targetInfo.title(), textX + 1, textY + 1, SHADOW_COLOR);
    ctx.drawText(m_targetInfo.title(), textX, textY, TITLE_COLOR);
    textY += titleHeight;

    if (!details.empty()) {
        textY += PANEL_TITLE_GAP;
        for (const auto& line : details) {
            ctx.drawText(line, textX + 1, textY + 1, SHADOW_COLOR);
            ctx.drawText(line, textX, textY, DETAIL_COLOR);
            textY += lineHeight;
        }
    }
}

} // namespace mc::client::ui::minecraft::targetinfo