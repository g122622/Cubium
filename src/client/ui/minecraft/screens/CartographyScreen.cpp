/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to Use, copy, modify, merge, publish, distribute, sublicense, and/or sell
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

#include "CartographyScreen.hpp"

#include "client/ui/kagero/widget/ItemTooltipBuilder.hpp"
#include "client/ui/kagero/widget/TooltipRenderer.hpp"
#include "client/ui/screen/ScreenManager.hpp"
#include "common/entity/inventory/Slot.hpp"
#include "common/item/Items.hpp"
#include "common/item/items/map/FilledMapItem.hpp"
#include <GLFW/glfw3.h>

namespace mc::client::ui::minecraft {

CartographyScreen::CartographyScreen(
    std::unique_ptr<mc::CartographyContainer> menu, ContainerClickSender clickSender, ContainerCloseSender closeSender)
    : ContainerScreenBase<mc::CartographyContainer>("cartography", std::move(menu))
{
    setModal(true);
    setPauseScreen(false);

    // 交互引擎：命中回调绑定到基类的 slotAt
    m_interaction = std::make_unique<kagero::widget::ContainerInteraction<mc::CartographyContainer>>(
        m_menu.get(), std::move(clickSender), std::move(closeSender), [this](i32 mouseX, i32 mouseY) {
            return this->slotAt(mouseX, mouseY);
        });

    // 构造所有槽位组件（地图 + 材料 + 结果 + 玩家主背包 + 快捷栏），数量由菜单决定
    buildSlots(m_menu != nullptr ? m_menu->getSlotCount() : 0);
}

std::pair<i32, i32> CartographyScreen::slotLocalPos(i32 slotIndex) const
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

const mc::ItemStack& CartographyScreen::getCarriedItem() const
{
    return (m_interaction != nullptr) ? m_interaction->getCarriedItem() : mc::ItemStack::EMPTY;
}

void CartographyScreen::renderContainerBackground(kagero::widget::PaintContext& ctx)
{
    // 制图台无独立纹理，统一用纯色矩形 + 手绘箭头
    const kagero::Rect bgRect(m_leftPos, m_topPos, GUI_WIDTH, GUI_HEIGHT);
    ctx.drawFilledRect(bgRect, Colors::fromARGB(255, 198, 198, 198));
    ctx.drawBorder(bgRect, 1.0f, Colors::fromARGB(255, 85, 85, 85));

    // 箭头（从输入/材料槽指向结果槽）
    const u32 arrowColor = Colors::fromARGB(255, 64, 64, 64);
    const i32 arrowX = m_leftPos + ARROW_X;
    const i32 arrowY = m_topPos + ARROW_Y;
    // 箭头主体
    ctx.drawFilledRect(kagero::Rect(arrowX, arrowY + 4, 22, 8), arrowColor);
    // 箭头头部
    ctx.drawFilledRect(kagero::Rect(arrowX + 18, arrowY, 8, 16), arrowColor);
    ctx.drawFilledRect(kagero::Rect(arrowX + 22, arrowY + 2, 4, 12), arrowColor);
    ctx.drawFilledRect(kagero::Rect(arrowX + 26, arrowY + 4, 4, 8), arrowColor);

    // 结果槽为已填充地图时绘制地图预览（MapRenderer 未注入则跳过）
    if (m_mapRenderer == nullptr || m_menu == nullptr) {
        return;
    }
    const mc::Slot* resultSlot = m_menu->getSlot(mc::CartographyContainer::SLOT_RESULT);
    if (resultSlot == nullptr) {
        return;
    }
    const mc::ItemStack& resultStack = resultSlot->getItem();
    if (resultStack.isEmpty() || resultStack.getItem() != mc::Items::FILLED_MAP) {
        return;
    }
    const i32 mapId = mc::item::items::FilledMapItem::getMapId(resultStack);
    if (mapId < 0) {
        return;
    }
    const f64 previewX = static_cast<f64>(m_leftPos) + MAP_PREVIEW_X;
    const f64 previewY = static_cast<f64>(m_topPos) + MAP_PREVIEW_Y;
    m_mapRenderer->renderMap(mapId, previewX, previewY, MAP_PREVIEW_SIZE, false, nullptr);
}

void CartographyScreen::renderContainerForeground(kagero::widget::PaintContext& ctx)
{
    ctx.drawText("Cartography Table", m_leftPos + TITLE_X, m_topPos + TITLE_Y, Colors::fromARGB(255, 64, 64, 64));
}

void CartographyScreen::renderTooltip(kagero::widget::PaintContext& ctx)
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

void CartographyScreen::updateHover(i32 mouseX, i32 mouseY)
{
    ContainerScreenBase::updateHover(mouseX, mouseY);
    if (m_interaction != nullptr) {
        m_interaction->updateHoveredSlot(mouseX, mouseY);
    }
}

bool CartographyScreen::onClick(i32 mouseX, i32 mouseY, i32 button, i32 mods)
{
    if (m_interaction == nullptr) {
        return false;
    }
    const bool handled = m_interaction->onClick(mouseX, mouseY, button, mods);
    syncSlots();
    return handled;
}

bool CartographyScreen::onRelease(i32 mouseX, i32 mouseY, i32 button, i32 mods)
{
    if (m_interaction == nullptr) {
        return false;
    }
    const bool handled = m_interaction->onRelease(mouseX, mouseY, button, mods);
    syncSlots();
    return handled;
}

bool CartographyScreen::onDrag(i32 mouseX, i32 mouseY, i32 deltaX, i32 deltaY, i32 button)
{
    if (m_interaction == nullptr) {
        return false;
    }
    const bool handled = m_interaction->onDrag(mouseX, mouseY, deltaX, deltaY, button);
    syncSlots();
    return handled;
}

bool CartographyScreen::onKey(i32 key, i32 scanCode, i32 action, i32 mods)
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
