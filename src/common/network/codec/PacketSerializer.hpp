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

#pragma once

#include "PacketDeserializer.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include <array>
#include <cstring>
#include <string>
#include <type_traits>
#include <vector>

namespace mc::network {

// ============================================================================
// 常量定义
// ============================================================================

/// 字符串最大长度 (2^21 - 1, VarInt 3字节能表示的最大值)
/// Minecraft协议中字符串长度限制为 32767 (Short.MAX_VALUE)，但命令树等数据可能超过此限制
/// 使用 VarInt 编码可支持更大的字符串
constexpr u32 MAX_STRING_LENGTH = 2097151;

// ============================================================================
// 网络字节序转换
// ============================================================================

/**
 * @brief 网络字节序转换工具类
 *
 * 提供主机字节序与网络字节序（大端序）之间的转换。
 */
class NetworkEndian {
public:
    static u16 hostToNetwork16(u16 value);
    static u32 hostToNetwork32(u32 value);
    static u64 hostToNetwork64(u64 value);
    static u16 networkToHost16(u16 value);
    static u32 networkToHost32(u32 value);
    static u64 networkToHost64(u64 value);

    template <typename T>
    static T hostToNetwork(T value)
    {
        static_assert(std::is_arithmetic_v<T>, "T must be arithmetic type");
        if constexpr (sizeof(T) == 2) {
            return static_cast<T>(hostToNetwork16(static_cast<u16>(value)));
        } else if constexpr (sizeof(T) == 4) {
            return static_cast<T>(hostToNetwork32(static_cast<u32>(value)));
        } else if constexpr (sizeof(T) == 8) {
            return static_cast<T>(hostToNetwork64(static_cast<u64>(value)));
        } else {
            return value; // 1字节无需转换
        }
    }

    template <typename T>
    static T networkToHost(T value)
    {
        return hostToNetwork(value); // 转换是对称的
    }
};

// ============================================================================
// VarInt/VarLong 编码结果
// ============================================================================

struct VarIntResult {
    i32 value;
    size_t bytesRead;
};

struct VarLongResult {
    i64 value;
    size_t bytesRead;
};

// 数据包序列化器
class PacketSerializer {
public:
    PacketSerializer();
    explicit PacketSerializer(size_t initialCapacity);
    PacketSerializer(const PacketSerializer& other) = default;
    PacketSerializer(PacketSerializer&& other) noexcept;
    PacketSerializer& operator=(const PacketSerializer& other) = default;
    PacketSerializer& operator=(PacketSerializer&& other) noexcept;
    ~PacketSerializer() = default;

    // 写入操作
    void writeU8(u8 value);
    void writeU16(u16 value);
    void writeU32(u32 value);
    void writeU64(u64 value);
    void writeI8(i8 value);
    void writeI16(i16 value);
    void writeI32(i32 value);
    void writeI64(i64 value);
    void writeF32(f32 value);
    void writeF64(f64 value);
    void writeBool(bool value);
    void writeString(const std::string& value);
    void writeStringView(std::string_view value);
    void writeBytes(const u8* data, size_t size);
    void writeBytes(const std::vector<u8>& data);

    // VarInt/VarLong 写入 (Minecraft协议格式)
    void writeVarInt(i32 value);
    void writeVarLong(i64 value);
    void writeVarUInt(u32 value);  // 无符号VarInt
    void writeVarULong(u64 value); // 无符号VarLong

    // 读取操作
    [[nodiscard]] Result<u8> readU8();
    [[nodiscard]] Result<u16> readU16();
    [[nodiscard]] Result<u32> readU32();
    [[nodiscard]] Result<u64> readU64();
    [[nodiscard]] Result<i8> readI8();
    [[nodiscard]] Result<i16> readI16();
    [[nodiscard]] Result<i32> readI32();
    [[nodiscard]] Result<i64> readI64();
    [[nodiscard]] Result<f32> readF32();
    [[nodiscard]] Result<f64> readF64();
    [[nodiscard]] Result<bool> readBool();
    [[nodiscard]] Result<std::string> readString();
    [[nodiscard]] Result<std::vector<u8>> readBytes(size_t size);

    // VarInt/VarLong 读取
    [[nodiscard]] Result<i32> readVarInt();
    [[nodiscard]] Result<i64> readVarLong();
    [[nodiscard]] Result<u32> readVarUInt();
    [[nodiscard]] Result<u64> readVarULong();

    // 缓冲区管理
    const std::vector<u8>& buffer() const { return m_buffer; }
    std::vector<u8>& buffer() { return m_buffer; }
    const u8* data() const { return m_buffer.data(); }
    u8* data() { return m_buffer.data(); }
    size_t size() const { return m_buffer.size(); }
    size_t remaining() const { return m_buffer.size() - m_readPos; }
    void clear();
    void resetRead();
    void resize(size_t size);
    void reserve(size_t capacity);

private:
    std::vector<u8> m_buffer;
    size_t m_readPos = 0;
};

} // namespace mc::network
