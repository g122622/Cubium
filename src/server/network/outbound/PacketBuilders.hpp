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
#include "common/network/ir/IrPacket.hpp"
#include "common/particle/ParticleTypes.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/sound/SoundCategory.hpp"
#include "common/util/math/Vector3.hpp"

#include <optional>

namespace mc::command {
class CommandRegistry;
}

namespace mc::server::net {

/**
 * @brief Play / Configuration 阶段单包 IR 构造器（无状态自由函数）
 *
 * 承接原 IServer 上耦合度低、可经公开数据源构造的"单包"业务方法：
 * buildPlaySoundIr（原 sendSoundToPlayer 的包构造）、buildPermissionLevelChangeIr
 * （原 sendPermissionLevelChange 的 EntityEvent 包构造）、buildCommandsIr（原
 * sendCommandTreePacket 的命令树包构造）、buildLevelParticlesIr（原弱类型
 * broadcastParticleInRange 的粒子包构造）。
 *
 * 与 PlayerBroadcaster 的边界：PlayerBroadcaster 承接"转发型广播"（持
 * MinecraftServer& 做距离过滤 + 多态发送）；本构造器只产 IrPacket，不含发送语义、
 * 不持状态——主调者（命令、登录流程）自行经 connectionManager().sendToPlayer 或
 * sendPacketToPlayer 投递。这样 IServer 不必为这些单包业务暴露纯虚。
 *
 * 数据源全部经入参或 IServer 公开访问器（commandRegistry()/getPlayerWorld()/
 * playerEntityManager()）取得，无 friend、无 down-cast。
 */

/**
 * @brief 构造 PlaySound IR（1.21.11，对齐 ClientboundSoundPacket）
 *
 * Holder<SoundEvent> 用内联 SoundEvent（direct=true，identifier=soundEventId），
 * 与 PlayerBroadcaster::broadcastSound 一致。坐标按 vanilla ×8 取整。seed 固定 0。
 *
 * @param soundEventId 声音事件 ResourceLocation
 * @param category 声音类别（SoundSource ordinal）
 * @param position 声音世界坐标
 * @param volume 音量倍率
 * @param pitch 音调倍率
 */
[[nodiscard]] mc::network::ir::IrPacket buildPlaySoundIr(const ResourceLocation& soundEventId,
    sound::SoundCategory category,
    const Vector3& position,
    f32 volume,
    f32 pitch);

/**
 * @brief 构造权限等级变更 EntityEvent IR（1.21.11）
 *
 * 1.21.11 权限等级走 EntityEvent（OP_PERMISSION_LEVEL_0..3 = 24..27），
 * eventId = 24 + permissionLevel。原 sendPermissionLevelChange 的 EntityEvent 部分。
 *
 * 注意：仅构造 EntityEvent 包，不含命令树刷新。完整权限变更语义（EntityEvent +
 * Commands）由主调者额外调 buildCommandsIr 并分别投递，对齐原 sendPermissionLevelChange
 * 末尾硬编码调 sendCommandTreePacket 的行为。
 *
 * @param entityId 玩家实体 ID（运行时经 playerEntityManager().getPlayerEntity 取得）
 * @param permissionLevel 权限等级 0..3
 */
[[nodiscard]] mc::network::ir::IrPacket buildPermissionLevelChangeIr(i32 entityId, i32 permissionLevel);

/**
 * @brief 构造命令树 Commands IR（1.21.11，对齐 ClientboundCommandsPacket）
 *
 * 经 CommandTreeEncoder 把 CommandRegistry 的命令树快照编码为二进制 payload。
 * 编码失败时返回 nullopt（主调者应跳过投递并记日志，对齐原 sendCommandTreePacket
 * 失败即 return 的行为）。
 *
 * @param registry 命令注册表（经 IServer::commandRegistry() 取得）
 */
[[nodiscard]] std::optional<mc::network::ir::IrPacket> buildCommandsIr(mc::command::CommandRegistry& registry);

/**
 * @brief 构造 LevelParticles IR（1.21.11，对齐 ClientboundLevelParticlesPacket）
 *
 * 简单粒子类型（SimpleParticleType，无额外 options）的便捷构造：偏移/速度归零、
 * count 由入参指定。原弱类型 broadcastParticleInRange 的包构造部分（ParticleCommand
 * 当前传 velocity=0/offset=0/count=1）。
 *
 * 复杂粒子选项（vibration/trail/block/item/entity_effect）仍由 PlayerBroadcaster 的
 * 强类型 broadcastXxxParticleInRange 承接（它们需距离过滤 + 多态发送）。
 *
 * @param type 粒子类型
 * @param pos 粒子世界坐标
 * @param count 粒子数量
 */
[[nodiscard]] mc::network::ir::IrPacket buildLevelParticlesIr(
    particle::ParticleTypeId type, const Vector3& pos, u32 count);

} // namespace mc::server::net
