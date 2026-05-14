#include "OptionsScreen.hpp"

namespace mc::client::ui::minecraft {

OptionsScreen::OptionsScreen()
    : Screen("options")
{}

void OptionsScreen::paint(kagero::widget::PaintContext& ctx)
{
    ctx.drawFilledRect(bounds(), Colors::fromARGB(220, 18, 18, 26));
}

} // namespace mc::client::ui::minecraft
