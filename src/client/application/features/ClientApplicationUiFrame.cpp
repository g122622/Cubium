#include "../ClientApplication.hpp"

#include "client/ui/minecraft/widgets/ScreenStackWidget.hpp"

#include <algorithm>

namespace mc::client {

void ClientApplication::updateUiFrameState(f32 deltaTime)
{
    // 更新 ScreenStackWidget 的 partialTick 和鼠标位置（用于 IScreen::render）
    auto* screenStack = m_kageroEngine ?
        static_cast<ui::minecraft::widgets::ScreenStackWidget*>(m_kageroEngine->getLayer(m_screenStackLayerId)) : nullptr;
    if (screenStack) {
        screenStack->setPartialTick(0.0f);  // TODO: 使用实际的 partialTick
        screenStack->setMousePosition(
            static_cast<i32>(m_input.mouseX() / static_cast<f64>(std::max(m_guiScaleState.scaleFactor, 1))),
            static_cast<i32>(m_input.mouseY() / static_cast<f64>(std::max(m_guiScaleState.scaleFactor, 1)))
        );
    }

    // 更新 KageroEngine
    if (m_kageroEngine) {
        m_kageroEngine->update(deltaTime);
    }
}

} // namespace mc::client