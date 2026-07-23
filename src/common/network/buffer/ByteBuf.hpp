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

#include "common/core/Result.hpp"
#include "common/core/Types.hpp"

#include <string>
#include <string_view>
#include <vector>

namespace mc::network::buffer {

/**
 * @brief 协议无关的可读写字节缓冲（大端）
 *
 * 对应 Java 版 FriendlyByteBuf 的核心静态能力：可增长字节存储 + 读写游标 +
 * 定长原语（大端）+ VarInt/VarLong + 字符串（VarInt 长度前缀 + UTF-8 字节）。
 *
 * 设计要点：
 * - 单缓冲双向：旧体系 PacketSerializer（双向）与 PacketDeserializer（只读）
 *   职责重叠，新体系统一为单一可读写缓冲，codec 层按需读/写。
 * - 大端：Java 线协议全程大端；基岩版小端由其专属 ByteBuf 变体处理（后续阶段）。
 * - 错误处理：读操作返回 Result<T>，越界/非法 VarInt 返回 InvalidData/OutOfBounds，
 *   不抛异常（遵循项目 Result 准则）。
 * - NBT 复用：本类不直接依赖 nbt 库（避免反向依赖）；提供字节视图与可重定位游标，
 *   RegistryByteBuf 经 stream 适配层桥接 nbt 库读写（见 buffer/NbtIo.hpp）。
 */
class ByteBuf {
public:
    ByteBuf() = default;
    ~ByteBuf() = default;

    // 可拷贝可移动：codec 经常需要持有/转移缓冲快照
    ByteBuf(const ByteBuf&) = default;
    ByteBuf(ByteBuf&&) noexcept = default;
    ByteBuf& operator=(const ByteBuf&) = default;
    ByteBuf& operator=(ByteBuf&&) noexcept = default;

    /**
     * @brief 从外部字节序列构造只读视图快照（反序列化入口）
     */
    explicit ByteBuf(const u8* data, usize size);

    // ============================================================================
    // 容量与游标
    // ============================================================================

    [[nodiscard]] usize size() const noexcept { return m_data.size(); }
    [[nodiscard]] usize readableBytes() const noexcept { return m_data.size() - m_readPos; }
    [[nodiscard]] usize readPosition() const noexcept { return m_readPos; }
    void setReadPosition(usize pos) noexcept { m_readPos = pos; }
    void clear() noexcept;
    void reserve(usize capacity) { m_data.reserve(capacity); }

    [[nodiscard]] const u8* data() const noexcept { return m_data.data(); }
    [[nodiscard]] const std::vector<u8>& bytes() const noexcept { return m_data; }

    /**
     * @brief 取出内部字节（移动语义，写完发送时用）
     */
    [[nodiscard]] std::vector<u8> takeBytes() noexcept;

    // ============================================================================
    // 定长原语写入（大端）
    // ============================================================================

    void writeU8(u8 value) { m_data.push_back(value); }
    void writeI8(i8 value) { writeU8(static_cast<u8>(value)); }
    void writeU16(u16 value);
    void writeI16(i16 value) { writeU16(static_cast<u16>(value)); }
    void writeU32(u32 value);
    void writeI32(i32 value) { writeU32(static_cast<u32>(value)); }
    void writeU64(u64 value);
    void writeI64(i64 value) { writeU64(static_cast<u64>(value)); }
    void writeF32(f32 value);
    void writeF64(f64 value);
    void writeBool(bool value) { writeU8(value ? 1 : 0); }

    void writeBytes(const u8* data, usize size);
    void writeBytes(std::string_view data);
    void writeBytes(const std::vector<u8>& data);

    // ============================================================================
    // VarInt / VarLong（标准 MC 协议，zigzag 不在此，见 VarInts.hpp 的变体）
    // ============================================================================

    void writeVarInt(i32 value) { writeVarUInt(static_cast<u32>(value)); }
    void writeVarLong(i64 value) { writeVarULong(static_cast<u64>(value)); }
    void writeVarUInt(u32 value);
    void writeVarULong(u64 value);

    // ============================================================================
    // 字符串（VarInt 长度前缀 + UTF-8 字节，上限 MAX_STRING_LENGTH）
    // ============================================================================

    void writeString(std::string_view value);
    void writeUtf8(std::string_view value) { writeString(value); }

    // ============================================================================
    // 定长原语读取（大端，越界返回 OutOfBounds）
    // ============================================================================

    [[nodiscard]] Result<u8> readU8();
    [[nodiscard]] Result<i8> readI8()
    {
        auto r = readU8();
        return r.success() ? Result<i8>(static_cast<i8>(r.value())) : Result<i8>(r.error());
    }
    [[nodiscard]] Result<u16> readU16();
    [[nodiscard]] Result<i16> readI16();
    [[nodiscard]] Result<u32> readU32();
    [[nodiscard]] Result<i32> readI32();
    [[nodiscard]] Result<u64> readU64();
    [[nodiscard]] Result<i64> readI64();
    [[nodiscard]] Result<f32> readF32();
    [[nodiscard]] Result<f64> readF64();
    [[nodiscard]] Result<bool> readBool();

    /**
     * @brief 读取 size 字节到调用方缓冲
     */
    [[nodiscard]] Result<void> readBytes(u8* out, usize size);

    /**
     * @brief 读取 size 字节并返回拷贝（小数据用）
     */
    [[nodiscard]] Result<std::vector<u8>> readBytes(usize size);

    /**
     * @brief 零拷贝读取：返回指向内部存储的只读视图（游标仍前进 size）
     *
     * 调用方须保证在使用视图期间本缓冲不被重分配/销毁。
     */
    [[nodiscard]] Result<std::string_view> readBytesView(usize size);

    // ============================================================================
    // VarInt / VarLong 读取（非法长度返回 InvalidData）
    // ============================================================================

    [[nodiscard]] Result<i32> readVarInt()
    {
        auto r = readVarUInt();
        return r.success() ? Result<i32>(static_cast<i32>(r.value())) : Result<i32>(r.error());
    }
    [[nodiscard]] Result<i64> readVarLong()
    {
        auto r = readVarULong();
        return r.success() ? Result<i64>(static_cast<i64>(r.value())) : Result<i64>(r.error());
    }
    [[nodiscard]] Result<u32> readVarUInt();
    [[nodiscard]] Result<u64> readVarULong();

    [[nodiscard]] Result<std::string> readString();

    /**
     * @brief VarInt 最多占用的字节数（5），用于游标安全检查常量
     */
    static constexpr usize kMaxVarIntBytes = 5;
    static constexpr usize kMaxVarLongBytes = 10;

    /**
     * @brief Java 协议字符串长度上限（2^21 - 1，含字节数非字符数）
     */
    static constexpr u32 kMaxStringLength = 2097151;

private:
    [[nodiscard]] Result<void> ensureReadable(usize size) const;

    std::vector<u8> m_data;
    usize m_readPos = 0;
};

} // namespace mc::network::buffer
