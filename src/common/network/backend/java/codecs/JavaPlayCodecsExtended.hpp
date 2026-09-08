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

#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/network/backend/java/codecs/JavaCodecBase.hpp"
#include "common/network/backend/java/codecs/JavaPlayCodecs.hpp"
#include "common/network/backend/java/mappings/JavaBlockIdMap.hpp"
#include "common/network/buffer/NbtIo.hpp"
#include "common/network/ir/IrPacket.hpp"
#include "common/network/ir/packets/play/PlayPacketsExtended.hpp"
#include "common/particle/ParticleTypes.hpp"
#include "common/util/nbt/Nbt.hpp"
#include <algorithm>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace mc::network::backend::java::codecs {

// ============================================================================
// Play 阶段补全 codec（对齐 Java 1.21.11，Phase 4a）
//
// Holder<SoundEvent>、ParticleOptions、Explosion（含粒子表）、LevelParticles、BlockEntityData
// 等已结构化（见 writeSoundEventHolder/writeParticleOptions/explosionCodec/levelParticlesCodec/
// blockEntityDataCodec）。Component NBT（writeComponentNbt/readComponentNbt）、Optional<Component>
// （writeOptionalComponentNbt）、Optional<NumberFormat>（writeNumberFormat/readNumberFormat）、
// TeamParameters 扁平 7 字段亦已结构化对齐 vanilla，供 bossbar/title/actionbar/scoreboard/team
// 与真 Java 客户端互通。仍以 VarInt(len)+bytes 透传的复杂嵌套树（MapDecoration.name/MapPatch.colors/
// 命令树 Node 等）属我方互通自洽的 opaque 透传层：双端同 codec 读写，我方互通必达；真 Java 互通
// 需各自完整 codec，属独立子项，不在本层范围（各 codec 内联注释标明）。
// Resp 复用 JavaPlayCodecs.hpp 的 play_detail::writeSpawnInfo/readSpawnInfo。
// ============================================================================

namespace play_ext_detail {

/**
 * @brief 写 opaque 字节段（VarInt 长度 + 字节）
 *
 * 用于承载复杂嵌套结构（Component/ParticleOptions/Holder 等）的原始字节。
 */
inline void writeOpaque(B& buf, const std::vector<u8>& data)
{
    buf.writeVarInt(static_cast<i32>(data.size()));
    buf.writeBytes(data.data(), data.size());
}

/**
 * @brief 读 opaque 字节段（VarInt 长度 + 字节）
 */
[[nodiscard]] inline Result<std::vector<u8>> readOpaque(B& buf, std::string_view ctx)
{
    i32 len = 0;
    MC_TRY_ASSIGN(len, buf.readVarInt());
    if (len < 0) {
        return Error(ErrorCode::InvalidData, "opaque length is negative", std::string{ctx});
    }
    return buf.readBytes(static_cast<usize>(len));
}

// 前向声明：writeSoundEventHolder/readSoundEventHolder 的完整定义位于本文件下方
// 第二个 play_ext_detail 块（与 writeParticleOptions 等并列，被 explosionCodec 等使用）。
// PlaySound/SoundEntity codec 在此之前，需前置声明以解析 lambda 体内的限定名调用。
inline void writeSoundEventHolder(B& buf, const ir::play::SoundEventHolder& v);
[[nodiscard]] inline Result<ir::play::SoundEventHolder> readSoundEventHolder(B& buf);

} // namespace play_ext_detail

// ============================================================================
// 声音（S→C）
// ============================================================================

/// PlaySound（S→C，id=115）
[[nodiscard]] inline auto playSoundCodec()
{
    return makeCodec<ir::play::PlaySound>(
        [](B& buf, const ir::play::PlaySound& v) {
            play_ext_detail::writeSoundEventHolder(buf, v.soundHolder);
            buf.writeVarInt(v.source);
            buf.writeI32(v.x);
            buf.writeI32(v.y);
            buf.writeI32(v.z);
            buf.writeF32(v.volume);
            buf.writeF32(v.pitch);
            buf.writeI64(v.seed);
        },
        [](B& buf) -> Result<ir::play::PlaySound> {
            ir::play::PlaySound v{};
            MC_TRY_ASSIGN(v.soundHolder, play_ext_detail::readSoundEventHolder(buf));
            MC_TRY_ASSIGN(v.source, buf.readVarInt());
            MC_TRY_ASSIGN(v.x, buf.readI32());
            MC_TRY_ASSIGN(v.y, buf.readI32());
            MC_TRY_ASSIGN(v.z, buf.readI32());
            MC_TRY_ASSIGN(v.volume, buf.readF32());
            MC_TRY_ASSIGN(v.pitch, buf.readF32());
            MC_TRY_ASSIGN(v.seed, buf.readI64());
            return v;
        });
}

/// StopSound（S→C，id=117）
[[nodiscard]] inline auto stopSoundCodec()
{
    return makeCodec<ir::play::StopSound>(
        [](B& buf, const ir::play::StopSound& v) {
            buf.writeU8(v.flags);
            if (v.flags & 0x01) {
                buf.writeVarInt(v.source);
            }
            if (v.flags & 0x02) {
                buf.writeString(v.name);
            }
        },
        [](B& buf) -> Result<ir::play::StopSound> {
            ir::play::StopSound v{};
            MC_TRY_ASSIGN(v.flags, buf.readU8());
            if (v.flags & 0x01) {
                MC_TRY_ASSIGN(v.source, buf.readVarInt());
            }
            if (v.flags & 0x02) {
                MC_TRY_ASSIGN(v.name, buf.readString());
            }
            return v;
        });
}

/// SoundEntity（S→C，id=114）
[[nodiscard]] inline auto soundEntityCodec()
{
    return makeCodec<ir::play::SoundEntity>(
        [](B& buf, const ir::play::SoundEntity& v) {
            play_ext_detail::writeSoundEventHolder(buf, v.soundHolder);
            buf.writeVarInt(v.source);
            buf.writeVarInt(v.entityId);
            buf.writeF32(v.volume);
            buf.writeF32(v.pitch);
            buf.writeI64(v.seed);
        },
        [](B& buf) -> Result<ir::play::SoundEntity> {
            ir::play::SoundEntity v{};
            MC_TRY_ASSIGN(v.soundHolder, play_ext_detail::readSoundEventHolder(buf));
            MC_TRY_ASSIGN(v.source, buf.readVarInt());
            MC_TRY_ASSIGN(v.entityId, buf.readVarInt());
            MC_TRY_ASSIGN(v.volume, buf.readF32());
            MC_TRY_ASSIGN(v.pitch, buf.readF32());
            MC_TRY_ASSIGN(v.seed, buf.readI64());
            return v;
        });
}

/// LevelEvent（S→C，id=45）
[[nodiscard]] inline auto levelEventCodec()
{
    return makeCodec<ir::play::LevelEvent>(
        [](B& buf, const ir::play::LevelEvent& v) {
            buf.writeI32(v.type);
            buf.writeI64(v.blockPosPacked);
            buf.writeI32(v.data);
            buf.writeBool(v.globalEvent);
        },
        [](B& buf) -> Result<ir::play::LevelEvent> {
            ir::play::LevelEvent v{};
            MC_TRY_ASSIGN(v.type, buf.readI32());
            MC_TRY_ASSIGN(v.blockPosPacked, buf.readI64());
            MC_TRY_ASSIGN(v.data, buf.readI32());
            MC_TRY_ASSIGN(v.globalEvent, buf.readBool());
            return v;
        });
}

// ============================================================================
// 粒子（S→C）
// ============================================================================

