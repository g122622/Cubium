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
#include "server/application/console/input/CommandCompletion.hpp"

#include <ftxui/component/component_base.hpp>

namespace mc::server::console {

/**
 * @brief 输入框上方的补全候选列表
 *
 * 多候选时展示前若干项，↑/↓ 由 CommandLineComponent 改写 CompletionState 的高亮，
 * 这里只负责把"围绕高亮项的窗口"画出来。候选为空时不占高度。
 */
class CompletionPopupComponent final : public ftxui::ComponentBase {
public:
    /**
     * @brief 构造候选列表
     * @param state 补全状态（须比本对象存活更久）
     */
    explicit CompletionPopupComponent(const CompletionState& state);

    ftxui::Element OnRender() override;

    /**
     * @brief 无候选时为 0；有候选时为列表高度（候选行 + 一行按键提示）
     */
    [[nodiscard]] i32 visibleRowCount() const;

    /**
     * @brief 最多同时展示的候选行数
     */
    static constexpr usize MAX_VISIBLE_ROWS = 6;

private:
    const CompletionState& m_state;
};

} // namespace mc::server::console
