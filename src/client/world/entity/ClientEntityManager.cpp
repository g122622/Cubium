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

#include "ClientEntityManager.hpp"
#include "client/world/entity/ClientEntity.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/core/EntitySize.hpp"
#include "common/entity/core/EntityType.hpp"
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include <spdlog/spdlog.h>

namespace mc::client {

ClientEntity* ClientEntityManager::spawnEntity(EntityInstanceId id, const std::string& typeId)
{
    // 检查是否已存在
    if (m_entities.find(id) != m_entities.end()) {
        spdlog::warn("ClientEntityManager::spawnEntity: Entity with ID {} already exists, skipping spawn", id);
        return nullptr;
    }

    // 不能创建与本地玩家ID相同的实体
    if (id == m_localPlayerEntityId) {
        spdlog::warn(
            "ClientEntityManager::spawnEntity: Cannot spawn entity with the same ID as local player (ID {})", id);
        return nullptr;
    }

    // 创建实体
    auto entity = std::make_unique<ClientEntity>(id, typeId);
    auto* ptr = entity.get();

    // 从实体注册表中初始化宽度、高度和眼高
    const auto& registry = entity::EntityRegistry::instance();
    const entity::EntityType* type = registry.getType(typeId);
    if (type != nullptr) {
        const entity::EntitySize& size = type->size();
        ptr->setWidth(size.width());
        ptr->setHeight(size.height());
        // 眼高默认为注册表中的值，后续 refreshEyeHeight() 会根据姿态调整
        ptr->setEyeHeight(size.eyeHeight());
    }

    m_entities[id] = std::move(entity);

    return ptr;
}

ClientEntity* ClientEntityManager::spawnLocalPlayer(
    EntityInstanceId entityId, PlayerId playerId, const std::string& username)
{
    // 如果已有本地玩家，先清除
    if (m_localPlayerEntityId != INVALID_ENTITY_ID) {
        m_entities.erase(m_localPlayerEntityId);
    }

    // 创建本地玩家实体
    auto entity = std::make_unique<ClientEntity>(entityId, mc::entity::EntityTypeKeys::PLAYER);
    auto* ptr = entity.get();

    // 玩家实体使用标准尺寸和站立眼高
    ptr->setWidth(0.6f);
    ptr->setHeight(1.8f);
    ptr->setEyeHeight(1.62f);

    m_entities[entityId] = std::move(entity);

    // 记录本地玩家信息
    m_localPlayerEntityId = entityId;
    m_localPlayerId = playerId;

    return ptr;
}

bool ClientEntityManager::removeEntity(EntityInstanceId id)
{
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

ClientEntity* ClientEntityManager::getEntity(EntityInstanceId id)
{
    auto it = m_entities.find(id);
    if (it == m_entities.end()) {
        return nullptr;
    }
    return it->second.get();
}

const ClientEntity* ClientEntityManager::getEntity(EntityInstanceId id) const
{
    auto it = m_entities.find(id);
    if (it == m_entities.end()) {
        return nullptr;
    }
    return it->second.get();
}

bool ClientEntityManager::hasEntity(EntityInstanceId id) const
{
    return m_entities.find(id) != m_entities.end();
}

void ClientEntityManager::clear()
{
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

void ClientEntityManager::clearLocalPlayer()
{
    if (m_localPlayerEntityId != INVALID_ENTITY_ID) {
        m_entities.erase(m_localPlayerEntityId);
        m_localPlayerEntityId = INVALID_ENTITY_ID;
        m_localPlayerId = 0;
    }
}

ClientEntity* ClientEntityManager::localPlayer()
{
    if (m_localPlayerEntityId == INVALID_ENTITY_ID) {
        return nullptr;
    }
    return getEntity(m_localPlayerEntityId);
}

const ClientEntity* ClientEntityManager::localPlayer() const
{
    if (m_localPlayerEntityId == INVALID_ENTITY_ID) {
        return nullptr;
    }
    return getEntity(m_localPlayerEntityId);
}

bool ClientEntityManager::isLocalPlayer(EntityInstanceId entityId) const
{
    return entityId == m_localPlayerEntityId;
}

size_t ClientEntityManager::entityCount() const
{
    // 不包括本地玩家
    if (m_localPlayerEntityId != INVALID_ENTITY_ID) {
        return m_entities.size() - 1;
    }
    return m_entities.size();
}

void ClientEntityManager::forEachEntity(std::function<void(ClientEntity&)> func)
{
    for (auto& [id, entity] : m_entities) {
        if (entity && entity->isAlive()) {
            func(*entity);
        }
    }
}

void ClientEntityManager::forEachEntity(std::function<void(const ClientEntity&)> func) const
{
    for (const auto& [id, entity] : m_entities) {
        if (entity && entity->isAlive()) {
            func(*entity);
        }
    }
}

void ClientEntityManager::forEachRemoteEntity(std::function<void(ClientEntity&)> func)
{
    for (auto& [id, entity] : m_entities) {
        if (entity && entity->isAlive() && id != m_localPlayerEntityId) {
            func(*entity);
        }
    }
}

void ClientEntityManager::removeDeadEntities()
{
    for (EntityInstanceId id : m_entitiesToRemove) {
        m_entities.erase(id);
    }
    m_entitiesToRemove.clear();
}

std::vector<EntityInstanceId> ClientEntityManager::getEntitiesByType(const std::string& typeId) const
{
    std::vector<EntityInstanceId> result;
    for (const auto& [id, entity] : m_entities) {
        if (entity && entity->isAlive() && entity->getTypeId() == typeId) {
            result.push_back(id);
        }
    }
    return result;
}

std::vector<EntityInstanceId> ClientEntityManager::getEntitiesInRange(f32 x, f32 y, f32 z, f32 radius) const
{
    std::vector<EntityInstanceId> result;
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

void ClientEntityManager::tick()
{
    // TODO(shouldTickDeath): vanilla ClientLevel.shouldTickDeath 对 deathTime>0 的实体按
    // 切比雪夫距离 <= serverSimulationDistance 门控 tickDeath() 推进。本项目 ClientEntity::tick()
    // 不本地推进 deathTime（服务端元数据同步），门控当前为空操作，故暂不实现——见 ClientWorld.hpp
    // setSimulationDistance 注释。待客户端 deathTime 改为本地推进时在此补门控（仅跳 deathTime 部分）。
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

            // 凋灵侧头朝向：客户端不运行 WitherEntity::aiStep()，
            // 在此调用 tickWitherSideHeads 本地镜像 MC 1.21.11 WitherBoss.aiStep()
            // 的侧头朝向计算逻辑。仅对凋灵实体有效。
            const std::string& typeId = entity->getTypeId();
            if (typeId == "minecraft:wither" || typeId == "wither") {
                // 实体查找回调：通过 this->getEntity(id) 查找目标实体
                // 使用引用捕获 this，避免拷贝；回调在当前 tick 内立即使用，生命周期安全。
                entity->tickWitherSideHeads(
                    [this](EntityInstanceId targetId) -> const ClientEntity* { return this->getEntity(targetId); });
            }
        }
    }

    // 移除已标记为移除的实体
    removeDeadEntities();
}

u32 ClientEntityManager::fixedTick(f32 deltaTime)
{
    m_tickAccumulator += std::min(deltaTime, TICK_INTERVAL * static_cast<f32>(MAX_TICKS_PER_FRAME));
    u32 tickCount = 0;
    while (m_tickAccumulator >= TICK_INTERVAL && tickCount < MAX_TICKS_PER_FRAME) {
        m_tickAccumulator -= TICK_INTERVAL;
        tick();
        ++tickCount;
    }
    return tickCount;
}

void ClientEntityManager::updateInterpolation(f32 deltaTime)
{
    // 每帧更新所有实体的平滑插值
    for (auto& [id, entity] : m_entities) {
        if (entity && entity->isAlive()) {
            entity->updateInterpolation(deltaTime);
        }
    }
}

void ClientEntityManager::updateAnimations(f32 /*partialTick*/)
{
    // 当前动画更新在tick()中完成
    // 如果需要更精确的插值，可以在这里进行
}

} // namespace mc::client
