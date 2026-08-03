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

#pragma once

#include "client/ui/kagero/paint/PaintContext.hpp"
#include "client/ui/kagero/widget/Widget.hpp"
#include "common/core/Constants.hpp"
#include "common/core/Types.hpp"

namespace mc::client::ui::minecraft {

/**
 * @brief 生命值条控件（简单矩形后备版本）
 *
 * 显示玩家生命值的简化 HUD 控件，使用矩形填充表示生命值。
 * 完整的心形图标渲染（含吸收、中毒、凋零变体等）已实现于
 * HudWidget::_renderHealth()，由 HudWidget 直接绘制。
 * 本控件仅作为独立调试用途保留。
 */
class HealthBarWidget : public kagero::widget::Widget {
public:
    HealthBarWidget();

    void setHealth(i32 health);
    [[nodiscard]] i32 health() const;

    void paint(kagero::widget::PaintContext& ctx) override;

private:
    i32 m_health = static_cast<i32>(mc::game::PLAYER_MAX_HEALTH);
};

} // namespace mc::client::ui::minecraft
