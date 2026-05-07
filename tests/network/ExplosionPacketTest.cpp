#include <gtest/gtest.h>
#include "network/packet/ExplosionPacket.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/core/Types.hpp"
#include <cmath>
#include <unordered_map>

using namespace mc::network;
using mc::Vector3;
using mc::BlockPos;
using mc::f32;
using mc::u64;
using mc::u8;

// ==================== ExplosionPacket 基础测试 ====================

class ExplosionPacketTest : public ::testing::Test {
protected:
    void SetUp() override {
        testPosition = Vector3(100.5f, 64.0f, -200.25f);
        testStrength = 4.0f;  // TNT 爆炸半径
        testAffectedBlocks = {
            BlockPos(100, 64, -200),
            BlockPos(101, 64, -200),
            BlockPos(100, 65, -200),
            BlockPos(100, 64, -199),
            BlockPos(99, 64, -200),
        };
        testPlayerKnockback = {
            {1ULL, Vector3(0.5f, 1.0f, 0.2f)},
            {2ULL, Vector3(-0.3f, 0.8f, 0.1f)},
        };
    }

    Vector3 testPosition;
    f32 testStrength;
    std::vector<BlockPos> testAffectedBlocks;
    std::unordered_map<u64, Vector3> testPlayerKnockback;
};

TEST_F(ExplosionPacketTest, DefaultConstruction) {
    ExplosionPacket packet;
    EXPECT_EQ(packet.type(), PacketType::Explosion);
    EXPECT_FLOAT_EQ(packet.x(), 0.0f);
    EXPECT_FLOAT_EQ(packet.y(), 0.0f);
    EXPECT_FLOAT_EQ(packet.z(), 0.0f);
    EXPECT_FLOAT_EQ(packet.strength(), 0.0f);
    EXPECT_TRUE(packet.affectedBlocks().empty());
    EXPECT_FLOAT_EQ(packet.motionX(), 0.0f);
    EXPECT_FLOAT_EQ(packet.motionY(), 0.0f);
    EXPECT_FLOAT_EQ(packet.motionZ(), 0.0f);
}

TEST_F(ExplosionPacketTest, ParameterizedConstruction) {
    ExplosionPacket packet(testPosition, testStrength, testAffectedBlocks, testPlayerKnockback, 1ULL);

    EXPECT_FLOAT_EQ(packet.x(), testPosition.x);
    EXPECT_FLOAT_EQ(packet.y(), testPosition.y);
    EXPECT_FLOAT_EQ(packet.z(), testPosition.z);
    EXPECT_FLOAT_EQ(packet.strength(), testStrength);
    EXPECT_EQ(packet.affectedBlocks().size(), testAffectedBlocks.size());
    EXPECT_FLOAT_EQ(packet.motionX(), 0.5f);
    EXPECT_FLOAT_EQ(packet.motionY(), 1.0f);
    EXPECT_FLOAT_EQ(packet.motionZ(), 0.2f);
}

TEST_F(ExplosionPacketTest, SettersAndGetters) {
    ExplosionPacket packet;

    packet.setPosition(50.0f, 70.0f, 100.0f);
    EXPECT_FLOAT_EQ(packet.x(), 50.0f);
    EXPECT_FLOAT_EQ(packet.y(), 70.0f);
    EXPECT_FLOAT_EQ(packet.z(), 100.0f);

    Vector3 newPos(25.5f, 30.0f, 45.75f);
    packet.setPosition(newPos);
    EXPECT_FLOAT_EQ(packet.x(), newPos.x);
    EXPECT_FLOAT_EQ(packet.y(), newPos.y);
    EXPECT_FLOAT_EQ(packet.z(), newPos.z);

    packet.setStrength(6.0f);
    EXPECT_FLOAT_EQ(packet.strength(), 6.0f);

    std::vector<BlockPos> newBlocks = {BlockPos(0, 0, 0), BlockPos(1, 1, 1)};
    packet.setAffectedBlocks(newBlocks);
    EXPECT_EQ(packet.affectedBlocks().size(), 2u);

    packet.setMotion(1.0f, 2.0f, 3.0f);
    EXPECT_FLOAT_EQ(packet.motionX(), 1.0f);
    EXPECT_FLOAT_EQ(packet.motionY(), 2.0f);
    EXPECT_FLOAT_EQ(packet.motionZ(), 3.0f);
}

TEST_F(ExplosionPacketTest, SetKnockbackForPlayer) {
    ExplosionPacket packet;

    // 设置玩家1的击退
    packet.setKnockbackForPlayer(testPlayerKnockback, 1ULL);
    EXPECT_FLOAT_EQ(packet.motionX(), 0.5f);
    EXPECT_FLOAT_EQ(packet.motionY(), 1.0f);
    EXPECT_FLOAT_EQ(packet.motionZ(), 0.2f);

    // 设置玩家2的击退
    packet.setKnockbackForPlayer(testPlayerKnockback, 2ULL);
    EXPECT_FLOAT_EQ(packet.motionX(), -0.3f);
    EXPECT_FLOAT_EQ(packet.motionY(), 0.8f);
    EXPECT_FLOAT_EQ(packet.motionZ(), 0.1f);

    // 设置不存在玩家的击退（应为零）
    packet.setKnockbackForPlayer(testPlayerKnockback, 999ULL);
    EXPECT_FLOAT_EQ(packet.motionX(), 0.0f);
    EXPECT_FLOAT_EQ(packet.motionY(), 0.0f);
    EXPECT_FLOAT_EQ(packet.motionZ(), 0.0f);
}

