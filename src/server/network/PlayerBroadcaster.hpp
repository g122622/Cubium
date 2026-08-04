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

#pragma once

#include "common/core/Types.hpp"
#include "common/particle/ParticleTypes.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/sound/SoundCategory.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/util/nbt/Nbt.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/blockentity/BlockEntityType.hpp"

#include <memory>
#include <unordered_map>
#include <vector>

namespace mc::gameevent {
class PositionSource;
}

namespace mc {
class ItemStack;
}

namespace mc::server {

class MinecraftServer;

} // namespace mc::server

namespace mc::server::net {

/**
 * @brief 玩家广播门面
 *
 * 承接原 MinecraftServer 的全部 broadcast 与 send 系转发型网络广播职责：声音、粒子、
 * 实体事件/动画、世界事件、方块事件/方块实体、方块破坏进度、爆炸、光照更新等。
 * 方法体逐字节搬移自 MinecraftServer，仅改前缀（成员访问经 m_server 引用）。
 *
 * 依赖面极窄：仅经 MinecraftServer& 取 playerManager()（forEachPlayer 距离过滤）、
 * playerEntityManager()（broadcastBlockBreakProgressInRange 排除破坏者）、
 * broadcastPacket 与 sendPacketToPlayer 两个发送原语。不持有任何状态。
 *
 * 装配层（attachWorldBindings 的 16 个 lambda）留 MinecraftServer，仅把 lambda 内
 * broadcastXxx() 改为 m_broadcaster.broadcastXxx()；捕获 &world 做 getEntity 位置
 * 补全的 lambda 保持不变。
 *
 * 不承接耦合 Server 私有状态的方法：broadcastDifficultyChange（依赖 m_difficulty）、
 * sendCommandTreePacket（依赖 m_commandRegistry）、sendPermissionLevelChange
 * （依赖 getPlayerWorld + playerEntityManager）——这些由 PacketBuilders builder +
 * connectionManager 在批5b 处理。broadcastServerMessage 仅 spdlog 不发包，批5b 删。
 */
class PlayerBroadcaster {
public:
    explicit PlayerBroadcaster(MinecraftServer& server);

    // ========== 声音 ==========
    void broadcastSound(const ResourceLocation& soundEventId,
        sound::SoundCategory category,
        const Vector3& position,
        f32 volume,
        f32 pitch);
    void broadcastSoundInRange(const ResourceLocation& soundEventId,
        sound::SoundCategory category,
        const Vector3& position,
        f32 range,
        f32 volume,
        f32 pitch);
    void sendSoundToPlayer(PlayerId playerId,
        const ResourceLocation& soundEventId,
        sound::SoundCategory category,
        const Vector3& position,
        f32 volume,
        f32 pitch);

    // ========== 粒子 ==========
    void broadcastParticleInRange(particle::ParticleTypeId type,
        const Vector3& pos,
        const Vector3& velocity,
        const Vector3& offset,
        u32 count,
        f32 range = 256.0f);
    void sendParticleToPlayer(PlayerId playerId,
        particle::ParticleTypeId type,
        const Vector3& pos,
        const Vector3& velocity,
        const Vector3& offset,
        u32 count);
    void broadcastVibrationParticleInRange(
        const Vector3& pos, const gameevent::PositionSource& targetSource, i32 arrivalInTicks, f32 range = 256.0f);
    void broadcastTrailParticleInRange(
        const Vector3& pos, const Vector3d& targetPosition, u32 color, i32 durationInTicks, f32 range = 256.0f);
    void broadcastEntityEffectParticleInRange(
        const Vector3& pos, const Vector3& velocity, const Vector3& offset, u32 count, u32 color, f32 range = 256.0f);
    void broadcastBlockParticleInRange(particle::ParticleTypeId type,
        const Vector3& pos,
        const Vector3& velocity,
        u32 blockStateId,
        f32 range = 256.0f);
    void broadcastItemParticleInRange(particle::ParticleTypeId type,
        const Vector3& pos,
        const Vector3& velocity,
        const ItemStack& itemStack,
        f32 range = 256.0f);

    // ========== 实体事件/动画 ==========
    void broadcastEntityStatusInRange(EntityInstanceId entityId, u8 status, const Vector3& pos, f32 range = 32.0f);
    void broadcastEntityAnimationInRange(
        EntityInstanceId entityId, u8 animation, const Vector3& pos, f32 range = 32.0f);
    void broadcastHurtAnimationInRange(EntityInstanceId entityId, f32 hurtDir, const Vector3& pos, f32 range = 32.0f);
    void broadcastSetEntityLinkInRange(
        EntityInstanceId entityId, EntityInstanceId linkedEntityId, const Vector3& pos, f32 range = 32.0f);

    // ========== 世界事件 ==========
    void broadcastWorldEvent(i32 eventId, i32 x, i32 y, i32 z, i32 data);
    void broadcastWorldEventInRange(i32 eventId, i32 x, i32 y, i32 z, i32 data, f32 range = 64.0f);
    void broadcastBlockEventInRange(i32 x, i32 y, i32 z, u8 paramA, u8 paramB, u32 blockId, f32 range = 64.0f);
    void broadcastBlockEntityInRange(
        const BlockPos& pos, BlockEntityType type, std::shared_ptr<nbt::CompoundTag> tag, f32 range = 64.0f);
    void broadcastBlockBreakProgressInRange(
        EntityInstanceId breakerId, i32 x, i32 y, i32 z, i32 progress, f32 range = 32.0f);

    // ========== 爆炸 ==========
    void broadcastExplosionInRange(const Vector3& position,
        f32 strength,
        const std::vector<BlockPos>& affectedBlocks,
        const std::unordered_map<u64, Vector3>& playerKnockback,
        f32 range = 64.0f);
    void sendExplosionToPlayer(PlayerId playerId,
        const Vector3& position,
        f32 strength,
        const std::vector<BlockPos>& affectedBlocks,
        const std::unordered_map<u64, Vector3>& playerKnockback);

    // ========== 光照更新 ==========
    void broadcastLightUpdate(ChunkCoord x,
        ChunkCoord z,
        i32 sectionY,
        const std::vector<u8>& skyLight,
        const std::vector<u8>& blockLight,
        bool trustEdges);

private:
    MinecraftServer& m_server;
};

} // namespace mc::server::net
