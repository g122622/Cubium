#include "BlockBreakAnimPacket.hpp"
#include "PacketSerializer.hpp"

namespace mc {
namespace network {

BlockBreakAnimPacket::BlockBreakAnimPacket()
    : Packet(PacketType::BlockBreakAnim)
{}

size_t BlockBreakAnimPacket::expectedSize() const
{
    // VarInt (breakerId, 最多5字节)
    // Position (x, y, z 各 i32 = 12字节)
    // Byte (stage = 1字节)
    // 总计最多 18 字节
    return 18;
}

Result<std::vector<u8>> BlockBreakAnimPacket::serialize() const
{
    PacketSerializer serializer(expectedSize());

    // 写入挖掘者实体ID (VarInt)
    serializer.writeVarInt(static_cast<i32>(m_breakerEntityId));

    // 写入方块位置
    serializer.writeI32(m_position.x);
    serializer.writeI32(m_position.y);
    serializer.writeI32(m_position.z);

    // 写入破坏阶段
    serializer.writeI8(m_stage);

    std::vector<u8> result;
    result.insert(result.end(), serializer.data(), serializer.data() + serializer.size());
    return result;
}

Result<void> BlockBreakAnimPacket::deserialize(const u8* data, size_t size)
{
    PacketDeserializer deserializer(data, size);

    // 读取挖掘者实体ID
    auto idResult = deserializer.readVarInt();
    if (!idResult.success()) {
        return idResult.error();
    }
    m_breakerEntityId = static_cast<EntityId>(idResult.value());

    // 读取方块位置
    auto xResult = deserializer.readI32();
    if (!xResult.success()) {
        return xResult.error();
    }
    m_position.x = xResult.value();

    auto yResult = deserializer.readI32();
    if (!yResult.success()) {
        return yResult.error();
    }
    m_position.y = yResult.value();

    auto zResult = deserializer.readI32();
    if (!zResult.success()) {
        return zResult.error();
    }
    m_position.z = zResult.value();

    // 读取破坏阶段
    auto stageResult = deserializer.readI8();
    if (!stageResult.success()) {
        return stageResult.error();
    }
    m_stage = stageResult.value();

    return {};
}

} // namespace network
} // namespace mc