// ==================== ExplosionPacket 序列化测试 ====================

class ExplosionPacketSerializeTest : public ::testing::Test {
protected:
    void SetUp() override {
        testPosition = Vector3(100.5f, 64.0f, -200.25f);
        testStrength = 4.0f;
        testAffectedBlocks = {
            BlockPos(100, 64, -200),
            BlockPos(101, 64, -200),
            BlockPos(100, 65, -200),
        };
    }

    Vector3 testPosition;
    f32 testStrength;
    std::vector<BlockPos> testAffectedBlocks;
};

TEST_F(ExplosionPacketSerializeTest, SerializeDeserializeBasic) {
    std::unordered_map<u64, Vector3> knockback = {{1ULL, Vector3(1.0f, 2.0f, 3.0f)}};

    ExplosionPacket original(testPosition, testStrength, testAffectedBlocks, knockback, 1ULL);

    auto serializeResult = original.serialize();
    ASSERT_TRUE(serializeResult.success()) << serializeResult.error().message();

    ExplosionPacket decoded;
    auto deserializeResult = decoded.deserialize(serializeResult.value().data(), serializeResult.value().size());
    ASSERT_TRUE(deserializeResult.success()) << deserializeResult.error().message();

    EXPECT_FLOAT_EQ(decoded.x(), original.x());
    EXPECT_FLOAT_EQ(decoded.y(), original.y());
    EXPECT_FLOAT_EQ(decoded.z(), original.z());
    EXPECT_FLOAT_EQ(decoded.strength(), original.strength());
    EXPECT_EQ(decoded.affectedBlocks().size(), original.affectedBlocks().size());
    EXPECT_FLOAT_EQ(decoded.motionX(), original.motionX());
    EXPECT_FLOAT_EQ(decoded.motionY(), original.motionY());
    EXPECT_FLOAT_EQ(decoded.motionZ(), original.motionZ());
}

TEST_F(ExplosionPacketSerializeTest, SerializeDeserializeEmptyBlocks) {
    std::vector<BlockPos> emptyBlocks;
    std::unordered_map<u64, Vector3> noKnockback;

    ExplosionPacket original(testPosition, testStrength, emptyBlocks, noKnockback, 0ULL);

    auto serializeResult = original.serialize();
    ASSERT_TRUE(serializeResult.success()) << serializeResult.error().message();

    ExplosionPacket decoded;
    auto deserializeResult = decoded.deserialize(serializeResult.value().data(), serializeResult.value().size());
    ASSERT_TRUE(deserializeResult.success()) << deserializeResult.error().message();

    EXPECT_FLOAT_EQ(decoded.x(), original.x());
    EXPECT_FLOAT_EQ(decoded.y(), original.y());
    EXPECT_FLOAT_EQ(decoded.z(), original.z());
    EXPECT_FLOAT_EQ(decoded.strength(), original.strength());
    EXPECT_TRUE(decoded.affectedBlocks().empty());
    EXPECT_FLOAT_EQ(decoded.motionX(), 0.0f);
    EXPECT_FLOAT_EQ(decoded.motionY(), 0.0f);
    EXPECT_FLOAT_EQ(decoded.motionZ(), 0.0f);
}

TEST_F(ExplosionPacketSerializeTest, SerializeDeserializeNegativeKnockback) {
    std::unordered_map<u64, Vector3> knockback = {{1ULL, Vector3(-5.0f, -10.0f, -15.0f)}};

    ExplosionPacket original(testPosition, testStrength, testAffectedBlocks, knockback, 1ULL);

    auto serializeResult = original.serialize();
    ASSERT_TRUE(serializeResult.success()) << serializeResult.error().message();

    ExplosionPacket decoded;
    auto deserializeResult = decoded.deserialize(serializeResult.value().data(), serializeResult.value().size());
    ASSERT_TRUE(deserializeResult.success()) << deserializeResult.error().message();

    EXPECT_FLOAT_EQ(decoded.motionX(), -5.0f);
    EXPECT_FLOAT_EQ(decoded.motionY(), -10.0f);
    EXPECT_FLOAT_EQ(decoded.motionZ(), -15.0f);
}

