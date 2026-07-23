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

#include "client/ui/screen/ScreenManager.hpp"

namespace mc::client {

ScreenManager& ScreenManager::instance() noexcept
{
    static ScreenManager instance;
    return instance;
}

void ScreenManager::setScreenStackWidget(ui::minecraft::widgets::ScreenStackWidget* stackWidget)
{
    m_stackWidget = stackWidget;

    // 桥接 ScreenStackWidget 的屏幕变化回调到 ScreenManager 的 IScreen 回调
    // 这样无论通过 ScreenStackWidget 还是 ScreenManager 操作屏幕，
    // ScreenManager 的回调都能被正确触发
    if (m_stackWidget) {
        m_stackWidget->setScreenChangeCallback([this](const ui::minecraft::widgets::ScreenChangeInfo& info) {
            if (m_onScreenChange) {
                m_onScreenChange(info.newIScreen);
            }
        });
    }
}

void ScreenManager::openScreen(std::unique_ptr<IScreen> screen)
{
    if (m_stackWidget && screen) {
        m_stackWidget->pushIScreen(std::move(screen));
        // 回调由 ScreenStackWidget 内部触发，无需在此重复触发
    }
}

void ScreenManager::openScreen(std::unique_ptr<ui::minecraft::Screen> screen)
{
    if (m_stackWidget && screen) {
        m_stackWidget->push(std::move(screen));
        // 回调由 ScreenStackWidget 内部触发，无需在此重复触发
    }
}

void ScreenManager::closeScreen()
{
    if (m_stackWidget && m_stackWidget->hasScreen()) {
        m_stackWidget->pop();
        // 回调由 ScreenStackWidget 内部触发，无需在此重复触发
    }
}

void ScreenManager::closeAll()
{
    if (m_stackWidget) {
        m_stackWidget->clear();
        // 回调由 ScreenStackWidget 内部触发，无需在此重复触发
    }
}

void ScreenManager::tick(f32 dt)
{
    // 由 ScreenStackWidget 在 KageroEngine 中处理
    (void)dt;
}

void ScreenManager::render(i32 mouseX, i32 mouseY, f32 partialTick)
{
    // 由 ScreenStackWidget 在 KageroEngine 中处理
    (void)mouseX;
    (void)mouseY;
    (void)partialTick;
}

bool ScreenManager::onClick(i32 mouseX, i32 mouseY, i32 button, i32 mods)
{
    if (m_stackWidget) {
        return m_stackWidget->onClick(mouseX, mouseY, button, mods);
    }
    return false;
}

bool ScreenManager::onRelease(i32 mouseX, i32 mouseY, i32 button, i32 mods)
{
    if (m_stackWidget) {
        return m_stackWidget->onRelease(mouseX, mouseY, button, mods);
    }
    return false;
}

bool ScreenManager::onDrag(i32 mouseX, i32 mouseY, i32 deltaX, i32 deltaY, i32 button)
{
    if (m_stackWidget) {
        return m_stackWidget->onDrag(mouseX, mouseY, deltaX, deltaY, button);
    }
    return false;
}

bool ScreenManager::onScroll(i32 mouseX, i32 mouseY, f64 delta)
{
    if (m_stackWidget) {
        return m_stackWidget->onScroll(mouseX, mouseY, delta);
    }
    return false;
}

bool ScreenManager::onKey(i32 key, i32 scanCode, i32 action, i32 mods)
{
    if (m_stackWidget) {
        return m_stackWidget->onKey(key, scanCode, action, mods);
    }
    return false;
}

bool ScreenManager::onChar(u32 codePoint)
{
    if (m_stackWidget) {
        return m_stackWidget->onChar(codePoint);
    }
    return false;
}

void ScreenManager::onResize(i32 width, i32 height)
{
    if (m_stackWidget) {
        m_stackWidget->onResize(width, height);
    }
}

bool ScreenManager::shouldPauseGame() const noexcept
{
    if (m_stackWidget) {
        return m_stackWidget->shouldPauseGame();
    }
    return false;
}

} // namespace mc::client
