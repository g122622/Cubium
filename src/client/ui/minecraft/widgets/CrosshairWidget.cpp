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

#include "CrosshairWidget.hpp"

#include "client/ui/kagero/Types.hpp"
#include "client/ui/kagero/paint/PaintContext.hpp"
#include "client/ui/kagero/widget/Widget.hpp"
#include "common/core/Types.hpp"

namespace mc::client::ui::minecraft::widgets {

using mc::client::ui::kagero::Rect;

CrosshairWidget::CrosshairWidget()
    : Widget("crosshair")
{
    // 准星始终可见且激活
    setVisible(true);
    setActive(true);
}

void CrosshairWidget::paint(kagero::widget::PaintContext& ctx)
{
    if (!isVisible()) {
        return;
    }

    // 获取屏幕中心位置
    const i32 centerX = width() / 2;
    const i32 centerY = height() / 2;

    // 绘制水平线
    ctx.drawFilledRect(Rect(static_cast<i32>(centerX - m_size),
                           static_cast<i32>(centerY - m_thickness / 2),
                           static_cast<i32>(m_size * 2),
                           static_cast<i32>(m_thickness)),
        m_color);

    // 绘制垂直线
    ctx.drawFilledRect(Rect(static_cast<i32>(centerX - m_thickness / 2),
                           static_cast<i32>(centerY - m_size),
                           static_cast<i32>(m_thickness),
                           static_cast<i32>(m_size * 2)),
        m_color);
}

} // namespace mc::client::ui::minecraft::widgets