namespace play_ext_detail {

/**
 * @brief 写 ParticleOptions（1.21.11，对齐 ParticleTypes.STREAM_CODEC）
 *
 * VarInt(registryId=toProtocolId(type)) + 各类型专属 payload。
 */
inline void writeParticleOptions(B& buf, const ir::play::ParticleOptions& v)
{
    using namespace mc::particle;
    buf.writeVarInt(toProtocolId(v.type));

    if (requiresBlockState(v.type)) {
        // BlockParticleOption：VarInt(blockStateId)
        buf.writeVarInt(static_cast<i32>(v.blockStateId));
    } else if (requiresItemData(v.type)) {
        // ItemParticleOption：完整 ItemStack wire
        play_detail::writeItemStack(buf, v.item);
    } else if (v.type == ParticleTypeId::Dust || v.type == ParticleTypeId::Redstone) {
        // DustParticleOptions：INT color(ARGB) + FLOAT scale
        buf.writeI32(static_cast<i32>(v.color));
        buf.writeF32(v.scale);
    } else if (v.type == ParticleTypeId::DustColorTransition) {
        // DustColorTransitionOptions：INT fromColor + INT toColor + FLOAT scale
        buf.writeI32(static_cast<i32>(v.fromColor));
        buf.writeI32(static_cast<i32>(v.toColor));
        buf.writeF32(v.scale);
    } else if (v.type == ParticleTypeId::EntityEffect || v.type == ParticleTypeId::Flash ||
        v.type == ParticleTypeId::TintedLeaves) {
        // ColorParticleOption：INT color（ARGB 大端）
        buf.writeI32(static_cast<i32>(v.color));
    } else if (v.type == ParticleTypeId::Vibration) {
        // VibrationParticleOption：PositionSource + VAR_INT arrivalInTicks
        buf.writeVarInt(static_cast<i32>(v.vibrationSourceKind));
        if (v.vibrationSourceKind == 0) {
            buf.writeI64(v.vibrationBlockPosPacked);
        } else {
            buf.writeVarInt(v.vibrationEntityId);
            buf.writeF32(v.vibrationYOffset);
        }
        buf.writeVarInt(v.arrivalInTicks);
    } else if (v.type == ParticleTypeId::Trail) {
        // TrailParticleOption：Vec3(3×F64) + INT color + VAR_INT duration
        buf.writeF64(v.trailTargetX);
        buf.writeF64(v.trailTargetY);
        buf.writeF64(v.trailTargetZ);
        buf.writeI32(static_cast<i32>(v.color));
        buf.writeVarInt(v.trailDuration);
    }
    // SimpleParticleType 及其余无 options 类型：无额外字节
}

/**
 * @brief 读 ParticleOptions
 */
[[nodiscard]] inline Result<ir::play::ParticleOptions> readParticleOptions(B& buf)
{
    using namespace mc::particle;
    ir::play::ParticleOptions v{};
    i32 protoId = 0;
    MC_TRY_ASSIGN(protoId, buf.readVarInt());
    v.type = fromProtocolId(protoId);

    if (requiresBlockState(v.type)) {
        i32 bsid = 0;
        MC_TRY_ASSIGN(bsid, buf.readVarInt());
        v.blockStateId = static_cast<u32>(bsid);
    } else if (requiresItemData(v.type)) {
        MC_TRY_ASSIGN(v.item, play_detail::readItemStack(buf));
    } else if (v.type == ParticleTypeId::Dust || v.type == ParticleTypeId::Redstone) {
        i32 c = 0;
        MC_TRY_ASSIGN(c, buf.readI32());
        v.color = static_cast<u32>(c);
        MC_TRY_ASSIGN(v.scale, buf.readF32());
    } else if (v.type == ParticleTypeId::DustColorTransition) {
        i32 fc = 0;
        i32 tc = 0;
        MC_TRY_ASSIGN(fc, buf.readI32());
        MC_TRY_ASSIGN(tc, buf.readI32());
        v.fromColor = static_cast<u32>(fc);
        v.toColor = static_cast<u32>(tc);
        MC_TRY_ASSIGN(v.scale, buf.readF32());
    } else if (v.type == ParticleTypeId::EntityEffect || v.type == ParticleTypeId::Flash ||
        v.type == ParticleTypeId::TintedLeaves) {
        i32 c = 0;
        MC_TRY_ASSIGN(c, buf.readI32());
        v.color = static_cast<u32>(c);
    } else if (v.type == ParticleTypeId::Vibration) {
        i32 kind = 0;
        MC_TRY_ASSIGN(kind, buf.readVarInt());
        v.vibrationSourceKind = static_cast<u8>(kind);
        if (v.vibrationSourceKind == 0) {
            MC_TRY_ASSIGN(v.vibrationBlockPosPacked, buf.readI64());
        } else {
            MC_TRY_ASSIGN(v.vibrationEntityId, buf.readVarInt());
            MC_TRY_ASSIGN(v.vibrationYOffset, buf.readF32());
        }
        MC_TRY_ASSIGN(v.arrivalInTicks, buf.readVarInt());
    } else if (v.type == ParticleTypeId::Trail) {
        MC_TRY_ASSIGN(v.trailTargetX, buf.readF64());
        MC_TRY_ASSIGN(v.trailTargetY, buf.readF64());
        MC_TRY_ASSIGN(v.trailTargetZ, buf.readF64());
        i32 c = 0;
        MC_TRY_ASSIGN(c, buf.readI32());
        v.color = static_cast<u32>(c);
        MC_TRY_ASSIGN(v.trailDuration, buf.readVarInt());
    }
    return v;
}

} // namespace play_ext_detail

/// LevelParticles（S→C，id=46）
[[nodiscard]] inline auto levelParticlesCodec()
{
    return makeCodec<ir::play::LevelParticles>(
        [](B& buf, const ir::play::LevelParticles& v) {
            buf.writeBool(v.overrideLimiter);
            buf.writeBool(v.alwaysShow);
            buf.writeF64(v.x);
            buf.writeF64(v.y);
            buf.writeF64(v.z);
            buf.writeF32(v.xDist);
            buf.writeF32(v.yDist);
            buf.writeF32(v.zDist);
            buf.writeF32(v.maxSpeed);
            buf.writeI32(v.count);
            play_ext_detail::writeParticleOptions(buf, v.particle);
        },
        [](B& buf) -> Result<ir::play::LevelParticles> {
            ir::play::LevelParticles v{};
            MC_TRY_ASSIGN(v.overrideLimiter, buf.readBool());
            MC_TRY_ASSIGN(v.alwaysShow, buf.readBool());
            MC_TRY_ASSIGN(v.x, buf.readF64());
            MC_TRY_ASSIGN(v.y, buf.readF64());
            MC_TRY_ASSIGN(v.z, buf.readF64());
            MC_TRY_ASSIGN(v.xDist, buf.readF32());
            MC_TRY_ASSIGN(v.yDist, buf.readF32());
            MC_TRY_ASSIGN(v.zDist, buf.readF32());
            MC_TRY_ASSIGN(v.maxSpeed, buf.readF32());
            MC_TRY_ASSIGN(v.count, buf.readI32());
            MC_TRY_ASSIGN(v.particle, play_ext_detail::readParticleOptions(buf));
            return v;
        });
}

// ============================================================================
// Boss 条（S→C，id=9，单包 + operation 分发）
// ============================================================================

/// BossEvent（S→C，id=9）
[[nodiscard]] inline auto bossEventCodec()
{
    return makeCodec<ir::play::BossEvent>(
        [](B& buf, const ir::play::BossEvent& v) {
            buf.writeBytes(v.uuid.data(), v.uuid.size());
            buf.writeVarInt(v.operation);
            switch (v.operation) {
                case 0: // ADD
                    writeComponentNbt(buf, v.name);
                    buf.writeF32(v.progress);
                    buf.writeVarInt(v.color);
                    buf.writeVarInt(v.overlay);
                    buf.writeU8(v.flags);
                    break;
                case 1: // REMOVE
                    break;
                case 2: // UPDATE_PROGRESS
                    buf.writeF32(v.progress);
                    break;
                case 3: // UPDATE_NAME
                    writeComponentNbt(buf, v.name);
                    break;
                case 4: // UPDATE_STYLE
                    buf.writeVarInt(v.color);
                    buf.writeVarInt(v.overlay);
                    break;
                case 5: // UPDATE_PROPERTIES
                    buf.writeU8(v.flags);
                    break;
                default:
                    break;
            }
        },
        [](B& buf) -> Result<ir::play::BossEvent> {
            ir::play::BossEvent v{};
            MC_TRY(buf.readBytes(v.uuid.data(), v.uuid.size()));
            MC_TRY_ASSIGN(v.operation, buf.readVarInt());
            switch (v.operation) {
                case 0:
                    MC_TRY_ASSIGN(v.name, readComponentNbt(buf));
                    MC_TRY_ASSIGN(v.progress, buf.readF32());
                    MC_TRY_ASSIGN(v.color, buf.readVarInt());
                    MC_TRY_ASSIGN(v.overlay, buf.readVarInt());
                    MC_TRY_ASSIGN(v.flags, buf.readU8());
                    break;
                case 1:
                    break;
                case 2:
                    MC_TRY_ASSIGN(v.progress, buf.readF32());
                    break;
                case 3:
                    MC_TRY_ASSIGN(v.name, readComponentNbt(buf));
                    break;
                case 4:
                    MC_TRY_ASSIGN(v.color, buf.readVarInt());
                    MC_TRY_ASSIGN(v.overlay, buf.readVarInt());
                    break;
                case 5:
                    MC_TRY_ASSIGN(v.flags, buf.readU8());
                    break;
                default:
                    break;
            }
            return v;
        });
}

