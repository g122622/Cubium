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

#include "EntitySpatialIndex.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/registry/VanillaEntityTypeKeys.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/world/block/BlockPos.hpp"

#include <cmath>
#include <limits>

namespace mc {

namespace {
/// 判定实体是否为玩家（与玩家专表维护一致）
bool _isPlayer(const Entity& entity)
{
    return entity.entityType() == entity::VanillaEntityTypeKeys::PLAYER;
}
} // namespace

i64 EntitySpatialIndex::_sectionKeyOf(const Entity& entity) noexcept
{
    // 用 AABB 中心而非脚底：大体积实体跨 section 时归属稳定，避免边界抖动反复迁移。
    const Vector3 center = entity.boundingBox().center();
    // BlockPos(Vector3) 内部 std::floor，负坐标正确；SectionPos(BlockPos) 用
    // chunkX/sectionCoord/chunkZ（算术右移，C++20 保证负数正确）。花括号避免 vexing parse。
    const SectionPos sec{BlockPos{center}};
    return sec.toLong();
}

EntitySectionBucket& EntitySpatialIndex::_getOrCreateSection(i64 key)
{
    auto it = m_sections.find(key);
    if (it == m_sections.end()) {
        it = m_sections.emplace(key, EntitySectionBucket{}).first;
    }
    return it->second;
}

void EntitySpatialIndex::_removeSectionIfEmpty(i64 key)
{
    auto it = m_sections.find(key);
    if (it != m_sections.end() && it->second.isEmpty()) {
        m_sections.erase(it);
    }
}

void EntitySpatialIndex::_syncPlayerMembership(Entity& entity, bool added)
{
    if (!_isPlayer(entity)) {
        return;
    }
    if (added) {
        m_players.push_back(&entity);
    } else {
        auto it = std::find(m_players.begin(), m_players.end(), &entity);
        if (it != m_players.end()) {
            *it = m_players.back();
            m_players.pop_back();
        }
    }
}

void EntitySpatialIndex::addEntity(Entity& entity)
{
    const i64 key = _sectionKeyOf(entity);
    _getOrCreateSection(key).add(entity);
    m_entitySection[entity.id()] = key;
    _syncPlayerMembership(entity, true);
}

void EntitySpatialIndex::removeEntity(const Entity& entity)
{
    const auto it = m_entitySection.find(entity.id());
    if (it == m_entitySection.end()) {
        // 未登记（重复移除或从未 add）：仅清理玩家专表后返回
        _syncPlayerMembership(const_cast<Entity&>(entity), false);
        return;
    }
    const i64 key = it->second;
    auto secIt = m_sections.find(key);
    if (secIt != m_sections.end()) {
        secIt->second.remove(entity);
        _removeSectionIfEmpty(key);
    }
    m_entitySection.erase(it);
    _syncPlayerMembership(const_cast<Entity&>(entity), false);
}

void EntitySpatialIndex::onEntityPositionChanged(Entity& entity)
{
    const i64 newKey = _sectionKeyOf(entity);
    const auto it = m_entitySection.find(entity.id());
    if (it == m_entitySection.end()) {
        // 未登记（addEntity 前的 setPosition）：忽略，addEntity 时按当前位置一次性登记
        return;
    }
    if (it->second == newKey) {
        return; // 同 section 内移动，无需迁移
    }
    // 跨 section 迁移：旧 section 移除 → 新 section 加入
    const i64 oldKey = it->second;
    auto oldSecIt = m_sections.find(oldKey);
    if (oldSecIt != m_sections.end()) {
        oldSecIt->second.remove(entity);
        _removeSectionIfEmpty(oldKey);
    }
    _getOrCreateSection(newKey).add(entity);
    it->second = newKey;
}

void EntitySpatialIndex::collectEntitiesInAABB(
    const AxisAlignedBB& box, const Entity* except, const std::function<bool(Entity&)>& callback) const
{
    // 算 box 覆盖的 section 范围。BlockPos(Vector3) 内部 floor，负坐标正确。花括号避免 vexing parse。
    const SectionPos minSec{BlockPos{Vector3(box.minX, box.minY, box.minZ)}};
    const SectionPos maxSec{BlockPos{Vector3(box.maxX, box.maxY, box.maxZ)}};

    for (i32 sy = minSec.y; sy <= maxSec.y; ++sy) {
        for (ChunkCoord sz = minSec.z; sz <= maxSec.z; ++sz) {
            for (ChunkCoord sx = minSec.x; sx <= maxSec.x; ++sx) {
                const i64 key = SectionPos(sx, sy, sz).toLong();
                auto secIt = m_sections.find(key);
                if (secIt == m_sections.end()) {
                    continue; // 空桶 O(1) 跳过
                }
                const std::vector<Entity*>& entities = secIt->second.allEntities();
                // 下标遍历 + size() 重取：回调内实体 move 可能触发本 bucket swap-remove，
                // 接受近似语义（对齐 Java：查询期间实体 move 不保证一致性），不崩溃。
                for (size_t i = 0; i < entities.size(); ++i) {
                    Entity* e = entities[i];
                    if (e == nullptr || e == except) {
                        continue;
                    }
                    if (!box.intersects(e->boundingBox())) {
                        continue;
                    }
                    if (!callback(*e)) {
                        return;
                    }
                }
            }
        }
    }
}

void EntitySpatialIndex::collectEntitiesInRange(
    const Vector3& pos, f32 range, const Entity* except, const std::function<bool(Entity&)>& callback) const
{
    // 球转外接盒 AABB 走 collectEntitiesInAABB 枚举候选，回调内用实体 position（脚底）
    // 到球心的距离平方精筛，与原 EntityManager::getEntitiesInRange 语义一致。
    const AxisAlignedBB box(pos.x - range, pos.y - range, pos.z - range, pos.x + range, pos.y + range, pos.z + range);
    const f32 rangeSq = range * range;
    collectEntitiesInAABB(box, except, [&](Entity& e) -> bool {
        const Vector3 ep = e.position();
        const f32 dx = ep.x - pos.x;
        const f32 dy = ep.y - pos.y;
        const f32 dz = ep.z - pos.z;
        if (dx * dx + dy * dy + dz * dz > rangeSq) {
            return true; // 距离过远，跳过但继续遍历
        }
        return callback(e);
    });
}

void EntitySpatialIndex::collectEntitiesByType(
    const entity::EntityType* type, const std::function<bool(Entity&)>& callback) const
{
    if (type == nullptr) {
        return;
    }
    // 遍历所有 section 的该类型子列表（懒加载）。下标 + size() 重取防 swap-remove。
    for (auto& [key, bucket] : m_sections) {
        (void)key;
        const std::vector<Entity*>& entities = bucket.entitiesOfType(type);
        for (size_t i = 0; i < entities.size(); ++i) {
            Entity* e = entities[i];
            if (e == nullptr) {
                continue;
            }
            if (!callback(*e)) {
                return;
            }
        }
    }
}

Entity* EntitySpatialIndex::getClosestPlayer(const Vector3& pos, f32 maxDistance, const Entity* exclude) const
{
    const f32 maxSq = maxDistance * maxDistance;
    Entity* best = nullptr;
    f32 bestSq = std::numeric_limits<f32>::max();
    for (Entity* e : m_players) {
        if (e == nullptr || e == exclude) {
            continue;
        }
        const f32 dSq = e->boundingBox().distanceToSqr(pos);
        if (dSq <= maxSq && dSq < bestSq) {
            bestSq = dSq;
            best = e;
        }
    }
    return best;
}

std::vector<EntityInstanceId> EntitySpatialIndex::getEntityIdsInChunkColumn(ChunkCoord cx, ChunkCoord cz) const
{
    std::vector<EntityInstanceId> result;
    // 遍历该 chunk 列所有 section（y 从 MIN_SECTION_Y 到 MAX_SECTION_Y，共 24 个）。
    for (i32 sy = world::MIN_SECTION_Y; sy <= world::MAX_SECTION_Y; ++sy) {
        const i64 key = SectionPos(cx, sy, cz).toLong();
        auto secIt = m_sections.find(key);
        if (secIt == m_sections.end()) {
            continue;
        }
        const std::vector<Entity*>& entities = secIt->second.allEntities();
        for (Entity* e : entities) {
            if (e != nullptr) {
                result.push_back(e->id());
            }
        }
    }
    return result;
}

void EntitySpatialIndex::_assertConsistent(
    const std::unordered_map<EntityInstanceId, std::unique_ptr<Entity>>& entities) const
{
#ifndef NDEBUG
    // 1. 每个主存储实体都在其 AABB 中心算出的 section 内，且 m_entitySection 一致
    for (const auto& [id, ptr] : entities) {
        const Entity& e = *ptr;
        const i64 expectedKey = _sectionKeyOf(e);
        const auto esIt = m_entitySection.find(id);
        MC_ASSERT_MSG(esIt != m_entitySection.end(), "entity not in spatial index");
        MC_ASSERT_MSG(esIt->second == expectedKey, "entity in wrong section");
        const auto secIt = m_sections.find(expectedKey);
        MC_ASSERT_MSG(secIt != m_sections.end(), "section missing for entity");
    }

    // 2. m_entitySection 条目数 == 主存储实体数
    MC_ASSERT_MSG(m_entitySection.size() == entities.size(), "entitySection size mismatch");

    // 3. 玩家专表与主存储中 PLAYER 一致
    size_t playerCount = 0;
    for (const auto& [id, ptr] : entities) {
        (void)id;
        if (_isPlayer(*ptr)) {
            ++playerCount;
        }
    }
    MC_ASSERT_MSG(m_players.size() == playerCount, "player table size mismatch");
#else
    (void)entities;
#endif
}

} // namespace mc
