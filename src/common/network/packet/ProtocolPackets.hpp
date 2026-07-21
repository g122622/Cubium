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

#include "PacketSerializer.hpp"
#include "common/command/ICommandSource.hpp" // for Uuid
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/profiler/TraceEvents.hpp"
#include "common/util/Direction.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/chunk/base/ChunkPos.hpp"
#include <algorithm>
#include <memory>

namespace mc::network {

// ============================================================================
// 协议常量
// ============================================================================

namespace protocol {
// Minecraft 1.16.5 协议版本 (754)
// 注意：753 是 1.16.4，754 才是 1.16.5
constexpr i32 VERSION = 754;
constexpr i32 MIN_VERSION = 754;
constexpr i32 MAX_VERSION = 754;

// 字符串长度限制
constexpr size_t MAX_USERNAME_LENGTH = 16;
constexpr size_t MAX_CHAT_LENGTH = 256;
constexpr size_t MAX_REASON_LENGTH = 1024;

// 位置精度
constexpr f32 POSITION_SCALE = 4096.0f;      // 用于delta位置编码
constexpr f32 ANGLE_SCALE = 256.0f / 360.0f; // 角度编码

// 传送确认超时
constexpr u32 TELEPORT_TIMEOUT_MS = 5000;

// 区块数据限制
constexpr size_t MAX_CHUNK_DATA_SIZE = 1024 * 1024; // 1MB
} // namespace protocol

// ============================================================================
// 协议状态
// ============================================================================

enum class ProtocolState : u8 { Handshaking = 0, Status = 1, Login = 2, Play = 3 };

// ============================================================================
// 玩家位置数据
// ============================================================================

struct PlayerPosition {
    f64 x = 0.0;
    f64 y = 0.0;
    f64 z = 0.0;
    f32 yaw = 0.0f;
    f32 pitch = 0.0f;
    bool onGround = true;

    PlayerPosition() = default;
    PlayerPosition(f64 x, f64 y, f64 z, f32 yaw = 0.0f, f32 pitch = 0.0f, bool ground = true)
        : x(x)
        , y(y)
        , z(z)
        , yaw(yaw)
        , pitch(pitch)
        , onGround(ground)
    {}

    [[nodiscard]] Vector3 toVector3() const
    {
        return Vector3(static_cast<f32>(x), static_cast<f32>(y), static_cast<f32>(z));
    }

    void serialize(PacketSerializer& ser) const
    {
        ser.writeF64(x);
        ser.writeF64(y);
        ser.writeF64(z);
        ser.writeF32(yaw);
        ser.writeF32(pitch);
        ser.writeBool(onGround);
    }

    [[nodiscard]] static Result<PlayerPosition> deserialize(PacketDeserializer& deser)
    {
        PlayerPosition pos;
        auto xResult = deser.readF64();
        if (xResult.failed()) return xResult.error();
        pos.x = xResult.value();

        auto yResult = deser.readF64();
        if (yResult.failed()) return yResult.error();
        pos.y = yResult.value();

        auto zResult = deser.readF64();
        if (zResult.failed()) return zResult.error();
        pos.z = zResult.value();

        auto yawResult = deser.readF32();
        if (yawResult.failed()) return yawResult.error();
        pos.yaw = yawResult.value();

        auto pitchResult = deser.readF32();
        if (pitchResult.failed()) return pitchResult.error();
        pos.pitch = pitchResult.value();

        auto groundResult = deser.readBool();
        if (groundResult.failed()) return groundResult.error();
        pos.onGround = groundResult.value();

        return pos;
    }
};

// ============================================================================
// 登录请求包 (客户端 -> 服务端)
// ============================================================================

class LoginRequestPacket {
public:
    LoginRequestPacket() = default;
    explicit LoginRequestPacket(std::string username, i32 protocolVersion = protocol::VERSION)
        : m_username(std::move(username))
        , m_protocolVersion(protocolVersion)
    {}

    // Getters
    const std::string& username() const { return m_username; }
    i32 protocolVersion() const { return m_protocolVersion; }

    // Setters
    void setUsername(const std::string& username) { m_username = username; }
    void setProtocolVersion(i32 version) { m_protocolVersion = version; }

    // 序列化
    void serialize(PacketSerializer& ser) const
    {
        ser.writeVarInt(m_protocolVersion);
        ser.writeString(m_username);
    }

    [[nodiscard]] static Result<LoginRequestPacket> deserialize(PacketDeserializer& deser)
    {
        LoginRequestPacket packet;

        auto versionResult = deser.readVarInt();
        if (versionResult.failed()) {
            return versionResult.error();
        }
        packet.m_protocolVersion = versionResult.value();

        auto usernameResult = deser.readString();
        if (usernameResult.failed()) {
            return usernameResult.error();
        }
        packet.m_username = usernameResult.value();

        // 验证
        if (packet.m_username.empty() || packet.m_username.size() > protocol::MAX_USERNAME_LENGTH) {
            return Error(ErrorCode::InvalidData, "Invalid username length");
        }

        return packet;
    }

private:
    std::string m_username;
    i32 m_protocolVersion = protocol::VERSION;
};

// ============================================================================
// 登录响应包 (服务端 -> 客户端)
// ============================================================================

/**
 * @brief 登录响应包 (服务端 -> 客户端)
 *
 * 服务端响应客户端登录请求，包含：
 * - 登录成功/失败状态
 * - 玩家ID（网络会话标识）
 * - 实体ID（世界实体标识）
 * - 玩家UUID（离线模式由服务端 generateOfflineUuid 生成）
 * - 用户名
 * - 初始维度
 *
 * 重要：playerId 和 entityId 是独立的标识符，不能互换。
 * UUID 是玩家持久身份标识，用于皮肤/玩家列表等需要跨会话标识的场景。
 */
class LoginResponsePacket {
public:
    LoginResponsePacket() = default;
    LoginResponsePacket(bool success,
        PlayerId playerId,
        EntityInstanceId entityId,
        const Uuid& uuid,
        const std::string& username,
        const std::string& message = "",
        bool isDebugWorld = false,
        DimensionId dimension = 0)
        : m_success(success)
        , m_playerId(playerId)
        , m_entityId(entityId)
        , m_uuid(uuid)
        , m_username(username)
        , m_message(message)
        , m_isDebugWorld(isDebugWorld)
        , m_dimension(dimension)
    {}

