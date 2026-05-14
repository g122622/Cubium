#include "client/ui/screen/ScreenManager.hpp"

namespace mc::client {

ScreenManager& ScreenManager::instance()
{
    static ScreenManager instance;
    return instance;
}

void ScreenManager::setScreenStackWidget(ui::minecraft::widgets::ScreenStackWidget* stackWidget)
{
    m_stackWidget = stackWidget;
}

void ScreenManager::openScreen(std::unique_ptr<IScreen> screen)
{
    if (m_stackWidget && screen) {
        m_stackWidget->pushIScreen(std::move(screen));

        if (m_onScreenChange) {
            m_onScreenChange(getCurrentScreen());
        }
    }
}

void ScreenManager::closeScreen()
{
    if (m_stackWidget && m_stackWidget->hasScreen()) {
        m_stackWidget->pop();

        if (m_onScreenChange) {
            m_onScreenChange(getCurrentScreen());
        }
    }
}

void ScreenManager::closeAll()
{
    if (m_stackWidget) {
        m_stackWidget->clear();
    }

    if (m_onScreenChange) {
        m_onScreenChange(nullptr);
    }
}

void ScreenManager::tick(f32 dt)
{
    // 由 ScreenStackWidget 在 KageroEngine 中处理
    (void)dt;
}

void ScreenManager::render(i32 mouseX, i32 mouseY, f32 partialTick)
{
    // 由 ScreenStackWidget 在 KageroEngine 中处理
    (void)mouseX;
    (void)mouseY;
    (void)partialTick;
}

bool ScreenManager::onClick(i32 mouseX, i32 mouseY, i32 button)
{
    if (m_stackWidget) {
        return m_stackWidget->onClick(mouseX, mouseY, button);
    }
    return false;
}

bool ScreenManager::onRelease(i32 mouseX, i32 mouseY, i32 button)
{
    if (m_stackWidget) {
        return m_stackWidget->onRelease(mouseX, mouseY, button);
    }
    return false;
}

bool ScreenManager::onDrag(i32 mouseX, i32 mouseY, i32 deltaX, i32 deltaY)
{
    if (m_stackWidget) {
        return m_stackWidget->onDrag(mouseX, mouseY, deltaX, deltaY);
    }
    return false;
}

bool ScreenManager::onScroll(i32 mouseX, i32 mouseY, f64 delta)
{
    if (m_stackWidget) {
        return m_stackWidget->onScroll(mouseX, mouseY, delta);
    }
    return false;
}

bool ScreenManager::onKey(i32 key, i32 scanCode, i32 action, i32 mods)
{
    if (m_stackWidget) {
        return m_stackWidget->onKey(key, scanCode, action, mods);
    }
    return false;
}

bool ScreenManager::onChar(u32 codePoint)
{
    if (m_stackWidget) {
        return m_stackWidget->onChar(codePoint);
    }
    return false;
}

void ScreenManager::onResize(i32 width, i32 height)
{
    if (m_stackWidget) {
        m_stackWidget->onResize(width, height);
    }
}

bool ScreenManager::shouldPauseGame() const
{
    if (m_stackWidget) {
        return m_stackWidget->shouldPauseGame();
    }
    return false;
}

} // namespace mc::client
