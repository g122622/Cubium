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
#include "common/core/Types.hpp"
#include <functional>
#include <utility>

namespace mc::client::ui::minecraft {

/**
 * @brief 主菜单界面
 *
 * 显示游戏主菜单，提供单人游戏、多人游戏、选项和退出按钮。
 * 按钮点击事件通过回调机制由外部设置，Escape键触发退出回调。
 */
class MainMenuScreen : public TemplateScreen {
public:
    using Callback = std::function<void()>;

    MainMenuScreen();

    void setOnSinglePlayer(Callback callback) { m_onSinglePlayer = std::move(callback); }
    void setOnMultiPlayer(Callback callback) { m_onMultiPlayer = std::move(callback); }
    void setOnOptions(Callback callback) { m_onOptions = std::move(callback); }
    void setOnQuit(Callback callback) { m_onQuit = std::move(callback); }

    bool onKey(i32 key, i32 scanCode, i32 action, i32 mods) override;

private:
    /** 注册模板回调，将UI按钮事件绑定到对应回调 */
    void _registerCallbacks();

    Callback m_onSinglePlayer;
    Callback m_onMultiPlayer;
    Callback m_onOptions;
    Callback m_onQuit;
};

} // namespace mc::client::ui::minecraft
