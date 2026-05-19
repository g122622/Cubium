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
#include "Screen.hpp"
#include <memory>
#include <vector>

namespace mc::client::ui::minecraft {

class ScreenManager {
public:
    void push(std::unique_ptr<Screen> screen);
    void pop();
    void clear();

    [[nodiscard]] Screen* top();
    [[nodiscard]] const Screen* top() const;

    /**
     * @brief 绘制所有屏幕
     * @param ctx 绘图上下文
     */
    void paint(kagero::widget::PaintContext& ctx);

    /**
     * @brief 更新所有屏幕的悬停状态
     * @param mouseX 鼠标X坐标
     * @param mouseY 鼠标Y坐标
     */
    void updateHover(i32 mouseX, i32 mouseY);

private:
    std::vector<std::unique_ptr<Screen>> m_stack;
};

} // namespace mc::client::ui::minecraft
