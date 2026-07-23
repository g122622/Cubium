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

#include "CreativeScreen.hpp"

#include "client/ui/Glyph.hpp"
#include "client/ui/kagero/widget/ItemTooltipBuilder.hpp"
#include "client/ui/kagero/widget/TooltipRenderer.hpp"
#include "client/ui/screen/ScreenManager.hpp"
#include "common/entity/inventory/CreativeInventory.hpp"
#include "common/entity/inventory/container/ItemPickerMenu.hpp"
#include "common/util/StringUtils.hpp"
#include <algorithm>
#include <string>
#include <utility>
#include <vector>
#include <GLFW/glfw3.h>

namespace mc::client::ui::minecraft {

CreativeScreen::CreativeScreen(
    std::unique_ptr<mc::ItemPickerMenu> menu, ContainerClickSender clickSender, ContainerCloseSender closeSender)
    : ContainerScreenBase<mc::ItemPickerMenu>("creative", std::move(menu))
{
    setModal(true);
    setPauseScreen(false);

    // 交互引擎：背包槽命中回调绑定到基类 slotAt
    m_interaction = std::make_unique<kagero::widget::ContainerInteraction<mc::ItemPickerMenu>>(
        m_menu.get(), std::move(clickSender), std::move(closeSender), [this](i32 mouseX, i32 mouseY) {
            return this->slotAt(mouseX, mouseY);
        });

    // 41 个玩家背包槽位（右面板）
    buildSlots(mc::ItemPickerMenu::TOTAL_SLOT_COUNT);

    // 左面板调色板网格（9×5 可见行）
    auto paletteGrid = std::make_unique<kagero::widget::CreativePaletteGridWidget>(
        "creative_palette", PALETTE_X, PALETTE_Y, PALETTE_VISIBLE_ROWS);
    paletteGrid->setPaletteClickCallback(
        [this](i32 /*paletteEntryIndex*/, i32 visibleIndex, i32 button, bool /*shiftHeld*/) {
            if (m_interaction != nullptr) {
                m_interaction->sendClone(mc::ItemPickerMenu::PALETTE_VIRTUAL_BASE + visibleIndex, button);
                syncSlots();
            }
        });
    m_paletteGrid = paletteGrid.get();
    addChild(std::move(paletteGrid));

    // 搜索框
    auto searchField = std::make_unique<kagero::widget::TextFieldWidget>(
        "creative_search", SEARCH_X, SEARCH_Y, SEARCH_WIDTH, SEARCH_HEIGHT);
    searchField->setPlaceholder("Search creative items...");
    searchField->setTextChangedCallback([this](const std::string& /*text*/) {
        _rebuildVisibleEntries();
        if (m_paletteGrid != nullptr) {
            m_paletteGrid->scrollToTop();
            m_paletteGrid->refresh();
        }
    });
    m_searchField = searchField.get();
    addChild(std::move(searchField));
}

void CreativeScreen::onOpen()
{
    _buildPalette();
    ContainerScreenBase::onOpen();
    // 搜索框初始聚焦（用容器方法 setFocusedWidget，否则 onKey/onChar 路由不到）
    if (m_searchField != nullptr) {
        setFocusedWidget(m_searchField);
    }
}

void CreativeScreen::updateHover(i32 mouseX, i32 mouseY)
{
    ContainerScreenBase::updateHover(mouseX, mouseY);
    if (m_interaction != nullptr) {
        m_interaction->updateHoveredSlot(mouseX, mouseY);
    }
}

// ============================================================================
// ContainerScreenBase 钩子
// ============================================================================

std::pair<i32, i32> CreativeScreen::slotLocalPos(i32 slotIndex) const
{
    using M = mc::ItemPickerMenu;
    // 右面板偏移：玩家背包槽位叠加在 (INVENTORY_X, INVENTORY_Y) 之上
    if (slotIndex >= M::ARMOR_SLOT_START && slotIndex < M::ARMOR_SLOT_START + M::ARMOR_SLOT_COUNT) {
        const i32 i = slotIndex - M::ARMOR_SLOT_START;
        constexpr i32 ARMOR_Y[] = {ARMOR_Y_HEAD, ARMOR_Y_CHEST, ARMOR_Y_LEGS, ARMOR_Y_FEET};
        return {INVENTORY_X + ARMOR_X, INVENTORY_Y + ARMOR_Y[i]};
    }
    if (slotIndex >= M::PLAYER_INV_START && slotIndex <= M::PLAYER_INV_END) {
        const i32 i = slotIndex - M::PLAYER_INV_START;
        return {
            INVENTORY_X + PLAYER_INV_X + (i % 9) * SLOT_SPACING, INVENTORY_Y + PLAYER_INV_Y + (i / 9) * SLOT_SPACING};
    }
    if (slotIndex >= M::HOTBAR_START && slotIndex <= M::HOTBAR_END) {
        const i32 i = slotIndex - M::HOTBAR_START;
        return {INVENTORY_X + HOTBAR_X + i * SLOT_SPACING, INVENTORY_Y + HOTBAR_Y};
    }
    if (slotIndex == M::OFFHAND_SLOT) {
        return {INVENTORY_X + OFFHAND_X, INVENTORY_Y + OFFHAND_Y};
    }
    return {0, 0};
}

const mc::ItemStack& CreativeScreen::getCarriedItem() const
{
    return (m_interaction != nullptr) ? m_interaction->getCarriedItem() : mc::ItemStack::EMPTY;
}

void CreativeScreen::renderContainerBackground(kagero::widget::PaintContext& ctx)
{
    // 左面板（调色板区）
    const kagero::Rect palettePanel(m_leftPos + 4, m_topPos + 18, 172, 170);
    ctx.drawFilledRect(palettePanel, Colors::fromARGB(204, 35, 38, 44));
    ctx.drawBorder(palettePanel, 1.0f, Colors::fromARGB(255, 77, 163, 255));

    // 右面板（玩家背包区）
    const kagero::Rect inventoryPanel(m_leftPos + INVENTORY_X, m_topPos + INVENTORY_Y, 176, 176);
    ctx.drawFilledRect(inventoryPanel, Colors::fromARGB(204, 42, 47, 55));
    ctx.drawBorder(inventoryPanel, 1.0f, Colors::fromARGB(255, 255, 184, 77));

    // 标题与分区文字
    ctx.drawText("Creative Inventory", m_leftPos + TITLE_X, m_topPos + TITLE_Y, Colors::fromARGB(255, 245, 247, 250));
    ctx.drawText("Search", m_leftPos + SEARCH_X, m_topPos + SEARCH_Y - 10, Colors::fromARGB(255, 185, 193, 204));
    ctx.drawText(
        "Inventory", m_leftPos + INVENTORY_X + 8, m_topPos + INVENTORY_Y - 10, Colors::fromARGB(255, 216, 207, 163));
}

void CreativeScreen::renderExtraWidgets(kagero::widget::PaintContext& ctx)
{
    // 调色板网格与搜索框使用绝对屏幕坐标（与槽位/命中检测一致）
    if (m_paletteGrid != nullptr) {
        m_paletteGrid->setBounds(kagero::Rect(m_leftPos + PALETTE_X,
            m_topPos + PALETTE_Y,
            m_paletteGrid->bounds().width,
            m_paletteGrid->bounds().height));
        m_paletteGrid->paint(ctx);
    }
    if (m_searchField != nullptr) {
        m_searchField->setBounds(kagero::Rect(m_leftPos + SEARCH_X, m_topPos + SEARCH_Y, SEARCH_WIDTH, SEARCH_HEIGHT));
        m_searchField->paint(ctx);
    }
}

void CreativeScreen::renderContainerForeground(kagero::widget::PaintContext& ctx)
{
    // 垃圾桶标记（左面板右上角）
    const i32 trashX = m_leftPos + TRASH_X;
    const i32 trashY = m_topPos + TRASH_Y;
    const kagero::Rect trashRect(trashX, trashY, SLOT_SIZE, SLOT_SIZE);
    ctx.drawFilledRect(trashRect, Colors::fromARGB(255, 39, 23, 23));
    ctx.drawBorder(trashRect, 1.0f, Colors::fromARGB(255, 184, 77, 77));
    ctx.drawText("X", trashX + 5, trashY + 3, Colors::fromARGB(255, 242, 214, 214));
}

void CreativeScreen::renderTooltip(kagero::widget::PaintContext& ctx)
{
    if (m_screenWidth <= 0 || m_screenHeight <= 0) {
        return;
    }

    // 客户端 Player::world() 返回 nullptr（ClientWorld 不继承 IWorld，见
    // ui/screen/README.md 的 tooltip 坑），故 tooltip 直接传 nullptr。
    mc::IWorld* world = nullptr;

    // 1. 调色板悬停物品
    if (m_paletteGrid != nullptr) {
        const i32 visibleIndex = m_paletteGrid->indexAt(m_mouseX, m_mouseY);
        if (visibleIndex >= 0 && visibleIndex < static_cast<i32>(m_visibleEntries.size())) {
            const i32 entryIndex = m_visibleEntries[static_cast<std::size_t>(visibleIndex)];
            if (entryIndex >= 0 && entryIndex < static_cast<i32>(m_paletteEntries.size())) {
                const mc::ItemStack& stack = m_paletteEntries[static_cast<std::size_t>(entryIndex)].stack;
                if (!stack.isEmpty()) {
                    auto tooltip = kagero::widget::ItemTooltipBuilder::build(stack, world);
                    kagero::widget::TooltipRenderer::render(ctx,
                        tooltip,
                        static_cast<f32>(m_mouseX),
                        static_cast<f32>(m_mouseY),
                        static_cast<f32>(m_screenWidth),
                        static_cast<f32>(m_screenHeight));
                    return;
                }
            }
        }
    }

    // 2. 玩家背包槽位悬停物品
    mc::Slot* slot = slotAt(m_mouseX, m_mouseY);
    if (slot != nullptr && !slot->getItem().isEmpty()) {
        auto tooltip = kagero::widget::ItemTooltipBuilder::build(slot->getItem(), world);
        kagero::widget::TooltipRenderer::render(ctx,
            tooltip,
            static_cast<f32>(m_mouseX),
            static_cast<f32>(m_mouseY),
            static_cast<f32>(m_screenWidth),
            static_cast<f32>(m_screenHeight));
    }
}

// ============================================================================
// 事件
// ============================================================================

bool CreativeScreen::onClick(i32 mouseX, i32 mouseY, i32 button, i32 mods)
{
    // 1. 调色板网格命中 → 创造取物 clone
    if (m_paletteGrid != nullptr && m_paletteGrid->indexAt(mouseX, mouseY) >= 0) {
        const bool handled = m_paletteGrid->onClick(mouseX, mouseY, button, mods);
        return handled;
    }

    // 2. 垃圾桶命中 → 丢弃 carried（发 Throw 到 -999）
    if (_isMouseOverTrash(mouseX, mouseY) && m_interaction != nullptr) {
        m_interaction->discardCarried();
        syncSlots();
        return true;
    }

    // 3. 玩家背包槽位 → interaction
    if (m_interaction != nullptr && slotAt(mouseX, mouseY) != nullptr) {
        const bool handled = m_interaction->onClick(mouseX, mouseY, button, mods);
        syncSlots();
        return handled;
    }

    // 4. 搜索框 / 其余区域 → ContainerWidget 分发（路由到 TextFieldWidget）
    return Screen::onClick(mouseX, mouseY, button, mods);
}

bool CreativeScreen::onRelease(i32 mouseX, i32 mouseY, i32 button, i32 mods)
{
    if (m_interaction == nullptr) {
        return false;
    }
    const bool handled = m_interaction->onRelease(mouseX, mouseY, button, mods);
    syncSlots();
    return handled;
}

bool CreativeScreen::onDrag(i32 mouseX, i32 mouseY, i32 deltaX, i32 deltaY, i32 button)
{
    if (m_interaction == nullptr) {
        return false;
    }
    const bool handled = m_interaction->onDrag(mouseX, mouseY, deltaX, deltaY, button);
    syncSlots();
    return handled;
}

bool CreativeScreen::onScroll(i32 mouseX, i32 mouseY, f64 delta)
{
    // 调色板区滚动（含格子间空隙，鼠标在网格 bounds 内即滚动）
    if (m_paletteGrid != nullptr) {
        const kagero::Rect b = m_paletteGrid->bounds();
        const i32 gx = m_leftPos + b.x;
        const i32 gy = m_topPos + b.y;
        if (mouseX >= gx && mouseX < gx + b.width && mouseY >= gy && mouseY < gy + b.height) {
            return m_paletteGrid->onScroll(mouseX, mouseY, delta);
        }
    }
    return Screen::onScroll(mouseX, mouseY, delta);
}

bool CreativeScreen::onKey(i32 key, i32 scanCode, i32 action, i32 mods)
{
    if (m_interaction != nullptr && (action == GLFW_PRESS || action == GLFW_REPEAT)) {
        if (key == GLFW_KEY_ESCAPE || key == GLFW_KEY_E) {
            m_interaction->close();
            ScreenManager::instance().closeScreen();
            return true;
        }
    }

    // 搜索框聚焦时，键盘事件转发给它
    if (m_searchField != nullptr && m_searchField->isFocused()) {
        return m_searchField->onKey(key, scanCode, action, mods);
    }

    // 否则走 interaction（Q 丢弃、数字键交换、F 副手交换）
    if (m_interaction != nullptr) {
        const bool handled = m_interaction->onKey(key, scanCode, action, mods);
        syncSlots();
        return handled;
    }

    return Screen::onKey(key, scanCode, action, mods);
}

bool CreativeScreen::onChar(u32 codePoint)
{
    if (m_searchField != nullptr && m_searchField->isFocused()) {
        return m_searchField->onChar(codePoint);
    }
    return Screen::onChar(codePoint);
}

// ============================================================================
// 内部逻辑
// ============================================================================

void CreativeScreen::_buildPalette()
{
    m_paletteEntries = buildCreativePaletteEntries();
    _rebuildVisibleEntries();
    if (m_paletteGrid != nullptr) {
        m_paletteGrid->setData(&m_paletteEntries, &m_visibleEntries);
        m_paletteGrid->refresh();
        _injectPalettePaintCallback();
    }
}

void CreativeScreen::_rebuildVisibleEntries()
{
    m_visibleEntries.clear();
    const std::string filter = _normalizeSearchText(m_searchField != nullptr ? m_searchField->text() : std::string());
    for (i32 index = 0; index < static_cast<i32>(m_paletteEntries.size()); ++index) {
        const auto& entry = m_paletteEntries[static_cast<std::size_t>(index)];
        if (filter.empty() || entry.searchKey.find(filter) != std::string::npos) {
            m_visibleEntries.push_back(index);
        }
    }
    if (m_paletteGrid != nullptr) {
        m_paletteGrid->setData(&m_paletteEntries, &m_visibleEntries);
        m_paletteGrid->refresh();
    }
}

void CreativeScreen::_injectPalettePaintCallback()
{
    if (m_paletteGrid == nullptr || m_gui == nullptr || m_itemRenderer == nullptr) {
        return;
    }
    auto* guiPtr = m_gui;
    auto* itemRendererPtr = m_itemRenderer;
    m_paletteGrid->setItemPaintCallback([guiPtr, itemRendererPtr](const mc::ItemStack& item, i32 x, i32 y, i32 size) {
        itemRendererPtr->renderItem(*guiPtr, item, static_cast<f64>(x), static_cast<f64>(y), static_cast<f64>(size));
    });
}

bool CreativeScreen::_isMouseOverTrash(i32 mouseX, i32 mouseY) const
{
    const i32 x = m_leftPos + TRASH_X;
    const i32 y = m_topPos + TRASH_Y;
    return mouseX >= x && mouseX < x + SLOT_SIZE && mouseY >= y && mouseY < y + SLOT_SIZE;
}

std::string CreativeScreen::_normalizeSearchText(std::string_view text)
{
    return util::toLowerAscii(text);
}

} // namespace mc::client::ui::minecraft