// ============================================================================
// 进度（Advancements）
// ============================================================================

/// SelectAdvancementTab（S→C，id=83）
[[nodiscard]] inline auto selectAdvancementTabCodec()
{
    return makeCodec<ir::play::SelectAdvancementTab>(
        [](B& buf, const ir::play::SelectAdvancementTab& v) {
            buf.writeBool(v.present);
            if (v.present) {
                buf.writeString(v.tab);
            }
        },
        [](B& buf) -> Result<ir::play::SelectAdvancementTab> {
            ir::play::SelectAdvancementTab v{};
            MC_TRY_ASSIGN(v.present, buf.readBool());
            if (v.present) {
                MC_TRY_ASSIGN(v.tab, buf.readString());
            }
            return v;
        });
}

/// SeenAdvancements（C→S，id=49）
[[nodiscard]] inline auto seenAdvancementsCodec()
{
    return makeCodec<ir::play::SeenAdvancements>(
        [](B& buf, const ir::play::SeenAdvancements& v) {
            buf.writeVarInt(v.action);
            if (v.action == 0) { // OPENED_TAB
                buf.writeString(v.tab);
            }
        },
        [](B& buf) -> Result<ir::play::SeenAdvancements> {
            ir::play::SeenAdvancements v{};
            MC_TRY_ASSIGN(v.action, buf.readVarInt());
            if (v.action == 0) {
                MC_TRY_ASSIGN(v.tab, buf.readString());
            }
            return v;
        });
}

// ============================================================================
// 记分板（S→C）
// ============================================================================

namespace play_ext_detail {

/**
 * @brief 写 Optional<NumberFormat> wire（对齐 vanilla NumberFormat.STREAM_CODEC）
 *
 * vanilla NumberFormat 三变体：blank(0)/styled(1)/fixed(2)。
 * - blank：仅 VarInt(0)
 * - styled：VarInt(1) + Style（CompoundTag，NBT 自定界）
 * - fixed：VarInt(2) + Component（NBT 自定界）
 *
 * 项目 IR 以 `std::vector<u8>` 承载 NumberFormat 的原始 wire 字节（空 vector 表示 absent）。
 * 本函数写 Bool(present)：absent→写 0；present→写 1 + 透传 vector 内的 wire 字节（已含
 * VarInt(typeId) + 变体 payload，由业务侧或未来 NumberFormat 工具产出）。
 */
inline void writeNumberFormat(B& buf, const std::vector<u8>& nfmt)
{
    if (nfmt.empty()) {
        buf.writeBool(false);
    } else {
        buf.writeBool(true);
        buf.writeBytes(nfmt.data(), nfmt.size());
    }
}

/**
 * @brief 读 Optional<NumberFormat> wire（writeNumberFormat 的对称）
 *
 * absent→空 vector；present→读取剩余 NumberFormat wire 字节。NumberFormat 变体由 VarInt(typeId)
 * 自描述：blank(0) 无后续字节；styled(1) 后跟 CompoundTag（skipCompound 定界）；fixed(2) 后跟
 * Component NBT（readComponentNbt 定界）。本函数按 typeId 定界消费，返回原始 wire 字节
 * （含 typeId + 变体 payload）。
 */
[[nodiscard]] inline Result<std::vector<u8>> readNumberFormat(B& buf)
{
    bool present = false;
    MC_TRY_ASSIGN(present, buf.readBool());
    if (!present) {
        return std::vector<u8>{};
    }
    const usize start = buf.readPosition();
    i32 typeId = 0;
    MC_TRY_ASSIGN(typeId, buf.readVarInt());
    if (typeId == 1) {
        // styled：后跟 Style CompoundTag。Style 经 Codec.encode(NbtOps)→CompoundTag，
        // readComponentNbt 已支持 CompoundTag 定界（0x0A + entries + 0x00）。
        MC_TRY(readComponentNbt(buf));
    } else if (typeId == 2) {
        // fixed：后跟 Component NBT（StringTag 或 CompoundTag）。
        MC_TRY(readComponentNbt(buf));
    } else if (typeId != 0) {
        return Error(ErrorCode::InvalidData,
            "NumberFormat: expected typeId 0/1/2, got " + std::to_string(typeId),
            "readNumberFormat");
    }
    // typeId==0 (blank) 无后续字节
    const usize end = buf.readPosition();
    const auto& all = buf.bytes();
    return std::vector<u8>(all.begin() + start, all.begin() + end);
}

/**
 * @brief 写 Optional<Component> wire（对齐 vanilla OptionalCodec + ComponentSerialization）
 *
 * present→Bool(true) + Component NBT（自定界，无外层长度）；absent→Bool(false)。
 * 项目 IR 以 `std::vector<u8>` 承载 Component NBT wire 字节，空 vector 表示 absent。
 */
inline void writeOptionalComponentNbt(B& buf, const std::vector<u8>& nbt)
{
    if (nbt.empty()) {
        buf.writeBool(false);
    } else {
        buf.writeBool(true);
        buf.writeBytes(nbt.data(), nbt.size());
    }
}

/**
 * @brief 读 Optional<Component> wire（writeOptionalComponentNbt 的对称）
 */
[[nodiscard]] inline Result<std::vector<u8>> readOptionalComponentNbt(B& buf)
{
    bool present = false;
    MC_TRY_ASSIGN(present, buf.readBool());
    if (!present) {
        return std::vector<u8>{};
    }
    return readComponentNbt(buf);
}

} // namespace play_ext_detail

/// SetObjective（S→C，id=104，单包 + method 分发）
[[nodiscard]] inline auto setObjectiveCodec()
{
    return makeCodec<ir::play::SetObjective>(
        [](B& buf, const ir::play::SetObjective& v) {
            buf.writeString(v.objectiveName);
            buf.writeU8(v.method);
            if (v.method == 0 || v.method == 2) {
                writeComponentNbt(buf, v.displayName);
                buf.writeVarInt(v.renderType);
                play_ext_detail::writeNumberFormat(buf, v.numberFormat);
            }
        },
        [](B& buf) -> Result<ir::play::SetObjective> {
            ir::play::SetObjective v{};
            MC_TRY_ASSIGN(v.objectiveName, buf.readString());
            MC_TRY_ASSIGN(v.method, buf.readU8());
            if (v.method == 0 || v.method == 2) {
                MC_TRY_ASSIGN(v.displayName, readComponentNbt(buf));
                MC_TRY_ASSIGN(v.renderType, buf.readVarInt());
                MC_TRY_ASSIGN(v.numberFormat, play_ext_detail::readNumberFormat(buf));
            }
            return v;
        });
}

/// SetScore（S→C，id=108）
[[nodiscard]] inline auto setScoreCodec()
{
    return makeCodec<ir::play::SetScore>(
        [](B& buf, const ir::play::SetScore& v) {
            buf.writeString(v.owner);
            buf.writeString(v.objectiveName);
            buf.writeVarInt(v.score);
            play_ext_detail::writeOptionalComponentNbt(buf, v.display);
            play_ext_detail::writeNumberFormat(buf, v.numberFormat);
        },
        [](B& buf) -> Result<ir::play::SetScore> {
            ir::play::SetScore v{};
            MC_TRY_ASSIGN(v.owner, buf.readString());
            MC_TRY_ASSIGN(v.objectiveName, buf.readString());
            MC_TRY_ASSIGN(v.score, buf.readVarInt());
            MC_TRY_ASSIGN(v.display, play_ext_detail::readOptionalComponentNbt(buf));
            MC_TRY_ASSIGN(v.numberFormat, play_ext_detail::readNumberFormat(buf));
            return v;
        });
}

/// ResetScore（S→C，id=77）
[[nodiscard]] inline auto resetScoreCodec()
{
    return makeCodec<ir::play::ResetScore>(
        [](B& buf, const ir::play::ResetScore& v) {
            buf.writeString(v.owner);
            buf.writeBool(v.objectiveName.has_value());
            if (v.objectiveName.has_value()) {
                buf.writeString(*v.objectiveName);
            }
        },
        [](B& buf) -> Result<ir::play::ResetScore> {
            ir::play::ResetScore v{};
            MC_TRY_ASSIGN(v.owner, buf.readString());
            bool present = false;
            MC_TRY_ASSIGN(present, buf.readBool());
            if (present) {
                std::string name;
                MC_TRY_ASSIGN(name, buf.readString());
                v.objectiveName = std::move(name);
            }
            return v;
        });
}

