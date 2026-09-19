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
#include "common/entity/inventory/PlayerInventory.hpp"
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

void InventoryManager::setInventoryResolver(std::function<PlayerInventory*(PlayerId)> resolver)
{
    m_resolveInventory = std::move(resolver);
}

PlayerInventory* InventoryManager::getInventory(PlayerId playerId)
{
    if (!m_resolveInventory) {
        return nullptr;
    }
    return m_resolveInventory(playerId);
}

const PlayerInventory* InventoryManager::getInventory(PlayerId playerId) const
{
    // 解析器不修改本对象，const 上下文可安全调用。
    if (!m_resolveInventory) {
        return nullptr;
    }
    return m_resolveInventory(playerId);
}

void InventoryManager::setSelectedSlot(PlayerId playerId, i32 slot)
{
    // 验证槽位范围
    if (slot < 0 || slot >= PlayerInventory::HOTBAR_SIZE) {
        spdlog::warn("Invalid slot {} for player {}, must be 0-{}", slot, playerId, PlayerInventory::HOTBAR_SIZE - 1);
        return;
    }

    PlayerInventory* inventory = getInventory(playerId);
    if (inventory != nullptr) {
        inventory->setSelectedSlot(slot);
    }
}

i32 InventoryManager::getSelectedSlot(PlayerId playerId) const
{
    const PlayerInventory* inventory = getInventory(playerId);
    if (inventory != nullptr) {
        return inventory->getSelectedSlot();
    }
    return -1;
}

ItemStack InventoryManager::getHeldItem(PlayerId playerId) const
{
    const PlayerInventory* inventory = getInventory(playerId);
    if (inventory != nullptr) {
        return inventory->getSelectedStack();
    }
    return ItemStack();
}

void InventoryManager::setItem(PlayerId playerId, i32 slot, const ItemStack& item)
{
    // 槽位号是物品栏内部索引，覆盖快捷栏、主背包、护甲与副手。
    if (slot < 0 || slot >= PlayerInventory::TOTAL_SIZE) {
        spdlog::warn("Invalid slot {} for player {}, must be 0-{}", slot, playerId, PlayerInventory::TOTAL_SIZE - 1);
        return;
    }

    PlayerInventory* inventory = getInventory(playerId);
    if (inventory != nullptr) {
        inventory->setItem(slot, item);
    }
}

void InventoryManager::syncToClient(PlayerId playerId)
{
    if (!m_onInventoryUpdate) {
        return;
    }

    PlayerInventory* inventory = getInventory(playerId);
    if (inventory == nullptr) {
        return;
    }

    m_onInventoryUpdate(playerId, *inventory);
}

void InventoryManager::syncAllToClient()
{
    if (!m_onInventoryUpdate) {
        return;
    }

    m_playerManager.forEachPlayer([this](const ServerPlayerData& playerData) {
        PlayerInventory* inventory = getInventory(playerData.playerId);
        if (inventory != nullptr) {
            m_onInventoryUpdate(playerData.playerId, *inventory);
        }
    });
}

void InventoryManager::setOnInventoryUpdate(std::function<void(PlayerId, const PlayerInventory&)> callback)
{
    m_onInventoryUpdate = std::move(callback);
}

} // namespace mc::server::interaction