    // Getters
    bool success() const { return m_success; }
    PlayerId playerId() const { return m_playerId; }
    EntityInstanceId entityId() const { return m_entityId; }
    const Uuid& uuid() const { return m_uuid; }
    const std::string& username() const { return m_username; }
    const std::string& message() const { return m_message; }
    bool isDebugWorld() const { return m_isDebugWorld; }
    DimensionId dimension() const { return m_dimension; }

    // Setters
    void setSuccess(bool success) { m_success = success; }
    void setPlayerId(PlayerId id) { m_playerId = id; }
    void setEntityId(EntityInstanceId id) { m_entityId = id; }
    void setUuid(const Uuid& uuid) { m_uuid = uuid; }
    void setUsername(const std::string& username) { m_username = username; }
    void setMessage(const std::string& message) { m_message = message; }
    void setIsDebugWorld(bool isDebugWorld) { m_isDebugWorld = isDebugWorld; }
    void setDimension(DimensionId dimension) { m_dimension = dimension; }

    // 序列化
    void serialize(PacketSerializer& ser) const
    {
        ser.writeBool(m_success);
        ser.writeU64(m_playerId);
        ser.writeU64(m_entityId);
        ser.writeBytes(m_uuid.data(), m_uuid.size());
        ser.writeString(m_username);
        ser.writeString(m_message);
        ser.writeBool(m_isDebugWorld);
        ser.writeI32(m_dimension);
    }

    [[nodiscard]] static Result<LoginResponsePacket> deserialize(PacketDeserializer& deser)
    {
        LoginResponsePacket packet;

        auto successResult = deser.readBool();
        if (successResult.failed()) return successResult.error();
        packet.m_success = successResult.value();

        auto playerIdResult = deser.readU64();
        if (playerIdResult.failed()) return playerIdResult.error();
        packet.m_playerId = playerIdResult.value();

        auto entityIdResult = deser.readU64();
        if (entityIdResult.failed()) return entityIdResult.error();
        packet.m_entityId = entityIdResult.value();

        auto uuidResult = deser.readBytes(packet.m_uuid.size());
        if (uuidResult.success()) {
            const auto& bytes = uuidResult.value();
            const size_t copyLen = bytes.size() < packet.m_uuid.size() ? bytes.size() : packet.m_uuid.size();
            std::copy_n(bytes.begin(), copyLen, packet.m_uuid.begin());
        }

        auto usernameResult = deser.readString();
        if (usernameResult.failed()) return usernameResult.error();
        packet.m_username = usernameResult.value();

        auto messageResult = deser.readString();
        if (messageResult.failed()) return messageResult.error();
        packet.m_message = messageResult.value();

        auto debugResult = deser.readBool();
        if (debugResult.success()) {
            packet.m_isDebugWorld = debugResult.value();
        }

        auto dimResult = deser.readI32();
        if (dimResult.success()) {
            packet.m_dimension = dimResult.value();
        }

        return packet;
    }

private:
    bool m_success = false;
    PlayerId m_playerId = 0;
    EntityInstanceId m_entityId = INVALID_ENTITY_ID;
    Uuid m_uuid{};
    std::string m_username;
    std::string m_message;
    bool m_isDebugWorld = false;
    DimensionId m_dimension = 0;
};

// ============================================================================
// 玩家移动包 (客户端 -> 服务端)
// ============================================================================

class PlayerMovePacket {
public:
    enum class MoveType : u8 {
        Full = 0,      // 位置 + 旋转 + 地面
        Position = 1,  // 仅位置 + 地面
        Rotation = 2,  // 仅旋转 + 地面
        GroundOnly = 3 // 仅地面状态
    };

    PlayerMovePacket() = default;
    explicit PlayerMovePacket(const PlayerPosition& pos, MoveType type = MoveType::Full)
        : m_position(pos)
        , m_type(type)
    {}

    // Getters
    const PlayerPosition& position() const { return m_position; }
    MoveType type() const { return m_type; }
    f64 x() const { return m_position.x; }
    f64 y() const { return m_position.y; }
    f64 z() const { return m_position.z; }
    f32 yaw() const { return m_position.yaw; }
    f32 pitch() const { return m_position.pitch; }
    bool onGround() const { return m_position.onGround; }

    // Setters
    void setPosition(const PlayerPosition& pos) { m_position = pos; }
    void setType(MoveType type) { m_type = type; }

    // 序列化
    void serialize(PacketSerializer& ser) const
    {
        ser.writeU8(static_cast<u8>(m_type));

        switch (m_type) {
            case MoveType::Full:
                m_position.serialize(ser);
                break;
            case MoveType::Position:
                ser.writeF64(m_position.x);
                ser.writeF64(m_position.y);
                ser.writeF64(m_position.z);
                ser.writeBool(m_position.onGround);
                break;
            case MoveType::Rotation:
                ser.writeF32(m_position.yaw);
                ser.writeF32(m_position.pitch);
                ser.writeBool(m_position.onGround);
                break;
            case MoveType::GroundOnly:
                ser.writeBool(m_position.onGround);
                break;
        }
    }

