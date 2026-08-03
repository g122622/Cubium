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

#include "LoomScreen.hpp"

#include "client/ui/Glyph.hpp"
#include "client/ui/kagero/event/EventBus.hpp"
#include "client/ui/kagero/state/StateStore.hpp"
#include "client/ui/kagero/template/binder/BindingContext.hpp"
#include "client/ui/kagero/widget/ButtonWidget.hpp"
#include "client/ui/kagero/widget/ScrollableWidget.hpp"
#include "client/ui/kagero/widget/SlotWidget.hpp"
#include "client/ui/minecraft/screens/TemplateScreen.hpp"
#include "common/core/Types.hpp"
#include "common/entity/inventory/ContainerTypes.hpp"
#include "common/entity/inventory/PlayerInventory.hpp"
#include "common/entity/inventory/Slot.hpp"
#include "common/entity/inventory/container/LoomContainer.hpp"
#include "common/world/block/BlockPos.hpp"
#include <memory>
#include <string>
#include <utility>
#include <GLFW/glfw3.h>
#include <spdlog/spdlog.h>

namespace mc::client::ui::minecraft {

using kagero::widget::ButtonWidget;
using kagero::widget::ScrollableWidget;
using kagero::widget::SlotWidget;

LoomScreen::LoomScreen(mc::ContainerId containerId,
    mc::PlayerInventory* playerInventory,
    ContainerClickSender clickSender,
    ContainerCloseSender closeSender)
    : TemplateScreen(std::make_unique<kagero::tpl::binder::BindingContext>(
                         kagero::state::StateStore::instance(), kagero::event::EventBus::instance()),
          "loom")
    , m_menu(std::make_unique<mc::entity::inventory::container::LoomContainer>(
          containerId, playerInventory, BlockPos(0, 0, 0)))
    , m_playerInventory(playerInventory)
    , m_clickSender(std::move(clickSender))
    , m_closeSender(std::move(closeSender))
{
    loadTemplateFile("src/client/ui/minecraft/templates/loom.tpl");
    _registerCallbacks();
}

void LoomScreen::onOpen()
{
    TemplateScreen::onOpen();
    _initSlots();
    _initPatternButtons();
    _refreshSlots();
    _refreshPatterns();
}

void LoomScreen::onClose()
{
    TemplateScreen::onClose();

    if (m_closeSender && m_menu) {
        m_closeSender(m_menu->getId());
    }
}

void LoomScreen::tick(f32 dt)
{
    TemplateScreen::tick(dt);
    _refreshSlots();
    _refreshPatterns();
}

bool LoomScreen::onKey(i32 key, i32 scanCode, i32 action, i32 mods)
{
    (void)scanCode;
    (void)mods;

    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        _onCloseScreen();
        return true;
    }

    if (key == GLFW_KEY_E && action == GLFW_PRESS) {
        _onCloseScreen();
        return true;
    }

    return TemplateScreen::onKey(key, scanCode, action, mods);
}

void LoomScreen::_registerCallbacks()
{
    exposeSimpleCallback("onSlotClick", [this]() {
        // 模板中的slot click通过on:click绑定，但实际槽位点击
        // 通过_initSlots()中设置的SlotWidget回调处理。
    });

    exposeSimpleCallback("onPatternScroll", [this]() {
        // 滚动回调通过ScrollableWidget的setOnScroll设置
    });

    exposeSimpleCallback("onClose", [this]() { _onCloseScreen(); });
}

