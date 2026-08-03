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

#include "HungerBarWidget.hpp"
#include "client/ui/Glyph.hpp"
#include "client/ui/kagero/Types.hpp"
#include "client/ui/kagero/paint/PaintContext.hpp"
#include "client/ui/kagero/widget/Widget.hpp"
#include "common/core/Constants.hpp"
#include "common/core/Types.hpp"
#include <algorithm>

namespace mc::client::ui::minecraft {

HungerBarWidget::HungerBarWidget()
    : Widget("hungerBar")
{}

void HungerBarWidget::setHunger(i32 hunger)
{
    m_hunger = std::max(0, std::min(mc::game::PLAYER_MAX_HUNGER, hunger));
}

i32 HungerBarWidget::hunger() const
{
    return m_hunger;
}

// 简化矩形后备渲染。完整的鸡腿图标渲染（含饥饿效果变体和饱和度抖动动画）
// 已实现于 HudWidget::_renderHunger()，由 HudWidget 直接绘制。
void HungerBarWidget::paint(kagero::widget::PaintContext& ctx)
{
    // 背景条（暗褐色）
    ctx.drawFilledRect(bounds(), Colors::fromARGB(255, 52, 28, 0));

    // 饥饿值填充（棕黄色），按饥饿值比例计算宽度
    const f32 hungerRatio = static_cast<f32>(m_hunger) / static_cast<f32>(mc::game::PLAYER_MAX_HUNGER);
    const i32 fillWidth = static_cast<i32>(static_cast<f32>(width()) * hungerRatio);
    ctx.drawFilledRect(kagero::Rect{x(), y(), fillWidth, height()}, Colors::fromARGB(255, 240, 140, 40));
}

} // namespace mc::client::ui::minecraft