    [[nodiscard]] static Result<PlayerMovePacket> deserialize(PacketDeserializer& deser)
    {
        PlayerMovePacket packet;

        auto typeResult = deser.readU8();
        if (typeResult.failed()) return typeResult.error();
        packet.m_type = static_cast<MoveType>(typeResult.value());

        switch (packet.m_type) {
            case MoveType::Full: {
                auto posResult = PlayerPosition::deserialize(deser);
                if (posResult.failed()) return posResult.error();
                packet.m_position = posResult.value();
                break;
            }
            case MoveType::Position: {
                auto xResult = deser.readF64();
                if (xResult.failed()) return xResult.error();
                packet.m_position.x = xResult.value();

                auto yResult = deser.readF64();
                if (yResult.failed()) return yResult.error();
                packet.m_position.y = yResult.value();

                auto zResult = deser.readF64();
                if (zResult.failed()) return zResult.error();
                packet.m_position.z = zResult.value();

                auto groundResult = deser.readBool();
                if (groundResult.failed()) return groundResult.error();
                packet.m_position.onGround = groundResult.value();
                break;
            }
            case MoveType::Rotation: {
                auto yawResult = deser.readF32();
                if (yawResult.failed()) return yawResult.error();
                packet.m_position.yaw = yawResult.value();

                auto pitchResult = deser.readF32();
                if (pitchResult.failed()) return pitchResult.error();
                packet.m_position.pitch = pitchResult.value();

                auto groundResult = deser.readBool();
                if (groundResult.failed()) return groundResult.error();
                packet.m_position.onGround = groundResult.value();
                break;
            }
            case MoveType::GroundOnly: {
                auto groundResult = deser.readBool();
                if (groundResult.failed()) return groundResult.error();
                packet.m_position.onGround = groundResult.value();
                break;
            }
            default:
                return Error(ErrorCode::InvalidData, "Invalid move type");
        }

        return packet;
    }

private:
    PlayerPosition m_position;
    MoveType m_type = MoveType::Full;
};

// ============================================================================
// 方块交互包 (客户端 -> 服务端)
// ============================================================================

enum class BlockInteractionAction : u8 { StartDestroyBlock = 0, AbortDestroyBlock = 1, StopDestroyBlock = 2 };

class BlockInteractionPacket {
public:
    BlockInteractionPacket() = default;
    BlockInteractionPacket(BlockInteractionAction action, i32 x, i32 y, i32 z, Direction face)
        : m_action(action)
        , m_x(x)
        , m_y(y)
        , m_z(z)
        , m_face(face)
    {}

    [[nodiscard]] BlockInteractionAction action() const { return m_action; }
    [[nodiscard]] i32 x() const { return m_x; }
    [[nodiscard]] i32 y() const { return m_y; }
    [[nodiscard]] i32 z() const { return m_z; }
    [[nodiscard]] Direction face() const { return m_face; }

    void serialize(PacketSerializer& ser) const
    {
        ser.writeU8(static_cast<u8>(m_action));
        ser.writeI32(m_x);
        ser.writeI32(m_y);
        ser.writeI32(m_z);
        ser.writeU8(static_cast<u8>(m_face));
    }

    [[nodiscard]] static Result<BlockInteractionPacket> deserialize(PacketDeserializer& deser)
    {
        BlockInteractionPacket packet;

        auto actionResult = deser.readU8();
        if (actionResult.failed()) return actionResult.error();
        packet.m_action = static_cast<BlockInteractionAction>(actionResult.value());

        auto xResult = deser.readI32();
        if (xResult.failed()) return xResult.error();
        packet.m_x = xResult.value();

        auto yResult = deser.readI32();
        if (yResult.failed()) return yResult.error();
        packet.m_y = yResult.value();

        auto zResult = deser.readI32();
        if (zResult.failed()) return zResult.error();
        packet.m_z = zResult.value();

        auto faceResult = deser.readU8();
        if (faceResult.failed()) return faceResult.error();
        packet.m_face = static_cast<Direction>(faceResult.value());

        if (packet.m_action != BlockInteractionAction::StartDestroyBlock &&
            packet.m_action != BlockInteractionAction::AbortDestroyBlock &&
            packet.m_action != BlockInteractionAction::StopDestroyBlock) {
            return Error(ErrorCode::InvalidData, "Invalid block interaction action");
        }

        return packet;
    }

private:
    BlockInteractionAction m_action = BlockInteractionAction::StartDestroyBlock;
    i32 m_x = 0;
    i32 m_y = 0;
    i32 m_z = 0;
    Direction m_face = Direction::None;
};

// ============================================================================
// 方块放置包 (客户端 -> 服务端)
// ============================================================================

/**
 * @brief 玩家尝试在方块上使用物品/放置方块
 *
 * 当玩家右键点击方块时发送。
 * 包含点击位置、面、点击坐标（方块内相对位置）和手持信息。
 */
class PlayerTryUseItemOnBlockPacket {
public:
    PlayerTryUseItemOnBlockPacket() = default;

    /**
     * @brief 构造方块放置包
     * @param x 方块X坐标
     * @param y 方块Y坐标
     * @param z 方块Z坐标
     * @param face 点击的面
     * @param hitX 点击点在方块内的X坐标 (0-1)
     * @param hitY 点击点在方块内的Y坐标 (0-1)
     * @param hitZ 点击点在方块内的Z坐标 (0-1)
     * @param hand 手 (0=主手, 1=副手)
     */
    PlayerTryUseItemOnBlockPacket(i32 x, i32 y, i32 z, Direction face, f32 hitX, f32 hitY, f32 hitZ, u8 hand)
        : m_x(x)
        , m_y(y)
        , m_z(z)
        , m_face(face)
        , m_hitX(hitX)
        , m_hitY(hitY)
        , m_hitZ(hitZ)
        , m_hand(hand)
    {}

