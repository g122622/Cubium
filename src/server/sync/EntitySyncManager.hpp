#pragma once

#include "common/core/Types.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/world/entity/EntityManager.hpp"
#include <unordered_map>
#include <functional>

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
    EntityId spawnEntity(std::unique_ptr<Entity> entity);

    /**
     * @brief 移除实体并通知客户端
     * @param entityId 实体ID
     * @return 被移除的实体指针
     */
    std::unique_ptr<Entity> removeEntity(EntityId entityId);

    /**
     * @brief 强制发送完整更新
     * @param entityId 实体ID
     */
    void forceFullUpdate(EntityId entityId);

    /**
     * @brief 设置实体生成回调
     */
    void setOnEntitySpawn(std::function<void(EntityId, const Entity&)> callback);

    /**
     * @brief 设置实体移除回调
     */
    void setOnEntityRemove(std::function<void(EntityId)> callback);

    /**
     * @brief 设置实体移动回调
     */
    void setOnEntityMove(std::function<void(EntityId, const Vector3&, f32, f32)> callback);

    /**
     * @brief 设置实体状态回调
     * @param callback 回调函数 (entityId, status)
     */
    void setOnEntityStatus(std::function<void(EntityId, u8)> callback);

    /**
     * @brief 广播实体状态
     * @param entityId 实体ID
     * @param status 状态码
     */
    void broadcastEntityStatus(EntityId entityId, u8 status);

private:
    /**
     * @brief 检查实体是否需要同步
     */
    [[nodiscard]] bool needsSync(EntityId entityId) const;

    /**
     * @brief 广播实体移动
     */
    void broadcastEntityMove(EntityId entityId, const Vector3& pos, f32 yaw, f32 pitch);

    /**
     * @brief 广播实体生成
     */
    void broadcastEntitySpawn(EntityId entityId, const Entity& entity);

    /**
     * @brief 广播实体移除
     */
    void broadcastEntityRemove(EntityId entityId);

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

    std::unordered_map<EntityId, EntityTrackData> m_entityTrackData;

    std::function<void(EntityId, const Entity&)> m_onEntitySpawn;
    std::function<void(EntityId)> m_onEntityRemove;
    std::function<void(EntityId, const Vector3&, f32, f32)> m_onEntityMove;
    std::function<void(EntityId, u8)> m_onEntityStatus;
};

} // namespace sync
} // namespace mc::server