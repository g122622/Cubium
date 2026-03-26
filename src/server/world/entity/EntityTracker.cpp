#include "EntityTracker.hpp"
#include "server/application/IServer.hpp"
#include "server/core/PlayerManager.hpp"
#include "server/core/ConnectionManager.hpp"
#include "common/network/packet/EntityPackets.hpp"
#include "common/network/packet/PacketSerializer.hpp"
#include "common/entity/Entity.hpp"
#include "common/entity/living/LivingEntity.hpp"
#include "common/entity/mob/MobEntity.hpp"
#include "common/entity/ItemEntity.hpp"
#include "common/item/Item.hpp"
#include "server/core/ServerPlayerData.hpp"
#include "common/world/entity/EntityManager.hpp"
#include <spdlog/spdlog.h>
#include <cmath>

namespace mc::server {

EntityTracker::EntityTracker()
    : m_trackingDistance(10)
    , m_positionUpdateThreshold(0.1f)
    , m_rotationUpdateThreshold(1.0f)
{
}

void EntityTracker::trackEntity(Entity* entity) {
    if (!entity) return;

    std::lock_guard<std::mutex> lock(m_mutex);

    EntityId entityId = entity->id();
    if (m_trackedEntities.find(entityId) != m_trackedEntities.end()) {
        return;  // 已经在追踪
    }

    TrackedEntity tracked;
    tracked.entityId = entityId;
    tracked.lastPosition = entity->position();
    tracked.lastYaw = entity->yaw();
    tracked.lastPitch = entity->pitch();
    tracked.needsFullUpdate = true;

    m_trackedEntities[entityId] = tracked;

    spdlog::debug("EntityTracker: Started tracking entity {} ({})",
                  entityId, entity->getTypeId());
}

void EntityTracker::untrackEntity(EntityId entityId) {
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
    spdlog::debug("EntityTracker: Stopped tracking entity {}", entityId);
}

bool EntityTracker::isTracking(EntityId entityId) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_trackedEntities.find(entityId) != m_trackedEntities.end();
}

size_t EntityTracker::trackedEntityCount() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_trackedEntities.size();
}

void EntityTracker::updatePlayerTracking(IServer& server, PlayerId playerId, const Vector3& playerPos) {
    std::lock_guard<std::mutex> lock(m_mutex);

    // 获取玩家当前追踪的实体集合
    auto& trackedSet = m_playerTrackedEntities[playerId];
    std::vector<EntityId> toStartTracking;
    std::vector<EntityId> toStopTracking;

    // 检查所有被追踪的实体
    for (auto& [entityId, tracked] : m_trackedEntities) {
        Entity* entity = server.entityManager().getEntity(entityId);
        if (!entity) continue;

        // 获取实体追踪范围
        i32 trackingRange = m_trackingDistance; // 默认使用全局追踪距离

        bool shouldTrackEntity = shouldTrack(playerPos, entity->position(), trackingRange);
        bool isTracking = trackedSet.find(entityId) != trackedSet.end();

        if (shouldTrackEntity && !isTracking) {
            toStartTracking.push_back(entityId);
        } else if (!shouldTrackEntity && isTracking) {
            toStopTracking.push_back(entityId);
        }
    }

    // 开始追踪新实体
    for (EntityId entityId : toStartTracking) {
        Entity* entity = server.entityManager().getEntity(entityId);
        if (entity) {
            sendSpawnPacket(server, playerId, entity);
            trackedSet.insert(entityId);
            m_trackedEntities[entityId].trackingPlayers.insert(playerId);
        }
    }

    // 停止追踪实体
    for (EntityId entityId : toStopTracking) {
        sendDestroyPacket(server, playerId, entityId);
        trackedSet.erase(entityId);
        m_trackedEntities[entityId].trackingPlayers.erase(playerId);
    }
}

