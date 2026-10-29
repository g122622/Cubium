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

#include "common/core/Types.hpp"

#include <string>
#include <ftxui/component/component_base.hpp>

namespace mc::server::console {

class CommandLineComponent;
class CompletionPopupComponent;
class LogViewComponent;
class StatusPanelComponent;

/**
 * @brief 终端三区布局
 *
 *   ┌ 标题栏 ──────────────────────────────────────────────┐
 *   │ 日志滚动区                     │ 右侧信息面板          │
 *   │                                │                       │
 *   ├ 候选列表（有候选时）──────────────────────────────────┤
 *   └ 输入框 ──────────────────────────────────────────────┘
 *
 * 键位分发顺序：输入框（编辑/历史/补全）→ 日志回看（PgUp/PgDn/滚轮）。
 * 输入框始终持有焦点，因此 ↑/↓ 归输入框（历史/候选切换），PgUp/PgDn 归日志区。
 */
class ConsoleLayout final : public ftxui::ComponentBase {
public:
    /// 右侧面板宽度（列）。
    static constexpr i32 PANEL_WIDTH = 48;

    /**
     * @brief 构造布局
     * @param logView 日志区（须比本对象存活更久）
     * @param statusPanel 右侧面板（须比本对象存活更久）
     * @param commandLine 输入框（须比本对象存活更久）
     * @param completionPopup 候选列表（须比本对象存活更久）
     * @param title 标题栏文本（如 "Cubium Server 0.1.0"）
     */
    ConsoleLayout(LogViewComponent& logView,
        StatusPanelComponent& statusPanel,
        CommandLineComponent& commandLine,
        CompletionPopupComponent& completionPopup,
        std::string title);

    ftxui::Element OnRender() override;
    bool OnEvent(ftxui::Event event) override;

    /**
     * @brief 输入框在查看候选/翻历史时需要占用的额外行数（候选列表高度）
     */
    [[nodiscard]] i32 overlayHeight() const;

private:
    LogViewComponent& m_logView;
    StatusPanelComponent& m_statusPanel;
    CommandLineComponent& m_commandLine;
    CompletionPopupComponent& m_completionPopup;
    const std::string m_title;

    /// 布局在屏幕上的包围盒（渲染时由 reflect 回填），用于把日志区可见行数告知日志区。
    ftxui::Box m_box;
};

} // namespace mc::server::console
