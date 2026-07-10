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
 * @file BlockEntityDataPacketTest.cpp
 * @brief 方块实体数据同步包序列化/反序列化测试
 *
 * 测试 BlockEntityDataPacket 的序列化与反序列化：
 * - 基础字段读写（pos、type、nbtData）
 * - 往返一致性（serialize → deserialize 字段不变）
 * - 边界条件（空 nbtData、负坐标、未知 BlockEntityType）
 * - 错误处理（截断数据、空数据、NBT 字节越界）
 * - 辅助方法（parseNbt / serializeNbtToBytes / deserializeNbtFromBytes）
 */

#include <gtest/gtest.h>

#include "common/network/packet/BlockEntityDataPacket.hpp"
#include "common/network/packet/PacketDeserializer.hpp"
#include "common/network/packet/PacketSerializer.hpp"
#include "common/util/nbt/Nbt.hpp"

using namespace mc::network;
using mc::BlockEntityType;
using mc::BlockPos;
using mc::i32;
using mc::i8;
using mc::u16;
using mc::u32;
using mc::u8;
using mc::nbt::CompoundTag;

// ============================================================================
// 默认构造与 Getters/Setters 测试
// ============================================================================

TEST(BlockEntityDataPacketTest, DefaultConstructorHasDefaults)
{
    BlockEntityDataPacket packet;
    EXPECT_EQ(packet.pos(), BlockPos(0, 0, 0));
    EXPECT_EQ(packet.type(), BlockEntityType::Unknown);
    EXPECT_TRUE(packet.nbtData().empty());
}

TEST(BlockEntityDataPacketTest, ParameterConstructorWithBytesSetsFields)
{
    std::vector<u8> data = {0x01, 0x02, 0x03, 0x04};
    BlockEntityDataPacket packet(BlockPos(10, 64, -20), BlockEntityType::Sign, data);

    EXPECT_EQ(packet.pos(), BlockPos(10, 64, -20));
    EXPECT_EQ(packet.type(), BlockEntityType::Sign);
    EXPECT_EQ(packet.nbtData(), data);
}

TEST(BlockEntityDataPacketTest, ParameterConstructorWithNbtTagSerializesToBytes)
{
    CompoundTag tag;
    tag.put("id", std::string("minecraft:sign"));
    tag.put("x", 10);
    tag.put("y", 64);
    tag.put("z", -20);

    BlockEntityDataPacket packet(BlockPos(10, 64, -20), BlockEntityType::Sign, tag);

    EXPECT_EQ(packet.pos(), BlockPos(10, 64, -20));
    EXPECT_EQ(packet.type(), BlockEntityType::Sign);
    // NBT 字节流不应为空（至少包含复合标签头和字段）
    EXPECT_FALSE(packet.nbtData().empty());
}

TEST(BlockEntityDataPacketTest, SettersModifyFields)
{
    BlockEntityDataPacket packet;
    packet.setPos(BlockPos(1, 2, 3));
    packet.setType(BlockEntityType::Chest);
    packet.setNbtData({0xAA, 0xBB, 0xCC});

    EXPECT_EQ(packet.pos(), BlockPos(1, 2, 3));
    EXPECT_EQ(packet.type(), BlockEntityType::Chest);
    EXPECT_EQ(packet.nbtData(), (std::vector<u8>{0xAA, 0xBB, 0xCC}));
}

TEST(BlockEntityDataPacketTest, SetNbtDataMovesData)
{
    BlockEntityDataPacket packet;
    std::vector<u8> data = {0x10, 0x20};
    packet.setNbtData(std::move(data));
    EXPECT_EQ(packet.nbtData(), (std::vector<u8>{0x10, 0x20}));
}

// ============================================================================
// serialize/deserialize 往返测试
// ============================================================================

TEST(BlockEntityDataPacketTest, SerializeDeserializeRoundTripWithNbt)
{
    CompoundTag tag;
    tag.put("id", std::string("minecraft:sign"));
    tag.put("x", 100);
    tag.put("y", 64);
    tag.put("z", -50);
    tag.put("Text1", std::string("Hello"));
    tag.put("Text2", std::string("World"));

    BlockEntityDataPacket original(BlockPos(100, 64, -50), BlockEntityType::Sign, tag);

    PacketSerializer ser;
    original.serialize(ser);
    EXPECT_GT(ser.size(), 0u);

    PacketDeserializer deser(ser.data(), ser.size());
    auto result = BlockEntityDataPacket::deserialize(deser);
    ASSERT_TRUE(result.success()) << result.error().message();

    const auto& packet = result.value();
    EXPECT_EQ(packet.pos(), BlockPos(100, 64, -50));
    EXPECT_EQ(packet.type(), BlockEntityType::Sign);
    EXPECT_FALSE(packet.nbtData().empty());
    // 字节流应与原始序列化结果一致
    EXPECT_EQ(packet.nbtData(), original.nbtData());
}

