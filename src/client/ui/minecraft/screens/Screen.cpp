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

#include "Screen.hpp"
#include "client/ui/kagero/paint/PaintContext.hpp"
#include "client/ui/kagero/widget/ContainerWidget.hpp"
#include "common/core/Types.hpp"
#include <string>
#include <utility>

namespace mc::client::ui::minecraft {

Screen::Screen(std::string id)
    : ContainerWidget(std::move(id))
{}

void Screen::onOpen() {}

void Screen::onClose() {}

void Screen::paint(kagero::widget::PaintContext& ctx)
{
    ContainerWidget::paint(ctx);
}

void Screen::updateHover(i32 mouseX, i32 mouseY)
{
    // 更新自身悬停状态
    setHovered(isMouseOver(mouseX, mouseY));

    // 更新所有子组件的悬停状态
    for (auto& child : m_children) {
        if (child->isVisible()) {
            child->updateHover(mouseX, mouseY);
        }
    }
}

bool Screen::isModal() const
{
    return m_modal;
}

void Screen::setModal(bool modal)
{
    m_modal = modal;
}

bool Screen::isPauseScreen() const
{
    return m_pauseScreen;
}

void Screen::setPauseScreen(bool pauseScreen)
{
    m_pauseScreen = pauseScreen;
}

} // namespace mc::client::ui::minecraft
