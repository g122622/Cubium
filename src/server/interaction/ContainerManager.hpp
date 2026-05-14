#pragma once

#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/entity/inventory/AbstractContainerMenu.hpp"
#include "common/entity/inventory/ContainerTypes.hpp"
#include "common/entity/inventory/IInventory.hpp"
#include "common/entity/inventory/PlayerInventory.hpp"
#include "common/world/block/BlockPos.hpp"
#include <functional>
#include <memory>
#include <unordered_map>

namespace mc::server {

// 前向声明
namespace core {
class PlayerManager;
}

/**
 * @brief 容器点击结果
 */
struct ContainerClickResult {
    bool success = false;
    ItemStack cursorItem;
    std::string message;
};

/**
 * @brief 容器菜单创建结果
 */
struct ContainerMenuCreateResult {
    std::unique_ptr<AbstractContainerMenu> menu;
    std::shared_ptr<IInventory> inventoryOwner;
};

namespace interaction {

class InventoryManager;

/**
 * @brief 容器管理器
 *
 * 管理玩家的容器交互：
 * - 打开/关闭容器菜单
 * - 处理容器点击
 * - 合成系统
 */
class ContainerManager {
public:
    /**
     * @brief 构造函数
     */
    explicit ContainerManager(core::PlayerManager& playerManager);

    /**
     * @brief 设置物品栏管理器（用于创建需要玩家背包的容器菜单）
     */
    void setInventoryManager(InventoryManager* inventoryManager);

    /**
     * @brief 设置菜单工厂
     */
    void setMenuFactory(
        std::function<ContainerMenuCreateResult(mc::ContainerId, mc::ContainerType, const BlockPos&, PlayerInventory*)>
            factory);

    /**
     * @brief 打开容器
     * @param playerId 玩家ID
     * @param type 容器类型
     * @param pos 方块位置（如果是方块容器）
     * @return 容器ID
     */
    [[nodiscard]] Result<mc::ContainerId> openContainer(PlayerId playerId, mc::ContainerType type, const BlockPos& pos);

    /**
     * @brief 关闭容器
     * @param playerId 玩家ID
     */
    void closeContainer(PlayerId playerId);

    /**
     * @brief 处理容器点击
     * @param playerId 玩家ID
     * @param containerId 容器ID
     * @param slot 槽位索引
     * @param button 鼠标按钮
     * @param mode 点击模式
     * @param carriedItem 手持物品
     * @return 点击结果
     */
    [[nodiscard]] Result<ContainerClickResult> handleClick(
        PlayerId playerId, mc::ContainerId containerId, i32 slot, u8 button, u8 mode, const ItemStack& carriedItem);

    /**
     * @brief 获取打开的菜单
     * @param playerId 玩家ID
     * @return 菜单指针，如果没有打开的菜单则返回 nullptr
     */
    [[nodiscard]] AbstractContainerMenu* getOpenMenu(PlayerId playerId);

    /**
     * @brief 获取打开的菜单（const版本）
     */
    [[nodiscard]] const AbstractContainerMenu* getOpenMenu(PlayerId playerId) const;

    /**
     * @brief 获取玩家打开的容器类型
     */
    [[nodiscard]] mc::ContainerType getOpenContainerType(PlayerId playerId) const;

    /**
     * @brief 检查玩家是否有打开的容器
     */
    [[nodiscard]] bool hasOpenContainer(PlayerId playerId) const;

    /**
     * @brief 设置容器打开回调
     */
    void setOnContainerOpen(
        std::function<void(PlayerId, mc::ContainerId, mc::ContainerType, const std::string&, i32)> callback);

    /**
     * @brief 设置容器关闭回调
     */
    void setOnContainerClose(
        std::function<void(PlayerId, mc::ContainerId, mc::ContainerType, const BlockPos&)> callback);

    /**
     * @brief 设置容器内容更新回调
     */
    void setOnContainerUpdate(std::function<void(PlayerId, const AbstractContainerMenu&)> callback);

private:
    core::PlayerManager& m_playerManager;
    InventoryManager* m_inventoryManager = nullptr;

    struct OpenContainer {
        std::unique_ptr<AbstractContainerMenu> menu;
        std::shared_ptr<IInventory> inventoryOwner;
        mc::ContainerType type = mc::ContainerType::Player;
        BlockPos position;
    };

    std::unordered_map<PlayerId, OpenContainer> m_openContainers;
    std::unordered_map<PlayerId, mc::ContainerId> m_nextContainerIds;

    std::function<ContainerMenuCreateResult(mc::ContainerId, mc::ContainerType, const BlockPos&, PlayerInventory*)>
        m_menuFactory;
    std::function<void(PlayerId, mc::ContainerId, mc::ContainerType, const std::string&, i32)> m_onContainerOpen;
    std::function<void(PlayerId, mc::ContainerId, mc::ContainerType, const BlockPos&)> m_onContainerClose;
    std::function<void(PlayerId, const AbstractContainerMenu&)> m_onContainerUpdate;
};

} // namespace interaction
} // namespace mc::server