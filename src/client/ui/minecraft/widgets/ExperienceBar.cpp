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

#include "ExperienceBar.hpp"
#include "client/ui/Glyph.hpp"
#include "client/ui/kagero/Types.hpp"
#include "client/ui/kagero/paint/PaintContext.hpp"
#include "client/ui/kagero/widget/Widget.hpp"
#include "common/core/Types.hpp"
#include <algorithm>

namespace mc::client::ui::minecraft {

ExperienceBar::ExperienceBar()
    : Widget("experienceBar")
{}

void ExperienceBar::setProgress(f32 progress) noexcept
{
    m_progress = std::max(0.0f, std::min(1.0f, progress));
}

f32 ExperienceBar::progress() const noexcept
{
    return m_progress;
}

void ExperienceBar::paint(kagero::widget::PaintContext& ctx)
{
    // 背景：深色底色
    ctx.drawFilledRect(bounds(), Colors::fromARGB(255, 20, 40, 0));
    // 填充：绿色经验条
    const i32 fillWidth = static_cast<i32>(static_cast<f32>(width()) * m_progress);
    ctx.drawFilledRect(kagero::Rect{x(), y(), fillWidth, height()}, Colors::fromARGB(255, 120, 255, 60));
}

} // namespace mc::client::ui::minecraft
