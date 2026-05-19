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

#include "../ClientApplication.hpp"

#include "client/ui/minecraft/widgets/ScreenStackWidget.hpp"

#include <algorithm>

namespace mc::client {

void ClientApplication::updateUiFrameState(f32 deltaTime, f32 partialTick)
{
    // 更新 ScreenStackWidget 的 partialTick 和鼠标位置（用于 IScreen::render）
    auto* screenStack = m_kageroEngine
        ? static_cast<ui::minecraft::widgets::ScreenStackWidget*>(m_kageroEngine->getLayer(m_screenStackLayerId))
        : nullptr;
    if (screenStack) {
        screenStack->setPartialTick(partialTick);
        screenStack->setMousePosition(
            static_cast<i32>(m_input.mouseX() / static_cast<f64>(std::max(m_guiScaleState.scaleFactor, 1))),
            static_cast<i32>(m_input.mouseY() / static_cast<f64>(std::max(m_guiScaleState.scaleFactor, 1))));
    }

    // 更新 KageroEngine
    if (m_kageroEngine) {
        m_kageroEngine->update(deltaTime);
    }
}

} // namespace mc::client