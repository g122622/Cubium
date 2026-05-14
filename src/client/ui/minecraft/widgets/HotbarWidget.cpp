#include "HotbarWidget.hpp"

namespace mc::client::ui::minecraft {

HotbarWidget::HotbarWidget()
    : kagero::widget::ContainerWidget("hotbar")
{}

void HotbarWidget::paint(kagero::widget::PaintContext& ctx)
{
    ctx.drawFilledRect(bounds(), Colors::fromARGB(220, 0, 0, 0));
    ctx.drawBorder(bounds(), 1.0f, Colors::fromARGB(255, 160, 160, 160));
}

} // namespace mc::client::ui::minecraft
