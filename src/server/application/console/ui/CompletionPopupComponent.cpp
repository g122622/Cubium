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

#include "CompletionPopupComponent.hpp"

#include <fmt/format.h>

#include <algorithm>
#include <string>
#include <ftxui/dom/elements.hpp>

namespace mc::server::console {

CompletionPopupComponent::CompletionPopupComponent(const CompletionState& state)
    : m_state(state)
{}

i32 CompletionPopupComponent::visibleRowCount() const
{
    if (!m_state.active()) {
        return 0;
    }
    const usize rows = std::min(m_state.candidates().size(), MAX_VISIBLE_ROWS);
    return static_cast<i32>(rows) + 1; // 候选行 + 按键提示行
}

ftxui::Element CompletionPopupComponent::OnRender()
{
    if (!m_state.active()) {
        return ftxui::emptyElement();
    }

    const auto& candidates = m_state.candidates();
    const usize count = candidates.size();
    const i32 selected = std::clamp(m_state.selectedIndex(), 0, static_cast<i32>(count) - 1);

    // 让高亮项始终落在可见窗口内。
    usize windowStart = 0;
    if (count > MAX_VISIBLE_ROWS) {
        const usize selectedIndex = static_cast<usize>(selected);
        const usize half = MAX_VISIBLE_ROWS / 2;
        windowStart = selectedIndex > half ? selectedIndex - half : 0;
        windowStart = std::min(windowStart, count - MAX_VISIBLE_ROWS);
    }
    const usize windowEnd = std::min(count, windowStart + MAX_VISIBLE_ROWS);

    ftxui::Elements rows;
    rows.reserve(windowEnd - windowStart);
    for (usize index = windowStart; index < windowEnd; ++index) {
        const auto& candidate = candidates[index];
        auto row = ftxui::text(" " + candidate.text);
        if (static_cast<i32>(index) == selected) {
            row = row | ftxui::inverted;
        }
        if (!candidate.tooltip.empty()) {
            row = ftxui::hbox({row, ftxui::text("  " + candidate.tooltip) | ftxui::dim}) | ftxui::xflex;
        }
        rows.push_back(std::move(row));
    }

    std::string hint =
        fmt::format(" Tab accept   Up/Down cycle   Esc dismiss   {}/{} ", static_cast<usize>(selected) + 1, count);

    return ftxui::vbox({
        ftxui::vbox(std::move(rows)),
        ftxui::text(hint) | ftxui::dim,
    });
}

} // namespace mc::server::console