    // Getters
    [[nodiscard]] i32 x() const { return m_x; }
    [[nodiscard]] i32 y() const { return m_y; }
    [[nodiscard]] i32 z() const { return m_z; }
    [[nodiscard]] Direction face() const { return m_face; }
    [[nodiscard]] f32 hitX() const { return m_hitX; }
    [[nodiscard]] f32 hitY() const { return m_hitY; }
    [[nodiscard]] f32 hitZ() const { return m_hitZ; }
    [[nodiscard]] u8 hand() const { return m_hand; }

    // Setters
    void setX(i32 x) { m_x = x; }
    void setY(i32 y) { m_y = y; }
    void setZ(i32 z) { m_z = z; }
    void setFace(Direction face) { m_face = face; }
    void setHitX(f32 hitX) { m_hitX = hitX; }
    void setHitY(f32 hitY) { m_hitY = hitY; }
    void setHitZ(f32 hitZ) { m_hitZ = hitZ; }
    void setHand(u8 hand) { m_hand = hand; }

    /**
     * @brief 获取方块位置
     */
    [[nodiscard]] BlockPos blockPos() const { return BlockPos(m_x, m_y, m_z); }

    /**
     * @brief 获取击中点（世界坐标）
     */
    [[nodiscard]] Vector3 hitPosition() const
    {
        return Vector3(static_cast<f32>(m_x) + m_hitX, static_cast<f32>(m_y) + m_hitY, static_cast<f32>(m_z) + m_hitZ);
    }

    // 序列化
    void serialize(PacketSerializer& ser) const
    {
        ser.writeI32(m_x);
        ser.writeI32(m_y);
        ser.writeI32(m_z);
        ser.writeU8(static_cast<u8>(m_face));
        ser.writeF32(m_hitX);
        ser.writeF32(m_hitY);
        ser.writeF32(m_hitZ);
        ser.writeU8(m_hand);
    }

    [[nodiscard]] static Result<PlayerTryUseItemOnBlockPacket> deserialize(PacketDeserializer& deser)
    {
        PlayerTryUseItemOnBlockPacket packet;

        auto xResult = deser.readI32();
        if (xResult.failed()) return xResult.error();
        packet.m_x = xResult.value();

        auto yResult = deser.readI32();
        if (yResult.failed()) return yResult.error();
        packet.m_y = yResult.value();

        auto zResult = deser.readI32();
        if (zResult.failed()) return zResult.error();
        packet.m_z = zResult.value();

        auto faceResult = deser.readU8();
        if (faceResult.failed()) return faceResult.error();
        packet.m_face = static_cast<Direction>(faceResult.value());

        auto hitXResult = deser.readF32();
        if (hitXResult.failed()) return hitXResult.error();
        packet.m_hitX = hitXResult.value();

        auto hitYResult = deser.readF32();
        if (hitYResult.failed()) return hitYResult.error();
        packet.m_hitY = hitYResult.value();

        auto hitZResult = deser.readF32();
        if (hitZResult.failed()) return hitZResult.error();
        packet.m_hitZ = hitZResult.value();

        auto handResult = deser.readU8();
        if (handResult.failed()) return handResult.error();
        packet.m_hand = handResult.value();

        // 验证面方向
        if (packet.m_face != Direction::Down && packet.m_face != Direction::Up && packet.m_face != Direction::North &&
            packet.m_face != Direction::South && packet.m_face != Direction::West && packet.m_face != Direction::East) {
            return Error(ErrorCode::InvalidData, "Invalid face direction for block placement");
        }

        return packet;
    }

private:
    i32 m_x = 0;
    i32 m_y = 0;
    i32 m_z = 0;
    Direction m_face = Direction::None;
    f32 m_hitX = 0.5f;
    f32 m_hitY = 0.5f;
    f32 m_hitZ = 0.5f;
    u8 m_hand = 0; // 0=主手, 1=副手
};

// ============================================================================
// 传送包 (服务端 -> 客户端)
// ============================================================================

class TeleportPacket {
public:
    TeleportPacket() = default;
    TeleportPacket(f64 x, f64 y, f64 z, f32 yaw, f32 pitch, u32 teleportId)
        : m_x(x)
        , m_y(y)
        , m_z(z)
        , m_yaw(yaw)
        , m_pitch(pitch)
        , m_teleportId(teleportId)
    {}

    // Getters
    f64 x() const { return m_x; }
    f64 y() const { return m_y; }
    f64 z() const { return m_z; }
    f32 yaw() const { return m_yaw; }
    f32 pitch() const { return m_pitch; }
    u32 teleportId() const { return m_teleportId; }

    // Setters
    void setX(f64 x) { m_x = x; }
    void setY(f64 y) { m_y = y; }
    void setZ(f64 z) { m_z = z; }
    void setYaw(f32 yaw) { m_yaw = yaw; }
    void setPitch(f32 pitch) { m_pitch = pitch; }
    void setTeleportId(u32 id) { m_teleportId = id; }

    // 序列化
    void serialize(PacketSerializer& ser) const
    {
        ser.writeF64(m_x);
        ser.writeF64(m_y);
        ser.writeF64(m_z);
        ser.writeF32(m_yaw);
        ser.writeF32(m_pitch);
        ser.writeVarUInt(m_teleportId);
    }

