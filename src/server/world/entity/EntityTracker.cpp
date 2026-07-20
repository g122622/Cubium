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

#include "EntityTracker.hpp"
#include "common/core/Constants.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/core/MobEntity.hpp"
#include "common/entity/entities/item/ItemEntity.hpp"
#include "common/entity/entities/orb/ExperienceOrbEntity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/item/core/Item.hpp"
#include "common/network/packet/EntityMetadataSerializer.hpp"
#include "common/network/packet/EntityPackets.hpp"
#include "common/network/packet/ExperiencePackets.hpp"
#include "common/network/packet/PacketSerializer.hpp"
#include "common/profiler/TraceEvents.hpp"
#include "common/util/UuidUtils.hpp"
#include "common/world/entity/EntityManager.hpp"
#include "server/application/IServer.hpp"
#include "server/core/ConnectionManager.hpp"
#include "server/core/PlayerManager.hpp"
#include "server/core/ServerPlayerData.hpp"
#include "server/world/ServerWorld.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <spdlog/spdlog.h>

using namespace mc::trace;

namespace mc::server {

EntityTracker::EntityTracker()
    : m_trackingDistance(10)
    , m_positionUpdateThreshold(0.1f)
    , m_rotationUpdateThreshold(1.0f)
{}

void EntityTracker::trackEntity(Entity* entity)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Entity,
        "EntityTracker::trackEntity",
        "entityId",
        entity ? entity->id() : -1,
        "typeId",
        entity ? entity->getTypeId() : "");

    if (!entity) return;

    std::lock_guard<std::mutex> lock(m_mutex);

    EntityInstanceId entityId = entity->id();
    if (m_trackedEntities.find(entityId) != m_trackedEntities.end()) {
        return; // 已经在追踪
    }

    TrackedEntity tracked;
    tracked.entityId = entityId;
    tracked.lastPosition = entity->position();
    tracked.lastYaw = entity->yaw();
    tracked.lastPitch = entity->pitch();
    tracked.needsFullUpdate = true;

    m_trackedEntities[entityId] = tracked;
}

void EntityTracker::untrackEntity(EntityInstanceId entityId)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_trackedEntities.find(entityId);
    if (it == m_trackedEntities.end()) {
        return;
    }

    // 通知所有正在追踪此实体的玩家
    for (PlayerId playerId : it->second.trackingPlayers) {
        auto playerIt = m_playerTrackedEntities.find(playerId);
        if (playerIt != m_playerTrackedEntities.end()) {
            playerIt->second.erase(entityId);
        }
    }

    m_trackedEntities.erase(it);
}

void EntityTracker::untrackEntity(IServer& server, EntityInstanceId entityId)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Entity, "EntityTracker::untrackEntity", "entityId", entityId);

    std::vector<PlayerId> playersToNotify;

    {
        std::lock_guard<std::mutex> lock(m_mutex);

        auto it = m_trackedEntities.find(entityId);
        if (it == m_trackedEntities.end()) {
            return;
        }

        playersToNotify.assign(it->second.trackingPlayers.begin(), it->second.trackingPlayers.end());
        for (PlayerId playerId : playersToNotify) {
            auto playerIt = m_playerTrackedEntities.find(playerId);
            if (playerIt != m_playerTrackedEntities.end()) {
                playerIt->second.erase(entityId);
            }
        }

        m_trackedEntities.erase(it);
    }

    for (PlayerId playerId : playersToNotify) {
        _sendDestroyPacket(server, playerId, entityId);
    }
}

void EntityTracker::broadcastDestroyToTrackingPlayers(IServer& server, EntityInstanceId entityId)
{
    std::vector<PlayerId> trackingPlayers;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_trackedEntities.find(entityId);
        if (it == m_trackedEntities.end()) {
            return;
        }

        trackingPlayers.assign(it->second.trackingPlayers.begin(), it->second.trackingPlayers.end());
        for (PlayerId playerId : trackingPlayers) {
            auto trackedIt = m_playerTrackedEntities.find(playerId);
            if (trackedIt != m_playerTrackedEntities.end()) {
                trackedIt->second.erase(entityId);
            }
        }
        m_trackedEntities.erase(it);
    }

    _sendDestroyPacket(server, trackingPlayers, entityId);
}

