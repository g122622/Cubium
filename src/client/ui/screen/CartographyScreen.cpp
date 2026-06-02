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
 * THE SOFTWARE IS PROVIDED "AS IS", WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "CartographyScreen.hpp"
#include "client/renderer/trident/gui/GuiRenderer.hpp"
#include "client/renderer/trident/item/ItemRenderer.hpp"
#include "common/item/Items.hpp"
#include "common/item/items/map/FilledMapItem.hpp"

namespace mc::client {

CartographyScreen::CartographyScreen(ContainerId containerId,
    mc::PlayerInventory* playerInventory,
    ContainerClickSender clickSender,
    ContainerCloseSender closeSender)
    : AbstractContainerScreen<mc::CartographyContainer>(
          std::make_unique<mc::CartographyContainer>(containerId, playerInventory, BlockPos(0, 0, 0), nullptr),
          std::move(clickSender),
          std::move(closeSender))
{
    setImageSize(GUI_WIDTH, GUI_HEIGHT);
}

void CartographyScreen::onInit()
{
    updatePosition();
}

void CartographyScreen::renderContainerBackground()
{
    // 背景
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

    // 箭头区域（从输入到输出）
    const f32 arrowX = static_cast<f32>(m_leftPos + 82);
    const f32 arrowY = static_cast<f32>(m_topPos + 28);
    constexpr u32 ARROW_COLOR = 0xFF404040;

    // 箭头主体
    m_gui->fillRect(arrowX, arrowY + 4.0f, 22.0f, 8.0f, ARROW_COLOR);
    // 箭头头部
    m_gui->fillRect(arrowX + 18.0f, arrowY, 8.0f, 16.0f, ARROW_COLOR);
    m_gui->fillRect(arrowX + 22.0f, arrowY + 2.0f, 4.0f, 12.0f, ARROW_COLOR);
    m_gui->fillRect(arrowX + 26.0f, arrowY + 4.0f, 4.0f, 8.0f, ARROW_COLOR);

    // 渲染地图预览
    const mc::Slot* resultSlot = m_menu->getSlot(mc::CartographyContainer::SLOT_RESULT);
    if (resultSlot != nullptr) {
        const mc::ItemStack& resultStack = resultSlot->getItem();
        if (!resultStack.isEmpty() && resultStack.getItem() == mc::Items::FILLED_MAP) {
            i32 mapId = mc::item::items::FilledMapItem::getMapId(resultStack);
            if (mapId >= 0) {
                const f64 previewX = static_cast<f64>(m_leftPos) + MAP_PREVIEW_X;
                const f64 previewY = static_cast<f64>(m_topPos) + MAP_PREVIEW_Y;
                m_mapRenderer->renderMap(mapId, previewX, previewY, MAP_PREVIEW_SIZE, false, nullptr);
            }
        }
    }
}

void CartographyScreen::renderContainerForeground(i32 mouseX, i32 mouseY)
{
    (void)mouseX;
    (void)mouseY;

    m_gui->drawText(
        "Cartography Table", static_cast<f32>(m_leftPos + 8), static_cast<f32>(m_topPos + 6), 0xFF404040, false);
}

void CartographyScreen::renderItemIcon(const mc::ItemStack& stack, i32 screenX, i32 screenY)
{
    if (stack.isEmpty()) {
        return;
    }

    m_itemRenderer->renderItem(
        *m_gui, stack, static_cast<f32>(screenX), static_cast<f32>(screenY), static_cast<f32>(SLOT_SIZE));
}

void CartographyScreen::renderTooltip(i32 mouseX, i32 mouseY)
{
    mc::Slot* slot = getSlotAt(mouseX, mouseY);
    if (slot != nullptr) {
        renderItemTooltip(slot->getItem(), mouseX, mouseY);
    }
}

} // namespace mc::client