    [[nodiscard]] static Result<TeleportPacket> deserialize(PacketDeserializer& deser)
    {
        TeleportPacket packet;

        auto xResult = deser.readF64();
        if (xResult.failed()) return xResult.error();
        packet.m_x = xResult.value();

        auto yResult = deser.readF64();
        if (yResult.failed()) return yResult.error();
        packet.m_y = yResult.value();

        auto zResult = deser.readF64();
        if (zResult.failed()) return zResult.error();
        packet.m_z = zResult.value();

        auto yawResult = deser.readF32();
        if (yawResult.failed()) return yawResult.error();
        packet.m_yaw = yawResult.value();

        auto pitchResult = deser.readF32();
        if (pitchResult.failed()) return pitchResult.error();
        packet.m_pitch = pitchResult.value();

        auto idResult = deser.readVarUInt();
        if (idResult.failed()) return idResult.error();
        packet.m_teleportId = idResult.value();

        return packet;
    }

private:
    f64 m_x = 0.0;
    f64 m_y = 0.0;
    f64 m_z = 0.0;
    f32 m_yaw = 0.0f;
    f32 m_pitch = 0.0f;
    u32 m_teleportId = 0;
};

// ============================================================================
// 传送确认包 (客户端 -> 服务端)
// ============================================================================

class TeleportConfirmPacket {
public:
    TeleportConfirmPacket() = default;
    explicit TeleportConfirmPacket(u32 teleportId)
        : m_teleportId(teleportId)
    {}

    // Getters
    u32 teleportId() const { return m_teleportId; }

    // Setters
    void setTeleportId(u32 id) { m_teleportId = id; }

    // 序列化
    void serialize(PacketSerializer& ser) const { ser.writeVarUInt(m_teleportId); }

    [[nodiscard]] static Result<TeleportConfirmPacket> deserialize(PacketDeserializer& deser)
    {
        TeleportConfirmPacket packet;

        auto idResult = deser.readVarUInt();
        if (idResult.failed()) return idResult.error();
        packet.m_teleportId = idResult.value();

        return packet;
    }

private:
    u32 m_teleportId = 0;
};

// ============================================================================
// Keep-Alive 数据包 (双向) - 用于 ProtocolPackets 内部
// 注意: 这是简化版本，完整的 KeepAlivePacket 见 Packet.hpp
// ============================================================================

class SimpleKeepAlive {
public:
    SimpleKeepAlive() = default;
    explicit SimpleKeepAlive(u64 id)
        : m_id(id)
    {}

    // Getters
    u64 id() const { return m_id; }

    // Setters
    void setId(u64 id) { m_id = id; }

    // 序列化
    void serialize(PacketSerializer& ser) const { ser.writeU64(m_id); }

    [[nodiscard]] static Result<SimpleKeepAlive> deserialize(PacketDeserializer& deser)
    {
        SimpleKeepAlive packet;

        auto idResult = deser.readU64();
        if (idResult.failed()) return idResult.error();
        packet.m_id = idResult.value();

        return packet;
    }

private:
    u64 m_id = 0;
};

// ============================================================================
// 区块数据包 (服务端 -> 客户端)
// ============================================================================

class ChunkDataPacket {
public:
    ChunkDataPacket() = default;
    ChunkDataPacket(ChunkCoord x, ChunkCoord z, DimensionId dimension, std::vector<u8> data)
        : m_x(x)
        , m_z(z)
        , m_dimension(dimension)
        , m_data(std::move(data))
    {}

    // Getters
    ChunkCoord x() const { return m_x; }
    ChunkCoord z() const { return m_z; }
    DimensionId dimension() const { return m_dimension; }
    const std::vector<u8>& data() const { return m_data; }
    size_t size() const { return m_data.size(); }

    // Setters
    void setX(ChunkCoord x) { m_x = x; }
    void setZ(ChunkCoord z) { m_z = z; }
    void setDimension(DimensionId dimension) { m_dimension = dimension; }
    void setData(const std::vector<u8>& data) { m_data = data; }
    void setData(std::vector<u8>&& data) { m_data = std::move(data); }

    // 序列化
    void serialize(PacketSerializer& ser) const
    {
        ser.writeI32(m_x);
        ser.writeI32(m_z);
        ser.writeI32(m_dimension);
        ser.writeVarUInt(static_cast<u32>(m_data.size()));
        ser.writeBytes(m_data);
    }

    [[nodiscard]] static Result<ChunkDataPacket> deserialize(PacketDeserializer& deser)
    {
        MC_TRACE_SCOPED_EVENT(::mc::trace::TraceEvents.Client.Network, "deserializeChunkDataPacket");

        ChunkDataPacket packet;

        auto xResult = deser.readI32();
        if (xResult.failed()) return xResult.error();
        packet.m_x = xResult.value();

        auto zResult = deser.readI32();
        if (zResult.failed()) return zResult.error();
        packet.m_z = zResult.value();

        MC_TRACE_INSTANT_EVENT(::mc::trace::TraceEvents.Client.Network,
            "readChunkCoordinates",
            [flow = ::perfetto::Flow::ProcessScoped(mc::world::chunk::ChunkPos(packet.m_x, packet.m_z).toId())](
                ::perfetto::EventContext ctx) { flow(ctx); });

        auto dimResult = deser.readI32();
        if (dimResult.failed()) return dimResult.error();
        packet.m_dimension = dimResult.value();

        auto sizeResult = deser.readVarUInt();
        if (sizeResult.failed()) return sizeResult.error();
        u32 size = sizeResult.value();

        if (size > protocol::MAX_CHUNK_DATA_SIZE) {
            return Error(ErrorCode::InvalidData, "Chunk data too large");
        }

        auto dataResult = deser.readBytes(size);
        if (dataResult.failed()) return dataResult.error();
        packet.m_data = std::move(dataResult.value());

        return packet;
    }

private:
    ChunkCoord m_x = 0;
    ChunkCoord m_z = 0;
    DimensionId m_dimension = 0; // 主世界=0, 下界=1, 末地=2
    std::vector<u8> m_data;
};

// ============================================================================
// 卸载区块包 (服务端 -> 客户端)
// ============================================================================

class UnloadChunkPacket {
public:
    UnloadChunkPacket() = default;
    UnloadChunkPacket(ChunkCoord x, ChunkCoord z, DimensionId dimension)
        : m_x(x)
        , m_z(z)
        , m_dimension(dimension)
    {}

