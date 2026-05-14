#include "InventoryScreen.hpp"

namespace mc::client::ui::minecraft {

InventoryScreen::InventoryScreen()
    : Screen("inventory")
{}

void InventoryScreen::paint(kagero::widget::PaintContext& ctx)
{
    ctx.drawFilledRect(bounds(), Colors::fromARGB(220, 25, 25, 25));
    ctx.drawBorder(bounds(), 1.0f, Colors::fromARGB(255, 130, 130, 130));
}

} // namespace mc::client::ui::minecraft
