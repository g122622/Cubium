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

#include "../../core/Result.hpp"
#include "../../core/Types.hpp"
#include <array>
#include <string>
#include <vector>

namespace mc::network {

// 数据包反序列化器 (读取现有数据)
class PacketDeserializer {
public:
    explicit PacketDeserializer(const u8* data, size_t size);
    explicit PacketDeserializer(const std::vector<u8>& data);

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

    /**
     * @brief 直接读取字节到目标缓冲区（零拷贝）
     * @param dest 目标缓冲区指针
     * @param size 要读取的字节数
     * @return 成功返回空，失败返回错误
     */
    [[nodiscard]] Result<void> readBytesInto(u8* dest, size_t size);

    /**
     * @brief 直接读取字节到固定大小数组（零拷贝）
     * @tparam N 数组大小
     * @param dest 目标数组
     * @return 成功返回空，失败返回错误
     */
    template <size_t N>
    [[nodiscard]] Result<void> readBytesInto(std::array<u8, N>& dest)
    {
        return readBytesInto(dest.data(), N);
    }

    // VarInt/VarLong 读取
    [[nodiscard]] Result<i32> readVarInt();
    [[nodiscard]] Result<i64> readVarLong();
    [[nodiscard]] Result<u32> readVarUInt();
    [[nodiscard]] Result<u64> readVarULong();

    // 状态查询
    const u8* data() const { return m_data; }
    size_t size() const { return m_size; }
    size_t remaining() const { return m_size - m_readPos; }
    bool hasRemaining(size_t bytes) const { return remaining() >= bytes; }
    /** 当前读取位置指针（已读 cursor 处），用于获取连续读取后的剩余数据视图。 */
    const u8* currentPosition() const noexcept { return m_data + m_readPos; }
    void reset() { m_readPos = 0; }

private:
    const u8* m_data;
    size_t m_size;
    size_t m_readPos = 0;
};

} // namespace mc::network