    // Getters
    ChunkCoord x() const { return m_x; }
    ChunkCoord z() const { return m_z; }
    DimensionId dimension() const { return m_dimension; }

    // Setters
    void setX(ChunkCoord x) { m_x = x; }
    void setZ(ChunkCoord z) { m_z = z; }
    void setDimension(DimensionId dimension) { m_dimension = dimension; }

    // 序列化
    void serialize(PacketSerializer& ser) const
    {
        ser.writeI32(m_x);
        ser.writeI32(m_z);
        ser.writeI32(m_dimension);
    }

    [[nodiscard]] static Result<UnloadChunkPacket> deserialize(PacketDeserializer& deser)
    {
        UnloadChunkPacket packet;

        auto xResult = deser.readI32();
        if (xResult.failed()) return xResult.error();
        packet.m_x = xResult.value();

        auto zResult = deser.readI32();
        if (zResult.failed()) return zResult.error();
        packet.m_z = zResult.value();

        auto dimResult = deser.readI32();
        if (dimResult.failed()) return dimResult.error();
        packet.m_dimension = dimResult.value();

        return packet;
    }

private:
    ChunkCoord m_x = 0;
    ChunkCoord m_z = 0;
    DimensionId m_dimension = 0; // 主世界=0, 下界=1, 末地=2
};

// ============================================================================
// 玩家生成包 (服务端 -> 客户端)
// ============================================================================

class PlayerSpawnPacket {
public:
    PlayerSpawnPacket() = default;
    PlayerSpawnPacket(PlayerId playerId, const std::string& username, const PlayerPosition& pos)
        : m_playerId(playerId)
        , m_username(username)
        , m_position(pos)
    {}

    // Getters
    PlayerId playerId() const { return m_playerId; }
    const std::string& username() const { return m_username; }
    const PlayerPosition& position() const { return m_position; }

    // Setters
    void setPlayerId(PlayerId id) { m_playerId = id; }
    void setUsername(const std::string& username) { m_username = username; }
    void setPosition(const PlayerPosition& pos) { m_position = pos; }

    // 序列化
    void serialize(PacketSerializer& ser) const
    {
        ser.writeU64(m_playerId);
        ser.writeString(m_username);
        m_position.serialize(ser);
    }

    [[nodiscard]] static Result<PlayerSpawnPacket> deserialize(PacketDeserializer& deser)
    {
        PlayerSpawnPacket packet;

        auto idResult = deser.readU64();
        if (idResult.failed()) return idResult.error();
        packet.m_playerId = idResult.value();

        auto usernameResult = deser.readString();
        if (usernameResult.failed()) return usernameResult.error();
        packet.m_username = usernameResult.value();

        auto posResult = PlayerPosition::deserialize(deser);
        if (posResult.failed()) return posResult.error();
        packet.m_position = posResult.value();

        return packet;
    }

private:
    PlayerId m_playerId = 0;
    std::string m_username;
    PlayerPosition m_position;
};

// ============================================================================
// 玩家消失包 (服务端 -> 客户端)
// ============================================================================

class PlayerDespawnPacket {
public:
    PlayerDespawnPacket() = default;
    explicit PlayerDespawnPacket(PlayerId playerId)
        : m_playerId(playerId)
    {}

    // Getters
    PlayerId playerId() const { return m_playerId; }

    // Setters
    void setPlayerId(PlayerId id) { m_playerId = id; }

    // 序列化
    void serialize(PacketSerializer& ser) const { ser.writeU64(m_playerId); }

    [[nodiscard]] static Result<PlayerDespawnPacket> deserialize(PacketDeserializer& deser)
    {
        PlayerDespawnPacket packet;

        auto idResult = deser.readU64();
        if (idResult.failed()) return idResult.error();
        packet.m_playerId = idResult.value();

        return packet;
    }

private:
    PlayerId m_playerId = 0;
};

// ============================================================================
// 方块更新包 (服务端 -> 客户端)
// ============================================================================

class BlockUpdatePacket {
public:
    BlockUpdatePacket() = default;
    BlockUpdatePacket(i32 x, i32 y, i32 z, u32 blockStateId)
        : m_x(x)
        , m_y(y)
        , m_z(z)
        , m_blockStateId(blockStateId)
    {}

    // Getters
    i32 x() const { return m_x; }
    i32 y() const { return m_y; }
    i32 z() const { return m_z; }
    u32 blockStateId() const { return m_blockStateId; }

    // Setters
    void setX(i32 x) { m_x = x; }
    void setY(i32 y) { m_y = y; }
    void setZ(i32 z) { m_z = z; }
    void setBlockStateId(u32 id) { m_blockStateId = id; }

    // 序列化
    void serialize(PacketSerializer& ser) const
    {
        ser.writeI32(m_x);
        ser.writeI32(m_y);
        ser.writeI32(m_z);
        ser.writeVarUInt(m_blockStateId);
    }

