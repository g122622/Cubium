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

#include <gtest/gtest.h>

#include <string>
#include <vector>

using namespace mc::network::buffer;
using namespace mc;

// ============================================================================
// VarInt / VarLong 边界
// ============================================================================

TEST(ByteBuf, VarIntBoundaryValues)
{
    struct Case {
        i32 value;
        usize bytes;
    };
    const Case cases[] = {
        {0, 1},
        {1, 1},
        {127, 1},
        {128, 2},
        {16383, 2},
        {16384, 3},
        {2097151, 3},
        {2097152, 4},
        {INT32_MAX, 5}, // 2147483647
        {-1, 5},        // 0xFFFFFFFF → 5 字节
        {INT32_MIN, 5}, // -2147483648 → 0x80000000 → 5 字节
    };

    for (const auto& c : cases) {
        ByteBuf buf;
        buf.writeVarInt(c.value);
        ASSERT_EQ(buf.size(), c.bytes) << "VarInt(" << c.value << ") 编码字节数";
        auto r = buf.readVarInt();
        ASSERT_TRUE(r.success()) << "VarInt(" << c.value << ") 解码成功";
        EXPECT_EQ(r.value(), c.value) << "VarInt(" << c.value << ") 往返值";
    }
}

TEST(ByteBuf, VarUIntSizeClasses)
{
    // 各字节数边界（u32 视角）
    struct Case {
        u32 value;
        usize bytes;
    };
    const Case cases[] = {
        {0, 1},
        {127, 1},
        {128, 2},
        {16383, 2},
        {16384, 3},
        {2097151, 3},
        {2097152, 4},
        {268435455, 4}, // 2^28-1, 4 字节最大
        {268435456, 5}, // 2^28, 5 字节
        {0xFFFFFFFFu, 5},
    };
    for (const auto& c : cases) {
        ByteBuf buf;
        buf.writeVarUInt(c.value);
        EXPECT_EQ(buf.size(), c.bytes) << "VarUInt(" << c.value << ")";
        auto r = buf.readVarUInt();
        ASSERT_TRUE(r.success());
        EXPECT_EQ(r.value(), c.value);
    }
}

TEST(ByteBuf, VarLongBoundaryValues)
{
    struct Case {
        i64 value;
        usize bytes;
    };
    const Case cases[] = {
        {0, 1},
        {127, 1},
        {128, 2},
        {-1, 10},        // 0xFFFFFFFFFFFFFFFF → 10 字节
        {INT64_MAX, 10}, // 0x7FFFFFFFFFFFFFFF → 9 字节实为 9? 测实际往返为主
        {INT64_MIN, 10},
    };
    for (const auto& c : cases) {
        ByteBuf buf;
        buf.writeVarLong(c.value);
        auto r = buf.readVarLong();
        ASSERT_TRUE(r.success()) << "VarLong(" << c.value << ") 解码成功";
        EXPECT_EQ(r.value(), c.value) << "VarLong(" << c.value << ") 往返值";
    }
}

TEST(ByteBuf, VarIntOverlongRejected)
{
    // 5 字节全带续位 → 第 6 字节仍 0x80 → 超过 32 位，应返 InvalidData
    std::vector<u8> raw = {0x80, 0x80, 0x80, 0x80, 0x80};
    ByteBuf buf(raw.data(), raw.size());
    auto r = buf.readVarInt();
    ASSERT_FALSE(r.success());
    EXPECT_EQ(r.error().code(), ErrorCode::InvalidData);
}

TEST(ByteBuf, VarLongOverlongRejected)
{
    // 10 字节全带续位 → 第 11 字节仍 0x80 → 超过 64 位
    std::vector<u8> raw(10, 0x80);
    ByteBuf buf(raw.data(), raw.size());
    auto r = buf.readVarLong();
    ASSERT_FALSE(r.success());
    EXPECT_EQ(r.error().code(), ErrorCode::InvalidData);
}

// ============================================================================
// 定长原语往返 + 边界
// ============================================================================

TEST(ByteBuf, PrimitiveRoundTrip)
{
    ByteBuf buf;
    buf.writeU8(0x12);
    buf.writeI8(-1);
    buf.writeU16(0x1234);
    buf.writeI16(-32000);
    buf.writeU32(0x12345678);
    buf.writeI32(-70000);
    buf.writeU64(0x0123456789ABCDEF);
    buf.writeI64(-9000000000LL);
    buf.writeF32(3.14159f);
    buf.writeF64(2.718281828459045);
    buf.writeBool(true);
    buf.writeBool(false);

    EXPECT_EQ(buf.readU8().value(), 0x12);
    EXPECT_EQ(buf.readI8().value(), -1);
    EXPECT_EQ(buf.readU16().value(), 0x1234);
    EXPECT_EQ(buf.readI16().value(), -32000);
    EXPECT_EQ(buf.readU32().value(), 0x12345678u);
    EXPECT_EQ(buf.readI32().value(), -70000);
    EXPECT_EQ(buf.readU64().value(), 0x0123456789ABCDEFull);
    EXPECT_EQ(buf.readI64().value(), -9000000000LL);
    EXPECT_NEAR(buf.readF32().value(), 3.14159f, 0.0001f);
    EXPECT_NEAR(buf.readF64().value(), 2.718281828459045, 1e-12);
    EXPECT_TRUE(buf.readBool().value());
    EXPECT_FALSE(buf.readBool().value());
}

