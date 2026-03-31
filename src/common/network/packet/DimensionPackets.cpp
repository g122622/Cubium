#include "DimensionPackets.hpp"
#include "PacketSerializer.hpp"
#include <algorithm>

namespace mc::network {

// ============================================================================
// ChangeDimensionPacket
// ============================================================================

ChangeDimensionPacket::ChangeDimensionPacket()
    : Packet(PacketType::ChangeDimension)
{
}

Result<std::vector<u8>> ChangeDimensionPacket::serialize() const {
    PacketSerializer ser;

    // 写入包头
    ser.writeU16(static_cast<u16>(type()));

    // 写入数据
    ser.writeI32(m_dimension);
    ser.writeF64(m_position.x);
    ser.writeF64(m_position.y);
    ser.writeF64(m_position.z);
    ser.writeBool(m_respawn);

    std::vector<u8> result;
    result.insert(result.end(), ser.data(), ser.data() + ser.size());
    return result;
}

Result<void> ChangeDimensionPacket::deserialize(const u8* data, size_t size) {
    PacketDeserializer deser(data, size);

    // 跳过包头 (类型已在外部读取)
    auto typeResult = deser.readU16();
    if (typeResult.failed()) {
        return typeResult.error();
    }

    // 读取数据
    auto dimResult = deser.readI32();
    if (dimResult.failed()) {
        return dimResult.error();
    }
    m_dimension = dimResult.value();

    auto xResult = deser.readF64();
    if (xResult.failed()) {
        return xResult.error();
    }
    m_position.x = xResult.value();

    auto yResult = deser.readF64();
    if (yResult.failed()) {
        return yResult.error();
    }
    m_position.y = yResult.value();

    auto zResult = deser.readF64();
    if (zResult.failed()) {
        return zResult.error();
    }
    m_position.z = zResult.value();

    auto respawnResult = deser.readBool();
    if (respawnResult.failed()) {
        return respawnResult.error();
    }
    m_respawn = respawnResult.value();

    return {};
}

size_t ChangeDimensionPacket::expectedSize() const {
    return sizeof(PacketHeader) + sizeof(i32) + sizeof(f64) * 3 + sizeof(bool);
}

// ============================================================================
// RespawnPacket
// ============================================================================

RespawnPacket::RespawnPacket()
    : Packet(PacketType::Respawn)
{
}

Result<std::vector<u8>> RespawnPacket::serialize() const {
    PacketSerializer ser;

    // 写入包头
    ser.writeU16(static_cast<u16>(type()));

    // 写入数据
    ser.writeI32(m_dimension);
    ser.writeF64(m_position.x);
    ser.writeF64(m_position.y);
    ser.writeF64(m_position.z);
    ser.writeF32(m_yaw);
    ser.writeF32(m_pitch);
    ser.writeU8(static_cast<u8>(m_gameMode));
    ser.writeU8(static_cast<u8>(m_previousGameMode));
    ser.writeBool(m_isDebug);
    ser.writeBool(m_isFlat);
    ser.writeBool(m_copyMetadata);

    std::vector<u8> result;
    result.insert(result.end(), ser.data(), ser.data() + ser.size());
    return result;
}

Result<void> RespawnPacket::deserialize(const u8* data, size_t size) {
    PacketDeserializer deser(data, size);

    // 跳过包头
    auto typeResult = deser.readU16();
    if (typeResult.failed()) {
        return typeResult.error();
    }

    // 读取数据
    auto dimResult = deser.readI32();
    if (dimResult.failed()) {
        return dimResult.error();
    }
    m_dimension = dimResult.value();

    auto xResult = deser.readF64();
    if (xResult.failed()) {
        return xResult.error();
    }
    m_position.x = xResult.value();

    auto yResult = deser.readF64();
    if (yResult.failed()) {
        return yResult.error();
    }
    m_position.y = yResult.value();

    auto zResult = deser.readF64();
    if (zResult.failed()) {
        return zResult.error();
    }
    m_position.z = zResult.value();

    auto yawResult = deser.readF32();
    if (yawResult.failed()) {
        return yawResult.error();
    }
    m_yaw = yawResult.value();

    auto pitchResult = deser.readF32();
    if (pitchResult.failed()) {
        return pitchResult.error();
    }
    m_pitch = pitchResult.value();

    auto gameModeResult = deser.readU8();
    if (gameModeResult.failed()) {
        return gameModeResult.error();
    }
    m_gameMode = static_cast<GameMode>(gameModeResult.value());

    auto prevGameModeResult = deser.readU8();
    if (prevGameModeResult.failed()) {
        return prevGameModeResult.error();
    }
    m_previousGameMode = static_cast<GameMode>(prevGameModeResult.value());

    auto debugResult = deser.readBool();
    if (debugResult.failed()) {
        return debugResult.error();
    }
    m_isDebug = debugResult.value();

    auto flatResult = deser.readBool();
    if (flatResult.failed()) {
        return flatResult.error();
    }
    m_isFlat = flatResult.value();

    auto copyResult = deser.readBool();
    if (copyResult.failed()) {
        return copyResult.error();
    }
    m_copyMetadata = copyResult.value();

    return {};
}

size_t RespawnPacket::expectedSize() const {
    return sizeof(PacketHeader) +
           sizeof(i32) +           // dimension
           sizeof(f64) * 3 +       // position
           sizeof(f32) * 2 +       // yaw, pitch
           sizeof(u8) * 2 +        // gameMode, previousGameMode
           sizeof(bool) * 3;       // isDebug, isFlat, copyMetadata
}

// ============================================================================
// DimensionInfoPacket
// ============================================================================

DimensionInfoPacket::DimensionInfoPacket()
    : Packet(PacketType::DimensionInfo)
{
}

Result<std::vector<u8>> DimensionInfoPacket::serialize() const {
    PacketSerializer ser;

    // 写入包头
    ser.writeU16(static_cast<u16>(type()));

    // 写入维度数量
    ser.writeVarUInt(static_cast<u32>(m_dimensions.size()));

    // 写入每个维度信息
    for (const auto& dim : m_dimensions) {
        ser.writeI32(dim.id);
        ser.writeString(dim.name);
        ser.writeBool(dim.hasSkyLight);
        ser.writeBool(dim.hasCeiling);
        ser.writeF32(dim.ambientLight);
    }

    std::vector<u8> result;
    result.insert(result.end(), ser.data(), ser.data() + ser.size());
    return result;
}

Result<void> DimensionInfoPacket::deserialize(const u8* data, size_t size) {
    PacketDeserializer deser(data, size);

    // 跳过包头
    auto typeResult = deser.readU16();
    if (typeResult.failed()) {
        return typeResult.error();
    }

    // 读取维度数量
    auto countResult = deser.readVarUInt();
    if (countResult.failed()) {
        return countResult.error();
    }
    u32 count = countResult.value();

    m_dimensions.clear();
    m_dimensions.reserve(count);

    // 读取每个维度信息
    for (u32 i = 0; i < count; ++i) {
        DimensionInfo info;

        auto idResult = deser.readI32();
        if (idResult.failed()) {
            return idResult.error();
        }
        info.id = idResult.value();

        auto nameResult = deser.readString();
        if (nameResult.failed()) {
            return nameResult.error();
        }
        info.name = std::move(nameResult.value());

        auto skyResult = deser.readBool();
        if (skyResult.failed()) {
            return skyResult.error();
        }
        info.hasSkyLight = skyResult.value();

        auto ceilResult = deser.readBool();
        if (ceilResult.failed()) {
            return ceilResult.error();
        }
        info.hasCeiling = ceilResult.value();

        auto lightResult = deser.readF32();
        if (lightResult.failed()) {
            return lightResult.error();
        }
        info.ambientLight = lightResult.value();

        m_dimensions.push_back(std::move(info));
    }

    return {};
}

size_t DimensionInfoPacket::expectedSize() const {
    size_t size = sizeof(PacketHeader) + sizeof(u32);
    for (const auto& dim : m_dimensions) {
        size += sizeof(i32) + dim.name.size() + sizeof(bool) * 2 + sizeof(f32);
    }
    return size;
}

// ============================================================================
// ConfirmDimensionChangePacket
// ============================================================================

ConfirmDimensionChangePacket::ConfirmDimensionChangePacket()
    : Packet(PacketType::ConfirmDimensionChange)
{
}

Result<std::vector<u8>> ConfirmDimensionChangePacket::serialize() const {
    PacketSerializer ser;

    // 写入包头
    ser.writeU16(static_cast<u16>(type()));

    // 写入数据
    ser.writeI32(m_dimension);

    std::vector<u8> result;
    result.insert(result.end(), ser.data(), ser.data() + ser.size());
    return result;
}

Result<void> ConfirmDimensionChangePacket::deserialize(const u8* data, size_t size) {
    PacketDeserializer deser(data, size);

    // 跳过包头
    auto typeResult = deser.readU16();
    if (typeResult.failed()) {
        return typeResult.error();
    }

    // 读取数据
    auto dimResult = deser.readI32();
    if (dimResult.failed()) {
        return dimResult.error();
    }
    m_dimension = dimResult.value();

    return {};
}

size_t ConfirmDimensionChangePacket::expectedSize() const {
    return sizeof(PacketHeader) + sizeof(i32);
}

} // namespace mc::network
