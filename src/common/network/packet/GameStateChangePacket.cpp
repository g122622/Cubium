#include "GameStateChangePacket.hpp"
#include "PacketSerializer.hpp"

namespace mc::network {

GameStateChangePacket::GameStateChangePacket()
    : Packet(PacketType::GameStateChange)
{}

GameStateChangePacket::GameStateChangePacket(GameStateChangeReason reason, f32 value)
    : Packet(PacketType::GameStateChange)
    , m_reason(reason)
    , m_value(value)
{}

Result<std::vector<u8>> GameStateChangePacket::serialize() const
{
    PacketSerializer serializer(expectedSize());

    serializer.writeU8(static_cast<u8>(m_reason));
    serializer.writeF32(m_value);

    std::vector<u8> result;
    result.insert(result.end(), serializer.data(), serializer.data() + serializer.size());
    return result;
}

Result<void> GameStateChangePacket::deserialize(const u8* data, size_t size)
{
    if (size < expectedSize()) {
        return Error(ErrorCode::InvalidData, "GameStateChangePacket: insufficient data");
    }

    PacketDeserializer deserializer(data, size);

    auto reasonResult = deserializer.readU8();
    if (!reasonResult.success()) {
        return reasonResult.error();
    }
    m_reason = static_cast<GameStateChangeReason>(reasonResult.value());

    auto valueResult = deserializer.readF32();
    if (!valueResult.success()) {
        return valueResult.error();
    }
    m_value = valueResult.value();

    return {};
}

size_t GameStateChangePacket::expectedSize() const
{
    // 1 byte reason + 4 bytes float
    return 5;
}

} // namespace mc::network
