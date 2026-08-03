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

#include "MainMenuScreen.hpp"
#include "client/ui/kagero/event/EventBus.hpp"
#include "client/ui/kagero/state/StateStore.hpp"
#include "client/ui/kagero/template/binder/BindingContext.hpp"
#include "client/ui/minecraft/screens/Screen.hpp"
#include "client/ui/minecraft/screens/TemplateScreen.hpp"
#include "common/core/Types.hpp"
#include <memory>
#include <GLFW/glfw3.h>

namespace mc::client::ui::minecraft {

MainMenuScreen::MainMenuScreen()
    : TemplateScreen(std::make_unique<kagero::tpl::binder::BindingContext>(
                         kagero::state::StateStore::instance(), kagero::event::EventBus::instance()),
          "mainMenu")
{
    loadTemplateFile("src/client/ui/minecraft/templates/main_menu.tpl");
    _registerCallbacks();
}

void MainMenuScreen::_registerCallbacks()
{
    // 将模板中的按钮回调名绑定到对应的外部回调
    exposeSimpleCallback("onSinglePlayer", [this]() {
        if (m_onSinglePlayer) {
            m_onSinglePlayer();
        }
    });

    exposeSimpleCallback("onMultiPlayer", [this]() {
        if (m_onMultiPlayer) {
            m_onMultiPlayer();
        }
    });

    exposeSimpleCallback("onOptions", [this]() {
        if (m_onOptions) {
            m_onOptions();
        }
    });

    exposeSimpleCallback("onQuit", [this]() {
        if (m_onQuit) {
            m_onQuit();
        }
    });
}

bool MainMenuScreen::onKey(i32 key, i32 scanCode, i32 action, i32 mods)
{
    (void)scanCode;
    (void)mods;

    // Escape键触发退出回调
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        if (m_onQuit) {
            m_onQuit();
        }
        return true;
    }

    return Screen::onKey(key, scanCode, action, mods);
}

} // namespace mc::client::ui::minecraft