void LoomScreen::_initSlots()
{
    if (!m_menu) {
        return;
    }

    // 旗帜槽
    auto* bannerSlot = dynamic_cast<SlotWidget*>(findWidget("bannerSlot"));
    if (bannerSlot) {
        bannerSlot->setSlotIndex(mc::entity::inventory::container::LoomContainer::SLOT_BANNER);
        bannerSlot->setOnSlotClick(
            [this](i32 slotIndex, i32 button, bool shiftHeld) { _onSlotClick(slotIndex, button, shiftHeld); });
    }

    // 染料槽
    auto* dyeSlot = dynamic_cast<SlotWidget*>(findWidget("dyeSlot"));
    if (dyeSlot) {
        dyeSlot->setSlotIndex(mc::entity::inventory::container::LoomContainer::SLOT_DYE);
        dyeSlot->setOnSlotClick(
            [this](i32 slotIndex, i32 button, bool shiftHeld) { _onSlotClick(slotIndex, button, shiftHeld); });
    }

    // 图案物品槽
    auto* patternSlot = dynamic_cast<SlotWidget*>(findWidget("patternSlot"));
    if (patternSlot) {
        patternSlot->setSlotIndex(mc::entity::inventory::container::LoomContainer::SLOT_PATTERN);
        patternSlot->setOnSlotClick(
            [this](i32 slotIndex, i32 button, bool shiftHeld) { _onSlotClick(slotIndex, button, shiftHeld); });
    }

    // 输出槽
    auto* resultSlot = dynamic_cast<SlotWidget*>(findWidget("resultSlot"));
    if (resultSlot) {
        resultSlot->setSlotIndex(mc::entity::inventory::container::LoomContainer::SLOT_RESULT);
        resultSlot->setOnSlotClick(
            [this](i32 slotIndex, i32 button, bool shiftHeld) { _onSlotClick(slotIndex, button, shiftHeld); });
    }

    // 玩家背包槽位（3行x9列）
    for (i32 row = 0; row < 3; ++row) {
        for (i32 col = 0; col < 9; ++col) {
            i32 slotIndex = mc::entity::inventory::container::LoomContainer::LOOM_SLOTS + row * 9 + col;
            std::string slotId = "invSlot_" + std::to_string(row) + "_" + std::to_string(col);
            auto* slot = dynamic_cast<SlotWidget*>(findWidget(slotId));
            if (slot) {
                slot->setSlotIndex(slotIndex);
                slot->setOnSlotClick([this](i32 si, i32 btn, bool shift) { _onSlotClick(si, btn, shift); });
            }
        }
    }

    // 快捷栏槽位（1行x9列）
    for (i32 col = 0; col < 9; ++col) {
        i32 slotIndex = mc::entity::inventory::container::LoomContainer::LOOM_SLOTS + 27 + col;
        std::string slotId = "hotbarSlot_" + std::to_string(col);
        auto* slot = dynamic_cast<SlotWidget*>(findWidget(slotId));
        if (slot) {
            slot->setSlotIndex(slotIndex);
            slot->setOnSlotClick([this](i32 si, i32 btn, bool shift) { _onSlotClick(si, btn, shift); });
        }
    }
}

void LoomScreen::_initPatternButtons()
{
    auto* patternList = dynamic_cast<ScrollableWidget*>(findWidget("patternList"));
    if (!patternList) {
        spdlog::warn("[LoomScreen] patternList widget not found in template");
        return;
    }

    // 35个基础图案（不需要图案物品的），4列布局 = 9行
    const i32 totalPatterns = mc::entity::inventory::container::LoomContainer::PATTERN_ITEM_INDEX;
    const i32 rows = (totalPatterns + PATTERN_GRID_COLS - 1) / PATTERN_GRID_COLS;
    patternList->setContentHeight(rows * PATTERN_BUTTON_SPACING);

    // 创建图案选择按钮
    m_patternButtonIds.clear();
    for (i32 i = 0; i < totalPatterns; ++i) {
        i32 col = i % PATTERN_GRID_COLS;
        i32 row = i / PATTERN_GRID_COLS;
        i32 x = col * PATTERN_BUTTON_SPACING;
        i32 y = row * PATTERN_BUTTON_SPACING;

        std::string buttonId = "patternBtn_" + std::to_string(i);
        m_patternButtonIds.push_back(buttonId);

        auto button = std::make_unique<ButtonWidget>(buttonId, x, y, PATTERN_BUTTON_SIZE, PATTERN_BUTTON_SIZE, "");

        // 图案索引（1-based，与clickMenuButton一致）
        i32 patternIndex = i + 1;
        button->setOnPress([this, patternIndex](ButtonWidget&) { _onPatternSelect(patternIndex); });

        patternList->addWidget(std::move(button));
    }

    // 设置滚动回调
    patternList->setOnScroll([this](i32 x, i32 y, f64 deltaX, f64 deltaY) { _onPatternScroll(x, y, deltaX, deltaY); });
}