void EntityTracker::broadcastItemEntityResync(IServer& server, const Entity& entity)
{
    std::vector<PlayerId> trackingPlayers;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_trackedEntities.find(entity.id());
        if (it == m_trackedEntities.end()) {
            return;
        }

        trackingPlayers.assign(it->second.trackingPlayers.begin(), it->second.trackingPlayers.end());
        it->second.lastPosition = entity.position();
        it->second.lastYaw = entity.yaw();
        it->second.lastPitch = entity.pitch();
        it->second.needsFullUpdate = false;
    }

    for (PlayerId playerId : trackingPlayers) {
        _sendItemEntityResyncPacket(server, playerId, entity);
    }
}

bool EntityTracker::isTracking(EntityInstanceId entityId) const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_trackedEntities.find(entityId) != m_trackedEntities.end();
}

size_t EntityTracker::trackedEntityCount() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_trackedEntities.size();
}

void EntityTracker::updatePlayerTracking(
    IServer& server, ServerWorld& world, PlayerId playerId, const Vector3& playerPos)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Entity, "EntityTracker::updatePlayerTracking", "playerId", playerId);

    std::lock_guard<std::mutex> lock(m_mutex);

    // 获取玩家当前追踪的实体集合
    auto& trackedSet = m_playerTrackedEntities[playerId];
    std::vector<EntityInstanceId> toStartTracking;
    std::vector<EntityInstanceId> toStopTracking;

    // 检查所有被追踪的实体
    for (auto& [entityId, tracked] : m_trackedEntities) {
        Entity* entity = world.entityManager().getEntity(entityId);
        if (!entity) continue;

        // 获取实体追踪范围
        i32 trackingRange = m_trackingDistance; // 默认使用全局追踪距离

        bool shouldTrackEntity = _shouldTrack(playerPos, entity->position(), trackingRange);
        bool isTracking = trackedSet.find(entityId) != trackedSet.end();

        if (shouldTrackEntity && !isTracking) {
            toStartTracking.push_back(entityId);
        } else if (!shouldTrackEntity && isTracking) {
            toStopTracking.push_back(entityId);
        }
    }

    // 开始追踪新实体
    for (EntityInstanceId entityId : toStartTracking) {
        Entity* entity = world.entityManager().getEntity(entityId);
        if (entity) {
            _sendSpawnPacket(server, playerId, entity);
            trackedSet.insert(entityId);
            m_trackedEntities[entityId].trackingPlayers.insert(playerId);
        }
    }

    // 停止追踪实体
    for (EntityInstanceId entityId : toStopTracking) {
        _sendDestroyPacket(server, playerId, entityId);
        trackedSet.erase(entityId);
        m_trackedEntities[entityId].trackingPlayers.erase(playerId);
    }
}

void EntityTracker::removePlayer(PlayerId playerId)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    auto trackedSet = m_playerTrackedEntities.find(playerId);
    if (trackedSet == m_playerTrackedEntities.end()) {
        return;
    }

    // 从所有实体的追踪玩家列表中移除此玩家
    for (EntityInstanceId entityId : trackedSet->second) {
        auto it = m_trackedEntities.find(entityId);
        if (it != m_trackedEntities.end()) {
            it->second.trackingPlayers.erase(playerId);
        }
    }

    m_playerTrackedEntities.erase(trackedSet);
}

std::vector<EntityInstanceId> EntityTracker::getPlayerTrackedEntities(PlayerId playerId) const
{
    std::lock_guard<std::mutex> lock(m_mutex);

    std::vector<EntityInstanceId> result;
    auto it = m_playerTrackedEntities.find(playerId);
    if (it != m_playerTrackedEntities.end()) {
        result.reserve(it->second.size());
        for (EntityInstanceId entityId : it->second) {
            result.push_back(entityId);
        }
    }
    return result;
}