/// SetDisplayObjective（S→C，id=96）
[[nodiscard]] inline auto setDisplayObjectiveCodec()
{
    return makeCodec<ir::play::SetDisplayObjective>(
        [](B& buf, const ir::play::SetDisplayObjective& v) {
            buf.writeVarInt(v.slot);
            buf.writeString(v.objectiveName);
        },
        [](B& buf) -> Result<ir::play::SetDisplayObjective> {
            ir::play::SetDisplayObjective v{};
            MC_TRY_ASSIGN(v.slot, buf.readVarInt());
            MC_TRY_ASSIGN(v.objectiveName, buf.readString());
            return v;
        });
}

/// SetPlayerTeam（S→C，id=107，单包 + method 分发）
[[nodiscard]] inline auto setPlayerTeamCodec()
{
    return makeCodec<ir::play::SetPlayerTeam>(
        [](B& buf, const ir::play::SetPlayerTeam& v) {
            buf.writeString(v.name);
            buf.writeU8(v.method);
            // TeamParameters（method 0/2）：扁平 7 字段，对齐 vanilla ClientboundSetPlayerTeamPacket.Parameters
            if (v.method == 0 || v.method == 2) {
                writeComponentNbt(buf, v.displayName); // Component NBT（自定界）
                buf.writeU8(v.options);                // friendlyFlags 打包 Byte
                buf.writeVarInt(v.visibility);         // Team.Visibility id
                buf.writeVarInt(v.collision);          // Team.CollisionRule id
                buf.writeVarInt(v.color);              // ChatFormatting ordinal
                writeComponentNbt(buf, v.prefix);      // Component NBT
                writeComponentNbt(buf, v.suffix);      // Component NBT
            }
            if (v.method == 0 || v.method == 3 || v.method == 4) {
                buf.writeVarInt(static_cast<i32>(v.players.size()));
                for (const auto& p : v.players) {
                    buf.writeString(p);
                }
            }
        },
        [](B& buf) -> Result<ir::play::SetPlayerTeam> {
            ir::play::SetPlayerTeam v{};
            MC_TRY_ASSIGN(v.name, buf.readString());
            MC_TRY_ASSIGN(v.method, buf.readU8());
            if (v.method == 0 || v.method == 2) {
                MC_TRY_ASSIGN(v.displayName, readComponentNbt(buf));
                MC_TRY_ASSIGN(v.options, buf.readU8());
                MC_TRY_ASSIGN(v.visibility, buf.readVarInt());
                MC_TRY_ASSIGN(v.collision, buf.readVarInt());
                MC_TRY_ASSIGN(v.color, buf.readVarInt());
                MC_TRY_ASSIGN(v.prefix, readComponentNbt(buf));
                MC_TRY_ASSIGN(v.suffix, readComponentNbt(buf));
            }
            if (v.method == 0 || v.method == 3 || v.method == 4) {
                i32 count = 0;
                MC_TRY_ASSIGN(count, buf.readVarInt());
                if (count < 0) {
                    return Error(ErrorCode::InvalidData, "players count is negative", "setPlayerTeamCodec");
                }
                for (i32 i = 0; i < count; ++i) {
                    std::string p;
                    MC_TRY_ASSIGN(p, buf.readString());
                    v.players.push_back(std::move(p));
                }
            }
            return v;
        });
}

// ============================================================================
// 标题（S→C，1.21.11 拆 5 包）
// ============================================================================

/// SetTitleText（S→C，id=112）
[[nodiscard]] inline auto setTitleTextCodec()
{
    return makeCodec<ir::play::SetTitleText>(
        [](B& buf, const ir::play::SetTitleText& v) { writeComponentNbt(buf, v.text); },
        [](B& buf) -> Result<ir::play::SetTitleText> {
            ir::play::SetTitleText v{};
            MC_TRY_ASSIGN(v.text, readComponentNbt(buf));
            return v;
        });
}

/// SetSubtitleText（S→C，id=110）
[[nodiscard]] inline auto setSubtitleTextCodec()
{
    return makeCodec<ir::play::SetSubtitleText>(
        [](B& buf, const ir::play::SetSubtitleText& v) { writeComponentNbt(buf, v.text); },
        [](B& buf) -> Result<ir::play::SetSubtitleText> {
            ir::play::SetSubtitleText v{};
            MC_TRY_ASSIGN(v.text, readComponentNbt(buf));
            return v;
        });
}

/// SetActionBarText（S→C，id=85）
[[nodiscard]] inline auto setActionBarTextCodec()
{
    return makeCodec<ir::play::SetActionBarText>(
        [](B& buf, const ir::play::SetActionBarText& v) { writeComponentNbt(buf, v.text); },
        [](B& buf) -> Result<ir::play::SetActionBarText> {
            ir::play::SetActionBarText v{};
            MC_TRY_ASSIGN(v.text, readComponentNbt(buf));
            return v;
        });
}

/// SystemChat（S→C，id=119）
/// 对齐 vanilla ClientboundSystemChatPacket：content = Component NBT（自定界，无外层长度前缀）
/// + isActionBar(bool)。overlay=true→动作栏，false→聊天窗口。
[[nodiscard]] inline auto systemChatCodec()
{
    return makeCodec<ir::play::SystemChat>(
        [](B& buf, const ir::play::SystemChat& v) {
            writeComponentNbt(buf, v.content);
            buf.writeBool(v.overlay);
        },
        [](B& buf) -> Result<ir::play::SystemChat> {
            ir::play::SystemChat v{};
            MC_TRY_ASSIGN(v.content, readComponentNbt(buf));
            MC_TRY_ASSIGN(v.overlay, buf.readBool());
            return v;
        });
}

// ============================================================================
// 玩家战斗（S→C，1.21.11 共 3 包）
// ============================================================================

/// PlayerCombatEnter（S→C，id=65）
/// 对齐 vanilla ClientboundPlayerCombatEnterPacket：无负载（INSTANCE 单例）。
[[nodiscard]] inline auto playerCombatEnterCodec()
{
    return makeCodec<ir::play::PlayerCombatEnter>(
        []([[maybe_unused]] B& buf, [[maybe_unused]] const ir::play::PlayerCombatEnter& v) {
            // 无负载：vanilla STREAM_CODEC = StreamCodec.unit(INSTANCE)
        },
        []([[maybe_unused]] B& buf) -> Result<ir::play::PlayerCombatEnter> { return ir::play::PlayerCombatEnter{}; });
}

/// PlayerCombatEnd（S→C，id=64）
/// 对齐 vanilla ClientboundPlayerCombatEndPacket：VarInt(duration)。
[[nodiscard]] inline auto playerCombatEndCodec()
{
    return makeCodec<ir::play::PlayerCombatEnd>(
        [](B& buf, const ir::play::PlayerCombatEnd& v) { buf.writeVarInt(v.duration); },
        [](B& buf) -> Result<ir::play::PlayerCombatEnd> {
            ir::play::PlayerCombatEnd v{};
            MC_TRY_ASSIGN(v.duration, buf.readVarInt());
            return v;
        });
}

/// PlayerCombatKill（S→C，id=66）
/// 对齐 vanilla ClientboundPlayerCombatKillPacket：VarInt(playerId) + Component(message)。
/// message 为 Component NBT wire 字节（自定界，对齐 ComponentSerialization.TRUSTED_STREAM_CODEC）。
[[nodiscard]] inline auto playerCombatKillCodec()
{
    return makeCodec<ir::play::PlayerCombatKill>(
        [](B& buf, const ir::play::PlayerCombatKill& v) {
            buf.writeVarInt(v.playerId);
            writeComponentNbt(buf, v.message);
        },
        [](B& buf) -> Result<ir::play::PlayerCombatKill> {
            ir::play::PlayerCombatKill v{};
            MC_TRY_ASSIGN(v.playerId, buf.readVarInt());
            MC_TRY_ASSIGN(v.message, readComponentNbt(buf));
            return v;
        });
}

/// SetTitlesAnimation（S→C，id=113）
[[nodiscard]] inline auto setTitlesAnimationCodec()
{
    return makeCodec<ir::play::SetTitlesAnimation>(
        [](B& buf, const ir::play::SetTitlesAnimation& v) {
            buf.writeI32(v.fadeIn);
            buf.writeI32(v.stay);
            buf.writeI32(v.fadeOut);
        },
        [](B& buf) -> Result<ir::play::SetTitlesAnimation> {
            ir::play::SetTitlesAnimation v{};
            MC_TRY_ASSIGN(v.fadeIn, buf.readI32());
            MC_TRY_ASSIGN(v.stay, buf.readI32());
            MC_TRY_ASSIGN(v.fadeOut, buf.readI32());
            return v;
        });
}

