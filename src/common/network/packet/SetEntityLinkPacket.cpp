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

#include "SetEntityLinkPacket.hpp"
#include "PacketSerializer.hpp"

namespace mc::network {

SetEntityLinkPacket::SetEntityLinkPacket()
    : Packet(PacketType::SetEntityLink)
{}

SetEntityLinkPacket::SetEntityLinkPacket(u32 entityId, u32 linkedEntityId)
    : Packet(PacketType::SetEntityLink)
    , m_entityId(entityId)
    , m_linkedEntityId(linkedEntityId)
{}

Result<std::vector<u8>> SetEntityLinkPacket::serialize() const
{
    PacketSerializer ser;
    ser.writeVarInt(static_cast<i32>(m_entityId));
    ser.writeVarInt(static_cast<i32>(m_linkedEntityId));
    return ser.buffer();
}

Result<void> SetEntityLinkPacket::deserialize(const u8* data, size_t size)
{
    PacketDeserializer deser(data, size);

    auto entityIdResult = deser.readVarInt();
    if (entityIdResult.failed()) {
        return entityIdResult.error();
    }
    m_entityId = static_cast<u32>(entityIdResult.value());

    auto linkedIdResult = deser.readVarInt();
    if (linkedIdResult.failed()) {
        return linkedIdResult.error();
    }
    m_linkedEntityId = static_cast<u32>(linkedIdResult.value());

    return Result<void>::ok();
}

} // namespace mc::network
