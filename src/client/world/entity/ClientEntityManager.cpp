#include "ClientEntityManager.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include <algorithm>

namespace mc::client {

ClientEntity* ClientEntityManager::spawnEntity(EntityId id, const String& typeId) {
    // 检查是否已存在
    if (m_entities.find(id) != m_entities.end()) {
        return nullptr;
    }

    // 不能创建与本地玩家ID相同的实体
    if (id == m_localPlayerEntityId) {
        return nullptr;
    }

    // 创建实体
    auto entity = std::make_unique<ClientEntity>(id, typeId);
    auto* ptr = entity.get();
    m_entities[id] = std::move(entity);

    return ptr;
}

ClientEntity* ClientEntityManager::spawnLocalPlayer(EntityId entityId, PlayerId playerId, const String& username) {
    // 如果已有本地玩家，先清除
    if (m_localPlayerEntityId != INVALID_ENTITY_ID) {
        m_entities.erase(m_localPlayerEntityId);
    }

    // 创建本地玩家实体
    auto entity = std::make_unique<ClientEntity>(entityId, mc::entity::EntityTypes::PLAYER);
    auto* ptr = entity.get();
    m_entities[entityId] = std::move(entity);

    // 记录本地玩家信息
    m_localPlayerEntityId = entityId;
    m_localPlayerId = playerId;

    return ptr;
}

bool ClientEntityManager::removeEntity(EntityId id) {
    // 不能移除本地玩家
    if (id == m_localPlayerEntityId) {
        return false;
    }

    auto it = m_entities.find(id);
    if (it == m_entities.end()) {
        return false;
    }

    // 标记为移除
    it->second->remove();
    m_entitiesToRemove.push_back(id);
    return true;
}

ClientEntity* ClientEntityManager::getEntity(EntityId id) {
    auto it = m_entities.find(id);
    if (it == m_entities.end()) {
        return nullptr;
    }
    return it->second.get();
}

const ClientEntity* ClientEntityManager::getEntity(EntityId id) const {
    auto it = m_entities.find(id);
    if (it == m_entities.end()) {
        return nullptr;
    }
    return it->second.get();
}

bool ClientEntityManager::hasEntity(EntityId id) const {
    return m_entities.find(id) != m_entities.end();
}

void ClientEntityManager::clear() {
    // 保留本地玩家
    if (m_localPlayerEntityId != INVALID_ENTITY_ID) {
        auto localPlayerIt = m_entities.find(m_localPlayerEntityId);
        if (localPlayerIt != m_entities.end()) {
            auto localPlayer = std::move(localPlayerIt->second);
            m_entities.clear();
            m_entities[m_localPlayerEntityId] = std::move(localPlayer);
        } else {
            m_entities.clear();
        }
    } else {
        m_entities.clear();
    }
    m_entitiesToRemove.clear();
}

void ClientEntityManager::clearLocalPlayer() {
    if (m_localPlayerEntityId != INVALID_ENTITY_ID) {
        m_entities.erase(m_localPlayerEntityId);
        m_localPlayerEntityId = INVALID_ENTITY_ID;
        m_localPlayerId = 0;
    }
}

ClientEntity* ClientEntityManager::localPlayer() {
    if (m_localPlayerEntityId == INVALID_ENTITY_ID) {
        return nullptr;
    }
    return getEntity(m_localPlayerEntityId);
}

const ClientEntity* ClientEntityManager::localPlayer() const {
    if (m_localPlayerEntityId == INVALID_ENTITY_ID) {
        return nullptr;
    }
    return getEntity(m_localPlayerEntityId);
}

bool ClientEntityManager::isLocalPlayer(EntityId entityId) const {
    return entityId == m_localPlayerEntityId;
}

size_t ClientEntityManager::entityCount() const {
    // 不包括本地玩家
    if (m_localPlayerEntityId != INVALID_ENTITY_ID) {
        return m_entities.size() - 1;
    }
    return m_entities.size();
}

void ClientEntityManager::forEachEntity(std::function<void(ClientEntity&)> func) {
    for (auto& [id, entity] : m_entities) {
        if (entity && entity->isAlive()) {
            func(*entity);
        }
    }
}

void ClientEntityManager::forEachEntity(std::function<void(const ClientEntity&)> func) const {
    for (const auto& [id, entity] : m_entities) {
        if (entity && entity->isAlive()) {
            func(*entity);
        }
    }
}

void ClientEntityManager::forEachRemoteEntity(std::function<void(ClientEntity&)> func) {
    for (auto& [id, entity] : m_entities) {
        if (entity && entity->isAlive() && id != m_localPlayerEntityId) {
            func(*entity);
        }
    }
}

void ClientEntityManager::removeDeadEntities() {
    for (EntityId id : m_entitiesToRemove) {
        m_entities.erase(id);
    }
    m_entitiesToRemove.clear();
}

std::vector<EntityId> ClientEntityManager::getEntitiesByType(const String& typeId) const {
    std::vector<EntityId> result;
    for (const auto& [id, entity] : m_entities) {
        if (entity && entity->isAlive() && entity->typeId() == typeId) {
            result.push_back(id);
        }
    }
    return result;
}

std::vector<EntityId> ClientEntityManager::getEntitiesInRange(f32 x, f32 y, f32 z, f32 radius) const {
    std::vector<EntityId> result;
    f32 radiusSq = radius * radius;

    for (const auto& [id, entity] : m_entities) {
        if (entity && entity->isAlive()) {
            f32 dx = entity->x() - x;
            f32 dy = entity->y() - y;
            f32 dz = entity->z() - z;
            f32 distSq = dx * dx + dy * dy + dz * dz;
            if (distSq <= radiusSq) {
                result.push_back(id);
            }
        }
    }

    return result;
}

void ClientEntityManager::tick() {
    // 更新所有实体
    for (auto& [id, entity] : m_entities) {
        if (entity && entity->isAlive()) {
            // 计算移动距离用于动画
            auto pos = entity->position();
            auto prevPos = entity->prevPosition();
            f32 dx = pos.x - prevPos.x;
            f32 dz = pos.z - prevPos.z;
            f32 distanceMoved = std::sqrt(dx * dx + dz * dz);

            entity->updateAnimation(distanceMoved);
            entity->tick();
        }
    }

    // 移除已标记为移除的实体
    removeDeadEntities();
}

void ClientEntityManager::updateInterpolation(f32 deltaTime) {
    // 每帧更新所有实体的平滑插值
    for (auto& [id, entity] : m_entities) {
        if (entity && entity->isAlive()) {
            entity->updateInterpolation(deltaTime);
        }
    }
}

void ClientEntityManager::updateAnimations(f32 /*partialTick*/) {
    // 当前动画更新在tick()中完成
    // 如果需要更精确的插值，可以在这里进行
}

} // namespace mc::client
