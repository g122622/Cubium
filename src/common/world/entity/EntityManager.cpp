#include "EntityManager.hpp"
#include "../../entity/core/Entity.hpp"
#include "../../entity/core/EntityRegistry.hpp"
#include <algorithm>
#include <spdlog/spdlog.h>

namespace mc {

EntityManager::EntityManager()
    : m_nextId(1)
{
}

EntityId EntityManager::addEntity(std::unique_ptr<Entity> entity) {
    if (!entity) {
        return 0;
    }

    std::lock_guard<std::recursive_mutex> lock(m_mutex);

    EntityId id = entity->id();

    // 如果ID为0或已存在，分配新ID
    if (id == 0 || m_entities.find(id) != m_entities.end()) {
        id = allocateId();
        // 设置实体的ID
        entity->setId(id);
    }

    m_entities[id] = std::move(entity);
    return id;
}

std::unique_ptr<Entity> EntityManager::removeEntity(EntityId id) {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);

    auto it = m_entities.find(id);
    if (it == m_entities.end()) {
        return nullptr;
    }

    auto entity = std::move(it->second);
    m_entities.erase(it);
    releaseId(id);

    return entity;
}

bool EntityManager::hasEntity(EntityId id) const {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    return m_entities.find(id) != m_entities.end();
}

size_t EntityManager::entityCount() const {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    return m_entities.size();
}

Entity* EntityManager::getEntity(EntityId id) {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    auto it = m_entities.find(id);
    return it != m_entities.end() ? it->second.get() : nullptr;
}

const Entity* EntityManager::getEntity(EntityId id) const {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    auto it = m_entities.find(id);
    return it != m_entities.end() ? it->second.get() : nullptr;
}

std::vector<Entity*> EntityManager::getEntitiesInAABB(
    const AxisAlignedBB& box,
    const Entity* except) const
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

std::vector<Entity*> EntityManager::getEntitiesInRange(
    const Vector3& pos,
    f32 range,
    const Entity* except) const
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

std::vector<Entity*> EntityManager::getEntitiesByType(LegacyEntityType type) const {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    std::vector<Entity*> result;

    for (const auto& [id, entity] : m_entities) {
        if (entity->legacyType() == type && !entity->isRemoved()) {
            result.push_back(entity.get());
        }
    }

    return result;
}

void EntityManager::forEachEntity(const std::function<bool(Entity*)>& callback) {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    for (auto& [id, entity] : m_entities) {
        if (!callback(entity.get())) {
            break;
        }
    }
}

void EntityManager::forEachEntity(const std::function<bool(const Entity*)>& callback) const {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    for (const auto& [id, entity] : m_entities) {
        if (!callback(entity.get())) {
            break;
        }
    }
}

void EntityManager::tick() {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);

    // 更新所有实体
    for (auto& [id, entity] : m_entities) {
        // spdlog::info("Ticking entity: id={}, detail={}", id, entity->toString());
        if (!entity->isRemoved()) {
            entity->tick();
        }
    }

    // 移除死亡实体
    removeDeadEntitiesInternal();
}

void EntityManager::removeDeadEntities() {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    removeDeadEntitiesInternal();
}

void EntityManager::removeDeadEntitiesInternal() {
    // 内部方法，假设已持有锁
    for (auto it = m_entities.begin(); it != m_entities.end(); ) {
        if (it->second->isRemoved()) {
            EntityId id = it->first;
            it = m_entities.erase(it);
            releaseId(id);
        } else {
            ++it;
        }
    }
}

EntityId EntityManager::allocateId() {
    // 内部方法，假设已持有锁
    if (!m_freeIds.empty()) {
        EntityId id = m_freeIds.back();
        m_freeIds.pop_back();
        return id;
    }
    return m_nextId++;
}

void EntityManager::releaseId(EntityId id) {
    // 内部方法，假设已持有锁
    if (id > 0 && id < m_nextId) {
        m_freeIds.push_back(id);
    }
}

std::unordered_map<entity::EntityClassification, i32> EntityManager::countEntitiesByClassification() const {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    std::unordered_map<entity::EntityClassification, i32> counts;

    // 初始化所有分类为 0
    counts[entity::EntityClassification::Monster] = 0;
    counts[entity::EntityClassification::Creature] = 0;
    counts[entity::EntityClassification::Ambient] = 0;
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
                counts[classification]++;
            }
        }
    }

    return counts;
}

i32 EntityManager::getCountByClassification(entity::EntityClassification classification) const {
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

std::vector<Entity*> EntityManager::getPlayers() const {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    std::vector<Entity*> result;

    for (const auto& [id, entity] : m_entities) {
        if (entity && !entity->isRemoved() && entity->legacyType() == LegacyEntityType::Player) {
            result.push_back(entity.get());
        }
    }

    return result;
}

} // namespace mc