    [[nodiscard]] static Result<BlockUpdatePacket> deserialize(PacketDeserializer& deser)
    {
        BlockUpdatePacket packet;

        auto xResult = deser.readI32();
        if (xResult.failed()) return xResult.error();
        packet.m_x = xResult.value();

        auto yResult = deser.readI32();
        if (yResult.failed()) return yResult.error();
        packet.m_y = yResult.value();

        auto zResult = deser.readI32();
        if (zResult.failed()) return zResult.error();
        packet.m_z = zResult.value();

        auto idResult = deser.readVarUInt();
        if (idResult.failed()) return idResult.error();
        packet.m_blockStateId = idResult.value();

        return packet;
    }

private:
    i32 m_x = 0;
    i32 m_y = 0;
    i32 m_z = 0;
    u32 m_blockStateId = 0; // 使用状态ID替代旧的BlockId
};

// ============================================================================
// 聊天消息包 (双向)
// ============================================================================

class ChatMessagePacket {
public:
    ChatMessagePacket() = default;
    ChatMessagePacket(const std::string& message, PlayerId senderId = 0)
        : m_message(message)
        , m_senderId(senderId)
    {}

    // Getters
    const std::string& message() const { return m_message; }
    PlayerId senderId() const { return m_senderId; }

    // Setters
    void setMessage(const std::string& message) { m_message = message; }
    void setSenderId(PlayerId id) { m_senderId = id; }

    // 序列化
    void serialize(PacketSerializer& ser) const
    {
        ser.writeString(m_message);
        ser.writeU64(m_senderId);
    }

    [[nodiscard]] static Result<ChatMessagePacket> deserialize(PacketDeserializer& deser)
    {
        ChatMessagePacket packet;

        auto msgResult = deser.readString();
        if (msgResult.failed()) return msgResult.error();
        packet.m_message = msgResult.value();

        auto idResult = deser.readU64();
        if (idResult.failed()) return idResult.error();
        packet.m_senderId = idResult.value();

        // 验证消息长度
        if (packet.m_message.size() > protocol::MAX_CHAT_LENGTH) {
            return Error(ErrorCode::InvalidData, "Chat message too long");
        }

        return packet;
    }

private:
    std::string m_message;
    PlayerId m_senderId = 0;
};

// ============================================================================
// 时间更新包 (服务端 -> 客户端)
// ============================================================================

class TimeUpdatePacket {
public:
    TimeUpdatePacket() = default;
    TimeUpdatePacket(i64 gameTime, i64 dayTime, bool daylightCycleEnabled)
        : m_gameTime(gameTime)
        , m_dayTime(dayTime)
        , m_daylightCycleEnabled(daylightCycleEnabled)
    {}

    // Getters
    i64 gameTime() const { return m_gameTime; }
    i64 dayTime() const { return m_dayTime; }
    bool daylightCycleEnabled() const { return m_daylightCycleEnabled; }

    // Setters
    void setGameTime(i64 time) { m_gameTime = time; }
    void setDayTime(i64 time) { m_dayTime = time; }
    void setDaylightCycleEnabled(bool enabled) { m_daylightCycleEnabled = enabled; }

    /**
     * @brief 序列化时间更新包
     *
     * 协议格式 (兼容 MC 1.16.5):
     * - gameTime: i64 (游戏启动以来的总 tick 数)
     * - dayTime: i64 (一天内的时间，负数表示日光周期禁用)
     *
     * 注意: MC 协议中，dayTime 为负数时表示日光周期被禁用
     * 实际的 dayTime 值为 |dayTime|
     */
    void serialize(PacketSerializer& ser) const
    {
        ser.writeI64(m_gameTime);
        // MC协议: 负数表示日光周期禁用
        ser.writeI64(m_daylightCycleEnabled ? m_dayTime : -m_dayTime);
    }

    /**
     * @brief 反序列化时间更新包
     * @return 解析结果
     */
    [[nodiscard]] static Result<TimeUpdatePacket> deserialize(PacketDeserializer& deser)
    {
        TimeUpdatePacket packet;

        auto gameTimeResult = deser.readI64();
        if (gameTimeResult.failed()) return gameTimeResult.error();
        packet.m_gameTime = gameTimeResult.value();

        auto dayTimeResult = deser.readI64();
        if (dayTimeResult.failed()) return dayTimeResult.error();
        i64 rawDayTime = dayTimeResult.value();

        // 负数表示日光周期禁用
        if (rawDayTime < 0) {
            packet.m_dayTime = -rawDayTime;
            packet.m_daylightCycleEnabled = false;
        } else {
            packet.m_dayTime = rawDayTime;
            packet.m_daylightCycleEnabled = true;
        }

        return packet;
    }

private:
    i64 m_gameTime = 0;
    i64 m_dayTime = 0;
    bool m_daylightCycleEnabled = true;
};

// ============================================================================
// 光照更新包 (服务端 -> 客户端)
// ============================================================================

/**
 * @brief 光照更新包
 *
 * 当服务端光照计算完成后，向客户端发送光照数据更新。
 * 可以发送整个区块段的光照数据，或仅发送变更标记。
 *
 * 协议格式:
 * - chunkX: i32 区块X坐标
 * - chunkZ: i32 区块Z坐标
 * - sectionY: i32 区块段Y坐标 (可以是负数，如下界)
 * - flags: u8 标志位
 *   - bit 0: 包含天空光照数据
 *   - bit 1: 包含方块光照数据
 *   - bit 2: 信任边缘 (客户端可信任边界光照)
 * - skyLightData: 天空光照数据 (可选，2048字节 = 4096个方块 / 2)
 * - blockLightData: 方块光照数据 (可选，2048字节)
 */
class LightUpdatePacket {
public:
    LightUpdatePacket() = default;

