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

#include "PacketDeserializer.hpp"
#include "PacketSerializer.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/util/assert/AssertAll.hpp"

#include <cstring>
#include <string>
#include <vector>

namespace mc::network {

namespace {

/// 空输入的基址。空 vector 的 data() 与 length==0 的缓冲区指针都可以是 nullptr，
/// 而 readBytes(0)/readString() 的零长度分支仍会把该基址交给 vector 范围构造、
/// std::string 构造与 std::memcpy，这些都是带 nonnull 属性的参数，传 nullptr 是 UB。
constexpr u8 s_emptyBase = 0;

/// 把可能为空的输入指针规范化为非空基址；长度为 0 时永远不会被解引用。
constexpr const u8* _baseOf(const u8* data) noexcept
{
    return data != nullptr ? data : &s_emptyBase;
}

} // namespace

PacketDeserializer::PacketDeserializer(const u8* data, size_t size)
    : m_data(_baseOf(data))
    , m_size(data != nullptr ? size : 0)
    , m_readPos(0)
{}

PacketDeserializer::PacketDeserializer(const std::vector<u8>& data)
    : m_data(_baseOf(data.data()))
    , m_size(data.size())
    , m_readPos(0)
{}

Result<u8> PacketDeserializer::readU8()
{
    if (m_readPos + 1 > m_size) {
        return Error(ErrorCode::OutOfBounds, "Not enough data to read u8");
    }
    return m_data[m_readPos++];
}

Result<u16> PacketDeserializer::readU16()
{
    if (m_readPos + 2 > m_size) {
        return Error(ErrorCode::OutOfBounds, "Not enough data to read u16");
    }
    u16 netValue;
    std::memcpy(&netValue, m_data + m_readPos, 2);
    m_readPos += 2;
    return NetworkEndian::networkToHost16(netValue);
}

Result<u32> PacketDeserializer::readU32()
{
    if (m_readPos + 4 > m_size) {
        return Error(ErrorCode::OutOfBounds, "Not enough data to read u32");
    }
    u32 netValue;
    std::memcpy(&netValue, m_data + m_readPos, 4);
    m_readPos += 4;
    return NetworkEndian::networkToHost32(netValue);
}

Result<u64> PacketDeserializer::readU64()
{
    if (m_readPos + 8 > m_size) {
        return Error(ErrorCode::OutOfBounds, "Not enough data to read u64");
    }
    u64 netValue;
    std::memcpy(&netValue, m_data + m_readPos, 8);
    m_readPos += 8;
    return NetworkEndian::networkToHost64(netValue);
}

Result<i8> PacketDeserializer::readI8()
{
    auto result = readU8();
    if (result.failed()) {
        return result.error();
    }
    return static_cast<i8>(result.value());
}

Result<i16> PacketDeserializer::readI16()
{
    auto result = readU16();
    if (result.failed()) {
        return result.error();
    }
    return static_cast<i16>(result.value());
}

Result<i32> PacketDeserializer::readI32()
{
    auto result = readU32();
    if (result.failed()) {
        return result.error();
    }
    return static_cast<i32>(result.value());
}

Result<i64> PacketDeserializer::readI64()
{
    auto result = readU64();
    if (result.failed()) {
        return result.error();
    }
    return static_cast<i64>(result.value());
}

Result<f32> PacketDeserializer::readF32()
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

Result<f64> PacketDeserializer::readF64()
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

Result<bool> PacketDeserializer::readBool()
{
    auto result = readU8();
    if (result.failed()) {
        return result.error();
    }
    return result.value() != 0;
}

Result<std::string> PacketDeserializer::readString()
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
    if (m_readPos + static_cast<size_t>(length) > m_size) {
        return Error(ErrorCode::OutOfBounds, "Not enough data to read string");
    }

    std::string str(reinterpret_cast<const char*>(m_data + m_readPos), static_cast<size_t>(length));
    m_readPos += static_cast<size_t>(length);
    return str;
}

Result<std::vector<u8>> PacketDeserializer::readBytes(size_t size)
{
    if (m_readPos + size > m_size) {
        return Error(ErrorCode::OutOfBounds, "Not enough data to read bytes");
    }
    std::vector<u8> data(m_data + m_readPos, m_data + m_readPos + size);
    m_readPos += size;
    return data;
}

Result<void> PacketDeserializer::readBytesInto(u8* dest, size_t size)
{
    if (m_readPos + size > m_size) {
        return Error(ErrorCode::OutOfBounds, "Not enough data to read bytes");
    }
    // 零长度读取不构成拷贝：dest 允许为空（memcpy 的 dst 带 nonnull 属性）。
    if (size > 0) {
        MC_ASSERT_RELEASE_MSG(dest != nullptr, "readBytesInto: null destination for non-zero size");
        std::memcpy(dest, m_data + m_readPos, size);
    }
    m_readPos += size;
    return {};
}

Result<i32> PacketDeserializer::readVarInt()
{
    i32 result = 0;
    i32 shift = 0;

    while (true) {
        if (m_readPos >= m_size) {
            return Error(ErrorCode::OutOfBounds, "Not enough data to read VarInt");
        }

        u8 byte = m_data[m_readPos++];
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

Result<i64> PacketDeserializer::readVarLong()
{
    i64 result = 0;
    i32 shift = 0;

    while (true) {
        if (m_readPos >= m_size) {
            return Error(ErrorCode::OutOfBounds, "Not enough data to read VarLong");
        }

        u8 byte = m_data[m_readPos++];
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

Result<u32> PacketDeserializer::readVarUInt()
{
    auto result = readVarInt();
    if (result.failed()) {
        return result.error();
    }
    return static_cast<u32>(result.value());
}

Result<u64> PacketDeserializer::readVarULong()
{
    auto result = readVarLong();
    if (result.failed()) {
        return result.error();
    }
    return static_cast<u64>(result.value());
}

} // namespace mc::network
