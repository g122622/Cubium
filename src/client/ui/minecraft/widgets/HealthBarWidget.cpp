#include "HealthBarWidget.hpp"

namespace mc::client::ui::minecraft {

HealthBarWidget::HealthBarWidget()
    : Widget("healthBar")
{}

void HealthBarWidget::setHealth(i32 health)
{
    m_health = std::max(0, std::min(20, health));
}

i32 HealthBarWidget::health() const
{
    return m_health;
}

void HealthBarWidget::paint(kagero::widget::PaintContext& ctx)
{
    ctx.drawFilledRect(bounds(), Colors::fromARGB(255, 45, 0, 0));
    const i32 fillWidth = static_cast<i32>(static_cast<f32>(width()) * (static_cast<f32>(m_health) / 20.0f));
    ctx.drawFilledRect(kagero::Rect{x(), y(), fillWidth, height()}, Colors::fromARGB(255, 220, 50, 50));
}

} // namespace mc::client::ui::minecraft
