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
#include "common/core/Types.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/core/MobEntity.hpp"
#include "common/entity/entities/item/ItemEntity.hpp"
#include "common/entity/entities/orb/ExperienceOrbEntity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/item/core/Item.hpp"
#include "common/network/backend/java/codecs/JavaWireHelpers.hpp"
#include "common/network/codec/EntityMetadataSerializer.hpp"
#include "common/network/ir/IrPacket.hpp"
#include "common/network/ir/packets/play/PlayPackets.hpp"
#include "common/network/protocol/ConnectionProtocol.hpp"
#include "common/profiler/TraceCategories.hpp"
#include "common/profiler/TraceEvents.hpp"
#include "common/util/UuidUtils.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/entity/EntityManager.hpp"
#include "server/application/IServer.hpp"
#include "server/core/PlayerManager.hpp"
#include "server/core/ServerPlayerData.hpp"
#include "server/world/ServerWorld.hpp"
#include <array>
#include <cmath>
#include <cstddef>
#include <mutex>
#include <utility>
#include <vector>

using namespace mc::trace;

namespace mc::server {

namespace {

// 构造 Play 阶段 IR 包的统一包装（所有实体同步 S→C 包均为 Play 阶段）。
mc::network::ir::IrPacket makePlayPacket(mc::network::ir::PlayPacket play)
{
    return mc::network::ir::IrPacket{
        mc::network::protocol::ConnectionProtocol::Play,
        std::move(play),
    };
}

} // namespace

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

void EntityTracker::notifyEntityTracked(IServer& server, ServerWorld& world, Entity* entity)
{
    if (entity == nullptr) {
        return;
    }

    const EntityInstanceId entityId = entity->id();

    // 1. 收集在线玩家及其位置（playerManager 内部线程安全，无需持 tracker 锁）。
    struct PlayerView {
        PlayerId playerId;
        Vector3 position;
    };
    std::vector<PlayerView> onlinePlayers;
    const std::vector<PlayerId> playerIds = server.playerManager().getPlayerIds();
    onlinePlayers.reserve(playerIds.size());
    for (PlayerId playerId : playerIds) {
        const ServerPlayerData* data = server.playerManager().getPlayer(playerId);
        if (data == nullptr) {
            continue;
        }
        const Vector3f pos = data->position();
        onlinePlayers.push_back({playerId, Vector3(pos.x, pos.y, pos.z)});
    }

    if (onlinePlayers.empty()) {
        return;
    }

    // 2. 加锁：确认实体仍在追踪表，并收集当前已追踪该实体的玩家集合，
    //    同时判定每个在线玩家是否应开始追踪（视距内、非自身）。
    std::vector<PlayerId> toStartTracking;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_trackedEntities.find(entityId);
        if (it == m_trackedEntities.end()) {
            // 并发 untrack：实体已不在追踪表，无需广播。
            return;
        }
        const std::unordered_set<PlayerId>& alreadyTracking = it->second.trackingPlayers;

        for (const PlayerView& pv : onlinePlayers) {
            // 跳过玩家自身：本地玩家由 login 包直接建立，不应收到自己的 AddEntity
            // （否则客户端 spawnEntity 会撞已存在的本地玩家并误回写位置）。
            if (auto* player = dynamic_cast<Player*>(entity); player != nullptr && player->playerId() == pv.playerId) {
                continue;
            }
            if (alreadyTracking.find(pv.playerId) != alreadyTracking.end()) {
                continue; // 已在追踪
            }
            if (_shouldTrack(pv.position, entity->position(), m_trackingDistance)) {
                toStartTracking.push_back(pv.playerId);
            }
        }
    }

    if (toStartTracking.empty()) {
        return;
    }

    // 3. 锁外发包（player->send 可能回调入 tracker，持锁有死锁风险）。
    for (PlayerId playerId : toStartTracking) {
        _sendSpawnPacket(server, playerId, entity);
    }

    // 4. 加锁登记双向追踪关系。
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_trackedEntities.find(entityId);
        if (it == m_trackedEntities.end()) {
            return; // 并发 untrack
        }
        for (PlayerId playerId : toStartTracking) {
            it->second.trackingPlayers.insert(playerId);
            m_playerTrackedEntities[playerId].insert(entityId);
        }
    }
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