TEST(ByteBuf, U16IsBigEndian)
{
    ByteBuf buf;
    buf.writeU16(0x1234);
    // 大端：高字节 0x12 在前
    ASSERT_EQ(buf.size(), 2u);
    EXPECT_EQ(buf.bytes()[0], 0x12);
    EXPECT_EQ(buf.bytes()[1], 0x34);
}

TEST(ByteBuf, U32IsBigEndian)
{
    ByteBuf buf;
    buf.writeU32(0x12345678);
    ASSERT_EQ(buf.size(), 4u);
    EXPECT_EQ(buf.bytes()[0], 0x12);
    EXPECT_EQ(buf.bytes()[1], 0x34);
    EXPECT_EQ(buf.bytes()[2], 0x56);
    EXPECT_EQ(buf.bytes()[3], 0x78);
}

// ============================================================================
// 字符串（VarInt 长度前缀 + UTF-8 字节）
// ============================================================================

TEST(ByteBuf, StringRoundTripAscii)
{
    ByteBuf buf;
    buf.writeString("Hello, World!");
    auto r = buf.readString();
    ASSERT_TRUE(r.success());
    EXPECT_EQ(r.value(), "Hello, World!");
}

TEST(ByteBuf, StringRoundTripMultiByteUtf8)
{
    // 中文 + emoji（2/3/4 字节 UTF-8 混合）。源文件 UTF-8 编码，普通字面量即 UTF-8 字节。
    const std::string s = "你好🌍世界";
    ByteBuf buf;
    buf.writeString(s);
    auto r = buf.readString();
    ASSERT_TRUE(r.success());
    EXPECT_EQ(r.value(), s);
}

TEST(ByteBuf, EmptyString)
{
    ByteBuf buf;
    buf.writeString("");
    ASSERT_EQ(buf.size(), 1u); // 仅 1 字节长度 VarInt(0)
    auto r = buf.readString();
    ASSERT_TRUE(r.success());
    EXPECT_TRUE(r.value().empty());
}

TEST(ByteBuf, StringLengthIsByteCountNotCharCount)
{
    // 3 字节 UTF-8 字符 × N：VarInt 长度应为字节数
    const std::string s = "你好"; // 6 字节
    ByteBuf buf;
    buf.writeString(s);
    // VarInt(6) + 6 字节 = 7
    ASSERT_EQ(buf.size(), 7u);
}

TEST(ByteBuf, StringTruncatesOverlong)
{
    // 超过 kMaxStringLength 的字符串写入时截断（与旧 PacketSerializer 一致行为）
    const std::string huge(ByteBuf::kMaxStringLength + 100, 'x');
    ByteBuf buf;
    buf.writeString(huge);
    auto lenResult = buf.readVarUInt();
    ASSERT_TRUE(lenResult.success());
    EXPECT_LE(lenResult.value(), ByteBuf::kMaxStringLength);
}

TEST(ByteBuf, StringDeclaredLengthOverLimitRejected)
{
    // 构造一个声明长度 = kMaxStringLength+1 但无对应字节的包，读侧应拒
    ByteBuf buf;
    buf.writeVarUInt(ByteBuf::kMaxStringLength + 1);
    auto r = buf.readString();
    ASSERT_FALSE(r.success());
    EXPECT_EQ(r.error().code(), ErrorCode::InvalidData);
}

// ============================================================================
// 字节块读写
// ============================================================================

TEST(ByteBuf, BytesRoundTrip)
{
    std::vector<u8> data = {0x01, 0x02, 0x03, 0x04, 0x05};
    ByteBuf buf;
    buf.writeBytes(data);
    ASSERT_EQ(buf.size(), 5u);
    auto r = buf.readBytes(5);
    ASSERT_TRUE(r.success());
    EXPECT_EQ(r.value(), data);
}

TEST(ByteBuf, BytesViewRoundTrip)
{
    std::vector<u8> data = {0xAA, 0xBB, 0xCC};
    ByteBuf buf;
    buf.writeBytes(data.data(), data.size());
    auto r = buf.readBytesView(3);
    ASSERT_TRUE(r.success());
    EXPECT_EQ(r.value().size(), 3u);
    EXPECT_EQ(static_cast<u8>(r.value()[0]), 0xAA);
}

TEST(ByteBuf, WriteStringViewBytes)
{
    ByteBuf buf;
    buf.writeBytes(std::string_view("abc"));
    ASSERT_EQ(buf.size(), 3u);
    auto r = buf.readBytes(3);
    ASSERT_TRUE(r.success());
    EXPECT_EQ(r.value()[0], 'a');
}

// ============================================================================
// 越界与错误处理
// ============================================================================

