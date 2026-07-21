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

#include "EntityManager.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/core/MobEntity.hpp"
#include "common/entity/registry/VanillaEntityTypeKeys.hpp"
#include "common/profiler/TraceEvents.hpp"
#include <algorithm>
#include <spdlog/spdlog.h>

using namespace mc::trace;

namespace mc {

EntityManager::EntityManager() {}

EntityInstanceId EntityManager::addEntity(std::unique_ptr<Entity> entity)
{
    if (!entity) {
        return 0;
    }

    std::lock_guard<std::recursive_mutex> lock(m_mutex);

    EntityInstanceId id = entity->id();

    // 如果ID为0或已存在，分配新ID
    if (id == 0 || m_entities.find(id) != m_entities.end()) {
        id = allocateId();
        // 设置实体的ID
        entity->setId(id);
    }

    // 维护 UUID 索引
    const std::string& uuid = entity->uuid();
    if (!uuid.empty()) {
        if (m_uuidToEntity.find(uuid) != m_uuidToEntity.end()) {
            // UUID 冲突：输出警告但不阻止添加
            spdlog::warn("Duplicate entity UUID {}: entity {} will override existing mapping", uuid, id);
        }
        m_uuidToEntity[uuid] = entity.get();
    }

    m_entities[id] = std::move(entity);
    return id;
}

std::unique_ptr<Entity> EntityManager::removeEntity(EntityInstanceId id)
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);

    auto it = m_entities.find(id);
    if (it == m_entities.end()) {
        return nullptr;
    }

    auto entity = std::move(it->second);

    // 维护 UUID 索引
    const std::string& uuid = entity->uuid();
    if (!uuid.empty()) {
        auto uuidIt = m_uuidToEntity.find(uuid);
        // 仅当映射指向当前实体时才移除（防止移除后添加的同 UUID 实体被误删）
        if (uuidIt != m_uuidToEntity.end() && uuidIt->second == entity.get()) {
            m_uuidToEntity.erase(uuidIt);
        }
    }

    m_entities.erase(it);

    return entity;
}

bool EntityManager::hasEntity(EntityInstanceId id) const
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    return m_entities.find(id) != m_entities.end();
}

size_t EntityManager::entityCount() const
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    return m_entities.size();
}

Entity* EntityManager::getEntity(EntityInstanceId id)
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    auto it = m_entities.find(id);
    return it != m_entities.end() ? it->second.get() : nullptr;
}

const Entity* EntityManager::getEntity(EntityInstanceId id) const
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    auto it = m_entities.find(id);
    return it != m_entities.end() ? it->second.get() : nullptr;
}

Entity* EntityManager::getEntityByUuid(const std::string& uuid)
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    auto it = m_uuidToEntity.find(uuid);
    return it != m_uuidToEntity.end() ? it->second : nullptr;
}

const Entity* EntityManager::getEntityByUuid(const std::string& uuid) const
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    auto it = m_uuidToEntity.find(uuid);
    return it != m_uuidToEntity.end() ? it->second : nullptr;
}

bool EntityManager::hasEntityWithUuid(const std::string& uuid) const
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    return m_uuidToEntity.find(uuid) != m_uuidToEntity.end();
}

std::vector<Entity*> EntityManager::getEntitiesInAABB(const AxisAlignedBB& box, const Entity* except) const
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    std::vector<Entity*> result;

    for (const auto& [id, entity] : m_entities) {
        if (entity.get() == except) {
            continue;
        }

        if (entity->isRemoved()) {
            continue;
        }

        // 检查碰撞箱是否相交
        AxisAlignedBB entityBox = entity->boundingBox();
        if (box.intersects(entityBox)) {
            result.push_back(entity.get());
        }
    }

    return result;
}

std::vector<Entity*> EntityManager::getEntitiesInRange(const Vector3& pos, f32 range, const Entity* except) const
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    std::vector<Entity*> result;

    f32 rangeSq = range * range;

    for (const auto& [id, entity] : m_entities) {
        if (entity.get() == except) {
            continue;
        }

        if (entity->isRemoved()) {
            continue;
        }

        // 检查距离
        Vector3 entityPos = entity->position();
        f32 dx = entityPos.x - pos.x;
        f32 dy = entityPos.y - pos.y;
        f32 dz = entityPos.z - pos.z;
        f32 distSq = dx * dx + dy * dy + dz * dz;

        if (distSq <= rangeSq) {
            result.push_back(entity.get());
        }
    }

    return result;
}

std::vector<Entity*> EntityManager::getEntitiesByType(const std::string& typeId) const
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    std::vector<Entity*> result;

    for (const auto& [id, entity] : m_entities) {
        if (entity->getTypeId() == typeId && !entity->isRemoved()) {
            result.push_back(entity.get());
        }
    }

    return result;
}

void EntityManager::forEachEntity(const std::function<bool(Entity*)>& callback)
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    for (auto& [id, entity] : m_entities) {
        if (!callback(entity.get())) {
            break;
        }
    }
}

void EntityManager::forEachEntity(const std::function<bool(const Entity*)>& callback) const
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    for (const auto& [id, entity] : m_entities) {
        if (!callback(entity.get())) {
            break;
        }
    }
}

