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

#include "ClientEntity.hpp"
#include "common/core/Types.hpp"
#include <cstddef>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace mc::client {

/**
 * @brief 客户端实体管理器
 *
 * 管理客户端所有实体的创建、更新、销毁。
 * 提供实体查询和迭代功能。
 *
 * ## 本地玩家支持
 *
 * 本地玩家实体也被纳入此管理器，与其他远程实体统一管理。
 * 通过 isLocalPlayer() 可以判断某个实体是否是本地玩家。
 */
class ClientEntityManager {
public:
    ClientEntityManager() = default;
    ~ClientEntityManager() = default;

    // 禁止拷贝
    ClientEntityManager(const ClientEntityManager&) = delete;
    ClientEntityManager& operator=(const ClientEntityManager&) = delete;

    // 允许移动
    ClientEntityManager(ClientEntityManager&&) noexcept = default;
    ClientEntityManager& operator=(ClientEntityManager&&) noexcept = default;

    // ========== 实体管理 ==========

    /**
     * @brief 创建实体
     * @param id 实体ID
     * @param typeId 实体类型标识符
     * @return 创建的实体指针，如果ID已存在则返回nullptr
     */
    [[nodiscard]] ClientEntity* spawnEntity(EntityInstanceId id, const std::string& typeId);

    /**
     * @brief 创建本地玩家实体
     *
     * 本地玩家实体与其他实体一样被管理，但会标记为本地玩家。
     * 用于正确处理网络包中的实体位置更新（本地玩家使用预测系统）。
     *
     * @param entityId 服务端分配的实体ID
     * @param playerId 玩家ID
     * @param username 用户名
     * @return 创建的实体指针
     */
    [[nodiscard]] ClientEntity* spawnLocalPlayer(
        EntityInstanceId entityId, PlayerId playerId, const std::string& username);

    /**
     * @brief 移除实体
     * @param id 实体ID
     * @return 是否成功移除
     *
     * 注意：不能移除本地玩家实体。如果尝试移除本地玩家，返回 false。
     */
    bool removeEntity(EntityInstanceId id);

    /**
     * @brief 获取实体
     * @param id 实体ID
     * @return 实体指针，如果不存在则返回nullptr
     */
    [[nodiscard]] ClientEntity* getEntity(EntityInstanceId id);
    [[nodiscard]] const ClientEntity* getEntity(EntityInstanceId id) const;

    /**
     * @brief 检查实体是否存在
     */
    [[nodiscard]] bool hasEntity(EntityInstanceId id) const;

    /**
     * @brief 移除所有实体（不包括本地玩家）
     */
    void clear();

    /**
     * @brief 移除所有已标记为移除的实体
     */
    void removeDeadEntities();

    // ========== 本地玩家 ==========

    /**
     * @brief 获取本地玩家实体
     * @return 本地玩家实体指针，未设置返回 nullptr
     */
    [[nodiscard]] ClientEntity* localPlayer();
    [[nodiscard]] const ClientEntity* localPlayer() const;

    /**
     * @brief 获取本地玩家的实体ID
     * @return 实体ID，未设置返回 INVALID_ENTITY_ID
     */
    [[nodiscard]] EntityInstanceId localPlayerEntityId() const { return m_localPlayerEntityId; }

    /**
     * @brief 获取本地玩家的玩家ID
     * @return 玩家ID，未设置返回 0
     */
    [[nodiscard]] PlayerId localPlayerId() const { return m_localPlayerId; }

    /**
     * @brief 检查实体是否是本地玩家
     * @param entityId 实体ID
     * @return true 如果是本地玩家
     */
    [[nodiscard]] bool isLocalPlayer(EntityInstanceId entityId) const;

    /**
     * @brief 检查是否已设置本地玩家
     */
    [[nodiscard]] bool hasLocalPlayer() const { return m_localPlayerEntityId != INVALID_ENTITY_ID; }

    /**
     * @brief 清除本地玩家（登出时调用）
     */
    void clearLocalPlayer();

    // ========== 实体查询 ==========

    /**
     * @brief 获取实体数量（不包括本地玩家）
     */
    [[nodiscard]] size_t entityCount() const;

    /**
     * @brief 遍历所有实体（包括本地玩家）
     * @param func 回调函数
     */
    void forEachEntity(std::function<void(ClientEntity&)> func);

    /**
     * @brief 遍历所有实体（const版本）
     * @param func 回调函数
     */
    void forEachEntity(std::function<void(const ClientEntity&)> func) const;

    /**
     * @brief 遍历远程实体（不包括本地玩家）
     * @param func 回调函数
     */
    void forEachRemoteEntity(std::function<void(ClientEntity&)> func);

    /**
     * @brief 获取指定类型的所有实体
     * @param typeId 实体类型标识符
     * @return 实体ID列表
     */
    [[nodiscard]] std::vector<EntityInstanceId> getEntitiesByType(const std::string& typeId) const;

    /**
     * @brief 获取指定范围内的实体
     * @param x 中心X
     * @param y 中心Y
     * @param z 中心Z
     * @param radius 半径
     * @return 范围内的实体ID列表
     */
    [[nodiscard]] std::vector<EntityInstanceId> getEntitiesInRange(f32 x, f32 y, f32 z, f32 radius) const;

    // ========== 更新 ==========

    /**
     * @brief 更新所有实体（每tick调用）
     */
    void tick();

    /**
     * @brief 固定频率更新所有实体
     *
     * 使用累积器实现固定频率的 tick 更新，确保实体逻辑以 20 TPS 运行。
     * 每帧调用此方法，传入帧时间，它会自动调用 tick() 零次或多次。
     *
     * @param deltaTime 帧时间（秒）
     * @return 本次调用执行的 tick 次数
     */
    u32 fixedTick(f32 deltaTime);

    /**
     * @brief 获取 tick 累积器当前值
     *
     * 用于计算 partialTick，表示当前帧在两个 tick 之间的位置。
     *
     * @return 累积器值（0.0 到 TICK_INTERVAL）
     */
    [[nodiscard]] f32 tickAccumulator() const { return m_tickAccumulator; }

    /**
     * @brief 获取 tick 间隔常量
     *
     * 用于外部代码计算 partialTick。
     */
    [[nodiscard]] static constexpr f32 tickInterval() { return TICK_INTERVAL; }

    /**
     * @brief 更新所有实体的平滑插值（每帧调用）
     * @param deltaTime 帧时间（秒）
     */
    void updateInterpolation(f32 deltaTime);

    /**
     * @brief 更新所有实体的动画状态
     * 在渲染前调用，用于插值计算
     * @param partialTick 部分 tick (0.0-1.0)
     */
    void updateAnimations(f32 partialTick);

private:
    // 实体存储
    std::unordered_map<EntityInstanceId, std::unique_ptr<ClientEntity>> m_entities;

    // 待移除的实体列表
    std::vector<EntityInstanceId> m_entitiesToRemove;

    // 本地玩家信息
    EntityInstanceId m_localPlayerEntityId = INVALID_ENTITY_ID;
    PlayerId m_localPlayerId = 0;

    // 固定频率 tick 累积器
    f32 m_tickAccumulator = 0.0f;
    static constexpr f32 TICK_INTERVAL = 1.0f / 20.0f; // 20 TPS
    static constexpr u32 MAX_TICKS_PER_FRAME = 5;      // 每帧最大 tick 数，防止螺旋死亡
};

} // namespace mc::client
