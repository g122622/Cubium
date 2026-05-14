#include "client/ui/screen/ChestScreen.hpp"
#include "client/renderer/trident/gui/GuiRenderer.hpp"
#include "client/renderer/trident/item/ItemRenderer.hpp"
#include "common/world/blockentity/core/SimpleInventory.hpp"

namespace mc::client {

ChestScreen::ChestScreen(ContainerId containerId,
    mc::PlayerInventory* playerInventory,
    i32 rows,
    ContainerClickSender clickSender,
    ContainerCloseSender closeSender)
    : AbstractContainerScreen<mc::blockentity::ChestContainer>(
          std::make_unique<mc::blockentity::ChestContainer>(containerId,
              playerInventory,
              std::shared_ptr<mc::IInventory>(std::make_shared<mc::blockentity::SimpleInventory>(
                  rows * mc::blockentity::ChestContainer::SLOTS_PER_ROW)),
              rows),
          std::move(clickSender),
          std::move(closeSender))
    , m_rows(rows)
{
    setImageSize(GUI_WIDTH, BASE_GUI_HEIGHT + m_rows * SLOT_SPACING);
}

void ChestScreen::onInit()
{
    updatePosition();
}

void ChestScreen::renderContainerBackground()
{
    if (m_gui == nullptr) {
        return;
    }

    constexpr u32 BG_COLOR = 0xFFC6C6C6;
    constexpr u32 BORDER_COLOR = 0xFF555555;

    m_gui->fillRect(static_cast<f32>(m_leftPos),
        static_cast<f32>(m_topPos),
        static_cast<f32>(GUI_WIDTH),
        static_cast<f32>(getImageHeight()),
        BG_COLOR);
    m_gui->drawRect(static_cast<f32>(m_leftPos),
        static_cast<f32>(m_topPos),
        static_cast<f32>(GUI_WIDTH),
        static_cast<f32>(getImageHeight()),
        BORDER_COLOR);
}

void ChestScreen::renderContainerForeground(i32 mouseX, i32 mouseY)
{
    (void)mouseX;
    (void)mouseY;

    if (m_gui != nullptr && m_gui->font() != nullptr) {
        m_gui->drawText(
            "Chest", static_cast<f32>(m_leftPos + TITLE_X), static_cast<f32>(m_topPos + TITLE_Y), 0xFF404040, false);
    }
}

void ChestScreen::renderItemIcon(const mc::ItemStack& stack, i32 screenX, i32 screenY)
{
    if (m_gui == nullptr || stack.isEmpty()) {
        return;
    }

    if (m_itemRenderer != nullptr) {
        m_itemRenderer->renderItem(
            *m_gui, stack, static_cast<f32>(screenX), static_cast<f32>(screenY), static_cast<f32>(SLOT_SIZE));
    }
}

void ChestScreen::renderTooltip(i32 mouseX, i32 mouseY)
{
    (void)mouseX;
    (void)mouseY;
}

} // namespace mc::client
