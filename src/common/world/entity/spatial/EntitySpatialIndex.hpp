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
 * The above copyright and this permission notice shall be included in all
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

#include "EntitySectionBucket.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/EntityType.hpp"
#include "common/util/AxisAlignedBB.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/chunk/base/SectionPos.hpp"
#include <cstddef>
#include <functional>
#include <unordered_map>
#include <vector>

namespace mc {

class Entity;

/**
 * @brief 3D section(16³)空间索引
 *
 * 把实体按其 AABB 中心所在 section 分桶，所有空间/类型查询从 O(全服实体) 降到
 * O(覆盖 section 数 × section 内实体数)。借鉴 Java `EntitySection` +
 * `EntitySectionStorage` + `ClassInstanceMultiMap` 的精华，但用 C++ 哈希表替代
 * Java 的 AVL 树、用 `const EntityType*` 指针替代反射做类型分桶。
 *
 * 索引实时准确：`Entity::reapplyPosition()`（位置变更统一收口）经 EntityManager
 * 反向指针调 `onEntityPositionChanged`，实体跨 section 移动立即迁移，同一 tick
 * 内移动后查询读到新位置（对齐 Lithium `entity_movement_tracking` 推送式更新）。
 *
 * section key 用 `SectionPos::toLong()`（22+20+22 位打包），`m_entitySection` 记
 * 每个实体当前所在 section key 供迁移对比。玩家走 `m_players` 专表 O(1)。
 *
 * 线程安全：本类自身不加锁，由持有方 `EntityManager` 的 `m_mutex` 保护。`m_sections`/
 * `m_players` 声明 `mutable`：const 查询方法内回调可能经 Entity→EntityManager→索引
 * 触发 section 迁移（逻辑 const），与 `m_mutex` 声明 mutable 同理。
 */
class EntitySpatialIndex {
public:
    /**
     * @brief 注册实体到索引（按当前 AABB 中心算 section）
     *
     * 由 `EntityManager::addEntity` 调用。同时按类型加入玩家专表（PLAYER）。
     * 前置：实体已设置有效位置（`boundingBox()` 可用）。
     */
    void addEntity(Entity& entity);

    /**
     * @brief 从索引移除实体（从其当前 section 移除，空桶立即回收）
     *
     * 由 `EntityManager::removeEntity`/`_removeDeadEntitiesInternal` 调用。
     * 同时从玩家专表移除（若是玩家）。
     */
    void removeEntity(const Entity& entity);

    /**
     * @brief 通知实体位置变更，必要时迁移到新 section
     *
     * 由 `Entity::reapplyPosition` 经 EntityManager 反向指针调用。算新 section key
     * 与 `m_entitySection[id]` 对比，不同则从旧 section 移除、加到新 section。
     * 玩家身份不受位置变更影响（专表不动）。
     */
    void onEntityPositionChanged(Entity& entity);

    // ========== 查询（回调返回 false 中止遍历） ==========

    /**
     * @brief 收集 AABB 内的实体
     *
     * 算 box 覆盖的 section 范围，三重循环枚举，每 section 命中则遍历其全部实体
     * 做 AABB 精筛（`intersects`）。`except` 跳过。下标遍历 + `size()` 重取
     * （回调内实体 move 可能触发本 bucket swap-remove，迭代器不安全）。
     */
    void collectEntitiesInAABB(
        const AxisAlignedBB& box, const Entity* except, const std::function<bool(Entity&)>& callback) const;

    /**
     * @brief 收集球范围内的实体
     *
     * 球转外接盒 AABB 走 `collectEntitiesInAABB`，回调内用 `distanceToSqr` 精筛
     * （<= range²）。
     */
    void collectEntitiesInRange(
        const Vector3& pos, f32 range, const Entity* except, const std::function<bool(Entity&)>& callback) const;

    /**
     * @brief 收集指定类型的全部实体
     *
     * 遍历所有 section 的该类型子列表（懒加载）。不维护全局类型索引——调用方仅
     * ~5 处，遍历各 section 子列表够快。下标遍历 + `size()` 重取。
     */
    void collectEntitiesByType(const entity::EntityType* type, const std::function<bool(Entity&)>& callback) const;

    // ========== 玩家专表（O(1)） ==========

    [[nodiscard]] const std::vector<Entity*>& players() const noexcept { return m_players; }

    /**
     * @brief 取距 pos 最近、距离 <= maxDistance 的玩家（exclude 跳过）
     *
     * 遍历玩家专表，O(玩家数)。无候选返回 nullptr。
     */
    [[nodiscard]] Entity* getClosestPlayer(const Vector3& pos, f32 maxDistance, const Entity* exclude) const;

    // ========== 区块卸载/关机保存支持 ==========

    /**
     * @brief 取指定 chunk 列内全部实体的 ID
     *
     * 遍历该 (cx,cz) 列的所有 section（y 从 `MIN_SECTION_Y` 到 `MAX_SECTION_Y`，
     * 共 24 个），合并实体 ID。供 `ServerWorld::onChunkUnloading`/`shutdown` 替代
     * 已删除的 `EntityChunkTracker::getEntitiesInChunk`。区块卸载是低频操作，24 次
     * 哈希查找 vs 原 1 次，差异可忽略。
     */
    [[nodiscard]] std::vector<EntityInstanceId> getEntityIdsInChunkColumn(ChunkCoord cx, ChunkCoord cz) const;

    // ========== 统计/诊断 ==========

    [[nodiscard]] size_t sectionCount() const noexcept { return m_sections.size(); }
    [[nodiscard]] size_t entityCount() const noexcept { return m_entitySection.size(); }

    /**
     * @brief DEBUG 一致性断言（仅调试构建生效）
     *
     * 校验：每个实体都在其 AABB 中心算出的 section 内、玩家专表与主存储中 PLAYER
     * 一致、`m_entitySection` 与 `m_sections` 双向一致。由 `EntityManager::tick`
     * 末尾调用，及早暴露索引与主存储不一致。
     */
    void _assertConsistent(const std::unordered_map<EntityInstanceId, std::unique_ptr<Entity>>& entities) const;

private:
    // sectionPos.toLong() → bucket
    mutable std::unordered_map<i64, EntitySectionBucket> m_sections;
    // entityId → 当前 sectionKey（迁移对比 + removeEntity 定位旧 section）
    std::unordered_map<EntityInstanceId, i64> m_entitySection;
    // 玩家专表（swap-remove O(1)）
    mutable std::vector<Entity*> m_players;

    /** 实体 AABB 中心 → SectionPos.toLong() */
    static i64 _sectionKeyOf(const Entity& entity) noexcept;

    EntitySectionBucket& _getOrCreateSection(i64 key);
    void _removeSectionIfEmpty(i64 key);
    void _syncPlayerMembership(Entity& entity, bool added);
};

} // namespace mc
