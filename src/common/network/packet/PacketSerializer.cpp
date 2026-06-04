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

#include "PacketSerializer.hpp"

#include <algorithm>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <arpa/inet.h>
#endif

namespace mc::network {

// ============================================================================
// NetworkEndian 实现
// ============================================================================

u16 NetworkEndian::hostToNetwork16(u16 value)
{
    return static_cast<u16>(htons(value));
}

u32 NetworkEndian::hostToNetwork32(u32 value)
{
    return static_cast<u32>(htonl(value));
}

u64 NetworkEndian::hostToNetwork64(u64 value)
{
    // 假设小端序系统 (x86/x64)
    // 将小端序转换为大端序 (网络字节序)
    return ((value & 0xFF00000000000000ULL) >> 56) | ((value & 0x00FF000000000000ULL) >> 40) |
        ((value & 0x0000FF0000000000ULL) >> 24) | ((value & 0x000000FF00000000ULL) >> 8) |
        ((value & 0x00000000FF000000ULL) << 8) | ((value & 0x0000000000FF0000ULL) << 24) |
        ((value & 0x000000000000FF00ULL) << 40) | ((value & 0x00000000000000FFULL) << 56);
}

u16 NetworkEndian::networkToHost16(u16 value)
{
    return static_cast<u16>(ntohs(value));
}

u32 NetworkEndian::networkToHost32(u32 value)
{
    return static_cast<u32>(ntohl(value));
}

u64 NetworkEndian::networkToHost64(u64 value)
{
    // 与hostToNetwork64相同 (转换是对称的)
    return hostToNetwork64(value);
}

// ============================================================================
// PacketSerializer 实现
// ============================================================================

PacketSerializer::PacketSerializer()
    : m_buffer()
{
    m_buffer.reserve(256);
}

PacketSerializer::PacketSerializer(size_t initialCapacity)
    : m_buffer()
{
    m_buffer.reserve(initialCapacity);
}

PacketSerializer::PacketSerializer(PacketSerializer&& other) noexcept
    : m_buffer(std::move(other.m_buffer))
    , m_readPos(other.m_readPos)
{
    other.m_readPos = 0;
}

PacketSerializer& PacketSerializer::operator=(PacketSerializer&& other) noexcept
{
    if (this != &other) {
        m_buffer = std::move(other.m_buffer);
        m_readPos = other.m_readPos;
        other.m_readPos = 0;
    }
    return *this;
}

void PacketSerializer::writeU8(u8 value)
{
    m_buffer.push_back(value);
}

void PacketSerializer::writeU16(u16 value)
{
    u16 netValue = NetworkEndian::hostToNetwork16(value);
    const u8* bytes = reinterpret_cast<const u8*>(&netValue);
    m_buffer.insert(m_buffer.end(), bytes, bytes + 2);
}

void PacketSerializer::writeU32(u32 value)
{
    u32 netValue = NetworkEndian::hostToNetwork32(value);
    const u8* bytes = reinterpret_cast<const u8*>(&netValue);
    m_buffer.insert(m_buffer.end(), bytes, bytes + 4);
}

void PacketSerializer::writeU64(u64 value)
{
    u64 netValue = NetworkEndian::hostToNetwork64(value);
    const u8* bytes = reinterpret_cast<const u8*>(&netValue);
    m_buffer.insert(m_buffer.end(), bytes, bytes + 8);
}

void PacketSerializer::writeI8(i8 value)
{
    writeU8(static_cast<u8>(value));
}

void PacketSerializer::writeI16(i16 value)
{
    writeU16(static_cast<u16>(value));
}

void PacketSerializer::writeI32(i32 value)
{
    writeU32(static_cast<u32>(value));
}

void PacketSerializer::writeI64(i64 value)
{
    writeU64(static_cast<u64>(value));
}

void PacketSerializer::writeF32(f32 value)
{
    u32 intValue;
    std::memcpy(&intValue, &value, sizeof(f32));
    writeU32(intValue);
}

void PacketSerializer::writeF64(f64 value)
{
    u64 intValue;
    std::memcpy(&intValue, &value, sizeof(f64));
    writeU64(intValue);
}

void PacketSerializer::writeBool(bool value)
{
    writeU8(value ? 1 : 0);
}

void PacketSerializer::writeString(const std::string& value)
{
    // 字符串格式: 长度(VarInt) + 数据
    // 使用 VarInt 编码，支持大字符串（如命令树JSON）
    if (value.size() > MAX_STRING_LENGTH) {
        // 截断过长的字符串
        writeVarInt(static_cast<i32>(MAX_STRING_LENGTH));
        writeBytes(reinterpret_cast<const u8*>(value.data()), MAX_STRING_LENGTH);
    } else {
        writeVarInt(static_cast<i32>(value.size()));
        writeBytes(reinterpret_cast<const u8*>(value.data()), value.size());
    }
}

void PacketSerializer::writeStringView(std::string_view value)
{
    // 直接写入，避免创建临时String对象
    // 使用 VarInt 编码，支持大字符串
    if (value.size() > MAX_STRING_LENGTH) {
        writeVarInt(static_cast<i32>(MAX_STRING_LENGTH));
        writeBytes(reinterpret_cast<const u8*>(value.data()), MAX_STRING_LENGTH);
    } else {
        writeVarInt(static_cast<i32>(value.size()));
        writeBytes(reinterpret_cast<const u8*>(value.data()), value.size());
    }
}

void PacketSerializer::writeBytes(const u8* data, size_t size)
{
    m_buffer.insert(m_buffer.end(), data, data + size);
}

void PacketSerializer::writeBytes(const std::vector<u8>& data)
{
    m_buffer.insert(m_buffer.end(), data.begin(), data.end());
}

// ============================================================================
// VarInt/VarLong 写入实现
// ============================================================================

void PacketSerializer::writeVarInt(i32 value)
{
    // VarInt 使用 ZigZag 编码将有符号转为无符号
    // 但Minecraft协议中VarInt直接使用有符号数
    // 使用无符号形式写入，每个字节的低7位存储数据
    u32 uvalue = static_cast<u32>(value);
    while (true) {
        if ((uvalue & ~0x7F) == 0) {
            m_buffer.push_back(static_cast<u8>(uvalue));
            break;
        }
        m_buffer.push_back(static_cast<u8>((uvalue & 0x7F) | 0x80));
        uvalue >>= 7;
    }
}

void PacketSerializer::writeVarLong(i64 value)
{
    u64 uvalue = static_cast<u64>(value);
    while (true) {
        if ((uvalue & ~0x7FULL) == 0) {
            m_buffer.push_back(static_cast<u8>(uvalue));
            break;
        }
        m_buffer.push_back(static_cast<u8>((uvalue & 0x7F) | 0x80));
        uvalue >>= 7;
    }
}

void PacketSerializer::writeVarUInt(u32 value)
{
    writeVarInt(static_cast<i32>(value));
}

void PacketSerializer::writeVarULong(u64 value)
{
    writeVarLong(static_cast<i64>(value));
}

Result<u8> PacketSerializer::readU8()
{
    if (m_readPos + 1 > m_buffer.size()) {
        return Error(ErrorCode::OutOfBounds, "Not enough data to read u8");
    }
    return m_buffer[m_readPos++];
}

Result<u16> PacketSerializer::readU16()
{
    if (m_readPos + 2 > m_buffer.size()) {
        return Error(ErrorCode::OutOfBounds, "Not enough data to read u16");
    }
    u16 netValue;
    std::memcpy(&netValue, m_buffer.data() + m_readPos, 2);
    m_readPos += 2;
    return NetworkEndian::networkToHost16(netValue);
}

Result<u32> PacketSerializer::readU32()
{
    if (m_readPos + 4 > m_buffer.size()) {
        return Error(ErrorCode::OutOfBounds, "Not enough data to read u32");
    }
    u32 netValue;
    std::memcpy(&netValue, m_buffer.data() + m_readPos, 4);
    m_readPos += 4;
    return NetworkEndian::networkToHost32(netValue);
}

Result<u64> PacketSerializer::readU64()
{
    if (m_readPos + 8 > m_buffer.size()) {
        return Error(ErrorCode::OutOfBounds, "Not enough data to read u64");
    }
    u64 netValue;
    std::memcpy(&netValue, m_buffer.data() + m_readPos, 8);
    m_readPos += 8;
    return NetworkEndian::networkToHost64(netValue);
}

Result<i8> PacketSerializer::readI8()
{
    auto result = readU8();
    if (result.failed()) {
        return result.error();
    }
    return static_cast<i8>(result.value());
}

Result<i16> PacketSerializer::readI16()
{
    auto result = readU16();
    if (result.failed()) {
        return result.error();
    }
    return static_cast<i16>(result.value());
}

Result<i32> PacketSerializer::readI32()
{
    auto result = readU32();
    if (result.failed()) {
        return result.error();
    }
    return static_cast<i32>(result.value());
}

Result<i64> PacketSerializer::readI64()
{
    auto result = readU64();
    if (result.failed()) {
        return result.error();
    }
    return static_cast<i64>(result.value());
}

Result<f32> PacketSerializer::readF32()
{
    auto result = readU32();
    if (result.failed()) {
        return result.error();
    }
    u32 intValue = result.value();
    f32 value;
    std::memcpy(&value, &intValue, sizeof(f32));
    return value;
}

Result<f64> PacketSerializer::readF64()
{
    auto result = readU64();
    if (result.failed()) {
        return result.error();
    }
    u64 intValue = result.value();
    f64 value;
    std::memcpy(&value, &intValue, sizeof(f64));
    return value;
}

Result<bool> PacketSerializer::readBool()
{
    auto result = readU8();
    if (result.failed()) {
        return result.error();
    }
    return result.value() != 0;
}

Result<std::string> PacketSerializer::readString()
{
    auto lengthResult = readVarInt();
    if (lengthResult.failed()) {
        return lengthResult.error();
    }
    i32 length = lengthResult.value();

    if (length < 0) {
        return Error(ErrorCode::InvalidData, "Negative string length");
    }
    if (static_cast<size_t>(length) > MAX_STRING_LENGTH) {
        return Error(ErrorCode::InvalidData, "std::string length exceeds maximum");
    }
    if (m_readPos + static_cast<size_t>(length) > m_buffer.size()) {
        return Error(ErrorCode::OutOfBounds, "Not enough data to read string");
    }

    std::string str(reinterpret_cast<const char*>(m_buffer.data() + m_readPos), static_cast<size_t>(length));
    m_readPos += static_cast<size_t>(length);
    return str;
}

Result<std::vector<u8>> PacketSerializer::readBytes(size_t size)
{
    if (m_readPos + size > m_buffer.size()) {
        return Error(ErrorCode::OutOfBounds, "Not enough data to read bytes");
    }
    std::vector<u8> data(m_buffer.begin() + m_readPos, m_buffer.begin() + m_readPos + size);
    m_readPos += size;
    return data;
}

// ============================================================================
// VarInt/VarLong 读取实现
// ============================================================================

Result<i32> PacketSerializer::readVarInt()
{
    i32 result = 0;
    int shift = 0;

    while (true) {
        if (m_readPos >= m_buffer.size()) {
            return Error(ErrorCode::OutOfBounds, "Not enough data to read VarInt");
        }

        u8 byte = m_buffer[m_readPos++];
        result |= static_cast<i32>(byte & 0x7F) << shift;

        if ((byte & 0x80) == 0) {
            break;
        }

        shift += 7;
        if (shift >= 35) {
            return Error(ErrorCode::InvalidData, "VarInt too long");
        }
    }

    return result;
}

Result<i64> PacketSerializer::readVarLong()
{
    i64 result = 0;
    int shift = 0;

    while (true) {
        if (m_readPos >= m_buffer.size()) {
            return Error(ErrorCode::OutOfBounds, "Not enough data to read VarLong");
        }

        u8 byte = m_buffer[m_readPos++];
        result |= static_cast<i64>(byte & 0x7F) << shift;

        if ((byte & 0x80) == 0) {
            break;
        }

        shift += 7;
        if (shift >= 70) {
            return Error(ErrorCode::InvalidData, "VarLong too long");
        }
    }

    return result;
}

Result<u32> PacketSerializer::readVarUInt()
{
    auto result = readVarInt();
    if (result.failed()) {
        return result.error();
    }
    return static_cast<u32>(result.value());
}

Result<u64> PacketSerializer::readVarULong()
{
    auto result = readVarLong();
    if (result.failed()) {
        return result.error();
    }
    return static_cast<u64>(result.value());
}

void PacketSerializer::clear()
{
    m_buffer.clear();
    m_readPos = 0;
}

void PacketSerializer::resetRead()
{
    m_readPos = 0;
}

void PacketSerializer::resize(size_t size)
{
    m_buffer.resize(size);
}

void PacketSerializer::reserve(size_t capacity)
{
    m_buffer.reserve(capacity);
}

} // namespace mc::network
