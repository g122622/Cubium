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

#include "server/network/PacketBuilders.hpp"

#include "common/network/backend/java/codecs/CommandTreeEncoder.hpp"
#include "common/network/ir/packets/play/PlayPacketsExtended.hpp"
#include "common/network/protocol/ConnectionProtocol.hpp"
#include "server/command/CommandRegistry.hpp"
#include <spdlog/spdlog.h>

namespace mc::server::net {

mc::network::ir::IrPacket buildPlaySoundIr(
    const ResourceLocation& soundEventId, sound::SoundCategory category, const Vector3& position, f32 volume, f32 pitch)
{
    // 1.21.11 PlaySound：Holder<SoundEvent>(结构化内联) + source + 坐标×8 + volume + pitch + seed。
    // soundHolder 用内联 SoundEvent（direct=true，identifier=soundEventId），对齐 vanilla wire。
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
    return mc::network::ir::IrPacket{
        mc::network::protocol::ConnectionProtocol::Play,
        mc::network::ir::PlayPacket{std::move(pkt)},
    };
}

mc::network::ir::IrPacket buildPermissionLevelChangeIr(i32 entityId, i32 permissionLevel)
{
    // 1.21.11 权限等级走 EntityEvent（OP_PERMISSION_LEVEL_0..3 = 24..27）。
    mc::network::ir::play::EntityEvent pkt;
    pkt.entityId = entityId;
    pkt.eventId = static_cast<u8>(24 + permissionLevel);
    return mc::network::ir::IrPacket{
        mc::network::protocol::ConnectionProtocol::Play,
        mc::network::ir::PlayPacket{std::move(pkt)},
    };
}

std::optional<mc::network::ir::IrPacket> buildCommandsIr(mc::command::CommandRegistry& registry)
{
    // 1.21.11 ClientboundCommandsPacket：二进制 CommandNode 树。对齐 Java 线格式
    // （VarInt(nodeCount) + nodes + VarInt(rootIndex)，每节点 flags/children/redirect/stub）。
    mc::network::ir::play::Commands pkt;
    auto snapshot = registry.getCommandTreeSnapshot();
    auto encoded = mc::network::java::codecs::encodeCommandTree(snapshot);
    if (!encoded.success()) {
        spdlog::error("CommandTreeEncoder: failed to encode command tree: {}", encoded.error().toString());
        return std::nullopt;
    }
    pkt.payload = std::move(encoded.value());
    return mc::network::ir::IrPacket{
        mc::network::protocol::ConnectionProtocol::Play,
        mc::network::ir::PlayPacket{std::move(pkt)},
    };
}

mc::network::ir::IrPacket buildLevelParticlesIr(particle::ParticleTypeId type, const Vector3& pos, u32 count)
{
    // 1.21.11 LevelParticles：简单粒子（SimpleParticleType）偏移/速度归零。
    mc::network::ir::play::ParticleOptions options;
    options.type = type;

    mc::network::ir::play::LevelParticles pkt;
    pkt.overrideLimiter = false;
    pkt.alwaysShow = false;
    pkt.x = pos.x;
    pkt.y = pos.y;
    pkt.z = pos.z;
    pkt.xDist = 0.0f;
    pkt.yDist = 0.0f;
    pkt.zDist = 0.0f;
    pkt.maxSpeed = 0.0f;
    pkt.count = static_cast<i32>(count);
    pkt.particle = std::move(options);
    return mc::network::ir::IrPacket{
        mc::network::protocol::ConnectionProtocol::Play,
        mc::network::ir::PlayPacket{std::move(pkt)},
    };
}

} // namespace mc::server::net
