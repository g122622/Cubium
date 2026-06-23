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

#include "client/ui/screen/FurnaceScreen.hpp"
#include "client/renderer/trident/gui/GuiRenderer.hpp"
#include "client/renderer/trident/gui/GuiTextureManager.hpp"
#include "client/renderer/trident/item/ItemRenderer.hpp"
#include "common/world/blockentity/processing/AbstractFurnaceEntity.hpp"
#include "common/world/blockentity/processing/FurnaceInventory.hpp"
#include <algorithm>

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
    if (m_gui == nullptr) {
        return;
    }

    // 绘制熔炉背景纹理（如果可用）或纯色后备
    if (m_textureManager != nullptr && m_textureManager->hasFurnaceTexture()) {
        m_textureManager->drawFurnaceBackground(*m_gui, static_cast<f64>(m_leftPos), static_cast<f64>(m_topPos));

        // 绘制燃烧火焰指示器
        f32 litProgress = getLitProgress();
        if (litProgress > 0.0f) {
            m_textureManager->drawFurnaceLitProgress(
                *m_gui, static_cast<f64>(m_leftPos), static_cast<f64>(m_topPos), litProgress);
        }

        // 绘制熔炼进度箭头
        f32 burnProgress = getBurnProgress();
        if (burnProgress > 0.0f) {
            m_textureManager->drawFurnaceBurnProgress(
                *m_gui, static_cast<f64>(m_leftPos), static_cast<f64>(m_topPos), burnProgress);
        }
    } else {
        // 后备：使用纯色矩形
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

        // 后备：纯色绘制火焰指示器区域
        if (getLitProgress() > 0.0f) {
            constexpr f32 FIRE_X = 56.0f;
            constexpr f32 FIRE_Y = 36.0f;
            constexpr f32 FIRE_W = 14.0f;
            constexpr f32 FIRE_H = 14.0f;
            f32 litProg = getLitProgress();
            i32 visH = static_cast<i32>(std::ceil(static_cast<f64>(litProg) * 13.0)) + 1;
            visH = std::clamp(visH, 1, 14);
            m_gui->fillRect(static_cast<f32>(m_leftPos) + FIRE_X,
                static_cast<f32>(m_topPos) + FIRE_Y + FIRE_H - static_cast<f32>(visH),
                FIRE_W,
                static_cast<f32>(visH),
                0xFFFFAA00);
        }

        // 后备：纯色绘制进度箭头区域
        if (getBurnProgress() > 0.0f) {
            constexpr f32 ARROW_X = 79.0f;
            constexpr f32 ARROW_Y = 34.0f;
            constexpr f32 ARROW_H = 16.0f;
            f32 burnProg = getBurnProgress();
            i32 visW = static_cast<i32>(std::ceil(static_cast<f64>(burnProg) * 24.0));
            visW = std::clamp(visW, 0, 24);
            if (visW > 0) {
                m_gui->fillRect(static_cast<f32>(m_leftPos) + ARROW_X,
                    static_cast<f32>(m_topPos) + ARROW_Y,
                    static_cast<f32>(visW),
                    ARROW_H,
                    0xFFC6C6C6);
            }
        }
    }
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

f32 FurnaceScreen::getLitProgress() const
{
    // 从容器获取熔炉实体的燃烧数据
    auto* furnaceEntity = m_menu->getFurnaceEntity();
    if (furnaceEntity == nullptr) {
        return 0.0f;
    }

    const i32 burnTime = furnaceEntity->getBurnTime();
    const i32 burnTimeTotal = furnaceEntity->getBurnTimeTotal();
    if (burnTimeTotal <= 0) {
        return 0.0f;
    }

    return std::clamp(static_cast<f32>(burnTime) / static_cast<f32>(burnTimeTotal), 0.0f, 1.0f);
}

f32 FurnaceScreen::getBurnProgress() const
{
    // 从容器获取熔炉实体的熔炼进度数据
    auto* furnaceEntity = m_menu->getFurnaceEntity();
    if (furnaceEntity == nullptr) {
        return 0.0f;
    }

    const i32 cookTime = furnaceEntity->getCookTime();
    const i32 cookTimeTotal = furnaceEntity->getCookTimeTotal();
    if (cookTimeTotal <= 0 || cookTime <= 0) {
        return 0.0f;
    }

    return std::clamp(static_cast<f32>(cookTime) / static_cast<f32>(cookTimeTotal), 0.0f, 1.0f);
}

} // namespace mc::client
