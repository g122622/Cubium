#include "CommandTreePacket.hpp"

#include "PacketSerializer.hpp"
#include <algorithm>

namespace mc::network {

Result<std::vector<u8>> CommandTreePacket::serialize() const {
    const size_t contentSize = sizeof(u16) + std::min(m_treeJson.size(), static_cast<size_t>(MAX_STRING_LENGTH));
    PacketSerializer serializer(contentSize);
    serializer.writeString(m_treeJson);

    return serializer.buffer();
}

Result<void> CommandTreePacket::deserialize(const u8* data, size_t size) {
    if (size < sizeof(u16)) {
        return Error(ErrorCode::InvalidArgument, "Packet too small for command tree payload");
    }

    PacketDeserializer deserializer(data, size);

    auto jsonResult = deserializer.readString();
    if (jsonResult.failed()) {
        return jsonResult.error();
    }

    m_treeJson = jsonResult.value();
    return Result<void>::ok();
}

} // namespace mc::network
