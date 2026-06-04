/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the conditions:
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
#include <functional>

namespace mc::client::ui::minecraft {

/**
 * @brief 游戏选项设置界面
 *
 * 提供游戏设置选项的骨架界面，使用模板驱动布局。
 * ESC 键或 Done 按钮可关闭界面返回上一级。
 */
class OptionsScreen : public TemplateScreen {
public:
    OptionsScreen();

    /**
     * @brief 设置关闭回调
     * @param callback 关闭时调用的回调函数
     */
    void setOnClose(std::function<void()> callback) { m_onClose = std::move(callback); }

    /**
     * @brief 处理键盘事件，ESC 键关闭界面
     */
    bool onKey(i32 key, i32 scanCode, i32 action, i32 mods) override;

private:
    void _registerCallbacks();

    std::function<void()> m_onClose;
};

} // namespace mc::client::ui::minecraft
