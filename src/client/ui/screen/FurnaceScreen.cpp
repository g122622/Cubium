/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

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
          std::make_unique<mc::blockentity::FurnaceContainer>(containerId,
              playerInventory,
              std::shared_ptr<mc::IInventory>(std::make_shared<mc::blockentity::FurnaceInventory>()),
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
    // TODO: 使用熔炉GUI纹理替代纯色矩形背景
    constexpr u32 BG_COLOR = 0xFFC6C6C6;
    constexpr u32 BORDER_COLOR = 0xFF555555;

    m_gui->fillRect(static_cast<f32>(m_leftPos),
        static_cast<f32>(m_topPos),
        static_cast<f32>(GUI_WIDTH),
        static_cast<f32>(GUI_HEIGHT),
        BG_COLOR);
    m_gui->drawRect(static_cast<f32>(m_leftPos),
        static_cast<f32>(m_topPos),
        static_cast<f32>(GUI_WIDTH),
        static_cast<f32>(GUI_HEIGHT),
        BORDER_COLOR);
}

void FurnaceScreen::renderContainerForeground(i32 mouseX, i32 mouseY)
{
    (void)mouseX;
    (void)mouseY;

    if (m_gui->font() != nullptr) {
        m_gui->drawText(
            "Furnace", static_cast<f32>(m_leftPos + TITLE_X), static_cast<f32>(m_topPos + TITLE_Y), 0xFF404040, false);
    }
}

void FurnaceScreen::renderItemIcon(const mc::ItemStack& stack, i32 screenX, i32 screenY)
{
    if (m_itemRenderer != nullptr) {
        m_itemRenderer->renderItem(
            *m_gui, stack, static_cast<f32>(screenX), static_cast<f32>(screenY), static_cast<f32>(SLOT_SIZE));
    }
}

void FurnaceScreen::renderTooltip(i32 mouseX, i32 mouseY)
{
    mc::Slot* slot = getSlotAt(mouseX, mouseY);
    if (slot == nullptr || slot->getItem().isEmpty()) {
        return;
    }
    renderItemTooltip(slot->getItem(), mouseX, mouseY);
}

} // namespace mc::client