void EntityTracker::tick(IServer& server, ServerWorld& world)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Entity, "EntityTracker::tick", "trackedCount", m_trackedEntities.size());

    std::vector<std::pair<EntityInstanceId, std::vector<PlayerId>>> removedEntities;
    std::vector<EntityInstanceId> entitiesToErase;

    {
        std::lock_guard<std::mutex> lock(m_mutex);

        for (auto& [entityId, tracked] : m_trackedEntities) {
            Entity* entity = world.entityManager().getEntity(entityId);
            if (!entity || entity->isRemoved()) {
                std::vector<PlayerId> players(tracked.trackingPlayers.begin(), tracked.trackingPlayers.end());
                removedEntities.emplace_back(entityId, std::move(players));
                entitiesToErase.push_back(entityId);
                continue;
            }

            tracked.updateCounter++;

            Vector3 currentPos = entity->position();
            f32 currentYaw = entity->yaw();
            f32 currentPitch = entity->pitch();

            bool positionChanged = (currentPos - tracked.lastPosition).lengthSquared() >
                m_positionUpdateThreshold * m_positionUpdateThreshold;
            bool rotationChanged = std::abs(currentYaw - tracked.lastYaw) > m_rotationUpdateThreshold ||
                std::abs(currentPitch - tracked.lastPitch) > m_rotationUpdateThreshold;

            if (tracked.needsFullUpdate || positionChanged || rotationChanged) {
                for (PlayerId playerId : tracked.trackingPlayers) {
                    _sendMovePacket(server, playerId, entity);
                }

                if (entity->dataManager().hasDirtyData() && !tracked.trackingPlayers.empty()) {
                    std::vector<u8> metadata =
                        network::EntityMetadataSerializer::serialize(entity->dataManager(), true);
                    if (metadata.size() > 1) {
                        for (PlayerId playerId : tracked.trackingPlayers) {
                            _sendMetadataPacket(server, playerId, entity, metadata);
                        }
                        entity->dataManager().clearDirty();
                    }
                }

                tracked.lastPosition = currentPos;
                tracked.lastYaw = currentYaw;
                tracked.lastPitch = currentPitch;
                tracked.needsFullUpdate = false;
            }

            // 速度同步：当实体的 hurtMarked 为 true 时，发送速度同步包
            // 注意：当 Player::causeExtraKnockback() 为 ServerPlayer 目标发送速度包后，
            // 会立即清除 hurtMarked，此分支不会执行，从而避免速度重复应用。
            if (entity->isHurtMarked()) {
                // 向所有追踪此实体的玩家发送速度同步包
                for (PlayerId playerId : tracked.trackingPlayers) {
                    _sendVelocityPacket(server, playerId, entity);
                }

                // 如果实体本身是 Player，也需要向其自身发送速度同步包
                // 即 "AndSelf" 模式：ServerPlayer 不会追踪自身，因此需要单独发送
                // 通过 Player::sendVelocityPacket() 虚方法，ServerPlayer 会实际发送网络包，
                // 而 Player 基类版本为空操作
                if (auto* playerEntity = dynamic_cast<Player*>(entity)) {
                    // 返回值不需要处理：此处是广播场景，发送失败也不影响逻辑
                    (void)playerEntity->sendVelocityPacket();
                }

                entity->clearHurtMarked();
            }
        }

        for (EntityInstanceId entityId : entitiesToErase) {
            for (auto& [_, trackedSet] : m_playerTrackedEntities) {
                trackedSet.erase(entityId);
            }
            m_trackedEntities.erase(entityId);
        }
    }

    for (const auto& [entityId, players] : removedEntities) {
        for (PlayerId playerId : players) {
            _sendDestroyPacket(server, playerId, entityId);
        }
    }
}

bool EntityTracker::_shouldTrack(const Vector3& playerPos, const Vector3& entityPos, i32 trackingRange) const
{
    f32 dx = playerPos.x - entityPos.x;
    f32 dz = playerPos.z - entityPos.z;
    f32 distanceSq = dx * dx + dz * dz;

    f32 rangeBlocks = static_cast<f32>(trackingRange * mc::world::CHUNK_WIDTH);
    return distanceSq <= rangeBlocks * rangeBlocks;
}

