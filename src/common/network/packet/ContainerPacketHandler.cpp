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

#include "ContainerPacketHandler.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/inventory/AbstractContainerMenu.hpp"
#include "common/entity/inventory/ContainerTypeUtils.hpp"
#include "common/entity/inventory/ContainerTypes.hpp"
#include "common/entity/inventory/PlayerInventory.hpp"
#include "common/entity/inventory/Slot.hpp"

namespace mc {

// ============================================================================
// ContainerPacketHandler 实现
// ============================================================================

bool ContainerPacketHandler::handleContainerClick(Player& player, const ContainerClickPacket& packet)
{
    auto* menu = player.openContainerMenu();
    if (menu == nullptr || menu->getId() != packet.containerId()) {
        return false;
    }

    menu->setCarriedItem(packet.cursorItem());
    const ClickType clickType = ContainerTypes::toClickType(packet.action(), packet.button());
    menu->clicked(packet.slotIndex(), packet.button(), clickType, player);
    return true;
}

void ContainerPacketHandler::handleCloseContainer(Player& player, const CloseContainerPacket& packet)
{
    auto* menu = player.openContainerMenu();
    if (menu == nullptr || menu->getId() != packet.containerId()) {
        return;
    }

    menu->removed(player);
    player.clearOpenContainerMenu();
}

void ContainerPacketHandler::handleHotbarSelect(Player& player, const HotbarSelectPacket& packet)
{
    // 设置玩家选中的快捷栏槽位
    player.inventory().setSelectedSlot(packet.slot());
}

ContainerContentPacket ContainerPacketHandler::createContentPacket(const AbstractContainerMenu& menu)
{
    std::vector<ItemStack> items;
    items.reserve(static_cast<size_t>(menu.getSlotCount()));

    for (i32 i = 0; i < menu.getSlotCount(); ++i) {
        const Slot* slot = menu.getSlot(i);
        if (slot != nullptr) {
            items.push_back(slot->getItem());
        } else {
            items.push_back(ItemStack::EMPTY);
        }
    }

    return ContainerContentPacket(menu.getId(), std::move(items), menu.getCarriedItem());
}

ContainerSlotPacket ContainerPacketHandler::createSlotPacket(const AbstractContainerMenu& menu, i32 slotIndex)
{
    const Slot* slot = menu.getSlot(slotIndex);
    ItemStack item = (slot != nullptr) ? slot->getItem() : ItemStack::EMPTY;
    return ContainerSlotPacket(menu.getId(), slotIndex, item);
}

OpenContainerPacket ContainerPacketHandler::createOpenContainerPacket(
    ContainerId containerId, i32 type, const std::string& title)
{
    return OpenContainerPacket(containerId, type, title);
}

} // namespace mc