/// ClearTitles（S→C，id=14）
[[nodiscard]] inline auto clearTitlesCodec()
{
    return makeCodec<ir::play::ClearTitles>([](B& buf, const ir::play::ClearTitles& v) { buf.writeBool(v.resetTimes); },
        [](B& buf) -> Result<ir::play::ClearTitles> {
            ir::play::ClearTitles v{};
            MC_TRY_ASSIGN(v.resetTimes, buf.readBool());
            return v;
        });
}

// ============================================================================
// 世界边界（S→C，1.21.11 拆 6 包）
// ============================================================================

/// InitializeBorder（S→C，id=42）
[[nodiscard]] inline auto initializeBorderCodec()
{
    return makeCodec<ir::play::InitializeBorder>(
        [](B& buf, const ir::play::InitializeBorder& v) {
            buf.writeF64(v.newCenterX);
            buf.writeF64(v.newCenterZ);
            buf.writeF64(v.oldSize);
            buf.writeF64(v.newSize);
            buf.writeVarLong(v.lerpTime);
            buf.writeVarInt(v.newAbsoluteMaxSize);
            buf.writeVarInt(v.warningBlocks);
            buf.writeVarInt(v.warningTime);
        },
        [](B& buf) -> Result<ir::play::InitializeBorder> {
            ir::play::InitializeBorder v{};
            MC_TRY_ASSIGN(v.newCenterX, buf.readF64());
            MC_TRY_ASSIGN(v.newCenterZ, buf.readF64());
            MC_TRY_ASSIGN(v.oldSize, buf.readF64());
            MC_TRY_ASSIGN(v.newSize, buf.readF64());
            MC_TRY_ASSIGN(v.lerpTime, buf.readVarLong());
            MC_TRY_ASSIGN(v.newAbsoluteMaxSize, buf.readVarInt());
            MC_TRY_ASSIGN(v.warningBlocks, buf.readVarInt());
            MC_TRY_ASSIGN(v.warningTime, buf.readVarInt());
            return v;
        });
}

/// SetBorderCenter（S→C，id=86）
[[nodiscard]] inline auto setBorderCenterCodec()
{
    return makeCodec<ir::play::SetBorderCenter>(
        [](B& buf, const ir::play::SetBorderCenter& v) {
            buf.writeF64(v.newCenterX);
            buf.writeF64(v.newCenterZ);
        },
        [](B& buf) -> Result<ir::play::SetBorderCenter> {
            ir::play::SetBorderCenter v{};
            MC_TRY_ASSIGN(v.newCenterX, buf.readF64());
            MC_TRY_ASSIGN(v.newCenterZ, buf.readF64());
            return v;
        });
}

/// SetBorderLerpSize（S→C，id=87）
[[nodiscard]] inline auto setBorderLerpSizeCodec()
{
    return makeCodec<ir::play::SetBorderLerpSize>(
        [](B& buf, const ir::play::SetBorderLerpSize& v) {
            buf.writeF64(v.oldSize);
            buf.writeF64(v.newSize);
            buf.writeVarLong(v.lerpTime);
        },
        [](B& buf) -> Result<ir::play::SetBorderLerpSize> {
            ir::play::SetBorderLerpSize v{};
            MC_TRY_ASSIGN(v.oldSize, buf.readF64());
            MC_TRY_ASSIGN(v.newSize, buf.readF64());
            MC_TRY_ASSIGN(v.lerpTime, buf.readVarLong());
            return v;
        });
}

/// SetBorderSize（S→C，id=88）
[[nodiscard]] inline auto setBorderSizeCodec()
{
    return makeCodec<ir::play::SetBorderSize>([](B& buf, const ir::play::SetBorderSize& v) { buf.writeF64(v.size); },
        [](B& buf) -> Result<ir::play::SetBorderSize> {
            ir::play::SetBorderSize v{};
            MC_TRY_ASSIGN(v.size, buf.readF64());
            return v;
        });
}

/// SetBorderWarningDelay（S→C，id=89）
[[nodiscard]] inline auto setBorderWarningDelayCodec()
{
    return makeCodec<ir::play::SetBorderWarningDelay>(
        [](B& buf, const ir::play::SetBorderWarningDelay& v) { buf.writeVarInt(v.warningDelay); },
        [](B& buf) -> Result<ir::play::SetBorderWarningDelay> {
            ir::play::SetBorderWarningDelay v{};
            MC_TRY_ASSIGN(v.warningDelay, buf.readVarInt());
            return v;
        });
}

/// SetBorderWarningDistance（S→C，id=90）
[[nodiscard]] inline auto setBorderWarningDistanceCodec()
{
    return makeCodec<ir::play::SetBorderWarningDistance>(
        [](B& buf, const ir::play::SetBorderWarningDistance& v) { buf.writeVarInt(v.warningBlocks); },
        [](B& buf) -> Result<ir::play::SetBorderWarningDistance> {
            ir::play::SetBorderWarningDistance v{};
            MC_TRY_ASSIGN(v.warningBlocks, buf.readVarInt());
            return v;
        });
}

// ============================================================================
// 地图（S→C，结构化，对齐 1.21.11 ClientboundMapItemDataPacket）
// ============================================================================

namespace play_ext_detail {

/// 写 MapDecoration wire：VarInt(registryId) + byte x + byte y + byte rot + Optional<Component>(opaque)
inline void writeMapDecoration(B& buf, const ir::play::MapDecorationWire& d)
{
    buf.writeVarInt(static_cast<i32>(d.typeRegistryId));
    buf.writeI8(d.x);
    buf.writeI8(d.y);
    buf.writeU8(d.rotation);
    if (d.name.has_value()) {
        buf.writeBool(true);
        writeOpaque(buf, *d.name);
    } else {
        buf.writeBool(false);
    }
}

/// 读 MapDecoration wire
[[nodiscard]] inline Result<ir::play::MapDecorationWire> readMapDecoration(B& buf)
{
    ir::play::MapDecorationWire d{};
    MC_TRY_ASSIGN(d.typeRegistryId, buf.readVarInt());
    d.typeRegistryId = static_cast<u32>(std::max(0, static_cast<i32>(d.typeRegistryId)));
    MC_TRY_ASSIGN(d.x, buf.readI8());
    MC_TRY_ASSIGN(d.y, buf.readI8());
    MC_TRY_ASSIGN(d.rotation, buf.readU8());
    d.rotation &= 0x0F;
    bool hasName = false;
    MC_TRY_ASSIGN(hasName, buf.readBool());
    if (hasName) {
        MC_TRY_ASSIGN(d.name, readOpaque(buf, "mapDecorationName"));
    }
    return d;
}

/// 写 Optional<MapPatch> wire：present 则 width,height,startX,startY,ByteArray；absent 则 writeByte(0)
inline void writeMapPatch(B& buf, const std::optional<ir::play::MapPatchWire>& patch)
{
    if (patch.has_value() && patch->width > 0) {
        buf.writeU8(patch->width);
        buf.writeU8(patch->height);
        buf.writeU8(patch->startX);
        buf.writeU8(patch->startY);
        writeOpaque(buf, patch->colors);
    } else {
        buf.writeU8(0); // width=0 哨兵表 absent
    }
}

/// 读 Optional<MapPatch> wire：先读 width，>0 才读 height/startX/startY/ByteArray
[[nodiscard]] inline Result<std::optional<ir::play::MapPatchWire>> readMapPatch(B& buf)
{
    u8 width = 0;
    MC_TRY_ASSIGN(width, buf.readU8());
    if (width == 0) {
        return std::optional<ir::play::MapPatchWire>{};
    }
    ir::play::MapPatchWire patch{};
    patch.width = width;
    MC_TRY_ASSIGN(patch.height, buf.readU8());
    MC_TRY_ASSIGN(patch.startX, buf.readU8());
    MC_TRY_ASSIGN(patch.startY, buf.readU8());
    MC_TRY_ASSIGN(patch.colors, readOpaque(buf, "mapPatchColors"));
    return std::optional<ir::play::MapPatchWire>{std::move(patch)};
}

} // namespace play_ext_detail

