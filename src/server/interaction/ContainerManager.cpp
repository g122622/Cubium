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
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/entity/ecs/context/EntityRegistry.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/inventory/ContainerTypeUtils.hpp"
#include "common/entity/inventory/ContainerTypes.hpp"
#include "common/entity/inventory/container/ChestContainer.hpp"
#include "common/entity/inventory/container/FurnaceContainer.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/blockentity/processing/AbstractFurnaceEntity.hpp"
#include "common/world/blockentity/storage/ChestEntity.hpp"
#include "server/core/PlayerManager.hpp"
#include "server/core/ServerPlayerData.hpp"
#include "server/menu/CraftingMenu.hpp"
#include <functional>
#include <memory>
#include <string>
#include <utility>

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

    // 获取下一个容器ID。
    // 从 1 起分配：0 是玩家背包的固定 id（PLAYER_CONTAINER_ID），若首个容器也拿到 0，
    // 两者就会撞号——此后该玩家背包屏的点击（containerId=0）会通过 id 校验、被当作对
    // 这个容器菜单的点击处理，槽位语义完全不同且不会报任何错。
    // 计数器按玩家各存一份，首次访问由 unordered_map 的值初始化得到 0，故此处需显式抬到 1。
    mc::ContainerId containerId = m_nextContainerIds[playerId];
    constexpr mc::ContainerId kFirstContainerId = mc::inventory::PLAYER_CONTAINER_ID + 1;
    if (containerId < kFirstContainerId) {
        containerId = kFirstContainerId;
    }
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

        auto menu = std::make_unique<CraftingMenu>(containerId, playerInventory);
        menu->updateResult();
        createdMenu.menu = std::move(menu);
    }

    if (type != mc::ContainerType::Player && !createdMenu.menu) {
        return Error(ErrorCode::InvalidState, "Unsupported container type");
    }

    openContainer.menu = std::move(createdMenu.menu);
    openContainer.inventoryOwner = std::move(createdMenu.inventoryOwner);

    m_openContainers[playerId] = std::move(openContainer);

    // 进度型容器（熔炉类）的火焰与箭头进度经 tracked int 同步。每 tick 由 tickMenus() 把状态
    // 从方块实体刷进这些 int 并比对变化，变化时经此监听器下推。不注册的话，服务端照常烧炼，
    // 但客户端那边进度永远是 0。
    if (auto* furnaceMenu = dynamic_cast<blockentity::FurnaceContainer*>(m_openContainers[playerId].menu.get());
        furnaceMenu != nullptr) {
        const mc::ContainerId openedId = containerId;
        furnaceMenu->addIntListener([this, playerId, openedId](i32 property, i32 value) {
            if (m_onContainerData) {
                m_onContainerData(playerId, openedId, property, value);
            }
        });
    }

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
        // ECS 迁移：占位 Player 构造需要 registry 句柄。此处 menuPlayer 仅作 removed 回调形参，
        // 无 world 上下文，故复用 handleClick 同款静态 registry。
        // TODO: 占位 Player 是临时方案，后续应重构容器系统避免构造完整 Player 仅为传参。
        static ecs::EntityRegistry s_menuPlayerRegistry{"container-menu"};
        Player menuPlayer(playerId, username, s_menuPlayerRegistry);
        it->second.menu->removed(menuPlayer);
    }

    if (m_onContainerClose) {
        m_onContainerClose(playerId, containerId, containerType, position);
    }

    m_openContainers.erase(it);
}

bool ContainerManager::openPlayerInventoryMenu(PlayerId playerId, PlayerInventory* playerInventory)
{
    // 幂等：登录流程可能因重连等原因对一个玩家触发多次。
    if (AbstractContainerMenu* existing = getPlayerInventoryMenu(playerId); existing != nullptr) {
        return true;
    }
    if (playerInventory == nullptr) {
        return false;
    }

    // containerId 固定为 PLAYER_CONTAINER_ID——客户端本地建背包屏时用的就是这个值，
    // 它上报的点击也只会带这个 id。
    auto menu = std::make_unique<InventoryCraftingMenu>(mc::inventory::PLAYER_CONTAINER_ID, playerInventory);
    menu->updateResult();
    m_playerInventoryMenus[playerId] = std::move(menu);
    return true;
}

void ContainerManager::tickMenus()
{
    for (auto& [playerId, openContainer] : m_openContainers) {
        (void)playerId;
        auto* menu = openContainer.menu.get();
        if (menu == nullptr) {
            continue;
        }

        // 进度型容器先从方块实体取最新状态写进 tracked int，再统一比对变化。
        if (auto* furnaceMenu = dynamic_cast<blockentity::FurnaceContainer*>(menu); furnaceMenu != nullptr) {
            furnaceMenu->syncProgressFromEntity();
        }
        menu->detectAndSendChanges();
    }
}

void ContainerManager::setOnContainerData(std::function<void(PlayerId, mc::ContainerId, i32, i32)> callback)
{
    m_onContainerData = std::move(callback);
}

