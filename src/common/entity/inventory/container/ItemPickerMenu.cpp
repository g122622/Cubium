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

#include "common/entity/inventory/container/ItemPickerMenu.hpp"

#include "common/core/Types.hpp"
#include "common/entity/entities/player/GameModeUtils.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/inventory/ContainerTypes.hpp"
#include "common/entity/inventory/PlayerInventory.hpp"
#include "common/entity/inventory/Slot.hpp"
#include <cstddef>
#include <utility>

namespace mc {

ItemPickerMenu::ItemPickerMenu(ContainerId id, PlayerInventory* playerInventory, bool loadPalette)
    : AbstractContainerMenu(id, playerInventory)
{
    // 槽位 0-3: 护甲
    addPlayerArmorSlots(8, 8);

    // 槽位 4-30: 主背包 (3x9)
    addPlayerInventorySlots(8, 84);

    // 槽位 31-39: 快捷栏 (1x9)
    addPlayerHotbarSlots(8, 142);

    // 槽位 40: 副手
    addPlayerOffhandSlot(77, 62);

    if (loadPalette) {
        reloadPalette();
    }
}

ItemStack ItemPickerMenu::clicked(i32 slotIndex, i32 button, ClickType clickType, Player& player)
{
    // 虚拟调色板槽：创造取物 clone（最前拦截，避免基类 getSlot 越界）
    if (slotIndex >= PALETTE_VIRTUAL_BASE) {
        return _handlePaletteClone(slotIndex, button, player);
    }

    return AbstractContainerMenu::clicked(slotIndex, button, clickType, player);
}

ItemStack ItemPickerMenu::_handlePaletteClone(i32 slotIndex, i32 button, Player& player)
{
    // 仅创造模式可取物（对齐原版 CreativeModeInventoryScreen）
    if (!entity::GameModeUtils::isCreative(player.gameMode())) {
        return m_carried;
    }

    const i32 visibleIndex = slotIndex - PALETTE_VIRTUAL_BASE;
    if (visibleIndex < 0 || visibleIndex >= static_cast<i32>(m_paletteEntries.size())) {
        return m_carried;
    }

    ItemStack stack = m_paletteEntries[static_cast<std::size_t>(visibleIndex)].stack.copy();
    // 右键取单个；左键取整组（池中 entry 已是 maxStackSize，damageable 为 1）
    if (button == 1 && stack.getCount() > 1) {
        stack.setCount(1);
    }

    m_carried = std::move(stack);
    return m_carried;
}

ItemStack ItemPickerMenu::quickMoveStack(i32 slotIndex, Player& player)
{
    (void)player;

    // 调色板虚拟槽不走 Shift 移动（创造取物用 Clone，非 QuickMove）
    if (slotIndex >= PALETTE_VIRTUAL_BASE) {
        return ItemStack();
    }

    Slot* slot = getSlot(slotIndex);
    if (slot == nullptr || slot->isEmpty()) {
        return ItemStack();
    }

    ItemStack originalStack = slot->getItem();
    ItemStack movingStack = originalStack.copy();

    // 槽位布局：0-3 护甲 / 4-30 主背包 / 31-39 快捷栏 / 40 副手
    if (slotIndex >= ARMOR_SLOT_START && slotIndex < ARMOR_SLOT_START + ARMOR_SLOT_COUNT) {
        // 护甲 → 主背包 → 快捷栏
        if (!moveItemToRange(movingStack, PLAYER_INV_START, PLAYER_INV_END, true)) {
            if (!moveItemToRange(movingStack, HOTBAR_START, HOTBAR_END, true)) {
                return ItemStack();
            }
        }
    } else if (slotIndex >= PLAYER_INV_START && slotIndex <= PLAYER_INV_END) {
        // 主背包 → 快捷栏
        if (!moveItemToRange(movingStack, HOTBAR_START, HOTBAR_END)) {
            return ItemStack();
        }
    } else if (slotIndex >= HOTBAR_START && slotIndex <= HOTBAR_END) {
        // 快捷栏 → 主背包
        if (!moveItemToRange(movingStack, PLAYER_INV_START, PLAYER_INV_END)) {
            return ItemStack();
        }
    } else if (slotIndex == OFFHAND_SLOT) {
        // 副手 → 主背包 → 快捷栏
        if (!moveItemToRange(movingStack, PLAYER_INV_START, PLAYER_INV_END, true)) {
            if (!moveItemToRange(movingStack, HOTBAR_START, HOTBAR_END, true)) {
                return ItemStack();
            }
        }
    }

    if (movingStack.isEmpty()) {
        slot->set(ItemStack());
    } else {
        slot->set(movingStack);
    }

    return originalStack;
}

} // namespace mc