/// MapItemData（S→C，id=49）
[[nodiscard]] inline auto mapItemDataCodec()
{
    return makeCodec<ir::play::MapItemData>(
        [](B& buf, const ir::play::MapItemData& v) {
            buf.writeVarInt(v.mapId);
            buf.writeU8(v.scale);
            buf.writeBool(v.locked);
            // Optional<List<MapDecoration>>：bool present + VarInt(count) + 项
            if (v.decorations.has_value()) {
                buf.writeBool(true);
                buf.writeVarInt(static_cast<i32>(v.decorations->size()));
                for (const auto& d : *v.decorations) {
                    play_ext_detail::writeMapDecoration(buf, d);
                }
            } else {
                buf.writeBool(false);
            }
            // Optional<MapPatch>：width==0 哨兵
            play_ext_detail::writeMapPatch(buf, v.colorPatch);
        },
        [](B& buf) -> Result<ir::play::MapItemData> {
            ir::play::MapItemData v{};
            MC_TRY_ASSIGN(v.mapId, buf.readVarInt());
            MC_TRY_ASSIGN(v.scale, buf.readU8());
            MC_TRY_ASSIGN(v.locked, buf.readBool());
            bool hasDecorations = false;
            MC_TRY_ASSIGN(hasDecorations, buf.readBool());
            if (hasDecorations) {
                i32 count = 0;
                MC_TRY_ASSIGN(count, buf.readVarInt());
                if (count < 0) {
                    return Error(ErrorCode::InvalidData, "map decorations count is negative", "mapItemDataCodec");
                }
                std::vector<ir::play::MapDecorationWire> decos;
                decos.reserve(static_cast<usize>(count));
                for (i32 i = 0; i < count; ++i) {
                    ir::play::MapDecorationWire d{};
                    MC_TRY_ASSIGN(d, play_ext_detail::readMapDecoration(buf));
                    decos.push_back(std::move(d));
                }
                v.decorations = std::move(decos);
            }
            MC_TRY_ASSIGN(v.colorPatch, play_ext_detail::readMapPatch(buf));
            return v;
        });
}

// ============================================================================
// 告示牌
// ============================================================================

/// OpenSignEditor（S→C，id=58）
[[nodiscard]] inline auto openSignEditorCodec()
{
    return makeCodec<ir::play::OpenSignEditor>(
        [](B& buf, const ir::play::OpenSignEditor& v) {
            buf.writeI64(v.blockPosPacked);
            buf.writeBool(v.isFrontText);
        },
        [](B& buf) -> Result<ir::play::OpenSignEditor> {
            ir::play::OpenSignEditor v{};
            MC_TRY_ASSIGN(v.blockPosPacked, buf.readI64());
            MC_TRY_ASSIGN(v.isFrontText, buf.readBool());
            return v;
        });
}

/// SignUpdate（C→S，id=59）
[[nodiscard]] inline auto signUpdateCodec()
{
    return makeCodec<ir::play::SignUpdate>(
        [](B& buf, const ir::play::SignUpdate& v) {
            buf.writeI64(v.blockPosPacked);
            buf.writeBool(v.isFrontText);
            for (const auto& line : v.lines) {
                buf.writeString(line);
            }
        },
        [](B& buf) -> Result<ir::play::SignUpdate> {
            ir::play::SignUpdate v{};
            MC_TRY_ASSIGN(v.blockPosPacked, buf.readI64());
            MC_TRY_ASSIGN(v.isFrontText, buf.readBool());
            for (usize i = 0; i < v.lines.size(); ++i) {
                std::string line;
                MC_TRY_ASSIGN(line, buf.readString());
                v.lines[i] = std::move(line);
            }
            return v;
        });
}

// ============================================================================
// 简单单包（S→C）
// ============================================================================

/// SetCamera（S→C，id=91）
[[nodiscard]] inline auto setCameraCodec()
{
    return makeCodec<ir::play::SetCamera>([](B& buf, const ir::play::SetCamera& v) { buf.writeVarInt(v.cameraId); },
        [](B& buf) -> Result<ir::play::SetCamera> {
            ir::play::SetCamera v{};
            MC_TRY_ASSIGN(v.cameraId, buf.readVarInt());
            return v;
        });
}

/// SetEntityLink（S→C，id=98）
[[nodiscard]] inline auto setEntityLinkCodec()
{
    return makeCodec<ir::play::SetEntityLink>(
        [](B& buf, const ir::play::SetEntityLink& v) {
            buf.writeI32(v.sourceId);
            buf.writeI32(v.destId);
        },
        [](B& buf) -> Result<ir::play::SetEntityLink> {
            ir::play::SetEntityLink v{};
            MC_TRY_ASSIGN(v.sourceId, buf.readI32());
            MC_TRY_ASSIGN(v.destId, buf.readI32());
            return v;
        });
}

/// SetPassengers（S→C，id=105）
[[nodiscard]] inline auto setPassengersCodec()
{
    return makeCodec<ir::play::SetPassengers>(
        [](B& buf, const ir::play::SetPassengers& v) {
            buf.writeVarInt(v.vehicle);
            buf.writeVarInt(static_cast<i32>(v.passengers.size()));
            for (i32 p : v.passengers) {
                buf.writeVarInt(p);
            }
        },
        [](B& buf) -> Result<ir::play::SetPassengers> {
            ir::play::SetPassengers v{};
            MC_TRY_ASSIGN(v.vehicle, buf.readVarInt());
            i32 count = 0;
            MC_TRY_ASSIGN(count, buf.readVarInt());
            if (count < 0) {
                return Error(ErrorCode::InvalidData, "passengers count is negative", "setPassengersCodec");
            }
            for (i32 i = 0; i < count; ++i) {
                i32 p = 0;
                MC_TRY_ASSIGN(p, buf.readVarInt());
                v.passengers.push_back(p);
            }
            return v;
        });
}

/// EntityEvent（S→C，id=34）
[[nodiscard]] inline auto entityEventCodec()
{
    return makeCodec<ir::play::EntityEvent>(
        [](B& buf, const ir::play::EntityEvent& v) {
            buf.writeI32(v.entityId);
            buf.writeU8(v.eventId);
        },
        [](B& buf) -> Result<ir::play::EntityEvent> {
            ir::play::EntityEvent v{};
            MC_TRY_ASSIGN(v.entityId, buf.readI32());
            MC_TRY_ASSIGN(v.eventId, buf.readU8());
            return v;
        });
}

/// Animate（S→C，id=2）
[[nodiscard]] inline auto animateCodec()
{
    return makeCodec<ir::play::Animate>(
        [](B& buf, const ir::play::Animate& v) {
            buf.writeVarInt(v.id);
            buf.writeU8(v.action);
        },
        [](B& buf) -> Result<ir::play::Animate> {
            ir::play::Animate v{};
            MC_TRY_ASSIGN(v.id, buf.readVarInt());
            MC_TRY_ASSIGN(v.action, buf.readU8());
            return v;
        });
}

/// HurtAnimation（S→C，id=41）
[[nodiscard]] inline auto hurtAnimationCodec()
{
    return makeCodec<ir::play::HurtAnimation>(
        [](B& buf, const ir::play::HurtAnimation& v) {
            buf.writeVarInt(v.id);
            buf.writeF32(v.yaw);
        },
        [](B& buf) -> Result<ir::play::HurtAnimation> {
            ir::play::HurtAnimation v{};
            MC_TRY_ASSIGN(v.id, buf.readVarInt());
            MC_TRY_ASSIGN(v.yaw, buf.readF32());
            return v;
        });
}

/// TakeItemEntity（S→C，id=122）
[[nodiscard]] inline auto takeItemEntityCodec()
{
    return makeCodec<ir::play::TakeItemEntity>(
        [](B& buf, const ir::play::TakeItemEntity& v) {
            buf.writeVarInt(v.itemId);
            buf.writeVarInt(v.playerId);
            buf.writeVarInt(v.amount);
        },
        [](B& buf) -> Result<ir::play::TakeItemEntity> {
            ir::play::TakeItemEntity v{};
            MC_TRY_ASSIGN(v.itemId, buf.readVarInt());
            MC_TRY_ASSIGN(v.playerId, buf.readVarInt());
            MC_TRY_ASSIGN(v.amount, buf.readVarInt());
            return v;
        });
}

/// BlockDestruction（S→C，id=5）
[[nodiscard]] inline auto blockDestructionCodec()
{
    return makeCodec<ir::play::BlockDestruction>(
        [](B& buf, const ir::play::BlockDestruction& v) {
            buf.writeVarInt(v.id);
            buf.writeI64(v.blockPosPacked);
            buf.writeU8(v.progress);
        },
        [](B& buf) -> Result<ir::play::BlockDestruction> {
            ir::play::BlockDestruction v{};
            MC_TRY_ASSIGN(v.id, buf.readVarInt());
            MC_TRY_ASSIGN(v.blockPosPacked, buf.readI64());
            MC_TRY_ASSIGN(v.progress, buf.readU8());
            return v;
        });
}

