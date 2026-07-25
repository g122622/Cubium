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

#include "ContainerManager.hpp"
#include "InventoryManager.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/inventory/ContainerTypeUtils.hpp"
#include "common/entity/inventory/container/ChestContainer.hpp"
#include "common/entity/inventory/container/FurnaceContainer.hpp"
#include "common/world/blockentity/processing/AbstractFurnaceEntity.hpp"
#include "common/world/blockentity/storage/ChestEntity.hpp"
#include "server/core/PlayerManager.hpp"
#include "server/core/ServerPlayerData.hpp"
#include "server/menu/CraftingMenu.hpp"

namespace mc::server::interaction {

ContainerManager::ContainerManager(core::PlayerManager& playerManager)
    : m_playerManager(playerManager)
{}

void ContainerManager::setInventoryManager(InventoryManager* inventoryManager)
{
    m_inventoryManager = inventoryManager;
}

void ContainerManager::setMenuFactory(
    std::function<ContainerMenuCreateResult(ContainerId, ContainerType, const BlockPos&, PlayerInventory*, PlayerId)>
        factory)
{
    m_menuFactory = std::move(factory);
}

Result<mc::ContainerId> ContainerManager::openContainer(PlayerId playerId, mc::ContainerType type, const BlockPos& pos)
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

    ContainerMenuCreateResult createdMenu;

    if (m_menuFactory) {
        createdMenu = m_menuFactory(containerId, type, pos, playerInventory, playerId);
    }

    if (!createdMenu.menu && type == mc::ContainerType::Crafting) {
        if (playerInventory == nullptr) {
            return Error(ErrorCode::InvalidState, "Player inventory not initialized");
        }

        auto menu = std::make_unique<CraftingMenu>(containerId, playerInventory, nullptr);
        menu->updateResult();
        createdMenu.menu = std::move(menu);
    }

    if (type != mc::ContainerType::Player && !createdMenu.menu) {
        return Error(ErrorCode::InvalidState, "Unsupported container type");
    }

    openContainer.menu = std::move(createdMenu.menu);
    openContainer.inventoryOwner = std::move(createdMenu.inventoryOwner);

    m_openContainers[playerId] = std::move(openContainer);

    std::string title = std::string(ContainerTypes::getDefaultTitle(type));
    i32 slotCount = ContainerTypes::getSlotCount(type);

    const auto& opened = m_openContainers[playerId];
    if (opened.menu) {
        slotCount = opened.menu->getSlotCount();
    }

    if (m_onContainerOpen) {
        m_onContainerOpen(playerId, containerId, type, title, slotCount);
    }

    return containerId;
}

void ContainerManager::closeContainer(PlayerId playerId)
{
    auto it = m_openContainers.find(playerId);
    if (it == m_openContainers.end()) {
        return;
    }

    mc::ContainerId containerId = 0;
    mc::ContainerType containerType = mc::ContainerType::Player;
    BlockPos position;
    if (it->second.menu) {
        containerId = it->second.menu->getId();
        containerType = it->second.type;
        position = it->second.position;

        auto* playerData = m_playerManager.getPlayer(playerId);
        const std::string username = (playerData != nullptr) ? playerData->username : std::string("ContainerPlayer");
        Player menuPlayer(playerId, username);
        it->second.menu->removed(menuPlayer);
    }

    if (m_onContainerClose) {
        m_onContainerClose(playerId, containerId, containerType, position);
    }

    m_openContainers.erase(it);
}

Result<ContainerClickResult> ContainerManager::handleClick(
    PlayerId playerId, mc::ContainerId containerId, i32 slot, u8 button, u8 mode, const ItemStack& carriedItem)
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

    const ClickType clickType = ContainerTypes::toClickType(static_cast<ClickAction>(mode), button);

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
    std::function<void(PlayerId, ContainerId, ContainerType, const std::string&, i32)> callback)
{
    m_onContainerOpen = std::move(callback);
}

void ContainerManager::setOnContainerClose(
    std::function<void(PlayerId, ContainerId, ContainerType, const BlockPos&)> callback)
{
    m_onContainerClose = std::move(callback);
}

void ContainerManager::setOnContainerUpdate(std::function<void(PlayerId, const AbstractContainerMenu&)> callback)
{
    m_onContainerUpdate = std::move(callback);
}

} // namespace mc::server::interaction