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

#include "TemplateScreen.hpp"
#include "client/ui/kagero/state/ReactiveState.hpp"
#include "common/core/Types.hpp"
#include <memory>
#include <string>

namespace mc::client::ui::minecraft {

/**
 * @brief 世界加载进度界面
 *
 * 显示世界加载过程中的标题、当前阶段描述和进度条，
 * 通过响应式状态与模板系统绑定实现 UI 更新。
 */
class LoadingScreen : public TemplateScreen {
public:
    LoadingScreen();

    /** @brief 设置当前加载阶段描述文本 */
    void setStage(const std::string& stage);

    /** @brief 设置进度条进度，范围 [0.0, 1.0] */
    void setProgress(f32 progress);

    /** @brief 设置加载界面标题 */
    void setTitle(const std::string& title);

private:
    /** 加载界面标题的响应式值 */
    kagero::state::Reactive<std::string> m_titleValue;
    /** 当前加载阶段的响应式值 */
    kagero::state::Reactive<std::string> m_stageValue;
    /** 进度条宽度的响应式值（像素） */
    kagero::state::Reactive<i32> m_progressWidth;

    /** 进度条最大宽度（像素） */
    static constexpr i32 PROGRESS_BAR_WIDTH = 300;
};

} // namespace mc::client::ui::minecraft
