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

#include "common/command/suggestions/Suggestions.hpp"
#include "common/core/Types.hpp"
#include "server/application/console/input/CommandCompletion.hpp"

#include <functional>
#include <string>
#include <vector>
#include <ftxui/component/component_base.hpp>
#include <ftxui/screen/box.hpp>

namespace mc::server::console {

/// 历史记录最多保留的条数。
inline constexpr usize MAX_HISTORY_ENTRIES = 500;

/**
 * @brief 底部命令输入框
 *
 * 以 ftxui::Input 为主体（自带行编辑、Home/End、Ctrl+←/→ 按词移动、鼠标点击定位），
 * 在其上叠加：
 * - ↑/↓ 历史（输入框有内容且未在翻历史时，↑/↓ 仍然翻历史，符合控制台习惯）；
 * - Ctrl+A/E/U/K/W 等 Emacs 键位（Input 未实现，这里补齐）；
 * - Tab/Shift+Tab 补全：唯一候选直接写入，多候选交给 CompletionState 高亮、Popup 展示，
 *   再按 Tab 采纳、↑/↓ 切换、Esc 关闭；
 * - Enter 提交：以 '/' 开头作为命令，否则包装为 say。
 *
 * 粘贴的多行文本在提交前被规范成单行（见 normalizePastedText）。
 */
class CommandLineComponent final : public ftxui::ComponentBase {
public:
    /**
     * @brief 提交回调
     * @param command 不含前导 '/' 的命令文本
     */
    using SubmitHandler = std::function<void(const std::string& command)>;

    /**
     * @brief 补全查询回调
     *
     * 在 UI 线程同步执行；实现方内部自行与 tick 线程交接（ServerConsole 用命令队列
     * 的查询通道完成，保证命令树只在 tick 线程被访问）。返回空表示无候选。
     *
     * @param input 当前输入
     * @param cursor 光标位置（字节偏移）
     * @return 服务端补全结果
     */
    using CompletionQuery = std::function<mc::command::Suggestions(const std::string& input, i32 cursor)>;

    /**
     * @brief 构造输入框
     * @param submit 提交回调（不得为空）
     * @param completions 补全候选提供者（可为空 = 不启用 Tab 补全）
     */
    CommandLineComponent(SubmitHandler submit, CompletionQuery completions);

    ftxui::Element OnRender() override;
    bool OnEvent(ftxui::Event event) override;

    /**
     * @brief 让输入框重新持有焦点
     *
     * 输入框是终端中唯一的文本输入目标，任何时刻都应持有焦点（面板按钮被点击后调用本
     * 方法把焦点抢回来），否则后续按键会落空。
     */
    void takeFocus();

    /**
     * @brief 以编程方式填入输入文本
     *
     * 供面板内的 `[refresh panel]` 按钮使用：点击等价于在输入框里敲好 `panel` 命令、
     * 光标停在行尾，用户按 Enter 即可提交，无需自己输入。
     *
     * @param input 输入文本（含 '/' 前缀）
     */
    void setInput(std::string input);

    /**
     * @brief 当前输入文本
     */
    [[nodiscard]] const std::string& text() const noexcept
    {
        return m_text;
    }

    /**
     * @brief 补全候选列表是否打开
     */
    [[nodiscard]] bool completionActive() const noexcept
    {
        return m_completion.active();
    }

    /**
     * @brief 补全状态（供候选列表组件读取高亮项与候选）
     */
    [[nodiscard]] const CompletionState& completionState() const noexcept
    {
        return m_completion;
    }

    /**
     * @brief 历史记录（由新到旧可读；内部为按提交顺序保存）
     */
    [[nodiscard]] const std::vector<std::string>& history() const noexcept
    {
        return m_history;
    }

    /**
     * @brief 追加一条历史（供外部注入，如测试或脚本）
     */
    void addHistoryEntry(std::string entry);

    /**
     * @brief 外部触发补全（供 ConsoleLayout 的 Tab 转发复用）
     * @return 是否产生了候选
     */
    bool triggerCompletion();

private:
    /**
     * @brief 提交当前输入
     */
    void _submit();

    /**
     * @brief 历史上下翻
     * @param delta -1 = 更近（新），+1 = 更旧
     */
    void _browseHistory(i32 delta);

    /**
     * @brief 应用 Emacs 键位
     * @return 是否已处理
     */
    bool _handleEmacsKey(const ftxui::Event& event);

    /**
     * @brief 处理补全相关的按键（Tab / ↑ / ↓ / Esc）
     * @return 是否已处理
     */
    bool _handleCompletionKey(const ftxui::Event& event);

    /**
     * @brief 输入变化后让过期候选失效（关闭候选列表）
     */
    void _closeCompletion();

    /**
     * @brief 把编辑器内容同步回本组件的输入状态
     */
    void _syncFromInput();

    SubmitHandler m_submit;
    CompletionQuery m_completionQuery;

    std::string m_text;
    i32 m_cursor = 0;

    std::vector<std::string> m_history;
    /// 历史浏览位置：0 表示正在编辑新输入，>0 表示正在查看倒数第 N 条历史。
    usize m_historyPosition = 0;
    /// 开始浏览历史时暂存的"未提交输入"，回到 0 时恢复。
    std::string m_pendingInput;

    CompletionState m_completion;

    ftxui::Component m_input;

    /// 输入行在屏幕上的包围盒（渲染时由 reflect 回填）。
    ftxui::Box m_box;
};

} // namespace mc::server::console