void ContainerManager::closePlayerInventoryMenu(PlayerId playerId)
{
    auto it = m_playerInventoryMenus.find(playerId);
    if (it == m_playerInventoryMenus.end()) {
        return;
    }

    // 与关闭普通容器一致：先让菜单把光标上的物品归还玩家背包，再丢弃菜单本身。
    if (it->second != nullptr) {
        if (auto* playerData = m_playerManager.getPlayer(playerId); playerData != nullptr) {
            static ecs::EntityRegistry s_menuPlayerRegistry{"container-menu"};
            Player menuPlayer(playerId, playerData->username, s_menuPlayerRegistry);
            it->second->removed(menuPlayer);
        }
    }
    m_playerInventoryMenus.erase(it);
}

AbstractContainerMenu* ContainerManager::getPlayerInventoryMenu(PlayerId playerId)
{
    auto it = m_playerInventoryMenus.find(playerId);
    return (it != m_playerInventoryMenus.end()) ? it->second.get() : nullptr;
}

Result<ContainerClickResult> ContainerManager::handleClick(
    PlayerId playerId, mc::ContainerId containerId, i32 slot, u8 button, u8 mode)
{
    auto* playerData = m_playerManager.getPlayer(playerId);
    if (!playerData || !playerData->loggedIn) {
        spdlog::warn("ContainerClick dropped: player {} not found or not logged in (found={} loggedIn={})",
            playerId,
            playerData != nullptr,
            playerData != nullptr && playerData->loggedIn);
        return Error(ErrorCode::InvalidArgument, "Player not found or not logged in");
    }

    // 点击落在哪个菜单上，取决于客户端上报的 containerId：
    //   - 打开着容器时，客户端只会以该容器的 id 上报（窗口里的玩家背包部分也用它）；
    //   - 没开容器时，客户端背包屏的点击一律以固定的 PLAYER_CONTAINER_ID 上报。
    // 故先按 id 匹配已打开的容器，不匹配再落到玩家背包菜单。
    AbstractContainerMenu* menu = nullptr;
    auto openIt = m_openContainers.find(playerId);
    if (openIt != m_openContainers.end() && openIt->second.menu && openIt->second.menu->getId() == containerId) {
        menu = openIt->second.menu.get();
    } else if (containerId == mc::inventory::PLAYER_CONTAINER_ID) {
        menu = getPlayerInventoryMenu(playerId);
    }

    if (menu == nullptr) {
        // 记一条诊断日志。客户端点击若长期被拒会表现为「物品栏点了没反应」，而从外部完全
        // 看不出原因；把双方各自的 id 与已注册的菜单一并记下，可直接对照是哪一侧对不上。
        const bool hasOpen = openIt != m_openContainers.end() && openIt->second.menu != nullptr;
        spdlog::warn("ContainerClick rejected: player {} reported containerId {} but no menu matched "
                     "(hasOpenContainer={}, openMenuId={}, hasPlayerInventoryMenu={})",
            playerId,
            containerId,
            hasOpen,
            hasOpen ? openIt->second.menu->getId() : -1,
            getPlayerInventoryMenu(playerId) != nullptr);
        return Error(ErrorCode::InvalidState, "No matching container menu for the reported container id");
    }

    const ClickType clickType = ContainerTypes::toClickType(static_cast<ClickAction>(mode), button);

    // ECS 迁移：占位 Player 构造需要 registry 句柄。此处 menuPlayer 仅作容器点击
    // 回调的 Player 形参（临时占位，非真实世界玩家），无 world 上下文，故配静态 registry。
    // TODO: 占位 Player 是临时方案，后续应重构容器系统避免构造完整 Player 仅为传参。
    static ecs::EntityRegistry s_menuPlayerRegistry{"container-menu"};
    Player menuPlayer(playerId, playerData->username, s_menuPlayerRegistry);

    // 客户端上报的 ContainerClick.carriedItem 是它在本地预测执行完这次点击「之后」的光标
    // （原版客户端先结算菜单再取光标组包），语义上不是点击前状态。把它当成点击前光标写回
    // 会让服务端用错位的起始状态结算：槽位有 1 个同种物品、光标也上报 1 个时，本该「拾取
    // 到光标」的点击会走成「合并进槽位」，物品原地翻倍而目标槽位始终为空。
    // 服务端的光标是权威状态，只由服务端自己在结算中推进；客户端预测值仅用于对账，本实现
    // 改为每次结算后全量下发权威光标（见 setOnContainerUpdate），客户端据此纠正本地预测。
    // 同时这也让「服务端拒绝某次点击」时客户端不会被自己的预测带偏。
    menu->clicked(slot, button, clickType, menuPlayer);

    if (m_onContainerUpdate) {
        m_onContainerUpdate(playerId, *menu);
    }

    return ContainerClickResult{true, menu->getCarriedItem(), "Click handled"};
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