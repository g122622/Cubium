#include "client/ui/screen/FurnaceScreen.hpp"
#include "client/renderer/trident/gui/GuiRenderer.hpp"
#include "client/renderer/trident/item/ItemRenderer.hpp"
#include "common/world/blockentity/processing/FurnaceInventory.hpp"

namespace mc::client {

FurnaceScreen::FurnaceScreen(ContainerId containerId,
                             mc::PlayerInventory* playerInventory,
                             ContainerClickSender clickSender,
                             ContainerCloseSender closeSender)
    : AbstractContainerScreen<mc::blockentity::FurnaceContainer>(
          std::make_unique<mc::blockentity::FurnaceContainer>(
              containerId,
              playerInventory,
              std::shared_ptr<mc::IInventory>(
                  std::make_shared<mc::blockentity::FurnaceInventory>()),
              nullptr),
          std::move(clickSender),
          std::move(closeSender))
{
    setImageSize(GUI_WIDTH, GUI_HEIGHT);
}

void FurnaceScreen::onInit()
{
    updatePosition();
}

void FurnaceScreen::renderContainerBackground()
{
    if (m_gui == nullptr) {
        return;
    }

    constexpr u32 BG_COLOR = 0xFFC6C6C6;
    constexpr u32 BORDER_COLOR = 0xFF555555;

    m_gui->fillRect(static_cast<f32>(m_leftPos), static_cast<f32>(m_topPos),
                    static_cast<f32>(GUI_WIDTH), static_cast<f32>(GUI_HEIGHT), BG_COLOR);
    m_gui->drawRect(static_cast<f32>(m_leftPos), static_cast<f32>(m_topPos),
                    static_cast<f32>(GUI_WIDTH), static_cast<f32>(GUI_HEIGHT), BORDER_COLOR);
}

void FurnaceScreen::renderContainerForeground(i32 mouseX, i32 mouseY)
{
    (void)mouseX;
    (void)mouseY;

    if (m_gui != nullptr && m_gui->font() != nullptr) {
        m_gui->drawText("Furnace",
                        static_cast<f32>(m_leftPos + TITLE_X),
                        static_cast<f32>(m_topPos + TITLE_Y),
                        0xFF404040,
                        false);
    }
}

void FurnaceScreen::renderItemIcon(const mc::ItemStack& stack, i32 screenX, i32 screenY)
{
    if (m_gui == nullptr || stack.isEmpty()) {
        return;
    }

    if (m_itemRenderer != nullptr) {
        m_itemRenderer->renderItem(*m_gui,
                                   stack,
                                   static_cast<f32>(screenX),
                                   static_cast<f32>(screenY),
                                   static_cast<f32>(SLOT_SIZE));
    }
}

void FurnaceScreen::renderTooltip(i32 mouseX, i32 mouseY)
{
    (void)mouseX;
    (void)mouseY;
}

} // namespace mc::client
