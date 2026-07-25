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

#include <gtest/gtest.h>

#include "common/network/codec/PacketDeserializer.hpp"
#include "common/network/codec/PacketSerializer.hpp"

using namespace mc::network;
using namespace mc;

// ============================================================================
// PacketSerializer 测试
// ============================================================================

TEST(PacketSerializer, WriteReadU8)
{
    PacketSerializer serializer;
    serializer.writeU8(0x12);
    serializer.writeU8(0xFF);

    EXPECT_EQ(serializer.size(), 2u);

    auto result1 = serializer.readU8();
    EXPECT_TRUE(result1.success());
    EXPECT_EQ(result1.value(), 0x12);

    auto result2 = serializer.readU8();
    EXPECT_TRUE(result2.success());
    EXPECT_EQ(result2.value(), 0xFF);
}

TEST(PacketSerializer, WriteReadU16)
{
    PacketSerializer serializer;
    serializer.writeU16(0x1234);
    serializer.writeU16(0xFFFF);

    EXPECT_EQ(serializer.size(), 4u);

    auto result1 = serializer.readU16();
    EXPECT_TRUE(result1.success());
    EXPECT_EQ(result1.value(), 0x1234);

    auto result2 = serializer.readU16();
    EXPECT_TRUE(result2.success());
    EXPECT_EQ(result2.value(), 0xFFFF);
}

TEST(PacketSerializer, WriteReadU32)
{
    PacketSerializer serializer;
    serializer.writeU32(0x12345678);
    serializer.writeU32(0xFFFFFFFF);

    auto result1 = serializer.readU32();
    EXPECT_TRUE(result1.success());
    EXPECT_EQ(result1.value(), 0x12345678u);

    auto result2 = serializer.readU32();
    EXPECT_TRUE(result2.success());
    EXPECT_EQ(result2.value(), 0xFFFFFFFFu);
}

TEST(PacketSerializer, WriteReadU64)
{
    PacketSerializer serializer;
    serializer.writeU64(0x0123456789ABCDEF);
    serializer.writeU64(0xFFFFFFFFFFFFFFFF);

    auto result1 = serializer.readU64();
    EXPECT_TRUE(result1.success());
    EXPECT_EQ(result1.value(), 0x0123456789ABCDEFull);

    auto result2 = serializer.readU64();
    EXPECT_TRUE(result2.success());
    EXPECT_EQ(result2.value(), 0xFFFFFFFFFFFFFFFFull);
}

TEST(PacketSerializer, WriteReadFloat)
{
    PacketSerializer serializer;
    serializer.writeF32(3.14159f);
    serializer.writeF32(-1234.5f);

    auto result1 = serializer.readF32();
    EXPECT_TRUE(result1.success());
    EXPECT_NEAR(result1.value(), 3.14159f, 0.0001f);

    auto result2 = serializer.readF32();
    EXPECT_TRUE(result2.success());
    EXPECT_NEAR(result2.value(), -1234.5f, 0.0001f);
}

TEST(PacketSerializer, WriteReadDouble)
{
    PacketSerializer serializer;
    serializer.writeF64(3.14159265358979);
    serializer.writeF64(-1234.5678);

    auto result1 = serializer.readF64();
    EXPECT_TRUE(result1.success());
    EXPECT_NEAR(result1.value(), 3.14159265358979, 0.00000001);

    auto result2 = serializer.readF64();
    EXPECT_TRUE(result2.success());
    EXPECT_NEAR(result2.value(), -1234.5678, 0.0001);
}

TEST(PacketSerializer, WriteReadBool)
{
    PacketSerializer serializer;
    serializer.writeBool(true);
    serializer.writeBool(false);

    auto result1 = serializer.readBool();
    EXPECT_TRUE(result1.success());
    EXPECT_TRUE(result1.value());

    auto result2 = serializer.readBool();
    EXPECT_TRUE(result2.success());
    EXPECT_FALSE(result2.value());
}

TEST(PacketSerializer, WriteReadString)
{
    PacketSerializer serializer;
    serializer.writeString("Hello");
    serializer.writeString("World");

    auto result1 = serializer.readString();
    EXPECT_TRUE(result1.success());
    EXPECT_EQ(result1.value(), "Hello");

    auto result2 = serializer.readString();
    EXPECT_TRUE(result2.success());
    EXPECT_EQ(result2.value(), "World");
}

TEST(PacketSerializer, WriteReadBytes)
{
    PacketSerializer serializer;
    std::vector<mc::u8> data = {0x01, 0x02, 0x03, 0x04, 0x05};
    serializer.writeBytes(data);

    auto result = serializer.readBytes(5);
    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value().size(), 5u);
    EXPECT_EQ(result.value()[0], 0x01);
    EXPECT_EQ(result.value()[4], 0x05);
}

TEST(PacketSerializer, OutOfBounds)
{
    PacketSerializer serializer;
    serializer.writeU8(0x12);

    // 读取成功
    auto result1 = serializer.readU8();
    EXPECT_TRUE(result1.success());

    // 读取失败 - 没有数据了
    auto result2 = serializer.readU8();
    EXPECT_FALSE(result2.success());
    EXPECT_EQ(result2.error().code(), mc::ErrorCode::OutOfBounds);
}

TEST(PacketSerializer, ResetRead)
{
    PacketSerializer serializer;
    serializer.writeU8(0x12);
    serializer.writeU8(0x34);

    // 读取一次
    auto result1 = serializer.readU8();
    EXPECT_TRUE(result1.success());
    EXPECT_EQ(result1.value(), 0x12);

    // 重置读位置
    serializer.resetRead();

    // 再次读取应该从头开始
    auto result2 = serializer.readU8();
    EXPECT_TRUE(result2.success());
    EXPECT_EQ(result2.value(), 0x12);
}

