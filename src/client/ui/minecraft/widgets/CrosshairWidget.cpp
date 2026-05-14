#include "CrosshairWidget.hpp"
#include "client/ui/kagero/Types.hpp"

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
