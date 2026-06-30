#include "BlockEventPacket.hpp"
#include "PacketSerializer.hpp"

namespace mc {
namespace network {

BlockEventPacket::BlockEventPacket()
    : Packet(PacketType::BlockEvent)
{}

size_t BlockEventPacket::expectedSize() const
{
    // Position (x/y/z 各 i32 = 12 字节) + Byte (paramA = 1 字节)
    // + Byte (paramB = 1 字节) + VarInt (blockStateId, 最大 5 字节) = 19 字节
    return 19;
}

Result<std::vector<u8>> BlockEventPacket::serialize() const
{
    PacketSerializer serializer(expectedSize());
    serializer.writeI32(m_position.x);
    serializer.writeI32(m_position.y);
    serializer.writeI32(m_position.z);
    serializer.writeU8(m_paramA);
    serializer.writeU8(m_paramB);
    serializer.writeVarInt(static_cast<i32>(m_blockStateId));

    std::vector<u8> result;
    result.insert(result.end(), serializer.data(), serializer.data() + serializer.size());
    return result;
}

Result<void> BlockEventPacket::deserialize(const u8* data, size_t size)
{
    PacketDeserializer deserializer(data, size);

    auto xResult = deserializer.readI32();
    if (!xResult.success()) return xResult.error();
    m_position.x = xResult.value();

    auto yResult = deserializer.readI32();
    if (!yResult.success()) return yResult.error();
    m_position.y = yResult.value();

    auto zResult = deserializer.readI32();
    if (!zResult.success()) return zResult.error();
    m_position.z = zResult.value();

    auto paramAResult = deserializer.readU8();
    if (!paramAResult.success()) return paramAResult.error();
    m_paramA = paramAResult.value();

    auto paramBResult = deserializer.readU8();
    if (!paramBResult.success()) return paramBResult.error();
    m_paramB = paramBResult.value();

    auto blockStateIdResult = deserializer.readVarInt();
    if (!blockStateIdResult.success()) return blockStateIdResult.error();
    m_blockStateId = static_cast<u32>(blockStateIdResult.value());

    return {};
}

} // namespace network
} // namespace mc
