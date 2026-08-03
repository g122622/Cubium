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

#include "HealthBarWidget.hpp"
#include "client/ui/Glyph.hpp"
#include "client/ui/kagero/Types.hpp"
#include "client/ui/kagero/paint/PaintContext.hpp"
#include "client/ui/kagero/widget/Widget.hpp"
#include "common/core/Constants.hpp"
#include "common/core/Types.hpp"
#include <algorithm>

namespace mc::client::ui::minecraft {

HealthBarWidget::HealthBarWidget()
    : Widget("healthBar")
{}

void HealthBarWidget::setHealth(i32 health)
{
    constexpr i32 maxHealth = static_cast<i32>(mc::game::PLAYER_MAX_HEALTH);
    m_health = std::max(0, std::min(maxHealth, health));
}

i32 HealthBarWidget::health() const
{
    return m_health;
}

// 简化矩形后备渲染。完整的心形图标渲染（含吸收、中毒、凋零变体等）
// 已实现于 HudWidget::_renderHealth()，由 HudWidget 直接绘制。
void HealthBarWidget::paint(kagero::widget::PaintContext& ctx)
{
    // 背景条（暗红色）
    ctx.drawFilledRect(bounds(), Colors::fromARGB(255, 45, 0, 0));

    // 生命值填充（红色），按生命值比例计算宽度
    const f32 healthRatio = static_cast<f32>(m_health) / mc::game::PLAYER_MAX_HEALTH;
    const i32 fillWidth = static_cast<i32>(static_cast<f32>(width()) * healthRatio);
    ctx.drawFilledRect(kagero::Rect{x(), y(), fillWidth, height()}, Colors::fromARGB(255, 220, 50, 50));
}

} // namespace mc::client::ui::minecraft