/// BlockEvent（S→C，id=7）
// vanilla 第4字段 blockId 是 Block 注册表 id（ByteBufCodecs.registry(Registries.BLOCK)），
// 非 stateId 非 state globalId。IR 层 v.blockId 存项目内部 blockId（见
// PlayerBroadcaster::broadcastBlockEventInRange），出站边界经 JavaBlockIdMap 译为 Java
// Block 注册表 id，decode 对称反翻译（客户端 ClientPlayVisitor 不消费 blockId，反翻译仅为
// codec 自对称/往返测试）。
[[nodiscard]] inline auto blockEventCodec()
{
    return makeCodec<ir::play::BlockEvent>(
        [](B& buf, const ir::play::BlockEvent& v) {
            buf.writeI64(v.blockPosPacked);
            buf.writeU8(v.b0);
            buf.writeU8(v.b1);
            // IR 内部 blockId → Java Block 注册表 id（miss 兜底 0=air）。
            buf.writeVarInt(static_cast<i32>(
                mc::network::backend::java::JavaBlockIdMap::instance().toJavaRegistryId(static_cast<u32>(v.blockId))));
        },
        [](B& buf) -> Result<ir::play::BlockEvent> {
            ir::play::BlockEvent v{};
            MC_TRY_ASSIGN(v.blockPosPacked, buf.readI64());
            MC_TRY_ASSIGN(v.b0, buf.readU8());
            MC_TRY_ASSIGN(v.b1, buf.readU8());
            i32 rawBlockId = 0;
            MC_TRY_ASSIGN(rawBlockId, buf.readVarInt());
            // Java Block 注册表 id → IR 内部 blockId（miss 兜底 0=air）。
            v.blockId = static_cast<i32>(mc::network::backend::java::JavaBlockIdMap::instance().fromJavaRegistryId(
                static_cast<u32>(rawBlockId)));
            return v;
        });
}

/// BlockEntityData（S→C，id=6）
[[nodiscard]] inline auto blockEntityDataCodec()
{
    return makeCodec<ir::play::BlockEntityData>(
        [](B& buf, const ir::play::BlockEntityData& v) {
            buf.writeI64(v.blockPosPacked);
            buf.writeVarInt(v.blockEntityType);
            // 1.21.11 ClientboundBlockEntityDataPacket：CompoundTag 无长度前缀（NBT 自定界）。
            // 空 NBT 仍需写一个 TAG_End（空复合标签），与 Java 行为一致。
            const nbt::CompoundTag empty{};
            const nbt::CompoundTag& tag = v.tag ? *v.tag : empty;
            // encode 返回 void，无法向上传播 Result；有效 CompoundTag 的 NBT 序列化只会在
            // 输出流错误时失败（实际不会发生），失败即丢弃——与其它 encode lambda 的容错约定一致。
            (void)buffer::nbt_io::writeCompound(buf, tag);
        },
        [](B& buf) -> Result<ir::play::BlockEntityData> {
            ir::play::BlockEntityData v{};
            MC_TRY_ASSIGN(v.blockPosPacked, buf.readI64());
            MC_TRY_ASSIGN(v.blockEntityType, buf.readVarInt());
            // readCompound 返回 Result<unique_ptr<CompoundTag>> 特化，其 value() 按值返回右值，
            // MC_TRY_ASSIGN 内的 std::move 会触发 -Wpessimizing-move；手动取值。
            auto tagResult = buffer::nbt_io::readCompound(buf);
            if (tagResult.failed()) {
                return tagResult.error();
            }
            v.tag = tagResult.value(); // value() 按值返回右值 unique_ptr，无需 std::move
            return v;
        });
}

// ============================================================================
// 维度（S→C）
// ============================================================================

/// Respawn（S→C，id=80）
[[nodiscard]] inline auto respawnCodec()
{
    return makeCodec<ir::play::Respawn>(
        [](B& buf, const ir::play::Respawn& v) {
            play_detail::writeSpawnInfo(buf, v.spawnInfo);
            buf.writeU8(v.dataToKeep);
        },
        [](B& buf) -> Result<ir::play::Respawn> {
            ir::play::Respawn v{};
            MC_TRY_ASSIGN(v.spawnInfo, play_detail::readSpawnInfo(buf));
            MC_TRY_ASSIGN(v.dataToKeep, buf.readU8());
            return v;
        });
}

// ============================================================================
// 经验（S→C）
// ============================================================================

/// SetExperience（S→C，id=101）
[[nodiscard]] inline auto setExperienceCodec()
{
    return makeCodec<ir::play::SetExperience>(
        [](B& buf, const ir::play::SetExperience& v) {
            buf.writeF32(v.experienceProgress);
            buf.writeVarInt(v.experienceLevel);
            buf.writeVarInt(v.totalExperience);
        },
        [](B& buf) -> Result<ir::play::SetExperience> {
            ir::play::SetExperience v{};
            MC_TRY_ASSIGN(v.experienceProgress, buf.readF32());
            MC_TRY_ASSIGN(v.experienceLevel, buf.readVarInt());
            MC_TRY_ASSIGN(v.totalExperience, buf.readVarInt());
            return v;
        });
}

// ============================================================================
// 爆炸（S→C）
// ============================================================================

namespace play_ext_detail {

/**
 * @brief 写 Holder<SoundEvent>（1.21.11，对齐 ByteBufCodecs.holder）
 *
 * 内联模式（direct=true）：VarInt(0) + Identifier(string) + Optional<Float>(bool + f32)
 * 引用模式（direct=false）：VarInt(referenceId + 1)
 */
inline void writeSoundEventHolder(B& buf, const ir::play::SoundEventHolder& v)
{
    if (v.direct) {
        buf.writeVarInt(0);
        buf.writeString(v.identifier);
        buf.writeBool(v.hasFixedRange);
        if (v.hasFixedRange) {
            buf.writeF32(v.fixedRange);
        }
    } else {
        buf.writeVarInt(v.referenceId + 1);
    }
}

/**
 * @brief 读 Holder<SoundEvent>
 *
 * 引用模式（mode>0）：仅 holder id（mode-1），本项目无 sound registry id 表，
 * 保留 referenceId 供后续对齐真 Java 时查表；当前我方互通统一用内联模式。
 */
[[nodiscard]] inline Result<ir::play::SoundEventHolder> readSoundEventHolder(B& buf)
{
    ir::play::SoundEventHolder v{};
    i32 mode = 0;
    MC_TRY_ASSIGN(mode, buf.readVarInt());
    if (mode == 0) {
        v.direct = true;
        MC_TRY_ASSIGN(v.identifier, buf.readString());
        MC_TRY_ASSIGN(v.hasFixedRange, buf.readBool());
        if (v.hasFixedRange) {
            MC_TRY_ASSIGN(v.fixedRange, buf.readF32());
        }
    } else {
        v.direct = false;
        v.referenceId = mode - 1;
    }
    return v;
}

/**
 * @brief 写 WeightedList<ExplosionParticleInfo>
 *
 * 线格式：VarInt(count) + count×{ ExplosionParticleInfo + VarInt(weight) }。
 * 对齐 vanilla WeightedList.streamCodec = Weighted.streamCodec(VarInt weight).apply(ByteBufCodecs.list)：
 * list 的 count 走 ByteBufCodecs.writeCount→VarInt，Weighted.weight 走 ByteBufCodecs.VAR_INT。
 * 若误用定长 I32 写 count/weight，空列表会多 3 字节（I32=4 vs VarInt(0)=1），
 * 致真 Java 客户端解 ClientboundExplodePacket 时报 "N bytes extra" 断连。
 */
inline void writeExplosionParticleList(B& buf, const std::vector<ir::play::ExplosionParticleInfo>& v)
{
    buf.writeVarInt(static_cast<i32>(v.size()));
    for (const auto& e : v) {
        writeParticleOptions(buf, e.particle);
        buf.writeF32(e.scaling);
        buf.writeF32(e.speed);
        buf.writeVarInt(1); // weight（我方互通统一权重 1，VarInt 编码对齐 Weighted.weight）
    }
}

[[nodiscard]] inline Result<std::vector<ir::play::ExplosionParticleInfo>> readExplosionParticleList(B& buf)
{
    std::vector<ir::play::ExplosionParticleInfo> out;
    i32 count = 0;
    MC_TRY_ASSIGN(count, buf.readVarInt());
    if (count < 0) {
        return Error(ErrorCode::InvalidData, "ExplosionParticleInfo count is negative", "readExplosionParticleList");
    }
    out.reserve(static_cast<usize>(count));
    for (i32 i = 0; i < count; ++i) {
        ir::play::ExplosionParticleInfo e{};
        MC_TRY_ASSIGN(e.particle, readParticleOptions(buf));
        MC_TRY_ASSIGN(e.scaling, buf.readF32());
        MC_TRY_ASSIGN(e.speed, buf.readF32());
        i32 weight = 0;
        MC_TRY_ASSIGN(weight, buf.readVarInt());
        (void)weight; // 我方互通统一权重，读侧丢弃
        out.push_back(std::move(e));
    }
    return out;
}

} // namespace play_ext_detail

