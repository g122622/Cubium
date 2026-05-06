#include "CommandTreePacket.hpp"

#include "PacketSerializer.hpp"
#include <algorithm>

namespace mc::network {

Result<std::vector<u8>> CommandTreePacket::serialize() const {
    // 使用 VarInt 编码长度，需要计算实际编码字节数
    const size_t jsonSize = std::min(m_treeJson.size(), static_cast<size_t>(MAX_STRING_LENGTH));
    const size_t varIntSize = (jsonSize < 128) ? 1 : (jsonSize < 16384) ? 2 : 3;

    PacketSerializer serializer(varIntSize + jsonSize);
    serializer.writeString(m_treeJson);

    return serializer.buffer();
}

Result<void> CommandTreePacket::deserialize(const u8* data, size_t size) {
    if (size == 0) {
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
