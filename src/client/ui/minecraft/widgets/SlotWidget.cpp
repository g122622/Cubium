#include "SlotWidget.hpp"

namespace mc::client::ui::minecraft {

void SlotWidget::paint(kagero::widget::PaintContext& ctx)
{
    kagero::widget::SlotWidget::paint(ctx);
    if (isHovered()) {
        ctx.drawBorder(bounds(), 1.0f, Colors::fromARGB(255, 255, 255, 160));
    }
}

} // namespace mc::client::ui::minecraft
