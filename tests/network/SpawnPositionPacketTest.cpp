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

#include "common/network/packet/SpawnPositionPacket.hpp"
#include "common/network/packet/Packet.hpp"
#include <gtest/gtest.h>

using namespace mc::network;
using mc::BlockCoord;
using mc::BlockPos;

// ==================== SpawnPositionPacket Tests ====================

class SpawnPositionPacketTest : public ::testing::Test {
protected:
    void SetUp() override { testPos = BlockPos(100, 64, -200); }

    BlockPos testPos;
};

TEST_F(SpawnPositionPacketTest, DefaultConstruction)
{
    SpawnPositionPacket packet;
    EXPECT_EQ(packet.type(), PacketType::SpawnPosition);
    EXPECT_EQ(packet.position().x, 0);
    EXPECT_EQ(packet.position().y, 0);
    EXPECT_EQ(packet.position().z, 0);
}

TEST_F(SpawnPositionPacketTest, PositionConstruction)
{
    SpawnPositionPacket packet(testPos);
    EXPECT_EQ(packet.type(), PacketType::SpawnPosition);
    EXPECT_EQ(packet.position().x, 100);
    EXPECT_EQ(packet.position().y, 64);
    EXPECT_EQ(packet.position().z, -200);
}

TEST_F(SpawnPositionPacketTest, SetPosition)
{
    SpawnPositionPacket packet;
    packet.setPosition(testPos);
    EXPECT_EQ(packet.position().x, 100);
    EXPECT_EQ(packet.position().y, 64);
    EXPECT_EQ(packet.position().z, -200);
}

TEST_F(SpawnPositionPacketTest, SerializeDeserialize)
{
    SpawnPositionPacket packet(testPos);

    auto result = packet.serialize();
    ASSERT_TRUE(result.success());

    const auto& data = result.value();
    // Serialized data is payload only (3 x i32 + 1 x f32 = 16 bytes)
    // expectedSize() returns total including header (12 + 16 = 28)
    EXPECT_EQ(data.size(), 16u);
    EXPECT_EQ(packet.expectedSize(), 28u);

    SpawnPositionPacket packet2;
    auto result2 = packet2.deserialize(data.data(), data.size());
    EXPECT_TRUE(result2.success());

    EXPECT_EQ(packet2.position().x, 100);
    EXPECT_EQ(packet2.position().y, 64);
    EXPECT_EQ(packet2.position().z, -200);
}

TEST_F(SpawnPositionPacketTest, SerializeDeserializeNegativeCoords)
{
    BlockPos negativePos(-1000, -50, -9999);
    SpawnPositionPacket packet(negativePos);

    auto result = packet.serialize();
    ASSERT_TRUE(result.success());

    SpawnPositionPacket packet2;
    auto result2 = packet2.deserialize(result.value().data(), result.value().size());
    EXPECT_TRUE(result2.success());

    EXPECT_EQ(packet2.position().x, -1000);
    EXPECT_EQ(packet2.position().y, -50);
    EXPECT_EQ(packet2.position().z, -9999);
}

TEST_F(SpawnPositionPacketTest, SerializeDeserializeZeroCoords)
{
    BlockPos zeroPos(0, 0, 0);
    SpawnPositionPacket packet(zeroPos);

    auto result = packet.serialize();
    ASSERT_TRUE(result.success());

    SpawnPositionPacket packet2;
    auto result2 = packet2.deserialize(result.value().data(), result.value().size());
    EXPECT_TRUE(result2.success());

    EXPECT_EQ(packet2.position().x, 0);
    EXPECT_EQ(packet2.position().y, 0);
    EXPECT_EQ(packet2.position().z, 0);
}

TEST_F(SpawnPositionPacketTest, DeserializeTooSmall)
{
    SpawnPositionPacket packet;
    std::vector<mc::u8> smallData = {0x01, 0x02}; // Too small

    auto result = packet.deserialize(smallData.data(), smallData.size());
    EXPECT_FALSE(result.success());
}

TEST_F(SpawnPositionPacketTest, PacketType)
{
    SpawnPositionPacket packet;
    EXPECT_EQ(packet.type(), PacketType::SpawnPosition);
}

