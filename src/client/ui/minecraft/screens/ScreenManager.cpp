#include "ScreenManager.hpp"

namespace mc::client::ui::minecraft {

void ScreenManager::push(std::unique_ptr<Screen> screen)
{
    if (!screen) {
        return;
    }
    screen->onOpen();
    m_stack.push_back(std::move(screen));
}

void ScreenManager::pop()
{
    if (m_stack.empty()) {
        return;
    }
    m_stack.back()->onClose();
    m_stack.pop_back();
}

void ScreenManager::clear()
{
    while (!m_stack.empty()) {
        pop();
    }
}

Screen* ScreenManager::top()
{
    return m_stack.empty() ? nullptr : m_stack.back().get();
}

const Screen* ScreenManager::top() const
{
    return m_stack.empty() ? nullptr : m_stack.back().get();
}

void ScreenManager::paint(kagero::widget::PaintContext& ctx)
{
    for (const auto& screen : m_stack) {
        if (screen->isVisible()) {
            screen->paint(ctx);
        }
        if (screen->isModal()) {
            break;
        }
    }
}

void ScreenManager::updateHover(i32 mouseX, i32 mouseY)
{
    for (const auto& screen : m_stack) {
        if (screen->isVisible()) {
            screen->updateHover(mouseX, mouseY);
        }
        if (screen->isModal()) {
            break;
        }
    }
}

} // namespace mc::client::ui::minecraft
