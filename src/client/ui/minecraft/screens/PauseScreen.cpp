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

#include "PauseScreen.hpp"
#include "client/ui/kagero/event/EventBus.hpp"
#include "client/ui/kagero/state/StateStore.hpp"
#include "client/ui/kagero/template/binder/BindingContext.hpp"
#include "client/ui/minecraft/screens/Screen.hpp"
#include "client/ui/minecraft/screens/TemplateScreen.hpp"
#include "common/core/Types.hpp"
#include <memory>
#include <GLFW/glfw3.h>
#include <spdlog/spdlog.h>

namespace mc::client::ui::minecraft {

PauseScreen::PauseScreen()
    : TemplateScreen(std::make_unique<kagero::tpl::binder::BindingContext>(
                         kagero::state::StateStore::instance(), kagero::event::EventBus::instance()),
          "pause")
{
    setPauseScreen(true);
    if (!loadTemplateFile("src/client/ui/minecraft/templates/pause_menu.tpl")) {
        spdlog::error("[PauseScreen] Failed to load pause_menu.tpl template");
    }
    _registerCallbacks();
}

void PauseScreen::_registerCallbacks()
{
    exposeSimpleCallback("onResume", [this]() {
        if (m_onResume) {
            m_onResume();
        }
    });

    exposeSimpleCallback("onOptions", [this]() {
        if (m_onOptions) {
            m_onOptions();
        }
    });

    exposeSimpleCallback("onSaveAndQuit", [this]() {
        if (m_onSaveAndQuit) {
            m_onSaveAndQuit();
        }
    });
}

bool PauseScreen::onKey(i32 key, i32 scanCode, i32 action, i32 mods)
{
    (void)scanCode;
    (void)mods;

    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        if (m_onResume) {
            m_onResume();
        }
        return true;
    }

    return Screen::onKey(key, scanCode, action, mods);
}

} // namespace mc::client::ui::minecraft
