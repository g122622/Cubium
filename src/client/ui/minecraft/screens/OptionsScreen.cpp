/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including limitation the rights
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

#include "OptionsScreen.hpp"
#include <GLFW/glfw3.h>
#include <spdlog/spdlog.h>

namespace mc::client::ui::minecraft {

OptionsScreen::OptionsScreen()
    : TemplateScreen(std::make_unique<kagero::tpl::binder::BindingContext>(
                         kagero::state::StateStore::instance(), kagero::event::EventBus::instance()),
          "options")
{
    loadTemplateFile("src/client/ui/minecraft/templates/options.tpl");
    _registerCallbacks();
}

bool OptionsScreen::onKey(i32 key, i32 scanCode, i32 action, i32 mods)
{
    (void)scanCode;
    (void)mods;

    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        if (m_onClose) {
            m_onClose();
        }
        return true;
    }

    return Screen::onKey(key, scanCode, action, mods);
}

void OptionsScreen::_registerCallbacks()
{
    exposeSimpleCallback("onClose", [this]() {
        if (m_onClose) {
            m_onClose();
        }
    });
}

} // namespace mc::client::ui::minecraft
