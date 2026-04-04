#include "ContainerManager.hpp"
#include "InventoryManager.hpp"
#include "server/core/PlayerManager.hpp"
#include "server/core/ServerPlayerData.hpp"
#include "server/menu/CraftingMenu.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/network/packet/ContainerPacketHandler.hpp"
#include <spdlog/spdlog.h>

namespace mc::server::interaction {

ContainerManager::ContainerManager(core::PlayerManager& playerManager)
    : m_playerManager(playerManager)
{
}

void ContainerManager::setInventoryManager(InventoryManager* inventoryManager)
{
    m_inventoryManager = inventoryManager;
}

Result<mc::ContainerId> ContainerManager::openContainer(
    PlayerId playerId,
    mc::ContainerType type,
    const BlockPos& pos)
{
    auto* playerData = m_playerManager.getPlayer(playerId);
    if (!playerData || !playerData->loggedIn) {
        return Error(ErrorCode::InvalidArgument, "Player not found or not logged in");
    }

    // 检查是否已有打开的容器
    auto it = m_openContainers.find(playerId);
    if (it != m_openContainers.end() && it->second.menu) {
        // 关闭现有容器
        closeContainer(playerId);
    }

    // 获取下一个容器ID
    mc::ContainerId containerId = m_nextContainerIds[playerId];
    m_nextContainerIds[playerId] = containerId + 1;

    // 创建容器
    OpenContainer openContainer;
    openContainer.type = type;
    openContainer.position = pos;

    PlayerInventory* playerInventory =
        (m_inventoryManager != nullptr) ? m_inventoryManager->getInventory(playerId) : nullptr;

    switch (type) {
        case mc::ContainerType::CraftingTable:
            if (playerInventory == nullptr) {
                return Error(ErrorCode::InvalidState, "Player inventory not initialized");
            }
            {
                auto menu = std::make_unique<CraftingMenu>(containerId, playerInventory, nullptr);
                menu->updateResult();
                openContainer.menu = std::move(menu);
            }
            break;
        case mc::ContainerType::Player:
        default:
            // Player 容器是默认的，不需要特殊菜单
            break;
    }

    m_openContainers[playerId] = std::move(openContainer);

    String title = String(ContainerTypes::getDefaultTitle(type));
    i32 slotCount = ContainerTypes::getSlotCount(type);

    const auto& opened = m_openContainers[playerId];
    if (opened.menu) {
        slotCount = opened.menu->getSlotCount();
    }

    if (m_onContainerOpen) {
        m_onContainerOpen(playerId, containerId, type, title, slotCount);
    }

    spdlog::debug("Player {} opened container type {} at ({}, {}, {})",
                  playerId, static_cast<i32>(type), pos.x, pos.y, pos.z);

    return containerId;
}

void ContainerManager::closeContainer(PlayerId playerId)
{
    auto it = m_openContainers.find(playerId);
    if (it == m_openContainers.end()) {
        return;
    }

    mc::ContainerId containerId = 0;
    if (it->second.menu) {
        containerId = it->second.menu->getId();

        auto* playerData = m_playerManager.getPlayer(playerId);
        const String username = (playerData != nullptr) ? playerData->username : String("ContainerPlayer");
        Player menuPlayer(playerId, username);
        it->second.menu->removed(menuPlayer);
    }

    if (m_onContainerClose) {
        m_onContainerClose(playerId, containerId);
    }

    m_openContainers.erase(it);

    spdlog::debug("Player {} closed container", playerId);
}

Result<ContainerClickResult> ContainerManager::handleClick(
    PlayerId playerId,
    mc::ContainerId containerId,
    i32 slot,
    u8 button,
    u8 mode,
    const ItemStack& carriedItem)
{
    auto* playerData = m_playerManager.getPlayer(playerId);
    if (!playerData || !playerData->loggedIn) {
        return Error(ErrorCode::InvalidArgument, "Player not found or not logged in");
    }

    auto it = m_openContainers.find(playerId);
    if (it == m_openContainers.end() || !it->second.menu) {
        return Error(ErrorCode::InvalidState, "No open container");
    }

    auto& openContainer = it->second;
    if (openContainer.menu->getId() != containerId) {
        return Error(ErrorCode::InvalidArgument, "Container ID mismatch");
    }

    ClickType clickType = (button == 0) ? ClickType::Pick : ClickType::PickSome;
    if (mode == static_cast<u8>(ClickAction::QuickMove)) {
        clickType = ClickType::QuickMove;
    }

    Player menuPlayer(playerId, playerData->username);
    openContainer.menu->setCarriedItem(carriedItem);
    openContainer.menu->clicked(slot, button, clickType, menuPlayer);

    if (m_onContainerUpdate) {
        m_onContainerUpdate(playerId, *openContainer.menu);
    }

    return ContainerClickResult{true, openContainer.menu->getCarriedItem(), "Click handled"};
}

AbstractContainerMenu* ContainerManager::getOpenMenu(PlayerId playerId)
{
    auto it = m_openContainers.find(playerId);
    if (it != m_openContainers.end()) {
        return it->second.menu.get();
    }
    return nullptr;
}

const AbstractContainerMenu* ContainerManager::getOpenMenu(PlayerId playerId) const
{
    auto it = m_openContainers.find(playerId);
    if (it != m_openContainers.end()) {
        return it->second.menu.get();
    }
    return nullptr;
}

ContainerType ContainerManager::getOpenContainerType(PlayerId playerId) const
{
    auto it = m_openContainers.find(playerId);
    if (it != m_openContainers.end()) {
        return it->second.type;
    }
    return ContainerType::Player;
}

bool ContainerManager::hasOpenContainer(PlayerId playerId) const
{
    return m_openContainers.find(playerId) != m_openContainers.end();
}

void ContainerManager::setOnContainerOpen(
    std::function<void(PlayerId, ContainerId, ContainerType, const String&, i32)> callback)
{
    m_onContainerOpen = std::move(callback);
}

void ContainerManager::setOnContainerClose(
    std::function<void(PlayerId, ContainerId)> callback)
{
    m_onContainerClose = std::move(callback);
}

void ContainerManager::setOnContainerUpdate(
    std::function<void(PlayerId, const AbstractContainerMenu&)> callback)
{
    m_onContainerUpdate = std::move(callback);
}

} // namespace mc::server::interaction