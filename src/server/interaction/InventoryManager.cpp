#include "InventoryManager.hpp"
#include "server/core/PlayerManager.hpp"
#include "server/core/ServerPlayerData.hpp"
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
    // 验证槽位范围 (0-8)
    if (slot < 0 || slot > 8) {
        spdlog::warn("Invalid slot {} for player {}, must be 0-8", slot, playerId);
        return;
    }

    auto it = m_inventories.find(playerId);
    if (it != m_inventories.end()) {
        it->second.setSelectedSlot(slot);
        spdlog::debug("Player {} selected slot {}", playerId, slot);
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
    // 验证槽位范围 (0-35 for main inventory + hotbar)
    if (slot < 0 || slot >= 36) {
        spdlog::warn("Invalid slot {} for player {}, must be 0-35", slot, playerId);
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

    spdlog::debug("Initialized inventory for player {}", playerId);
}

void InventoryManager::cleanupInventory(PlayerId playerId)
{
    m_inventories.erase(playerId);
    spdlog::debug("Cleaned up inventory for player {}", playerId);
}

void InventoryManager::setOnInventoryUpdate(std::function<void(PlayerId, const PlayerInventory&)> callback)
{
    m_onInventoryUpdate = std::move(callback);
}

} // namespace mc::server::interaction