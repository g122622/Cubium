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
#include "common/entity/ecs/context/EntityRegistry.hpp"
#include "common/entity/ecs/systems/EntitySystemScheduler.hpp"
#include "common/entity/registry/VanillaEntityTypeKeys.hpp"
#include "common/util/AxisAlignedBB.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/world/chunk/base/ChunkPos.hpp"
#include "common/world/entity/spatial/EntitySpatialIndex.hpp"
#include <cstddef>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
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
    /**
     * @brief 构造实体管理器
     *
     * @param registry 所属世界的 ECS 实体注册表引用。EntityManager 不拥有 registry，
     *   仅持有引用——registry 的所有权归所在 ServerWorld（每维度一个，三维度三 registry
     *   天然隔离）。Entity 工厂构造实体时经此 registry 在 ECS 层 create 实体并 attach
     *   高频组件；entt 实体不可跨 registry 迁移，故 registry 须与 EntityManager 同生命周期。
     */
    explicit EntityManager(ecs::EntityRegistry& registry);
    ~EntityManager() = default;

    // 禁止拷贝
    EntityManager(const EntityManager&) = delete;
    EntityManager& operator=(const EntityManager&) = delete;

    /**
     * @brief 获取所属 ECS 实体注册表
     *
     * 供 EntityType::create(world, registry) 等工厂调用点补 registry 实参：
     * `world->entityManager().registry()`。System/Scheduler 亦经此访问底层 entt registry。
     */
    [[nodiscard]] ecs::EntityRegistry& registry() noexcept { return m_registry; }
    [[nodiscard]] const ecs::EntityRegistry& registry() const noexcept { return m_registry; }

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
     *
     * 实体激活范围：仅 ServerPlayer 永远 tick；其余实体仅当其所在区块
     * 相对任一玩家的切比雪夫距离 <= 模拟距离时才 tick，否则冻结（AI/移动/碰撞停止），
     * 与原版 inEntityTickingRange 等价。冻结实体仍由 EntityTracker 同步给可见玩家。
     */
    void tick();

    /**
     * @brief 移除所有已标记为移除的实体
     */
    void removeDeadEntities();

    /**
     * @brief 实体位置变更通知（由 Entity::reapplyPosition 经反向指针调用）
     *
     * 假设已持有 m_mutex（Entity::reapplyPosition 在持锁上下文执行）。转调
     * `m_spatialIndex.onEntityPositionChanged`，实体跨 section 移动时迁移。
     * public 是因为 Entity 非本类友元需经反向指针调用；`_` 前缀表明仅内部使用。
     */
    void _onEntityPositionChanged(Entity& entity);

    // ========== 模拟距离 ==========

    /**
     * @brief 设置模拟距离（区块数）
     *
     * 控制实体激活范围：超出该距离的非玩家实体不 tick。>=32 时等价于全量 tick。
     */
    void setSimulationDistance(i32 distance) { m_simulationDistance = distance; }

    /**
     * @brief 获取模拟距离（区块数）
     */
    [[nodiscard]] i32 simulationDistance() const { return m_simulationDistance; }

    /**
     * @brief 获取空间索引（供 ServerWorld 区块卸载/关机保存取实体）
     *
     * `ServerWorld::onChunkUnloading`/`shutdown` 经此调
     * `getEntityIdsInChunkColumn` 替代已删除的 `EntityChunkTracker::getEntitiesInChunk`。
     */
    [[nodiscard]] EntitySpatialIndex& spatialIndex() noexcept { return m_spatialIndex; }
    [[nodiscard]] const EntitySpatialIndex& spatialIndex() const noexcept { return m_spatialIndex; }

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

    /**
     * @brief 请求延迟跨维度迁移（EntityTick 遍历期间安全）
     *
     * 当 `changeDimension` 在 `entity->tick()` 调用栈内被触发时（如 `doBlockCollisions`
     * → `EndPortalBlock::onEntityCollision` → `changeDimension`），源 EntityManager 的
     * `_tickEntities` 正持有当前迭代器遍历 `m_entities`。此时同步调 `removeEntity`
     * 会 erase 当前节点，for 循环 `++it` 解引用失效迭代器→SIGSEGV。
     *
     * 本方法把迁移回调入队 `m_pendingDimensionTransfers`，由 `tick()` 在
     * `_tickEntities` 遍历完成后（`m_scheduler.tick` 返回后）统一执行。回调内封装
     * `removeEntity` + `spawnEntity`，由调用方（`ServerPlayer::_performDimensionTransfer`）
     * 构造，故本类无需感知 `ServerWorld`（避免 common 层依赖 server 层）。
     *
     * @param transferAction 迁移回调（从源 EntityManager 取出实体并 spawn 到目标）
     */
    void requestDimensionTransfer(std::function<void()> transferAction);

