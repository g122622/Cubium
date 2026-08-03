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

#include "ContainerScreen.hpp"
#include "client/ui/Glyph.hpp"
#include "client/ui/kagero/paint/PaintContext.hpp"
#include "client/ui/minecraft/screens/Screen.hpp"

namespace mc::client::ui::minecraft {

ContainerScreen::ContainerScreen()
    : Screen("container")
{}

void ContainerScreen::paint(kagero::widget::PaintContext& ctx)
{
    // 绘制半透明深色遮罩，遮挡游戏画面以突出容器界面
    ctx.drawFilledRect(bounds(), Colors::fromARGB(220, 30, 24, 20));
}

} // namespace mc::client::ui::minecraft