// ============================================================================
// PacketDeserializer 测试
// ============================================================================

TEST(PacketDeserializer, ReadOperations)
{
    PacketSerializer serializer;
    serializer.writeU8(0x12);
    serializer.writeU16(0x1234);
    serializer.writeU32(0x12345678);

    PacketDeserializer deserializer(serializer.buffer());

    auto result1 = deserializer.readU8();
    EXPECT_TRUE(result1.success());
    EXPECT_EQ(result1.value(), 0x12);

    auto result2 = deserializer.readU16();
    EXPECT_TRUE(result2.success());
    EXPECT_EQ(result2.value(), 0x1234);

    auto result3 = deserializer.readU32();
    EXPECT_TRUE(result3.success());
    EXPECT_EQ(result3.value(), 0x12345678u);
}

TEST(PacketDeserializer, HasRemaining)
{
    PacketSerializer serializer;
    serializer.writeU32(0x12345678);

    PacketDeserializer deserializer(serializer.buffer());

    EXPECT_TRUE(deserializer.hasRemaining(4));
    EXPECT_TRUE(deserializer.hasRemaining(2));
    EXPECT_FALSE(deserializer.hasRemaining(5));

    (void)deserializer.readU32();
    EXPECT_FALSE(deserializer.hasRemaining(1));
}

// ============================================================================
// VarInt/VarLong 测试
// ============================================================================

TEST(PacketSerializer, WriteReadVarInt)
{
    PacketSerializer serializer;

    // 测试各种值
    serializer.writeVarInt(0);
    serializer.writeVarInt(1);
    serializer.writeVarInt(127);
    serializer.writeVarInt(128);
    serializer.writeVarInt(16383);
    serializer.writeVarInt(16384);
    serializer.writeVarInt(2097151);
    serializer.writeVarInt(2097152);
    serializer.writeVarInt(-1);
    serializer.writeVarInt(-2147483648); // INT_MIN

    auto result0 = serializer.readVarInt();
    EXPECT_TRUE(result0.success());
    EXPECT_EQ(result0.value(), 0);

    auto result1 = serializer.readVarInt();
    EXPECT_TRUE(result1.success());
    EXPECT_EQ(result1.value(), 1);

    auto result127 = serializer.readVarInt();
    EXPECT_TRUE(result127.success());
    EXPECT_EQ(result127.value(), 127);

    auto result128 = serializer.readVarInt();
    EXPECT_TRUE(result128.success());
    EXPECT_EQ(result128.value(), 128);

    auto result16383 = serializer.readVarInt();
    EXPECT_TRUE(result16383.success());
    EXPECT_EQ(result16383.value(), 16383);

    auto result16384 = serializer.readVarInt();
    EXPECT_TRUE(result16384.success());
    EXPECT_EQ(result16384.value(), 16384);

    auto result2097151 = serializer.readVarInt();
    EXPECT_TRUE(result2097151.success());
    EXPECT_EQ(result2097151.value(), 2097151);

    auto result2097152 = serializer.readVarInt();
    EXPECT_TRUE(result2097152.success());
    EXPECT_EQ(result2097152.value(), 2097152);

    auto resultNeg1 = serializer.readVarInt();
    EXPECT_TRUE(resultNeg1.success());
    EXPECT_EQ(resultNeg1.value(), -1);

    auto resultMin = serializer.readVarInt();
    EXPECT_TRUE(resultMin.success());
    EXPECT_EQ(resultMin.value(), -2147483647 - 1);
}

TEST(PacketSerializer, VarIntSize)
{
    // VarInt 编码后的大小应该是可变的
    PacketSerializer serializer;

    // 0-127: 1字节
    size_t start = serializer.size();
    serializer.writeVarInt(0);
    EXPECT_EQ(serializer.size() - start, 1u);

    serializer.clear();
    start = serializer.size();
    serializer.writeVarInt(127);
    EXPECT_EQ(serializer.size() - start, 1u);

    // 128-16383: 2字节
    serializer.clear();
    start = serializer.size();
    serializer.writeVarInt(128);
    EXPECT_EQ(serializer.size() - start, 2u);

    serializer.clear();
    start = serializer.size();
    serializer.writeVarInt(16383);
    EXPECT_EQ(serializer.size() - start, 2u);

    // 16384-2097151: 3字节
    serializer.clear();
    start = serializer.size();
    serializer.writeVarInt(16384);
    EXPECT_EQ(serializer.size() - start, 3u);
}

TEST(PacketSerializer, WriteReadVarLong)
{
    PacketSerializer serializer;

    serializer.writeVarLong(0);
    serializer.writeVarLong(9223372036854775807LL); // LONG_MAX
    serializer.writeVarLong(-1);
    serializer.writeVarLong(-9223372036854775807LL - 1); // LONG_MIN

    auto result0 = serializer.readVarLong();
    EXPECT_TRUE(result0.success());
    EXPECT_EQ(result0.value(), 0);

    auto resultMax = serializer.readVarLong();
    EXPECT_TRUE(resultMax.success());
    EXPECT_EQ(resultMax.value(), 9223372036854775807LL);

    auto resultNeg1 = serializer.readVarLong();
    EXPECT_TRUE(resultNeg1.success());
    EXPECT_EQ(resultNeg1.value(), -1);

    auto resultMin = serializer.readVarLong();
    EXPECT_TRUE(resultMin.success());
    EXPECT_EQ(resultMin.value(), -9223372036854775807LL - 1);
}
