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

#include "ScreenManager.hpp"
#include "client/ui/kagero/paint/PaintContext.hpp"
#include "client/ui/minecraft/screens/Screen.hpp"
#include "common/core/Types.hpp"
#include <memory>
#include <utility>

namespace mc::client::ui::minecraft {

void ScreenManager::push(std::unique_ptr<Screen> screen)
{
    if (!screen) {
        return;
    }
    screen->onOpen();
    m_stack.push_back(std::move(screen));
}

void ScreenManager::pop()
{
    if (m_stack.empty()) {
        return;
    }
    m_stack.back()->onClose();
    m_stack.pop_back();
}

void ScreenManager::clear()
{
    while (!m_stack.empty()) {
        pop();
    }
}

Screen* ScreenManager::top()
{
    return m_stack.empty() ? nullptr : m_stack.back().get();
}

const Screen* ScreenManager::top() const
{
    return m_stack.empty() ? nullptr : m_stack.back().get();
}

void ScreenManager::paint(kagero::widget::PaintContext& ctx)
{
    for (const auto& screen : m_stack) {
        if (screen->isVisible()) {
            screen->paint(ctx);
        }
        if (screen->isModal()) {
            break;
        }
    }
}

void ScreenManager::updateHover(i32 mouseX, i32 mouseY)
{
    for (const auto& screen : m_stack) {
        if (screen->isVisible()) {
            screen->updateHover(mouseX, mouseY);
        }
        if (screen->isModal()) {
            break;
        }
    }
}

} // namespace mc::client::ui::minecraft
