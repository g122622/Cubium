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

#pragma once

#include "common/entity/inventory/AbstractContainerMenu.hpp"
#include "common/entity/inventory/ContainerTypes.hpp"
#include "common/entity/inventory/PlayerInventory.hpp"
#include "common/entity/inventory/Slot.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/network/packet/InventoryPackets.hpp"
#include "core/Types.hpp"
#include <chrono>
#include <functional>
#include <vector>
#include <GLFW/glfw3.h>

namespace mc::client::ui::kagero::widget {

/**
 * @brief 容器交互引擎（无 IScreen 依赖）
 *
 * 容器点击/拖拽的纯交互逻辑，不继承 IScreen、不依赖 GuiRenderer，
 * 仅供 kagero 体系的容器屏幕复用。行为字节级对齐 MC
 * 容器点击/拖拽网络协议（ClickAction / DragConstants 编码）。
 *
 * 依赖：
 * - Menu*（AbstractContainerMenu 子类，提供槽位数据与 clicked/getCarriedItem）
 * - ContainerClickSender / ContainerCloseSender（网络或本地模式）
 * - 槽位命中回调 SlotHitTest（由宿主屏幕基于自身槽位布局提供）
 *
 * 宿主屏幕职责：
 * 1. 持有 Menu 与本引擎，把 onClick/onDrag/onRelease/onKey 事件转发进来；
 * 2. 提供 SlotHitTest（基于 SlotWidget bounds 命中检测）；
 * 3. 每帧调用 updateHoveredSlot 更新悬停索引（用于 Q 键丢弃、数字键交换）；
 * 4. 交互完成后由宿主自行 syncSlots 刷新槽位渲染。
 *
 * @tparam Menu 菜单类型（AbstractContainerMenu 子类）
 */
template <typename Menu>
class ContainerInteraction {
public:
    using ContainerClickSender = std::function<void(ContainerId, i32, i32, i16, ClickAction, const mc::ItemStack&)>;
    using ContainerCloseSender = std::function<void(ContainerId)>;

    /**
     * @brief 槽位命中检测回调
     *
     * 给定屏幕坐标，返回命中的槽位指针（未命中返回 nullptr）。
     * 宿主屏幕基于自身槽位布局实现。
     */
    using SlotHitTest = std::function<mc::Slot*(i32 mouseX, i32 mouseY)>;

    ContainerInteraction(
        Menu* menu, ContainerClickSender clickSender, ContainerCloseSender closeSender, SlotHitTest slotHitTest)
        : m_menu(menu)
        , m_clickSender(std::move(clickSender))
        , m_closeSender(std::move(closeSender))
        , m_slotHitTest(std::move(slotHitTest))
    {}

    // ==================== 事件入口（转发自宿主屏幕） ====================

    /**
     * @brief 处理鼠标点击
     *
     * Shift+左键 QuickMove、双击 PickupAll、中键 Clone、左/右键 Pickup、
     * 光标有物品时启动拖拽分发。
     */
    bool onClick(i32 mouseX, i32 mouseY, i32 button, i32 mods)
    {
        if (m_menu == nullptr) {
            return false;
        }

        const bool shiftHeld = (mods & GLFW_MOD_SHIFT) != 0;
        const bool ctrlHeld = (mods & GLFW_MOD_CONTROL) != 0;

        mc::Slot* slot = getSlotAt(mouseX, mouseY);
        const bool clickedOutside = (slot == nullptr);

        m_hoveredSlotIndex = clickedOutside ? -1 : slot->getIndex();

        const bool carriedHasItems = !getCarriedItem().isEmpty();
        if (carriedHasItems && !m_isQuickCrafting && !clickedOutside && button <= 1) {
            m_isQuickCrafting = true;
            m_quickCraftingButton = button;
            m_quickCraftingType = _getQuickCraftType(button, ctrlHeld);
            m_quickCraftSlots.clear();
            m_skipNextRelease = false;
            return true;
        }

        if (clickedOutside) {
            return _handleClickOutside(button, shiftHeld);
        }

        // 双击检测（500ms 内同一槽位）
        const auto now = std::chrono::steady_clock::now();
        const bool isDoubleClick =
            (m_lastClickSlot == slot && m_lastClickTime > std::chrono::steady_clock::time_point{} &&
                std::chrono::duration_cast<std::chrono::nanoseconds>(now - m_lastClickTime).count() <
                    DOUBLE_CLICK_THRESHOLD_NS);
        m_lastClickSlot = slot;
        m_lastClickTime = now;

        if (isDoubleClick && button == 0 && !shiftHeld) {
            _sendSlotClick(*slot, slot->getIndex(), 0, ClickAction::PickupAll);
            m_skipNextRelease = true;
            return true;
        }

        if (shiftHeld && button == 0) {
            _sendSlotClick(*slot, slot->getIndex(), 0, ClickAction::QuickMove);
            m_skipNextRelease = true;
            return true;
        }

        if (button == 2) {
            _sendSlotClick(*slot, slot->getIndex(), 2, ClickAction::Clone);
            m_skipNextRelease = true;
            return true;
        }

        if (button == 0 || button == 1) {
            _sendSlotClick(*slot, slot->getIndex(), button, ClickAction::Pickup);
            m_skipNextRelease = true;
            return true;
        }

        return false;
    }

