#include "DimensionPackets.hpp"
#include "PacketSerializer.hpp"
#include <algorithm>

namespace mc::network {

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

    // 参考 MC 1.16.5 SRespawnPacket 格式:
    // 1. DimensionType (使用 VarInt 编码的类型 ID)
    // 2. RegistryKey<World> (维度名称: minecraft:overworld/nether/the_end)
    // 3. hashedSeed (long)
    // 4. gameType (ubyte)
    // 5. previousGameType (ubyte)
    // 6. isDebug (boolean)
    // 7. isFlat (boolean)
    // 8. copyMetadata (boolean)

    // 维度类型 (简化: 使用维度ID作为类型ID)
    // MC 1.16.5 维度类型 ID:
    //   0 = minecraft:overworld
    //   1 = minecraft:the_nether
    //   2 = minecraft:the_end
    // 注意：这与维度 ID 不同（下界类型ID是1，但维度ID是-1）
    i32 dimensionTypeId = m_dimensionType;
    if (dimensionTypeId == 0) {
        dimensionTypeId = 0;  // Overworld
    } else if (dimensionTypeId == -1 || dimensionTypeId == 1) {
        // 下界维度 ID=-1, 类型 ID=1
        // 末地维度 ID=1, 类型 ID=2
        dimensionTypeId = (m_dimensionType == -1) ? 1 : 2;
    }
    ser.writeVarInt(dimensionTypeId);

    // 维度名称
    const char* dimensionName = "minecraft:overworld";
    switch (m_dimension) {
        case 0:
            dimensionName = "minecraft:overworld";
            break;
        case -1:
            dimensionName = "minecraft:the_nether";
            break;
        case 1:
            dimensionName = "minecraft:the_end";
            break;
        default:
            dimensionName = "minecraft:overworld";
            break;
    }
    ser.writeString(std::string(dimensionName));

    // 世界种子哈希
    ser.writeU64(m_hashedSeed);

    // 游戏模式
    ser.writeU8(static_cast<u8>(m_gameMode));
    ser.writeU8(static_cast<u8>(m_previousGameMode));

    // 世界标志
    ser.writeBool(m_isDebug);
    ser.writeBool(m_isFlat);
    ser.writeBool(m_keepData);

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

    // 读取维度类型
    auto dimTypeResult = deser.readVarInt();
    if (dimTypeResult.failed()) {
        return dimTypeResult.error();
    }
    m_dimensionType = dimTypeResult.value();

    // 读取维度名称
    auto nameResult = deser.readString();
    if (nameResult.failed()) {
        return nameResult.error();
    }
    // 解析维度名称到维度 ID
    const std::string& name = nameResult.value();
    if (name == "minecraft:overworld" || name == "overworld") {
        m_dimension = 0;
    } else if (name == "minecraft:the_nether" || name == "the_nether") {
        m_dimension = -1;
    } else if (name == "minecraft:the_end" || name == "the_end") {
        m_dimension = 1;
    } else {
        m_dimension = 0;  // 默认主世界
    }

    // 读取种子哈希
    auto seedResult = deser.readU64();
    if (seedResult.failed()) {
        return seedResult.error();
    }
    m_hashedSeed = seedResult.value();

    // 读取游戏模式
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

    // 读取世界标志
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

    auto keepResult = deser.readBool();
    if (keepResult.failed()) {
        return keepResult.error();
    }
    m_keepData = keepResult.value();

    return {};
}

size_t RespawnPacket::expectedSize() const {
    // VarInt + 字符串 + u64 + 3*u8 + 3*bool
    // 保守估计
    return sizeof(PacketHeader) + 5 + 32 + 8 + 3 + 3;
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
