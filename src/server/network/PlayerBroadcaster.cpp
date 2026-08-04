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

#include "server/network/PlayerBroadcaster.hpp"

#include "common/core/Types.hpp"
#include "common/network/ir/IrPacket.hpp"
#include "common/network/ir/ItemStackBridge.hpp"
#include "common/network/ir/packets/play/PlayPackets.hpp"
#include "common/network/ir/packets/play/PlayPacketsExtended.hpp"
#include "common/network/protocol/ConnectionProtocol.hpp"
#include "common/profiler/TraceCategories.hpp"
#include "common/profiler/TraceEvents.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/sound/SoundCategory.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/math/MathUtils.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/util/nbt/Nbt.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/blockentity/BlockEntityType.hpp"
#include "common/world/chunk/base/SectionPos.hpp"
#include "common/world/gameevent/PositionSource.hpp"
#include "server/application/MinecraftServer.hpp"
#include "server/core/ServerPlayerData.hpp"
#include "server/world/player/ServerPlayerEntityManager.hpp"
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
#include <fmt/format.h>

using namespace mc::trace;

namespace mc::server::net {

namespace {

/// 构造 LevelParticles IR（1.21.11，对齐 ClientboundLevelParticlesPacket）。
///
/// 外层字段取自广播参数：位置/偏移/count；maxSpeed 沿用旧实现固定 0（客户端按
/// 偏移扇出，不消费该字段）。ParticleOptions 由调用方按粒子类型预先填充。
[[nodiscard]] mc::network::ir::IrPacket buildLevelParticlesIr(
    const Vector3& pos, const Vector3& offset, u32 count, mc::network::ir::play::ParticleOptions options)
{
    mc::network::ir::play::LevelParticles pkt;
    pkt.overrideLimiter = false;
    pkt.alwaysShow = false;
    pkt.x = pos.x;
    pkt.y = pos.y;
    pkt.z = pos.z;
    pkt.xDist = offset.x;
    pkt.yDist = offset.y;
    pkt.zDist = offset.z;
    pkt.maxSpeed = 0.0f;
    pkt.count = static_cast<i32>(count);
    pkt.particle = std::move(options);
    return mc::network::ir::IrPacket{
        mc::network::protocol::ConnectionProtocol::Play,
        mc::network::ir::PlayPacket{std::move(pkt)},
    };
}

} // namespace

PlayerBroadcaster::PlayerBroadcaster(MinecraftServer& server)
    : m_server(server)
{}

// ============================================================================
// 声音
// ============================================================================

void PlayerBroadcaster::broadcastSound(
    const ResourceLocation& soundEventId, sound::SoundCategory category, const Vector3& position, f32 volume, f32 pitch)
{
    // 1.21.11 PlaySound：Holder<SoundEvent>(结构化内联) + source + 坐标×8 + volume + pitch + seed。
    // soundHolder 用内联 SoundEvent（direct=true，identifier=soundEventId），对齐 vanilla wire。
    //   seed 暂用固定值 0。
    mc::network::ir::play::PlaySound pkt;
    pkt.soundHolder.direct = true;
    pkt.soundHolder.identifier = soundEventId.toString();
    pkt.soundHolder.hasFixedRange = false;
    pkt.source = static_cast<i32>(category);
    pkt.x = static_cast<i32>(position.x * 8.0f);
    pkt.y = static_cast<i32>(position.y * 8.0f);
    pkt.z = static_cast<i32>(position.z * 8.0f);
    pkt.volume = volume;
    pkt.pitch = pitch;
    pkt.seed = 0;
    m_server.broadcastPacket(mc::network::ir::IrPacket{
        mc::network::protocol::ConnectionProtocol::Play,
        mc::network::ir::PlayPacket{std::move(pkt)},
    });
}

void PlayerBroadcaster::broadcastSoundInRange(const ResourceLocation& soundEventId,
    sound::SoundCategory category,
    const Vector3& position,
    f32 range,
    f32 volume,
    f32 pitch)
{
    // 1.21.11 PlaySound（同上），仅发送给范围内玩家。
    mc::network::ir::play::PlaySound pkt;
    pkt.soundHolder.direct = true;
    pkt.soundHolder.identifier = soundEventId.toString();
    pkt.soundHolder.hasFixedRange = false;
    pkt.source = static_cast<i32>(category);
    pkt.x = static_cast<i32>(position.x * 8.0f);
    pkt.y = static_cast<i32>(position.y * 8.0f);
    pkt.z = static_cast<i32>(position.z * 8.0f);
    pkt.volume = volume;
    pkt.pitch = pitch;
    pkt.seed = 0;

    mc::network::ir::IrPacket packet{
        mc::network::protocol::ConnectionProtocol::Play,
        mc::network::ir::PlayPacket{pkt},
    };

    // 只发送给范围内的玩家
    u32 playersNotified = 0;
    m_server.playerManager().forEachPlayer(
        [this, &position, range, &packet, &playersNotified](ServerPlayerData& player) {
            if (!player.loggedIn || !player.hasConnection()) {
                return;
            }

            f32 dx = player.x - position.x;
            f32 dy = player.y - position.y;
            f32 dz = player.z - position.z;
            f32 distSq = dx * dx + dy * dy + dz * dz;
            f32 rangeSq = range * range;

            if (distSq <= rangeSq) {
                m_server.sendPacketToPlayer(player.playerId, packet);
                playersNotified++;
            }
        });
}

void PlayerBroadcaster::sendSoundToPlayer(PlayerId playerId,
    const ResourceLocation& soundEventId,
    sound::SoundCategory category,
    const Vector3& position,
    f32 volume,
    f32 pitch)
{
    // 1.21.11 PlaySound（同上），定向发送。
    mc::network::ir::play::PlaySound pkt;
    pkt.soundHolder.direct = true;
    pkt.soundHolder.identifier = soundEventId.toString();
    pkt.soundHolder.hasFixedRange = false;
    pkt.source = static_cast<i32>(category);
    pkt.x = static_cast<i32>(position.x * 8.0f);
    pkt.y = static_cast<i32>(position.y * 8.0f);
    pkt.z = static_cast<i32>(position.z * 8.0f);
    pkt.volume = volume;
    pkt.pitch = pitch;
    pkt.seed = 0;
    m_server.sendPacketToPlayer(playerId,
        mc::network::ir::IrPacket{
            mc::network::protocol::ConnectionProtocol::Play,
            mc::network::ir::PlayPacket{std::move(pkt)},
        });
}

// ============================================================================
// 粒子
// ============================================================================

void PlayerBroadcaster::broadcastParticleInRange(particle::ParticleTypeId type,
    const Vector3& pos,
    const Vector3& velocity,
    const Vector3& offset,
    u32 count,
    f32 range)
{
    mc::network::ir::play::ParticleOptions options;
    options.type = type;
    auto irPacket = buildLevelParticlesIr(pos, offset, count, std::move(options));

    // 只发送给范围内的玩家
    u32 playersNotified = 0;
    m_server.playerManager().forEachPlayer([this, &pos, range, &irPacket, &playersNotified](ServerPlayerData& player) {
        if (!player.loggedIn || !player.hasConnection()) {
            return;
        }

        f32 dx = player.x - pos.x;
        f32 dy = player.y - pos.y;
        f32 dz = player.z - pos.z;
        f32 distSq = dx * dx + dy * dy + dz * dz;
        f32 rangeSq = range * range;

        if (distSq <= rangeSq) {
            m_server.sendPacketToPlayer(player.playerId, irPacket);
            playersNotified++;
        }
    });
}

void PlayerBroadcaster::sendParticleToPlayer(PlayerId playerId,
    particle::ParticleTypeId type,
    const Vector3& pos,
    const Vector3& velocity,
    const Vector3& offset,
    u32 count)
{
    mc::network::ir::play::ParticleOptions options;
    options.type = type;
    auto irPacket = buildLevelParticlesIr(pos, offset, count, std::move(options));
    m_server.sendPacketToPlayer(playerId, irPacket);
}

void PlayerBroadcaster::broadcastVibrationParticleInRange(
    const Vector3& pos, const gameevent::PositionSource& targetSource, i32 arrivalInTicks, f32 range)
{
    // VibrationParticleOption.STREAM_CODEC = PositionSource + VAR_INT arrivalInTicks
    // PositionSource = VarInt(kind: 0=Block 1=Entity)
    //   kind=0: i64 packedBlockPos；kind=1: VarInt entityId + FLOAT yOffset
    mc::network::ir::play::ParticleOptions options;
    options.type = particle::ParticleTypeId::Vibration;
    options.arrivalInTicks = arrivalInTicks;
    const std::string sourceType = targetSource.type();
    if (sourceType == "entity") {
        const auto& entitySource = static_cast<const gameevent::EntityPositionSource&>(targetSource);
        options.vibrationSourceKind = 1;
        options.vibrationEntityId = static_cast<i32>(entitySource.entityId());
        options.vibrationYOffset = entitySource.yOffset();
    } else {
        // 默认按方块位置源处理（"block" 或任何未知类型）
        const auto& blockSource = static_cast<const gameevent::BlockPositionSource&>(targetSource);
        options.vibrationSourceKind = 0;
        options.vibrationBlockPosPacked = blockSource.pos().asLong();
    }

    auto irPacket = buildLevelParticlesIr(pos, Vector3(0.0f, 0.0f, 0.0f), 1, std::move(options));

    // 只发送给范围内的玩家
    m_server.playerManager().forEachPlayer([this, &pos, range, &irPacket](ServerPlayerData& player) {
        if (!player.loggedIn || !player.hasConnection()) {
            return;
        }

        f32 dx = player.x - pos.x;
        f32 dy = player.y - pos.y;
        f32 dz = player.z - pos.z;
        f32 distSq = dx * dx + dy * dy + dz * dz;
        f32 rangeSq = range * range;

        if (distSq <= rangeSq) {
            m_server.sendPacketToPlayer(player.playerId, irPacket);
        }
    });
}

void PlayerBroadcaster::broadcastTrailParticleInRange(
    const Vector3& pos, const Vector3d& targetPosition, u32 color, i32 durationInTicks, f32 range)
{
    // TrailParticleOption.STREAM_CODEC = Vec3(3×F64 target) + INT color(ARGB) + VAR_INT duration
    mc::network::ir::play::ParticleOptions options;
    options.type = particle::ParticleTypeId::Trail;
    options.trailTargetX = targetPosition.x;
    options.trailTargetY = targetPosition.y;
    options.trailTargetZ = targetPosition.z;
    options.color = color;
    options.trailDuration = durationInTicks;
    auto irPacket = buildLevelParticlesIr(pos, Vector3(0.0f, 0.0f, 0.0f), 1, std::move(options));

    // 只发送给范围内的玩家
    m_server.playerManager().forEachPlayer([this, &pos, range, &irPacket](ServerPlayerData& player) {
        if (!player.loggedIn || !player.hasConnection()) {
            return;
        }

        f32 dx = player.x - pos.x;
        f32 dy = player.y - pos.y;
        f32 dz = player.z - pos.z;
        f32 distSq = dx * dx + dy * dy + dz * dz;
        f32 rangeSq = range * range;

        if (distSq <= rangeSq) {
            m_server.sendPacketToPlayer(player.playerId, irPacket);
        }
    });
}

void PlayerBroadcaster::broadcastEntityEffectParticleInRange(
    const Vector3& pos, const Vector3& velocity, const Vector3& offset, u32 count, u32 color, f32 range)
{
    // ColorParticleOption(ENTITY_EFFECT).STREAM_CODEC = INT color（ARGB 大端）
    mc::network::ir::play::ParticleOptions options;
    options.type = particle::ParticleTypeId::EntityEffect;
    options.color = color;
    auto irPacket = buildLevelParticlesIr(pos, offset, count, std::move(options));

    // 只发送给范围内的玩家
    m_server.playerManager().forEachPlayer([this, &pos, range, &irPacket](ServerPlayerData& player) {
        if (!player.loggedIn || !player.hasConnection()) {
            return;
        }

        f32 dx = player.x - pos.x;
        f32 dy = player.y - pos.y;
        f32 dz = player.z - pos.z;
        f32 distSq = dx * dx + dy * dy + dz * dz;
        f32 rangeSq = range * range;

        if (distSq <= rangeSq) {
            m_server.sendPacketToPlayer(player.playerId, irPacket);
        }
    });
}

void PlayerBroadcaster::broadcastBlockParticleInRange(
    particle::ParticleTypeId type, const Vector3& pos, const Vector3& velocity, u32 blockStateId, f32 range)
{
    // BlockParticleOption.STREAM_CODEC = VarInt(blockStateId)
    mc::network::ir::play::ParticleOptions options;
    options.type = type;
    options.blockStateId = blockStateId;
    auto irPacket = buildLevelParticlesIr(pos, Vector3(0.0f, 0.0f, 0.0f), 1, std::move(options));

    // 只发送给范围内的玩家
    m_server.playerManager().forEachPlayer([this, &pos, range, &irPacket](ServerPlayerData& player) {
        if (!player.loggedIn || !player.hasConnection()) {
            return;
        }

        f32 dx = player.x - pos.x;
        f32 dy = player.y - pos.y;
        f32 dz = player.z - pos.z;
        f32 distSq = dx * dx + dy * dy + dz * dz;
        f32 rangeSq = range * range;

        if (distSq <= rangeSq) {
            m_server.sendPacketToPlayer(player.playerId, irPacket);
        }
    });
}

void PlayerBroadcaster::broadcastItemParticleInRange(
    particle::ParticleTypeId type, const Vector3& pos, const Vector3& velocity, const ItemStack& itemStack, f32 range)
{
    // ItemParticleOption.STREAM_CODEC = 完整 ItemStack wire（VarInt count + Item holder + DataComponentPatch）
    mc::network::ir::play::ParticleOptions options;
    options.type = type;
    options.item = mc::network::ir::toItemStackView(itemStack);
    auto irPacket = buildLevelParticlesIr(pos, Vector3(0.0f, 0.0f, 0.0f), 1, std::move(options));

    // 只发送给范围内的玩家
    m_server.playerManager().forEachPlayer([this, &pos, range, &irPacket](ServerPlayerData& player) {
        if (!player.loggedIn || !player.hasConnection()) {
            return;
        }

        f32 dx = player.x - pos.x;
        f32 dy = player.y - pos.y;
        f32 dz = player.z - pos.z;
        f32 distSq = dx * dx + dy * dy + dz * dz;
        f32 rangeSq = range * range;

        if (distSq <= rangeSq) {
            m_server.sendPacketToPlayer(player.playerId, irPacket);
        }
    });
}

// ============================================================================
// 实体事件/动画
// ============================================================================

void PlayerBroadcaster::broadcastEntityStatusInRange(
    EntityInstanceId entityId, u8 status, const Vector3& pos, f32 range)
{
    // 1.21.11 EntityEvent：entityId + eventId(status)。
    mc::network::ir::play::EntityEvent pkt;
    pkt.entityId = static_cast<i32>(entityId);
    pkt.eventId = status;
    mc::network::ir::IrPacket packet{
        mc::network::protocol::ConnectionProtocol::Play,
        mc::network::ir::PlayPacket{pkt},
    };

    f32 rangeSq = range * range;
    m_server.playerManager().forEachPlayer([this, &pos, rangeSq, &packet](ServerPlayerData& player) {
        if (!player.loggedIn || !player.hasConnection()) {
            return;
        }

        f32 distSq = math::distanceSq(player.x, player.y, player.z, pos.x, pos.y, pos.z);
        if (distSq <= rangeSq) {
            m_server.sendPacketToPlayer(player.playerId, packet);
        }
    });
}

void PlayerBroadcaster::broadcastEntityAnimationInRange(
    EntityInstanceId entityId, u8 animation, const Vector3& pos, f32 range)
{
    // 1.21.11 Animate：entityId + action(animation)。
    mc::network::ir::play::Animate pkt;
    pkt.id = static_cast<i32>(entityId);
    pkt.action = animation;
    mc::network::ir::IrPacket packet{
        mc::network::protocol::ConnectionProtocol::Play,
        mc::network::ir::PlayPacket{pkt},
    };

    f32 rangeSq = range * range;
    m_server.playerManager().forEachPlayer([this, &pos, rangeSq, &packet](ServerPlayerData& player) {
        if (!player.loggedIn || !player.hasConnection()) {
            return;
        }

        f32 distSq = math::distanceSq(player.x, player.y, player.z, pos.x, pos.y, pos.z);
        if (distSq <= rangeSq) {
            m_server.sendPacketToPlayer(player.playerId, packet);
        }
    });
}

void PlayerBroadcaster::broadcastHurtAnimationInRange(
    EntityInstanceId entityId, f32 hurtDir, const Vector3& pos, f32 range)
{
    // 1.21.11 HurtAnimation：entityId + yaw(hurtDir)。
    // 受害者自身与范围内追踪者均会收到。
    mc::network::ir::play::HurtAnimation pkt;
    pkt.id = static_cast<i32>(entityId);
    pkt.yaw = hurtDir;
    mc::network::ir::IrPacket packet{
        mc::network::protocol::ConnectionProtocol::Play,
        mc::network::ir::PlayPacket{pkt},
    };

    f32 rangeSq = range * range;
    m_server.playerManager().forEachPlayer([this, &pos, rangeSq, &packet](ServerPlayerData& player) {
        if (!player.loggedIn || !player.hasConnection()) {
            return;
        }

        f32 distSq = math::distanceSq(player.x, player.y, player.z, pos.x, pos.y, pos.z);
        if (distSq <= rangeSq) {
            m_server.sendPacketToPlayer(player.playerId, packet);
        }
    });
}

void PlayerBroadcaster::broadcastSetEntityLinkInRange(
    EntityInstanceId entityId, EntityInstanceId linkedEntityId, const Vector3& pos, f32 range)
{
    // 1.21.11 SetEntityLink：sourceId + destId（leash/riding 关系）。
    mc::network::ir::play::SetEntityLink pkt;
    pkt.sourceId = static_cast<i32>(entityId);
    pkt.destId = static_cast<i32>(linkedEntityId);
    mc::network::ir::IrPacket packet{
        mc::network::protocol::ConnectionProtocol::Play,
        mc::network::ir::PlayPacket{pkt},
    };

    f32 rangeSq = range * range;
    m_server.playerManager().forEachPlayer([this, &pos, rangeSq, &packet](ServerPlayerData& player) {
        if (!player.loggedIn || !player.hasConnection()) {
            return;
        }

        f32 distSq = math::distanceSq(player.x, player.y, player.z, pos.x, pos.y, pos.z);
        if (distSq <= rangeSq) {
            m_server.sendPacketToPlayer(player.playerId, packet);
        }
    });
}

// ============================================================================
// 世界事件
// ============================================================================

void PlayerBroadcaster::broadcastWorldEvent(i32 eventId, i32 x, i32 y, i32 z, i32 data)
{
    // 1.21.11 LevelEvent：type + blockPosPacked + data + globalEvent。
    mc::network::ir::play::LevelEvent pkt;
    pkt.type = eventId;
    pkt.blockPosPacked = BlockPos(x, y, z).asLong();
    pkt.data = data;
    pkt.globalEvent = false;
    m_server.broadcastPacket(mc::network::ir::IrPacket{
        mc::network::protocol::ConnectionProtocol::Play,
        mc::network::ir::PlayPacket{std::move(pkt)},
    });
}

void PlayerBroadcaster::broadcastWorldEventInRange(i32 eventId, i32 x, i32 y, i32 z, i32 data, f32 range)
{
    mc::network::ir::play::LevelEvent pkt;
    pkt.type = eventId;
    pkt.blockPosPacked = BlockPos(x, y, z).asLong();
    pkt.data = data;
    pkt.globalEvent = false;
    mc::network::ir::IrPacket packet{
        mc::network::protocol::ConnectionProtocol::Play,
        mc::network::ir::PlayPacket{pkt},
    };

    f32 rangeSq = range * range;
    Vector3 pos(static_cast<f32>(x), static_cast<f32>(y), static_cast<f32>(z));
    m_server.playerManager().forEachPlayer([this, &pos, rangeSq, &packet](ServerPlayerData& player) {
        if (!player.loggedIn || !player.hasConnection()) {
            return;
        }

        f32 distSq = math::distanceSq(player.x, player.y, player.z, pos.x, pos.y, pos.z);
        if (distSq <= rangeSq) {
            m_server.sendPacketToPlayer(player.playerId, packet);
        }
    });
}

void PlayerBroadcaster::broadcastBlockEventInRange(i32 x, i32 y, i32 z, u8 paramA, u8 paramB, u32 blockId, f32 range)
{
    // 1.21.11 BlockEvent wire：blockPosPacked + b0 + b1 + blockId(VarInt)。
    // IR 层 blockId 存项目内部 blockId（与 BlockUpdate.blockStateId 存内部 stateId 同范式），
    // 由 blockEventCodec 出站边界译为 Java Block 注册表 id（JavaBlockIdMap::toJavaRegistryId）。
    // 本地客户端经 LocalTransport 直传 IR 不经 codec，且客户端 ClientPlayVisitor 不消费
    // blockId（仅用 pos+b0+b1 调 BlockEntity::triggerEvent），故内部 id 直传自洽。
    mc::network::ir::play::BlockEvent pkt;
    pkt.blockPosPacked = BlockPos(x, y, z).asLong();
    pkt.b0 = paramA;
    pkt.b1 = paramB;
    pkt.blockId = static_cast<i32>(blockId);
    mc::network::ir::IrPacket packet{
        mc::network::protocol::ConnectionProtocol::Play,
        mc::network::ir::PlayPacket{pkt},
    };

    f32 rangeSq = range * range;
    Vector3 pos(static_cast<f32>(x), static_cast<f32>(y), static_cast<f32>(z));
    m_server.playerManager().forEachPlayer([this, &pos, rangeSq, &packet](ServerPlayerData& player) {
        if (!player.loggedIn || !player.hasConnection()) {
            return;
        }

        f32 distSq = math::distanceSq(player.x, player.y, player.z, pos.x, pos.y, pos.z);
        if (distSq <= rangeSq) {
            m_server.sendPacketToPlayer(player.playerId, packet);
        }
    });
}

void PlayerBroadcaster::broadcastBlockEntityInRange(
    const BlockPos& pos, BlockEntityType type, std::shared_ptr<nbt::CompoundTag> tag, f32 range)
{
    // 参考 MC Java: PlayerList.broadcast(null, x, y, z, 64.0, dimension,
    //   new ClientboundBlockEntityDataPacket(pos, type, tag))
    // 方块实体数据变化后，将最新 NBT 快照发送给附近客户端。
    // 1.21.11 BlockEntityData：blockPosPacked + blockEntityType + CompoundTag（无长度前缀）。
    mc::network::ir::play::BlockEntityData pkt;
    pkt.blockPosPacked = pos.asLong();
    pkt.blockEntityType = static_cast<i32>(type);
    pkt.tag = std::move(tag);
    mc::network::ir::IrPacket packet{
        mc::network::protocol::ConnectionProtocol::Play,
        mc::network::ir::PlayPacket{pkt},
    };

    f32 rangeSq = range * range;
    Vector3 fpos(static_cast<f32>(pos.x), static_cast<f32>(pos.y), static_cast<f32>(pos.z));
    m_server.playerManager().forEachPlayer([this, &fpos, rangeSq, &packet](ServerPlayerData& player) {
        if (!player.loggedIn || !player.hasConnection()) {
            return;
        }

        f32 distSq = math::distanceSq(player.x, player.y, player.z, fpos.x, fpos.y, fpos.z);
        if (distSq <= rangeSq) {
            m_server.sendPacketToPlayer(player.playerId, packet);
        }
    });
}

void PlayerBroadcaster::broadcastBlockBreakProgressInRange(
    EntityInstanceId breakerId, i32 x, i32 y, i32 z, i32 progress, f32 range)
{
    // 对应 MC Java: ServerLevel.destroyBlockProgress()
    // 1.21.11 BlockDestruction：breakerId + blockPosPacked + progress(0-9)。
    // MC Java 原版行为：排除破坏者自身（serverplayer.getId() != breakerId），
    // 只向同维度、32格范围内的其他玩家发送。破坏者自身的动画由客户端本地直接驱动。
    mc::network::ir::play::BlockDestruction pkt;
    pkt.id = static_cast<i32>(breakerId);
    pkt.blockPosPacked = BlockPos(x, y, z).asLong();
    pkt.progress = static_cast<u8>(progress);

    // 将 breakerId (EntityInstanceId) 转换为 PlayerId，用于排除破坏者自身
    PlayerId breakerPlayerId = m_server.playerEntityManager().getPlayerIdByEntityId(breakerId);

    f32 rangeSq = range * range;
    Vector3 pos(static_cast<f32>(x), static_cast<f32>(y), static_cast<f32>(z));
    m_server.playerManager().forEachPlayer([this, breakerPlayerId, &pos, rangeSq, &pkt](ServerPlayerData& player) {
        if (!player.loggedIn || !player.hasConnection()) {
            return;
        }

        // 排除破坏者自身：MC Java 原版中 serverplayer.getId() != breakerId
        if (breakerPlayerId != 0 && player.playerId == breakerPlayerId) {
            return;
        }

        f32 distSq = math::distanceSq(player.x, player.y, player.z, pos.x, pos.y, pos.z);
        if (distSq <= rangeSq) {
            m_server.sendPacketToPlayer(player.playerId,
                mc::network::ir::IrPacket{
                    mc::network::protocol::ConnectionProtocol::Play,
                    mc::network::ir::PlayPacket{pkt},
                });
        }
    });
}

// ============================================================================
// 爆炸
// ============================================================================

void PlayerBroadcaster::broadcastExplosionInRange(const Vector3& position,
    f32 strength,
    const std::vector<BlockPos>& affectedBlocks,
    const std::unordered_map<u64, Vector3>& playerKnockback,
    f32 range)
{
    // 发送给爆炸点 64 格范围内的玩家
    // 每个玩家收到的击退向量不同，需要为每个玩家单独构建数据包

    f32 rangeSq = range * range;

    m_server.playerManager().forEachPlayer(
        [this, &position, strength, &affectedBlocks, &playerKnockback, rangeSq](ServerPlayerData& player) {
            if (!player.loggedIn || !player.hasConnection()) {
                return;
            }

            // 检查玩家是否在范围内
            f32 dx = player.x - position.x;
            f32 dy = player.y - position.y;
            f32 dz = player.z - position.z;
            f32 distSq = dx * dx + dy * dy + dz * dz;

            if (distSq <= rangeSq) {
                // 为每个玩家创建单独的爆炸包（击退向量不同）
                sendExplosionToPlayer(player.playerId, position, strength, affectedBlocks, playerKnockback);
            }
        });
}

void PlayerBroadcaster::sendExplosionToPlayer(PlayerId playerId,
    const Vector3& position,
    f32 strength,
    const std::vector<BlockPos>& affectedBlocks,
    const std::unordered_map<u64, Vector3>& playerKnockback)
{
    // 构造 1.21.11 Explosion IR（结构化：中心/半径/方块数/击退/粒子/声音）
    mc::network::ir::play::Explosion pkt;
    pkt.centerX = position.x;
    pkt.centerY = position.y;
    pkt.centerZ = position.z;
    pkt.radius = strength;
    pkt.blockCount = static_cast<i32>(affectedBlocks.size());
    const auto kbIt = playerKnockback.find(static_cast<u64>(playerId));
    pkt.hasPlayerKnockback = (kbIt != playerKnockback.end());
    if (pkt.hasPlayerKnockback) {
        pkt.knockbackX = kbIt->second.x;
        pkt.knockbackY = kbIt->second.y;
        pkt.knockbackZ = kbIt->second.z;
    }
    pkt.explosionParticle.type = particle::ParticleTypeId::Explosion;
    pkt.explosionSound.direct = true;
    pkt.explosionSound.identifier = "minecraft:entity.generic.explode";
    pkt.explosionSound.hasFixedRange = false;

    m_server.sendPacketToPlayer(playerId,
        mc::network::ir::IrPacket{
            mc::network::protocol::ConnectionProtocol::Play,
            mc::network::ir::PlayPacket{std::move(pkt)},
        });
}

// ============================================================================
// 光照更新
// ============================================================================

void PlayerBroadcaster::broadcastLightUpdate(ChunkCoord x,
    ChunkCoord z,
    i32 sectionY,
    const std::vector<u8>& skyLight,
    const std::vector<u8>& blockLight,
    bool trustEdges)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Lighting,
        "BroadcastLightUpdate",
        "Section",
        fmt::format("({}, {}, {})", x, sectionY, z),
        "SkyLightSize",
        skyLight.size(),
        "BlockLightSize",
        blockLight.size(),
        [flow = ::perfetto::Flow::ProcessScoped(SectionPos(x, sectionY, z).toLong())](
            ::perfetto::EventContext ctx) { flow(ctx); });

    // 1.21.11 ClientboundLightUpdatePacket 线格式：
    //   VarInt(x)+VarInt(z)+4×BitSet(skyYMask/blockYMask/emptySkyYMask/emptyBlockYMask)+2×List<byte[≤2048]>
    // BitSet 位 i 对应光照段 Y = minLightSection + i（minLightSection = MIN_SECTION_Y - 1，主世界=-5）。
    // 单段光照更新：仅在该段对应的 yMask（sky 或 block）置位，并往对应列表塞一条 2048 字节 nibble。
    // 注意 setOnLightChanged 回调按 LightType 单独触发，故每个包只含一种光照类型的数据。
    constexpr i32 kMinLightSection = mc::world::MIN_SECTION_Y - 1; // 主世界=-5
    const i32 bitIndex = sectionY - kMinLightSection;
    if (bitIndex < 0) {
        return; // 段坐标越界（低于最小光照段），丢弃
    }

    // BitSet 以最小长整型数组表示：bit 所在 long = bit / 64，long 内位 = bit % 64。
    auto buildSingleBitMask = [&](i32 bit) -> std::vector<i64> {
        const usize li = static_cast<usize>(bit) / 64;
        std::vector<i64> mask(li + 1, 0);
        mask[li] = static_cast<i64>(static_cast<u64>(1) << (static_cast<usize>(bit) % 64));
        return mask;
    };

    mc::network::ir::play::LightUpdate pkt;
    pkt.x = static_cast<i32>(x);
    pkt.z = static_cast<i32>(z);
    // lightMasks 顺序：skyYMask / blockYMask / emptySkyYMask / emptyBlockYMask
    if (!skyLight.empty()) {
        pkt.lightMasks[0] = buildSingleBitMask(bitIndex); // skyYMask
        pkt.lightUpdates[0].push_back(skyLight);          // skyUpdates
    }
    if (!blockLight.empty()) {
        pkt.lightMasks[1] = buildSingleBitMask(bitIndex); // blockYMask
        pkt.lightUpdates[1].push_back(blockLight);        // blockUpdates
    }
    MC_UNUSED(trustEdges); // 1.21.11 已从该包移除 trustEdges，保留参数以兼容调用方签名

    mc::network::ir::IrPacket packet{
        mc::network::protocol::ConnectionProtocol::Play,
        mc::network::ir::PlayPacket{std::move(pkt)},
    };
    m_server.playerManager().forEachPlayer([this, &packet](ServerPlayerData& player) {
        if (player.loggedIn && player.hasConnection()) {
            m_server.sendPacketToPlayer(player.playerId, mc::network::ir::IrPacket{packet});
        }
    });
}

} // namespace mc::server::net