private:
    // 所属 ECS 实体注册表（非拥有，引用 ServerWorld 持有的 m_entityRegistry）。
    ecs::EntityRegistry& m_registry;

    // ECS 系统调度器：按 SystemPhase 顺序执行注册的 ITickingSystem。
    // EntityTick 阶段跑 EntityLegacyTickSystem（包装 OOP Entity::tick()），
    // PostEntityTick 阶段跑状态递减/环境交互类 System（PortalTickSystem / FireTickSystem）。
    ecs::EntitySystemScheduler m_scheduler;

    // 实体 tick/回调中可能重入查询接口，需允许同线程递归加锁。
    mutable std::recursive_mutex m_mutex;
    std::unordered_map<EntityInstanceId, std::unique_ptr<Entity>> m_entities;
    std::unordered_map<std::string, Entity*> m_uuidToEntity; // UUID 到实体的索引

    // 延迟析构队列：本 tick 移除的实体先暂存于此，下一 tick 末尾（entity tick 之后）再析构。
    // 目的：给持有裸实体指针的 goal 一帧时间通过 isAlive() 检查并 reset 指针，
    // 避免 use-after-free（LookAtGoal::shouldContinueExecuting 等解引用已被 erase 析构的目标）。
    std::vector<std::unique_ptr<Entity>> m_graveyard;

    // 延迟跨维度迁移请求队列（本 EntityManager 为源）。changeDimension 在 entity->tick()
    // 调用栈内触发时，_tickEntities 正遍历 m_entities，同步 removeEntity 会 erase 当前节点
    // 致 for 循环 ++it 解引用失效迭代器→SIGSEGV。故 _performDimensionTransfer 改为把迁移
    // 回调入队此队列，tick() 在 _tickEntities 遍历完成后统一执行。
    std::vector<std::function<void()>> m_pendingDimensionTransfers;

    /**
     * @brief 处理延迟跨维度迁移队列
     *
     * 由 tick() 在 m_scheduler.tick 返回后调用。对每个待迁移实体：从源 EntityManager
     * removeEntity 取出 unique_ptr<Entity>，再向目标 EntityManager spawnEntity。
     * 遍历已结束，erase 安全。
     */
    void _processPendingDimensionTransfers();

    EntityInstanceId m_nextId = 1;

    // 模拟距离（区块数）：超出该距离的非玩家实体不 tick。默认值与 defaults::server::simulationDistance 一致。
    i32 m_simulationDistance = 10;

    // 3D section 空间索引：实体按 AABB 中心所在 section 分桶，所有空间/类型查询走索引。
    // mutable：const 查询方法内回调经 Entity→_onEntityPositionChanged 触发 section 迁移（逻辑 const）。
    mutable EntitySpatialIndex m_spatialIndex;

    // 内部方法（假设已持有锁）
    void _removeDeadEntitiesInternal();

    /**
     * @brief 逐实体 tick（EntityTick 阶段回调）
     *
     * 由 EntityLegacyTickSystem 回调委托，承载原 tick() 步骤1 的全部逻辑：
     * playerChunks 快照 + 遍历 m_entities 调 entity->tick() + 模拟距离门控 +
     * ServerPlayer 永远 tick + per-entity trace + isRemoved() 跳过。
     * 假设已持有 m_mutex（由 tick() 调用 scheduler 时传入）。
     */
    void _tickEntities();

    /**
     * @brief 逐 Brain 持有者 tick（PostEntityTick 阶段回调）
     *
     * 由 BrainTickSystem 回调委托，承载原 VillagerEntity::tick() 中 m_brain->tick()
     * 代码块的逻辑。复用 _tickEntities 的遍历+门控框架：playerChunks 快照 +
     * isRemoved() 跳过 + ServerPlayer 短路 + 模拟距离门控。对 dynamic_cast
     * <VillagerEntity*> 成功的实体调 brain().tick()（当前仅 VillagerEntity 持 Brain）。
     *
     * Brain 仍是 OOP 成员（VillagerEntity::m_brain），本方法只搬 tick 调度决策，
     * 不 ECS 化 Brain 数据（第19行决策"AI 保留 OOP，System 做 tick 调度"）。
     * 假设已持有 m_mutex（由 tick() 调用 scheduler 时传入）。
     */
    void _tickBrains();

    /**
     * @brief 判定实体是否处于任一玩家的模拟距离内（假设已持有锁）
     *
     * 对齐原版 inEntityTickingRange：实体所在区块相对任一玩家区块的切比雪夫距离
     * <= simulationDistance 即视为在范围内（多玩家取并集）。simulationDistance >= 32
     * 时短路返回 true（配置上限 32，等价全量 tick）。
     *
     * @param entity 待判定实体
     * @param playerChunks 循环外快照的玩家区块坐标集合
     * @return 是否在模拟距离内（在则 tick，否则冻结）
     */
    [[nodiscard]] bool _isEntityInSimulationRange(
        const Entity& entity, const std::vector<world::chunk::ChunkPos>& playerChunks) const;
};

} // namespace mc
