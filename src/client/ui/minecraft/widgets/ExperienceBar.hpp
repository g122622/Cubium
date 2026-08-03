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

#include "../../kagero/paint/PaintContext.hpp"
#include "../../kagero/widget/Widget.hpp"
#include "common/core/Types.hpp"

namespace mc::client::ui::minecraft {

/**
 * @brief 经验条组件
 *
 * 显示玩家经验进度的 HUD 组件，以水平进度条形式渲染。
 * 进度值范围为 [0.0, 1.0]。
 */
class ExperienceBar : public kagero::widget::Widget {
public:
    ExperienceBar();

    /**
     * @brief 设置经验进度
     * @param progress 进度值，会被自动钳位到 [0.0, 1.0] 范围
     */
    void setProgress(f32 progress) noexcept;

    /**
     * @brief 获取当前经验进度
     * @return 当前进度值，范围 [0.0, 1.0]
     */
    [[nodiscard]] f32 progress() const noexcept;

    void paint(kagero::widget::PaintContext& ctx) override;

private:
    f32 m_progress = 0.0f;
};

} // namespace mc::client::ui::minecraft
