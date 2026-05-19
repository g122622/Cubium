/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

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