    /**
     * @brief 处理鼠标释放（完成拖拽分发）
     */
    bool onRelease(i32 mouseX, i32 mouseY, i32 button, i32 mods)
    {
        (void)mouseX;
        (void)mouseY;

        if (m_menu == nullptr) {
            return false;
        }

        if (m_skipNextRelease) {
            m_skipNextRelease = false;
            return true;
        }

        if (m_isQuickCrafting && button == m_quickCraftingButton) {
            const bool shiftHeld = (mods & GLFW_MOD_SHIFT) != 0;
            _finishQuickCraft(shiftHeld);
            return true;
        }

        return false;
    }

    /**
     * @brief 处理鼠标拖动（收集拖拽目标槽位）
     */
    bool onDrag(i32 mouseX, i32 mouseY, i32 deltaX, i32 deltaY, i32 button)
    {
        (void)deltaX;
        (void)deltaY;

        if (m_menu == nullptr || !m_isQuickCrafting || button != m_quickCraftingButton) {
            return false;
        }

        mc::Slot* slot = getSlotAt(mouseX, mouseY);
        if (slot == nullptr) {
            return true;
        }

        m_hoveredSlotIndex = slot->getIndex();

        const mc::ItemStack& carried = getCarriedItem();
        if (carried.isEmpty()) {
            return true;
        }

        const i32 slotIndex = slot->getIndex();
        for (i32 idx : m_quickCraftSlots) {
            if (idx == slotIndex) {
                return true;
            }
        }

        if (!slot->mayPlace(carried)) {
            return true;
        }

        if (m_quickCraftingType != DragConstants::MODE_FILL) {
            if (carried.getCount() <= static_cast<i32>(m_quickCraftSlots.size())) {
                return true;
            }
        }

        if (!slot->getItem().isEmpty() && !carried.isSameItem(slot->getItem())) {
            return true;
        }

        m_quickCraftSlots.push_back(slotIndex);
        return true;
    }

    /**
     * @brief 处理键盘按键
     *
     * ESC/E 关闭屏幕、Q 丢弃、1-9 快捷栏交换、F 副手交换。
     * @return true 表示事件已处理（宿主屏幕据此决定是否 pop）
     */
    bool onKey(i32 key, i32 scanCode, i32 action, i32 mods)
    {
        (void)scanCode;

        if (m_menu == nullptr) {
            return false;
        }

        if (action == GLFW_PRESS || action == GLFW_REPEAT) {
            if (key == GLFW_KEY_ESCAPE || key == GLFW_KEY_E) {
                close();
                return true;
            }

            if (key == GLFW_KEY_Q) {
                return _handleDropKey(mods);
            }

            if (key >= GLFW_KEY_1 && key <= GLFW_KEY_9) {
                const i32 hotbarIndex = key - GLFW_KEY_1;
                return _handleHotbarSwap(hotbarIndex);
            }

            if (key == GLFW_KEY_F) {
                return _handleHotbarSwap(40); // 40 = 副手槽位索引
            }
        }

        return false;
    }

    // ==================== 状态查询 ====================

    [[nodiscard]] mc::ItemStack& getCarriedItem() { return m_menu ? m_menu->getCarriedItem() : s_emptyStack; }
    [[nodiscard]] const mc::ItemStack& getCarriedItem() const
    {
        return m_menu ? m_menu->getCarriedItem() : s_emptyStack;
    }

    [[nodiscard]] i32 getHoveredSlotIndex() const { return m_hoveredSlotIndex; }

    /**
     * @brief 更新悬停槽位索引（宿主每帧调用）
     */
    void updateHoveredSlot(i32 mouseX, i32 mouseY)
    {
        mc::Slot* slot = getSlotAt(mouseX, mouseY);
        m_hoveredSlotIndex = (slot != nullptr) ? slot->getIndex() : -1;
    }

