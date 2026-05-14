#include "Viewport3DWidget.hpp"

namespace mc::client::ui::minecraft {

void Viewport3DWidget::paint(kagero::widget::PaintContext& ctx)
{
    kagero::widget::Viewport3DWidget::paint(ctx);
    ctx.drawBorder(bounds(), 1.0f, Colors::fromARGB(255, 180, 180, 220));
}

} // namespace mc::client::ui::minecraft