void EntityManager::tick()
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Tick, "EntityManager::tick");

    std::lock_guard<std::recursive_mutex> lock(m_mutex);

    // 1. 更新所有实体。期间 goal 通过 isAlive() 检测上一 tick 入 graveyard 的目标
    //    （对象仍存活、m_removed=true）并 resetTask 清空裸指针，避免悬垂解引用。
    for (auto& [id, entity] : m_entities) {
        // spdlog::info("Ticking entity: id={}, detail={}", id, entity->toString());
        if (!entity->isRemoved()) {
            MC_TRACE_SCOPED_EVENT(
                TraceEvents.Server.Tick, "EntityManager::tick.perEntity", "entityId", id, "name", entity->getTypeId());
            entity->tick();
        }
    }

    // 2. 释放上一 tick 入 graveyard 的实体。此时引用者已在步骤 1 通过 isAlive()==false 放手，可安全析构。
    //    必须在 entity tick 之后：若放开头，graveyard 实体在 goal 跑 shouldContinueExecuting 前就析构了。
    m_graveyard.clear();

    // 3. 移除本帧死亡实体（入 graveyard，延迟到下一 tick 末尾析构）。
    _removeDeadEntitiesInternal();
}

void EntityManager::removeDeadEntities()
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    // 与 tick 时序对齐：先释放上一批 graveyard，再收集本批死亡实体入 graveyard。
    m_graveyard.clear();
    _removeDeadEntitiesInternal();
}

void EntityManager::_removeDeadEntitiesInternal()
{
    // 内部方法，假设已持有锁
    for (auto it = m_entities.begin(); it != m_entities.end();) {
        if (it->second->isRemoved()) {
            // 维护 UUID 索引（erase 时同步清，不等 graveyard 析构时才清）
            const std::string& uuid = it->second->uuid();
            if (!uuid.empty()) {
                auto uuidIt = m_uuidToEntity.find(uuid);
                if (uuidIt != m_uuidToEntity.end() && uuidIt->second == it->second.get()) {
                    m_uuidToEntity.erase(uuidIt);
                }
            }

            // 延迟析构：实体对象入 graveyard，下一 tick 末尾才真正析构。
            // 避免持裸指针的 goal 在本 tick 末尾或下 tick 开头解引用悬垂内存。
            m_graveyard.push_back(std::move(it->second));
            it = m_entities.erase(it); // it->second 已被 move 成 nullptr，erase 仅移除 map 条目
        } else {
            ++it;
        }
    }
}

EntityInstanceId EntityManager::allocateId()
{
    // 内部方法，假设已持有锁
    // ID 单调递增、永不复用（见头文件说明）。
    return m_nextId++;
}

std::unordered_map<entity::EntityClassification, i32> EntityManager::countEntitiesByClassification() const
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    std::unordered_map<entity::EntityClassification, i32> counts;

    // 初始化所有分类为 0
    counts[entity::EntityClassification::Monster] = 0;
    counts[entity::EntityClassification::Creature] = 0;
    counts[entity::EntityClassification::Ambient] = 0;
    counts[entity::EntityClassification::Axolotls] = 0;
    counts[entity::EntityClassification::UndergroundWaterCreature] = 0;
    counts[entity::EntityClassification::WaterCreature] = 0;
    counts[entity::EntityClassification::WaterAmbient] = 0;
    counts[entity::EntityClassification::Misc] = 0;

    // 遍历所有实体，统计各分类数量
    auto& registry = entity::EntityRegistry::instance();
    for (const auto& [id, entity] : m_entities) {
        if (entity && !entity->isRemoved()) {
            // 通过实体类型ID获取分类
            const std::string& typeId = entity->getTypeId();
            const entity::EntityType* type = registry.getType(typeId);
            if (type) {
                entity::EntityClassification classification = type->classification();

                // 跳过持久化的 Mob（命名/桶装等）：createState 不把持久化实体计入
                // 容量计数，否则会挤占刷新名额。
                if (classification != entity::EntityClassification::Misc) {
                    const auto* mob = dynamic_cast<const MobEntity*>(entity.get());
                    if (mob != nullptr && (mob->isNoDespawnRequired() || mob->preventDespawn())) {
                        continue;
                    }
                }

                counts[classification]++;
            }
        }
    }

    return counts;
}

i32 EntityManager::getCountByClassification(entity::EntityClassification classification) const
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    i32 count = 0;

    auto& registry = entity::EntityRegistry::instance();
    for (const auto& [id, entity] : m_entities) {
        if (entity && !entity->isRemoved()) {
            const std::string& typeId = entity->getTypeId();
            const entity::EntityType* type = registry.getType(typeId);
            if (type && type->classification() == classification) {
                count++;
            }
        }
    }

    return count;
}

std::vector<Entity*> EntityManager::getPlayers() const
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    std::vector<Entity*> result;

    for (const auto& [id, entity] : m_entities) {
        if (entity && !entity->isRemoved() && entity->entityType() == entity::VanillaEntityTypeKeys::PLAYER) {
            result.push_back(entity.get());
        }
    }

    return result;
}

} // namespace mc