    /**
     * @brief 关闭容器（发 CloseContainerPacket）
     *
     * 玩家背包（containerId=0）同样发送：服务端已为其建立 InventoryCraftingMenu，
     * 需要 CloseContainerPacket 触发菜单清理与背包同步。
     */
    void close()
    {
        if (m_closeSender && m_menu != nullptr) {
            m_closeSender(m_menu->getId());
        }
    }

    /**
     * @brief 发送创造调色板取物点击（Clone 语义）
     *
     * 创造屏调色板虚拟槽无 Slot 对象，故不走 onClick 的 slotAt 命中路径，由宿主
     * 直接调用本方法发送 `ClickAction::Clone`。slotIndex 通常为
     * `ItemPickerMenu::PALETTE_VIRTUAL_BASE + visibleIndex`。服务端 ItemPickerMenu
     * 的 clicked override 拦截该虚拟索引做 clone，光标经 ContainerContentPacket
     * 的 carried 字段回传。
     *
     * @param slotIndex 虚拟槽索引（>= PALETTE_VIRTUAL_BASE）
     * @param button 0=左键整组 / 1=右键单个 / 2=中键
     */
    void sendClone(i32 slotIndex, i32 button)
    {
        if (m_menu == nullptr) {
            return;
        }
        const i16 transactionId = m_menu->incrementTransactionId();
        if (m_clickSender) {
            m_clickSender(
                m_menu->getId(), slotIndex, button, transactionId, ClickAction::Clone, m_menu->getCarriedItem());
            return;
        }
        // 无网络发送器（本地模式）：直接本地 clicked
        auto* playerInventory = m_menu->getPlayerInventory();
        if (playerInventory == nullptr || playerInventory->getPlayer() == nullptr) {
            return;
        }
        m_menu->clicked(slotIndex, button, ClickType::Clone, *playerInventory->getPlayer());
    }

    /**
     * @brief 丢弃光标携带物品（垃圾桶语义）
     *
     * 发送 `ClickAction::Throw` 到 -999（槽外），服务端把 carried 整组丢出。
     * 创造屏垃圾桶点击时调用。button==0 整组丢弃。
     */
    void discardCarried()
    {
        if (m_menu == nullptr || getCarriedItem().isEmpty()) {
            return;
        }
        _sendOutsideClick(0, ClickAction::Throw);
    }

private:
    static constexpr i64 DOUBLE_CLICK_THRESHOLD_NS = 500'000'000L;
    static constexpr i32 SLOT_CLICKED_OUTSIDE = -999;

    [[nodiscard]] mc::Slot* getSlotAt(i32 mouseX, i32 mouseY)
    {
        if (m_menu == nullptr || !m_slotHitTest) {
            return nullptr;
        }
        return m_slotHitTest(mouseX, mouseY);
    }

    void _sendSlotClick(mc::Slot& slot, i32 slotIndex, i32 button, ClickAction action)
    {
        if (m_clickSender) {
            const i16 transactionId = m_menu->incrementTransactionId();
            m_clickSender(m_menu->getId(), slotIndex, button, transactionId, action, m_menu->getCarriedItem());
            return;
        }

        auto* playerInventory = m_menu->getPlayerInventory();
        if (playerInventory == nullptr || playerInventory->getPlayer() == nullptr) {
            return;
        }

        ClickType clickType = _actionToClickType(action, button);
        m_menu->clicked(slotIndex, button, clickType, *playerInventory->getPlayer());
    }

    void _sendOutsideClick(i32 button, ClickAction action)
    {
        if (m_clickSender) {
            const i16 transactionId = m_menu->incrementTransactionId();
            m_clickSender(
                m_menu->getId(), SLOT_CLICKED_OUTSIDE, button, transactionId, action, m_menu->getCarriedItem());
            return;
        }

        auto* playerInventory = m_menu->getPlayerInventory();
        if (playerInventory == nullptr || playerInventory->getPlayer() == nullptr) {
            return;
        }

        ClickType clickType = _actionToClickType(action, button);
        m_menu->clicked(SLOT_CLICKED_OUTSIDE, button, clickType, *playerInventory->getPlayer());
    }

    bool _handleClickOutside(i32 button, bool shiftHeld)
    {
        if (m_menu == nullptr || getCarriedItem().isEmpty()) {
            return false;
        }

        if (button == 0) {
            _sendOutsideClick(0, ClickAction::Pickup);
            return true;
        }
        if (button == 1) {
            _sendOutsideClick(1, ClickAction::Pickup);
            return true;
        }

        (void)shiftHeld;
        return false;
    }

