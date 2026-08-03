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

#include "ChestScreen.hpp"

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
#include "common/entity/inventory/container/ChestContainer.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/world/IWorld.hpp"
#include <memory>
#include <utility>
#include <GLFW/glfw3.h>

namespace mc::client::ui::minecraft {

ChestScreen::ChestScreen(std::unique_ptr<mc::blockentity::ChestContainer> menu,
    ContainerClickSender clickSender,
    ContainerCloseSender closeSender)
    : ContainerScreenBase<mc::blockentity::ChestContainer>("chest", std::move(menu))
{
    setModal(true);
    setPauseScreen(false);

    // 交互引擎：命中回调绑定到基类的 slotAt
    m_interaction = std::make_unique<kagero::widget::ContainerInteraction<mc::blockentity::ChestContainer>>(
        m_menu.get(), std::move(clickSender), std::move(closeSender), [this](i32 mouseX, i32 mouseY) {
            return this->slotAt(mouseX, mouseY);
        });

    // 构造所有槽位组件（箱子槽 + 玩家主背包 + 快捷栏），数量由菜单决定
    buildSlots(m_menu != nullptr ? m_menu->getSlotCount() : 0);
}

i32 ChestScreen::guiHeight() const
{
    const i32 rows = (m_menu != nullptr) ? m_menu->getRowCount() : 0;
    return BASE_GUI_HEIGHT + rows * SLOT_SPACING;
}

std::pair<i32, i32> ChestScreen::slotLocalPos(i32 slotIndex) const
{
    // 槽位坐标直接取自菜单内 Slot 的 getX/getY（菜单为权威布局源）
    if (m_menu == nullptr) {
        return {0, 0};
    }
    const mc::Slot* slot = m_menu->getSlot(slotIndex);
    if (slot == nullptr) {
        return {0, 0};
    }
    return {slot->getX(), slot->getY()};
}

const mc::ItemStack& ChestScreen::getCarriedItem() const
{
    return (m_interaction != nullptr) ? m_interaction->getCarriedItem() : mc::ItemStack::EMPTY;
}

void ChestScreen::renderContainerBackground(kagero::widget::PaintContext& ctx)
{
    const kagero::Rect bgRect(m_leftPos, m_topPos, GUI_WIDTH, guiHeight());
    ctx.drawFilledRect(bgRect, Colors::fromARGB(255, 198, 198, 198));
    ctx.drawBorder(bgRect, 1.0f, Colors::fromARGB(255, 85, 85, 85));
}

void ChestScreen::renderContainerForeground(kagero::widget::PaintContext& ctx)
{
    ctx.drawText("Chest", m_leftPos + TITLE_X, m_topPos + TITLE_Y, Colors::fromARGB(255, 64, 64, 64));
}

void ChestScreen::renderTooltip(kagero::widget::PaintContext& ctx)
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

void ChestScreen::updateHover(i32 mouseX, i32 mouseY)
{
    ContainerScreenBase::updateHover(mouseX, mouseY);
    if (m_interaction != nullptr) {
        m_interaction->updateHoveredSlot(mouseX, mouseY);
    }
}

bool ChestScreen::onClick(i32 mouseX, i32 mouseY, i32 button, i32 mods)
{
    if (m_interaction == nullptr) {
        return false;
    }
    const bool handled = m_interaction->onClick(mouseX, mouseY, button, mods);
    syncSlots();
    return handled;
}

bool ChestScreen::onRelease(i32 mouseX, i32 mouseY, i32 button, i32 mods)
{
    if (m_interaction == nullptr) {
        return false;
    }
    const bool handled = m_interaction->onRelease(mouseX, mouseY, button, mods);
    syncSlots();
    return handled;
}

bool ChestScreen::onDrag(i32 mouseX, i32 mouseY, i32 deltaX, i32 deltaY, i32 button)
{
    if (m_interaction == nullptr) {
        return false;
    }
    const bool handled = m_interaction->onDrag(mouseX, mouseY, deltaX, deltaY, button);
    syncSlots();
    return handled;
}

bool ChestScreen::onKey(i32 key, i32 scanCode, i32 action, i32 mods)
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