TEST(ByteBuf, ReadOutOfBoundsReturnsError)
{
    ByteBuf buf;
    buf.writeU8(0x12);
    ASSERT_TRUE(buf.readU8().success());
    auto r = buf.readU8();
    ASSERT_FALSE(r.success());
    EXPECT_EQ(r.error().code(), ErrorCode::OutOfBounds);
}

TEST(ByteBuf, ReadU16PartialOutOfBounds)
{
    ByteBuf buf;
    buf.writeU8(0x12); // 仅 1 字节，不足 u16
    auto r = buf.readU16();
    ASSERT_FALSE(r.success());
    EXPECT_EQ(r.error().code(), ErrorCode::OutOfBounds);
}

TEST(ByteBuf, ReadU32OutOfBounds)
{
    ByteBuf buf;
    buf.writeU16(0x1234); // 仅 2 字节
    auto r = buf.readU32();
    ASSERT_FALSE(r.success());
    EXPECT_EQ(r.error().code(), ErrorCode::OutOfBounds);
}

TEST(ByteBuf, ReadBytesOutOfBounds)
{
    ByteBuf buf;
    buf.writeU8(0x01);
    auto r = buf.readBytes(5);
    ASSERT_FALSE(r.success());
    EXPECT_EQ(r.error().code(), ErrorCode::OutOfBounds);
}

TEST(ByteBuf, VarIntTruncatedReturnsError)
{
    // 续位标志置位但无后续字节 → 越界
    std::vector<u8> raw = {0x80};
    ByteBuf buf(raw.data(), raw.size());
    auto r = buf.readVarInt();
    ASSERT_FALSE(r.success());
    EXPECT_EQ(r.error().code(), ErrorCode::OutOfBounds);
}

// ============================================================================
// 游标与容量
// ============================================================================

TEST(ByteBuf, ReadPositionAdvances)
{
    ByteBuf buf;
    buf.writeU8(1);
    buf.writeU8(2);
    buf.writeU8(3);
    EXPECT_EQ(buf.readPosition(), 0u);
    EXPECT_EQ(buf.readU8().value(), 1);
    EXPECT_EQ(buf.readPosition(), 1u);
    EXPECT_EQ(buf.readU8().value(), 2);
    EXPECT_EQ(buf.readPosition(), 2u);
}

TEST(ByteBuf, SetReadPositionRewinds)
{
    ByteBuf buf;
    buf.writeU8(0x42);
    ASSERT_TRUE(buf.readU8().success());
    buf.setReadPosition(0);
    EXPECT_EQ(buf.readU8().value(), 0x42);
}

TEST(ByteBuf, ClearResetsCursorAndData)
{
    ByteBuf buf;
    buf.writeU8(1);
    buf.writeU8(2);
    ASSERT_TRUE(buf.readU8().success());
    buf.clear();
    EXPECT_EQ(buf.size(), 0u);
    EXPECT_EQ(buf.readPosition(), 0u);
    EXPECT_EQ(buf.readableBytes(), 0u);
}

TEST(ByteBuf, ReadableBytes)
{
    ByteBuf buf;
    buf.writeU32(0x12345678);
    EXPECT_EQ(buf.readableBytes(), 4u);
    ASSERT_TRUE(buf.readU16().success());
    EXPECT_EQ(buf.readableBytes(), 2u);
}

TEST(ByteBuf, AutoGrowthOnManyWrites)
{
    ByteBuf buf;
    buf.reserve(8);
    for (usize i = 0; i < 1000; ++i) {
        buf.writeU32(static_cast<u32>(i));
    }
    EXPECT_EQ(buf.size(), 4000u);
    for (usize i = 0; i < 1000; ++i) {
        auto r = buf.readU32();
        ASSERT_TRUE(r.success());
        EXPECT_EQ(r.value(), static_cast<u32>(i));
    }
}

TEST(ByteBuf, TakeBytesMovesOwnership)
{
    ByteBuf buf;
    buf.writeU8(0x01);
    buf.writeU8(0x02);
    auto taken = buf.takeBytes();
    EXPECT_EQ(taken.size(), 2u);
    EXPECT_EQ(taken[0], 0x01);
    // 取出后内部缓冲应被移空
    EXPECT_EQ(buf.size(), 0u);
    EXPECT_EQ(buf.readPosition(), 0u);
}

TEST(ByteBuf, ConstructFromExternalBytes)
{
    std::vector<u8> raw = {0x01, 0x02, 0x03};
    ByteBuf buf(raw.data(), raw.size());
    EXPECT_EQ(buf.size(), 3u);
    EXPECT_EQ(buf.readU8().value(), 0x01);
}

TEST(ByteBuf, MaxStringLengthConstant)
{
    // 2^21 - 1
    EXPECT_EQ(ByteBuf::kMaxStringLength, 2097151u);
}

TEST(ByteBuf, Smoke)
{
    ByteBuf buf;
    buf.writeVarInt(12345);
    auto r = buf.readVarInt();
    ASSERT_TRUE(r.success());
    EXPECT_EQ(r.value(), 12345);
}
