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

#include "../../item/crafting/RecipeManager.hpp"
#include "InventoryPackets.hpp"
#include "common/entity/inventory/ContainerTypeUtils.hpp"
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
    static OpenContainerPacket createOpenContainerPacket(ContainerId containerId, i32 type, const std::string& title);
};

} // namespace mc
