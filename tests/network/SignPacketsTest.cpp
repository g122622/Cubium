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

/**
 * @file SignPacketsTest.cpp
 * @brief 告示牌网络包序列化/反序列化测试
 *
 * 测试 OpenSignEditorPacket 和 UpdateSignPacket 的序列化与反序列化：
 * - 基础字段读写（pos、isFrontSide、lines）
 * - 往返一致性（serialize → deserialize 字段不变）
 * - 边界条件（空字符串、最大长度字符串、负坐标）
 * - 错误处理（截断数据、空数据）
 */

#include <gtest/gtest.h>

#include "common/network/packet/PacketDeserializer.hpp"
#include "common/network/packet/PacketSerializer.hpp"
#include "common/network/packet/SignPackets.hpp"

using namespace mc::network;
using mc::BlockPos;
using mc::i32;

// ============================================================================
// OpenSignEditorPacket 测试
// ============================================================================

TEST(OpenSignEditorPacketTest, DefaultConstructorHasDefaults)
{
    OpenSignEditorPacket packet;
    EXPECT_EQ(packet.pos(), BlockPos(0, 0, 0));
    EXPECT_TRUE(packet.isFrontSide());
}

TEST(OpenSignEditorPacketTest, ParameterConstructorSetsFields)
{
    OpenSignEditorPacket packet(BlockPos(10, 64, -20), false);
    EXPECT_EQ(packet.pos(), BlockPos(10, 64, -20));
    EXPECT_FALSE(packet.isFrontSide());
}

TEST(OpenSignEditorPacketTest, SettersModifyFields)
{
    OpenSignEditorPacket packet;
    packet.setPos(BlockPos(1, 2, 3));
    packet.setIsFrontSide(false);
    EXPECT_EQ(packet.pos(), BlockPos(1, 2, 3));
    EXPECT_FALSE(packet.isFrontSide());
}

TEST(OpenSignEditorPacketTest, SerializeDeserializeRoundTrip)
{
    OpenSignEditorPacket original(BlockPos(100, -50, 200), true);

    PacketSerializer ser;
    original.serialize(ser);
    EXPECT_GT(ser.size(), 0u);

    PacketDeserializer deser(ser.data(), ser.size());
    auto result = OpenSignEditorPacket::deserialize(deser);
    ASSERT_TRUE(result.success()) << result.error().message();

    const auto& packet = result.value();
    EXPECT_EQ(packet.pos(), BlockPos(100, -50, 200));
    EXPECT_TRUE(packet.isFrontSide());
}

TEST(OpenSignEditorPacketTest, SerializeDeserializeFrontSideFalse)
{
    OpenSignEditorPacket original(BlockPos(0, 0, 0), false);

    PacketSerializer ser;
    original.serialize(ser);

    PacketDeserializer deser(ser.data(), ser.size());
    auto result = OpenSignEditorPacket::deserialize(deser);
    ASSERT_TRUE(result.success());
    EXPECT_FALSE(result.value().isFrontSide());
}

TEST(OpenSignEditorPacketTest, SerializeDeserializeNegativeCoordinates)
{
    OpenSignEditorPacket original(BlockPos(-1000, -64, -2000), true);

    PacketSerializer ser;
    original.serialize(ser);

    PacketDeserializer deser(ser.data(), ser.size());
    auto result = OpenSignEditorPacket::deserialize(deser);
    ASSERT_TRUE(result.success());
    EXPECT_EQ(result.value().pos(), BlockPos(-1000, -64, -2000));
}

TEST(OpenSignEditorPacketTest, DeserializeEmptyDataFails)
{
    PacketDeserializer deser(nullptr, 0);
    auto result = OpenSignEditorPacket::deserialize(deser);
    EXPECT_FALSE(result.success());
}

TEST(OpenSignEditorPacketTest, DeserializeTruncatedDataFails)
{
    // 只写入 x 和 y，缺少 z 和 isFrontSide
    PacketSerializer ser;
    ser.writeI32(10);
    ser.writeI32(20);

    PacketDeserializer deser(ser.data(), ser.size());
    auto result = OpenSignEditorPacket::deserialize(deser);
    EXPECT_FALSE(result.success());
}

// ============================================================================
// UpdateSignPacket 测试
// ============================================================================

TEST(UpdateSignPacketTest, DefaultConstructorHasDefaults)
{
    UpdateSignPacket packet;
    EXPECT_EQ(packet.pos(), BlockPos(0, 0, 0));
    EXPECT_TRUE(packet.isFrontSide());
    EXPECT_EQ(packet.lines().size(), static_cast<size_t>(UpdateSignPacket::LINE_COUNT));
    for (const auto& line : packet.lines()) {
        EXPECT_TRUE(line.empty());
    }
}

TEST(UpdateSignPacketTest, ParameterConstructorSetsFields)
{
    std::array<std::string, UpdateSignPacket::LINE_COUNT> lines = {"Hello", "World", "Sign", "Test"};
    UpdateSignPacket packet(BlockPos(5, 70, 15), lines, false);

    EXPECT_EQ(packet.pos(), BlockPos(5, 70, 15));
    EXPECT_FALSE(packet.isFrontSide());
    for (i32 i = 0; i < UpdateSignPacket::LINE_COUNT; ++i) {
        EXPECT_EQ(packet.lines()[static_cast<size_t>(i)], lines[static_cast<size_t>(i)]);
    }
}

