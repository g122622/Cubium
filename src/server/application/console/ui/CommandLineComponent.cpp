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

#include "CommandLineComponent.hpp"

#include "common/util/assert/AssertAll.hpp"
#include "server/application/console/ui/TerminalUi.hpp"

#include <algorithm>
#include <utility>
#include <ftxui/component/component.hpp>
#include <ftxui/component/component_options.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>

namespace mc::server::console {

CommandLineComponent::CommandLineComponent(SubmitHandler submit, CompletionQuery completions)
    : m_submit(std::move(submit))
    , m_completionQuery(std::move(completions))
{
    MC_ASSERT_RELEASE_MSG(m_submit != nullptr, "CommandLineComponent requires a submit handler");

    ftxui::InputOption option;
    option.content = &m_text;
    option.cursor_position = &m_cursor;
    option.placeholder = "Type a command (/...) or chat; Tab completes";
    option.multiline = false;
    m_input = ftxui::Input(option);
}

ftxui::Element CommandLineComponent::OnRender()
{
    auto prompt = ftxui::text("> ") | ftxui::bold | ftxui::color(ftxui::Color::GreenLight);
    auto editor = m_input->Render() | ftxui::xflex;
    return ftxui::hbox({std::move(prompt), std::move(editor)}) | ftxui::reflect(m_box);
}

bool CommandLineComponent::OnEvent(ftxui::Event event)
{
    // 0. Ctrl+C / Ctrl+Q：退出 UI（终端模式下 Ctrl+C 被 FTXUI 当作按键而非 SIGINT，
    //    这里显式转发为退出请求，服务端随后走正常关服流程）。提示文本用 Ctrl+Q 避免歧义。
    if (event == ftxui::Event::CtrlC || event == ftxui::Event::CtrlQ) {
        TerminalUi::requestExit();
        return true;
    }

    // 1. 补全列表打开时：Tab 采纳、↑/↓ 切换、Esc 关闭。
    if (m_completion.active() && _handleCompletionKey(event)) {
        return true;
    }

    // 2. 输入框自身的编辑逻辑（含 Return → on_enter 的等价：这里直接拦截 Return）。
    if (event == ftxui::Event::Return) {
        _syncFromInput();
        _submit();
        return true;
    }

    // 3. Emacs 键位：Input 未内建，这里补齐。
    if (_handleEmacsKey(event)) {
        _syncFromInput();
        return true;
    }

    // 4. 历史：↑ 翻旧、↓ 翻新。Input 在单行模式下对 ↑/↓ 无动作，不会冲突。
    if (event == ftxui::Event::ArrowUp) {
        _syncFromInput();
        _browseHistory(+1);
        return true;
    }
    if (event == ftxui::Event::ArrowDown) {
        _syncFromInput();
        _browseHistory(-1);
        return true;
    }

    // 5. Tab / Shift+Tab：首次触发补全，已有候选时采纳。
    if (event == ftxui::Event::Tab || event == ftxui::Event::TabReverse) {
        _syncFromInput();
        if (m_completion.active()) {
            if (m_completion.applySelected(m_text, m_cursor)) {
                m_completion.clear();
                m_historyPosition = 0;
            }
        } else {
            triggerCompletion();
        }
        return true;
    }

    // 6. 其余按键交给 Input。内容若变化则关闭过期候选。
    const std::string before = m_text;
    const bool handled = m_input->OnEvent(event);
    if (handled) {
        _syncFromInput();
        if (m_text != before) {
            _closeCompletion();
        }
        m_historyPosition = 0;
    }
    return handled;
}

void CommandLineComponent::takeFocus()
{
    // 本组件自身不是子容器：让 ftxui::Input 直接成为顶层组件的焦点。
    m_input->TakeFocus();
}

void CommandLineComponent::setInput(std::string input)
{
    m_text = std::move(input);
    m_cursor = static_cast<i32>(m_text.size());
    m_completion.clear();
    m_historyPosition = 0;
}

void CommandLineComponent::addHistoryEntry(std::string entry)
{
    if (entry.empty()) {
        return;
    }
    if (!m_history.empty() && m_history.back() == entry) {
        return; // 连续重复的命令不重复记入历史
    }
    m_history.push_back(std::move(entry));
    if (m_history.size() > MAX_HISTORY_ENTRIES) {
        m_history.erase(m_history.begin());
    }
    m_historyPosition = 0;
}

bool CommandLineComponent::triggerCompletion()
{
    m_completion.clear();
    if (m_completionQuery == nullptr) {
        return false;
    }

    std::string raw = m_text;
    i32 cursor = std::clamp(m_cursor, 0, static_cast<i32>(raw.size()));
    // 补全以光标前缀为准；光标不在行尾时，尾巴不参与解析。
    raw.erase(static_cast<usize>(cursor));

    // 以 '/' 开头的才按命令补全；纯聊天输入没有候选。
    if (!isCommandInput(raw)) {
        return false;
    }

    const mc::command::Suggestions suggestions = m_completionQuery(raw, cursor);
    const usize count = m_completion.reset(raw, suggestions);
    if (count == 0) {
        return false;
    }

    // 唯一候选：直接写入，等价于 shell 的"无歧义自动补全"。
    if (count == 1) {
        m_completion.applySelected(m_text, m_cursor);
        m_completion.clear();
        return true;
    }
    return true;
}

void CommandLineComponent::_syncFromInput()
{
    // InputOption 直接引用 m_text / m_cursor，理论上无需同步；此处保留一次夹取，
    // 保证光标落在合法范围内（鼠标点击、粘贴后可能出现越界值）。
    m_cursor = std::clamp(m_cursor, 0, static_cast<i32>(m_text.size()));
}

void CommandLineComponent::_closeCompletion()
{
    m_completion.clear();
}

void CommandLineComponent::_submit()
{
    std::string input = normalizePastedText(m_text);
    // 去掉首尾空白：空行不提交。
    const usize begin = input.find_first_not_of(' ');
    if (begin == std::string::npos) {
        m_text.clear();
        m_cursor = 0;
        return;
    }
    const usize end = input.find_last_not_of(' ');
    input = input.substr(begin, end - begin + 1);

    std::string command;
    if (isCommandInput(input)) {
        // 命令文本不需要 '/' 前缀：CommandDispatcher::parse 会自行剥离。
        const usize slash = input.find('/');
        command = input.substr(slash + 1);
    } else {
        command = toSayCommand(input);
    }

    addHistoryEntry(input);
    m_text.clear();
    m_cursor = 0;
    m_completion.clear();

    if (!command.empty()) {
        m_submit(command);
    }
}

void CommandLineComponent::_browseHistory(i32 delta)
{
    if (m_history.empty()) {
        return;
    }

    const i32 size = static_cast<i32>(m_history.size());
    i32 position = static_cast<i32>(m_historyPosition) + delta;
    position = std::clamp(position, 0, size);

    // 从"正在编辑新输入"进入历史时，先暂存当前输入，回到底部时恢复。
    if (m_historyPosition == 0 && position > 0) {
        m_pendingInput = m_text;
    }

    m_historyPosition = static_cast<usize>(position);
    if (m_historyPosition == 0) {
        m_text = m_pendingInput;
    } else {
        m_text = m_history[static_cast<usize>(size - position)];
    }
    m_cursor = static_cast<i32>(m_text.size());
    m_completion.clear();
}

bool CommandLineComponent::_handleCompletionKey(const ftxui::Event& event)
{
    if (event == ftxui::Event::ArrowUp) {
        m_completion.selectPrevious();
        return true;
    }
    if (event == ftxui::Event::ArrowDown) {
        m_completion.selectNext();
        return true;
    }
    if (event == ftxui::Event::Escape) {
        m_completion.clear();
        return true;
    }
    if (event == ftxui::Event::Tab || event == ftxui::Event::TabReverse) {
        if (m_completion.applySelected(m_text, m_cursor)) {
            m_completion.clear();
            m_historyPosition = 0;
        }
        return true;
    }
    return false;
}

bool CommandLineComponent::_handleEmacsKey(const ftxui::Event& event)
{
    if (event == ftxui::Event::CtrlA) {
        m_cursor = 0;
        return true;
    }
    if (event == ftxui::Event::CtrlE) {
        m_cursor = static_cast<i32>(m_text.size());
        return true;
    }
    if (event == ftxui::Event::CtrlU) {
        m_text.erase(0, static_cast<usize>(m_cursor));
        m_cursor = 0;
        return true;
    }
    if (event == ftxui::Event::CtrlK) {
        if (static_cast<usize>(m_cursor) < m_text.size()) {
            m_text.erase(static_cast<usize>(m_cursor));
        }
        return true;
    }
    if (event == ftxui::Event::CtrlW) {
        // 删除光标前的一个"词"：先跳过尾部空白，再删到下一个空白。
        usize position = static_cast<usize>(std::clamp(m_cursor, 0, static_cast<i32>(m_text.size())));
        while (position > 0 && m_text[position - 1] == ' ') {
            --position;
        }
        while (position > 0 && m_text[position - 1] != ' ') {
            --position;
        }
        m_text.erase(position, static_cast<usize>(std::clamp(m_cursor, 0, static_cast<i32>(m_text.size()))) - position);
        m_cursor = static_cast<i32>(position);
        return true;
    }
    return false;
}

} // namespace mc::server::console
