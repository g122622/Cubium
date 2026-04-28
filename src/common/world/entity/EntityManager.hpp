#pragma once

#include "../../core/Types.hpp"
#include "../../util/math/Vector3.hpp"
#include "../../util/AxisAlignedBB.hpp"
#include "../../entity/core/Entity.hpp"
#include "../../entity/core/EntityClassification.hpp"
#include <memory>
#include <unordered_map>
#include <vector>
#include <functional>
#include <mutex>

namespace mc {

/**
 * @brief 实体管理器
 *
 * 负责管理世界中的所有实体，包括：
 * - 实体创建和销毁
 * - 实体ID分配
 * - 实体查询（按ID、按范围、按碰撞箱）
 * - 实体更新循环
 *
 * 线程安全：所有公共方法都是线程安全的。
 *
 * 参考 MC 1.16.5 World (实体管理部分)
 */
class EntityManager {
public:
    EntityManager();
    ~EntityManager() = default;

    // 禁止拷贝
    EntityManager(const EntityManager&) = delete;
    EntityManager& operator=(const EntityManager&) = delete;

    // ========== 实体创建和销毁 ==========

    /**
     * @brief 添加实体到管理器
     * @param entity 实体指针（管理器获得所有权）
     * @return 实体ID
     *
     * 如果实体ID为0，将自动分配新ID
     */
    EntityId addEntity(std::unique_ptr<Entity> entity);

    /**
     * @brief 移除实体
     * @param id 实体ID
     * @return 被移除的实体指针（调用者获得所有权），如果不存在返回nullptr
     */
    std::unique_ptr<Entity> removeEntity(EntityId id);

    /**
     * @brief 检查实体是否存在
     * @param id 实体ID
     */
    [[nodiscard]] bool hasEntity(EntityId id) const;

    /**
     * @brief 获取实体数量
     */
    [[nodiscard]] size_t entityCount() const;

    // ========== 实体查询 ==========

    /**
     * @brief 通过ID获取实体
     * @param id 实体ID
     * @return 实体指针，如果不存在返回nullptr
     */
    [[nodiscard]] Entity* getEntity(EntityId id);
    [[nodiscard]] const Entity* getEntity(EntityId id) const;

    /**
     * @brief 获取碰撞箱内的所有实体
     * @param box 碰撞箱
     * @param except 排除的实体（可选）
     * @return 实体列表
     */
    [[nodiscard]] std::vector<Entity*> getEntitiesInAABB(
        const AxisAlignedBB& box,
        const Entity* except = nullptr) const;

    /**
     * @brief 获取范围内的所有实体
     * @param pos 中心位置
     * @param range 范围
     * @param except 排除的实体（可选）
     * @return 实体列表
     */
    [[nodiscard]] std::vector<Entity*> getEntitiesInRange(
        const Vector3& pos,
        f32 range,
        const Entity* except = nullptr) const;

    /**
     * @brief 获取指定类型的所有实体
     * @param type 实体类型
     * @return 实体列表
     */
    [[nodiscard]] std::vector<Entity*> getEntitiesByType(LegacyEntityType type) const;

    /**
     * @brief 按分类统计实体数量
     * @return 各分类的实体数量映射
     */
    [[nodiscard]] std::unordered_map<entity::EntityClassification, i32> countEntitiesByClassification() const;

    /**
     * @brief 获取指定分类的实体数量
     * @param classification 实体分类
     * @return 该分类的实体数量
     */
    [[nodiscard]] i32 getCountByClassification(entity::EntityClassification classification) const;

    /**
     * @brief 获取所有玩家实体
     * @return 玩家实体列表
     */
    [[nodiscard]] std::vector<Entity*> getPlayers() const;

    /**
     * @brief 遍历所有实体
     * @param callback 回调函数，返回false停止遍历
     */
    void forEachEntity(const std::function<bool(Entity*)>& callback);
    void forEachEntity(const std::function<bool(const Entity*)>& callback) const;

    // ========== 更新 ==========

    /**
     * @brief 更新所有实体
     *
     * 调用每个实体的tick()方法，并移除已标记为移除的实体
     */
    void tick();

    /**
     * @brief 移除所有已标记为移除的实体
     */
    void removeDeadEntities();

    // ========== ID分配 ==========

    /**
     * @brief 分配新的实体ID
     * @return 新的实体ID
     */
    EntityId allocateId();

    /**
     * @brief 释放实体ID（供重用）
     * @param id 要释放的ID
     */
    void releaseId(EntityId id);

private:
    // 实体 tick/回调中可能重入查询接口，需允许同线程递归加锁。
    mutable std::recursive_mutex m_mutex;
    std::unordered_map<EntityId, std::unique_ptr<Entity>> m_entities;
    EntityId m_nextId = 1;
    std::vector<EntityId> m_freeIds;  // 可重用的ID池

    // 内部方法（假设已持有锁）
    void removeDeadEntitiesInternal();
};

} // namespace mc