    /**
     * @brief 构造光照更新包
     * @param chunkX 区块X坐标
     * @param chunkZ 区块Z坐标
     * @param sectionY 区块段Y坐标
     * @param skyLight 天空光照数据 (可选)
     * @param blockLight 方块光照数据 (可选)
     * @param trustEdges 是否信任边缘光照
     */
    LightUpdatePacket(i32 chunkX,
        i32 chunkZ,
        i32 sectionY,
        std::vector<u8>&& skyLight,
        std::vector<u8>&& blockLight,
        bool trustEdges = false)
        : m_chunkX(chunkX)
        , m_chunkZ(chunkZ)
        , m_sectionY(sectionY)
        , m_skyLight(std::move(skyLight))
        , m_blockLight(std::move(blockLight))
        , m_trustEdges(trustEdges)
    {}

    // Getters
    i32 chunkX() const { return m_chunkX; }
    i32 chunkZ() const { return m_chunkZ; }
    i32 sectionY() const { return m_sectionY; }
    const std::vector<u8>& skyLight() const { return m_skyLight; }
    const std::vector<u8>& blockLight() const { return m_blockLight; }
    bool hasSkyLight() const { return !m_skyLight.empty(); }
    bool hasBlockLight() const { return !m_blockLight.empty(); }
    bool trustEdges() const { return m_trustEdges; }

    // Setters
    void setChunkX(i32 x) { m_chunkX = x; }
    void setChunkZ(i32 z) { m_chunkZ = z; }
    void setSectionY(i32 y) { m_sectionY = y; }
    void setSkyLight(std::vector<u8>&& data) { m_skyLight = std::move(data); }
    void setBlockLight(std::vector<u8>&& data) { m_blockLight = std::move(data); }
    void setTrustEdges(bool trust) { m_trustEdges = trust; }

    /**
     * @brief 获取区块段位置编码
     * @return 区块段位置 (用于索引光照数据)
     */
    [[nodiscard]] i64 sectionPos() const
    {
        // SectionPos 编码: (x & 0x3FFFFF) << 42 | (y & 0xFFFFF) << 20 | (z & 0x3FFFFF)
        return (static_cast<i64>(m_chunkX) & 0x3FFFFFLL) << 42 | (static_cast<i64>(m_sectionY) & 0xFFFFFLL) << 20 |
            (static_cast<i64>(m_chunkZ) & 0x3FFFFFLL);
    }

    // 序列化
    void serialize(PacketSerializer& ser) const
    {
        ser.writeI32(m_chunkX);
        ser.writeI32(m_chunkZ);
        ser.writeI32(m_sectionY);

        // 标志位
        u8 flags = 0;
        if (!m_skyLight.empty()) flags |= 0x01;
        if (!m_blockLight.empty()) flags |= 0x02;
        if (m_trustEdges) flags |= 0x04;
        ser.writeU8(flags);

        // 天空光照数据
        if (!m_skyLight.empty()) {
            ser.writeVarUInt(static_cast<u32>(m_skyLight.size()));
            ser.writeBytes(m_skyLight);
        }

        // 方块光照数据
        if (!m_blockLight.empty()) {
            ser.writeVarUInt(static_cast<u32>(m_blockLight.size()));
            ser.writeBytes(m_blockLight);
        }
    }

    [[nodiscard]] static Result<LightUpdatePacket> deserialize(PacketDeserializer& deser)
    {
        LightUpdatePacket packet;

        auto xResult = deser.readI32();
        if (xResult.failed()) return xResult.error();
        packet.m_chunkX = xResult.value();

        auto zResult = deser.readI32();
        if (zResult.failed()) return zResult.error();
        packet.m_chunkZ = zResult.value();

        auto yResult = deser.readI32();
        if (yResult.failed()) return yResult.error();
        packet.m_sectionY = yResult.value();

        auto flagsResult = deser.readU8();
        if (flagsResult.failed()) return flagsResult.error();
        u8 flags = flagsResult.value();

        packet.m_trustEdges = (flags & 0x04) != 0;

        // 天空光照数据
        if (flags & 0x01) {
            auto skySizeResult = deser.readVarUInt();
            if (skySizeResult.failed()) return skySizeResult.error();
            u32 skySize = skySizeResult.value();

            if (skySize > 4096) { // 最大 2048 字节 (4096 个 nibble)
                return Error(ErrorCode::InvalidData, "Sky light data too large");
            }

            auto skyDataResult = deser.readBytes(skySize);
            if (skyDataResult.failed()) return skyDataResult.error();
            packet.m_skyLight = std::move(skyDataResult.value());
        }

        // 方块光照数据
        if (flags & 0x02) {
            auto blockSizeResult = deser.readVarUInt();
            if (blockSizeResult.failed()) return blockSizeResult.error();
            u32 blockSize = blockSizeResult.value();

            if (blockSize > 4096) {
                return Error(ErrorCode::InvalidData, "Block light data too large");
            }

            auto blockDataResult = deser.readBytes(blockSize);
            if (blockDataResult.failed()) return blockDataResult.error();
            packet.m_blockLight = std::move(blockDataResult.value());
        }

        return packet;
    }

private:
    i32 m_chunkX = 0;
    i32 m_chunkZ = 0;
    i32 m_sectionY = 0;
    std::vector<u8> m_skyLight;
    std::vector<u8> m_blockLight;
    bool m_trustEdges = false;
};

} // namespace mc::network
