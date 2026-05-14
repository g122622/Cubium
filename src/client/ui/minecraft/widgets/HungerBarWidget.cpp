#include "HungerBarWidget.hpp"

namespace mc::client::ui::minecraft {

HungerBarWidget::HungerBarWidget()
    : Widget("hungerBar")
{}

void HungerBarWidget::setHunger(i32 hunger)
{
    m_hunger = std::max(0, std::min(20, hunger));
}

i32 HungerBarWidget::hunger() const
{
    return m_hunger;
}

void HungerBarWidget::paint(kagero::widget::PaintContext& ctx)
{
    ctx.drawFilledRect(bounds(), Colors::fromARGB(255, 52, 28, 0));
    const i32 fillWidth = static_cast<i32>(static_cast<f32>(width()) * (static_cast<f32>(m_hunger) / 20.0f));
    ctx.drawFilledRect(kagero::Rect{x(), y(), fillWidth, height()}, Colors::fromARGB(255, 240, 140, 40));
}

} // namespace mc::client::ui::minecraft
