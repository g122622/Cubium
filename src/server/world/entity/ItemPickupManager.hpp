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

#include "common/core/Types.hpp"
#include <vector>

namespace mc {

// 前向声明
class Entity;
class ItemEntity;
class Player;
class ItemStack;
class AxisAlignedBB;

namespace server {

// 前向声明
class IServer;

/**
 * @brief 物品拾取管理器
 *
 * 每tick检测玩家附近的ItemEntity，处理拾取逻辑。
 *
 * 特性:
 * - 检测玩家附近的掉落物
 * - 处理拾取延迟和所有者限制
 * - 合并相同物品
 * - 发送背包更新和实体销毁包
 *
 * 参考 MC 1.16.5 EntityItem.onCollideWithPlayer
 */
class ItemPickupManager {
public:
    // ========== 常量 ==========

    /// 基础拾取范围（方块）
    static constexpr f32 PICKUP_RANGE = 1.0f;

    /// 拾取范围扩展（当物品向玩家移动时）
    static constexpr f32 PICKUP_RANGE_EXTENDED = 1.5f;

    /// 创造模式拾取范围（与普通相同）
    static constexpr f32 PICKUP_RANGE_CREATIVE = 1.0f;

    /// 潜行时拾取范围缩小
    static constexpr f32 PICKUP_RANGE_SNEAKING = 0.5f;

    /// 物品合并检测范围
    static constexpr f32 MERGE_RANGE = 0.5f;

    /// 拾取延迟（ticks）- 刚丢弃的物品不能立即被拾取
    static constexpr i32 DEFAULT_THROWER_PICKUP_DELAY = 10;

    /// 物品合并延迟（ticks）
    static constexpr i32 MERGE_DELAY = 20;

    // ========== 构造函数 ==========

    ItemPickupManager() = default;
    ~ItemPickupManager() = default;

    // 禁止拷贝
    ItemPickupManager(const ItemPickupManager&) = delete;
    ItemPickupManager& operator=(const ItemPickupManager&) = delete;

    // ========== 拾取处理 ==========

    /**
     * @brief 执行拾取检测
     *
     * 检查所有玩家附近的ItemEntity，触发拾取。
     * 应在每tick调用。
     *
     * @param server 服务器接口
     */
    void tick(IServer& server);

    /**
     * @brief 检查单个玩家的拾取
     *
     * @param server 服务器接口
     * @param player 玩家实体
     */
    void checkPlayerPickup(IServer& server, Entity& player);

    /**
     * @brief 尝试拾取物品
     *
     * @param server 服务器接口
     * @param player 玩家实体
     * @param itemEntity 物品实体
     * @return true 如果物品被完全拾取（实体应被移除）
     */
    bool tryPickupItem(IServer& server, Entity& player, ItemEntity& itemEntity);

    // ========== 物品合并 ==========

    /**
     * @brief 处理物品实体合并
     *
     * 检查附近的ItemEntity，合并相同物品。
     *
     * @param server 服务器接口
     */
    void processItemMerging(IServer& server);

private:
    /**
     * @brief 计算玩家的实际拾取范围
     * @param player 玩家实体
     * @return 拾取范围
     */
    [[nodiscard]] f32 calculatePickupRange(const Entity& player) const;

    /**
     * @brief 检查玩家是否可以拾取物品
     *
     * 检查:
     * - 拾取延迟
     * - 所有者限制
     * - 游戏模式限制
     *
     * @param player 玩家实体
     * @param itemEntity 物品实体
     * @return true 如果可以拾取
     */
    [[nodiscard]] bool canPickup(const Entity& player, const ItemEntity& itemEntity) const;

    /**
     * @brief 发送背包更新给客户端
     *
     * @param server 服务器接口
     * @param player 玩家实体
     */
    void sendInventoryUpdate(IServer& server, Player& player);

    /**
     * @brief 通过 EntityTracker 的统一重同步路径刷新物品实体状态。
     */
    void sendItemEntityUpdate(IServer& server, const ItemEntity& itemEntity);

    /**
     * @brief 发送物品拾取动画包
     *
     * @param server 服务器接口
     * @param entityId 实体ID
     * @param collectorId 拾取者实体ID
     */
    void sendCollectItem(IServer& server, EntityId entityId, EntityId collectorId, i32 pickupItemCount);
};

} // namespace server
} // namespace mc
