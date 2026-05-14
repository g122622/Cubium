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
    // Serialized data is payload only (3 x i32 = 12 bytes)
    // expectedSize() returns total including header (12 + 12 = 24)
    EXPECT_EQ(data.size(), 12u);
    EXPECT_EQ(packet.expectedSize(), 24u);

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
