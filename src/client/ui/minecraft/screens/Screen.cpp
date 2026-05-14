#include "Screen.hpp"

namespace mc::client::ui::minecraft {

Screen::Screen(std::string id)
    : ContainerWidget(std::move(id))
{}

void Screen::onOpen() {}

void Screen::onClose() {}

void Screen::paint(kagero::widget::PaintContext& ctx)
{
    ContainerWidget::paint(ctx);
}

void Screen::updateHover(i32 mouseX, i32 mouseY)
{
    // 更新自身悬停状态
    setHovered(isMouseOver(mouseX, mouseY));

    // 更新所有子组件的悬停状态
    for (auto& child : m_children) {
        if (child->isVisible()) {
            child->updateHover(mouseX, mouseY);
        }
    }
}

bool Screen::isModal() const
{
    return m_modal;
}

void Screen::setModal(bool modal)
{
    m_modal = modal;
}

} // namespace mc::client::ui::minecraft
