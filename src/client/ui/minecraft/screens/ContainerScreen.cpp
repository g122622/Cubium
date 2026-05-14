#include "ContainerScreen.hpp"

namespace mc::client::ui::minecraft {

ContainerScreen::ContainerScreen()
    : Screen("container")
{}

void ContainerScreen::paint(kagero::widget::PaintContext& ctx)
{
    ctx.drawFilledRect(bounds(), Colors::fromARGB(220, 30, 24, 20));
}

} // namespace mc::client::ui::minecraft