TEST(BlockEntityDataPacketTest, SerializeDeserializeRoundTripWithEmptyNbt)
{
    // 空 nbtData 边界情况
    std::vector<u8> emptyData;
    BlockEntityDataPacket original(BlockPos(0, 0, 0), BlockEntityType::Chest, emptyData);

    PacketSerializer ser;
    original.serialize(ser);

    PacketDeserializer deser(ser.data(), ser.size());
    auto result = BlockEntityDataPacket::deserialize(deser);
    ASSERT_TRUE(result.success()) << result.error().message();

    const auto& packet = result.value();
    EXPECT_EQ(packet.pos(), BlockPos(0, 0, 0));
    EXPECT_EQ(packet.type(), BlockEntityType::Chest);
    EXPECT_TRUE(packet.nbtData().empty());
}

TEST(BlockEntityDataPacketTest, SerializeDeserializeNegativeCoordinates)
{
    std::vector<u8> data = {0xFF};
    BlockEntityDataPacket original(BlockPos(-1000, -64, -2000), BlockEntityType::Furnace, data);

    PacketSerializer ser;
    original.serialize(ser);

    PacketDeserializer deser(ser.data(), ser.size());
    auto result = BlockEntityDataPacket::deserialize(deser);
    ASSERT_TRUE(result.success());
    EXPECT_EQ(result.value().pos(), BlockPos(-1000, -64, -2000));
}

TEST(BlockEntityDataPacketTest, SerializeDeserializeUnknownBlockEntityType)
{
    // 未知类型仍然可以序列化/反序列化（类型仅作为 u16 传输）
    std::vector<u8> data = {0x01};
    BlockEntityDataPacket original(BlockPos(0, 0, 0), BlockEntityType::Unknown, data);

    PacketSerializer ser;
    original.serialize(ser);

    PacketDeserializer deser(ser.data(), ser.size());
    auto result = BlockEntityDataPacket::deserialize(deser);
    ASSERT_TRUE(result.success());
    EXPECT_EQ(result.value().type(), BlockEntityType::Unknown);
}

TEST(BlockEntityDataPacketTest, SerializeDeserializeLargeNbtData)
{
    // 较大的 NBT 数据（超过 128 字节，触发 VarUInt 多字节编码）
    std::vector<u8> largeData(512, 0x42);
    BlockEntityDataPacket original(BlockPos(10, 20, 30), BlockEntityType::Chest, largeData);

    PacketSerializer ser;
    original.serialize(ser);

    PacketDeserializer deser(ser.data(), ser.size());
    auto result = BlockEntityDataPacket::deserialize(deser);
    ASSERT_TRUE(result.success());
    EXPECT_EQ(result.value().nbtData().size(), largeData.size());
    EXPECT_EQ(result.value().nbtData(), largeData);
}

TEST(BlockEntityDataPacketTest, SerializedLayoutMatchesProtocolSpec)
{
    // 协议格式：x(i32) + y(i32) + z(i32) + type(u16) + nbtLen(varuint) + nbtBytes
    // 空字节流时：4 + 4 + 4 + 2 + 1 = 15 字节
    BlockEntityDataPacket packet(BlockPos(1, 2, 3), BlockEntityType::Sign, std::vector<u8>{});

    PacketSerializer ser;
    packet.serialize(ser);

    // 空字节流的包体大小应为 15 字节
    EXPECT_EQ(ser.size(), 15u);
}

// ============================================================================
// 反序列化错误处理测试
// ============================================================================

TEST(BlockEntityDataPacketTest, DeserializeEmptyDataFails)
{
    PacketDeserializer deser(nullptr, 0);
    auto result = BlockEntityDataPacket::deserialize(deser);
    EXPECT_FALSE(result.success());
}

TEST(BlockEntityDataPacketTest, DeserializeTruncatedBeforeZFails)
{
    // 只写入 x 和 y，缺少 z、type 等
    PacketSerializer ser;
    ser.writeI32(10);
    ser.writeI32(20);

    PacketDeserializer deser(ser.data(), ser.size());
    auto result = BlockEntityDataPacket::deserialize(deser);
    EXPECT_FALSE(result.success());
}

TEST(BlockEntityDataPacketTest, DeserializeTruncatedBeforeTypeFails)
{
    // 只写入 x、y、z，缺少 type
    PacketSerializer ser;
    ser.writeI32(10);
    ser.writeI32(20);
    ser.writeI32(30);

    PacketDeserializer deser(ser.data(), ser.size());
    auto result = BlockEntityDataPacket::deserialize(deser);
    EXPECT_FALSE(result.success());
}