    bool _handleDropKey(i32 mods)
    {
        if (m_menu == nullptr) {
            return false;
        }

        if (!getCarriedItem().isEmpty()) {
            const bool ctrlHeld = (mods & GLFW_MOD_CONTROL) != 0;
            _sendOutsideClick(ctrlHeld ? 1 : 0, ClickAction::Throw);
            return true;
        }

        if (m_hoveredSlotIndex >= 0 && m_hoveredSlotIndex < m_menu->getSlotCount()) {
            const bool ctrlHeld = (mods & GLFW_MOD_CONTROL) != 0;
            mc::Slot* slot = m_menu->getSlot(m_hoveredSlotIndex);
            if (slot != nullptr && !slot->getItem().isEmpty()) {
                _sendSlotClick(*slot, m_hoveredSlotIndex, ctrlHeld ? 1 : 0, ClickAction::Throw);
                return true;
            }
        }

        return false;
    }

    bool _handleHotbarSwap(i32 hotbarIndex)
    {
        if (m_menu == nullptr || m_hoveredSlotIndex < 0) {
            return false;
        }

        mc::Slot* slot = m_menu->getSlot(m_hoveredSlotIndex);
        if (slot == nullptr) {
            return false;
        }

        _sendSlotClick(*slot, m_hoveredSlotIndex, hotbarIndex, ClickAction::Swap);
        return true;
    }

    void _finishQuickCraft(bool shiftHeld)
    {
        m_isQuickCrafting = false;

        if (m_quickCraftSlots.empty()) {
            m_quickCraftSlots.clear();
            return;
        }

        if (m_quickCraftSlots.size() == 1) {
            const i32 slotIndex = m_quickCraftSlots[0];
            mc::Slot* slot = m_menu->getSlot(slotIndex);
            if (slot != nullptr) {
                if (shiftHeld) {
                    _sendSlotClick(*slot, slotIndex, 0, ClickAction::QuickMove);
                } else {
                    _sendSlotClick(*slot, slotIndex, m_quickCraftingButton, ClickAction::Pickup);
                }
            }
            m_quickCraftSlots.clear();
            return;
        }

        const i32 startButton = (DragConstants::EVENT_START) | (m_quickCraftingType << DragConstants::MODE_SHIFT);
        const i32 addButton = (DragConstants::EVENT_ADD_SLOT) | (m_quickCraftingType << DragConstants::MODE_SHIFT);
        const i32 endButton = (DragConstants::EVENT_END) | (m_quickCraftingType << DragConstants::MODE_SHIFT);

        _sendOutsideClick(startButton, ClickAction::QuickCraft);
        for (i32 slotIndex : m_quickCraftSlots) {
            mc::Slot* slot = m_menu->getSlot(slotIndex);
            if (slot != nullptr) {
                _sendSlotClick(*slot, slotIndex, addButton, ClickAction::QuickCraft);
            }
        }
        _sendOutsideClick(endButton, ClickAction::QuickCraft);

        m_quickCraftSlots.clear();
    }

    static i32 _getQuickCraftType(i32 button, bool ctrlHeld)
    {
        (void)ctrlHeld;
        switch (button) {
            case 0:
                return DragConstants::MODE_EVEN;
            case 1:
                return DragConstants::MODE_SINGLE;
            case 2:
                return DragConstants::MODE_FILL;
            default:
                return DragConstants::MODE_EVEN;
        }
    }

    static ClickType _actionToClickType(ClickAction action, i32 button)
    {
        switch (action) {
            case ClickAction::Pickup:
                return (button == 0) ? ClickType::Pick : ClickType::PickSome;
            case ClickAction::QuickMove:
                return ClickType::QuickMove;
            case ClickAction::Swap:
                return ClickType::Swap;
            case ClickAction::Clone:
                return ClickType::Clone;
            case ClickAction::Throw:
                return (button == 0) ? ClickType::Throw : ClickType::ThrowAll;
            case ClickAction::QuickCraft:
                return ClickType::QuickCraft;
            case ClickAction::PickupAll:
                return ClickType::PickAll;
            default:
                return ClickType::Pick;
        }
    }

    Menu* m_menu;
    ContainerClickSender m_clickSender;
    ContainerCloseSender m_closeSender;
    SlotHitTest m_slotHitTest;

    bool m_isQuickCrafting = false;
    i32 m_quickCraftingButton = -1;
    i32 m_quickCraftingType = DragConstants::DRAG_MODE_NONE;
    std::vector<i32> m_quickCraftSlots;
    bool m_skipNextRelease = false;

    mc::Slot* m_lastClickSlot = nullptr;
    std::chrono::steady_clock::time_point m_lastClickTime{};

    i32 m_hoveredSlotIndex = -1;

    static mc::ItemStack s_emptyStack;
};

template <typename Menu>
mc::ItemStack ContainerInteraction<Menu>::s_emptyStack;

} // namespace mc::client::ui::kagero::widget