void EntityTracker::broadcastPassengers(IServer& server, ServerWorld& world, EntityInstanceId vehicleId)
{
    std::vector<PlayerId> trackingPlayers;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_trackedEntities.find(vehicleId);
        if (it == m_trackedEntities.end()) {
            return;
        }
        trackingPlayers.assign(it->second.trackingPlayers.begin(), it->second.trackingPlayers.end());
    }

    if (trackingPlayers.empty()) {
        return;
    }

    // 载具实体可能尚未进入追踪表（如 spawn 时刻立即挂载乘客），这里按 id 现取。
    Entity* vehicle = world.entityManager().getEntity(vehicleId);
    if (vehicle == nullptr) {
        return;
    }

    // 1.21.11 SetPassengers：VarInt(vehicle) + VarInt(count) + count×VarInt(passenger)。
    mc::network::ir::play::SetPassengers pkt;
    pkt.vehicle = static_cast<i32>(vehicleId);
    pkt.passengers.reserve(vehicle->getPassengers().size());
    for (EntityInstanceId passengerId : vehicle->getPassengers()) {
        pkt.passengers.push_back(static_cast<i32>(passengerId));
    }
    auto packet = makePlayPacket(mc::network::ir::PlayPacket{pkt});

    for (PlayerId playerId : trackingPlayers) {
        ServerPlayerData* player = server.playerManager().getPlayer(playerId);
        if (player != nullptr && player->hasConnection()) {
            player->send(packet);
        }
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

        // 跳过玩家自身实体：本地玩家由 login 包直接建立，不应再收到自己的
        // SpawnEntity/SpawnMob 包，否则客户端 spawnEntity 会撞已存在的本地玩家
        // 触发 "Entity with ID already exists" warning，并误把自身位置回写。
        if (auto* player = dynamic_cast<Player*>(entity); player && player->playerId() == playerId) {
            continue;
        }

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

            // 位置/朝向变更：发送移动包（_sendMovePacket 内部按位移量选 delta 或 teleport）。
            // 注意：metadata 的发送已从此门内拆出独立判定——静止实体若仅 metadata 变化
            // （ItemEntity 物品变、Mob 目标/状态变、Creeper 膨胀等）也必须同步，
            // 否则客户端永不更新（旧实现把 metadata 发送嵌在位置门内，静止实体丢元数据）。
            if (tracked.needsFullUpdate || positionChanged || rotationChanged) {
                for (PlayerId playerId : tracked.trackingPlayers) {
                    _sendMovePacket(server, playerId, entity, tracked, positionChanged, rotationChanged);
                }

                tracked.lastPosition = currentPos;
                tracked.lastYaw = currentYaw;
                tracked.lastPitch = currentPitch;
                tracked.needsFullUpdate = false;
            }

            // 头部旋转同步：LivingEntity 的 headYaw 变化时发 RotateHead。
            // AddEntity 时刻已透传 yHeadRot 一次；此处负责之后的持续同步。
            // 对齐 MC Java ServerEntity：headRot 与 bodyRot 分离追踪，headRot 变化即发包。
            if (auto* living = dynamic_cast<LivingEntity*>(entity); living != nullptr) {
                const f32 currentHeadYaw = living->rotationYawHead();
                if (!tracked.lastHeadYawValid ||
                    std::abs(currentHeadYaw - tracked.lastHeadYaw) > m_rotationUpdateThreshold) {
                    for (PlayerId playerId : tracked.trackingPlayers) {
                        _sendHeadRotatePacket(server, playerId, entity);
                    }
                    tracked.lastHeadYaw = currentHeadYaw;
                    tracked.lastHeadYawValid = true;
                }
            }

            // 元数据同步：独立于位置门。只要 dataManager 有脏数据就序列化下发，
            // 与实体是否移动无关。对齐 MC Java ServerEntity.sendDirtyEntityData()
            // 对 SynchedEntityData 脏条目的单独刷新路径。
            if (entity->dataManager().hasDirtyData() && !tracked.trackingPlayers.empty()) {
                std::vector<u8> metadata = network::EntityMetadataSerializer::serialize(entity->dataManager(), true);
                if (metadata.size() > 1) { // >1 表示除 0xFF 结束符外还有实际条目
                    for (PlayerId playerId : tracked.trackingPlayers) {
                        _sendMetadataPacket(server, playerId, entity, metadata);
                    }
                }
                entity->dataManager().clearDirty();
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

    // 1.21.11 统一用 AddEntity 生成实体（含经验球/掉落物/Mob），
    // 元数据走独立的 SetEntityData 包。MobEntity 的 headYaw 经 AddEntity.yHeadRot 透传。
    auto* livingEntity = dynamic_cast<LivingEntity*>(entity);
    // entityTypeId 必须用 vanilla 1.21.11 entity_type 注册表 id（entity_type 注册表不在 Configuration
    // 同步的 23 个注册表内，真 Java 客户端用内置 core 包 id 解析）。项目内部 EntityType::id() 是
    // registerType 注册序，与 vanilla 字母序不同，直接发会让客户端 spawn 错误实体类型。
    // 船类按木种选变体，其余按名查 JavaEntityTypeIdMap。见 Entity::getJavaEntityTypeId()。
    const i32 entityTypeId = static_cast<i32>(entity->getJavaEntityTypeId());

    mc::network::ir::play::AddEntity spawn;
    spawn.entityId = static_cast<i32>(entity->id());
    spawn.uuid = util::uuidFromString(entity->uuid());
    spawn.entityTypeId = entityTypeId;
    spawn.x = entity->x();
    spawn.y = entity->y();
    spawn.z = entity->z();
    // 1.21.11 movement 用 LpVec3（低精度 m/tick），此处直接填 m/tick 值。
    auto velocity = entity->velocity();
    spawn.movementX = static_cast<f64>(velocity.x);
    spawn.movementY = static_cast<f64>(velocity.y);
    spawn.movementZ = static_cast<f64>(velocity.z);
    spawn.yRot = static_cast<i8>(mc::network::backend::java::wire::packDegrees(entity->yaw()));
    spawn.xRot = static_cast<i8>(mc::network::backend::java::wire::packDegrees(entity->pitch()));
    spawn.yHeadRot = livingEntity != nullptr
        ? static_cast<i8>(mc::network::backend::java::wire::packDegrees(livingEntity->rotationYawHead()))
        : spawn.yRot;
    // 实体特定 data：对齐 vanilla ClientboundAddEntityPacket.entityData。
    // FallingBlockEntity 据此下发 BlockState 的 stateId（vanilla 同路径），其余实体默认 0。
    spawn.data = entity->getSpawnData();

    player->send(makePlayPacket(mc::network::ir::PlayPacket{spawn}));

    // 紧随其后发送元数据（spawn 时刻的完整快照，dirtyOnly=false）。
    // ItemEntity 的 ItemStack 等关键状态都走元数据，客户端收到 AddEntity 后必须再收一次
    // SetEntityData 才能正确渲染（1.21.11 AddEntity 不携带 metadata）。
    std::vector<u8> metadata = network::EntityMetadataSerializer::serialize(entity->dataManager(), false);
    if (metadata.size() > 1) { // >1 表示除 0xFF 结束符外还有实际条目
        mc::network::ir::play::SetEntityData meta;
        meta.entityId = static_cast<i32>(entity->id());
        meta.packedItems = std::move(metadata);
        player->send(makePlayPacket(mc::network::ir::PlayPacket{std::move(meta)}));
    }
    entity->dataManager().clearDirty();
}

void EntityTracker::_sendMetadataPacket(
    IServer& server, PlayerId playerId, Entity* entity, const std::vector<u8>& metadata)
{
    if (!entity || metadata.empty()) return;

    ServerPlayerData* player = server.playerManager().getPlayer(playerId);
    if (!player || !player->hasConnection()) return;

    // 1.21.11 SetEntityData：entityId + 已序列化的元数据字节（含 EOF 0xFF）。
    mc::network::ir::play::SetEntityData pkt;
    pkt.entityId = static_cast<i32>(entity->id());
    pkt.packedItems = metadata;
    player->send(makePlayPacket(mc::network::ir::PlayPacket{std::move(pkt)}));
}

void EntityTracker::_sendDestroyPacket(IServer& server, PlayerId playerId, EntityInstanceId entityId)
{
    ServerPlayerData* player = server.playerManager().getPlayer(playerId);
    if (!player || !player->hasConnection()) return;

    // 1.21.11 RemoveEntities：VarInt(count) + count×VarInt(entityId)。
    mc::network::ir::play::RemoveEntities pkt;
    pkt.entityIds.push_back(static_cast<i32>(entityId));
    player->send(makePlayPacket(mc::network::ir::PlayPacket{std::move(pkt)}));
}

void EntityTracker::_sendDestroyPacket(
    IServer& server, const std::vector<PlayerId>& playerIds, EntityInstanceId entityId)
{
    for (PlayerId playerId : playerIds) {
        _sendDestroyPacket(server, playerId, entityId);
    }
}

void EntityTracker::_sendMovePacket(IServer& server,
    PlayerId playerId,
    Entity* entity,
    const TrackedEntity& tracked,
    bool positionChanged,
    bool rotationChanged)
{
    if (!entity) return;

    ServerPlayerData* player = server.playerManager().getPlayer(playerId);
    if (!player || !player->hasConnection()) return;

    // 计算 1/4096 定点 delta（对齐 vanilla 1.21.11 MoveEntity*.deltaX/Y/Z: Short）。
    // 超出 i16 范围（±32767，即 ±8 格）则回退 TeleportEntity 绝对位置包——
    // vanilla ServerEntity 同样在 delta 超 Short 范围时改发 ClientboundTeleportEntityPacket。
    const f32 kFixedPoint = 4096.0f;
    auto toDeltaI16 = [kFixedPoint](f32 current, f32 last) -> bool {
        const f32 scaled = (current - last) * kFixedPoint;
        return scaled > -32768.0f && scaled < 32767.0f;
    };

    const bool deltaInRange = toDeltaI16(entity->x(), tracked.lastPosition.x) &&
        toDeltaI16(entity->y(), tracked.lastPosition.y) && toDeltaI16(entity->z(), tracked.lastPosition.z);

    // needsFullUpdate（首次/重同步）或 delta 超 i16 范围：发绝对 TeleportEntity。
    // delta 置 0、relatives 置 0 表示纯绝对传送。
    if (tracked.needsFullUpdate || (positionChanged && !deltaInRange)) {
        mc::network::ir::play::TeleportEntity pkt;
        pkt.entityId = static_cast<i32>(entity->id());
        pkt.x = static_cast<f64>(entity->x());
        pkt.y = static_cast<f64>(entity->y());
        pkt.z = static_cast<f64>(entity->z());
        pkt.deltaX = 0.0;
        pkt.deltaY = 0.0;
        pkt.deltaZ = 0.0;
        pkt.yRot = entity->yaw();
        pkt.xRot = entity->pitch();
        pkt.relatives = 0;
        pkt.onGround = entity->onGround();
        player->send(makePlayPacket(mc::network::ir::PlayPacket{std::move(pkt)}));
        return;
    }

    // 低带宽 delta 包：客户端用 setTargetPosition/setTargetRotation 做插值，视觉平滑。
    const auto packDelta = [kFixedPoint](f32 current, f32 last) -> i16 {
        return static_cast<i16>(std::round((current - last) * kFixedPoint));
    };
    const i8 yRotPacked = static_cast<i8>(mc::network::backend::java::wire::packDegrees(entity->yaw()));
    const i8 xRotPacked = static_cast<i8>(mc::network::backend::java::wire::packDegrees(entity->pitch()));

    if (positionChanged && rotationChanged) {
        mc::network::ir::play::MoveEntityPosRot pkt;
        pkt.entityId = static_cast<i32>(entity->id());
        pkt.deltaX = packDelta(entity->x(), tracked.lastPosition.x);
        pkt.deltaY = packDelta(entity->y(), tracked.lastPosition.y);
        pkt.deltaZ = packDelta(entity->z(), tracked.lastPosition.z);
        pkt.yRot = yRotPacked;
        pkt.xRot = xRotPacked;
        pkt.onGround = entity->onGround();
        player->send(makePlayPacket(mc::network::ir::PlayPacket{std::move(pkt)}));
    } else if (positionChanged) {
        mc::network::ir::play::MoveEntityPos pkt;
        pkt.entityId = static_cast<i32>(entity->id());
        pkt.deltaX = packDelta(entity->x(), tracked.lastPosition.x);
        pkt.deltaY = packDelta(entity->y(), tracked.lastPosition.y);
        pkt.deltaZ = packDelta(entity->z(), tracked.lastPosition.z);
        pkt.onGround = entity->onGround();
        player->send(makePlayPacket(mc::network::ir::PlayPacket{std::move(pkt)}));
    } else if (rotationChanged) {
        mc::network::ir::play::MoveEntityRot pkt;
        pkt.entityId = static_cast<i32>(entity->id());
        pkt.yRot = yRotPacked;
        pkt.xRot = xRotPacked;
        pkt.onGround = entity->onGround();
        player->send(makePlayPacket(mc::network::ir::PlayPacket{std::move(pkt)}));
    }
}

void EntityTracker::_sendHeadRotatePacket(IServer& server, PlayerId playerId, Entity* entity)
{
    if (entity == nullptr) {
        return;
    }

    ServerPlayerData* player = server.playerManager().getPlayer(playerId);
    if (player == nullptr || !player->hasConnection()) {
        return;
    }

    // 1.21.11 RotateHead：entityId + Byte(yHeadRot)。仅 LivingEntity 有 headYaw 概念。
    auto* living = dynamic_cast<LivingEntity*>(entity);
    if (living == nullptr) {
        return;
    }

    mc::network::ir::play::RotateHead pkt;
    pkt.entityId = static_cast<i32>(entity->id());
    pkt.yHeadRot = static_cast<i8>(mc::network::backend::java::wire::packDegrees(living->rotationYawHead()));
    player->send(makePlayPacket(mc::network::ir::PlayPacket{std::move(pkt)}));
}

void EntityTracker::_sendVelocityPacket(IServer& server, PlayerId playerId, Entity* entity)
{
    if (!entity) return;

    ServerPlayerData* player = server.playerManager().getPlayer(playerId);
    if (!player || !player->hasConnection()) return;

    // 1.21.11 SetEntityMotion：entityId + LpVec3 速度（m/tick）。
    // 旧路径用 1/8000 i16；1.21.11 codec 用 LpVec3 直接承载 double。
    const auto velocity = entity->velocity();
    mc::network::ir::play::SetEntityMotion pkt;
    pkt.entityId = static_cast<i32>(entity->id());
    pkt.x = static_cast<f64>(velocity.x);
    pkt.y = static_cast<f64>(velocity.y);
    pkt.z = static_cast<f64>(velocity.z);
    player->send(makePlayPacket(mc::network::ir::PlayPacket{std::move(pkt)}));
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

    // 重同步：重新发一次 AddEntity + SetEntityData 全量快照（与 _sendSpawnPacket 同构），
    // ItemEntity 的 ItemStack 状态由元数据承载，重新序列化即可刷新到最新。
    // entityTypeId 用 vanilla id（同 _sendSpawnPacket，见 Entity::getJavaEntityTypeId()）。
    const i32 entityTypeId = static_cast<i32>(itemEntity->getJavaEntityTypeId());

    mc::network::ir::play::AddEntity spawn;
    spawn.entityId = static_cast<i32>(itemEntity->id());
    spawn.uuid = util::uuidFromString(itemEntity->uuid());
    spawn.entityTypeId = entityTypeId;
    spawn.x = static_cast<f64>(itemEntity->x());
    spawn.y = static_cast<f64>(itemEntity->y());
    spawn.z = static_cast<f64>(itemEntity->z());
    const auto velocity = itemEntity->velocity();
    spawn.movementX = static_cast<f64>(velocity.x);
    spawn.movementY = static_cast<f64>(velocity.y);
    spawn.movementZ = static_cast<f64>(velocity.z);
    spawn.yRot = static_cast<i8>(mc::network::backend::java::wire::packDegrees(itemEntity->yaw()));
    spawn.xRot = static_cast<i8>(mc::network::backend::java::wire::packDegrees(itemEntity->pitch()));
    spawn.yHeadRot = spawn.yRot;
    spawn.data = itemEntity->getSpawnData();
    player->send(makePlayPacket(mc::network::ir::PlayPacket{spawn}));

    std::vector<u8> metadata = network::EntityMetadataSerializer::serialize(itemEntity->dataManager(), false);
    if (metadata.size() > 1) {
        mc::network::ir::play::SetEntityData meta;
        meta.entityId = static_cast<i32>(itemEntity->id());
        meta.packedItems = std::move(metadata);
        player->send(makePlayPacket(mc::network::ir::PlayPacket{std::move(meta)}));
    }
}

} // namespace mc::server
