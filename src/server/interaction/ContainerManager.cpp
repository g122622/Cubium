#include "ContainerManager.hpp"
#include "server/core/PlayerManager.hpp"
#include "server/core/ServerPlayerData.hpp"
#include "server/menu/CraftingMenu.hpp"
#include <spdlog/spdlog.h>

namespace mc::server::interaction {

ContainerManager::ContainerManager(core::PlayerManager& playerManager)
    : m_playerManager(playerManager)
{
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

    switch (type) {
        case mc::ContainerType::CraftingTable:
            // CraftingMenu 需要 PlayerInventory，这里暂时不创建菜单
            // 实际菜单创建应该在玩家打开工作台时进行
            // openContainer.menu = std::make_unique<CraftingMenu>(containerId, playerInventory, nullptr);
            break;
        case mc::ContainerType::Player:
        default:
            // Player 容器是默认的，不需要特殊菜单
            break;
    }

    m_openContainers[playerId] = std::move(openContainer);

    String title;
    i32 slotCount = 0;

    if (type == mc::ContainerType::CraftingTable) {
        title = "Crafting";
        slotCount = 10;  // 9 格合成 + 1 格结果
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

    // 处理点击
    // TODO: 实现 Player 对象传递
    // ClickType clickType = (button == 0) ? ClickType::Pick : ClickType::PickSome;
    // if (mode == 1) clickType = ClickType::QuickMove;
    // openContainer.menu->clicked(slot, button, clickType, player);

    if (m_onContainerUpdate) {
        m_onContainerUpdate(playerId, *openContainer.menu);
    }

    return ContainerClickResult{true, ItemStack{}, "Click handled"};
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