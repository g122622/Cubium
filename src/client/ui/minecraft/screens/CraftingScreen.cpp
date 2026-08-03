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

#include "CraftingScreen.hpp"

#include "client/ui/Glyph.hpp"
#include "client/ui/kagero/Types.hpp"
#include "client/ui/kagero/paint/PaintContext.hpp"
#include "client/ui/kagero/widget/ContainerInteraction.hpp"
#include "client/ui/kagero/widget/ItemTooltipBuilder.hpp"
#include "client/ui/kagero/widget/TooltipRenderer.hpp"
#include "client/ui/minecraft/screens/ContainerScreenBase.hpp"
#include "client/ui/screen/ScreenManager.hpp"
#include "common/core/Types.hpp"
#include "common/entity/inventory/Slot.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/world/IWorld.hpp"
#include "server/menu/CraftingMenu.hpp"
#include <memory>
#include <utility>
#include <GLFW/glfw3.h>

namespace mc::client::ui::minecraft {

CraftingScreen::CraftingScreen(
    std::unique_ptr<mc::CraftingMenu> menu, ContainerClickSender clickSender, ContainerCloseSender closeSender)
    : ContainerScreenBase<mc::CraftingMenu>("crafting_table", std::move(menu))
{
    setModal(true);
    setPauseScreen(false);

    // 交互引擎：命中回调绑定到基类的 slotAt
    m_interaction = std::make_unique<kagero::widget::ContainerInteraction<mc::CraftingMenu>>(
        m_menu.get(), std::move(clickSender), std::move(closeSender), [this](i32 mouseX, i32 mouseY) {
            return this->slotAt(mouseX, mouseY);
        });

    // 构造 46 个槽位组件（9 网格 + 1 结果 + 27 主背包 + 9 快捷栏）
    buildSlots(mc::CraftingMenu::TOTAL_SLOT_COUNT);
}

std::pair<i32, i32> CraftingScreen::slotLocalPos(i32 slotIndex) const
{
    using M = mc::CraftingMenu;
    // 3x3 合成网格（槽位 0-8）
    if (slotIndex >= M::GRID_SLOT_START && slotIndex < M::GRID_SLOT_START + M::GRID_SLOT_COUNT) {
        const i32 i = slotIndex - M::GRID_SLOT_START;
        return {GRID_X + (i % GRID_COL_COUNT) * SLOT_SPACING, GRID_Y + (i / GRID_COL_COUNT) * SLOT_SPACING};
    }
    // 结果槽（槽位 9）
    if (slotIndex == M::RESULT_SLOT) {
        return {RESULT_X, RESULT_Y};
    }
    // 玩家主背包（槽位 10-36，3x9）
    if (slotIndex >= M::PLAYER_INV_START && slotIndex < M::PLAYER_INV_START + M::PLAYER_INV_COUNT) {
        const i32 i = slotIndex - M::PLAYER_INV_START;
        return {PLAYER_INV_X + (i % 9) * SLOT_SPACING, PLAYER_INV_Y + (i / 9) * SLOT_SPACING};
    }
    // 快捷栏（槽位 37-45，1x9）
    if (slotIndex >= M::HOTBAR_START && slotIndex < M::HOTBAR_START + M::HOTBAR_COUNT) {
        const i32 i = slotIndex - M::HOTBAR_START;
        return {HOTBAR_X + i * SLOT_SPACING, HOTBAR_Y};
    }
    return {0, 0};
}

const mc::ItemStack& CraftingScreen::getCarriedItem() const
{
    return (m_interaction != nullptr) ? m_interaction->getCarriedItem() : mc::ItemStack::EMPTY;
}

void CraftingScreen::renderContainerBackground(kagero::widget::PaintContext& ctx)
{
    const kagero::Rect bgRect(m_leftPos, m_topPos, GUI_WIDTH, GUI_HEIGHT);
    if (m_textureManager != nullptr && m_textureManager->hasCraftingTableTexture() && m_gui != nullptr) {
        m_textureManager->drawCraftingTableBackground(*m_gui, static_cast<f64>(m_leftPos), static_cast<f64>(m_topPos));
    } else {
        ctx.drawFilledRect(bgRect, Colors::fromARGB(255, 198, 198, 198));
        ctx.drawBorder(bgRect, 1.0f, Colors::fromARGB(255, 85, 85, 85));
    }
}

void CraftingScreen::renderContainerForeground(kagero::widget::PaintContext& ctx)
{
    ctx.drawText("Crafting", m_leftPos + TITLE_X, m_topPos + TITLE_Y, Colors::fromARGB(255, 64, 64, 64));
}

void CraftingScreen::renderTooltip(kagero::widget::PaintContext& ctx)
{
    if (m_screenWidth <= 0 || m_screenHeight <= 0) {
        return;
    }

    // 客户端 Player::world() 返回 nullptr（ClientWorld 不继承 IWorld），tooltip 直接传 nullptr
    mc::IWorld* world = nullptr;

    mc::Slot* slot = slotAt(m_mouseX, m_mouseY);
    if (slot == nullptr || slot->getItem().isEmpty()) {
        return;
    }

    auto tooltip = kagero::widget::ItemTooltipBuilder::build(slot->getItem(), world);
    kagero::widget::TooltipRenderer::render(ctx,
        tooltip,
        static_cast<f32>(m_mouseX),
        static_cast<f32>(m_mouseY),
        static_cast<f32>(m_screenWidth),
        static_cast<f32>(m_screenHeight));
}

void CraftingScreen::updateHover(i32 mouseX, i32 mouseY)
{
    ContainerScreenBase::updateHover(mouseX, mouseY);
    if (m_interaction != nullptr) {
        m_interaction->updateHoveredSlot(mouseX, mouseY);
    }
}

bool CraftingScreen::onClick(i32 mouseX, i32 mouseY, i32 button, i32 mods)
{
    if (m_interaction == nullptr) {
        return false;
    }
    const bool handled = m_interaction->onClick(mouseX, mouseY, button, mods);
    syncSlots();
    return handled;
}

bool CraftingScreen::onRelease(i32 mouseX, i32 mouseY, i32 button, i32 mods)
{
    if (m_interaction == nullptr) {
        return false;
    }
    const bool handled = m_interaction->onRelease(mouseX, mouseY, button, mods);
    syncSlots();
    return handled;
}

bool CraftingScreen::onDrag(i32 mouseX, i32 mouseY, i32 deltaX, i32 deltaY, i32 button)
{
    if (m_interaction == nullptr) {
        return false;
    }
    const bool handled = m_interaction->onDrag(mouseX, mouseY, deltaX, deltaY, button);
    syncSlots();
    return handled;
}

bool CraftingScreen::onKey(i32 key, i32 scanCode, i32 action, i32 mods)
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
