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
    // 计算居中位置
    updatePosition();
}

void CraftingScreen::renderContainerBackground()
{
    if (m_gui == nullptr) {
        return;
    }

    // 渲染工作台GUI背景
    if (m_textureManager != nullptr && m_textureManager->hasCraftingTableTexture()) {
        // 使用纹理渲染背景
        m_textureManager->drawCraftingTableBackground(*m_gui, static_cast<f32>(m_leftPos), static_cast<f32>(m_topPos));
    } else {
        // 使用纯色背景（回退）
        // 背景颜色 (MC GUI 浅灰色)
        constexpr u32 BG_COLOR = 0xFFC6C6C6;
        m_gui->fillRect(static_cast<f32>(m_leftPos),
            static_cast<f32>(m_topPos),
            static_cast<f32>(GUI_WIDTH),
            static_cast<f32>(GUI_HEIGHT),
            BG_COLOR);

        // 绘制边框
        constexpr u32 BORDER_COLOR = 0xFF555555;
        m_gui->drawRect(static_cast<f32>(m_leftPos),
            static_cast<f32>(m_topPos),
            static_cast<f32>(GUI_WIDTH),
            static_cast<f32>(GUI_HEIGHT),
            BORDER_COLOR);
    }

    // 渲染合成网格和结果槽位的物品
    renderCraftingGrid();
    renderResultSlot();
    renderPlayerInventory();
}

void CraftingScreen::renderContainerForeground(i32 mouseX, i32 mouseY)
{
    (void)mouseX;
    (void)mouseY;

    // 渲染标题 "Crafting"
    if (m_gui != nullptr && m_gui->font() != nullptr) {
        m_gui->drawText(
            "Crafting", static_cast<f32>(m_leftPos + TITLE_X), static_cast<f32>(m_topPos + TITLE_Y), 0xFF404040, false);
    }
}

void CraftingScreen::renderCraftingGrid()
{
    if (m_menu == nullptr || m_gui == nullptr) {
        return;
    }

    // 遍历合成网格槽位 (0-8)
    for (i32 i = 0; i < mc::CraftingMenu::GRID_SLOT_COUNT; ++i) {
        mc::Slot* slot = m_menu->getSlot(i);
        if (slot != nullptr) {
            // 计算位置
            i32 gridX = i % 3;
            i32 gridY = i / 3;
            i32 screenX = m_leftPos + GRID_X + gridX * SLOT_SPACING;
            i32 screenY = m_topPos + GRID_Y + gridY * SLOT_SPACING;

            renderSlot(*slot, screenX, screenY);
        }
    }
}

void CraftingScreen::renderResultSlot()
{
    if (m_menu == nullptr || m_gui == nullptr) {
        return;
    }

    mc::Slot* resultSlot = m_menu->getSlot(mc::CraftingMenu::RESULT_SLOT);
    if (resultSlot != nullptr) {
        i32 screenX = m_leftPos + RESULT_X;
        i32 screenY = m_topPos + RESULT_Y;

        renderSlot(*resultSlot, screenX, screenY);
    }
}

void CraftingScreen::renderPlayerInventory()
{
    if (m_menu == nullptr || m_gui == nullptr) {
        return;
    }

    // 主背包槽位 (10-36)
    for (i32 i = mc::CraftingMenu::PLAYER_INV_START; i < mc::CraftingMenu::PLAYER_INV_START + 27; ++i) {
        mc::Slot* slot = m_menu->getSlot(i);
        if (slot != nullptr) {
            // 计算位置 (3行9列)
            i32 invIndex = i - mc::CraftingMenu::PLAYER_INV_START;
            i32 row = invIndex / 9;
            i32 col = invIndex % 9;
            i32 screenX = m_leftPos + PLAYER_INV_X + col * SLOT_SPACING;
            i32 screenY = m_topPos + PLAYER_INV_Y + row * SLOT_SPACING;

            renderSlot(*slot, screenX, screenY);
        }
    }

    // 快捷栏槽位 (37-45)
    for (i32 i = mc::CraftingMenu::PLAYER_INV_START + 27; i < mc::CraftingMenu::PLAYER_INV_START + 27 + 9; ++i) {
        mc::Slot* slot = m_menu->getSlot(i);
        if (slot != nullptr) {
            i32 col = i - (mc::CraftingMenu::PLAYER_INV_START + 27);
            i32 screenX = m_leftPos + PLAYER_INV_X + col * SLOT_SPACING;
            i32 screenY = m_topPos + PLAYER_INV_Y + 3 * SLOT_SPACING + 4; // 4像素间隔

            renderSlot(*slot, screenX, screenY);
        }
    }
}

