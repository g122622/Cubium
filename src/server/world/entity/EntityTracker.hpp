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
#include "common/entity/core/Entity.hpp"
#include "common/util/math/Vector3.hpp"
#include <mutex>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace mc::server {

// 前向声明
class IServer;

/**
 * @brief 被追踪的实体信息
 *
 * 存储单个实体的追踪状态，包括哪些玩家正在追踪它。
 */
struct TrackedEntity {
    EntityId entityId;
    std::unordered_set<PlayerId> trackingPlayers; // 正在追踪此实体的玩家
    Vector3 lastPosition;                         // 上次同步的位置
    f32 lastYaw = 0.0f;                           // 上次同步的偏航角
    f32 lastPitch = 0.0f;                         // 上次同步的俯仰角
    u32 updateCounter = 0;                        // 更新计数器
    bool needsFullUpdate = true;                  // 是否需要完整更新
};

/**
 * @brief 实体追踪器
 *
 * 负责管理实体的客户端可见性：
 * - 确定哪些玩家应该看到哪些实体
 * - 发送实体生成/销毁/更新包
 * - 基于距离和视距进行追踪范围计算
 *
 * 参考 MC 1.16.5 EntityTracker
 */
class EntityTracker {
public:
    EntityTracker();
    ~EntityTracker() = default;

    // 禁止拷贝
    EntityTracker(const EntityTracker&) = delete;
    EntityTracker& operator=(const EntityTracker&) = delete;

    // ========== 实体追踪 ==========

    /**
     * @brief 开始追踪一个实体
     * @param entity 要追踪的实体
     */
    void trackEntity(Entity* entity);

    /**
     * @brief 停止追踪一个实体
     * @param entityId 实体ID
     */
    void untrackEntity(EntityId entityId);

    /**
     * @brief 检查实体是否正在被追踪
     * @param entityId 实体ID
     */
    [[nodiscard]] bool isTracking(EntityId entityId) const;

    /**
     * @brief 获取被追踪的实体数量
     */
    [[nodiscard]] size_t trackedEntityCount() const;

    // ========== 玩家追踪 ==========

    /**
     * @brief 更新玩家的追踪状态
     *
     * 根据玩家位置更新应该追踪的实体列表。
     * 应在玩家移动时调用。
     *
     * @param server 服务器接口
     * @param playerId 玩家ID
     * @param playerPos 玩家位置
     */
    void updatePlayerTracking(IServer& server, PlayerId playerId, const Vector3& playerPos);

    /**
     * @brief 移除玩家的所有追踪
     * @param playerId 玩家ID
     */
    void removePlayer(PlayerId playerId);

    /**
     * @brief 获取玩家正在追踪的实体列表
     * @param playerId 玩家ID
     * @return 实体ID列表
     */
    [[nodiscard]] std::vector<EntityId> getPlayerTrackedEntities(PlayerId playerId) const;

    // ========== 更新 ==========

    /**
     * @brief 每tick更新
     *
     * 检查所有追踪实体的位置变化，发送更新包。
     *
     * @param server 服务器接口
     */
    void tick(IServer& server);

    // ========== 配置 ==========

    /**
     * @brief 设置实体追踪距离
     * @param chunks 区块数
     */
    void setTrackingDistance(i32 chunks) { m_trackingDistance = chunks; }

    /**
     * @brief 获取实体追踪距离
     */
    [[nodiscard]] i32 trackingDistance() const { return m_trackingDistance; }

private:
    /**
     * @brief 检查玩家是否应该追踪实体
     * @param playerPos 玩家位置
     * @param entityPos 实体位置
     * @param trackingRange 实体的追踪范围（区块）
     * @return 是否应该追踪
     */
    [[nodiscard]] bool shouldTrack(const Vector3& playerPos, const Vector3& entityPos, i32 trackingRange) const;

    /**
     * @brief 发送实体生成包给玩家
     * @param server 服务器接口
     * @param playerId 玩家ID
     * @param entity 实体
     */
    void sendSpawnPacket(IServer& server, PlayerId playerId, Entity* entity);

    /**
     * @brief 发送实体销毁包给玩家
     * @param server 服务器接口
     * @param playerId 玩家ID
     * @param entityId 实体ID
     */
    void sendDestroyPacket(IServer& server, PlayerId playerId, EntityId entityId);

    /**
     * @brief 发送实体移动包给玩家
     * @param server 服务器接口
     * @param playerId 玩家ID
     * @param entity 实体
     */
    void sendMovePacket(IServer& server, PlayerId playerId, Entity* entity);

    /**
     * @brief 发送实体元数据包给玩家
     */
    void sendMetadataPacket(IServer& server, PlayerId playerId, Entity* entity, const std::vector<u8>& metadata);

private:
    mutable std::mutex m_mutex;

    /// 被追踪的实体 (entityId -> TrackedEntity)
    std::unordered_map<EntityId, TrackedEntity> m_trackedEntities;

    /// 每个玩家追踪的实体集合 (playerId -> entityIds)
    std::unordered_map<PlayerId, std::unordered_set<EntityId>> m_playerTrackedEntities;

    /// 追踪距离（区块）
    i32 m_trackingDistance = 10;

    /// 位置更新阈值（方块）
    f32 m_positionUpdateThreshold = 0.1f;

    /// 旋转更新阈值（度）
    f32 m_rotationUpdateThreshold = 1.0f;
};

} // namespace mc::server
