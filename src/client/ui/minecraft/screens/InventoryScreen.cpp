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

#include "InventoryScreen.hpp"

#include "client/ui/Glyph.hpp"
#include "client/ui/kagero/Types.hpp"
#include "client/ui/kagero/paint/PaintContext.hpp"
#include "client/ui/kagero/widget/ContainerInteraction.hpp"
#include "client/ui/minecraft/screens/ContainerScreenBase.hpp"
#include "client/ui/screen/ScreenManager.hpp"
#include "common/core/Types.hpp"
#include "common/item/core/ItemStack.hpp"
#include "server/menu/CraftingMenu.hpp"
#include <memory>
#include <utility>
#include <GLFW/glfw3.h>

namespace mc::client::ui::minecraft {

InventoryScreen::InventoryScreen(
    std::unique_ptr<mc::InventoryCraftingMenu> menu, ContainerClickSender clickSender, ContainerCloseSender closeSender)
    : ContainerScreenBase<mc::InventoryCraftingMenu>("inventory", std::move(menu))
{
    // 背包屏模态、不暂停游戏
    setModal(true);
    setPauseScreen(false);

    // 交互引擎：命中回调绑定到基类的 slotAt
    m_interaction = std::make_unique<kagero::widget::ContainerInteraction<mc::InventoryCraftingMenu>>(
        m_menu.get(), std::move(clickSender), std::move(closeSender), [this](i32 mouseX, i32 mouseY) {
            return this->slotAt(mouseX, mouseY);
        });

    // 构造 46 个槽位组件
    buildSlots(mc::InventoryCraftingMenu::TOTAL_SLOT_COUNT);
}

std::pair<i32, i32> InventoryScreen::slotLocalPos(i32 slotIndex) const
{
    using M = mc::InventoryCraftingMenu;
    if (slotIndex == M::RESULT_SLOT) {
        return {RESULT_X, RESULT_Y};
    }
    if (slotIndex >= M::GRID_SLOT_START && slotIndex <= M::GRID_SLOT_END) {
        const i32 i = slotIndex - M::GRID_SLOT_START;
        return {GRID_X + (i % 2) * SLOT_SPACING, GRID_Y + (i / 2) * SLOT_SPACING};
    }
    if (slotIndex >= M::ARMOR_SLOT_START && slotIndex < M::ARMOR_SLOT_START + M::ARMOR_SLOT_COUNT) {
        const i32 i = slotIndex - M::ARMOR_SLOT_START;
        constexpr i32 ARMOR_Y[] = {ARMOR_Y_HEAD, ARMOR_Y_CHEST, ARMOR_Y_LEGS, ARMOR_Y_FEET};
        return {ARMOR_X, ARMOR_Y[i]};
    }
    if (slotIndex >= M::PLAYER_INV_START && slotIndex <= M::PLAYER_INV_END) {
        const i32 i = slotIndex - M::PLAYER_INV_START;
        return {PLAYER_INV_X + (i % 9) * SLOT_SPACING, PLAYER_INV_Y + (i / 9) * SLOT_SPACING};
    }
    if (slotIndex >= M::HOTBAR_START && slotIndex <= M::HOTBAR_END) {
        const i32 i = slotIndex - M::HOTBAR_START;
        return {HOTBAR_X + i * SLOT_SPACING, HOTBAR_Y};
    }
    if (slotIndex == M::OFFHAND_SLOT) {
        return {OFFHAND_X, OFFHAND_Y};
    }
    return {0, 0};
}

const mc::ItemStack& InventoryScreen::getCarriedItem() const
{
    return (m_interaction != nullptr) ? m_interaction->getCarriedItem() : mc::ItemStack::EMPTY;
}

void InventoryScreen::renderContainerBackground(kagero::widget::PaintContext& ctx)
{
    const kagero::Rect bgRect(m_leftPos, m_topPos, GUI_WIDTH, GUI_HEIGHT);
    if (m_textureManager != nullptr && m_textureManager->hasInventoryTexture() && m_gui != nullptr) {
        m_textureManager->drawInventoryBackground(*m_gui, static_cast<f64>(m_leftPos), static_cast<f64>(m_topPos));
    } else {
        ctx.drawFilledRect(bgRect, Colors::fromARGB(255, 198, 198, 198));
        ctx.drawBorder(bgRect, 1.0f, Colors::fromARGB(255, 85, 85, 85));
    }
}

void InventoryScreen::renderContainerForeground(kagero::widget::PaintContext& ctx)
{
    ctx.drawText("Inventory", m_leftPos + TITLE_X, m_topPos + TITLE_Y, Colors::fromARGB(255, 64, 64, 64));
}

void InventoryScreen::updateHover(i32 mouseX, i32 mouseY)
{
    ContainerScreenBase::updateHover(mouseX, mouseY);
    if (m_interaction != nullptr) {
        m_interaction->updateHoveredSlot(mouseX, mouseY);
    }
}

bool InventoryScreen::onClick(i32 mouseX, i32 mouseY, i32 button, i32 mods)
{
    if (m_interaction == nullptr) {
        return false;
    }
    const bool handled = m_interaction->onClick(mouseX, mouseY, button, mods);
    syncSlots();
    return handled;
}

bool InventoryScreen::onRelease(i32 mouseX, i32 mouseY, i32 button, i32 mods)
{
    if (m_interaction == nullptr) {
        return false;
    }
    const bool handled = m_interaction->onRelease(mouseX, mouseY, button, mods);
    syncSlots();
    return handled;
}

bool InventoryScreen::onDrag(i32 mouseX, i32 mouseY, i32 deltaX, i32 deltaY, i32 button)
{
    if (m_interaction == nullptr) {
        return false;
    }
    const bool handled = m_interaction->onDrag(mouseX, mouseY, deltaX, deltaY, button);
    syncSlots();
    return handled;
}

bool InventoryScreen::onKey(i32 key, i32 scanCode, i32 action, i32 mods)
{
    if (m_interaction == nullptr) {
        return false;
    }
    const bool handled = m_interaction->onKey(key, scanCode, action, mods);
    syncSlots();
    if (handled && (key == GLFW_KEY_ESCAPE || key == GLFW_KEY_E) && (action == GLFW_PRESS || action == GLFW_REPEAT)) {
        ScreenManager::instance().closeScreen();
    }
    return handled;
}

} // namespace mc::client::ui::minecraft
