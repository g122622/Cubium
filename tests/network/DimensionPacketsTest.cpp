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

#include "common/network/packet/DimensionPackets.hpp"
#include "common/network/packet/Packet.hpp"
#include <gtest/gtest.h>

using namespace mc::network;
using mc::BlockPos;
using mc::GlobalPos;

// ==================== RespawnPacket Tests ====================

class RespawnPacketTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(RespawnPacketTest, DefaultConstruction)
{
    RespawnPacket packet;
    EXPECT_EQ(packet.type(), PacketType::Respawn);
    EXPECT_EQ(packet.dimensionType(), 0);
    EXPECT_EQ(packet.dimension(), 0);
    EXPECT_EQ(packet.hashedSeed(), 0u);
    EXPECT_EQ(packet.gameMode(), mc::GameMode::Survival);
    EXPECT_EQ(packet.previousGameMode(), mc::GameMode::NotSet);
    EXPECT_FALSE(packet.isDebug());
    EXPECT_FALSE(packet.isFlat());
    EXPECT_FALSE(packet.keepData());
    EXPECT_FALSE(packet.lastDeathLocation().has_value());
}

TEST_F(RespawnPacketTest, SetLastDeathLocation)
{
    RespawnPacket packet;
    GlobalPos deathPos(mc::DimensionId(-1), BlockPos(50, 30, -100));

    packet.setLastDeathLocation(deathPos);
    ASSERT_TRUE(packet.lastDeathLocation().has_value());
    EXPECT_EQ(packet.lastDeathLocation()->getDimensionId(), mc::DimensionId(-1));
    EXPECT_EQ(packet.lastDeathLocation()->x(), 50);
    EXPECT_EQ(packet.lastDeathLocation()->y(), 30);
    EXPECT_EQ(packet.lastDeathLocation()->z(), -100);

    // 清除
    packet.setLastDeathLocation(std::nullopt);
    EXPECT_FALSE(packet.lastDeathLocation().has_value());
}

TEST_F(RespawnPacketTest, SerializeDeserializeWithDeathLocation)
{
    RespawnPacket original;
    original.setDimensionType(0);
    original.setDimension(mc::DimensionId(0));
    original.setHashedSeed(12345678901234ULL);
    original.setGameMode(mc::GameMode::Survival);
    original.setPreviousGameMode(mc::GameMode::NotSet);
    original.setKeepData(true);
    original.setLastDeathLocation(GlobalPos(mc::DimensionId(-1), BlockPos(100, 64, -200)));

    auto serializeResult = original.serialize();
    ASSERT_TRUE(serializeResult.success());
    const auto& data = serializeResult.value();
    EXPECT_FALSE(data.empty());

    RespawnPacket restored;
    auto deserializeResult = restored.deserialize(data.data(), data.size());
    EXPECT_TRUE(deserializeResult.success());

    EXPECT_EQ(restored.dimensionType(), 0);
    EXPECT_EQ(restored.dimension(), mc::DimensionId(0));
    EXPECT_EQ(restored.hashedSeed(), 12345678901234ULL);
    EXPECT_EQ(restored.gameMode(), mc::GameMode::Survival);
    EXPECT_EQ(restored.previousGameMode(), mc::GameMode::NotSet);
    EXPECT_TRUE(restored.keepData());

    // 验证 lastDeathLocation
    ASSERT_TRUE(restored.lastDeathLocation().has_value());
    EXPECT_EQ(restored.lastDeathLocation()->getDimensionId(), mc::DimensionId(-1));
    EXPECT_EQ(restored.lastDeathLocation()->x(), 100);
    EXPECT_EQ(restored.lastDeathLocation()->y(), 64);
    EXPECT_EQ(restored.lastDeathLocation()->z(), -200);
}

TEST_F(RespawnPacketTest, SerializeDeserializeWithoutDeathLocation)
{
    RespawnPacket original;
    original.setDimensionType(-1); // 下界维度ID=-1，序列化后类型ID=1
    original.setDimension(mc::DimensionId(-1));
    original.setHashedSeed(9876543210ULL);
    original.setGameMode(mc::GameMode::Creative);
    original.setKeepData(true);
    // 不设置 lastDeathLocation

    auto serializeResult = original.serialize();
    ASSERT_TRUE(serializeResult.success());

    RespawnPacket restored;
    auto deserializeResult = restored.deserialize(serializeResult.value().data(), serializeResult.value().size());
    EXPECT_TRUE(deserializeResult.success());

    // 反序列化后 dimensionType 存储的是序列化映射后的值 1（下界类型ID）
    EXPECT_EQ(restored.dimensionType(), 1);
    EXPECT_EQ(restored.dimension(), mc::DimensionId(-1));
    EXPECT_EQ(restored.hashedSeed(), 9876543210ULL);
    EXPECT_EQ(restored.gameMode(), mc::GameMode::Creative);
    EXPECT_TRUE(restored.keepData());

    // lastDeathLocation 应该为空
    EXPECT_FALSE(restored.lastDeathLocation().has_value());
}

TEST_F(RespawnPacketTest, SerializeDeserializeEndDimensionDeathLocation)
{
    RespawnPacket original;
    original.setDimensionType(2); // The End
    original.setDimension(mc::DimensionId(1));
    original.setHashedSeed(5555555555ULL);
    original.setGameMode(mc::GameMode::Spectator);
    original.setLastDeathLocation(GlobalPos(mc::DimensionId(1), BlockPos(-300, 80, 150)));

    auto serializeResult = original.serialize();
    ASSERT_TRUE(serializeResult.success());

    RespawnPacket restored;
    auto deserializeResult = restored.deserialize(serializeResult.value().data(), serializeResult.value().size());
    EXPECT_TRUE(deserializeResult.success());

    EXPECT_EQ(restored.dimensionType(), 2);
    EXPECT_EQ(restored.dimension(), mc::DimensionId(1));
    EXPECT_EQ(restored.gameMode(), mc::GameMode::Spectator);

    ASSERT_TRUE(restored.lastDeathLocation().has_value());
    EXPECT_EQ(restored.lastDeathLocation()->getDimensionId(), mc::DimensionId(1));
    EXPECT_EQ(restored.lastDeathLocation()->x(), -300);
    EXPECT_EQ(restored.lastDeathLocation()->y(), 80);
    EXPECT_EQ(restored.lastDeathLocation()->z(), 150);
}

TEST_F(RespawnPacketTest, ExpectedSizeWithAndWithoutDeathLocation)
{
    RespawnPacket withoutDeath;
    withoutDeath.setDimensionType(0);
    withoutDeath.setDimension(mc::DimensionId(0));
    size_t sizeWithout = withoutDeath.expectedSize();

    RespawnPacket withDeath;
    withDeath.setDimensionType(0);
    withDeath.setDimension(mc::DimensionId(0));
    withDeath.setLastDeathLocation(GlobalPos(mc::DimensionId(0), BlockPos(1, 2, 3)));
    size_t sizeWith = withDeath.expectedSize();

    // 带 lastDeathLocation 的包应该更大
    EXPECT_GT(sizeWith, sizeWithout);
}
