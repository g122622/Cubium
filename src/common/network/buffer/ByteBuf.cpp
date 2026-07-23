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

#include "common/network/buffer/ByteBuf.hpp"
#include "common/network/buffer/Endian.hpp"

#include <cstring>

namespace mc::network::buffer {

ByteBuf::ByteBuf(const u8* data, usize size)
{
    if (data != nullptr && size > 0) {
        m_data.assign(data, data + size);
    }
}

void ByteBuf::clear() noexcept
{
    m_data.clear();
    m_readPos = 0;
}

std::vector<u8> ByteBuf::takeBytes() noexcept
{
    m_readPos = 0;
    return std::move(m_data);
}

Result<void> ByteBuf::ensureReadable(usize size) const
{
    if (readableBytes() < size) {
        return Error(ErrorCode::OutOfBounds, "ByteBuf 读取越界", "mc::network::buffer::ByteBuf::ensureReadable");
    }
    return Result<void>::ok();
}

// ============================================================================
// 定长原语写入
// ============================================================================

void ByteBuf::writeU16(u16 value)
{
    const u16 net = Endian::hostToNetwork16(value);
    m_data.push_back(static_cast<u8>(net & 0xFFu));
    m_data.push_back(static_cast<u8>((net >> 8) & 0xFFu));
}

void ByteBuf::writeU32(u32 value)
{
    const u32 net = Endian::hostToNetwork32(value);
    for (int shift = 0; shift < 32; shift += 8) {
        m_data.push_back(static_cast<u8>((net >> shift) & 0xFFu));
    }
}

void ByteBuf::writeU64(u64 value)
{
    const u64 net = Endian::hostToNetwork64(value);
    for (int shift = 0; shift < 64; shift += 8) {
        m_data.push_back(static_cast<u8>((net >> shift) & 0xFFu));
    }
}

void ByteBuf::writeF32(f32 value)
{
    u32 bits = 0;
    std::memcpy(&bits, &value, sizeof(bits));
    writeU32(bits);
}

void ByteBuf::writeF64(f64 value)
{
    u64 bits = 0;
    std::memcpy(&bits, &value, sizeof(bits));
    writeU64(bits);
}

void ByteBuf::writeBytes(const u8* data, usize size)
{
    if (data != nullptr && size > 0) {
        m_data.insert(m_data.end(), data, data + size);
    }
}

void ByteBuf::writeBytes(std::string_view data)
{
    writeBytes(reinterpret_cast<const u8*>(data.data()), data.size());
}

void ByteBuf::writeBytes(const std::vector<u8>& data)
{
    writeBytes(data.data(), data.size());
}

// ============================================================================
// VarInt / VarLong 写入
// ============================================================================

void ByteBuf::writeVarUInt(u32 value)
{
    // 标准 MC VarInt：每字节低 7 位 + 续位标志 0x80，最多 5 字节。
    while (true) {
        if ((value & ~static_cast<u32>(0x7F)) == 0) {
            m_data.push_back(static_cast<u8>(value));
            return;
        }
        m_data.push_back(static_cast<u8>((value & 0x7Fu) | 0x80u));
        value >>= 7;
    }
}

void ByteBuf::writeVarULong(u64 value)
{
    while (true) {
        if ((value & ~static_cast<u64>(0x7F)) == 0) {
            m_data.push_back(static_cast<u8>(value));
            return;
        }
        m_data.push_back(static_cast<u8>((value & 0x7Fu) | 0x80u));
        value >>= 7;
    }
}

void ByteBuf::writeString(std::string_view value)
{
    // 超长截断（与旧 PacketSerializer 行为一致；合法长度由 codec/调用方校验）。
    const u32 len = static_cast<u32>(value.size() > kMaxStringLength ? kMaxStringLength : value.size());
    writeVarUInt(len);
    writeBytes(reinterpret_cast<const u8*>(value.data()), len);
}

// ============================================================================
// 定长原语读取
// ============================================================================

Result<u8> ByteBuf::readU8()
{
    MC_TRY(ensureReadable(1));
    return m_data[m_readPos++];
}

Result<u16> ByteBuf::readU16()
{
    MC_TRY(ensureReadable(2));
    const u16 lo = static_cast<u16>(m_data[m_readPos++]);
    const u16 hi = static_cast<u16>(m_data[m_readPos++]);
    return Endian::networkToHost16(static_cast<u16>((hi << 8) | lo));
}

Result<i16> ByteBuf::readI16()
{
    auto r = readU16();
    return r.success() ? Result<i16>(static_cast<i16>(r.value())) : Result<i16>(r.error());
}

Result<u32> ByteBuf::readU32()
{
    MC_TRY(ensureReadable(4));
    u32 net = 0;
    for (int shift = 0; shift < 32; shift += 8) {
        net |= static_cast<u32>(m_data[m_readPos++]) << shift;
    }
    return Endian::networkToHost32(net);
}

Result<i32> ByteBuf::readI32()
{
    auto r = readU32();
    return r.success() ? Result<i32>(static_cast<i32>(r.value())) : Result<i32>(r.error());
}

Result<u64> ByteBuf::readU64()
{
    MC_TRY(ensureReadable(8));
    u64 net = 0;
    for (int shift = 0; shift < 64; shift += 8) {
        net |= static_cast<u64>(m_data[m_readPos++]) << shift;
    }
    return Endian::networkToHost64(net);
}

Result<i64> ByteBuf::readI64()
{
    auto r = readU64();
    return r.success() ? Result<i64>(static_cast<i64>(r.value())) : Result<i64>(r.error());
}

Result<f32> ByteBuf::readF32()
{
    auto r = readU32();
    if (!r.success()) {
        return Result<f32>(r.error());
    }
    f32 value = 0;
    u32 bits = r.value();
    std::memcpy(&value, &bits, sizeof(value));
    return value;
}

Result<f64> ByteBuf::readF64()
{
    auto r = readU64();
    if (!r.success()) {
        return Result<f64>(r.error());
    }
    f64 value = 0;
    u64 bits = r.value();
    std::memcpy(&value, &bits, sizeof(value));
    return value;
}

Result<bool> ByteBuf::readBool()
{
    auto r = readU8();
    return r.success() ? Result<bool>(r.value() != 0) : Result<bool>(r.error());
}

Result<void> ByteBuf::readBytes(u8* out, usize size)
{
    MC_TRY(ensureReadable(size));
    if (out != nullptr && size > 0) {
        std::memcpy(out, m_data.data() + m_readPos, size);
    }
    m_readPos += size;
    return Result<void>::ok();
}

Result<std::vector<u8>> ByteBuf::readBytes(usize size)
{
    MC_TRY(ensureReadable(size));
    std::vector<u8> out(m_data.begin() + static_cast<std::ptrdiff_t>(m_readPos),
        m_data.begin() + static_cast<std::ptrdiff_t>(m_readPos + size));
    m_readPos += size;
    return out;
}

Result<std::string_view> ByteBuf::readBytesView(usize size)
{
    MC_TRY(ensureReadable(size));
    std::string_view view(reinterpret_cast<const char*>(m_data.data() + m_readPos), size);
    m_readPos += size;
    return view;
}

// ============================================================================
// VarInt / VarLong 读取
// ============================================================================

Result<u32> ByteBuf::readVarUInt()
{
    u32 value = 0;
    for (usize i = 0; i < kMaxVarIntBytes; ++i) {
        const auto byteResult = readU8();
        if (!byteResult.success()) {
            return Error(byteResult.error());
        }
        const u8 byte = byteResult.value();
        value |= static_cast<u32>(byte & 0x7Fu) << (7 * i);
        if ((byte & 0x80u) == 0) {
            return value;
        }
    }
    // 第 5 字节仍有续位 → 非法 VarInt（超过 32 位）
    return Error(ErrorCode::InvalidData, "VarInt 超过 5 字节", "mc::network::buffer::ByteBuf::readVarUInt");
}

Result<u64> ByteBuf::readVarULong()
{
    u64 value = 0;
    for (usize i = 0; i < kMaxVarLongBytes; ++i) {
        const auto byteResult = readU8();
        if (!byteResult.success()) {
            return Error(byteResult.error());
        }
        const u8 byte = byteResult.value();
        value |= static_cast<u64>(byte & 0x7Fu) << (7 * i);
        if ((byte & 0x80u) == 0) {
            return value;
        }
    }
    return Error(ErrorCode::InvalidData, "VarLong 超过 10 字节", "mc::network::buffer::ByteBuf::readVarULong");
}

Result<std::string> ByteBuf::readString()
{
    u32 length = 0;
    MC_TRY_ASSIGN(length, readVarUInt());
    if (length > kMaxStringLength) {
        return Error(ErrorCode::InvalidData, "字符串长度超过上限", "mc::network::buffer::ByteBuf::readString");
    }
    std::string_view view;
    MC_TRY_ASSIGN(view, readBytesView(length));
    return std::string(view);
}

} // namespace mc::network::buffer
