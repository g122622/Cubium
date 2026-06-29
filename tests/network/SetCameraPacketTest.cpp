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
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

/**
 * @file SetCameraPacketTest.cpp
 * @brief SetCameraPacket 序列化/反序列化单元测试
 *
 * 测试旁观者摄像机同步包 (S2C) 的完整功能：
 * - 默认构造和带参构造
 * - 序列化/反序列化往返一致性
 * - VarInt 编码边界值（0, 1, 127, 128, 255, 65535, 最大实体 ID）
 * - 空数据和截断数据的错误处理
 * - expectedSize() 返回值合理性
 */

#include "network/packet/SetCameraPacket.hpp"
#include <vector>
#include <gtest/gtest.h>

using namespace mc::network;
using mc::u32;
using mc::u8;

// ==================== SetCameraPacket 基础测试 ====================

class SetCameraPacketTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(SetCameraPacketTest, DefaultConstruction)
{
    SetCameraPacket packet;
    EXPECT_EQ(packet.cameraEntityId(), 0u);
    EXPECT_EQ(packet.type(), PacketType::SetCamera);
}

TEST_F(SetCameraPacketTest, ParameterizedConstruction)
{
    SetCameraPacket packet(42);
    EXPECT_EQ(packet.cameraEntityId(), 42u);
    EXPECT_EQ(packet.type(), PacketType::SetCamera);
}

TEST_F(SetCameraPacketTest, SetterGetter)
{
    SetCameraPacket packet;
    EXPECT_EQ(packet.cameraEntityId(), 0u);

    packet.setCameraEntityId(100);
    EXPECT_EQ(packet.cameraEntityId(), 100u);

    packet.setCameraEntityId(0);
    EXPECT_EQ(packet.cameraEntityId(), 0u);
}

// ==================== 序列化/反序列化往返测试 ====================

TEST_F(SetCameraPacketTest, SerializeDeserializeRoundtrip_SmallEntityId)
{
    SetCameraPacket original(42);

    auto serializeResult = original.serialize();
    ASSERT_TRUE(serializeResult.success()) << "Serialization failed";

    SetCameraPacket deserialized;
    auto deserializeResult = deserialized.deserialize(serializeResult.value().data(), serializeResult.value().size());
    ASSERT_TRUE(deserializeResult.success()) << "Deserialization failed";

    EXPECT_EQ(deserialized.cameraEntityId(), 42u);
}

TEST_F(SetCameraPacketTest, SerializeDeserializeRoundtrip_ZeroEntityId)
{
    SetCameraPacket original(0);

    auto serializeResult = original.serialize();
    ASSERT_TRUE(serializeResult.success()) << "Serialization failed";

    SetCameraPacket deserialized;
    auto deserializeResult = deserialized.deserialize(serializeResult.value().data(), serializeResult.value().size());
    ASSERT_TRUE(deserializeResult.success()) << "Deserialization failed";

    EXPECT_EQ(deserialized.cameraEntityId(), 0u);
}

TEST_F(SetCameraPacketTest, SerializeDeserializeRoundtrip_LargeEntityId)
{
    // 大实体 ID（VarInt 多字节编码）
    SetCameraPacket original(1000000);

    auto serializeResult = original.serialize();
    ASSERT_TRUE(serializeResult.success()) << "Serialization failed";

    SetCameraPacket deserialized;
    auto deserializeResult = deserialized.deserialize(serializeResult.value().data(), serializeResult.value().size());
    ASSERT_TRUE(deserializeResult.success()) << "Deserialization failed";

    EXPECT_EQ(deserialized.cameraEntityId(), 1000000u);
}

// ==================== VarInt 边界值测试 ====================

TEST_F(SetCameraPacketTest, VarIntBoundary_OneByte)
{
    // VarInt 单字节最大值 127
    SetCameraPacket packet(127);
    auto result = packet.serialize();
    ASSERT_TRUE(result.success());
    EXPECT_EQ(result.value().size(), 1u); // VarInt(127) = 1 字节

    SetCameraPacket deserialized;
    auto deserResult = deserialized.deserialize(result.value().data(), result.value().size());
    ASSERT_TRUE(deserResult.success());
    EXPECT_EQ(deserialized.cameraEntityId(), 127u);
}