void EntityTracker::removePlayer(PlayerId playerId) {
    std::lock_guard<std::mutex> lock(m_mutex);

    auto trackedSet = m_playerTrackedEntities.find(playerId);
    if (trackedSet == m_playerTrackedEntities.end()) {
        return;
    }

    // 从所有实体的追踪玩家列表中移除此玩家
    for (EntityId entityId : trackedSet->second) {
        auto it = m_trackedEntities.find(entityId);
        if (it != m_trackedEntities.end()) {
            it->second.trackingPlayers.erase(playerId);
        }
    }

    m_playerTrackedEntities.erase(trackedSet);
    spdlog::debug("EntityTracker: Removed player {} from tracking", playerId);
}

std::vector<EntityId> EntityTracker::getPlayerTrackedEntities(PlayerId playerId) const {
    std::lock_guard<std::mutex> lock(m_mutex);

    std::vector<EntityId> result;
    auto it = m_playerTrackedEntities.find(playerId);
    if (it != m_playerTrackedEntities.end()) {
        result.reserve(it->second.size());
        for (EntityId entityId : it->second) {
            result.push_back(entityId);
        }
    }
    return result;
}

void EntityTracker::tick(IServer& server) {
    std::lock_guard<std::mutex> lock(m_mutex);

    // 更新所有被追踪实体的位置
    for (auto& [entityId, tracked] : m_trackedEntities) {
        Entity* entity = server.entityManager().getEntity(entityId);
        if (!entity || entity->isRemoved()) {
            continue;
        }

        tracked.updateCounter++;

        // 检查是否需要发送位置更新
        Vector3 currentPos = entity->position();
        f32 currentYaw = entity->yaw();
        f32 currentPitch = entity->pitch();

        bool positionChanged = (currentPos - tracked.lastPosition).lengthSquared() >
                              m_positionUpdateThreshold * m_positionUpdateThreshold;
        bool rotationChanged = std::abs(currentYaw - tracked.lastYaw) > m_rotationUpdateThreshold ||
                              std::abs(currentPitch - tracked.lastPitch) > m_rotationUpdateThreshold;

        if (tracked.needsFullUpdate || positionChanged || rotationChanged) {
            // 检查是否有玩家在追踪此实体
            if (!tracked.trackingPlayers.empty()) {
                spdlog::debug("EntityTracker: Entity {} moved from ({:.1f},{:.1f},{:.1f}) to ({:.1f},{:.1f},{:.1f}), sending to {} players",
                              entityId,
                              tracked.lastPosition.x, tracked.lastPosition.y, tracked.lastPosition.z,
                              currentPos.x, currentPos.y, currentPos.z,
                              tracked.trackingPlayers.size());
            }

            // 发送更新包给所有追踪此实体的玩家
            for (PlayerId playerId : tracked.trackingPlayers) {
                if (tracked.needsFullUpdate) {
                    // 发送完整传送包
                    sendMovePacket(server, playerId, entity);
                } else {
                    // 发送相对移动包
                    sendMovePacket(server, playerId, entity);
                }
            }

            tracked.lastPosition = currentPos;
            tracked.lastYaw = currentYaw;
            tracked.lastPitch = currentPitch;
            tracked.needsFullUpdate = false;
        }
    }
}

bool EntityTracker::shouldTrack(const Vector3& playerPos, const Vector3& entityPos, i32 trackingRange) const {
    f32 dx = playerPos.x - entityPos.x;
    f32 dz = playerPos.z - entityPos.z;
    f32 distanceSq = dx * dx + dz * dz;

    f32 rangeBlocks = static_cast<f32>(trackingRange * 16);
    return distanceSq <= rangeBlocks * rangeBlocks;
}