void EntityTracker::_sendSpawnPacket(IServer& server, PlayerId playerId, Entity* entity)
{
    if (!entity) return;

    // 获取玩家数据
    ServerPlayerData* player = server.playerManager().getPlayer(playerId);
    if (!player || !player->hasConnection()) return;

    // 判断是 Mob 还是普通实体
    // MobEntity 继承自 LivingEntity，使用 SpawnMobPacket
    // 其他实体使用 SpawnEntityPacket
    auto* livingEntity = dynamic_cast<LivingEntity*>(entity);

    if (livingEntity != nullptr) {
        // 使用 SpawnMobPacket（包含 headYaw）
        network::SpawnMobPacket packet;
        packet.setEntityId(static_cast<u32>(entity->id()));

        // 使用实体的真实 UUID（MC 1.16.5 行为：UUID 在实体构造时随机生成）
        packet.setUuid(util::uuidFromString(entity->uuid()));

        packet.setEntityTypeId(entity->getTypeId());
        packet.setPosition(entity->x(), entity->y(), entity->z());
        // 使用身体的yaw和头部的yaw
        packet.setRotation(entity->yaw(), entity->pitch(), livingEntity->rotationYawHead());

        // 转换速度（m/tick -> 1/8000 block/tick）
        auto velocity = entity->velocity();
        packet.setVelocity(static_cast<i16>(std::clamp(velocity.x * 8000.0f, -32768.0f, 32767.0f)),
            static_cast<i16>(std::clamp(velocity.y * 8000.0f, -32768.0f, 32767.0f)),
            static_cast<i16>(std::clamp(velocity.z * 8000.0f, -32768.0f, 32767.0f)));

        packet.setMetadata(network::EntityMetadataSerializer::serialize(entity->dataManager(), false));

        auto result = packet.serialize();
        if (result.success()) {
            // 封装为完整数据包
            network::PacketSerializer fullPacket;
            fullPacket.writeU32(static_cast<u32>(network::PACKET_HEADER_SIZE + result.value().size()));
            fullPacket.writeU16(static_cast<u16>(network::PacketType::SpawnMob));
            fullPacket.writeU16(0);
            fullPacket.writeU16(0);
            fullPacket.writeU16(0);
            fullPacket.writeBytes(result.value());

            player->send(fullPacket.data(), fullPacket.size());
            entity->dataManager().clearDirty();
            // spdlog::info("Sent SpawnMob packet for entity {} (type: {}) to player {}",
            //               entity->id(), entity->getTypeId(), playerId);
        }
    } else {
        // 非生物实体，检查是否是 ExperienceOrbEntity
        ExperienceOrbEntity* xpOrb = dynamic_cast<ExperienceOrbEntity*>(entity);
        if (xpOrb != nullptr) {
            // 经验球使用 SpawnExperienceOrbPacket
            network::SpawnExperienceOrbPacket packet(static_cast<i32>(entity->id()),
                entity->x(),
                entity->y(),
                entity->z(),
                static_cast<i16>(xpOrb->getXpValue()));

            auto result = packet.serialize();
            if (result.success()) {
                network::PacketSerializer fullPacket;
                fullPacket.writeU32(static_cast<u32>(network::PACKET_HEADER_SIZE + result.value().size()));
                fullPacket.writeU16(static_cast<u16>(network::PacketType::SpawnExperienceOrb));
                fullPacket.writeU16(0);
                fullPacket.writeU16(0);
                fullPacket.writeU16(0);
                fullPacket.writeBytes(result.value());

                player->send(fullPacket.data(), fullPacket.size());
            }
        } else {
            // 其他非生物实体，使用 SpawnEntityPacket
            network::SpawnEntityPacket packet;
            packet.setEntityId(static_cast<u32>(entity->id()));

            // 使用实体的真实 UUID（MC 1.16.5 行为：UUID 在实体构造时随机生成）
            packet.setUuid(util::uuidFromString(entity->uuid()));

            packet.setEntityTypeId(entity->getTypeId());
            packet.setPosition(entity->x(), entity->y(), entity->z());
            packet.setRotation(entity->yaw(), entity->pitch());

            // 转换速度（m/tick -> 1/8000 block/tick）
            auto velocity = entity->velocity();
            packet.setVelocity(static_cast<i16>(std::clamp(velocity.x * 8000.0f, -32768.0f, 32767.0f)),
                static_cast<i16>(std::clamp(velocity.y * 8000.0f, -32768.0f, 32767.0f)),
                static_cast<i16>(std::clamp(velocity.z * 8000.0f, -32768.0f, 32767.0f)));

            // 检查是否是 ItemEntity，如果是则序列化 ItemStack 数据
            ItemEntity* itemEntity = dynamic_cast<ItemEntity*>(entity);
            if (itemEntity != nullptr) {
                packet.setItemStack(itemEntity->getItemStack());
            }

            auto result = packet.serialize();
            if (result.success()) {
                // 封装为完整数据包
                network::PacketSerializer fullPacket;
                fullPacket.writeU32(static_cast<u32>(network::PACKET_HEADER_SIZE + result.value().size()));
                fullPacket.writeU16(static_cast<u16>(network::PacketType::SpawnEntity));
                fullPacket.writeU16(0);
                fullPacket.writeU16(0);
                fullPacket.writeU16(0);
                fullPacket.writeBytes(result.value());

                player->send(fullPacket.data(), fullPacket.size());
            }
        }
    }
}

