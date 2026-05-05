#pragma once

#include "RecipePackets.hpp"
#include "InventoryPackets.hpp"
#include "../../item/crafting/RecipeManager.hpp"
#include <functional>
#include <memory>

namespace mc {

// Forward declarations
class Player;
class PlayerInventory;
class AbstractContainerMenu;

/**
 * @brief 容器网络处理器
 *
 * 处理容器相关的网络包，包括：
 * - 槽位更新
 * - 容器内容同步
 * - 点击操作
 * - 配方同步
 *
 * 参考: MC 1.16.5 容器网络处理
 */
class ContainerPacketHandler {
public:
    /**
     * @brief 处理容器点击包（服务端）
     * @param player 玩家
     * @param packet 点击包
     * @return 处理成功返回true
     */
    static bool handleContainerClick(Player& player, const ContainerClickPacket& packet);

    /**
     * @brief 处理关闭容器包（服务端）
     * @param player 玩家
     * @param packet 关闭包
     */
    static void handleCloseContainer(Player& player, const CloseContainerPacket& packet);

    /**
     * @brief 处理快捷栏选择包（服务端）
     * @param player 玩家
     * @param packet 选择包
     */
    static void handleHotbarSelect(Player& player, const HotbarSelectPacket& packet);

    /**
     * @brief 创建容器内容同步包
     * @param menu 容器菜单
     * @return 同步包
     */
    static ContainerContentPacket createContentPacket(const AbstractContainerMenu& menu);

    /**
     * @brief 创建槽位更新包
     * @param menu 容器菜单
     * @param slotIndex 槽位索引
     * @return 更新包
     */
    static ContainerSlotPacket createSlotPacket(const AbstractContainerMenu& menu, i32 slotIndex);

    /**
     * @brief 创建打开容器包
     * @param containerId 容器ID
     * @param type 容器类型
     * @param title 标题
     * @return 打开包
     */
    static OpenContainerPacket createOpenContainerPacket(ContainerId containerId, i32 type,
                                                          const String& title);

    /**
     * @brief 创建配方列表同步包
     * @return 配方列表包
     */
    static RecipeListSyncPacket createRecipeListPacket();

    /**
     * @brief 创建合成结果预览包
     * @param containerId 容器ID
     * @param menu 合成菜单
     * @return 预览包
     */
    static CraftResultPreviewPacket createCraftResultPreview(ContainerId containerId,
                                                              const AbstractContainerMenu& menu);
};

/**
 * @brief 容器类型工具函数
 */
namespace ContainerTypes {

/**
 * @brief 获取容器类型的槽位数
 * @param type 容器类型
 * @return 槽位数
 */
[[nodiscard]] i32 getSlotCount(ContainerType type);

/**
 * @brief 获取容器类型的标题
 * @param type 容器类型
 * @return 默认标题
 */
[[nodiscard]] const char* getDefaultTitle(ContainerType type);

/**
 * @brief 将容器类型转换为网络传输的u8值
 * @param type 容器类型
 * @return 网络值
 */
[[nodiscard]] u8 toNetworkType(ContainerType type);

/**
 * @brief 将网络点击动作转换为菜单点击类型
 * @param action 网络点击动作
 * @param button 鼠标按钮或快捷栏索引
 * @return 对应的菜单点击类型
 */
[[nodiscard]] ClickType toClickType(ClickAction action, i32 button);

/**
 * @brief 将菜单点击类型转换为网络点击动作
 * @param clickType 菜单点击类型
 * @return 对应的网络点击动作
 */
[[nodiscard]] ClickAction toClickAction(ClickType clickType);

} // namespace ContainerTypes

} // namespace mc
