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

#include "FurnaceScreen.hpp"

#include "client/ui/kagero/widget/ItemTooltipBuilder.hpp"
#include "client/ui/kagero/widget/TooltipRenderer.hpp"
#include "client/ui/screen/ScreenManager.hpp"
#include "common/entity/inventory/Slot.hpp"
#include <algorithm>
#include <cmath>
#include <GLFW/glfw3.h>

namespace mc::client::ui::minecraft {

FurnaceScreen::FurnaceScreen(std::unique_ptr<mc::blockentity::FurnaceContainer> menu,
    ContainerClickSender clickSender,
    ContainerCloseSender closeSender)
    : ContainerScreenBase<mc::blockentity::FurnaceContainer>("furnace", std::move(menu))
{
    setModal(true);
    setPauseScreen(false);

    // 交互引擎：命中回调绑定到基类的 slotAt
    m_interaction = std::make_unique<kagero::widget::ContainerInteraction<mc::blockentity::FurnaceContainer>>(
        m_menu.get(), std::move(clickSender), std::move(closeSender), [this](i32 mouseX, i32 mouseY) {
            return this->slotAt(mouseX, mouseY);
        });

    // 构造所有槽位组件（输入 + 燃料 + 输出 + 玩家主背包 + 快捷栏），数量由菜单决定
    buildSlots(m_menu != nullptr ? m_menu->getSlotCount() : 0);
}

std::pair<i32, i32> FurnaceScreen::slotLocalPos(i32 slotIndex) const
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

const mc::ItemStack& FurnaceScreen::getCarriedItem() const
{
    return (m_interaction != nullptr) ? m_interaction->getCarriedItem() : mc::ItemStack::EMPTY;
}

void FurnaceScreen::renderContainerBackground(kagero::widget::PaintContext& ctx)
{
    const kagero::Rect bgRect(m_leftPos, m_topPos, GUI_WIDTH, GUI_HEIGHT);

    if (m_textureManager != nullptr && m_textureManager->hasFurnaceTexture() && m_gui != nullptr) {
        m_textureManager->drawFurnaceBackground(*m_gui, static_cast<f64>(m_leftPos), static_cast<f64>(m_topPos));

        // 燃烧火焰指示器（litProgress 由 FurnaceContainer 经 tracked int 同步）
        const f32 litProgress = (m_menu != nullptr) ? m_menu->getLitProgress() : 0.0f;
        if (litProgress > 0.0f) {
            m_textureManager->drawFurnaceLitProgress(
                *m_gui, static_cast<f64>(m_leftPos), static_cast<f64>(m_topPos), litProgress);
        }

        // 熔炼进度箭头（burnProgress 由 FurnaceContainer 经 tracked int 同步）
        const f32 burnProgress = (m_menu != nullptr) ? m_menu->getBurnProgress() : 0.0f;
        if (burnProgress > 0.0f) {
            m_textureManager->drawFurnaceBurnProgress(
                *m_gui, static_cast<f64>(m_leftPos), static_cast<f64>(m_topPos), burnProgress);
        }
    } else {
        // 后备：纯色矩形 + 纯色火焰/箭头区域
        ctx.drawFilledRect(bgRect, Colors::fromARGB(255, 198, 198, 198));
        ctx.drawBorder(bgRect, 1.0f, Colors::fromARGB(255, 85, 85, 85));

        const f32 litProgress = (m_menu != nullptr) ? m_menu->getLitProgress() : 0.0f;
        if (litProgress > 0.0f) {
            constexpr i32 FIRE_X = 56;
            constexpr i32 FIRE_Y = 36;
            constexpr i32 FIRE_W = 14;
            constexpr i32 FIRE_H = 14;
            i32 visH = static_cast<i32>(std::ceil(static_cast<f64>(litProgress) * 13.0)) + 1;
            visH = std::clamp(visH, 1, 14);
            // 从底部向上填充（火焰随燃烧进度增高）
            ctx.drawFilledRect(kagero::Rect(m_leftPos + FIRE_X, m_topPos + FIRE_Y + FIRE_H - visH, FIRE_W, visH),
                Colors::fromARGB(255, 170, 85, 0));
        }

        const f32 burnProgress = (m_menu != nullptr) ? m_menu->getBurnProgress() : 0.0f;
        if (burnProgress > 0.0f) {
            constexpr i32 ARROW_X = 79;
            constexpr i32 ARROW_Y = 34;
            constexpr i32 ARROW_H = 16;
            i32 visW = static_cast<i32>(std::ceil(static_cast<f64>(burnProgress) * 24.0));
            visW = std::clamp(visW, 0, 24);
            if (visW > 0) {
                ctx.drawFilledRect(kagero::Rect(m_leftPos + ARROW_X, m_topPos + ARROW_Y, visW, ARROW_H),
                    Colors::fromARGB(255, 198, 198, 198));
            }
        }
    }
}

void FurnaceScreen::renderContainerForeground(kagero::widget::PaintContext& ctx)
{
    ctx.drawText("Furnace", m_leftPos + TITLE_X, m_topPos + TITLE_Y, Colors::fromARGB(255, 64, 64, 64));
}

void FurnaceScreen::renderTooltip(kagero::widget::PaintContext& ctx)
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

void FurnaceScreen::updateHover(i32 mouseX, i32 mouseY)
{
    ContainerScreenBase::updateHover(mouseX, mouseY);
    if (m_interaction != nullptr) {
        m_interaction->updateHoveredSlot(mouseX, mouseY);
    }
}

bool FurnaceScreen::onClick(i32 mouseX, i32 mouseY, i32 button, i32 mods)
{
    if (m_interaction == nullptr) {
        return false;
    }
    const bool handled = m_interaction->onClick(mouseX, mouseY, button, mods);
    syncSlots();
    return handled;
}

bool FurnaceScreen::onRelease(i32 mouseX, i32 mouseY, i32 button, i32 mods)
{
    if (m_interaction == nullptr) {
        return false;
    }
    const bool handled = m_interaction->onRelease(mouseX, mouseY, button, mods);
    syncSlots();
    return handled;
}

bool FurnaceScreen::onDrag(i32 mouseX, i32 mouseY, i32 deltaX, i32 deltaY, i32 button)
{
    if (m_interaction == nullptr) {
        return false;
    }
    const bool handled = m_interaction->onDrag(mouseX, mouseY, deltaX, deltaY, button);
    syncSlots();
    return handled;
}

bool FurnaceScreen::onKey(i32 key, i32 scanCode, i32 action, i32 mods)
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