void EntityTracker::_sendMetadataPacket(
    IServer& server, PlayerId playerId, Entity* entity, const std::vector<u8>& metadata)
{
    if (!entity || metadata.empty()) return;

    ServerPlayerData* player = server.playerManager().getPlayer(playerId);
    if (!player || !player->hasConnection()) return;

    network::EntityMetadataPacket packet;
    packet.setEntityId(static_cast<u32>(entity->id()));
    packet.setMetadata(metadata);

    auto result = packet.serialize();
    if (result.success()) {
        network::PacketSerializer fullPacket;
        fullPacket.writeU32(static_cast<u32>(network::PACKET_HEADER_SIZE + result.value().size()));
        fullPacket.writeU16(static_cast<u16>(network::PacketType::EntityMetadata));
        fullPacket.writeU16(0);
        fullPacket.writeU16(0);
        fullPacket.writeU16(0);
        fullPacket.writeBytes(result.value());

        player->send(fullPacket.data(), fullPacket.size());
    }
}

void EntityTracker::_sendDestroyPacket(IServer& server, PlayerId playerId, EntityInstanceId entityId)
{
    ServerPlayerData* player = server.playerManager().getPlayer(playerId);
    if (!player || !player->hasConnection()) return;

    network::EntityDestroyPacket packet;
    packet.addEntityId(static_cast<u32>(entityId)); // EntityInstanceId 转 u32（协议限制）

    auto result = packet.serialize();
    if (result.success()) {
        network::PacketSerializer fullPacket;
        fullPacket.writeU32(static_cast<u32>(network::PACKET_HEADER_SIZE + result.value().size()));
        fullPacket.writeU16(static_cast<u16>(network::PacketType::EntityDestroy));
        fullPacket.writeU16(0);
        fullPacket.writeU16(0);
        fullPacket.writeU16(0);
        fullPacket.writeBytes(result.value());

        player->send(fullPacket.data(), fullPacket.size());
    }
}

void EntityTracker::_sendDestroyPacket(
    IServer& server, const std::vector<PlayerId>& playerIds, EntityInstanceId entityId)
{
    for (PlayerId playerId : playerIds) {
        _sendDestroyPacket(server, playerId, entityId);
    }
}

