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
#include "common/network/backend/java/codecs/JavaConfigurationCodecs.hpp"
#include "common/network/backend/java/codecs/JavaPlayCodecs.hpp"

#include <array>

namespace mc::network::backend::java::codecs {

// B 与 makeCodec 定义在 JavaCodecBase.hpp（供各阶段 codec 头文件独立包含）。

// ============================================================================
// Handshake 阶段
// ============================================================================

/**
 * @brief ClientIntention（C→S，id=0）
 *
 * 线格式：VarInt(protocolVersion) + Utf8(255, hostName) + U16(port) + VarInt(intendedState)。
 * 注意 Java 写 port 用 writeShort（带符号 short），读用 readUnsignedShort；此处按 u16 处理一致。
 */
[[nodiscard]] inline auto clientIntentionCodec()
{
    return makeCodec<ir::handshake::ClientIntention>(
        [](B& buf, const ir::handshake::ClientIntention& v) {
            buf.writeVarInt(v.protocolVersion);
            buf.writeString(v.hostName);
            buf.writeU16(v.port);
            buf.writeVarInt(v.intendedState);
        },
        [](B& buf) -> Result<ir::handshake::ClientIntention> {
            ir::handshake::ClientIntention v{};
            MC_TRY_ASSIGN(v.protocolVersion, buf.readVarInt());
            MC_TRY_ASSIGN(v.hostName, buf.readString());
            MC_TRY_ASSIGN(v.port, buf.readU16());
            MC_TRY_ASSIGN(v.intendedState, buf.readVarInt());
            return v;
        });
}

// ============================================================================
// Status 阶段
// ============================================================================

/// StatusRequest（C→S，id=0）：无字段，空包。
[[nodiscard]] inline auto statusRequestCodec()
{
    return makeCodec<ir::status::StatusRequest>([](B&, const ir::status::StatusRequest&) {},
        [](B&) -> Result<ir::status::StatusRequest> { return ir::status::StatusRequest{}; });
}

/// StatusResponse（S→C，id=0）：Utf8(json)。
[[nodiscard]] inline auto statusResponseCodec()
{
    return makeCodec<ir::status::StatusResponse>(
        [](B& buf, const ir::status::StatusResponse& v) { buf.writeString(v.json); },
        [](B& buf) -> Result<ir::status::StatusResponse> {
            ir::status::StatusResponse v{};
            MC_TRY_ASSIGN(v.json, buf.readString());
            return v;
        });
}

/// PingRequest（C→S，id=1）：I64(payload)。
[[nodiscard]] inline auto pingRequestCodec()
{
    return makeCodec<ir::status::PingRequest>([](B& buf, const ir::status::PingRequest& v) { buf.writeI64(v.payload); },
        [](B& buf) -> Result<ir::status::PingRequest> {
            ir::status::PingRequest v{};
            MC_TRY_ASSIGN(v.payload, buf.readI64());
            return v;
        });
}

/// PingResponse（S→C，id=1）：I64(payload)。
[[nodiscard]] inline auto pingResponseCodec()
{
    return makeCodec<ir::status::PingResponse>(
        [](B& buf, const ir::status::PingResponse& v) { buf.writeI64(v.payload); },
        [](B& buf) -> Result<ir::status::PingResponse> {
            ir::status::PingResponse v{};
            MC_TRY_ASSIGN(v.payload, buf.readI64());
            return v;
        });
}

// ============================================================================
// Login 阶段
// ============================================================================

namespace login_detail {

/**
 * @brief 写 ByteArray（VarInt 长度 + 字节），对齐 Java FriendlyByteBuf.writeByteArray
 */
inline void writeByteArray(B& buf, const u8* data, usize size)
{
    buf.writeVarInt(static_cast<i32>(size));
    buf.writeBytes(data, size);
}

[[nodiscard]] inline Result<std::vector<u8>> readByteArray(B& buf)
{
    i32 length = 0;
    MC_TRY_ASSIGN(length, buf.readVarInt());
    if (length < 0) {
        return Error(ErrorCode::InvalidData, "ByteArray 长度为负", "readByteArray");
    }
    return buf.readBytes(static_cast<usize>(length));
}

} // namespace login_detail

/**
 * @brief Hello（C→S，id=0）
 *
 * 线格式：Utf8(16, name) + Uuid(profileId)。
 * 当前 IR 的 Hello.username 对应 name；publicKey/keySignature 在离线模式省略。
 * TODO(Phase3/在线模式): 在线模式补 profile 公钥 + keySignature。
 */
[[nodiscard]] inline auto helloCodec()
{
    return makeCodec<ir::login::Hello>(
        [](B& buf, const ir::login::Hello& v) {
            buf.writeString(v.username);
            // UUID：当前 IR Hello 无独立 UUID 字段（离线模式由服务端按用户名生成），
            // 写一个零 UUID 占位以对齐 Java 帧格式（在线模式由 profileId 填充）。
            for (int i = 0; i < 16; ++i) {
                buf.writeU8(0);
            }
        },
        [](B& buf) -> Result<ir::login::Hello> {
            ir::login::Hello v{};
            MC_TRY_ASSIGN(v.username, buf.readString());
            std::array<u8, 16> uuid{};
            MC_TRY(buf.readBytes(uuid.data(), 16)); // 丢弃 profileId（离线模式不校验）
            return v;
        });
}

/**
 * @brief HelloBound（S→C，id=1）
 *
 * 线格式：Utf8(20, serverId) + ByteArray(publicKey) + ByteArray(challenge) + Bool(shouldAuthenticate)。
 */
[[nodiscard]] inline auto helloBoundCodec()
{
    return makeCodec<ir::login::HelloBound>(
        [](B& buf, const ir::login::HelloBound& v) {
            buf.writeString(v.serverId);
            login_detail::writeByteArray(buf, v.publicKey.data(), v.publicKey.size());
            login_detail::writeByteArray(buf, v.verifyToken.data(), v.verifyToken.size());
            buf.writeBool(v.shouldAuthenticate);
        },
        [](B& buf) -> Result<ir::login::HelloBound> {
            ir::login::HelloBound v{};
            MC_TRY_ASSIGN(v.serverId, buf.readString());
            MC_TRY_ASSIGN(v.publicKey, login_detail::readByteArray(buf));
            MC_TRY_ASSIGN(v.verifyToken, login_detail::readByteArray(buf));
            MC_TRY_ASSIGN(v.shouldAuthenticate, buf.readBool());
            return v;
        });
}

/**
 * @brief Key（C→S，id=1）
 *
 * 线格式：ByteArray(encryptedSharedSecret) + ByteArray(encryptedVerifyToken)。
 */
[[nodiscard]] inline auto keyCodec()
{
    return makeCodec<ir::login::Key>(
        [](B& buf, const ir::login::Key& v) {
            login_detail::writeByteArray(buf, v.encryptedSharedSecret.data(), v.encryptedSharedSecret.size());
            login_detail::writeByteArray(buf, v.encryptedVerifyToken.data(), v.encryptedVerifyToken.size());
        },
        [](B& buf) -> Result<ir::login::Key> {
            ir::login::Key v{};
            MC_TRY_ASSIGN(v.encryptedSharedSecret, login_detail::readByteArray(buf));
            MC_TRY_ASSIGN(v.encryptedVerifyToken, login_detail::readByteArray(buf));
            return v;
        });
}

/**
 * @brief LoginFinished（S→C，id=2）
 *
 * 线格式：Uuid + Utf8(16, name) + 属性表(VarInt count + 每项 name/value)。
 * 当前 IR LoginFinished.uuid 是 16 字节数组、properties 是 name→value 对，与此对应。
 */
[[nodiscard]] inline auto loginFinishedCodec()
{
    return makeCodec<ir::login::LoginFinished>(
        [](B& buf, const ir::login::LoginFinished& v) {
            buf.writeBytes(v.uuid.data(), v.uuid.size());
            buf.writeString(v.username);
            buf.writeVarInt(static_cast<i32>(v.properties.size()));
            for (const auto& prop : v.properties) {
                buf.writeString(prop.first);
                buf.writeString(prop.second);
                buf.writeBool(false); // signature 可空：false 表示无签名（省略 signature 字段）
            }
        },
        [](B& buf) -> Result<ir::login::LoginFinished> {
            ir::login::LoginFinished v{};
            MC_TRY(buf.readBytes(v.uuid.data(), v.uuid.size()));
            MC_TRY_ASSIGN(v.username, buf.readString());
            i32 count = 0;
            MC_TRY_ASSIGN(count, buf.readVarInt());
            if (count < 0) {
                return Error(ErrorCode::InvalidData, "属性数为负", "loginFinishedCodec");
            }
            for (i32 i = 0; i < count; ++i) {
                std::string name;
                std::string value;
                MC_TRY_ASSIGN(name, buf.readString());
                MC_TRY_ASSIGN(value, buf.readString());
                bool hasSignature = false;
                MC_TRY_ASSIGN(hasSignature, buf.readBool());
                if (hasSignature) {
                    std::string sig;
                    MC_TRY_ASSIGN(sig, buf.readString()); // 丢弃 signature
                }
                v.properties.emplace_back(std::move(name), std::move(value));
            }
            return v;
        });
}

/**
 * @brief LoginCompression（S→C，id=3）：VarInt(threshold)。
 */
[[nodiscard]] inline auto loginCompressionCodec()
{
    return makeCodec<ir::login::LoginCompression>(
        [](B& buf, const ir::login::LoginCompression& v) { buf.writeVarInt(v.threshold); },
        [](B& buf) -> Result<ir::login::LoginCompression> {
            ir::login::LoginCompression v{};
            MC_TRY_ASSIGN(v.threshold, buf.readVarInt());
            return v;
        });
}

/**
 * @brief LoginAcknowledged（C→S，id=3）：无字段，空包。
 */
[[nodiscard]] inline auto loginAcknowledgedCodec()
{
    return makeCodec<ir::login::LoginAcknowledged>([](B&, const ir::login::LoginAcknowledged&) {},
        [](B&) -> Result<ir::login::LoginAcknowledged> { return ir::login::LoginAcknowledged{}; });
}

/**
 * @brief Disconnect（S→C，id=0）：Utf8(reason JSON)。
 */
[[nodiscard]] inline auto loginDisconnectCodec()
{
    return makeCodec<ir::login::Disconnect>([](B& buf, const ir::login::Disconnect& v) { buf.writeString(v.reason); },
        [](B& buf) -> Result<ir::login::Disconnect> {
            ir::login::Disconnect v{};
            MC_TRY_ASSIGN(v.reason, buf.readString());
            return v;
        });
}

// ============================================================================
// Configuration 阶段
//
// 全部 codec 已迁移到 JavaConfigurationCodecs.hpp（对齐 1.21.11 在用包子集：
// ClientInformation/CustomPayload/Disconnect/FinishConfiguration/KeepAlive/Ping/
// RegistryData/SelectKnownPacks/UpdateEnabledFeatures/UpdateTags）。本文件不再重复定义。
// ============================================================================

// ============================================================================
// Play 阶段
//
// 全部 codec 已迁移到 JavaPlayCodecs.hpp（对齐 1.21.11 在用包子集，含 LpVec3/
// packedDegrees/UUID 经 JavaWireHelpers.hpp 实现，以及 HashedStack/ItemStackView/
// SpawnInfo/PlayerInfo 等子结构）。本文件不再重复定义。
// ============================================================================

} // namespace mc::network::backend::java::codecs