TEST(UpdateSignPacketTest, SettersModifyFields)
{
    UpdateSignPacket packet;
    packet.setPos(BlockPos(7, 8, 9));
    packet.setIsFrontSide(false);
    std::array<std::string, UpdateSignPacket::LINE_COUNT> lines = {"A", "B", "C", "D"};
    packet.setLines(lines);

    EXPECT_EQ(packet.pos(), BlockPos(7, 8, 9));
    EXPECT_FALSE(packet.isFrontSide());
    for (i32 i = 0; i < UpdateSignPacket::LINE_COUNT; ++i) {
        EXPECT_EQ(packet.lines()[static_cast<size_t>(i)], lines[static_cast<size_t>(i)]);
    }
}

TEST(UpdateSignPacketTest, SerializeDeserializeRoundTrip)
{
    std::array<std::string, UpdateSignPacket::LINE_COUNT> lines = {"Line 1", "Line 2", "Line 3", "Line 4"};
    UpdateSignPacket original(BlockPos(32, 64, -32), lines, true);

    PacketSerializer ser;
    original.serialize(ser);
    EXPECT_GT(ser.size(), 0u);

    PacketDeserializer deser(ser.data(), ser.size());
    auto result = UpdateSignPacket::deserialize(deser);
    ASSERT_TRUE(result.success()) << result.error().message();

    const auto& packet = result.value();
    EXPECT_EQ(packet.pos(), BlockPos(32, 64, -32));
    EXPECT_TRUE(packet.isFrontSide());
    for (i32 i = 0; i < UpdateSignPacket::LINE_COUNT; ++i) {
        EXPECT_EQ(packet.lines()[static_cast<size_t>(i)], lines[static_cast<size_t>(i)]);
    }
}

TEST(UpdateSignPacketTest, SerializeDeserializeEmptyLines)
{
    std::array<std::string, UpdateSignPacket::LINE_COUNT> lines = {"", "", "", ""};
    UpdateSignPacket original(BlockPos(0, 0, 0), lines, false);

    PacketSerializer ser;
    original.serialize(ser);

    PacketDeserializer deser(ser.data(), ser.size());
    auto result = UpdateSignPacket::deserialize(deser);
    ASSERT_TRUE(result.success());
    for (i32 i = 0; i < UpdateSignPacket::LINE_COUNT; ++i) {
        EXPECT_TRUE(result.value().lines()[static_cast<size_t>(i)].empty());
    }
}

TEST(UpdateSignPacketTest, SerializeDeserializeMaxLineLength)
{
    // 每行 15 字符（MAX_LINE_LENGTH）
    const std::string maxLine(15, 'X');
    std::array<std::string, UpdateSignPacket::LINE_COUNT> lines = {maxLine, maxLine, maxLine, maxLine};
    UpdateSignPacket original(BlockPos(1, 2, 3), lines, true);

    PacketSerializer ser;
    original.serialize(ser);

    PacketDeserializer deser(ser.data(), ser.size());
    auto result = UpdateSignPacket::deserialize(deser);
    ASSERT_TRUE(result.success());
    for (i32 i = 0; i < UpdateSignPacket::LINE_COUNT; ++i) {
        EXPECT_EQ(result.value().lines()[static_cast<size_t>(i)], maxLine);
    }
}

TEST(UpdateSignPacketTest, SerializeDeserializeUnicodeLines)
{
    // Unicode 字符串（中文）
    std::array<std::string, UpdateSignPacket::LINE_COUNT> lines = {"你好", "世界", "告示牌", "测试"};
    UpdateSignPacket original(BlockPos(10, 20, 30), lines, true);

    PacketSerializer ser;
    original.serialize(ser);

    PacketDeserializer deser(ser.data(), ser.size());
    auto result = UpdateSignPacket::deserialize(deser);
    ASSERT_TRUE(result.success());
    for (i32 i = 0; i < UpdateSignPacket::LINE_COUNT; ++i) {
        EXPECT_EQ(result.value().lines()[static_cast<size_t>(i)], lines[static_cast<size_t>(i)]);
    }
}

TEST(UpdateSignPacketTest, DeserializeEmptyDataFails)
{
    PacketDeserializer deser(nullptr, 0);
    auto result = UpdateSignPacket::deserialize(deser);
    EXPECT_FALSE(result.success());
}

TEST(UpdateSignPacketTest, DeserializeTruncatedDataFails)
{
    // 只写入 pos，缺少 lines 和 isFrontSide
    PacketSerializer ser;
    ser.writeI32(10);
    ser.writeI32(20);
    ser.writeI32(30);

    PacketDeserializer deser(ser.data(), ser.size());
    auto result = UpdateSignPacket::deserialize(deser);
    EXPECT_FALSE(result.success());
}

TEST(UpdateSignPacketTest, DeserializePartialLinesFails)
{
    // 写入 pos + 2行文本，缺少后2行和 isFrontSide
    PacketSerializer ser;
    ser.writeI32(10);
    ser.writeI32(20);
    ser.writeI32(30);
    ser.writeString("Line 1");
    ser.writeString("Line 2");

    PacketDeserializer deser(ser.data(), ser.size());
    auto result = UpdateSignPacket::deserialize(deser);
    EXPECT_FALSE(result.success());
}
