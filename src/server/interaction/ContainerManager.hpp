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
#include <string>
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
    void setMenuFactory(std::function<ContainerMenuCreateResult(
            mc::ContainerId, mc::ContainerType, const BlockPos&, PlayerInventory*, PlayerId)> factory);

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
     * @brief 为玩家建立常驻的「玩家背包菜单」（containerId 固定为 0）
     *
     * 该菜单承载玩家背包屏的全部交互：2x2 合成格、合成结果、护甲、副手与 36 格背包。
     * 客户端本地会建出同布局的窗口，其中的点击直接以 containerId=0 上行；服务端若无对应
     * 菜单，这些点击会被当成「没有打开的容器」拒绝——玩家背包屏于是完全不可交互
     * （移动物品、装备、2x2 合成全部静默失效）。
     *
     * 它不算「打开的容器」：与玩家同时打开的箱子/熔炉等各用各的 containerId，互不冲突，
     * 故不入 m_openContainers。玩家加入时建立、离开时销毁。
     *
     * @param playerId 玩家ID
     * @param playerInventory 玩家物品栏（实体上那一份）
     * @return 是否可用（已存在视为成功）
     */
    bool openPlayerInventoryMenu(PlayerId playerId, PlayerInventory* playerInventory);

    /**
     * @brief 销毁玩家的背包菜单（玩家离开时调用）
     */
    void closePlayerInventoryMenu(PlayerId playerId);

    /**
     * @brief 获取玩家的背包菜单
     * @return 菜单指针，未建立则返回 nullptr
     */
    [[nodiscard]] AbstractContainerMenu* getPlayerInventoryMenu(PlayerId playerId);

    /**
     * @brief 逐 tick 推进所有打开的菜单
     *
     * 把进度型容器（熔炉类）的状态从方块实体刷到菜单的 tracked int，再比对变化、经监听器
     * 下推给客户端。缺了这一步，熔炉的火焰与箭头进度在客户端恒为 0——服务端确实在烧炼，
     * 只是从没告诉过客户端。
     *
     * 由服务器每 tick 调用。
     */
    void tickMenus();

    /**
     * @brief 设置容器进度数据回调
     *
     * 菜单里某个 tracked int 发生变化时触发，参数为 (property, value)，由服务器转换为
     * `container_set_data` 下发。
     */
    void setOnContainerData(std::function<void(PlayerId, mc::ContainerId, i32, i32)> callback);

    /**
     * @brief 处理容器点击
     *
     * 菜单的光标（carried）由服务端独占维护：点击按服务端当前光标结算，客户端上报的
     * 光标预测值一律不参与结算。详见实现处说明。
     *
     * @param playerId 玩家ID
     * @param containerId 容器ID
     * @param slot 槽位索引
     * @param button 鼠标按钮
     * @param mode 点击模式
     * @return 点击结果（含结算后的权威光标）
     */
    [[nodiscard]] Result<ContainerClickResult> handleClick(
        PlayerId playerId, mc::ContainerId containerId, i32 slot, u8 button, u8 mode);

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

    /// 玩家背包菜单（containerId 恒为 0），每个在线玩家常驻一个，与「打开的容器」并存。
    std::unordered_map<PlayerId, std::unique_ptr<AbstractContainerMenu>> m_playerInventoryMenus;

    std::function<ContainerMenuCreateResult(
        mc::ContainerId, mc::ContainerType, const BlockPos&, PlayerInventory*, PlayerId)>
        m_menuFactory;
    std::function<void(PlayerId, mc::ContainerId, mc::ContainerType, const std::string&, i32)> m_onContainerOpen;
    std::function<void(PlayerId, mc::ContainerId, mc::ContainerType, const BlockPos&)> m_onContainerClose;
    std::function<void(PlayerId, const AbstractContainerMenu&)> m_onContainerUpdate;
    std::function<void(PlayerId, mc::ContainerId, i32, i32)> m_onContainerData;
};

} // namespace interaction
} // namespace mc::server