// ==================== Angle 字段测试 ====================

TEST_F(SpawnPositionPacketTest, DefaultAngleIsZero)
{
    SpawnPositionPacket packet;
    EXPECT_FLOAT_EQ(packet.angle(), 0.0f);
}

TEST_F(SpawnPositionPacketTest, PositionAndAngleConstruction)
{
    SpawnPositionPacket packet(testPos, 90.0f);
    EXPECT_EQ(packet.position().x, 100);
    EXPECT_EQ(packet.position().y, 64);
    EXPECT_EQ(packet.position().z, -200);
    EXPECT_FLOAT_EQ(packet.angle(), 90.0f);
}

TEST_F(SpawnPositionPacketTest, SetAngle)
{
    SpawnPositionPacket packet;
    packet.setAngle(-45.0f);
    EXPECT_FLOAT_EQ(packet.angle(), -45.0f);
}

TEST_F(SpawnPositionPacketTest, SerializeDeserializeWithAngle)
{
    SpawnPositionPacket packet(testPos, 135.0f);

    auto result = packet.serialize();
    ASSERT_TRUE(result.success());

    // 序列化数据包含 3 x i32 (position) + 1 x f32 (angle) = 16 bytes
    EXPECT_EQ(result.value().size(), 16u);

    SpawnPositionPacket packet2;
    auto result2 = packet2.deserialize(result.value().data(), result.value().size());
    EXPECT_TRUE(result2.success());

    EXPECT_EQ(packet2.position().x, 100);
    EXPECT_EQ(packet2.position().y, 64);
    EXPECT_EQ(packet2.position().z, -200);
    EXPECT_FLOAT_EQ(packet2.angle(), 135.0f);
}

TEST_F(SpawnPositionPacketTest, SerializeDeserializeNegativeAngle)
{
    SpawnPositionPacket packet(BlockPos(50, 64, -100), -180.0f);

    auto result = packet.serialize();
    ASSERT_TRUE(result.success());

    SpawnPositionPacket packet2;
    auto result2 = packet2.deserialize(result.value().data(), result.value().size());
    EXPECT_TRUE(result2.success());

    EXPECT_FLOAT_EQ(packet2.angle(), -180.0f);
}

TEST_F(SpawnPositionPacketTest, SerializeDeserializeZeroAngle)
{
    SpawnPositionPacket packet(testPos, 0.0f);

    auto result = packet.serialize();
    ASSERT_TRUE(result.success());

    SpawnPositionPacket packet2;
    auto result2 = packet2.deserialize(result.value().data(), result.value().size());
    EXPECT_TRUE(result2.success());

    EXPECT_FLOAT_EQ(packet2.angle(), 0.0f);
}

TEST_F(SpawnPositionPacketTest, DeserializeAngleFallbackOnTruncatedData)
{
    // 仅提供位置数据（12 bytes），不包含 angle
    SpawnPositionPacket packet(testPos);
    auto result = packet.serialize();
    ASSERT_TRUE(result.success());

    // 截取仅位置部分（12 bytes），模拟无 angle 的旧版数据
    std::vector<mc::u8> positionOnly(result.value().begin(), result.value().begin() + 12);

    SpawnPositionPacket packet2;
    auto result2 = packet2.deserialize(positionOnly.data(), positionOnly.size());
    // 反序列化应成功（angle 使用默认值 0.0f 向后兼容）
    EXPECT_TRUE(result2.success());
    EXPECT_FLOAT_EQ(packet2.angle(), 0.0f);
}

TEST_F(SpawnPositionPacketTest, AngleBoundary180)
{
    SpawnPositionPacket packet(BlockPos(0, 0, 0), 180.0f);

    auto result = packet.serialize();
    ASSERT_TRUE(result.success());

    SpawnPositionPacket packet2;
    auto result2 = packet2.deserialize(result.value().data(), result.value().size());
    EXPECT_TRUE(result2.success());
    EXPECT_FLOAT_EQ(packet2.angle(), 180.0f);
}
