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

#include "ConsoleLayout.hpp"

#include "CommandLineComponent.hpp"
#include "CompletionPopupComponent.hpp"
#include "LogViewComponent.hpp"
#include "StatusPanelComponent.hpp"

#include <algorithm>
#include <string>
#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>

namespace mc::server::console {

namespace {

using namespace ftxui;

/// 标题栏展示的按键提示。
constexpr const char* HINT = " Enter run   Tab complete   PgUp/PgDn scroll   Ctrl+Q quit ";

/// 固定占用的行数：标题栏 1 + 分隔线 1 + 输入框 1。
constexpr i32 FIXED_ROWS = 3;

} // namespace

ConsoleLayout::ConsoleLayout(LogViewComponent& logView,
    StatusPanelComponent& statusPanel,
    CommandLineComponent& commandLine,
    CompletionPopupComponent& completionPopup,
    std::string title)
    : m_logView(logView)
    , m_statusPanel(statusPanel)
    , m_commandLine(commandLine)
    , m_completionPopup(completionPopup)
    , m_title(std::move(title))
{}

i32 ConsoleLayout::overlayHeight() const
{
    return m_completionPopup.visibleRowCount();
}

ftxui::Element ConsoleLayout::OnRender()
{
    // 标题栏右侧的滚动状态：日志区不会自动折行/滚动，内容溢出时用户必须知道
    // "上面还有未显示的日志"，否则会误以为服务端没有输出。
    ftxui::Elements hints;
    if (m_logView.hasHiddenOlderLines()) {
        hints.push_back(text("PgUp/PgDn or wheel to scroll | "));
    }
    hints.push_back(text(HINT));

    auto header = hbox({
                      text(" " + m_title) | bold | color(Color::GreenLight),
                      filler(),
                      hbox(std::move(hints)) | dim,
                  }) |
        bgcolor(Color::GrayDark);

    auto logArea = m_logView.OnRender() | xflex;
    auto panel = m_statusPanel.OnRender() | size(WIDTH, EQUAL, PANEL_WIDTH);

    auto body = hbox({
                    logArea,
                    separator(),
                    panel,
                }) |
        yflex;

    // 候选列表只在有候选时占高度（v6 的 Maybe 在条件为假时直接不渲染资源）。
    const bool showPopup = m_commandLine.completionActive();
    Element popup = showPopup ? m_completionPopup.OnRender() : emptyElement();

    Element main = vbox({
        header,
        body,
        separator(),
        std::move(popup),
        m_commandLine.OnRender(),
    });

    // 渲染时把日志区可见尺寸告知日志区：行数 = 终端总行数减去固定行与候选列表高度；
    // 宽度 = 正文区宽度减去面板与分隔线（FTXUI 的 text() 不折行，日志必须自己折）。
    const i32 totalRows = m_box.y_max - m_box.y_min + 1;
    const i32 logRows = std::max<i32>(totalRows - FIXED_ROWS - overlayHeight(), 1);
    if (totalRows > 0) {
        m_logView.setViewportRows(static_cast<usize>(logRows));
    }
    const i32 totalColumns = m_box.x_max - m_box.x_min + 1;
    if (totalColumns > 0) {
        m_logView.setWrapWidth(static_cast<usize>(std::max<i32>(totalColumns - PANEL_WIDTH - 1, 8)));
    }

    return main | reflect(m_box);
}

bool ConsoleLayout::OnEvent(ftxui::Event event)
{
    // 输入框优先：它承载编辑、历史、补全键位（含 Tab/↑/↓/Esc/Ctrl+*）。
    if (m_commandLine.OnEvent(event)) {
        return true;
    }

    // 右侧面板的鼠标点击（`[refresh panel]` 按钮）：必须在日志区之前判定，
    // 否则同一鼠标事件会被当作"点在不属于日志区的位置"而丢弃。
    if (event.is_mouse() && m_statusPanel.OnEvent(event)) {
        return true;
    }

    // 日志回看：PgUp/PgDn/滚轮（Home/End 留给输入框）。输入框对 PgUp/PgDn 无动作，
    // 因此这里不会抢走编辑键。
    if (event == Event::PageUp || event == Event::PageDown || event.is_mouse()) {
        if (m_logView.handleScrollEvent(event)) {
            return true;
        }
    }

    return false;
}

} // namespace mc::server::console
