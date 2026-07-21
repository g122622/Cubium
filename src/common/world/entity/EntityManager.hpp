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
#include "common/entity/core/EntityClassification.hpp"
#include "common/entity/registry/VanillaEntityTypeKeys.hpp"
#include "common/util/AxisAlignedBB.hpp"
#include "common/util/math/Vector3.hpp"
#include <functional>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>

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
    EntityInstanceId addEntity(std::unique_ptr<Entity> entity);

    /**
     * @brief 移除实体
     * @param id 实体ID
     * @return 被移除的实体指针（调用者获得所有权），如果不存在返回nullptr
     */
    std::unique_ptr<Entity> removeEntity(EntityInstanceId id);

    /**
     * @brief 检查实体是否存在
     * @param id 实体ID
     */
    [[nodiscard]] bool hasEntity(EntityInstanceId id) const;

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
    [[nodiscard]] Entity* getEntity(EntityInstanceId id);
    [[nodiscard]] const Entity* getEntity(EntityInstanceId id) const;

    /**
     * @brief 通过UUID获取实体
     *
     * 利用内部 UUID 索引进行 O(1) 查找，避免全量遍历。
     *
     * @param uuid 实体UUID字符串
     * @return 实体指针，如果不存在返回nullptr
     */
    [[nodiscard]] Entity* getEntityByUuid(const std::string& uuid);
    [[nodiscard]] const Entity* getEntityByUuid(const std::string& uuid) const;

    /**
     * @brief 检查指定UUID的实体是否已存在
     * @param uuid 实体UUID字符串
     * @return 是否存在
     */
    [[nodiscard]] bool hasEntityWithUuid(const std::string& uuid) const;

    /**
     * @brief 获取碰撞箱内的所有实体
     * @param box 碰撞箱
     * @param except 排除的实体（可选）
     * @return 实体列表
     */
    [[nodiscard]] std::vector<Entity*> getEntitiesInAABB(
        const AxisAlignedBB& box, const Entity* except = nullptr) const;

    /**
     * @brief 获取范围内的所有实体
     * @param pos 中心位置
     * @param range 范围
     * @param except 排除的实体（可选）
     * @return 实体列表
     */
    [[nodiscard]] std::vector<Entity*> getEntitiesInRange(
        const Vector3& pos, f32 range, const Entity* except = nullptr) const;

    /**
     * @brief 获取指定类型的所有实体
     * @param typeId 实体类型字符串 (来自 EntityTypeKeys)
     * @return 实体列表
     */
    [[nodiscard]] std::vector<Entity*> getEntitiesByType(const std::string& typeId) const;

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
     *
     * ID 单调递增、永不复用。u64 空间实际不可能耗尽，不复用可避免：
     * 旧实体死亡后其 ID 被新实体复用，导致客户端缓存的旧 ClientEntity
     * （typeId 不可变、网格按 ID 缓存）被错误地套用到新实体上，渲染成
     * 旧类型（如掉落物/下落方块显示成猪、僵尸马）。同时 EntityTracker
     * 按 ID 追踪，ID 复用会让追踪器把新实体误判为"还活着的旧实体"，
     * 从而不发旧实体的 destroy 包。
     *
     * @return 新的实体ID
     */
    EntityInstanceId allocateId();

private:
    // 实体 tick/回调中可能重入查询接口，需允许同线程递归加锁。
    mutable std::recursive_mutex m_mutex;
    std::unordered_map<EntityInstanceId, std::unique_ptr<Entity>> m_entities;
    std::unordered_map<std::string, Entity*> m_uuidToEntity; // UUID 到实体的索引

    // 延迟析构队列：本 tick 移除的实体先暂存于此，下一 tick 末尾（entity tick 之后）再析构。
    // 目的：给持有裸实体指针的 goal 一帧时间通过 isAlive() 检查并 reset 指针，
    // 避免 use-after-free（LookAtGoal::shouldContinueExecuting 等解引用已被 erase 析构的目标）。
    std::vector<std::unique_ptr<Entity>> m_graveyard;

    EntityInstanceId m_nextId = 1;

    // 内部方法（假设已持有锁）
    void _removeDeadEntitiesInternal();
};

} // namespace mc
