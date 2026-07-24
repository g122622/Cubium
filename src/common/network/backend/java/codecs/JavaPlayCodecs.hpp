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

#include "common/network/backend/java/codecs/JavaCodecBase.hpp"
#include "common/network/backend/java/codecs/JavaWireHelpers.hpp"
#include "common/network/ir/IrPacket.hpp"

namespace mc::network::backend::java::codecs {

// ============================================================================
// Play 阶段 codec（对齐 Java 1.21.11 GameProtocols 注册顺序）
// 在用包子集：移动/交互/容器/实体同步/区块/全局状态。完整包后续按需补。
// ============================================================================

namespace play_detail {

/**
 * @brief 写 ItemStackView（optional 物品，1.21.11 数据组件格式）
 *
 * 线格式：VarInt(count) —— count<=0 即空（停止）；否则
 *         VarInt(itemId) + DataComponentPatch wire（view.componentsPatch 的原始字节）。
 * componentsPatch 由 ItemStack↔ItemStackView 桥接（ItemStackBridge.hpp）预先序列化，
 * 本 codec 只透传其字节，保持 IR 对线格式中立。
 */
inline void writeItemStack(B& buf, const ir::play::ItemStackView& v)
{
    if (v.count <= 0) {
        buf.writeVarInt(0);
        return;
    }
    buf.writeVarInt(v.count);
    buf.writeVarInt(static_cast<i32>(v.itemId));
    buf.writeVarInt(static_cast<i32>(v.componentsPatch.size()));
    if (!v.componentsPatch.empty()) {
        buf.writeBytes(v.componentsPatch.data(), v.componentsPatch.size());
    }
}

[[nodiscard]] inline Result<ir::play::ItemStackView> readItemStack(B& buf)
{
    ir::play::ItemStackView v{};
    i32 count = 0;
    MC_TRY_ASSIGN(count, buf.readVarInt());
    if (count <= 0) {
        return v; // 空
    }
    v.count = count;
    i32 id = 0;
    MC_TRY_ASSIGN(id, buf.readVarInt());
    v.itemId = static_cast<u32>(id);
    i32 patchLen = 0;
    MC_TRY_ASSIGN(patchLen, buf.readVarInt());
    if (patchLen < 0) {
        return Error(ErrorCode::InvalidData, "ItemStack componentsPatch length is negative", "readItemStack");
    }
    if (patchLen > 0) {
        MC_TRY_ASSIGN(v.componentsPatch, buf.readBytes(static_cast<usize>(patchLen)));
    }
    return v;
}

/**
 * @brief 写 HashedStack（ContainerClick 用）
 *
 * 对应 Java HashedStack：present=false 写 Bool(false)；true 写 Bool(true)+ActualItem
 * { VarInt(itemId) + VarInt(count) + HashedPatchMap }。
 * TODO(Phase5): HashedPatchMap（added/removed 组件哈希），当前省略。
 */
inline void writeHashedStack(B& buf, const ir::play::HashedStack& v)
{
    if (!v.present) {
        buf.writeBool(false);
        return;
    }
    buf.writeBool(true);
    buf.writeVarInt(static_cast<i32>(v.itemId));
    buf.writeVarInt(v.count);
    // HashedPatchMap 暂省（Phase5）
}

[[nodiscard]] inline Result<ir::play::HashedStack> readHashedStack(B& buf)
{
    ir::play::HashedStack v{};
    bool present = false;
    MC_TRY_ASSIGN(present, buf.readBool());
    v.present = present;
    if (!present) {
        return v;
    }
    i32 id = 0;
    MC_TRY_ASSIGN(id, buf.readVarInt());
    v.itemId = static_cast<u32>(id);
    MC_TRY_ASSIGN(v.count, buf.readVarInt());
    return v;
}

/**
 * @brief 写 CommonPlayerSpawnInfo（Login 内联子结构）
 */
inline void writeSpawnInfo(B& buf, const ir::play::CommonPlayerSpawnInfo& s)
{
    // dimensionType holder：TODO(Phase6) 对齐 registry holder；当前写 VarInt(dimensionType)
    buf.writeVarInt(s.dimensionType);
    buf.writeString(s.dimension);
    buf.writeI64(s.seed);
    buf.writeU8(static_cast<u8>(s.gameType));
    buf.writeI8(s.previousGameType);
    buf.writeBool(s.isDebug);
    buf.writeBool(s.isFlat);
    if (s.lastDeathLocation.has_value()) {
        buf.writeBool(true);
        buf.writeString(s.lastDeathLocation->first);
        buf.writeI64(s.lastDeathLocation->second); // BlockPos.asLong
    } else {
        buf.writeBool(false);
    }
    buf.writeVarInt(s.portalCooldown);
    buf.writeVarInt(s.seaLevel);
}

[[nodiscard]] inline Result<ir::play::CommonPlayerSpawnInfo> readSpawnInfo(B& buf)
{
    ir::play::CommonPlayerSpawnInfo s{};
    MC_TRY_ASSIGN(s.dimensionType, buf.readVarInt());
    MC_TRY_ASSIGN(s.dimension, buf.readString());
    MC_TRY_ASSIGN(s.seed, buf.readI64());
    u8 gt = 0;
    MC_TRY_ASSIGN(gt, buf.readU8());
    s.gameType = static_cast<GameMode>(gt);
    MC_TRY_ASSIGN(s.previousGameType, buf.readI8());
    MC_TRY_ASSIGN(s.isDebug, buf.readBool());
    MC_TRY_ASSIGN(s.isFlat, buf.readBool());
    bool hasDeath = false;
    MC_TRY_ASSIGN(hasDeath, buf.readBool());
    if (hasDeath) {
        std::string dim;
        i64 pos = 0;
        MC_TRY_ASSIGN(dim, buf.readString());
        MC_TRY_ASSIGN(pos, buf.readI64());
        s.lastDeathLocation = std::make_pair(std::move(dim), pos);
    }
    MC_TRY_ASSIGN(s.portalCooldown, buf.readVarInt());
    MC_TRY_ASSIGN(s.seaLevel, buf.readVarInt());
    return s;
}

} // namespace play_detail

// ============================================================================
// 通用包
// ============================================================================

/// KeepAlive（双向，VarLong(id)）
[[nodiscard]] inline auto keepAliveCodec()
{
    return makeCodec<ir::play::KeepAlive>([](B& buf, const ir::play::KeepAlive& v) { buf.writeVarLong(v.id); },
        [](B& buf) -> Result<ir::play::KeepAlive> {
            ir::play::KeepAlive v{};
            MC_TRY_ASSIGN(v.id, buf.readVarLong());
            return v;
        });
}

/// Disconnect（S→C，Utf8(reason JSON)）
[[nodiscard]] inline auto playDisconnectCodec()
{
    return makeCodec<ir::play::Disconnect>([](B& buf, const ir::play::Disconnect& v) { buf.writeString(v.reason); },
        [](B& buf) -> Result<ir::play::Disconnect> {
            ir::play::Disconnect v{};
            MC_TRY_ASSIGN(v.reason, buf.readString());
            return v;
        });
}

/// Chat（C→S，id=8）
[[nodiscard]] inline auto chatCodec()
{
    return makeCodec<ir::play::Chat>(
        [](B& buf, const ir::play::Chat& v) {
            buf.writeString(v.message);
            buf.writeI64(v.timestamp);
            buf.writeI64(v.salt);
            if (v.signature.has_value()) {
                buf.writeBool(true);
                buf.writeBytes(v.signature->data(), v.signature->size());
            } else {
                buf.writeBool(false);
            }
            buf.writeVarInt(v.lastSeenOffset);
            buf.writeBytes(v.lastSeenAcknowledged.data(), v.lastSeenAcknowledged.size());
            buf.writeU8(v.lastSeenChecksum);
        },
        [](B& buf) -> Result<ir::play::Chat> {
            ir::play::Chat v{};
            MC_TRY_ASSIGN(v.message, buf.readString());
            MC_TRY_ASSIGN(v.timestamp, buf.readI64());
            MC_TRY_ASSIGN(v.salt, buf.readI64());
            bool hasSig = false;
            MC_TRY_ASSIGN(hasSig, buf.readBool());
            if (hasSig) {
                std::vector<u8> sig;
                MC_TRY_ASSIGN(sig, buf.readBytes(256));
                v.signature = std::move(sig);
            }
            MC_TRY_ASSIGN(v.lastSeenOffset, buf.readVarInt());
            MC_TRY(buf.readBytes(v.lastSeenAcknowledged.data(), v.lastSeenAcknowledged.size()));
            MC_TRY_ASSIGN(v.lastSeenChecksum, buf.readU8());
            return v;
        });
}

// ============================================================================
// 玩家移动（C→S）
// ============================================================================

/// MovePlayerPos（C→S，id=29）
[[nodiscard]] inline auto movePlayerPosCodec()
{
    return makeCodec<ir::play::MovePlayerPos>(
        [](B& buf, const ir::play::MovePlayerPos& v) {
            buf.writeF64(v.x);
            buf.writeF64(v.y);
            buf.writeF64(v.z);
            buf.writeU8(v.flags.pack());
        },
        [](B& buf) -> Result<ir::play::MovePlayerPos> {
            ir::play::MovePlayerPos v{};
            MC_TRY_ASSIGN(v.x, buf.readF64());
            MC_TRY_ASSIGN(v.y, buf.readF64());
            MC_TRY_ASSIGN(v.z, buf.readF64());
            u8 flags = 0;
            MC_TRY_ASSIGN(flags, buf.readU8());
            v.flags = ir::play::MovePlayerFlags::unpack(flags);
            return v;
        });
}

/// MovePlayerPosRot（C→S，id=30）
[[nodiscard]] inline auto movePlayerPosRotCodec()
{
    return makeCodec<ir::play::MovePlayerPosRot>(
        [](B& buf, const ir::play::MovePlayerPosRot& v) {
            buf.writeF64(v.x);
            buf.writeF64(v.y);
            buf.writeF64(v.z);
            buf.writeF32(v.yRot);
            buf.writeF32(v.xRot);
            buf.writeU8(v.flags.pack());
        },
        [](B& buf) -> Result<ir::play::MovePlayerPosRot> {
            ir::play::MovePlayerPosRot v{};
            MC_TRY_ASSIGN(v.x, buf.readF64());
            MC_TRY_ASSIGN(v.y, buf.readF64());
            MC_TRY_ASSIGN(v.z, buf.readF64());
            MC_TRY_ASSIGN(v.yRot, buf.readF32());
            MC_TRY_ASSIGN(v.xRot, buf.readF32());
            u8 flags = 0;
            MC_TRY_ASSIGN(flags, buf.readU8());
            v.flags = ir::play::MovePlayerFlags::unpack(flags);
            return v;
        });
}

/// MovePlayerRot（C→S，id=31）
[[nodiscard]] inline auto movePlayerRotCodec()
{
    return makeCodec<ir::play::MovePlayerRot>(
        [](B& buf, const ir::play::MovePlayerRot& v) {
            buf.writeF32(v.yRot);
            buf.writeF32(v.xRot);
            buf.writeU8(v.flags.pack());
        },
        [](B& buf) -> Result<ir::play::MovePlayerRot> {
            ir::play::MovePlayerRot v{};
            MC_TRY_ASSIGN(v.yRot, buf.readF32());
            MC_TRY_ASSIGN(v.xRot, buf.readF32());
            u8 flags = 0;
            MC_TRY_ASSIGN(flags, buf.readU8());
            v.flags = ir::play::MovePlayerFlags::unpack(flags);
            return v;
        });
}

/// MovePlayerStatusOnly（C→S，id=32）
[[nodiscard]] inline auto movePlayerStatusOnlyCodec()
{
    return makeCodec<ir::play::MovePlayerStatusOnly>(
        [](B& buf, const ir::play::MovePlayerStatusOnly& v) { buf.writeU8(v.flags.pack()); },
        [](B& buf) -> Result<ir::play::MovePlayerStatusOnly> {
            ir::play::MovePlayerStatusOnly v{};
            u8 flags = 0;
            MC_TRY_ASSIGN(flags, buf.readU8());
            v.flags = ir::play::MovePlayerFlags::unpack(flags);
            return v;
        });
}

/// AcceptTeleportation（C→S，id=0）
[[nodiscard]] inline auto acceptTeleportationCodec()
{
    return makeCodec<ir::play::AcceptTeleportation>(
        [](B& buf, const ir::play::AcceptTeleportation& v) { buf.writeVarInt(v.teleportId); },
        [](B& buf) -> Result<ir::play::AcceptTeleportation> {
            ir::play::AcceptTeleportation v{};
            MC_TRY_ASSIGN(v.teleportId, buf.readVarInt());
            return v;
        });
}

/// PlayerCommand（C→S，id=41）
[[nodiscard]] inline auto playerCommandCodec()
{
    return makeCodec<ir::play::PlayerCommand>(
        [](B& buf, const ir::play::PlayerCommand& v) {
            buf.writeVarInt(v.entityId);
            buf.writeVarInt(v.action);
            buf.writeVarInt(v.data);
        },
        [](B& buf) -> Result<ir::play::PlayerCommand> {
            ir::play::PlayerCommand v{};
            MC_TRY_ASSIGN(v.entityId, buf.readVarInt());
            MC_TRY_ASSIGN(v.action, buf.readVarInt());
            MC_TRY_ASSIGN(v.data, buf.readVarInt());
            return v;
        });
}

/// PlayerInput（C→S，id=42）
[[nodiscard]] inline auto playerInputCodec()
{
    return makeCodec<ir::play::PlayerInput>([](B& buf, const ir::play::PlayerInput& v) { buf.writeU8(v.input); },
        [](B& buf) -> Result<ir::play::PlayerInput> {
            ir::play::PlayerInput v{};
            MC_TRY_ASSIGN(v.input, buf.readU8());
            return v;
        });
}

/// UseItemOn（C→S，id=63）：hand + blockHit + sequence
[[nodiscard]] inline auto useItemOnCodec()
{
    return makeCodec<ir::play::UseItemOn>(
        [](B& buf, const ir::play::UseItemOn& v) {
            buf.writeVarInt(v.hand);
            const auto& h = v.blockHit;
            buf.writeI64(h.blockPosPacked);
            buf.writeVarInt(h.direction);
            buf.writeF32(h.hitX);
            buf.writeF32(h.hitY);
            buf.writeF32(h.hitZ);
            buf.writeBool(h.inside);
            buf.writeBool(h.worldBorderHit);
            buf.writeVarInt(v.sequence);
        },
        [](B& buf) -> Result<ir::play::UseItemOn> {
            ir::play::UseItemOn v{};
            MC_TRY_ASSIGN(v.hand, buf.readVarInt());
            MC_TRY_ASSIGN(v.blockHit.blockPosPacked, buf.readI64());
            MC_TRY_ASSIGN(v.blockHit.direction, buf.readVarInt());
            MC_TRY_ASSIGN(v.blockHit.hitX, buf.readF32());
            MC_TRY_ASSIGN(v.blockHit.hitY, buf.readF32());
            MC_TRY_ASSIGN(v.blockHit.hitZ, buf.readF32());
            MC_TRY_ASSIGN(v.blockHit.inside, buf.readBool());
            MC_TRY_ASSIGN(v.blockHit.worldBorderHit, buf.readBool());
            MC_TRY_ASSIGN(v.sequence, buf.readVarInt());
            return v;
        });
}

/// UseItem（C→S，id=64）：hand + sequence + yRot + xRot
[[nodiscard]] inline auto useItemCodec()
{
    return makeCodec<ir::play::UseItem>(
        [](B& buf, const ir::play::UseItem& v) {
            buf.writeVarInt(v.hand);
            buf.writeVarInt(v.sequence);
            buf.writeF32(v.yRot);
            buf.writeF32(v.xRot);
        },
        [](B& buf) -> Result<ir::play::UseItem> {
            ir::play::UseItem v{};
            MC_TRY_ASSIGN(v.hand, buf.readVarInt());
            MC_TRY_ASSIGN(v.sequence, buf.readVarInt());
            MC_TRY_ASSIGN(v.yRot, buf.readF32());
            MC_TRY_ASSIGN(v.xRot, buf.readF32());
            return v;
        });
}

/// PlayerAction（C→S，id=40）：action + BlockPos + Direction + sequence
[[nodiscard]] inline auto playerActionCodec()
{
    return makeCodec<ir::play::PlayerAction>(
        [](B& buf, const ir::play::PlayerAction& v) {
            buf.writeVarInt(v.action);
            buf.writeI64(v.blockPosPacked);
            buf.writeU8(static_cast<u8>(v.direction));
            buf.writeVarInt(v.sequence);
        },
        [](B& buf) -> Result<ir::play::PlayerAction> {
            ir::play::PlayerAction v{};
            MC_TRY_ASSIGN(v.action, buf.readVarInt());
            MC_TRY_ASSIGN(v.blockPosPacked, buf.readI64());
            u8 dir = 0;
            MC_TRY_ASSIGN(dir, buf.readU8());
            v.direction = static_cast<i32>(dir);
            MC_TRY_ASSIGN(v.sequence, buf.readVarInt());
            return v;
        });
}

// ============================================================================
// 玩家手持物品
// ============================================================================

/// SetCarriedItem（C→S，id=52）：Short(slot)
[[nodiscard]] inline auto setCarriedItemCodec()
{
    return makeCodec<ir::play::SetCarriedItem>([](B& buf, const ir::play::SetCarriedItem& v) { buf.writeI16(v.slot); },
        [](B& buf) -> Result<ir::play::SetCarriedItem> {
            ir::play::SetCarriedItem v{};
            MC_TRY_ASSIGN(v.slot, buf.readI16());
            return v;
        });
}

// ============================================================================
// 容器交互
// ============================================================================

/// ContainerClick（C→S，id=17）
[[nodiscard]] inline auto containerClickCodec()
{
    return makeCodec<ir::play::ContainerClick>(
        [](B& buf, const ir::play::ContainerClick& v) {
            buf.writeVarInt(v.containerId);
            buf.writeVarInt(v.stateId);
            buf.writeI16(v.slotNum);
            buf.writeI8(v.buttonNum);
            buf.writeVarInt(v.clickType);
            buf.writeVarInt(static_cast<i32>(v.changedSlots.size()));
            for (const auto& cs : v.changedSlots) {
                buf.writeI16(cs.slot);
                play_detail::writeHashedStack(buf, cs.stack);
            }
            play_detail::writeHashedStack(buf, v.carriedItem);
        },
        [](B& buf) -> Result<ir::play::ContainerClick> {
            ir::play::ContainerClick v{};
            MC_TRY_ASSIGN(v.containerId, buf.readVarInt());
            MC_TRY_ASSIGN(v.stateId, buf.readVarInt());
            MC_TRY_ASSIGN(v.slotNum, buf.readI16());
            MC_TRY_ASSIGN(v.buttonNum, buf.readI8());
            MC_TRY_ASSIGN(v.clickType, buf.readVarInt());
            i32 count = 0;
            MC_TRY_ASSIGN(count, buf.readVarInt());
            if (count < 0) {
                return Error(ErrorCode::InvalidData, "changedSlots 数为负", "containerClickCodec");
            }
            for (i32 i = 0; i < count; ++i) {
                ir::play::ChangedSlot cs{};
                MC_TRY_ASSIGN(cs.slot, buf.readI16());
                MC_TRY_ASSIGN(cs.stack, play_detail::readHashedStack(buf));
                v.changedSlots.push_back(std::move(cs));
            }
            MC_TRY_ASSIGN(v.carriedItem, play_detail::readHashedStack(buf));
            return v;
        });
}

/// ContainerClose（C→S id=18 / S→C id=17）：VarInt(containerId)
[[nodiscard]] inline auto containerCloseCodec()
{
    return makeCodec<ir::play::ContainerClose>(
        [](B& buf, const ir::play::ContainerClose& v) { buf.writeVarInt(v.containerId); },
        [](B& buf) -> Result<ir::play::ContainerClose> {
            ir::play::ContainerClose v{};
            MC_TRY_ASSIGN(v.containerId, buf.readVarInt());
            return v;
        });
}

/// ConfigurationAcknowledged（C→S，id=15，terminal，空包）
[[nodiscard]] inline auto configurationAcknowledgedCodec()
{
    return makeCodec<ir::play::ConfigurationAcknowledged>([](B&, const ir::play::ConfigurationAcknowledged&) {},
        [](B&) -> Result<ir::play::ConfigurationAcknowledged> { return ir::play::ConfigurationAcknowledged{}; });
}

// ============================================================================
// 服务端→客户端：进游戏与全局状态
// ============================================================================

/// Login（S→C，id=48）
[[nodiscard]] inline auto loginCodec()
{
    return makeCodec<ir::play::Login>(
        [](B& buf, const ir::play::Login& v) {
            buf.writeI32(v.playerId);
            buf.writeBool(v.hardcore);
            buf.writeVarInt(static_cast<i32>(v.levels.size()));
            for (const auto& lvl : v.levels) {
                buf.writeString(lvl);
            }
            buf.writeVarInt(v.maxPlayers);
            buf.writeVarInt(v.chunkRadius);
            buf.writeVarInt(v.simulationDistance);
            buf.writeBool(v.reducedDebugInfo);
            buf.writeBool(v.showDeathScreen);
            buf.writeBool(v.doLimitedCrafting);
            play_detail::writeSpawnInfo(buf, v.spawnInfo);
            buf.writeBool(v.enforcesSecureChat);
        },
        [](B& buf) -> Result<ir::play::Login> {
            ir::play::Login v{};
            MC_TRY_ASSIGN(v.playerId, buf.readI32());
            MC_TRY_ASSIGN(v.hardcore, buf.readBool());
            i32 levelCount = 0;
            MC_TRY_ASSIGN(levelCount, buf.readVarInt());
            if (levelCount < 0) {
                return Error(ErrorCode::InvalidData, "levels 数为负", "loginCodec");
            }
            for (i32 i = 0; i < levelCount; ++i) {
                std::string lvl;
                MC_TRY_ASSIGN(lvl, buf.readString());
                v.levels.push_back(std::move(lvl));
            }
            MC_TRY_ASSIGN(v.maxPlayers, buf.readVarInt());
            MC_TRY_ASSIGN(v.chunkRadius, buf.readVarInt());
            MC_TRY_ASSIGN(v.simulationDistance, buf.readVarInt());
            MC_TRY_ASSIGN(v.reducedDebugInfo, buf.readBool());
            MC_TRY_ASSIGN(v.showDeathScreen, buf.readBool());
            MC_TRY_ASSIGN(v.doLimitedCrafting, buf.readBool());
            MC_TRY_ASSIGN(v.spawnInfo, play_detail::readSpawnInfo(buf));
            MC_TRY_ASSIGN(v.enforcesSecureChat, buf.readBool());
            return v;
        });
}

/// PlayerPosition（S→C，id=70）
[[nodiscard]] inline auto playerPositionCodec()
{
    return makeCodec<ir::play::PlayerPosition>(
        [](B& buf, const ir::play::PlayerPosition& v) {
            buf.writeVarInt(v.teleportId);
            buf.writeF64(v.x);
            buf.writeF64(v.y);
            buf.writeF64(v.z);
            buf.writeF64(v.deltaX);
            buf.writeF64(v.deltaY);
            buf.writeF64(v.deltaZ);
            buf.writeF32(v.yRot);
            buf.writeF32(v.xRot);
            buf.writeU32(v.relatives); // Relative.SET_STREAM_CODEC = writeInt（位掩码）
        },
        [](B& buf) -> Result<ir::play::PlayerPosition> {
            ir::play::PlayerPosition v{};
            MC_TRY_ASSIGN(v.teleportId, buf.readVarInt());
            MC_TRY_ASSIGN(v.x, buf.readF64());
            MC_TRY_ASSIGN(v.y, buf.readF64());
            MC_TRY_ASSIGN(v.z, buf.readF64());
            MC_TRY_ASSIGN(v.deltaX, buf.readF64());
            MC_TRY_ASSIGN(v.deltaY, buf.readF64());
            MC_TRY_ASSIGN(v.deltaZ, buf.readF64());
            MC_TRY_ASSIGN(v.yRot, buf.readF32());
            MC_TRY_ASSIGN(v.xRot, buf.readF32());
            MC_TRY_ASSIGN(v.relatives, buf.readU32());
            return v;
        });
}

/// SetTime（S→C，id=111）
[[nodiscard]] inline auto setTimeCodec()
{
    return makeCodec<ir::play::SetTime>(
        [](B& buf, const ir::play::SetTime& v) {
            buf.writeI64(v.gameTime);
            buf.writeI64(v.dayTime);
            buf.writeBool(v.tickDayTime);
        },
        [](B& buf) -> Result<ir::play::SetTime> {
            ir::play::SetTime v{};
            MC_TRY_ASSIGN(v.gameTime, buf.readI64());
            MC_TRY_ASSIGN(v.dayTime, buf.readI64());
            MC_TRY_ASSIGN(v.tickDayTime, buf.readBool());
            return v;
        });
}

/// PlayerAbilities（S→C，id=62）
[[nodiscard]] inline auto playerAbilitiesCodec()
{
    return makeCodec<ir::play::PlayerAbilities>(
        [](B& buf, const ir::play::PlayerAbilities& v) {
            buf.writeU8(v.flags);
            buf.writeF32(v.flyingSpeed);
            buf.writeF32(v.walkingSpeed);
        },
        [](B& buf) -> Result<ir::play::PlayerAbilities> {
            ir::play::PlayerAbilities v{};
            MC_TRY_ASSIGN(v.flags, buf.readU8());
            MC_TRY_ASSIGN(v.flyingSpeed, buf.readF32());
            MC_TRY_ASSIGN(v.walkingSpeed, buf.readF32());
            return v;
        });
}

/// SetHeldSlot（S→C，id=103）
[[nodiscard]] inline auto setHeldSlotCodec()
{
    return makeCodec<ir::play::SetHeldSlot>([](B& buf, const ir::play::SetHeldSlot& v) { buf.writeVarInt(v.slot); },
        [](B& buf) -> Result<ir::play::SetHeldSlot> {
            ir::play::SetHeldSlot v{};
            MC_TRY_ASSIGN(v.slot, buf.readVarInt());
            return v;
        });
}

/// SetDefaultSpawnPosition（S→C，id=95，1.21.11 RespawnData）
[[nodiscard]] inline auto setDefaultSpawnPositionCodec()
{
    return makeCodec<ir::play::SetDefaultSpawnPosition>(
        [](B& buf, const ir::play::SetDefaultSpawnPosition& v) {
            buf.writeString(v.dimension);
            buf.writeI64(v.blockPosPacked);
            buf.writeF32(v.yaw);
            buf.writeF32(v.pitch);
        },
        [](B& buf) -> Result<ir::play::SetDefaultSpawnPosition> {
            ir::play::SetDefaultSpawnPosition v{};
            MC_TRY_ASSIGN(v.dimension, buf.readString());
            MC_TRY_ASSIGN(v.blockPosPacked, buf.readI64());
            MC_TRY_ASSIGN(v.yaw, buf.readF32());
            MC_TRY_ASSIGN(v.pitch, buf.readF32());
            return v;
        });
}

/// ChangeDifficulty（S→C，id=10）
[[nodiscard]] inline auto changeDifficultyCodec()
{
    return makeCodec<ir::play::ChangeDifficulty>(
        [](B& buf, const ir::play::ChangeDifficulty& v) {
            buf.writeVarInt(v.difficulty);
            buf.writeBool(v.locked);
        },
        [](B& buf) -> Result<ir::play::ChangeDifficulty> {
            ir::play::ChangeDifficulty v{};
            MC_TRY_ASSIGN(v.difficulty, buf.readVarInt());
            MC_TRY_ASSIGN(v.locked, buf.readBool());
            return v;
        });
}

/// GameEvent（S→C，id=38，原 GameStateChange）：Byte(event)+Float(value)
[[nodiscard]] inline auto gameEventCodec()
{
    return makeCodec<ir::play::GameEvent>(
        [](B& buf, const ir::play::GameEvent& v) {
            buf.writeU8(v.event);
            buf.writeF32(v.value);
        },
        [](B& buf) -> Result<ir::play::GameEvent> {
            ir::play::GameEvent v{};
            MC_TRY_ASSIGN(v.event, buf.readU8());
            MC_TRY_ASSIGN(v.value, buf.readF32());
            return v;
        });
}

// ============================================================================
// 服务端→客户端：实体同步
// ============================================================================

/// AddEntity（S→C，id=1）
[[nodiscard]] inline auto addEntityCodec()
{
    return makeCodec<ir::play::AddEntity>(
        [](B& buf, const ir::play::AddEntity& v) {
            buf.writeVarInt(v.entityId);
            wire::writeUuid(buf, v.uuid);
            buf.writeVarInt(v.entityTypeId);
            buf.writeF64(v.x);
            buf.writeF64(v.y);
            buf.writeF64(v.z);
            wire::writeLpVec3(buf, v.movementX, v.movementY, v.movementZ);
            buf.writeI8(v.yRot);
            buf.writeI8(v.xRot);
            buf.writeI8(v.yHeadRot);
            buf.writeVarInt(v.data);
        },
        [](B& buf) -> Result<ir::play::AddEntity> {
            ir::play::AddEntity v{};
            MC_TRY_ASSIGN(v.entityId, buf.readVarInt());
            MC_TRY_ASSIGN(v.uuid, wire::readUuid(buf));
            MC_TRY_ASSIGN(v.entityTypeId, buf.readVarInt());
            MC_TRY_ASSIGN(v.x, buf.readF64());
            MC_TRY_ASSIGN(v.y, buf.readF64());
            MC_TRY_ASSIGN(v.z, buf.readF64());
            MC_TRY(wire::readLpVec3(buf, v.movementX, v.movementY, v.movementZ));
            MC_TRY_ASSIGN(v.yRot, buf.readI8());
            MC_TRY_ASSIGN(v.xRot, buf.readI8());
            MC_TRY_ASSIGN(v.yHeadRot, buf.readI8());
            MC_TRY_ASSIGN(v.data, buf.readVarInt());
            return v;
        });
}

/// RemoveEntities（S→C，id=75）
[[nodiscard]] inline auto removeEntitiesCodec()
{
    return makeCodec<ir::play::RemoveEntities>(
        [](B& buf, const ir::play::RemoveEntities& v) {
            buf.writeVarInt(static_cast<i32>(v.entityIds.size()));
            for (i32 id : v.entityIds) {
                buf.writeVarInt(id);
            }
        },
        [](B& buf) -> Result<ir::play::RemoveEntities> {
            ir::play::RemoveEntities v{};
            i32 count = 0;
            MC_TRY_ASSIGN(count, buf.readVarInt());
            if (count < 0) {
                return Error(ErrorCode::InvalidData, "entityIds 数为负", "removeEntitiesCodec");
            }
            for (i32 i = 0; i < count; ++i) {
                i32 id = 0;
                MC_TRY_ASSIGN(id, buf.readVarInt());
                v.entityIds.push_back(id);
            }
            return v;
        });
}

/// TeleportEntity（S→C，id=123）
[[nodiscard]] inline auto teleportEntityCodec()
{
    return makeCodec<ir::play::TeleportEntity>(
        [](B& buf, const ir::play::TeleportEntity& v) {
            buf.writeVarInt(v.entityId);
            buf.writeF64(v.x);
            buf.writeF64(v.y);
            buf.writeF64(v.z);
            buf.writeF64(v.deltaX);
            buf.writeF64(v.deltaY);
            buf.writeF64(v.deltaZ);
            buf.writeF32(v.yRot);
            buf.writeF32(v.xRot);
            buf.writeU32(v.relatives);
            buf.writeBool(v.onGround);
        },
        [](B& buf) -> Result<ir::play::TeleportEntity> {
            ir::play::TeleportEntity v{};
            MC_TRY_ASSIGN(v.entityId, buf.readVarInt());
            MC_TRY_ASSIGN(v.x, buf.readF64());
            MC_TRY_ASSIGN(v.y, buf.readF64());
            MC_TRY_ASSIGN(v.z, buf.readF64());
            MC_TRY_ASSIGN(v.deltaX, buf.readF64());
            MC_TRY_ASSIGN(v.deltaY, buf.readF64());
            MC_TRY_ASSIGN(v.deltaZ, buf.readF64());
            MC_TRY_ASSIGN(v.yRot, buf.readF32());
            MC_TRY_ASSIGN(v.xRot, buf.readF32());
            MC_TRY_ASSIGN(v.relatives, buf.readU32());
            MC_TRY_ASSIGN(v.onGround, buf.readBool());
            return v;
        });
}

/// MoveEntityPos（S→C，id=51）
[[nodiscard]] inline auto moveEntityPosCodec()
{
    return makeCodec<ir::play::MoveEntityPos>(
        [](B& buf, const ir::play::MoveEntityPos& v) {
            buf.writeVarInt(v.entityId);
            buf.writeI16(v.deltaX);
            buf.writeI16(v.deltaY);
            buf.writeI16(v.deltaZ);
            buf.writeBool(v.onGround);
        },
        [](B& buf) -> Result<ir::play::MoveEntityPos> {
            ir::play::MoveEntityPos v{};
            MC_TRY_ASSIGN(v.entityId, buf.readVarInt());
            MC_TRY_ASSIGN(v.deltaX, buf.readI16());
            MC_TRY_ASSIGN(v.deltaY, buf.readI16());
            MC_TRY_ASSIGN(v.deltaZ, buf.readI16());
            MC_TRY_ASSIGN(v.onGround, buf.readBool());
            return v;
        });
}

/// MoveEntityPosRot（S→C，id=52）
[[nodiscard]] inline auto moveEntityPosRotCodec()
{
    return makeCodec<ir::play::MoveEntityPosRot>(
        [](B& buf, const ir::play::MoveEntityPosRot& v) {
            buf.writeVarInt(v.entityId);
            buf.writeI16(v.deltaX);
            buf.writeI16(v.deltaY);
            buf.writeI16(v.deltaZ);
            buf.writeI8(v.yRot);
            buf.writeI8(v.xRot);
            buf.writeBool(v.onGround);
        },
        [](B& buf) -> Result<ir::play::MoveEntityPosRot> {
            ir::play::MoveEntityPosRot v{};
            MC_TRY_ASSIGN(v.entityId, buf.readVarInt());
            MC_TRY_ASSIGN(v.deltaX, buf.readI16());
            MC_TRY_ASSIGN(v.deltaY, buf.readI16());
            MC_TRY_ASSIGN(v.deltaZ, buf.readI16());
            MC_TRY_ASSIGN(v.yRot, buf.readI8());
            MC_TRY_ASSIGN(v.xRot, buf.readI8());
            MC_TRY_ASSIGN(v.onGround, buf.readBool());
            return v;
        });
}

/// MoveEntityRot（S→C，id=54）
[[nodiscard]] inline auto moveEntityRotCodec()
{
    return makeCodec<ir::play::MoveEntityRot>(
        [](B& buf, const ir::play::MoveEntityRot& v) {
            buf.writeVarInt(v.entityId);
            buf.writeI8(v.yRot);
            buf.writeI8(v.xRot);
            buf.writeBool(v.onGround);
        },
        [](B& buf) -> Result<ir::play::MoveEntityRot> {
            ir::play::MoveEntityRot v{};
            MC_TRY_ASSIGN(v.entityId, buf.readVarInt());
            MC_TRY_ASSIGN(v.yRot, buf.readI8());
            MC_TRY_ASSIGN(v.xRot, buf.readI8());
            MC_TRY_ASSIGN(v.onGround, buf.readBool());
            return v;
        });
}

/// SetEntityMotion（S→C，id=99，1.21.11 用 LpVec3）
[[nodiscard]] inline auto setEntityMotionCodec()
{
    return makeCodec<ir::play::SetEntityMotion>(
        [](B& buf, const ir::play::SetEntityMotion& v) {
            buf.writeVarInt(v.entityId);
            wire::writeLpVec3(buf, v.x, v.y, v.z);
        },
        [](B& buf) -> Result<ir::play::SetEntityMotion> {
            ir::play::SetEntityMotion v{};
            MC_TRY_ASSIGN(v.entityId, buf.readVarInt());
            MC_TRY(wire::readLpVec3(buf, v.x, v.y, v.z));
            return v;
        });
}

/// RotateHead（S→C，id=81）
[[nodiscard]] inline auto rotateHeadCodec()
{
    return makeCodec<ir::play::RotateHead>(
        [](B& buf, const ir::play::RotateHead& v) {
            buf.writeVarInt(v.entityId);
            buf.writeI8(v.yHeadRot);
        },
        [](B& buf) -> Result<ir::play::RotateHead> {
            ir::play::RotateHead v{};
            MC_TRY_ASSIGN(v.entityId, buf.readVarInt());
            MC_TRY_ASSIGN(v.yHeadRot, buf.readI8());
            return v;
        });
}

/// SetEntityData（S→C，id=97）
/// 线格式：VarInt(entityId) + packedItems 字节（含 EOF 0xFF）。
/// TODO(Phase6): 完整 DataValue serializer 注册表对齐 1.21.11。当前 packedItems 透传。
[[nodiscard]] inline auto setEntityDataCodec()
{
    return makeCodec<ir::play::SetEntityData>(
        [](B& buf, const ir::play::SetEntityData& v) {
            buf.writeVarInt(v.entityId);
            buf.writeBytes(v.packedItems.data(), v.packedItems.size());
        },
        [](B& buf) -> Result<ir::play::SetEntityData> {
            ir::play::SetEntityData v{};
            MC_TRY_ASSIGN(v.entityId, buf.readVarInt());
            // 剩余字节即 packedItems（含 EOF 0xFF）
            usize remaining = buf.readableBytes();
            MC_TRY_ASSIGN(v.packedItems, buf.readBytes(remaining));
            return v;
        });
}

// ============================================================================
// 服务端→客户端：区块与方块
// ============================================================================

/// LevelChunkWithLight（S→C，id=44）
/// 线格式：Int(x)+Int(z)+VarInt(chunkData 长度)+chunkData+VarInt(lightData 长度)+lightData。
/// TODO(Phase6): chunk section palette / 光照 masks 拆解对齐 1.21.11。当前整体透传。
[[nodiscard]] inline auto levelChunkWithLightCodec()
{
    return makeCodec<ir::play::LevelChunkWithLight>(
        [](B& buf, const ir::play::LevelChunkWithLight& v) {
            buf.writeI32(v.x);
            buf.writeI32(v.z);
            buf.writeVarInt(static_cast<i32>(v.chunkData.size()));
            buf.writeBytes(v.chunkData.data(), v.chunkData.size());
            buf.writeVarInt(static_cast<i32>(v.lightData.size()));
            buf.writeBytes(v.lightData.data(), v.lightData.size());
        },
        [](B& buf) -> Result<ir::play::LevelChunkWithLight> {
            ir::play::LevelChunkWithLight v{};
            MC_TRY_ASSIGN(v.x, buf.readI32());
            MC_TRY_ASSIGN(v.z, buf.readI32());
            i32 chunkLen = 0;
            MC_TRY_ASSIGN(chunkLen, buf.readVarInt());
            if (chunkLen < 0) {
                return Error(ErrorCode::InvalidData, "chunkData 长度为负", "levelChunkWithLightCodec");
            }
            MC_TRY_ASSIGN(v.chunkData, buf.readBytes(static_cast<usize>(chunkLen)));
            i32 lightLen = 0;
            MC_TRY_ASSIGN(lightLen, buf.readVarInt());
            if (lightLen < 0) {
                return Error(ErrorCode::InvalidData, "lightData 长度为负", "levelChunkWithLightCodec");
            }
            MC_TRY_ASSIGN(v.lightData, buf.readBytes(static_cast<usize>(lightLen)));
            return v;
        });
}

/// LightUpdate（S→C，id=47）
/// 线格式：VarInt(x)+VarInt(z)+VarInt(lightData 长度)+lightData。
[[nodiscard]] inline auto lightUpdateCodec()
{
    return makeCodec<ir::play::LightUpdate>(
        [](B& buf, const ir::play::LightUpdate& v) {
            buf.writeVarInt(v.x);
            buf.writeVarInt(v.z);
            buf.writeVarInt(static_cast<i32>(v.lightData.size()));
            buf.writeBytes(v.lightData.data(), v.lightData.size());
        },
        [](B& buf) -> Result<ir::play::LightUpdate> {
            ir::play::LightUpdate v{};
            MC_TRY_ASSIGN(v.x, buf.readVarInt());
            MC_TRY_ASSIGN(v.z, buf.readVarInt());
            i32 len = 0;
            MC_TRY_ASSIGN(len, buf.readVarInt());
            if (len < 0) {
                return Error(ErrorCode::InvalidData, "lightData 长度为负", "lightUpdateCodec");
            }
            MC_TRY_ASSIGN(v.lightData, buf.readBytes(static_cast<usize>(len)));
            return v;
        });
}

/// BlockUpdate（S→C，id=8）
[[nodiscard]] inline auto blockUpdateCodec()
{
    return makeCodec<ir::play::BlockUpdate>(
        [](B& buf, const ir::play::BlockUpdate& v) {
            buf.writeI64(v.blockPosPacked);
            buf.writeVarInt(v.blockStateId);
        },
        [](B& buf) -> Result<ir::play::BlockUpdate> {
            ir::play::BlockUpdate v{};
            MC_TRY_ASSIGN(v.blockPosPacked, buf.readI64());
            MC_TRY_ASSIGN(v.blockStateId, buf.readVarInt());
            return v;
        });
}

// ============================================================================
// 服务端→客户端：容器同步
// ============================================================================

/// ContainerSetContent（S→C，id=18）
[[nodiscard]] inline auto containerSetContentCodec()
{
    return makeCodec<ir::play::ContainerSetContent>(
        [](B& buf, const ir::play::ContainerSetContent& v) {
            buf.writeVarInt(v.containerId);
            buf.writeVarInt(v.stateId);
            buf.writeVarInt(static_cast<i32>(v.items.size()));
            for (const auto& item : v.items) {
                play_detail::writeItemStack(buf, item);
            }
            play_detail::writeItemStack(buf, v.carriedItem);
        },
        [](B& buf) -> Result<ir::play::ContainerSetContent> {
            ir::play::ContainerSetContent v{};
            MC_TRY_ASSIGN(v.containerId, buf.readVarInt());
            MC_TRY_ASSIGN(v.stateId, buf.readVarInt());
            i32 count = 0;
            MC_TRY_ASSIGN(count, buf.readVarInt());
            if (count < 0) {
                return Error(ErrorCode::InvalidData, "items 数为负", "containerSetContentCodec");
            }
            for (i32 i = 0; i < count; ++i) {
                ir::play::ItemStackView item{};
                MC_TRY_ASSIGN(item, play_detail::readItemStack(buf));
                v.items.push_back(std::move(item));
            }
            MC_TRY_ASSIGN(v.carriedItem, play_detail::readItemStack(buf));
            return v;
        });
}

/// ContainerSetSlot（S→C，id=20）
[[nodiscard]] inline auto containerSetSlotCodec()
{
    return makeCodec<ir::play::ContainerSetSlot>(
        [](B& buf, const ir::play::ContainerSetSlot& v) {
            buf.writeVarInt(v.containerId);
            buf.writeVarInt(v.stateId);
            buf.writeI16(v.slot);
            play_detail::writeItemStack(buf, v.item);
        },
        [](B& buf) -> Result<ir::play::ContainerSetSlot> {
            ir::play::ContainerSetSlot v{};
            MC_TRY_ASSIGN(v.containerId, buf.readVarInt());
            MC_TRY_ASSIGN(v.stateId, buf.readVarInt());
            MC_TRY_ASSIGN(v.slot, buf.readI16());
            MC_TRY_ASSIGN(v.item, play_detail::readItemStack(buf));
            return v;
        });
}

/// OpenScreen（S→C，id=57）
[[nodiscard]] inline auto openScreenCodec()
{
    return makeCodec<ir::play::OpenScreen>(
        [](B& buf, const ir::play::OpenScreen& v) {
            buf.writeVarInt(v.containerId);
            buf.writeVarInt(v.menuType);
            buf.writeString(v.title);
        },
        [](B& buf) -> Result<ir::play::OpenScreen> {
            ir::play::OpenScreen v{};
            MC_TRY_ASSIGN(v.containerId, buf.readVarInt());
            MC_TRY_ASSIGN(v.menuType, buf.readVarInt());
            MC_TRY_ASSIGN(v.title, buf.readString());
            return v;
        });
}

/// ContainerSetData（S→C，id=19，原 WindowProperty）
[[nodiscard]] inline auto containerSetDataCodec()
{
    return makeCodec<ir::play::ContainerSetData>(
        [](B& buf, const ir::play::ContainerSetData& v) {
            buf.writeVarInt(v.containerId);
            buf.writeI16(v.property);
            buf.writeI16(v.value);
        },
        [](B& buf) -> Result<ir::play::ContainerSetData> {
            ir::play::ContainerSetData v{};
            MC_TRY_ASSIGN(v.containerId, buf.readVarInt());
            MC_TRY_ASSIGN(v.property, buf.readI16());
            MC_TRY_ASSIGN(v.value, buf.readI16());
            return v;
        });
}

// ============================================================================
// 服务端→客户端：玩家列表
// ============================================================================

namespace player_info_detail {

// PlayerInfoUpdate action 位（与 Java Action ordinal 一致）
inline constexpr u16 kActionAddPlayer = 1u << 0;         // ADD_PLAYER(0)
inline constexpr u16 kActionInitializeChat = 1u << 1;    // INITIALIZE_CHAT(1)
inline constexpr u16 kActionUpdateGameMode = 1u << 2;    // UPDATE_GAME_MODE(2)
inline constexpr u16 kActionUpdateListed = 1u << 3;      // UPDATE_LISTED(3)
inline constexpr u16 kActionUpdateLatency = 1u << 4;     // UPDATE_LATENCY(4)
inline constexpr u16 kActionUpdateDisplayName = 1u << 5; // UPDATE_DISPLAY_NAME(5)
inline constexpr u16 kActionUpdateListOrder = 1u << 6;   // UPDATE_LIST_ORDER(6)
inline constexpr u16 kActionUpdateHat = 1u << 7;         // UPDATE_HAT(7)
// 注：1.21.11 还可能有第 9 位 INITIALIZE_CHAT 之外的位；按 9 位 fixedBitSet，当前 8 位够用。

/**
 * @brief 写 entries 的 action 负载（按 actions 中 set 的位顺序）
 *
 * INITIALIZE_CHAT 当前未承载（写 0 表示无 chat session），TODO(Phase6) 对齐 RemoteChatSession.Data。
 */
inline void writeEntry(B& buf, u16 actions, const ir::play::PlayerInfoEntry& e)
{
    wire::writeUuid(buf, e.uuid);
    if ((actions & kActionAddPlayer) != 0) {
        buf.writeString(e.name.value_or(""));
        buf.writeVarInt(static_cast<i32>(e.properties.size()));
        for (const auto& prop : e.properties) {
            buf.writeString(prop.first);
            buf.writeString(prop.second);
            buf.writeBool(false); // 无 signature
        }
    }
    if ((actions & kActionInitializeChat) != 0) {
        buf.writeBool(false); // 无 chat session
    }
    if ((actions & kActionUpdateGameMode) != 0) {
        buf.writeVarInt(e.gameMode.value_or(0));
    }
    if ((actions & kActionUpdateListed) != 0) {
        buf.writeBool(e.listed.value_or(false));
    }
    if ((actions & kActionUpdateLatency) != 0) {
        buf.writeVarInt(e.latency.value_or(0));
    }
    if ((actions & kActionUpdateDisplayName) != 0) {
        if (e.displayName.has_value()) {
            buf.writeBool(true);
            buf.writeString(*e.displayName); // TODO(Phase6): Component NBT
        } else {
            buf.writeBool(false);
        }
    }
    if ((actions & kActionUpdateListOrder) != 0) {
        buf.writeVarInt(e.listOrder.value_or(0));
    }
    if ((actions & kActionUpdateHat) != 0) {
        buf.writeBool(e.showHat.value_or(false));
    }
}

[[nodiscard]] inline Result<ir::play::PlayerInfoEntry> readEntry(B& buf, u16 actions)
{
    ir::play::PlayerInfoEntry e{};
    MC_TRY_ASSIGN(e.uuid, wire::readUuid(buf));
    if ((actions & kActionAddPlayer) != 0) {
        std::string name;
        MC_TRY_ASSIGN(name, buf.readString());
        e.name = std::move(name);
        i32 propCount = 0;
        MC_TRY_ASSIGN(propCount, buf.readVarInt());
        if (propCount < 0) {
            return Error(ErrorCode::InvalidData, "properties 数为负", "player_info_detail::readEntry");
        }
        for (i32 i = 0; i < propCount; ++i) {
            std::string pn;
            std::string pv;
            MC_TRY_ASSIGN(pn, buf.readString());
            MC_TRY_ASSIGN(pv, buf.readString());
            bool hasSig = false;
            MC_TRY_ASSIGN(hasSig, buf.readBool());
            if (hasSig) {
                std::string sig;
                MC_TRY_ASSIGN(sig, buf.readString());
            }
            e.properties.emplace_back(std::move(pn), std::move(pv));
        }
    }
    if ((actions & kActionInitializeChat) != 0) {
        bool hasSession = false;
        MC_TRY_ASSIGN(hasSession, buf.readBool());
        if (hasSession) {
            // RemoteChatSession.Data 暂跳过：profileId(UUID)+sessionPublicKey(ByteArray)。
            // 当前无法在不解析子结构的情况下安全跳过；TODO(Phase6) 完整实现。
            return Error(
                ErrorCode::InvalidData, "INITIALIZE_CHAT chat session 暂未支持解析", "player_info_detail::readEntry");
        }
    }
    if ((actions & kActionUpdateGameMode) != 0) {
        i32 gm = 0;
        MC_TRY_ASSIGN(gm, buf.readVarInt());
        e.gameMode = gm;
    }
    if ((actions & kActionUpdateListed) != 0) {
        bool listed = false;
        MC_TRY_ASSIGN(listed, buf.readBool());
        e.listed = listed;
    }
    if ((actions & kActionUpdateLatency) != 0) {
        i32 latency = 0;
        MC_TRY_ASSIGN(latency, buf.readVarInt());
        e.latency = latency;
    }
    if ((actions & kActionUpdateDisplayName) != 0) {
        bool hasName = false;
        MC_TRY_ASSIGN(hasName, buf.readBool());
        if (hasName) {
            std::string dn;
            MC_TRY_ASSIGN(dn, buf.readString());
            e.displayName = std::move(dn);
        }
    }
    if ((actions & kActionUpdateListOrder) != 0) {
        i32 order = 0;
        MC_TRY_ASSIGN(order, buf.readVarInt());
        e.listOrder = order;
    }
    if ((actions & kActionUpdateHat) != 0) {
        bool hat = false;
        MC_TRY_ASSIGN(hat, buf.readBool());
        e.showHat = hat;
    }
    return e;
}

} // namespace player_info_detail

/// PlayerInfoUpdate（S→C，id=68）
[[nodiscard]] inline auto playerInfoUpdateCodec()
{
    return makeCodec<ir::play::PlayerInfoUpdate>(
        [](B& buf, const ir::play::PlayerInfoUpdate& v) {
            // actions：9 位 fixedBitSet → 2 字节。写 actions 的低 16 位（小端 2 字节）。
            buf.writeU8(static_cast<u8>(v.actions & 0xFF));
            buf.writeU8(static_cast<u8>((v.actions >> 8) & 0xFF));
            buf.writeVarInt(static_cast<i32>(v.entries.size()));
            for (const auto& e : v.entries) {
                player_info_detail::writeEntry(buf, v.actions, e);
            }
        },
        [](B& buf) -> Result<ir::play::PlayerInfoUpdate> {
            ir::play::PlayerInfoUpdate v{};
            u8 b0 = 0;
            MC_TRY_ASSIGN(b0, buf.readU8());
            u8 b1 = 0;
            MC_TRY_ASSIGN(b1, buf.readU8());
            v.actions = static_cast<u16>(b0) | (static_cast<u16>(b1) << 8);
            i32 count = 0;
            MC_TRY_ASSIGN(count, buf.readVarInt());
            if (count < 0) {
                return Error(ErrorCode::InvalidData, "entries 数为负", "playerInfoUpdateCodec");
            }
            for (i32 i = 0; i < count; ++i) {
                ir::play::PlayerInfoEntry e{};
                MC_TRY_ASSIGN(e, player_info_detail::readEntry(buf, v.actions));
                v.entries.push_back(std::move(e));
            }
            return v;
        });
}

/// PlayerInfoRemove（S→C，id=67）
[[nodiscard]] inline auto playerInfoRemoveCodec()
{
    return makeCodec<ir::play::PlayerInfoRemove>(
        [](B& buf, const ir::play::PlayerInfoRemove& v) {
            buf.writeVarInt(static_cast<i32>(v.uuids.size()));
            for (const auto& uuid : v.uuids) {
                wire::writeUuid(buf, uuid);
            }
        },
        [](B& buf) -> Result<ir::play::PlayerInfoRemove> {
            ir::play::PlayerInfoRemove v{};
            i32 count = 0;
            MC_TRY_ASSIGN(count, buf.readVarInt());
            if (count < 0) {
                return Error(ErrorCode::InvalidData, "uuids 数为负", "playerInfoRemoveCodec");
            }
            for (i32 i = 0; i < count; ++i) {
                std::array<u8, 16> uuid{};
                MC_TRY_ASSIGN(uuid, wire::readUuid(buf));
                v.uuids.push_back(std::move(uuid));
            }
            return v;
        });
}

} // namespace mc::network::backend::java::codecs