void EntityTracker::_sendMovePacket(IServer& server, PlayerId playerId, Entity* entity)
{
    if (!entity) return;

    ServerPlayerData* player = server.playerManager().getPlayer(playerId);
    if (!player || !player->hasConnection()) return;

    // 发送传送包（完整位置）
    network::EntityTeleportPacket packet;
    packet.setEntityId(static_cast<u32>(entity->id())); // EntityInstanceId 转 u32（协议限制）
    packet.setPosition(entity->x(), entity->y(), entity->z());
    packet.setRotation(entity->yaw(), entity->pitch());
    packet.setOnGround(entity->onGround());

    auto result = packet.serialize();
    if (result.success()) {
        network::PacketSerializer fullPacket;
        fullPacket.writeU32(static_cast<u32>(network::PACKET_HEADER_SIZE + result.value().size()));
        fullPacket.writeU16(static_cast<u16>(network::PacketType::EntityTeleport));
        fullPacket.writeU16(0);
        fullPacket.writeU16(0);
        fullPacket.writeU16(0);
        fullPacket.writeBytes(result.value());

        player->send(fullPacket.data(), fullPacket.size());
    }
}

void EntityTracker::_sendVelocityPacket(IServer& server, PlayerId playerId, Entity* entity)
{
    if (!entity) return;

    ServerPlayerData* player = server.playerManager().getPlayer(playerId);
    if (!player || !player->hasConnection()) return;

    // 发送实体速度同步包
    // 速度单位：1/8000 block/tick（与 SpawnMobPacket/SpawnEntityPacket 一致）
    network::EntityVelocityPacket packet;
    packet.setEntityId(static_cast<u32>(entity->id()));
    const auto velocity = entity->velocity();
    packet.setVelocity(static_cast<i16>(std::clamp(velocity.x * 8000.0f, -32768.0f, 32767.0f)),
        static_cast<i16>(std::clamp(velocity.y * 8000.0f, -32768.0f, 32767.0f)),
        static_cast<i16>(std::clamp(velocity.z * 8000.0f, -32768.0f, 32767.0f)));

    auto result = packet.serialize();
    if (result.success()) {
        network::PacketSerializer fullPacket;
        fullPacket.writeU32(static_cast<u32>(network::PACKET_HEADER_SIZE + result.value().size()));
        fullPacket.writeU16(static_cast<u16>(network::PacketType::EntityVelocity));
        fullPacket.writeU16(0);
        fullPacket.writeU16(0);
        fullPacket.writeU16(0);
        fullPacket.writeBytes(result.value());

        player->send(fullPacket.data(), fullPacket.size());
    }
}

void EntityTracker::_sendItemEntityResyncPacket(IServer& server, PlayerId playerId, const Entity& entity)
{
    auto* itemEntity = dynamic_cast<const ItemEntity*>(&entity);
    if (itemEntity == nullptr) {
        return;
    }

    ServerPlayerData* player = server.playerManager().getPlayer(playerId);
    if (!player || !player->hasConnection()) {
        return;
    }

    network::SpawnEntityPacket packet;
    packet.setEntityId(static_cast<u32>(itemEntity->id()));
    packet.setUuid(util::uuidFromString(itemEntity->uuid()));
    packet.setEntityTypeId(itemEntity->getTypeId());
    packet.setPosition(itemEntity->x(), itemEntity->y(), itemEntity->z());
    packet.setRotation(itemEntity->yaw(), itemEntity->pitch());

    const auto velocity = itemEntity->velocity();
    packet.setVelocity(static_cast<i16>(std::clamp(velocity.x * 8000.0f, -32768.0f, 32767.0f)),
        static_cast<i16>(std::clamp(velocity.y * 8000.0f, -32768.0f, 32767.0f)),
        static_cast<i16>(std::clamp(velocity.z * 8000.0f, -32768.0f, 32767.0f)));
    packet.setItemStack(itemEntity->getItemStack());

    auto result = packet.serialize();
    if (result.failed()) {
        return;
    }

    network::PacketSerializer fullPacket;
    fullPacket.writeU32(static_cast<u32>(network::PACKET_HEADER_SIZE + result.value().size()));
    fullPacket.writeU16(static_cast<u16>(network::PacketType::SpawnEntity));
    fullPacket.writeU16(0);
    fullPacket.writeU16(0);
    fullPacket.writeU16(0);
    fullPacket.writeBytes(result.value());
    player->send(fullPacket.data(), fullPacket.size());
}

} // namespace mc::server
