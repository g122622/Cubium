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

#include "EntitySyncManager.hpp"
#include "common/network/packet/EntityPackets.hpp"
#include "common/network/packet/PacketSerializer.hpp"
#include "common/profiler/TraceEvents.hpp"
#include <cmath>
#include <spdlog/spdlog.h>

using namespace mc::trace;

namespace mc::server::sync {

EntitySyncManager::EntitySyncManager(EntityManager& entityManager)
    : m_entityManager(entityManager)
{}

void EntitySyncManager::tick()
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Entity, "EntitySyncManager::tick");

    // 遍历所有实体，检查是否需要同步
    for (auto& [entityId, trackData] : m_entityTrackData) {
        Entity* entity = m_entityManager.getEntity(entityId);
        if (!entity || !entity->isAlive()) {
            continue;
        }

        const Vector3& pos = entity->position();
        f32 yaw = entity->yaw();
        f32 pitch = entity->pitch();

        // 检查位置是否变化
        bool positionChanged = false;
        bool rotationChanged = false;

        if (trackData.needsFullUpdate) {
            positionChanged = true;
            rotationChanged = true;
        } else {
            f32 dx = pos.x - trackData.lastPosition.x;
            f32 dy = pos.y - trackData.lastPosition.y;
            f32 dz = pos.z - trackData.lastPosition.z;
            f32 distSq = dx * dx + dy * dy + dz * dz;

            if (distSq > POSITION_THRESHOLD * POSITION_THRESHOLD) {
                positionChanged = true;
            }

            f32 yawDiff = std::abs(yaw - trackData.lastYaw);
            f32 pitchDiff = std::abs(pitch - trackData.lastPitch);

            if (yawDiff > ROTATION_THRESHOLD || pitchDiff > ROTATION_THRESHOLD) {
                rotationChanged = true;
            }
        }

        if (positionChanged || rotationChanged) {
            _broadcastEntityMove(entityId, pos, yaw, pitch);

            trackData.lastPosition = pos;
            trackData.lastYaw = yaw;
            trackData.lastPitch = pitch;
            trackData.needsFullUpdate = false;
        }
    }
}

EntityId EntitySyncManager::spawnEntity(std::unique_ptr<Entity> entity)
{
    if (!entity) {
        return 0;
    }

    EntityId entityId = entity->id();
    Vector3 pos = entity->position();
    f32 yaw = entity->yaw();
    f32 pitch = entity->pitch();

    // 添加到实体管理器
    m_entityManager.addEntity(std::move(entity));

    // 初始化追踪数据
    EntityTrackData& trackData = m_entityTrackData[entityId];
    trackData.lastPosition = pos;
    trackData.lastYaw = yaw;
    trackData.lastPitch = pitch;
    trackData.needsFullUpdate = true;

    // 广播生成
    Entity* spawnedEntity = m_entityManager.getEntity(entityId);
    if (spawnedEntity) {
        _broadcastEntitySpawn(entityId, *spawnedEntity);

        if (m_onEntitySpawn) {
            m_onEntitySpawn(entityId, *spawnedEntity);
        }
    }

    return entityId;
}

std::unique_ptr<Entity> EntitySyncManager::removeEntity(EntityId entityId)
{
    Entity* entity = m_entityManager.getEntity(entityId);

    // 广播移除
    _broadcastEntityRemove(entityId);

    // 移除追踪数据
    m_entityTrackData.erase(entityId);

    // 从实体管理器移除
    auto removedEntity = m_entityManager.removeEntity(entityId);

    if (m_onEntityRemove) {
        m_onEntityRemove(entityId);
    }

    return removedEntity;
}

void EntitySyncManager::forceFullUpdate(EntityId entityId)
{
    auto it = m_entityTrackData.find(entityId);
    if (it != m_entityTrackData.end()) {
        it->second.needsFullUpdate = true;
    }
}

bool EntitySyncManager::_needsSync(EntityId entityId) const
{
    auto it = m_entityTrackData.find(entityId);
    if (it != m_entityTrackData.end()) {
        return it->second.needsFullUpdate;
    }
    return false;
}

void EntitySyncManager::_broadcastEntityMove(EntityId entityId, const Vector3& pos, f32 yaw, f32 pitch)
{
    if (m_onEntityMove) {
        m_onEntityMove(entityId, pos, yaw, pitch);
    }
}

void EntitySyncManager::_broadcastEntitySpawn(EntityId entityId, const Entity& entity)
{
    // 通过回调发送实体生成包（由 MinecraftServer 设置）
    if (m_onEntitySpawn) {
        m_onEntitySpawn(entityId, entity);
    }
}

void EntitySyncManager::_broadcastEntityRemove(EntityId entityId)
{
    // 通过回调发送实体移除包（由 MinecraftServer 设置）
    if (m_onEntityRemove) {
        m_onEntityRemove(entityId);
    }
}

void EntitySyncManager::setOnEntitySpawn(std::function<void(EntityId, const Entity&)> callback)
{
    m_onEntitySpawn = std::move(callback);
}

void EntitySyncManager::setOnEntityRemove(std::function<void(EntityId)> callback)
{
    m_onEntityRemove = std::move(callback);
}

void EntitySyncManager::setOnEntityMove(std::function<void(EntityId, const Vector3&, f32, f32)> callback)
{
    m_onEntityMove = std::move(callback);
}

void EntitySyncManager::setOnEntityStatus(std::function<void(EntityId, u8)> callback)
{
    m_onEntityStatus = std::move(callback);
}

void EntitySyncManager::broadcastEntityStatus(EntityId entityId, u8 status)
{
    if (m_onEntityStatus) {
        m_onEntityStatus(entityId, status);
    }
}

} // namespace mc::server::sync