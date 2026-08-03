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

#include "Screen.hpp"
#include "client/ui/kagero/paint/PaintContext.hpp"
#include "common/core/Types.hpp"
#include <memory>
#include <vector>

namespace mc::client::ui::minecraft {

/**
 * @brief 屏幕栈管理器
 *
 * 管理屏幕的生命周期与渲染顺序。屏幕以栈结构组织，
 * 模态屏幕（isModal）会阻止其下方屏幕的绘制和交互。
 */
class ScreenManager {
public:
    /**
     * @brief 将屏幕压入栈顶
     * @param screen 要压入的屏幕，若为空则忽略
     */
    void push(std::unique_ptr<Screen> screen);

    /**
     * @brief 弹出栈顶屏幕，若栈为空则忽略
     */
    void pop();

    /**
     * @brief 依次弹出所有屏幕，清空屏幕栈
     */
    void clear();

    /**
     * @brief 获取栈顶屏幕
     * @return 栈顶屏幕指针，若栈为空则返回 nullptr
     */
    [[nodiscard]] Screen* top();

    /**
     * @brief 获取栈顶屏幕（const 版本）
     * @return 栈顶屏幕的 const 指针，若栈为空则返回 nullptr
     */
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
    /// 屏幕栈，后入的屏幕在栈顶
    std::vector<std::unique_ptr<Screen>> m_stack;
};

} // namespace mc::client::ui::minecraft