void EntityTracker::sendSpawnPacket(IServer& server, PlayerId playerId, Entity* entity) {
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

        // 生成 UUID（简化：使用实体ID作为基础）
        std::array<u8, 16> uuid = {};
        uuid[0] = static_cast<u8>(entity->id() & 0xFF);
        uuid[1] = static_cast<u8>((entity->id() >> 8) & 0xFF);
        uuid[2] = static_cast<u8>((entity->id() >> 16) & 0xFF);
        uuid[3] = static_cast<u8>((entity->id() >> 24) & 0xFF);
        packet.setUuid(uuid);

        packet.setEntityTypeId(entity->getTypeId());
        packet.setPosition(entity->x(), entity->y(), entity->z());
        // 使用身体的yaw和头部的yaw
        packet.setRotation(entity->yaw(), entity->pitch(), livingEntity->rotationYawHead());

        // 转换速度（m/tick -> 1/8000 block/tick）
        auto velocity = entity->velocity();
        packet.setVelocity(
            static_cast<i16>(std::clamp(velocity.x * 8000.0f, -32768.0f, 32767.0f)),
            static_cast<i16>(std::clamp(velocity.y * 8000.0f, -32768.0f, 32767.0f)),
            static_cast<i16>(std::clamp(velocity.z * 8000.0f, -32768.0f, 32767.0f))
        );

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
            // spdlog::info("Sent SpawnMob packet for entity {} (type: {}) to player {}",
            //               entity->id(), entity->getTypeId(), playerId);
        }
    } else {
        // 非生物实体，使用 SpawnEntityPacket
        network::SpawnEntityPacket packet;
        packet.setEntityId(static_cast<u32>(entity->id()));

        // 生成 UUID（简化：使用实体ID作为基础）
        std::array<u8, 16> uuid = {};
        uuid[0] = static_cast<u8>(entity->id() & 0xFF);
        uuid[1] = static_cast<u8>((entity->id() >> 8) & 0xFF);
        uuid[2] = static_cast<u8>((entity->id() >> 16) & 0xFF);
        uuid[3] = static_cast<u8>((entity->id() >> 24) & 0xFF);
        packet.setUuid(uuid);

        packet.setEntityTypeId(entity->getTypeId());
        packet.setPosition(entity->x(), entity->y(), entity->z());
        packet.setRotation(entity->yaw(), entity->pitch());

        // 转换速度（m/tick -> 1/8000 block/tick）
        auto velocity = entity->velocity();
        packet.setVelocity(
            static_cast<i16>(std::clamp(velocity.x * 8000.0f, -32768.0f, 32767.0f)),
            static_cast<i16>(std::clamp(velocity.y * 8000.0f, -32768.0f, 32767.0f)),
            static_cast<i16>(std::clamp(velocity.z * 8000.0f, -32768.0f, 32767.0f))
        );

        // 检查是否是 ItemEntity，如果是则序列化 ItemStack 数据
        ItemEntity* itemEntity = dynamic_cast<ItemEntity*>(entity);
        if (itemEntity != nullptr) {
            packet.setItemStack(itemEntity->getItemStack());
            const auto& stack = itemEntity->getItemStack();
            spdlog::debug("SpawnEntity packet includes ItemStack: {} x{}",
                          stack.getItem() ? std::to_string(stack.getItem()->itemId()) : "null",
                          stack.getCount());
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
            spdlog::debug("Sent SpawnEntity packet for entity {} (type: {}) to player {}",
                          entity->id(), entity->getTypeId(), playerId);
        }
    }
}

void EntityTracker::sendDestroyPacket(IServer& server, PlayerId playerId, EntityId entityId) {
    ServerPlayerData* player = server.playerManager().getPlayer(playerId);
    if (!player || !player->hasConnection()) return;

    network::EntityDestroyPacket packet;
    packet.addEntityId(static_cast<u32>(entityId));  // EntityId 转 u32（协议限制）

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
        spdlog::debug("Sent EntityDestroy packet for entity {} to player {}", entityId, playerId);
    }
}

void EntityTracker::sendMovePacket(IServer& server, PlayerId playerId, Entity* entity) {
    if (!entity) return;

    ServerPlayerData* player = server.playerManager().getPlayer(playerId);
    if (!player || !player->hasConnection()) return;

    // 发送传送包（完整位置）
    network::EntityTeleportPacket packet;
    packet.setEntityId(static_cast<u32>(entity->id()));  // EntityId 转 u32（协议限制）
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

} // namespace mc::server
