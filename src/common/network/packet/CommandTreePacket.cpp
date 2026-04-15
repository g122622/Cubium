#include "CommandTreePacket.hpp"

#include "PacketSerializer.hpp"
#include <algorithm>

namespace mc::network {

Result<std::vector<u8>> CommandTreePacket::serialize() const {
    PacketSerializer serializer;
    const size_t contentSize = sizeof(u16) + std::min(m_treeJson.size(), static_cast<size_t>(MAX_STRING_LENGTH));

    serializer.writeU32(static_cast<u32>(PACKET_HEADER_SIZE + contentSize));
    serializer.writeU16(static_cast<u16>(m_type));
    serializer.writeU16(m_flags);
    serializer.writeU16(0);
    serializer.writeU16(0);
    serializer.writeString(m_treeJson);

    return serializer.buffer();
}

Result<void> CommandTreePacket::deserialize(const u8* data, size_t size) {
    if (size < PACKET_HEADER_SIZE) {
        return Error(ErrorCode::InvalidArgument, "Packet too small for command tree");
    }

    PacketDeserializer deserializer(data, size);

    (void)deserializer.readU32();
    (void)deserializer.readU16();
    auto flagsResult = deserializer.readU16();
    if (flagsResult.success()) {
        m_flags = flagsResult.value();
    }
    (void)deserializer.readU16();
    (void)deserializer.readU16();

    auto jsonResult = deserializer.readString();
    if (jsonResult.failed()) {
        return jsonResult.error();
    }

    m_treeJson = jsonResult.value();
    return Result<void>::ok();
}

} // namespace mc::network
