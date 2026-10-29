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

#include "LogViewComponent.hpp"

#include "server/application/console/render/LineWrapper.hpp"

#include <algorithm>
#include <utility>
#include <ftxui/component/event.hpp>
#include <ftxui/component/mouse.hpp>
#include <ftxui/dom/elements.hpp>

namespace mc::server::console {

namespace {

/// 回看时每次滚轮滚动的行数。
constexpr usize WHEEL_STEP = 3;

/// 事件行在日志区中的前缀标记，便于与普通日志区分。
const char* _eventPrefix(TerminalEventKind kind)
{
    switch (kind) {
        case TerminalEventKind::Chat:
            return "[chat] ";
        case TerminalEventKind::Player:
            return "[player] ";
        case TerminalEventKind::Command:
            return "[cmd] ";
        case TerminalEventKind::Error:
            return "[error] ";
        case TerminalEventKind::Game:
            return "[game] ";
        case TerminalEventKind::Info:
            break;
    }
    return "[info] ";
}

ftxui::Color _eventColor(TerminalEventKind kind)
{
    switch (kind) {
        case TerminalEventKind::Chat:
            return ftxui::Color::CyanLight;
        case TerminalEventKind::Player:
            return ftxui::Color::GreenLight;
        case TerminalEventKind::Command:
            return ftxui::Color::BlueLight;
        case TerminalEventKind::Error:
            return ftxui::Color::RedLight;
        case TerminalEventKind::Game:
            return ftxui::Color::MagentaLight;
        case TerminalEventKind::Info:
            break;
    }
    return ftxui::Color::Default;
}

} // namespace

LogViewComponent::LogViewComponent(const LogRingBuffer& logs, const EventRingBuffer& events)
    : m_logs(logs)
    , m_events(events)
{}

void LogViewComponent::refreshIfDirty()
{
    const u64 logCount = m_logs.pushedCount();
    const u64 eventCount = m_events.pushedCount();
    if (logCount != m_observedLogCount || eventCount != m_observedEventCount) {
        m_observedLogCount = logCount;
        m_observedEventCount = eventCount;

        const auto logLines = m_logs.snapshot();
        const auto events = m_events.snapshot();

        m_compactLines.clear();
        m_compactLines.reserve(logLines.size() + events.size());
        for (const auto& line : logLines) {
            m_compactLines.emplace_back(TerminalEventKind::Info, expandTabs(line.text));
        }
        m_compactLogLineCount = m_compactLines.size();
        for (const auto& event : events) {
            std::string text = _eventPrefix(event.kind);
            text += event.text;
            m_compactLines.emplace_back(event.kind, expandTabs(text));
        }

        _rewrap(/*preserveViewport=*/true);
    }

    // 终端宽度变化：内容没变，但显示行数变了，必须重折行。
    if (m_wrappedWidth != m_wrapWidth || m_wrappedFromCount != m_compactLines.size()) {
        _rewrap(/*preserveViewport=*/true);
    }
}

void LogViewComponent::_rewrap(bool preserveViewport)
{
    // 太窄时不做无意义折行（每行只剩几个字符），交给终端自身的折行。
    constexpr usize MIN_WRAP_WIDTH = 8;
    const usize width = std::max(m_wrapWidth, MIN_WRAP_WIDTH);

    const usize previousCount = m_wrappedLines.size();

    m_wrappedLines.clear();
    m_wrappedLines.reserve(m_compactLines.size());
    for (usize index = 0; index < m_compactLines.size(); ++index) {
        const auto& [kind, text] = m_compactLines[index];
        const bool isEvent = index >= m_compactLogLineCount;
        for (auto& piece : wrapByDisplayWidth(text, width)) {
            // 折行后每行都记住所属事件类别，着色与原逻辑一致。
            m_wrappedLines.emplace_back(isEvent, kind, std::move(piece));
        }
    }
    m_wrappedFromCount = m_compactLines.size();
    m_wrappedWidth = m_wrapWidth;

    // 回看时保持"距底部固定行数"的观感：新增显示行不会把视野顶走。
    if (preserveViewport && !m_following) {
        const usize count = _contentLineCount();
        m_scrollOffset += count >= previousCount ? count - previousCount : 0;
    }
}

void LogViewComponent::setWrapWidth(usize columns)
{
    m_wrapWidth = columns;
}

ftxui::Element LogViewComponent::OnRender()
{
    refreshIfDirty();

    const usize lineCount = _contentLineCount();
    if (m_following) {
        m_scrollOffset = 0;
    }
    m_scrollOffset = std::min(m_scrollOffset, lineCount);

    // 贴底时展示最后 viewportRows 行；回看时整体上移 scrollOffset 行。
    const usize visible = m_viewportRows;
    const usize end = lineCount > m_scrollOffset ? lineCount - m_scrollOffset : 0;
    const usize begin = end > visible ? end - visible : 0;

    ftxui::Elements rows;
    rows.reserve(end - begin);
    for (usize index = begin; index < end; ++index) {
        const auto& [isEvent, kind, text] = m_wrappedLines[index];
        const auto color = isEvent ? _eventColor(kind) : ftxui::Color::Default;
        auto element = ftxui::text(text) | ftxui::color(color);
        if (isEvent && kind == TerminalEventKind::Error) {
            element = element | ftxui::bold;
        }
        rows.push_back(element);
    }

    if (rows.empty()) {
        rows.push_back(ftxui::text("(no logs yet)") | ftxui::dim);
    }

    // 内容量随缓冲变化：外层给多少高度就画多少（不额外撑高）。贴底由 begin 的取窗口
    // 逻辑保证。注意不要套 yframe：Frame 会把子节点盒撑到 min_y，导致内容溢出容器。
    return ftxui::vbox(std::move(rows)) | ftxui::reflect(m_box);
}

bool LogViewComponent::handleScrollEvent(const ftxui::Event& event)
{
    if (event.is_mouse()) {
        // v6 的 Event::mouse() 是非 const 成员，取非 const 引用来读坐标。
        const auto& mouse = const_cast<ftxui::Event&>(event).mouse();
        // 只有指针位于日志区时才响应滚轮：输入框/右侧面板上的滚轮不应带动日志。
        if (!m_box.Contain(mouse.x, mouse.y)) {
            return false;
        }
        if (mouse.button == ftxui::Mouse::WheelUp && mouse.motion == ftxui::Mouse::Pressed) {
            _scrollBy(static_cast<i64>(WHEEL_STEP));
            return true;
        }
        if (mouse.button == ftxui::Mouse::WheelDown && mouse.motion == ftxui::Mouse::Pressed) {
            _scrollBy(-static_cast<i64>(WHEEL_STEP));
            return true;
        }
        return false;
    }

    if (event == ftxui::Event::PageUp) {
        _scrollPage(1);
        return true;
    }
    if (event == ftxui::Event::PageDown) {
        _scrollPage(-1);
        return true;
    }
    if (event == ftxui::Event::Home) {
        // Home 跳到缓冲最旧一行；由外层决定是否让给输入框（输入框的 Home 是光标到行首）。
        m_following = false;
        m_scrollOffset = _contentLineCount();
        return true;
    }
    if (event == ftxui::Event::End) {
        _scrollToBottom();
        return true;
    }
    return false;
}

void LogViewComponent::_scrollBy(i64 delta)
{
    const i64 lineCount = static_cast<i64>(_contentLineCount());
    const i64 next = static_cast<i64>(m_scrollOffset) + delta;
    m_scrollOffset = static_cast<usize>(std::clamp<i64>(next, 0, lineCount));
    m_following = m_scrollOffset == 0;
}

void LogViewComponent::_scrollPage(i64 pages)
{
    const i64 step = static_cast<i64>(std::max<usize>(m_viewportRows, 1));
    _scrollBy(step * pages);
}

void LogViewComponent::_scrollToBottom()
{
    m_scrollOffset = 0;
    m_following = true;
}

} // namespace mc::server::console