void CraftingScreen::renderItemIcon(const mc::ItemStack& stack, i32 screenX, i32 screenY)
{
    if (m_gui == nullptr || stack.isEmpty()) {
        return;
    }

    // 使用 ItemRenderer 渲染物品
    if (m_itemRenderer != nullptr) {
        m_itemRenderer->renderItem(
            *m_gui, stack, static_cast<f32>(screenX), static_cast<f32>(screenY), static_cast<f32>(SLOT_SIZE));
    } else {
        // 回退：绘制占位符
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
    // 特殊处理结果槽位
    if (isResultSlot(slotIndex)) {
        // 结果槽位点击逻辑
        // 1. 检查是否有结果
        // 2. 检查玩家是否持有物品
        // 3. 如果持有物品且能合并，合并
        // 4. 否则取出结果
        // 5. 消耗原料（由菜单处理）
    }

    // 普通槽位处理
    return AbstractContainerScreen::onSlotClick(slot, slotIndex, button);
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
    // 计算居中位置
    updatePosition();
}

void InventoryCraftingScreen::renderContainerBackground()
{
    if (m_gui == nullptr) {
        return;
    }

    // 渲染背包GUI背景
    if (m_textureManager != nullptr && m_textureManager->hasInventoryTexture()) {
        // 使用纹理渲染背景
        m_textureManager->drawInventoryBackground(*m_gui, static_cast<f32>(m_leftPos), static_cast<f32>(m_topPos));
    } else {
        // 使用纯色背景（回退）
        // 背景颜色 (MC GUI 浅灰色)
        constexpr u32 BG_COLOR = 0xFFC6C6C6;
        m_gui->fillRect(static_cast<f32>(m_leftPos),
            static_cast<f32>(m_topPos),
            static_cast<f32>(GUI_WIDTH),
            static_cast<f32>(GUI_HEIGHT),
            BG_COLOR);

        // 绘制边框
        constexpr u32 BORDER_COLOR = 0xFF555555;
        m_gui->drawRect(static_cast<f32>(m_leftPos),
            static_cast<f32>(m_topPos),
            static_cast<f32>(GUI_WIDTH),
            static_cast<f32>(GUI_HEIGHT),
            BORDER_COLOR);
    }

    // 渲染所有槽位的物品
    renderCraftingGrid();
    renderResultSlot();
    renderArmorSlots();
    renderOffhandSlot();
    renderPlayerInventory();
}

void InventoryCraftingScreen::renderContainerForeground(i32 mouseX, i32 mouseY)
{
    (void)mouseX;
    (void)mouseY;

    // 渲染标题 "Inventory"
    if (m_gui != nullptr && m_gui->font() != nullptr) {
        m_gui->drawText("Inventory",
            static_cast<f32>(m_leftPos + TITLE_X),
            static_cast<f32>(m_topPos + TITLE_Y),
            0xFF404040,
            false);
    }
}

void InventoryCraftingScreen::renderCraftingGrid()
{
    if (m_menu == nullptr || m_gui == nullptr) {
        return;
    }

    // 遍历合成网格槽位 (槽位 1-4)
    for (i32 i = 0; i < mc::InventoryCraftingMenu::GRID_SLOT_COUNT; ++i) {
        const i32 slotIndex = mc::InventoryCraftingMenu::GRID_SLOT_START + i;
        const mc::Slot* slot = m_menu->getSlot(slotIndex);

        if (slot != nullptr) {
            // 计算位置 (2x2 网格)
            const i32 gridX = i % 2;
            const i32 gridY = i / 2;
            const i32 screenX = m_leftPos + GRID_X + gridX * SLOT_SPACING;
            const i32 screenY = m_topPos + GRID_Y + gridY * SLOT_SPACING;

            renderSlot(*slot, screenX, screenY);
        }
    }
}

void InventoryCraftingScreen::renderResultSlot()
{
    if (m_menu == nullptr || m_gui == nullptr) {
        return;
    }

    const mc::Slot* resultSlot = m_menu->getSlot(mc::InventoryCraftingMenu::RESULT_SLOT);
    if (resultSlot != nullptr) {
        const i32 screenX = m_leftPos + RESULT_X;
        const i32 screenY = m_topPos + RESULT_Y;

        renderSlot(*resultSlot, screenX, screenY);
    }
}

void InventoryCraftingScreen::renderArmorSlots()
{
    if (m_menu == nullptr || m_gui == nullptr) {
        return;
    }

    // 护甲槽位位置常量
    constexpr i32 ARMOR_Y_POSITIONS[] = {
        ARMOR_Y_HEAD,  // 头盔
        ARMOR_Y_CHEST, // 胸甲
        ARMOR_Y_LEGS,  // 护腿
        ARMOR_Y_FEET   // 靴子
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

void InventoryCraftingScreen::renderOffhandSlot()
{
    if (m_menu == nullptr || m_gui == nullptr) {
        return;
    }

    const mc::Slot* offhandSlot = m_menu->getSlot(mc::InventoryCraftingMenu::OFFHAND_SLOT);
    if (offhandSlot != nullptr) {
        const i32 screenX = m_leftPos + OFFHAND_X;
        const i32 screenY = m_topPos + OFFHAND_Y;

        renderSlot(*offhandSlot, screenX, screenY);
    }
}

void InventoryCraftingScreen::renderPlayerInventory()
{
    if (m_menu == nullptr || m_gui == nullptr) {
        return;
    }

    // 主背包槽位 (槽位 9-35)
    for (i32 i = 0; i < mc::InventoryCraftingMenu::PLAYER_INV_COUNT; ++i) {
        const i32 slotIndex = mc::InventoryCraftingMenu::PLAYER_INV_START + i;
        const mc::Slot* slot = m_menu->getSlot(slotIndex);

        if (slot != nullptr) {
            // 计算位置 (3行9列)
            const i32 row = i / 9;
            const i32 col = i % 9;
            const i32 screenX = m_leftPos + PLAYER_INV_X + col * SLOT_SPACING;
            const i32 screenY = m_topPos + PLAYER_INV_Y + row * SLOT_SPACING;

            renderSlot(*slot, screenX, screenY);
        }
    }

    // 快捷栏槽位 (槽位 36-44)
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
    if (m_gui == nullptr || stack.isEmpty()) {
        return;
    }

    // 使用 ItemRenderer 渲染物品
    if (m_itemRenderer != nullptr) {
        m_itemRenderer->renderItem(
            *m_gui, stack, static_cast<f32>(screenX), static_cast<f32>(screenY), static_cast<f32>(SLOT_SIZE));
    } else {
        // 回退：绘制占位符
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
    // 特殊处理结果槽位
    if (isResultSlot(slotIndex)) {
        // 结果槽位点击逻辑
        // 点击结果槽位时，合成结果物品会被取出
        // 原料会被消耗
    }

    // 护甲槽位特殊处理
    if (isArmorSlot(slotIndex)) {
        // 护甲槽位只能放置护甲
        // ArmorSlot 的 mayPlace 方法会进行验证
    }

    return AbstractContainerScreen::onSlotClick(slot, slotIndex, button);
}

} // namespace mc::client