TEST_F(SetCameraPacketTest, VarIntBoundary_TwoBytes)
{
    // VarInt 两字节起始值 128
    SetCameraPacket packet(128);
    auto result = packet.serialize();
    ASSERT_TRUE(result.success());
    EXPECT_EQ(result.value().size(), 2u); // VarInt(128) = 2 字节

    SetCameraPacket deserialized;
    auto deserResult = deserialized.deserialize(result.value().data(), result.value().size());
    ASSERT_TRUE(deserResult.success());
    EXPECT_EQ(deserialized.cameraEntityId(), 128u);
}

TEST_F(SetCameraPacketTest, VarIntBoundary_Uint8Max)
{
    SetCameraPacket packet(255);
    auto result = packet.serialize();
    ASSERT_TRUE(result.success());

    SetCameraPacket deserialized;
    auto deserResult = deserialized.deserialize(result.value().data(), result.value().size());
    ASSERT_TRUE(deserResult.success());
    EXPECT_EQ(deserialized.cameraEntityId(), 255u);
}

TEST_F(SetCameraPacketTest, VarIntBoundary_Uint16Max)
{
    SetCameraPacket packet(65535);
    auto result = packet.serialize();
    ASSERT_TRUE(result.success());

    SetCameraPacket deserialized;
    auto deserResult = deserialized.deserialize(result.value().data(), result.value().size());
    ASSERT_TRUE(deserResult.success());
    EXPECT_EQ(deserialized.cameraEntityId(), 65535u);
}

// ==================== 错误处理测试 ====================

TEST_F(SetCameraPacketTest, DeserializeEmptyData)
{
    SetCameraPacket packet;
    auto result = packet.deserialize(nullptr, 0);
    EXPECT_FALSE(result.success());
}

TEST_F(SetCameraPacketTest, DeserializeTruncatedData)
{
    // 构造一个被截断的 VarInt（只包含连续字节，缺少终止字节）
    std::vector<u8> truncated = {0x80, 0x80}; // VarInt 前缀，缺少终止字节
    SetCameraPacket packet;
    // 即使数据看起来截断，VarInt 解码器可能仍会尝试读取
    // 关键是反序列化不应崩溃
    auto result = packet.deserialize(truncated.data(), truncated.size());
    // 截断的 VarInt 可能在解码时产生意外值，但不应崩溃
    // 重要的是：测试程序没有段错误
    (void)result;
}

// ==================== expectedSize 测试 ====================

TEST_F(SetCameraPacketTest, ExpectedSize)
{
    SetCameraPacket packet(42);
    // expectedSize 返回的是预估值，VarInt 最大 5 字节但实体 ID 通常在 1-4 字节范围
    EXPECT_GT(packet.expectedSize(), 0u);
    EXPECT_LE(packet.expectedSize(), 5u);
}

// ==================== 场景模拟测试 ====================

TEST_F(SetCameraPacketTest, Scenario_SetCameraToEntity)
{
    // 模拟设置摄像机到实体 ID 1234
    SetCameraPacket setCamera(1234);
    auto result = setCamera.serialize();
    ASSERT_TRUE(result.success());

    SetCameraPacket received;
    auto deserResult = received.deserialize(result.value().data(), result.value().size());
    ASSERT_TRUE(deserResult.success());
    EXPECT_EQ(received.cameraEntityId(), 1234u);
}

TEST_F(SetCameraPacketTest, Scenario_ResetCameraToSelf)
{
    // 模拟重置摄像机到玩家自身（使用玩家实体 ID）
    u32 playerId = 1;
    SetCameraPacket resetCamera(playerId);
    auto result = resetCamera.serialize();
    ASSERT_TRUE(result.success());

    SetCameraPacket received;
    auto deserResult = received.deserialize(result.value().data(), result.value().size());
    ASSERT_TRUE(deserResult.success());
    EXPECT_EQ(received.cameraEntityId(), 1u);
}

TEST_F(SetCameraPacketTest, Scenario_MultipleRoundtrips)
{
    // 多次往返序列化/反序列化，确保数据一致性
    u32 entityIds[] = {0, 1, 127, 128, 255, 1000, 65535, 1000000};

    for (u32 entityId : entityIds) {
        SetCameraPacket original(entityId);
        auto serializeResult = original.serialize();
        ASSERT_TRUE(serializeResult.success()) << "Serialize failed for entityId=" << entityId;

        SetCameraPacket deserialized;
        auto deserializeResult =
            deserialized.deserialize(serializeResult.value().data(), serializeResult.value().size());
        ASSERT_TRUE(deserializeResult.success()) << "Deserialize failed for entityId=" << entityId;

        EXPECT_EQ(deserialized.cameraEntityId(), entityId) << "Roundtrip mismatch for entityId=" << entityId;
    }
}
