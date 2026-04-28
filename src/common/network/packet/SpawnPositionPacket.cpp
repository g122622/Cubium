#include "SpawnPositionPacket.hpp"
#include "PacketSerializer.hpp"

namespace mc::network {

SpawnPositionPacket::SpawnPositionPacket()
    : Packet(PacketType::SpawnPosition)
    , m_position(0, 0, 0) {
}

SpawnPositionPacket::SpawnPositionPacket(const BlockPos& pos)
    : Packet(PacketType::SpawnPosition)
    , m_position(pos) {
}

Result<std::vector<u8>> SpawnPositionPacket::serialize() const {
    PacketSerializer ser;

    // 写入出生点坐标 (MC 1.16.5 协议格式)
    ser.writeI32(m_position.x);
    ser.writeI32(m_position.y);
    ser.writeI32(m_position.z);

    return ser.buffer();
}

Result<void> SpawnPositionPacket::deserialize(const u8* data, size_t size) {
    PacketDeserializer deser(data, size);

    // 读取出生点坐标
    auto xResult = deser.readI32();
    if (xResult.failed()) {
        return xResult.error();
    }

    auto yResult = deser.readI32();
    if (yResult.failed()) {
        return yResult.error();
    }

    auto zResult = deser.readI32();
    if (zResult.failed()) {
        return zResult.error();
    }

    m_position = BlockPos(xResult.value(), yResult.value(), zResult.value());

    return Result<void>::ok();
}

size_t SpawnPositionPacket::expectedSize() const {
    // 12字节包头 + 12字节数据 (3个i32)
    return sizeof(PacketHeader) + sizeof(i32) * 3;
}

} // namespace mc::network