void LoomScreen::_refreshSlots()
{
    if (!m_menu) {
        return;
    }

    // 旗帜槽
    auto* bannerSlot = dynamic_cast<SlotWidget*>(findWidget("bannerSlot"));
    if (bannerSlot) {
        const mc::Slot* slot = m_menu->getSlot(mc::entity::inventory::container::LoomContainer::SLOT_BANNER);
        if (slot) {
            bannerSlot->setItem(slot->getItem());
        }
    }

    // 染料槽
    auto* dyeSlot = dynamic_cast<SlotWidget*>(findWidget("dyeSlot"));
    if (dyeSlot) {
        const mc::Slot* slot = m_menu->getSlot(mc::entity::inventory::container::LoomContainer::SLOT_DYE);
        if (slot) {
            dyeSlot->setItem(slot->getItem());
        }
    }

    // 图案物品槽
    auto* patternSlot = dynamic_cast<SlotWidget*>(findWidget("patternSlot"));
    if (patternSlot) {
        const mc::Slot* slot = m_menu->getSlot(mc::entity::inventory::container::LoomContainer::SLOT_PATTERN);
        if (slot) {
            patternSlot->setItem(slot->getItem());
        }
    }

    // 输出槽
    auto* resultSlot = dynamic_cast<SlotWidget*>(findWidget("resultSlot"));
    if (resultSlot) {
        const mc::Slot* slot = m_menu->getSlot(mc::entity::inventory::container::LoomContainer::SLOT_RESULT);
        if (slot) {
            resultSlot->setItem(slot->getItem());
        }
    }

    // 玩家背包槽位
    for (i32 row = 0; row < 3; ++row) {
        for (i32 col = 0; col < 9; ++col) {
            i32 slotIndex = mc::entity::inventory::container::LoomContainer::LOOM_SLOTS + row * 9 + col;
            std::string slotId = "invSlot_" + std::to_string(row) + "_" + std::to_string(col);
            auto* widget = dynamic_cast<SlotWidget*>(findWidget(slotId));
            if (widget) {
                const mc::Slot* slot = m_menu->getSlot(slotIndex);
                if (slot) {
                    widget->setItem(slot->getItem());
                }
            }
        }
    }

    // 快捷栏槽位
    for (i32 col = 0; col < 9; ++col) {
        i32 slotIndex = mc::entity::inventory::container::LoomContainer::LOOM_SLOTS + 27 + col;
        std::string slotId = "hotbarSlot_" + std::to_string(col);
        auto* widget = dynamic_cast<SlotWidget*>(findWidget(slotId));
        if (widget) {
            const mc::Slot* slot = m_menu->getSlot(slotIndex);
            if (slot) {
                widget->setItem(slot->getItem());
            }
        }
    }
}

void LoomScreen::_refreshPatterns()
{
    if (!m_menu) {
        return;
    }

    i32 selectedPattern = m_menu->getSelectedPattern();

    // 更新图案按钮的选中状态
    for (i32 i = 0; i < static_cast<i32>(m_patternButtonIds.size()); ++i) {
        const std::string& buttonId = m_patternButtonIds[i];
        auto* button = dynamic_cast<ButtonWidget*>(findWidget(buttonId));
        if (button) {
            bool isSelected = (i + 1) == selectedPattern;
            // 选中状态通过视觉反馈区分
            if (isSelected) {
                button->setStyle(ButtonWidget::Style{
                    Colors::fromARGB(255, 100, 100, 180), // 选中背景色
                    Colors::fromARGB(255, 120, 120, 200), // 选中悬停色
                    Colors::fromARGB(255, 40, 40, 40),    // 禁用色
                    Colors::WHITE,                        // 文本色
                    Colors::fromARGB(255, 128, 128, 128), // 禁用文本色
                    Colors::fromARGB(255, 180, 180, 255), // 选中边框色
                    Colors::fromARGB(255, 180, 180, 255), // 选中悬停边框色
                    2,                                    // 圆角
                    true                                  // 绘制边框
                });
            } else {
                button->setStyle(ButtonWidget::Style{
                    Colors::fromARGB(255, 60, 60, 60),    // 正常背景色
                    Colors::fromARGB(255, 80, 80, 80),    // 悬停色
                    Colors::fromARGB(255, 40, 40, 40),    // 禁用色
                    Colors::WHITE,                        // 文本色
                    Colors::fromARGB(255, 128, 128, 128), // 禁用文本色
                    Colors::fromARGB(255, 100, 100, 100), // 边框色
                    Colors::fromARGB(255, 150, 150, 150), // 悬停边框色
                    3,                                    // 圆角
                    true                                  // 绘制边框
                });
            }
        }
    }
}

void LoomScreen::_onSlotClick(i32 slotIndex, i32 button, bool shiftHeld)
{
    if (!m_menu || !m_clickSender) {
        return;
    }

    const mc::ClickAction action = shiftHeld ? mc::ClickAction::QuickMove : mc::ClickAction::Pickup;
    const i16 transactionId = m_menu->incrementTransactionId();
    m_clickSender(m_menu->getId(), slotIndex, button, transactionId, action, m_menu->getCarriedItem());
}

void LoomScreen::_onPatternSelect(i32 patternIndex)
{
    if (!m_menu || !m_playerInventory || !m_playerInventory->getPlayer()) {
        return;
    }

    m_menu->clickMenuButton(*m_playerInventory->getPlayer(), patternIndex);
    _refreshPatterns();
}

void LoomScreen::_onPatternScroll(i32 x, i32 y, f64 deltaX, f64 deltaY)
{
    (void)x;
    (void)y;
    (void)deltaX;
    (void)deltaY;
    // ScrollableWidget自行处理滚动偏移
}

void LoomScreen::_onCloseScreen()
{
    if (m_closeSender && m_menu) {
        m_closeSender(m_menu->getId());
    }
}

} // namespace mc::client::ui::minecraft
