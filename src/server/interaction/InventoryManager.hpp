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
#include "common/entity/inventory/PlayerInventory.hpp"
#include "common/item/core/ItemStack.hpp"
#include <functional>
#include <unordered_map>

namespace mc::server {

namespace core {
class PlayerManager;
class ConnectionManager;
} // namespace core

namespace interaction {

/**
 * @brief 物品栏管理器
 *
 * 管理玩家的物品栏：
 * - 物品栏同步到客户端
 * - 手持物品槽位
 * - 物品栏操作
 *
 * 网络发送通过回调实现，由 MinecraftServer 设置。
 */
class InventoryManager {
public:
    /**
     * @brief 构造函数
     * @param playerManager 玩家管理器引用
     */
    explicit InventoryManager(core::PlayerManager& playerManager);

    /// 析构函数
    ~InventoryManager() = default;

    // 禁止拷贝
    InventoryManager(const InventoryManager&) = delete;
    InventoryManager& operator=(const InventoryManager&) = delete;

    // 允许移动
    InventoryManager(InventoryManager&&) noexcept = default;
    InventoryManager& operator=(InventoryManager&&) noexcept = default;

    /**
     * @brief 获取玩家物品栏
     * @param playerId 玩家ID
     * @return 物品栏指针，如果玩家不存在则返回 nullptr
     */
    [[nodiscard]] PlayerInventory* getInventory(PlayerId playerId);
    [[nodiscard]] const PlayerInventory* getInventory(PlayerId playerId) const;

    /**
     * @brief 设置选中槽位
     * @param playerId 玩家ID
     * @param slot 槽位索引 (0-8)
     */
    void setSelectedSlot(PlayerId playerId, i32 slot);

    /**
     * @brief 获取选中槽位
     * @param playerId 玩家ID
     * @return 槽位索引，如果玩家不存在则返回 -1
     */
    [[nodiscard]] i32 getSelectedSlot(PlayerId playerId) const;

    /**
     * @brief 获取手持物品
     * @param playerId 玩家ID
     * @return 物品栈副本，如果不存在则返回空栈
     */
    [[nodiscard]] ItemStack getHeldItem(PlayerId playerId) const;

    /**
     * @brief 设置槽位物品
     * @param playerId 玩家ID
     * @param slot 槽位索引
     * @param item 物品栈
     */
    void setItem(PlayerId playerId, i32 slot, const ItemStack& item);

    /**
     * @brief 同步物品栏到客户端
     * @param playerId 玩家ID
     */
    void syncToClient(PlayerId playerId);

    /**
     * @brief 同步所有物品栏到客户端
     */
    void syncAllToClient();

    /**
     * @brief 初始化玩家物品栏
     * @param playerId 玩家ID
     */
    void initializeInventory(PlayerId playerId);

    /**
     * @brief 清理玩家物品栏数据
     * @param playerId 玩家ID
     */
    void cleanupInventory(PlayerId playerId);

    /**
     * @brief 设置物品栏更新回调
     */
    void setOnInventoryUpdate(std::function<void(PlayerId, const PlayerInventory&)> callback);

private:
    core::PlayerManager& m_playerManager;

    std::unordered_map<PlayerId, PlayerInventory> m_inventories;

    std::function<void(PlayerId, const PlayerInventory&)> m_onInventoryUpdate;
};

} // namespace interaction
} // namespace mc::server