TEST(BlockEntityDataPacketTest, DeserializeTruncatedBeforeNbtLenFails)
{
    // 写入 x、y、z、type，缺少 nbtLen
    PacketSerializer ser;
    ser.writeI32(10);
    ser.writeI32(20);
    ser.writeI32(30);
    ser.writeU16(static_cast<u16>(BlockEntityType::Sign));

    PacketDeserializer deser(ser.data(), ser.size());
    auto result = BlockEntityDataPacket::deserialize(deser);
    EXPECT_FALSE(result.success());
}

TEST(BlockEntityDataPacketTest, DeserializeNbtBytesExceedPacketBoundsFails)
{
    // nbtLen 声明 100 字节，但实际数据只有 5 字节
    PacketSerializer ser;
    ser.writeI32(10);
    ser.writeI32(20);
    ser.writeI32(30);
    ser.writeU16(static_cast<u16>(BlockEntityType::Sign));
    ser.writeVarUInt(100);
    ser.writeBytes({0x01, 0x02, 0x03, 0x04, 0x05});

    PacketDeserializer deser(ser.data(), ser.size());
    auto result = BlockEntityDataPacket::deserialize(deser);
    EXPECT_FALSE(result.success());
}

TEST(BlockEntityDataPacketTest, DeserializeZeroNbtLenSucceedsWithEmptyData)
{
    // nbtLen 为 0 时不应读取任何字节
    PacketSerializer ser;
    ser.writeI32(10);
    ser.writeI32(20);
    ser.writeI32(30);
    ser.writeU16(static_cast<u16>(BlockEntityType::Sign));
    ser.writeVarUInt(0);

    PacketDeserializer deser(ser.data(), ser.size());
    auto result = BlockEntityDataPacket::deserialize(deser);
    ASSERT_TRUE(result.success());
    EXPECT_TRUE(result.value().nbtData().empty());
}

// ============================================================================
// parseNbt 辅助方法测试
// ============================================================================

TEST(BlockEntityDataPacketTest, ParseNbtReturnsValidCompoundTag)
{
    CompoundTag originalTag;
    originalTag.put("id", std::string("minecraft:sign"));
    originalTag.put("x", 10);
    originalTag.put("y", 64);
    originalTag.put("z", -20);

    BlockEntityDataPacket packet(BlockPos(10, 64, -20), BlockEntityType::Sign, originalTag);

    auto parseResult = packet.parseNbt();
    ASSERT_TRUE(parseResult.success()) << parseResult.error().message();

    const auto& tag = parseResult.value();
    EXPECT_EQ(tag.get<mc::nbt::tags::string_tag>("id"), "minecraft:sign");
    EXPECT_EQ(tag.get<mc::nbt::tags::int_tag>("x"), 10);
    EXPECT_EQ(tag.get<mc::nbt::tags::int_tag>("y"), 64);
    EXPECT_EQ(tag.get<mc::nbt::tags::int_tag>("z"), -20);
}

TEST(BlockEntityDataPacketTest, ParseNbtFailsOnEmptyData)
{
    BlockEntityDataPacket packet(BlockPos(0, 0, 0), BlockEntityType::Unknown, std::vector<u8>{});

    auto result = packet.parseNbt();
    EXPECT_FALSE(result.success());
}

TEST(BlockEntityDataPacketTest, ParseNbtFailsOnInvalidBytes)
{
    // 非法 NBT 字节流（首字节不是合法的标签 ID）
    std::vector<u8> invalidData = {0xFF, 0xFF, 0xFF, 0xFF};
    BlockEntityDataPacket packet(BlockPos(0, 0, 0), BlockEntityType::Unknown, invalidData);

    auto result = packet.parseNbt();
    EXPECT_FALSE(result.success());
}

// ============================================================================
// serializeNbtToBytes / deserializeNbtFromBytes 辅助方法测试
// ============================================================================

TEST(BlockEntityDataPacketTest, SerializeNbtToBytesReturnsNonEmptyForValidTag)
{
    CompoundTag tag;
    tag.put("name", std::string("test"));

    auto bytes = BlockEntityDataPacket::serializeNbtToBytes(tag);
    EXPECT_FALSE(bytes.empty());
}

