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

#include "Packet.hpp"
#include "PacketSerializer.hpp"

namespace mc::network {

// ============================================================================
// Packet 基类实现
// ============================================================================

Packet::Packet(PacketType type)
    : m_type(type)
{}

// ============================================================================
// KeepAlivePacket 实现
// ============================================================================

Result<std::vector<u8>> KeepAlivePacket::serialize() const
{
    PacketSerializer serializer;
    serializer.writeU32(static_cast<u32>(PACKET_HEADER_SIZE + sizeof(u64))); // size
    serializer.writeU16(static_cast<u16>(m_type));                           // type
    serializer.writeU16(m_flags);                                            // flags
    serializer.writeU16(0);                                                  // reserved
    serializer.writeU16(0);                                                  // padding (确保头部为12字节)
    serializer.writeU64(m_timestamp);                                        // timestamp
    return serializer.buffer();
}

Result<void> KeepAlivePacket::deserialize(const u8* data, size_t size)
{
    if (size < PACKET_HEADER_SIZE + sizeof(u64)) {
        return Error(ErrorCode::InvalidArgument, "Packet too small for keep alive");
    }

    PacketDeserializer deserializer(data, size);

    // 读取头部
    (void)deserializer.readU32();              // size
    (void)deserializer.readU16();              // type (可以忽略，因为我们知道包类型)
    auto flagsResult = deserializer.readU16(); // flags
    if (flagsResult.success()) {
        m_flags = flagsResult.value();
    }
    (void)deserializer.readU16(); // reserved
    (void)deserializer.readU16(); // padding

    auto timestampResult = deserializer.readU64();
    if (timestampResult.failed()) {
        return timestampResult.error();
    }
    m_timestamp = timestampResult.value();

    return Result<void>::ok();
}

// ============================================================================
// DisconnectPacket 实现
// ============================================================================

Result<std::vector<u8>> DisconnectPacket::serialize() const
{
    PacketSerializer serializer;

    // VarInt 编码的字符串长度（1-3字节）
    const size_t strSize = std::min(m_reason.size(), static_cast<size_t>(MAX_STRING_LENGTH));
    const size_t varIntSize = (strSize < 128) ? 1 : (strSize < 16384) ? 2 : 3;
    const size_t contentSize = varIntSize + strSize;

    serializer.writeU32(static_cast<u32>(PACKET_HEADER_SIZE + contentSize)); // size
    serializer.writeU16(static_cast<u16>(m_type));                           // type
    serializer.writeU16(m_flags);                                            // flags
    serializer.writeU16(0);                                                  // reserved
    serializer.writeU16(0);                                                  // padding

    // 写入断开原因
    serializer.writeString(m_reason);

    return serializer.buffer();
}

Result<void> DisconnectPacket::deserialize(const u8* data, size_t size)
{
    if (size < PACKET_HEADER_SIZE) {
        return Error(ErrorCode::InvalidArgument, "Packet too small for disconnect");
    }

    PacketDeserializer deserializer(data, size);

    // 读取头部
    (void)deserializer.readU32();              // size
    (void)deserializer.readU16();              // type
    auto flagsResult = deserializer.readU16(); // flags
    if (flagsResult.success()) {
        m_flags = flagsResult.value();
    }
    (void)deserializer.readU16(); // reserved
    (void)deserializer.readU16(); // padding

    // 读取断开原因
    auto reasonResult = deserializer.readString();
    if (reasonResult.failed()) {
        return reasonResult.error();
    }
    m_reason = reasonResult.value();

    return Result<void>::ok();
}

} // namespace mc::network