TEST_F(ExplosionPacketSerializeTest, BlockCoordinatesRelativeEncoding) {
    // 测试方块坐标使用相对编码
    // 基准坐标 = floor(爆炸位置)
    // 相对坐标 = 方块坐标 - 基准坐标

    Vector3 pos(100.9f, 64.1f, -200.5f);
    std::vector<BlockPos> blocks = {
        BlockPos(100, 64, -201),   // 相对: (0, 0, -1)
        BlockPos(101, 64, -201),   // 相对: (1, 0, -1)
        BlockPos(100, 65, -201),   // 相对: (0, 1, -1)
    };

    ExplosionPacket original(pos, 3.0f, blocks, {}, 0ULL);

    auto serializeResult = original.serialize();
    ASSERT_TRUE(serializeResult.success()) << serializeResult.error().message();

    ExplosionPacket decoded;
    auto deserializeResult = decoded.deserialize(serializeResult.value().data(), serializeResult.value().size());
    ASSERT_TRUE(deserializeResult.success()) << deserializeResult.error().message();

    // 验证方块坐标正确还原
    EXPECT_EQ(decoded.affectedBlocks().size(), blocks.size());
    for (size_t i = 0; i < blocks.size(); ++i) {
        EXPECT_EQ(decoded.affectedBlocks()[i].x, blocks[i].x);
        EXPECT_EQ(decoded.affectedBlocks()[i].y, blocks[i].y);
        EXPECT_EQ(decoded.affectedBlocks()[i].z, blocks[i].z);
    }
}

TEST_F(ExplosionPacketSerializeTest, DeserializeInvalidData) {
    ExplosionPacket packet;

    // 数据太短
    std::vector<u8> shortData(5, 0);
    auto result = packet.deserialize(shortData.data(), shortData.size());
    EXPECT_FALSE(result.success());

    // 空数据
    result = packet.deserialize(nullptr, 0);
    EXPECT_FALSE(result.success());
}

TEST_F(ExplosionPacketSerializeTest, LargeExplosion) {
    // 测试大量方块（模拟大型爆炸）
    std::vector<BlockPos> manyBlocks;
    for (int x = -10; x <= 10; ++x) {
        for (int y = -10; y <= 10; ++y) {
            for (int z = -10; z <= 10; ++z) {
                manyBlocks.emplace_back(100 + x, 64 + y, -200 + z);
            }
        }
    }

    Vector3 pos(100.0f, 64.0f, -200.0f);
    std::unordered_map<u64, Vector3> knockback = {
        {1ULL, Vector3(2.0f, 5.0f, 1.0f)},
        {2ULL, Vector3(-1.0f, 3.0f, 2.0f)},
    };

    ExplosionPacket original(pos, 10.0f, manyBlocks, knockback, 1ULL);

    auto serializeResult = original.serialize();
    ASSERT_TRUE(serializeResult.success()) << serializeResult.error().message();

    ExplosionPacket decoded;
    auto deserializeResult = decoded.deserialize(serializeResult.value().data(), serializeResult.value().size());
    ASSERT_TRUE(deserializeResult.success()) << deserializeResult.error().message();

    EXPECT_FLOAT_EQ(decoded.strength(), 10.0f);
    EXPECT_EQ(decoded.affectedBlocks().size(), manyBlocks.size());
    EXPECT_FLOAT_EQ(decoded.motionX(), 2.0f);
    EXPECT_FLOAT_EQ(decoded.motionY(), 5.0f);
    EXPECT_FLOAT_EQ(decoded.motionZ(), 1.0f);
}

TEST_F(ExplosionPacketSerializeTest, ExpectedSizeEstimation) {
    ExplosionPacket packet(testPosition, testStrength, testAffectedBlocks, {}, 0ULL);
    size_t estimatedSize = packet.expectedSize();
    auto serializeResult = packet.serialize();
    ASSERT_TRUE(serializeResult.success());

    // 估计大小应该略大于或等于实际大小
    EXPECT_GE(estimatedSize, serializeResult.value().size());
}

// ==================== ExplosionPacket 边界测试 ====================

TEST_F(ExplosionPacketTest, ExplosionStrengthBoundary) {
    ExplosionPacket packet;

    // 测试各种爆炸强度
    packet.setStrength(0.0f);
    EXPECT_FLOAT_EQ(packet.strength(), 0.0f);

    packet.setStrength(3.0f);  // 苦力怕
    EXPECT_FLOAT_EQ(packet.strength(), 3.0f);

    packet.setStrength(4.0f);  // TNT
    EXPECT_FLOAT_EQ(packet.strength(), 4.0f);

    packet.setStrength(6.0f);  // 高压苦力怕
    EXPECT_FLOAT_EQ(packet.strength(), 6.0f);

    packet.setStrength(7.0f);  // 凋灵召唤
    EXPECT_FLOAT_EQ(packet.strength(), 7.0f);
}

TEST_F(ExplosionPacketTest, MotionBoundary) {
    ExplosionPacket packet;

    // 测试极端击退值
    packet.setMotion(1000.0f, -1000.0f, 500.0f);
    EXPECT_FLOAT_EQ(packet.motionX(), 1000.0f);
    EXPECT_FLOAT_EQ(packet.motionY(), -1000.0f);
    EXPECT_FLOAT_EQ(packet.motionZ(), 500.0f);

    packet.setMotion(-0.001f, 0.001f, 0.0f);
    EXPECT_FLOAT_EQ(packet.motionX(), -0.001f);
    EXPECT_FLOAT_EQ(packet.motionY(), 0.001f);
    EXPECT_FLOAT_EQ(packet.motionZ(), 0.0f);
}
