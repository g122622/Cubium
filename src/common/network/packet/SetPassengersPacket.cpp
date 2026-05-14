#include "SetPassengersPacket.hpp"
#include "PacketSerializer.hpp"

namespace mc::network {

SetPassengersPacket::SetPassengersPacket()
    : Packet(PacketType::SetPassengers)
{}

SetPassengersPacket::SetPassengersPacket(u32 entityId, const std::vector<u32>& passengerIds)
    : Packet(PacketType::SetPassengers)
    , m_entityId(entityId)
    , m_passengerIds(passengerIds)
{}

Result<std::vector<u8>> SetPassengersPacket::serialize() const
{
    PacketSerializer ser;
    ser.writeVarInt(static_cast<i32>(m_entityId));
    ser.writeVarInt(static_cast<i32>(m_passengerIds.size()));
    for (u32 passengerId : m_passengerIds) {
        ser.writeVarInt(static_cast<i32>(passengerId));
    }
    return ser.buffer();
}

Result<void> SetPassengersPacket::deserialize(const u8* data, size_t size)
{
    PacketDeserializer deser(data, size);

    auto entityIdResult = deser.readVarInt();
    if (entityIdResult.failed()) {
        return entityIdResult.error();
    }
    m_entityId = static_cast<u32>(entityIdResult.value());

    auto countResult = deser.readVarInt();
    if (countResult.failed()) {
        return countResult.error();
    }
    i32 count = countResult.value();

    m_passengerIds.clear();
    m_passengerIds.reserve(static_cast<size_t>(count));
    for (i32 i = 0; i < count; ++i) {
        auto passengerResult = deser.readVarInt();
        if (passengerResult.failed()) {
            return passengerResult.error();
        }
        m_passengerIds.push_back(static_cast<u32>(passengerResult.value()));
    }

    return Result<void>::ok();
}

} // namespace mc::network
