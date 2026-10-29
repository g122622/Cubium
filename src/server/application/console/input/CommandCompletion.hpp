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

#include <string>
#include <string_view>
#include <vector>

namespace mc::server::console {

/**
 * @brief 一条补全候选及其在输入中的替换范围
 */
struct CompletionCandidate {
    std::string text;     ///< 补全后的完整输入（已把建议套用到原输入）
    i32 replaceStart = 0; ///< 替换起点（原输入中的字节偏移）
    std::string tooltip;  ///< 提示文本（可能为空）
};

/**
 * @brief 补全候选的纯数据视图（与 FTXUI 解耦，便于单测）
 *
 * 负责三件事：把服务端 Suggestions 转成候选列表；维护当前高亮项；把高亮项写回输入文本。
 * 交互约定（已与需求方确认）：
 * - 只有一个候选：直接写入；
 * - 多个候选：由 UI 在输入框上方列出，↑/↓ 切换高亮，Tab 采纳；
 * - 输入变化或光标移动后候选失效。
 */
class CompletionState {
public:
    /// 单次最多展示/保存的候选数量（避免超长列表拖垮渲染）。
    static constexpr usize MAX_CANDIDATES = 64;

    /**
     * @brief 用服务端补全结果重建候选
     * @param input 触发补全时的完整输入
     * @param suggestions 服务端返回的建议
     * @return 候选数量（0 表示无候选，UI 应关闭列表）
     */
    usize reset(std::string_view input, const mc::command::Suggestions& suggestions);

    /**
     * @brief 清空候选
     */
    void clear();

    [[nodiscard]] bool active() const noexcept
    {
        return !m_candidates.empty();
    }

    [[nodiscard]] const std::vector<CompletionCandidate>& candidates() const noexcept
    {
        return m_candidates;
    }

    /**
     * @brief 当前高亮项下标
     */
    [[nodiscard]] i32 selectedIndex() const noexcept
    {
        return m_selected;
    }

    /**
     * @brief 上移高亮（循环）
     */
    void selectPrevious();

    /**
     * @brief 下移高亮（循环）
     */
    void selectNext();

    /**
     * @brief 把高亮候选写入输入
     * @param input 被就地修改的输入文本
     * @param cursor 被就地修改的光标位置（字节偏移）
     * @return 是否写入了候选（候选为空时返回 false）
     */
    bool applySelected(std::string& input, i32& cursor) const;

private:
    std::vector<CompletionCandidate> m_candidates;
    i32 m_selected = 0;
};

/**
 * @brief 把单行输入框中的粘贴内容规范化
 *
 * 输入框是单行语义：粘贴的多行文本用空格拼接，并去掉控制字符（保留可打印 UTF-8）。
 *
 * @param pasted 粘贴的原始文本
 * @return 规范化后的单行文本
 */
[[nodiscard]] std::string normalizePastedText(std::string_view pasted);

/**
 * @brief 判断输入是否应作为命令执行
 *
 * 以 '/' 开头为命令；否则视为聊天（由调用方包装成 /say）。
 *
 * @param input 输入文本
 */
[[nodiscard]] bool isCommandInput(std::string_view input);

/**
 * @brief 把非命令输入包装成 say 命令
 * @param input 玩家输入的聊天文本
 * @return "say <text>"；空输入返回空串
 */
[[nodiscard]] std::string toSayCommand(std::string_view input);

} // namespace mc::server::console
