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
#include "common/util/math/Vector3.hpp"
#include "common/world/entity/EntityManager.hpp"
#include <functional>
#include <memory>
#include <unordered_map>

namespace mc::server {
namespace sync {

/**
 * @brief 实体同步管理器
 *
 * 负责实体位置的客户端同步：
 * - 追踪实体位置变化
 * - 发送实体生成/移动/销毁包
 * - 多玩家可见性管理
 *
 * 网络发送通过回调实现，由 MinecraftServer 设置。
 */
class EntitySyncManager {
public:
    /**
     * @brief 构造函数
     * @param entityManager 实体管理器引用
     */
    explicit EntitySyncManager(EntityManager& entityManager);

    /**
     * @brief 每 tick 同步实体位置
     */
    void tick();

    /**
     * @brief 生成新实体并通知客户端
     * @param entity 实体指针（所有权转移）
     * @return 实体ID
     */
    EntityInstanceId spawnEntity(std::unique_ptr<Entity> entity);

    /**
     * @brief 移除实体并通知客户端
     * @param entityId 实体ID
     * @return 被移除的实体指针
     */
    std::unique_ptr<Entity> removeEntity(EntityInstanceId entityId);

    /**
     * @brief 强制发送完整更新
     * @param entityId 实体ID
     */
    void forceFullUpdate(EntityInstanceId entityId);

    /**
     * @brief 设置实体生成回调
     */
    void setOnEntitySpawn(std::function<void(EntityInstanceId, const Entity&)> callback);

    /**
     * @brief 设置实体移除回调
     */
    void setOnEntityRemove(std::function<void(EntityInstanceId)> callback);

    /**
     * @brief 设置实体移动回调
     */
    void setOnEntityMove(std::function<void(EntityInstanceId, const Vector3&, f32, f32)> callback);

    /**
     * @brief 设置实体状态回调
     * @param callback 回调函数 (entityId, status)
     */
    void setOnEntityStatus(std::function<void(EntityInstanceId, u8)> callback);

    /**
     * @brief 广播实体状态
     * @param entityId 实体ID
     * @param status 状态码
     */
    void broadcastEntityStatus(EntityInstanceId entityId, u8 status);

private:
    /**
     * @brief 检查实体是否需要同步
     */
    [[nodiscard]] bool _needsSync(EntityInstanceId entityId) const;

    /**
     * @brief 广播实体移动
     */
    void _broadcastEntityMove(EntityInstanceId entityId, const Vector3& pos, f32 yaw, f32 pitch);

    /**
     * @brief 广播实体生成
     */
    void _broadcastEntitySpawn(EntityInstanceId entityId, const Entity& entity);

    /**
     * @brief 广播实体移除
     */
    void _broadcastEntityRemove(EntityInstanceId entityId);

private:
    /**
     * @brief 实体追踪数据
     */
    struct EntityTrackData {
        Vector3 lastPosition;
        f32 lastYaw = 0.0f;
        f32 lastPitch = 0.0f;
        bool needsFullUpdate = true;
    };

    static constexpr f32 POSITION_THRESHOLD = 0.01f;
    static constexpr f32 ROTATION_THRESHOLD = 1.0f;

    EntityManager& m_entityManager;

    std::unordered_map<EntityInstanceId, EntityTrackData> m_entityTrackData;

    std::function<void(EntityInstanceId, const Entity&)> m_onEntitySpawn;
    std::function<void(EntityInstanceId)> m_onEntityRemove;
    std::function<void(EntityInstanceId, const Vector3&, f32, f32)> m_onEntityMove;
    std::function<void(EntityInstanceId, u8)> m_onEntityStatus;
};

} // namespace sync
} // namespace mc::server