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

#include "common/item/component/DataComponentPatchWire.hpp"
#include "common/network/backend/java/codecs/JavaCodecBase.hpp"
#include "common/network/backend/java/codecs/JavaWireHelpers.hpp"
#include "common/network/buffer/NbtIo.hpp"
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
 * 线格式（对齐 ItemStack.OPTIONAL_STREAM_CODEC）：VarInt(count) —— count<=0 即空（停止）；
 * 否则 VarInt(itemId) + DataComponentPatch wire（view.componentsPatch 的原始字节）。
 * 注意：DataComponentPatch 之前**无外层长度前缀**，patch 以自身的 VarInt(addedCount)+VarInt(removedCount)
 * 自终止（对齐 vanilla DataComponentPatch.STREAM_CODEC）。componentsPatch 由 ItemStack↔ItemStackView
 * 桥接（ItemStackBridge.hpp）预先序列化，本 codec 只透传其字节，保持 IR 对线格式中立。
 * **空 patch 也必须写出 0x00 0x00**（VarInt(0)+VarInt(0)）：vanilla DataComponentPatch.STREAM_CODEC
 * 永远产出 patch 区段，读侧（readItemStack/readPatchBytesFromWire）无条件消费之。若 componentsPatch
 * 为空 vector（如 particle item 直接构造、未走 bridge 的路径）却省略写入，读侧会越界。故空时补写
 * 两个 VarInt(0)（与 writePatchToWire 对空 patch 的产出一致）。
 */
