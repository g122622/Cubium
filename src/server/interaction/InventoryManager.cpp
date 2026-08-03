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

#include "InventoryManager.hpp"
#include "common/core/Types.hpp"
#include "common/item/core/ItemStack.hpp"
#include "server/core/PlayerManager.hpp"
#include "server/core/ServerPlayerData.hpp"
#include <functional>
#include <utility>
#include <spdlog/spdlog.h>

namespace mc::server::interaction {

InventoryManager::InventoryManager(core::PlayerManager& playerManager)
    : m_playerManager(playerManager)
{}

PlayerInventory* InventoryManager::getInventory(PlayerId playerId)
{
    auto it = m_inventories.find(playerId);
    if (it != m_inventories.end()) {
        return &it->second;
    }
    return nullptr;
}

const PlayerInventory* InventoryManager::getInventory(PlayerId playerId) const
{
    auto it = m_inventories.find(playerId);
    if (it != m_inventories.end()) {
        return &it->second;
    }
    return nullptr;
}

void InventoryManager::setSelectedSlot(PlayerId playerId, i32 slot)
{
    // 验证槽位范围
    if (slot < 0 || slot >= PlayerInventory::HOTBAR_SIZE) {
        spdlog::warn("Invalid slot {} for player {}, must be 0-{}", slot, playerId, PlayerInventory::HOTBAR_SIZE - 1);
        return;
    }

    auto it = m_inventories.find(playerId);
    if (it != m_inventories.end()) {
        it->second.setSelectedSlot(slot);
    }
}

i32 InventoryManager::getSelectedSlot(PlayerId playerId) const
{
    auto it = m_inventories.find(playerId);
    if (it != m_inventories.end()) {
        return it->second.getSelectedSlot();
    }
    return -1;
}

ItemStack InventoryManager::getHeldItem(PlayerId playerId) const
{
    auto it = m_inventories.find(playerId);
    if (it != m_inventories.end()) {
        return it->second.getSelectedStack();
    }
    return ItemStack();
}

void InventoryManager::setItem(PlayerId playerId, i32 slot, const ItemStack& item)
{
    // 验证槽位范围（快捷栏 + 主背包）
    constexpr i32 MAIN_INVENTORY_SIZE = PlayerInventory::HOTBAR_SIZE + PlayerInventory::MAIN_SIZE;
    if (slot < 0 || slot >= MAIN_INVENTORY_SIZE) {
        spdlog::warn("Invalid slot {} for player {}, must be 0-{}", slot, playerId, MAIN_INVENTORY_SIZE - 1);
        return;
    }

    auto it = m_inventories.find(playerId);
    if (it != m_inventories.end()) {
        it->second.setItem(slot, item);
    }
}

void InventoryManager::syncToClient(PlayerId playerId)
{
    auto* inventory = getInventory(playerId);
    if (!inventory) {
        return;
    }

    if (m_onInventoryUpdate) {
        m_onInventoryUpdate(playerId, *inventory);
    }
}

void InventoryManager::syncAllToClient()
{
    for (const auto& [playerId, inventory] : m_inventories) {
        if (m_onInventoryUpdate) {
            m_onInventoryUpdate(playerId, inventory);
        }
    }
}

void InventoryManager::initializeInventory(PlayerId playerId)
{
    auto& inventory = m_inventories[playerId];
    inventory.clear();
    inventory.setSelectedSlot(0);
}

void InventoryManager::cleanupInventory(PlayerId playerId)
{
    m_inventories.erase(playerId);
}

void InventoryManager::setOnInventoryUpdate(std::function<void(PlayerId, const PlayerInventory&)> callback)
{
    m_onInventoryUpdate = std::move(callback);
}

} // namespace mc::server::interaction