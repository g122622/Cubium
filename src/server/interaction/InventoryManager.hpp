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
 * 提供按 playerId 访问玩家物品栏的入口，以及「把物品栏推送给客户端」的触发点。
 *
 * **本类不持有物品栏数据**。玩家的物品栏只有一份，在玩家实体上
 * （`Player::m_inventory`，经 `Player::inventory()` 访问）——拾取、丢弃、装备、
 * 吃东西、存盘以及所有游戏逻辑都直接操作它，它是唯一权威源。
 *
 * 本类曾持有一份按值副本，于是同一玩家的背包在内存中存在两份、且互不感知：
 * 拾取走实体层、容器点击后的同步读副本，任何一侧的改动都会被另一侧的下一次
 * 同步覆盖（表现之一是拾取到的物品被抹掉）。两份数据无法靠补写回点维持一致，
 * 故副本已整体移除——凡需要物品栏之处，一律经本类定位到实体上的那一份。
 *
 * 定位实体需要世界上下文（按 playerId 找到其所在世界，再取实体），而本类位于
 * 交互层、不持有世界，故由服务器在初始化时注入解析器。
 */
class InventoryManager {
public:
    /**
     * @brief 构造函数
     * @param playerManager 玩家管理器引用（用于遍历在线玩家）
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
     * @brief 设置物品栏解析器
     *
     * 解析器负责「按 playerId 找到其玩家实体上的物品栏」，由服务器注入——只有它
     * 知道该去哪个世界找实体。未设置时本类所有查询返回 nullptr（等价于该玩家没有
     * 物品栏），不会退化为任何内部副本。
     *
     * @param resolver 解析函数；玩家不存在、无实体或不在线时返回 nullptr
     */
    void setInventoryResolver(std::function<PlayerInventory*(PlayerId)> resolver);

    /**
     * @brief 获取玩家物品栏
     * @param playerId 玩家ID
     * @return 物品栏指针（指向玩家实体持有的那一份）；玩家不存在或无实体时返回 nullptr
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
     *
     * 槽位号是物品栏内部索引，覆盖快捷栏、主背包、护甲与副手（0-40）。
     *
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
     * @brief 同步所有在线玩家的物品栏到各自客户端
     */
    void syncAllToClient();

    /**
     * @brief 设置物品栏更新回调
     */
    void setOnInventoryUpdate(std::function<void(PlayerId, const PlayerInventory&)> callback);

private:
    core::PlayerManager& m_playerManager;

    std::function<PlayerInventory*(PlayerId)> m_resolveInventory;

    std::function<void(PlayerId, const PlayerInventory&)> m_onInventoryUpdate;
};

} // namespace interaction
} // namespace mc::server