/// Explosion（S→C，id=36，1.21.11 结构化）
[[nodiscard]] inline auto explosionCodec()
{
    return makeCodec<ir::play::Explosion>(
        [](B& buf, const ir::play::Explosion& v) {
            // Vec3 center
            buf.writeF64(v.centerX);
            buf.writeF64(v.centerY);
            buf.writeF64(v.centerZ);
            buf.writeF32(v.radius);
            buf.writeI32(v.blockCount);
            // Optional<Vec3> playerKnockback
            buf.writeBool(v.hasPlayerKnockback);
            if (v.hasPlayerKnockback) {
                buf.writeF64(v.knockbackX);
                buf.writeF64(v.knockbackY);
                buf.writeF64(v.knockbackZ);
            }
            play_ext_detail::writeParticleOptions(buf, v.explosionParticle);
            play_ext_detail::writeSoundEventHolder(buf, v.explosionSound);
            play_ext_detail::writeExplosionParticleList(buf, v.blockParticles);
        },
        [](B& buf) -> Result<ir::play::Explosion> {
            ir::play::Explosion v{};
            MC_TRY_ASSIGN(v.centerX, buf.readF64());
            MC_TRY_ASSIGN(v.centerY, buf.readF64());
            MC_TRY_ASSIGN(v.centerZ, buf.readF64());
            MC_TRY_ASSIGN(v.radius, buf.readF32());
            MC_TRY_ASSIGN(v.blockCount, buf.readI32());
            MC_TRY_ASSIGN(v.hasPlayerKnockback, buf.readBool());
            if (v.hasPlayerKnockback) {
                MC_TRY_ASSIGN(v.knockbackX, buf.readF64());
                MC_TRY_ASSIGN(v.knockbackY, buf.readF64());
                MC_TRY_ASSIGN(v.knockbackZ, buf.readF64());
            }
            MC_TRY_ASSIGN(v.explosionParticle, play_ext_detail::readParticleOptions(buf));
            MC_TRY_ASSIGN(v.explosionSound, play_ext_detail::readSoundEventHolder(buf));
            MC_TRY_ASSIGN(v.blockParticles, play_ext_detail::readExplosionParticleList(buf));
            return v;
        });
}

// ============================================================================
// 载具 / 交互
// ============================================================================

/// ServerboundMoveVehicle（C→S，id=33）
[[nodiscard]] inline auto serverboundMoveVehicleCodec()
{
    return makeCodec<ir::play::ServerboundMoveVehicle>(
        [](B& buf, const ir::play::ServerboundMoveVehicle& v) {
            buf.writeF64(v.x);
            buf.writeF64(v.y);
            buf.writeF64(v.z);
            buf.writeF32(v.yRot);
            buf.writeF32(v.xRot);
            buf.writeBool(v.onGround);
        },
        [](B& buf) -> Result<ir::play::ServerboundMoveVehicle> {
            ir::play::ServerboundMoveVehicle v{};
            MC_TRY_ASSIGN(v.x, buf.readF64());
            MC_TRY_ASSIGN(v.y, buf.readF64());
            MC_TRY_ASSIGN(v.z, buf.readF64());
            MC_TRY_ASSIGN(v.yRot, buf.readF32());
            MC_TRY_ASSIGN(v.xRot, buf.readF32());
            MC_TRY_ASSIGN(v.onGround, buf.readBool());
            return v;
        });
}

/// ClientboundMoveVehicle（S→C，id=55）
[[nodiscard]] inline auto clientboundMoveVehicleCodec()
{
    return makeCodec<ir::play::ClientboundMoveVehicle>(
        [](B& buf, const ir::play::ClientboundMoveVehicle& v) {
            buf.writeF64(v.x);
            buf.writeF64(v.y);
            buf.writeF64(v.z);
            buf.writeF32(v.yRot);
            buf.writeF32(v.xRot);
        },
        [](B& buf) -> Result<ir::play::ClientboundMoveVehicle> {
            ir::play::ClientboundMoveVehicle v{};
            MC_TRY_ASSIGN(v.x, buf.readF64());
            MC_TRY_ASSIGN(v.y, buf.readF64());
            MC_TRY_ASSIGN(v.z, buf.readF64());
            MC_TRY_ASSIGN(v.yRot, buf.readF32());
            MC_TRY_ASSIGN(v.xRot, buf.readF32());
            return v;
        });
}

/// PaddleBoat（C→S，id=34）
[[nodiscard]] inline auto paddleBoatCodec()
{
    return makeCodec<ir::play::PaddleBoat>(
        [](B& buf, const ir::play::PaddleBoat& v) {
            buf.writeBool(v.left);
            buf.writeBool(v.right);
        },
        [](B& buf) -> Result<ir::play::PaddleBoat> {
            ir::play::PaddleBoat v{};
            MC_TRY_ASSIGN(v.left, buf.readBool());
            MC_TRY_ASSIGN(v.right, buf.readBool());
            return v;
        });
}

/// Interact（C→S，id=25，分发 action）
[[nodiscard]] inline auto interactCodec()
{
    return makeCodec<ir::play::Interact>(
        [](B& buf, const ir::play::Interact& v) {
            buf.writeVarInt(v.entityId);
            buf.writeVarInt(v.action);
            if (v.action == 0) { // INTERACT
                buf.writeVarInt(v.hand);
            } else if (v.action == 2) { // INTERACT_AT
                buf.writeF32(v.hitX);
                buf.writeF32(v.hitY);
                buf.writeF32(v.hitZ);
                buf.writeVarInt(v.hand);
            }
            // ATTACK(1) 无 action 专属字段
            buf.writeBool(v.usingSecondaryAction);
        },
        [](B& buf) -> Result<ir::play::Interact> {
            ir::play::Interact v{};
            MC_TRY_ASSIGN(v.entityId, buf.readVarInt());
            MC_TRY_ASSIGN(v.action, buf.readVarInt());
            if (v.action == 0) {
                MC_TRY_ASSIGN(v.hand, buf.readVarInt());
            } else if (v.action == 2) {
                MC_TRY_ASSIGN(v.hitX, buf.readF32());
                MC_TRY_ASSIGN(v.hitY, buf.readF32());
                MC_TRY_ASSIGN(v.hitZ, buf.readF32());
                MC_TRY_ASSIGN(v.hand, buf.readVarInt());
            }
            MC_TRY_ASSIGN(v.usingSecondaryAction, buf.readBool());
            return v;
        });
}

// ============================================================================
// 命令树（S→C，opaque）
// ============================================================================

/// Commands（S→C，id=16）
/// 线格式（ClientboundCommandsPacket）：VarInt(nodeCount) + nodes + VarInt(rootIndex)，
/// 即整个包体就是命令树二进制，无外层长度前缀（包长已在传输层 VarInt 帧头）。
/// v.payload 由 CommandTreeEncoder 产出完整包体字节，故写侧直接 writeBytes（勿用 writeOpaque
/// 加 VarInt 长度前缀——旧 JSON-opaque 占位用 writeOpaque 是有意让客户端按 JSON 跳过，真 Java
/// 客户端会把这个额外长度前缀当成 nodeCount 解码，致游标错位 IndexOutOfBoundsException）。
[[nodiscard]] inline auto commandsCodec()
{
    return makeCodec<ir::play::Commands>(
        [](B& buf, const ir::play::Commands& v) { buf.writeBytes(v.payload.data(), v.payload.size()); },
        [](B& buf) -> Result<ir::play::Commands> {
            ir::play::Commands v{};
            // 包体无前缀，剩余字节即完整命令树二进制。
            usize remaining = buf.readableBytes();
            MC_TRY_ASSIGN(v.payload, buf.readBytes(remaining));
            return v;
        });
}

/// PlaceRecipe（C→S，id=38）
[[nodiscard]] inline auto placeRecipeCodec()
{
    return makeCodec<ir::play::PlaceRecipe>(
        [](B& buf, const ir::play::PlaceRecipe& v) {
            buf.writeVarInt(v.containerId);
            buf.writeVarInt(v.recipe);
            buf.writeBool(v.useMaxItems);
        },
        [](B& buf) -> Result<ir::play::PlaceRecipe> {
            ir::play::PlaceRecipe v{};
            MC_TRY_ASSIGN(v.containerId, buf.readVarInt());
            MC_TRY_ASSIGN(v.recipe, buf.readVarInt());
            MC_TRY_ASSIGN(v.useMaxItems, buf.readBool());
            return v;
        });
}

} // namespace mc::network::backend::java::codecs
