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

#include "client/ui/screen/CraftingScreen.hpp"
#include "client/renderer/trident/gui/GuiRenderer.hpp"
#include "client/renderer/trident/gui/GuiTextureManager.hpp"
#include "client/renderer/trident/item/ItemRenderer.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "entity/entities/player/Player.hpp"
#include "entity/inventory/Slot.hpp"
#include "item/core/Item.hpp"
#include "item/core/ItemStack.hpp"

namespace mc::client {

// ========== CraftingScreen 实现 ==========

CraftingScreen::CraftingScreen(
    std::unique_ptr<mc::CraftingMenu> menu, ContainerClickSender clickSender, ContainerCloseSender closeSender)
    : AbstractContainerScreen<mc::CraftingMenu>(std::move(menu), std::move(clickSender), std::move(closeSender))
{
    setImageSize(GUI_WIDTH, GUI_HEIGHT);
}

void CraftingScreen::onInit()
{
    updatePosition();
}

void CraftingScreen::renderContainerBackground()
{
    if (m_gui == nullptr) {
        return;
    }

    if (m_textureManager != nullptr && m_textureManager->hasCraftingTableTexture()) {
        m_textureManager->drawCraftingTableBackground(*m_gui, static_cast<f32>(m_leftPos), static_cast<f32>(m_topPos));
    } else {
        // 回退：使用纯色背景
        constexpr u32 BG_COLOR = 0xFFC6C6C6;
        m_gui->fillRect(static_cast<f32>(m_leftPos),
            static_cast<f32>(m_topPos),
            static_cast<f32>(GUI_WIDTH),
            static_cast<f32>(GUI_HEIGHT),
            BG_COLOR);

        constexpr u32 BORDER_COLOR = 0xFF555555;
        m_gui->drawRect(static_cast<f32>(m_leftPos),
            static_cast<f32>(m_topPos),
            static_cast<f32>(GUI_WIDTH),
            static_cast<f32>(GUI_HEIGHT),
            BORDER_COLOR);
    }

    _renderCraftingGrid();
    _renderResultSlot();
    _renderPlayerInventory();
}

void CraftingScreen::renderContainerForeground(i32 mouseX, i32 mouseY)
{
    (void)mouseX;
    (void)mouseY;

    if (m_gui != nullptr && m_gui->font() != nullptr) {
        m_gui->drawText(
            "Crafting", static_cast<f32>(m_leftPos + TITLE_X), static_cast<f32>(m_topPos + TITLE_Y), 0xFF404040, false);
    }
}

void CraftingScreen::_renderCraftingGrid()
{
    MC_ASSERT_RELEASE(m_menu != nullptr);

    for (i32 i = 0; i < mc::CraftingMenu::GRID_SLOT_COUNT; ++i) {
        mc::Slot* slot = m_menu->getSlot(i);
        if (slot != nullptr) {
            const i32 gridX = i % mc::CraftingMenu::GRID_WIDTH;
            const i32 gridY = i / mc::CraftingMenu::GRID_WIDTH;
            const i32 screenX = m_leftPos + GRID_X + gridX * SLOT_SPACING;
            const i32 screenY = m_topPos + GRID_Y + gridY * SLOT_SPACING;

            renderSlot(*slot, screenX, screenY);
        }
    }
}

void CraftingScreen::_renderResultSlot()
{
    MC_ASSERT_RELEASE(m_menu != nullptr);

    mc::Slot* resultSlot = m_menu->getSlot(mc::CraftingMenu::RESULT_SLOT);
    if (resultSlot != nullptr) {
        const i32 screenX = m_leftPos + RESULT_X;
        const i32 screenY = m_topPos + RESULT_Y;

        renderSlot(*resultSlot, screenX, screenY);
    }
}

void CraftingScreen::_renderPlayerInventory()
{
    MC_ASSERT_RELEASE(m_menu != nullptr);

    // 主背包槽位
    constexpr i32 mainInvSlotCount = PLAYER_INV_ROW_COUNT * PLAYER_INV_COL_COUNT;
    for (i32 i = mc::CraftingMenu::PLAYER_INV_START; i < mc::CraftingMenu::PLAYER_INV_START + mainInvSlotCount; ++i) {
        mc::Slot* slot = m_menu->getSlot(i);
        if (slot != nullptr) {
            const i32 invIndex = i - mc::CraftingMenu::PLAYER_INV_START;
            const i32 row = invIndex / PLAYER_INV_COL_COUNT;
            const i32 col = invIndex % PLAYER_INV_COL_COUNT;
            const i32 screenX = m_leftPos + PLAYER_INV_X + col * SLOT_SPACING;
            const i32 screenY = m_topPos + PLAYER_INV_Y + row * SLOT_SPACING;

            renderSlot(*slot, screenX, screenY);
        }
    }

    // 快捷栏槽位
    constexpr i32 hotbarStart = mc::CraftingMenu::PLAYER_INV_START + mainInvSlotCount;
    for (i32 i = hotbarStart; i < hotbarStart + HOTBAR_COL_COUNT; ++i) {
        mc::Slot* slot = m_menu->getSlot(i);
        if (slot != nullptr) {
            const i32 col = i - hotbarStart;
            const i32 screenX = m_leftPos + PLAYER_INV_X + col * SLOT_SPACING;
            const i32 screenY = m_topPos + PLAYER_INV_Y + PLAYER_INV_ROW_COUNT * SLOT_SPACING + 4; // 4像素间隔

            renderSlot(*slot, screenX, screenY);
        }
    }
}

void CraftingScreen::renderItemIcon(const mc::ItemStack& stack, i32 screenX, i32 screenY)
{
    if (stack.isEmpty()) {
        return;
    }

    if (m_itemRenderer != nullptr) {
        m_itemRenderer->renderItem(
            *m_gui, stack, static_cast<f32>(screenX), static_cast<f32>(screenY), static_cast<f32>(SLOT_SIZE));
    } else {
        // 回退：绘制占位符
        MC_ASSERT_RELEASE(m_gui != nullptr);
        m_gui->fillRect(static_cast<f32>(screenX),
            static_cast<f32>(screenY),
            static_cast<f32>(SLOT_SIZE),
            static_cast<f32>(SLOT_SIZE),
            0x80FFFFFF);
    }
}

void CraftingScreen::renderTooltip(i32 mouseX, i32 mouseY)
{
    mc::Slot* slot = getSlotAt(mouseX, mouseY);
    if (slot == nullptr || slot->getItem().isEmpty()) {
        return;
    }

    renderItemTooltip(slot->getItem(), mouseX, mouseY);
}

bool CraftingScreen::onSlotClick(mc::Slot& slot, i32 slotIndex, i32 button)
{
    // 合成结果槽位的点击由 CraftingMenu::clicked() 处理（_handleResultSlotClick），
    // 护甲槽位的验证由 ArmorSlot::mayPlace() 处理，无需屏幕层额外逻辑。
    // 所有交互类型（Shift+点击、拖拽、数字键交换等）由基类 AbstractContainerScreen 统一处理。
    (void)slot;
    (void)slotIndex;
    (void)button;
    return false;
}

// ========== InventoryCraftingScreen 实现 ==========

InventoryCraftingScreen::InventoryCraftingScreen(
    std::unique_ptr<mc::InventoryCraftingMenu> menu, ContainerClickSender clickSender, ContainerCloseSender closeSender)
    : AbstractContainerScreen<mc::InventoryCraftingMenu>(
          std::move(menu), std::move(clickSender), std::move(closeSender))
{
    setImageSize(GUI_WIDTH, GUI_HEIGHT);
}

void InventoryCraftingScreen::onInit()
{
    updatePosition();
}

void InventoryCraftingScreen::renderContainerBackground()
{
    if (m_gui == nullptr) {
        return;
    }

    if (m_textureManager != nullptr && m_textureManager->hasInventoryTexture()) {
        m_textureManager->drawInventoryBackground(*m_gui, static_cast<f32>(m_leftPos), static_cast<f32>(m_topPos));
    } else {
        // 回退：使用纯色背景
        constexpr u32 BG_COLOR = 0xFFC6C6C6;
        m_gui->fillRect(static_cast<f32>(m_leftPos),
            static_cast<f32>(m_topPos),
            static_cast<f32>(GUI_WIDTH),
            static_cast<f32>(GUI_HEIGHT),
            BG_COLOR);

        constexpr u32 BORDER_COLOR = 0xFF555555;
        m_gui->drawRect(static_cast<f32>(m_leftPos),
            static_cast<f32>(m_topPos),
            static_cast<f32>(GUI_WIDTH),
            static_cast<f32>(GUI_HEIGHT),
            BORDER_COLOR);
    }

    _renderCraftingGrid();
    _renderResultSlot();
    _renderArmorSlots();
    _renderOffhandSlot();
    _renderPlayerInventory();
}

void InventoryCraftingScreen::renderContainerForeground(i32 mouseX, i32 mouseY)
{
    (void)mouseX;
    (void)mouseY;

    if (m_gui != nullptr && m_gui->font() != nullptr) {
        m_gui->drawText("Inventory",
            static_cast<f32>(m_leftPos + TITLE_X),
            static_cast<f32>(m_topPos + TITLE_Y),
            0xFF404040,
            false);
    }
}

void InventoryCraftingScreen::_renderCraftingGrid()
{
    MC_ASSERT_RELEASE(m_menu != nullptr);

    for (i32 i = 0; i < mc::InventoryCraftingMenu::GRID_SLOT_COUNT; ++i) {
        const i32 slotIndex = mc::InventoryCraftingMenu::GRID_SLOT_START + i;
        const mc::Slot* slot = m_menu->getSlot(slotIndex);

        if (slot != nullptr) {
            const i32 gridX = i % GRID_COL_COUNT;
            const i32 gridY = i / GRID_COL_COUNT;
            const i32 screenX = m_leftPos + GRID_X + gridX * SLOT_SPACING;
            const i32 screenY = m_topPos + GRID_Y + gridY * SLOT_SPACING;

            renderSlot(*slot, screenX, screenY);
        }
    }
}

void InventoryCraftingScreen::_renderResultSlot()
{
    MC_ASSERT_RELEASE(m_menu != nullptr);

    const mc::Slot* resultSlot = m_menu->getSlot(mc::InventoryCraftingMenu::RESULT_SLOT);
    if (resultSlot != nullptr) {
        const i32 screenX = m_leftPos + RESULT_X;
        const i32 screenY = m_topPos + RESULT_Y;

        renderSlot(*resultSlot, screenX, screenY);
    }
}

void InventoryCraftingScreen::_renderArmorSlots()
{
    MC_ASSERT_RELEASE(m_menu != nullptr);

    constexpr i32 ARMOR_Y_POSITIONS[] = {
        ARMOR_Y_HEAD,
        ARMOR_Y_CHEST,
        ARMOR_Y_LEGS,
        ARMOR_Y_FEET,
    };

    for (i32 i = 0; i < mc::InventoryCraftingMenu::ARMOR_SLOT_COUNT; ++i) {
        const i32 slotIndex = mc::InventoryCraftingMenu::ARMOR_SLOT_START + i;
        const mc::Slot* slot = m_menu->getSlot(slotIndex);

        if (slot != nullptr) {
            const i32 screenX = m_leftPos + ARMOR_X;
            const i32 screenY = m_topPos + ARMOR_Y_POSITIONS[i];

            renderSlot(*slot, screenX, screenY);
        }
    }
}

void InventoryCraftingScreen::_renderOffhandSlot()
{
    MC_ASSERT_RELEASE(m_menu != nullptr);

    const mc::Slot* offhandSlot = m_menu->getSlot(mc::InventoryCraftingMenu::OFFHAND_SLOT);
    if (offhandSlot != nullptr) {
        const i32 screenX = m_leftPos + OFFHAND_X;
        const i32 screenY = m_topPos + OFFHAND_Y;

        renderSlot(*offhandSlot, screenX, screenY);
    }
}

void InventoryCraftingScreen::_renderPlayerInventory()
{
    MC_ASSERT_RELEASE(m_menu != nullptr);

    // 主背包槽位
    for (i32 i = 0; i < mc::InventoryCraftingMenu::PLAYER_INV_COUNT; ++i) {
        const i32 slotIndex = mc::InventoryCraftingMenu::PLAYER_INV_START + i;
        const mc::Slot* slot = m_menu->getSlot(slotIndex);

        if (slot != nullptr) {
            const i32 row = i / INVENTORY_COL_COUNT;
            const i32 col = i % INVENTORY_COL_COUNT;
            const i32 screenX = m_leftPos + PLAYER_INV_X + col * SLOT_SPACING;
            const i32 screenY = m_topPos + PLAYER_INV_Y + row * SLOT_SPACING;

            renderSlot(*slot, screenX, screenY);
        }
    }

    // 快捷栏槽位
    for (i32 i = 0; i < mc::InventoryCraftingMenu::HOTBAR_COUNT; ++i) {
        const i32 slotIndex = mc::InventoryCraftingMenu::HOTBAR_START + i;
        const mc::Slot* slot = m_menu->getSlot(slotIndex);

        if (slot != nullptr) {
            const i32 screenX = m_leftPos + HOTBAR_X + i * SLOT_SPACING;
            const i32 screenY = m_topPos + HOTBAR_Y;

            renderSlot(*slot, screenX, screenY);
        }
    }
}

void InventoryCraftingScreen::renderItemIcon(const mc::ItemStack& stack, i32 screenX, i32 screenY)
{
    if (stack.isEmpty()) {
        return;
    }

    if (m_itemRenderer != nullptr) {
        m_itemRenderer->renderItem(
            *m_gui, stack, static_cast<f32>(screenX), static_cast<f32>(screenY), static_cast<f32>(SLOT_SIZE));
    } else {
        // 回退：绘制占位符
        MC_ASSERT_RELEASE(m_gui != nullptr);
        m_gui->fillRect(static_cast<f32>(screenX),
            static_cast<f32>(screenY),
            static_cast<f32>(SLOT_SIZE),
            static_cast<f32>(SLOT_SIZE),
            0x80FFFFFF);
    }
}

void InventoryCraftingScreen::renderTooltip(i32 mouseX, i32 mouseY)
{
    mc::Slot* slot = getSlotAt(mouseX, mouseY);
    if (slot == nullptr || slot->getItem().isEmpty()) {
        return;
    }

    renderItemTooltip(slot->getItem(), mouseX, mouseY);
}

bool InventoryCraftingScreen::onSlotClick(mc::Slot& slot, i32 slotIndex, i32 button)
{
    // 合成结果槽位的点击由 InventoryCraftingMenu::clicked() 处理（_handleResultSlotClick），
    // 护甲槽位的验证由 ArmorSlot::mayPlace() 处理，无需屏幕层额外逻辑。
    // 所有交互类型（Shift+点击、拖拽、数字键交换等）由基类 AbstractContainerScreen 统一处理。
    (void)slot;
    (void)slotIndex;
    (void)button;
    return false;
}

} // namespace mc::client
