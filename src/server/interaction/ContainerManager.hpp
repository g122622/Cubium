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
#include "common/entity/inventory/INamedContainerProvider.hpp"
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
     * @brief 打开实体容器（村民交易、箱子船等）
     *
     * 与方块容器共用同一条打开链路：关闭旧容器 → 分配容器 id → 由提供者创建菜单 →
     * 登记进 m_openContainers → 下发 OpenScreen。菜单由 `provider.createMenu` 创建，
     * 容器类型取自 `provider.getMenuType`，标题取自 `provider.getDisplayName`。
     *
     * @param playerId 玩家ID
     * @param provider 命名容器提供者（实体）
     * @param player 玩家的真实实体（创建菜单需要其物品栏）
     * @return 成功打开返回 true；提供者拒绝（返回空菜单）或玩家未登录返回 false
     */
    [[nodiscard]] bool openEntityContainer(PlayerId playerId, INamedContainerProvider& provider, Player& player);

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
     * @param player 玩家的**真实实体**（须带 world）。丢弃类点击（slot=-999，即把光标上的
     *               物品丢出窗口）要在世界里生成掉落物实体，而占位 Player 没有 world，
     *               会静默什么都不做——客户端本地预测看起来已经丢出去了，服务端却什么都没发生。
     *               nullptr 表示调用方无法解析出实体，此时会退回占位 Player 并记一条告警。
     * @param containerId 容器ID
     * @param slot 槽位索引
     * @param button 鼠标按钮
     * @param mode 点击模式
     * @return 点击结果（含结算后的权威光标）
     */
    [[nodiscard]] Result<ContainerClickResult> handleClick(
        PlayerId playerId, Player* player, mc::ContainerId containerId, i32 slot, u8 button, u8 mode);

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
    /**
     * @brief 给菜单装上容器管理器统一提供的回调。
     *
     * 目前只有一项：**丢弃物品**。`AbstractContainerMenu::dropItem` 只是转调一个回调，
     * 而本项目管理器从未设置过它——于是「把光标上的物品丢出窗口」（slot=-999，所有第三方
     * 客户端的丢物品路径）在服务端**完全没有效果**，且没有任何日志。
     *
     * @param menu 目标菜单（其生命周期由调用方持有）。
     * @param playerId 玩家ID（仅用于日志定位）。
     */
    void _installMenuCallbacks(AbstractContainerMenu& menu, PlayerId playerId);

    /**
     * @brief 通知服务器「某玩家打开了容器」并下发 OpenScreen
     *
     * 方块容器与实体容器共用：标题为空时回落到容器类型的默认标题，槽位数以实际菜单为准。
     *
     * @param playerId 玩家ID
     * @param containerId 容器ID
     * @param type 容器类型（决定客户端建出哪个窗口）
     * @param title 自定义标题（空则用默认标题）
     */
    void _announceContainerOpened(PlayerId playerId,
        mc::ContainerId containerId,
        mc::ContainerType type,
        const std::string& title = std::string());

    core::PlayerManager& m_playerManager;
    InventoryManager* m_inventoryManager = nullptr;

    struct OpenContainer {
        std::unique_ptr<AbstractContainerMenu> menu;
        std::shared_ptr<IInventory> inventoryOwner;
        mc::ContainerType type = mc::ContainerType::Player;
        BlockPos position;
        /// 自上次下发全量内容以来，菜单槽位是否被服务端自己改动过（熔炉烧出产物、
        /// 燃料被消耗等）。由 tickMenus 消费：置位则重发一次全量并清零。
        bool slotChangePending = false;
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