inline void writeItemStack(B& buf, const ir::play::ItemStackView& v)
{
    if (v.count <= 0) {
        buf.writeVarInt(0);
        return;
    }
    buf.writeVarInt(v.count);
    buf.writeVarInt(static_cast<i32>(v.itemId));
    if (!v.componentsPatch.empty()) {
        buf.writeBytes(v.componentsPatch.data(), v.componentsPatch.size());
    } else {
        // 空 patch 区段：VarInt(addedCount=0) + VarInt(removedCount=0)，即 0x00 0x00。
        buf.writeVarInt(0);
        buf.writeVarInt(0);
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
    // DataComponentPatch 按 vanilla 自终止规则消费，原样存入 componentsPatch（无外层长度前缀）。
    MC_TRY_ASSIGN(v.componentsPatch, ::mc::item::component::readPatchBytesFromWire(buf));
    return v;
}

/**
 * @brief 写 HashedStack（ContainerClick 用）
 *
 * 对应 Java HashedStack：present=false 写 Bool(false)；true 写 Bool(true)+ActualItem
 * { VarInt(itemId) + VarInt(count) + HashedPatchMap }。
 * HashedPatchMap 我方双端均写空（added=VarInt(0) + removed=VarInt(0)），帧格式完整、
 * 双端自洽；真 Java 客户端能解析（哈希值为空表示无组件校验）。
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
    // HashedPatchMap：added(Map) 与 removed(Set) 各以 VarInt(size) 起始。我方写空。
    buf.writeVarInt(0); // addedComponents size
    buf.writeVarInt(0); // removedComponents size
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
    // 跳过 HashedPatchMap：added(Map<DataComponentType,Int>) + removed(Set<DataComponentType>)。
    // 每项以 VarInt(registryId) 标识组件类型；Map 项多一个 Int 哈希值。我方不消费哈希，仅按定界跳过，
    // 保证真 Java 服务端发来的带组件哈希的 HashedStack 不错位。
    i32 addedCount = 0;
    MC_TRY_ASSIGN(addedCount, buf.readVarInt());
    if (addedCount < 0) {
        return Error(ErrorCode::InvalidData, "HashedPatchMap added count is negative", "readHashedStack");
    }
    for (i32 i = 0; i < addedCount; ++i) {
        MC_TRY(buf.readVarInt()); // componentType registryId
        MC_TRY(buf.readI32());    // hash value
    }
    i32 removedCount = 0;
    MC_TRY_ASSIGN(removedCount, buf.readVarInt());
    if (removedCount < 0) {
        return Error(ErrorCode::InvalidData, "HashedPatchMap removed count is negative", "readHashedStack");
    }
    for (i32 i = 0; i < removedCount; ++i) {
        MC_TRY(buf.readVarInt()); // componentType registryId
    }
    return v;
}

/**
 * @brief 写 CommonPlayerSpawnInfo（Login 内联子结构）
 */
inline void writeSpawnInfo(B& buf, const ir::play::CommonPlayerSpawnInfo& s)
{
    // dimensionType：vanilla Holder<DimensionType> = ByteBufCodecs.holderRegistry
    // （DimensionType.STREAM_CODEC）。wire = 纯 VarInt(registryId)，无 mode 前缀、无内联 NBT 分支
    // （holderRegistry 编码 = VarInt.write(registry.getIdOrThrow(holder))）。dimension_type 注册表
    // 由 Configuration 阶段 RegistryDataBuilder 同步（overworld=id0 / overworld_caves=id1 /
    // the_nether=id2 / the_end=id3）。s.dimensionType 存 dimension_type registry id，直接编 id。
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
    // dimensionType：vanilla holderRegistry = 纯 VarInt(registryId)，无 mode 前缀、无内联 NBT 分支。
    // 直接读 registry id（dimension_type registry：overworld=id0 …）。
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

/// KeepAlive（双向，I64(id) 固定8字节大端，对齐 vanilla writeLong/readLong）
[[nodiscard]] inline auto keepAliveCodec()
{
    return makeCodec<ir::play::KeepAlive>([](B& buf, const ir::play::KeepAlive& v) { buf.writeI64(v.id); },
        [](B& buf) -> Result<ir::play::KeepAlive> {
            ir::play::KeepAlive v{};
            MC_TRY_ASSIGN(v.id, buf.readI64());
            return v;
        });
}

/// Disconnect（S→C，reason 为 NBT 承载的 Component，非裸字符串）
[[nodiscard]] inline auto playDisconnectCodec()
{
    return makeCodec<ir::play::Disconnect>(
        [](B& buf, const ir::play::Disconnect& v) { writeTextComponentNbt(buf, v.reason); },
        [](B& buf) -> Result<ir::play::Disconnect> {
            ir::play::Disconnect v{};
            MC_TRY_ASSIGN(v.reason, readTextComponentNbt(buf));
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
                return Error(ErrorCode::InvalidData, "changedSlots count is negative", "containerClickCodec");
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
                return Error(ErrorCode::InvalidData, "levels count is negative", "loginCodec");
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

/// SetChunkCacheCenter（S→C，id=92）：VarInt(x) + VarInt(z)
[[nodiscard]] inline auto setChunkCacheCenterCodec()
{
    return makeCodec<ir::play::SetChunkCacheCenter>(
        [](B& buf, const ir::play::SetChunkCacheCenter& v) {
            buf.writeVarInt(v.x);
            buf.writeVarInt(v.z);
        },
        [](B& buf) -> Result<ir::play::SetChunkCacheCenter> {
            ir::play::SetChunkCacheCenter v{};
            MC_TRY_ASSIGN(v.x, buf.readVarInt());
            MC_TRY_ASSIGN(v.z, buf.readVarInt());
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
                return Error(ErrorCode::InvalidData, "entityIds count is negative", "removeEntitiesCodec");
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
/// packedItems 由 EntityMetadataSerializer 按 1.21.11 格式（byte idx + VarInt serializerId + value）生成。
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
/// 线格式（1.21.11 ClientboundLevelChunkWithLightPacket，无外层长度前缀）：
///   i32 x + i32 z +
///   [heightmaps + VarInt(sectionBufLen)+sectionBuf + blockEntities] +
///   [skyYMask + blockYMask + emptySkyYMask + emptyBlockYMask + skyUpdates + blockUpdates]
/// heightmaps = VarInt(count) + count×(VarInt(typeId) + VarInt(longCount) + longCount×i64)。
/// sectionBuf = 24 段连续 { i16 nonEmptyBlockCount + states PalettedContainer + biomes PalettedContainer }。
/// PalettedContainer = u8 bits + palette(bits=0:VarInt(1×globalId);1-8:VarInt(size)+size×VarInt;
///                     ≥9:空) + long[] storage(无前缀，longCount 由 bits+entryCount 隐含，大端)。
/// blockEntities = VarInt(count) + count×(u8 packedXZ + i16 y + VarInt(typeRegistryId) + 根 NBT)。
/// BitSet = VarInt(longCount) + longCount×i64(最小数组)；List = VarInt(n) + n×(VarInt(2048)+bytes)。
namespace play_detail {
/// 编码 PalettedContainer IR 为 vanilla wire（无外层长度前缀）。
inline void writePalettedContainerWire(B& buf, const ir::play::PalettedContainerWire& pc)
{
    buf.writeU8(pc.bits);
    if (pc.bits == 0) {
        // SingleValue：1 个全局 id，storage 空。
        buf.writeVarInt(pc.paletteGlobalIds.empty() ? 0 : static_cast<i32>(pc.paletteGlobalIds[0]));
    } else if (pc.bits <= 8) {
        // Linear/HashMap：VarInt(size) + size×VarInt(globalId)。
        buf.writeVarInt(static_cast<i32>(pc.paletteGlobalIds.size()));
        for (const u32 gid : pc.paletteGlobalIds) {
            buf.writeVarInt(static_cast<i32>(gid));
        }
    }
    // bits >= 9（Global）：palette 空，storage 直接存全局 id。
    // storage：无长度前缀，逐元素大端 writeI64。
    for (const u64 word : pc.storage) {
        buf.writeI64(static_cast<i64>(word));
    }
}

/// 解码 vanilla wire PalettedContainer 为 IR。entryCount 仅用于 bits==0 时不参与（仅校验），
/// storage 长度由 bits 与 entryCount 隐含（longCount = ceil(entryCount / floor(64/bits))）。
[[nodiscard]] inline Result<ir::play::PalettedContainerWire> readPalettedContainerWire(B& buf, int entryCount)
{
    ir::play::PalettedContainerWire pc;
    u8 bits = 0;
    MC_TRY_ASSIGN(bits, buf.readU8());
    pc.bits = bits;
    if (bits == 0) {
        // SingleValue
        i32 value = 0;
        MC_TRY_ASSIGN(value, buf.readVarInt());
        if (value < 0) {
            return Error(
                ErrorCode::InvalidData, "PalettedContainer single value is negative", "levelChunkWithLightCodec");
        }
        pc.paletteGlobalIds.push_back(static_cast<u32>(value));
        return pc;
    }
    if (bits > 15) {
        return Error(ErrorCode::InvalidData, "PalettedContainer bits out of range", "levelChunkWithLightCodec");
    }
    if (bits <= 8) {
        // Linear/HashMap palette
        i32 size = 0;
        MC_TRY_ASSIGN(size, buf.readVarInt());
        if (size < 0 || size > 4096) {
            return Error(
                ErrorCode::InvalidData, "PalettedContainer palette size out of range", "levelChunkWithLightCodec");
        }
        pc.paletteGlobalIds.reserve(static_cast<usize>(size));
        for (i32 i = 0; i < size; ++i) {
            i32 gid = 0;
            MC_TRY_ASSIGN(gid, buf.readVarInt());
            if (gid < 0) {
                return Error(
                    ErrorCode::InvalidData, "PalettedContainer palette id is negative", "levelChunkWithLightCodec");
            }
            pc.paletteGlobalIds.push_back(static_cast<u32>(gid));
        }
    }
    // Global（bits >= 9）：palette 空。
    // storage：longCount = ceil(entryCount / floor(64/bits))，与 VanillaChunkWire 打包侧一致。
    const int valuesPerLong = 64 / bits;
    const int longCount = (entryCount + valuesPerLong - 1) / valuesPerLong;
    pc.storage.reserve(static_cast<usize>(longCount));
    for (int i = 0; i < longCount; ++i) {
        i64 word = 0;
        MC_TRY_ASSIGN(word, buf.readI64());
        pc.storage.push_back(static_cast<u64>(word));
    }
    return pc;
}
} // namespace play_detail

[[nodiscard]] inline auto levelChunkWithLightCodec()
{
    return makeCodec<ir::play::LevelChunkWithLight>(
        [](B& buf, const ir::play::LevelChunkWithLight& v) {
            buf.writeI32(v.x);
            buf.writeI32(v.z);

            // heightmaps：VarInt(count) + count×(VarInt(typeId) + VarInt(longCount) + longCount×i64)。
            buf.writeVarInt(static_cast<i32>(v.heightmaps.size()));
            for (const auto& hm : v.heightmaps) {
                buf.writeVarInt(static_cast<i32>(hm.typeId));
                buf.writeVarInt(static_cast<i32>(hm.data.size()));
                for (const u64 word : hm.data) {
                    buf.writeI64(static_cast<i64>(word));
                }
            }

            // sections：编进子 buffer，前置 VarInt(sectionBufLen)。
            B sectionBuf;
            for (const auto& sec : v.sections) {
                sectionBuf.writeI16(sec.nonEmptyBlockCount);
                play_detail::writePalettedContainerWire(sectionBuf, sec.states);
                play_detail::writePalettedContainerWire(sectionBuf, sec.biomes);
            }
            buf.writeVarInt(static_cast<i32>(sectionBuf.size()));
            buf.writeBytes(sectionBuf.data(), sectionBuf.size());

            // blockEntities：VarInt(count) + count×(u8 packedXZ + i16 y + VarInt(typeRegistryId) + 根 NBT)。
            buf.writeVarInt(static_cast<i32>(v.blockEntities.size()));
            for (const auto& be : v.blockEntities) {
                buf.writeU8(be.packedXZ);
                buf.writeI16(be.y);
                buf.writeVarInt(static_cast<i32>(be.typeRegistryId));
                const nbt::CompoundTag emptyTag{};
                const nbt::CompoundTag& tag = be.tag ? *be.tag : emptyTag;
                (void)buffer::nbt_io::writeRootCompound(buf, tag);
            }

            // 光照：4 BitSet（最小 long 数组）+ 2 List。
            for (const auto& mask : v.lightMasks) {
                buf.writeVarInt(static_cast<i32>(mask.size()));
                for (const u64 word : mask) {
                    buf.writeI64(static_cast<i64>(word));
                }
            }
            for (const auto& updates : v.lightUpdates) {
                buf.writeVarInt(static_cast<i32>(updates.size()));
                for (const auto& nibble : updates) {
                    buf.writeVarInt(static_cast<i32>(nibble.size()));
                    buf.writeBytes(nibble.data(), nibble.size());
                }
            }
        },
        [](B& buf) -> Result<ir::play::LevelChunkWithLight> {
            ir::play::LevelChunkWithLight v{};
            MC_TRY_ASSIGN(v.x, buf.readI32());
            MC_TRY_ASSIGN(v.z, buf.readI32());

            // heightmaps
            i32 hmCount = 0;
            MC_TRY_ASSIGN(hmCount, buf.readVarInt());
            if (hmCount < 0 || hmCount > 16) {
                return Error(ErrorCode::InvalidData, "heightmap count out of range", "levelChunkWithLightCodec");
            }
            v.heightmaps.reserve(static_cast<usize>(hmCount));
            for (i32 i = 0; i < hmCount; ++i) {
                ir::play::HeightmapEntryWire hm{};
                i32 typeId = 0;
                MC_TRY_ASSIGN(typeId, buf.readVarInt());
                hm.typeId = static_cast<u8>(typeId);
                i32 longCount = 0;
                MC_TRY_ASSIGN(longCount, buf.readVarInt());
                if (longCount < 0 || longCount > 64) {
                    return Error(
                        ErrorCode::InvalidData, "heightmap longCount out of range", "levelChunkWithLightCodec");
                }
                hm.data.reserve(static_cast<usize>(longCount));
                for (i32 j = 0; j < longCount; ++j) {
                    i64 word = 0;
                    MC_TRY_ASSIGN(word, buf.readI64());
                    hm.data.push_back(static_cast<u64>(word));
                }
                v.heightmaps.push_back(std::move(hm));
            }

            // sections
            i32 sectionBufLen = 0;
            MC_TRY_ASSIGN(sectionBufLen, buf.readVarInt());
            if (sectionBufLen < 0 || sectionBufLen > 1024 * 1024) {
                return Error(ErrorCode::InvalidData, "section buffer length out of range", "levelChunkWithLightCodec");
            }
            // 段数由 sectionBufLen 隐含：逐段解码直到子 buffer 耗尽。
            // 但 wire 里 sectionBuf 是主 buf 的一段连续字节，需切出子视图读取。
            // 这里直接在主 buf 上顺序读：先记录起点，读到消费 sectionBufLen 字节为止。
            const usize sectionStart = 0; // 占位，下方用 readBytesView 切片
            (void)sectionStart;
            // 取出 sectionBufLen 字节作为子 buffer，逐段解码。
            std::vector<u8> sectionBytes;
            MC_TRY_ASSIGN(sectionBytes, buf.readBytes(static_cast<usize>(sectionBufLen)));
            B sectionBuf(sectionBytes.data(), sectionBytes.size());
            // 段数上限防御：主世界 24，扩展维度最多 64。
            while (sectionBuf.readableBytes() > 0) {
                if (v.sections.size() >= 64) {
                    return Error(ErrorCode::InvalidData, "too many sections", "levelChunkWithLightCodec");
                }
                ir::play::ChunkSectionWire sec{};
                MC_TRY_ASSIGN(sec.nonEmptyBlockCount, sectionBuf.readI16());
                MC_TRY_ASSIGN(sec.states, play_detail::readPalettedContainerWire(sectionBuf, 4096));
                MC_TRY_ASSIGN(sec.biomes, play_detail::readPalettedContainerWire(sectionBuf, 64));
                v.sections.push_back(std::move(sec));
            }

            // blockEntities
            i32 beCount = 0;
            MC_TRY_ASSIGN(beCount, buf.readVarInt());
            if (beCount < 0 || beCount > 1024) {
                return Error(ErrorCode::InvalidData, "block entity count out of range", "levelChunkWithLightCodec");
            }
            v.blockEntities.reserve(static_cast<usize>(beCount));
            for (i32 i = 0; i < beCount; ++i) {
                ir::play::BlockEntityInfoWire be{};
                MC_TRY_ASSIGN(be.packedXZ, buf.readU8());
                MC_TRY_ASSIGN(be.y, buf.readI16());
                i32 typeId = 0;
                MC_TRY_ASSIGN(typeId, buf.readVarInt());
                be.typeRegistryId = static_cast<u32>(typeId);
                auto tagResult = buffer::nbt_io::readRootCompound(buf);
                if (tagResult.failed()) {
                    return tagResult.error();
                }
                be.tag = std::shared_ptr<nbt::CompoundTag>(tagResult.value().release());
                v.blockEntities.push_back(std::move(be));
            }

            // 光照：4 BitSet + 2 List
            for (auto& mask : v.lightMasks) {
                i32 longCount = 0;
                MC_TRY_ASSIGN(longCount, buf.readVarInt());
                if (longCount < 0 || longCount > 64) {
                    return Error(
                        ErrorCode::InvalidData, "light mask longCount out of range", "levelChunkWithLightCodec");
                }
                mask.reserve(static_cast<usize>(longCount));
                for (i32 i = 0; i < longCount; ++i) {
                    i64 word = 0;
                    MC_TRY_ASSIGN(word, buf.readI64());
                    mask.push_back(static_cast<u64>(word));
                }
            }
            for (auto& updates : v.lightUpdates) {
                i32 elemCount = 0;
                MC_TRY_ASSIGN(elemCount, buf.readVarInt());
                if (elemCount < 0 || elemCount > 64) {
                    return Error(ErrorCode::InvalidData, "light list count out of range", "levelChunkWithLightCodec");
                }
                updates.reserve(static_cast<usize>(elemCount));
                for (i32 i = 0; i < elemCount; ++i) {
                    i32 byteLen = 0;
                    MC_TRY_ASSIGN(byteLen, buf.readVarInt());
                    if (byteLen < 0 || byteLen > 2048) {
                        return Error(
                            ErrorCode::InvalidData, "light nibble size out of range", "levelChunkWithLightCodec");
                    }
                    std::vector<u8> nibble;
                    MC_TRY_ASSIGN(nibble, buf.readBytes(static_cast<usize>(byteLen)));
                    updates.push_back(std::move(nibble));
                }
            }
            return v;
        });
}

/// LightUpdate（S→C，id=47）
///
/// 线格式（1.21.11 ClientboundLightUpdatePacket）：
///   VarInt(x) + VarInt(z) +
///   skyYMask + blockYMask + emptySkyYMask + emptyBlockYMask +
///   skyUpdates + blockUpdates
/// BitSet = VarInt(longCount) + longCount×i64（大端）；空 BitSet 写 longCount=0。
/// 列表 = VarInt(elemCount) + elemCount×(VarInt(byteLen) + bytes)，每条 nibble ≤2048 字节。
/// 无外层长度前缀（包长已在传输层 VarInt 帧头）。
[[nodiscard]] inline auto lightUpdateCodec()
{
    return makeCodec<ir::play::LightUpdate>(
        [](B& buf, const ir::play::LightUpdate& v) {
            buf.writeVarInt(v.x);
            buf.writeVarInt(v.z);
            // 四个 BitSet（最小长整型数组形式）：skyYMask / blockYMask / emptySkyYMask / emptyBlockYMask
            for (const auto& mask : v.lightMasks) {
                buf.writeVarInt(static_cast<i32>(mask.size()));
                for (i64 word : mask) {
                    buf.writeI64(word);
                }
            }
            // 两个 nibble 列表：skyUpdates / blockUpdates
            for (const auto& updates : v.lightUpdates) {
                buf.writeVarInt(static_cast<i32>(updates.size()));
                for (const auto& nibble : updates) {
                    buf.writeVarInt(static_cast<i32>(nibble.size()));
                    buf.writeBytes(nibble.data(), nibble.size());
                }
            }
        },
        [](B& buf) -> Result<ir::play::LightUpdate> {
            ir::play::LightUpdate v{};
            MC_TRY_ASSIGN(v.x, buf.readVarInt());
            MC_TRY_ASSIGN(v.z, buf.readVarInt());
            for (auto& mask : v.lightMasks) {
                i32 longCount = 0;
                MC_TRY_ASSIGN(longCount, buf.readVarInt());
                if (longCount < 0) {
                    return Error(ErrorCode::InvalidData, "LightUpdate mask longCount is negative", "lightUpdateCodec");
                }
                // 上限：单 BitSet 不应超过光照段数对应的 long 数（防御游标错位致 OOM）。
                // 主世界 26 段 < 1 long；256 段维度也仅 4 long。给 64 long（4096 段）余量足够。
                if (longCount > 64) {
                    return Error(ErrorCode::InvalidData, "LightUpdate mask longCount too large", "lightUpdateCodec");
                }
                mask.reserve(static_cast<usize>(longCount));
                for (i32 i = 0; i < longCount; ++i) {
                    i64 word = 0;
                    MC_TRY_ASSIGN(word, buf.readI64());
                    mask.push_back(word);
                }
            }
            for (auto& updates : v.lightUpdates) {
                i32 elemCount = 0;
                MC_TRY_ASSIGN(elemCount, buf.readVarInt());
                if (elemCount < 0) {
                    return Error(ErrorCode::InvalidData, "LightUpdate list count is negative", "lightUpdateCodec");
                }
                if (elemCount > 64) {
                    return Error(ErrorCode::InvalidData, "LightUpdate list count too large", "lightUpdateCodec");
                }
                updates.reserve(static_cast<usize>(elemCount));
                for (i32 i = 0; i < elemCount; ++i) {
                    i32 byteLen = 0;
                    MC_TRY_ASSIGN(byteLen, buf.readVarInt());
                    if (byteLen < 0 || byteLen > 2048) {
                        return Error(
                            ErrorCode::InvalidData, "LightUpdate nibble size out of range", "lightUpdateCodec");
                    }
                    std::vector<u8> nibble;
                    MC_TRY_ASSIGN(nibble, buf.readBytes(static_cast<usize>(byteLen)));
                    updates.push_back(std::move(nibble));
                }
            }
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
                return Error(ErrorCode::InvalidData, "items count is negative", "containerSetContentCodec");
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
 * @brief 跳过 VarInt 长度前缀的字节串（ByteArray）
 *
 * 对齐 Java FriendlyByteBuf.readByteArray：VarInt(len) + len 字节。仅消费不解析。
 */
[[nodiscard]] inline Result<void> skipByteArray(B& buf)
{
    i32 length = 0;
    MC_TRY_ASSIGN(length, buf.readVarInt());
    if (length < 0) {
        return Error(ErrorCode::InvalidData, "ByteArray length is negative", "player_info_detail::skipByteArray");
    }
    MC_TRY(buf.readBytesView(static_cast<usize>(length))); // 零拷贝消费
    return Result<void>::ok();
}

/**
 * @brief 安全跳过 RemoteChatSession.Data（INITIALIZE_CHAT 携带的 chat session）
 *
 * Java 线格式（RemoteChatSession.Data + ProfilePublicKey.Data）：
 *   UUID sessionId + Instant expiresAt(i64 epochMilli 大端)
 *   + PublicKey key(VarInt derLen + der 字节) + byte[] keySignature(VarInt len + 字节)
 *
 * 本函数仅按定界跳过字节，不解析语义——我方不消费 chat session，只需保证真 Java
 * 服务端发来的 PlayerInfoUpdate(INITIALIZE_CHAT) 不因 hasSession=true 而整体解析失败。
 */
[[nodiscard]] inline Result<void> skipChatSessionData(B& buf)
{
    MC_TRY(wire::readUuid(buf)); // sessionId
    MC_TRY(buf.readI64());       // expiresAt (Instant, i64 epochMilli 大端)
    MC_TRY(skipByteArray(buf));  // key (PublicKey der)
    MC_TRY(skipByteArray(buf));  // keySignature
    return Result<void>::ok();
}

/**
 * @brief 写 entries 的 action 负载（按 actions 中 set 的位顺序）
 *
 * INITIALIZE_CHAT：我方双端均不携带 chat session（写 Bool(false)），自洽；
 * 读侧若对端（真 Java 服务端）发来 hasSession=true，则按 RemoteChatSession.Data
 * 定界安全跳过（见 skipChatSessionData），不解析语义、不拖垮整个 PlayerInfoUpdate。
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
        // 我方不生产显示名（IR PlayerInfoEntry 不承载 displayName 字段），写 Bool(false)。
        // 真实 Component NBT 写入（nbt_io::writeCompound）待上层接入 ITextComponent NBT codec 后补。
        buf.writeBool(false);
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
            return Error(ErrorCode::InvalidData, "properties count is negative", "player_info_detail::readEntry");
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
            // 我方不消费 chat session，但对端（真 Java 服务端）可能携带 RemoteChatSession.Data。
            // 按定界安全跳过，避免拖垮整个 PlayerInfoUpdate 包解析。
            MC_TRY(skipChatSessionData(buf));
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
            // 真实 Component NBT（Java ComponentSerialization.STREAM_CODEC，NBT compound 自定界）。
            // 我方不消费显示名，按 NBT compound 跳过以保证真 Java 服务端包不错位。
            MC_TRY(buffer::nbt_io::skipCompound(buf));
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
                return Error(ErrorCode::InvalidData, "entries count is negative", "playerInfoUpdateCodec");
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
                return Error(ErrorCode::InvalidData, "uuids count is negative", "playerInfoRemoveCodec");
            }
            for (i32 i = 0; i < count; ++i) {
                std::array<u8, 16> uuid{};
                MC_TRY_ASSIGN(uuid, wire::readUuid(buf));
                v.uuids.push_back(std::move(uuid));
            }
            return v;
        });
}

// ============================================================================
// 简单状态同步单包（S→C / C→S，1.21.11 纯字段包，altIndex 90..98）
// ============================================================================

/// SetChunkCacheRadius（S→C，id=93）：VarInt(radius)
[[nodiscard]] inline auto setChunkCacheRadiusCodec()
{
    return makeCodec<ir::play::SetChunkCacheRadius>(
        [](B& buf, const ir::play::SetChunkCacheRadius& v) { buf.writeVarInt(v.radius); },
        [](B& buf) -> Result<ir::play::SetChunkCacheRadius> {
            ir::play::SetChunkCacheRadius v{};
            MC_TRY_ASSIGN(v.radius, buf.readVarInt());
            return v;
        });
}

/// SetSimulationDistance（S→C，id=109）：VarInt(simulationDistance)
[[nodiscard]] inline auto setSimulationDistanceCodec()
{
    return makeCodec<ir::play::SetSimulationDistance>(
        [](B& buf, const ir::play::SetSimulationDistance& v) { buf.writeVarInt(v.simulationDistance); },
        [](B& buf) -> Result<ir::play::SetSimulationDistance> {
            ir::play::SetSimulationDistance v{};
            MC_TRY_ASSIGN(v.simulationDistance, buf.readVarInt());
            return v;
        });
}

/// SetHealth（S→C，id=102）：Float(health)+VarInt(food)+Float(saturation)
[[nodiscard]] inline auto setHealthCodec()
{
    return makeCodec<ir::play::SetHealth>(
        [](B& buf, const ir::play::SetHealth& v) {
            buf.writeF32(v.health);
            buf.writeVarInt(v.food);
            buf.writeF32(v.saturation);
        },
        [](B& buf) -> Result<ir::play::SetHealth> {
            ir::play::SetHealth v{};
            MC_TRY_ASSIGN(v.health, buf.readF32());
            MC_TRY_ASSIGN(v.food, buf.readVarInt());
            MC_TRY_ASSIGN(v.saturation, buf.readF32());
            return v;
        });
}

/// ClientboundPing（S→C，id=59，common 通道）：Int(id)
[[nodiscard]] inline auto clientboundPingCodec()
{
    return makeCodec<ir::play::ClientboundPing>([](B& buf, const ir::play::ClientboundPing& v) { buf.writeI32(v.id); },
        [](B& buf) -> Result<ir::play::ClientboundPing> {
            ir::play::ClientboundPing v{};
            MC_TRY_ASSIGN(v.id, buf.readI32());
            return v;
        });
}

/// PongResponse（S→C，id=60，ping 协议通道）：Long(time)
[[nodiscard]] inline auto pongResponseCodec()
{
    return makeCodec<ir::play::PongResponse>([](B& buf, const ir::play::PongResponse& v) { buf.writeI64(v.time); },
        [](B& buf) -> Result<ir::play::PongResponse> {
            ir::play::PongResponse v{};
            MC_TRY_ASSIGN(v.time, buf.readI64());
            return v;
        });
}

/// ServerboundPingRequest（C→S，id=37，ping 协议通道）：Long(time)
[[nodiscard]] inline auto serverboundPingRequestCodec()
{
    return makeCodec<ir::play::ServerboundPingRequest>(
        [](B& buf, const ir::play::ServerboundPingRequest& v) { buf.writeI64(v.time); },
        [](B& buf) -> Result<ir::play::ServerboundPingRequest> {
            ir::play::ServerboundPingRequest v{};
            MC_TRY_ASSIGN(v.time, buf.readI64());
            return v;
        });
}

/// ServerboundPong（C→S，id=44，common 通道）：Int(id)
[[nodiscard]] inline auto serverboundPongCodec()
{
    return makeCodec<ir::play::ServerboundPong>([](B& buf, const ir::play::ServerboundPong& v) { buf.writeI32(v.id); },
        [](B& buf) -> Result<ir::play::ServerboundPong> {
            ir::play::ServerboundPong v{};
            MC_TRY_ASSIGN(v.id, buf.readI32());
            return v;
        });
}

/// ServerboundChangeDifficulty（C→S，id=3）：VarInt(difficulty)
[[nodiscard]] inline auto serverboundChangeDifficultyCodec()
{
    return makeCodec<ir::play::ServerboundChangeDifficulty>(
        [](B& buf, const ir::play::ServerboundChangeDifficulty& v) { buf.writeVarInt(v.difficulty); },
        [](B& buf) -> Result<ir::play::ServerboundChangeDifficulty> {
            ir::play::ServerboundChangeDifficulty v{};
            MC_TRY_ASSIGN(v.difficulty, buf.readVarInt());
            return v;
        });
}

/// LockDifficulty（C→S，id=28）：Bool(locked)
[[nodiscard]] inline auto lockDifficultyCodec()
{
    return makeCodec<ir::play::LockDifficulty>(
        [](B& buf, const ir::play::LockDifficulty& v) { buf.writeBool(v.locked); },
        [](B& buf) -> Result<ir::play::LockDifficulty> {
            ir::play::LockDifficulty v{};
            MC_TRY_ASSIGN(v.locked, buf.readBool());
            return v;
        });
}

} // namespace mc::network::backend::java::codecs