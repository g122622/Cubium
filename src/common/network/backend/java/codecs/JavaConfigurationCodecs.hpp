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
#include "common/network/buffer/NbtIo.hpp"
#include "common/network/ir/IrPacket.hpp"

namespace mc::network::backend::java::codecs {

// ============================================================================
// Configuration 阶段 codec（对齐 Java 1.21.11 ConfigurationProtocols 注册顺序）
// ============================================================================

namespace cfg_detail {

/**
 * @brief 写 KnownPack（Utf8(ns)+Utf8(id)+Utf8(version)）
 */
inline void writeKnownPack(B& buf, const ir::configuration::KnownPack& kp)
{
    buf.writeString(kp.ns);
    buf.writeString(kp.id);
    buf.writeString(kp.version);
}

[[nodiscard]] inline Result<ir::configuration::KnownPack> readKnownPack(B& buf)
{
    ir::configuration::KnownPack kp{};
    MC_TRY_ASSIGN(kp.ns, buf.readString());
    MC_TRY_ASSIGN(kp.id, buf.readString());
    MC_TRY_ASSIGN(kp.version, buf.readString());
    return kp;
}

} // namespace cfg_detail

/// ClientInformation（C→S，id=0）
[[nodiscard]] inline auto clientInformationCodec()
{
    return makeCodec<ir::configuration::ClientInformation>(
        [](B& buf, const ir::configuration::ClientInformation& v) {
            buf.writeString(v.language);
            buf.writeU8(v.viewDistance);
            buf.writeVarInt(v.chatVisibility);
            buf.writeBool(v.chatColors);
            buf.writeU8(v.modelCustomisation);
            buf.writeVarInt(v.mainHand);
            buf.writeBool(v.textFilteringEnabled);
            buf.writeBool(v.allowsListing);
            buf.writeVarInt(v.particleStatus);
        },
        [](B& buf) -> Result<ir::configuration::ClientInformation> {
            ir::configuration::ClientInformation v{};
            MC_TRY_ASSIGN(v.language, buf.readString());
            MC_TRY_ASSIGN(v.viewDistance, buf.readU8());
            MC_TRY_ASSIGN(v.chatVisibility, buf.readVarInt());
            MC_TRY_ASSIGN(v.chatColors, buf.readBool());
            MC_TRY_ASSIGN(v.modelCustomisation, buf.readU8());
            MC_TRY_ASSIGN(v.mainHand, buf.readVarInt());
            MC_TRY_ASSIGN(v.textFilteringEnabled, buf.readBool());
            MC_TRY_ASSIGN(v.allowsListing, buf.readBool());
            MC_TRY_ASSIGN(v.particleStatus, buf.readVarInt());
            return v;
        });
}

/// CustomPayload（双向，S→C id=1 / C→S id=2）
[[nodiscard]] inline auto configurationCustomPayloadCodec()
{
    return makeCodec<ir::configuration::CustomPayload>(
        [](B& buf, const ir::configuration::CustomPayload& v) {
            buf.writeString(v.identifier);
            buf.writeVarInt(static_cast<i32>(v.payload.size()));
            buf.writeBytes(v.payload.data(), v.payload.size());
        },
        [](B& buf) -> Result<ir::configuration::CustomPayload> {
            ir::configuration::CustomPayload v{};
            MC_TRY_ASSIGN(v.identifier, buf.readString());
            i32 len = 0;
            MC_TRY_ASSIGN(len, buf.readVarInt());
            if (len < 0) {
                return Error(
                    ErrorCode::InvalidData, "CustomPayload length is negative", "configurationCustomPayloadCodec");
            }
            MC_TRY_ASSIGN(v.payload, buf.readBytes(static_cast<usize>(len)));
            return v;
        });
}

/// Disconnect（S→C，id=2，reason 为 NBT 承载的 Component，非裸字符串）
[[nodiscard]] inline auto configurationDisconnectCodec()
{
    return makeCodec<ir::configuration::Disconnect>(
        [](B& buf, const ir::configuration::Disconnect& v) { writeTextComponentNbt(buf, v.reason); },
        [](B& buf) -> Result<ir::configuration::Disconnect> {
            ir::configuration::Disconnect v{};
            MC_TRY_ASSIGN(v.reason, readTextComponentNbt(buf));
            return v;
        });
}

/// FinishConfiguration（S→C id=3 / C→S id=3）：空包
[[nodiscard]] inline auto finishConfigurationCodec()
{
    return makeCodec<ir::configuration::FinishConfiguration>([](B&, const ir::configuration::FinishConfiguration&) {},
        [](B&) -> Result<ir::configuration::FinishConfiguration> { return ir::configuration::FinishConfiguration{}; });
}

/// KeepAlive（配置阶段双向，id=4）：I64(id) 固定8字节大端，对齐 vanilla writeLong/readLong
[[nodiscard]] inline auto configurationKeepAliveCodec()
{
    return makeCodec<ir::configuration::KeepAlive>(
        [](B& buf, const ir::configuration::KeepAlive& v) { buf.writeI64(v.id); },
        [](B& buf) -> Result<ir::configuration::KeepAlive> {
            ir::configuration::KeepAlive v{};
            MC_TRY_ASSIGN(v.id, buf.readI64());
            return v;
        });
}

/// Ping（S→C id=5 / C→S id=5 pong）：I32(parameter)
[[nodiscard]] inline auto configurationPingCodec()
{
    return makeCodec<ir::configuration::Ping>(
        [](B& buf, const ir::configuration::Ping& v) { buf.writeI32(v.parameter); },
        [](B& buf) -> Result<ir::configuration::Ping> {
            ir::configuration::Ping v{};
            MC_TRY_ASSIGN(v.parameter, buf.readI32());
            return v;
        });
}

/// RegistryData（S→C，id=7）
/// 线格式：Utf8(registryKey) + VarInt(count) + count×{ Utf8(id) + Bool(present) + [根 NBT 自定界] }
///
/// 对齐 Java RegistrySynchronization.PackedRegistryEntry.STREAM_CODEC：data 字段用
/// ByteBufCodecs.TAG.apply(ByteBufCodecs::optional)。optional 写 Bool 前缀；present 时
/// ByteBufCodecs.TAG = FriendlyByteBuf.writeNbt 写【自定界根 NBT】（0x0A + entries + End，
/// **无 root name 前缀、无外部长度前缀**——NbtIo.writeAnyTag 只写 writeByte(0x0A)+tag.write()，
/// readAnyTag 对称不读 name，NBT 自带 End 定界）。曾误加 writeVarInt(size) 长度前缀致 "Invalid
/// tag id"，又曾误加 0x00 0x00 空 root name 致 "Expected non-null compound tag"
/// （disconnect-2026-07-29_13.51.13-client.txt）。
[[nodiscard]] inline auto registryDataCodec()
{
    return makeCodec<ir::configuration::RegistryData>(
        [](B& buf, const ir::configuration::RegistryData& v) {
            buf.writeString(v.registryKey);
            buf.writeVarInt(static_cast<i32>(v.entries.size()));
            for (const auto& e : v.entries) {
                buf.writeString(e.id);
                if (e.data.has_value()) {
                    buf.writeBool(true);
                    // 直接写根 NBT 原始字节（e.data 内须已是 0x0A + body + End 线格式，无 root name，
                    // 由 EnchantmentNbtBuilder::serializeRootCompoundToBytes 产出）。
                    buf.writeBytes(e.data->data(), e.data->size());
                } else {
                    buf.writeBool(false);
                }
            }
        },
        [](B& buf) -> Result<ir::configuration::RegistryData> {
            ir::configuration::RegistryData v{};
            MC_TRY_ASSIGN(v.registryKey, buf.readString());
            i32 count = 0;
            MC_TRY_ASSIGN(count, buf.readVarInt());
            if (count < 0) {
                return Error(ErrorCode::InvalidData, "RegistryData entry count is negative", "registryDataCodec");
            }
            for (i32 i = 0; i < count; ++i) {
                ir::configuration::RegistryEntry e{};
                MC_TRY_ASSIGN(e.id, buf.readString());
                bool present = false;
                MC_TRY_ASSIGN(present, buf.readBool());
                if (present) {
                    // 根 NBT 自定界：记录起始游标，readRootCompound 推进到 End 之后，
                    // 切出 [start, end) 原始字节存入 e.data（与写侧对称，无 root name）。
                    const usize start = buf.readPosition();
                    auto nbtResult = buffer::nbt_io::readRootCompound(buf);
                    if (!nbtResult.success()) {
                        return nbtResult.error();
                    }
                    const usize end = buf.readPosition();
                    if (end < start) {
                        return Error(
                            ErrorCode::InvalidData, "RegistryEntry NBT cursor went backwards", "registryDataCodec");
                    }
                    const auto& all = buf.bytes();
                    e.data = std::vector<u8>(all.begin() + static_cast<std::ptrdiff_t>(start),
                        all.begin() + static_cast<std::ptrdiff_t>(end));
                }
                v.entries.push_back(std::move(e));
            }
            return v;
        });
}

/// SelectKnownPacks（S→C id=14 / C→S id=7）
[[nodiscard]] inline auto selectKnownPacksCodec()
{
    return makeCodec<ir::configuration::SelectKnownPacks>(
        [](B& buf, const ir::configuration::SelectKnownPacks& v) {
            buf.writeVarInt(static_cast<i32>(v.knownPacks.size()));
            for (const auto& kp : v.knownPacks) {
                cfg_detail::writeKnownPack(buf, kp);
            }
        },
        [](B& buf) -> Result<ir::configuration::SelectKnownPacks> {
            ir::configuration::SelectKnownPacks v{};
            i32 count = 0;
            MC_TRY_ASSIGN(count, buf.readVarInt());
            if (count < 0) {
                return Error(ErrorCode::InvalidData, "SelectKnownPacks count is negative", "selectKnownPacksCodec");
            }
            for (i32 i = 0; i < count; ++i) {
                ir::configuration::KnownPack kp{};
                MC_TRY_ASSIGN(kp, cfg_detail::readKnownPack(buf));
                v.knownPacks.push_back(std::move(kp));
            }
            return v;
        });
}

/// UpdateEnabledFeatures（S→C，id=12）
[[nodiscard]] inline auto updateEnabledFeaturesCodec()
{
    return makeCodec<ir::configuration::UpdateEnabledFeatures>(
        [](B& buf, const ir::configuration::UpdateEnabledFeatures& v) {
            buf.writeVarInt(static_cast<i32>(v.features.size()));
            for (const auto& f : v.features) {
                buf.writeString(f);
            }
        },
        [](B& buf) -> Result<ir::configuration::UpdateEnabledFeatures> {
            ir::configuration::UpdateEnabledFeatures v{};
            i32 count = 0;
            MC_TRY_ASSIGN(count, buf.readVarInt());
            if (count < 0) {
                return Error(ErrorCode::InvalidData, "Features count is negative", "updateEnabledFeaturesCodec");
            }
            for (i32 i = 0; i < count; ++i) {
                std::string f;
                MC_TRY_ASSIGN(f, buf.readString());
                v.features.push_back(std::move(f));
            }
            return v;
        });
}

/// UpdateTags（S→C，id=13）
[[nodiscard]] inline auto updateTagsCodec()
{
    return makeCodec<ir::configuration::UpdateTags>(
        [](B& buf, const ir::configuration::UpdateTags& v) {
            buf.writeVarInt(static_cast<i32>(v.registries.size()));
            for (const auto& reg : v.registries) {
                buf.writeString(reg.registryKey);
                buf.writeVarInt(static_cast<i32>(reg.tags.size()));
                for (const auto& tag : reg.tags) {
                    buf.writeString(tag.tagName);
                    buf.writeVarInt(static_cast<i32>(tag.elementIds.size()));
                    for (i32 id : tag.elementIds) {
                        buf.writeVarInt(id);
                    }
                }
            }
        },
        [](B& buf) -> Result<ir::configuration::UpdateTags> {
            ir::configuration::UpdateTags v{};
            i32 regCount = 0;
            MC_TRY_ASSIGN(regCount, buf.readVarInt());
            if (regCount < 0) {
                return Error(ErrorCode::InvalidData, "UpdateTags registry count is negative", "updateTagsCodec");
            }
            for (i32 r = 0; r < regCount; ++r) {
                ir::configuration::TagRegistry reg{};
                MC_TRY_ASSIGN(reg.registryKey, buf.readString());
                i32 tagCount = 0;
                MC_TRY_ASSIGN(tagCount, buf.readVarInt());
                if (tagCount < 0) {
                    return Error(ErrorCode::InvalidData, "UpdateTags tag count is negative", "updateTagsCodec");
                }
                for (i32 t = 0; t < tagCount; ++t) {
                    ir::configuration::TagListEntry tag{};
                    MC_TRY_ASSIGN(tag.tagName, buf.readString());
                    i32 idCount = 0;
                    MC_TRY_ASSIGN(idCount, buf.readVarInt());
                    if (idCount < 0) {
                        return Error(ErrorCode::InvalidData, "UpdateTags id count is negative", "updateTagsCodec");
                    }
                    for (i32 i = 0; i < idCount; ++i) {
                        i32 id = 0;
                        MC_TRY_ASSIGN(id, buf.readVarInt());
                        tag.elementIds.push_back(id);
                    }
                    reg.tags.push_back(std::move(tag));
                }
                v.registries.push_back(std::move(reg));
            }
            return v;
        });
}

} // namespace mc::network::backend::java::codecs