TEST(BlockEntityDataPacketTest, SerializeAndDeserializeNbtBytesRoundTrip)
{
    CompoundTag original;
    original.put("id", std::string("minecraft:chest"));
    original.put("x", 100);
    original.put("y", -64);
    original.put("z", 200);
    original.put("CustomName", std::string("My Chest"));

    auto bytes = BlockEntityDataPacket::serializeNbtToBytes(original);
    ASSERT_FALSE(bytes.empty());

    auto result = BlockEntityDataPacket::deserializeNbtFromBytes(bytes);
    ASSERT_TRUE(result.success()) << result.error().message();

    const auto& restored = result.value();
    EXPECT_EQ(restored.get<mc::nbt::tags::string_tag>("id"), "minecraft:chest");
    EXPECT_EQ(restored.get<mc::nbt::tags::int_tag>("x"), 100);
    EXPECT_EQ(restored.get<mc::nbt::tags::int_tag>("y"), -64);
    EXPECT_EQ(restored.get<mc::nbt::tags::int_tag>("z"), 200);
    EXPECT_EQ(restored.get<mc::nbt::tags::string_tag>("CustomName"), "My Chest");
}

TEST(BlockEntityDataPacketTest, DeserializeNbtFromBytesFailsOnEmptyInput)
{
    std::vector<u8> empty;
    auto result = BlockEntityDataPacket::deserializeNbtFromBytes(empty);
    EXPECT_FALSE(result.success());
}

TEST(BlockEntityDataPacketTest, DeserializeNbtFromBytesFailsOnGarbageInput)
{
    std::vector<u8> garbage = {0xDE, 0xAD, 0xBE, 0xEF, 0x00, 0x00, 0x00};
    auto result = BlockEntityDataPacket::deserializeNbtFromBytes(garbage);
    EXPECT_FALSE(result.success());
}

TEST(BlockEntityDataPacketTest, SerializeNbtToBytesProducesJavaEndianFormat)
{
    // Java 版 NBT 使用大端序，整型字段的高字节在前
    CompoundTag tag;
    tag.put("value", 0x01020304); // int_tag

    auto bytes = BlockEntityDataPacket::serializeNbtToBytes(tag);
    ASSERT_FALSE(bytes.empty());

    // 字节流应包含 0x01 0x02 0x03 0x04 的大端序表示
    // 不验证精确偏移（依赖 NBT 库内部布局），只验证字节序列存在
    bool foundBigEndian = false;
    for (size_t i = 0; i + 3 < bytes.size(); ++i) {
        if (bytes[i] == 0x01 && bytes[i + 1] == 0x02 && bytes[i + 2] == 0x03 && bytes[i + 3] == 0x04) {
            foundBigEndian = true;
            break;
        }
    }
    EXPECT_TRUE(foundBigEndian) << "Java edition NBT should use big-endian byte order";
}

// ============================================================================
// 完整流程集成测试
// ============================================================================

TEST(BlockEntityDataPacketTest, FullRoundTripPreservesComplexNbtStructure)
{
    // 构造包含多种字段类型的复杂 NBT 结构
    CompoundTag original;
    original.put("id", std::string("minecraft:sign"));
    original.put("x", 100);
    original.put("y", 64);
    original.put("z", -50);
    original.put("Text1", std::string("Line 1"));
    original.put("Text2", std::string("Line 2"));
    original.put("Text3", std::string("Line 3"));
    original.put("Text4", std::string("Line 4"));
    original.put("Color", std::string("black"));
    original.put("GlowingText", static_cast<i8>(0));

    // 序列化 NBT → bytes → packet → serialize → deserialize → parseNbt
    auto nbtBytes = BlockEntityDataPacket::serializeNbtToBytes(original);
    ASSERT_FALSE(nbtBytes.empty());

    BlockEntityDataPacket packet(BlockPos(100, 64, -50), BlockEntityType::Sign, nbtBytes);

    PacketSerializer ser;
    packet.serialize(ser);

    PacketDeserializer deser(ser.data(), ser.size());
    auto deserResult = BlockEntityDataPacket::deserialize(deser);
    ASSERT_TRUE(deserResult.success()) << deserResult.error().message();

    auto parseResult = deserResult.value().parseNbt();
    ASSERT_TRUE(parseResult.success()) << parseResult.error().message();

    const auto& restored = parseResult.value();
    EXPECT_EQ(restored.get<mc::nbt::tags::string_tag>("id"), "minecraft:sign");
    EXPECT_EQ(restored.get<mc::nbt::tags::int_tag>("x"), 100);
    EXPECT_EQ(restored.get<mc::nbt::tags::int_tag>("y"), 64);
    EXPECT_EQ(restored.get<mc::nbt::tags::int_tag>("z"), -50);
    EXPECT_EQ(restored.get<mc::nbt::tags::string_tag>("Text1"), "Line 1");
    EXPECT_EQ(restored.get<mc::nbt::tags::string_tag>("Text2"), "Line 2");
    EXPECT_EQ(restored.get<mc::nbt::tags::string_tag>("Text3"), "Line 3");
    EXPECT_EQ(restored.get<mc::nbt::tags::string_tag>("Text4"), "Line 4");
    EXPECT_EQ(restored.get<mc::nbt::tags::string_tag>("Color"), "black");
}
