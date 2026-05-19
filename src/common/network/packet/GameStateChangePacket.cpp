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
