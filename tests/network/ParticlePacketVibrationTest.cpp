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

#include "client/renderer/trident/particle/ParticleTypes.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/world/block/BlockPos.hpp"
#include "network/packet/ParticlePacket.hpp"
#include <gtest/gtest.h>

using namespace mc::network;
using namespace mc::client::renderer::trident::particle;
using mc::BlockPos;
using mc::EntityInstanceId;
using mc::f32;
using mc::f64;
using mc::i32;
using mc::i64;
using mc::u32;
using mc::u8;
using mc::Vector3;
using mc::Vector3d;

// ==================== createVibration (Block) 工厂方法测试 ====================

TEST(ParticlePacketVibrationTest, CreateVibrationBlock_SetsCorrectType)
{
    Vector3 pos(10.0f, 64.0f, -20.0f);
    BlockPos target(100, 70, -50);

    auto packet = ParticlePacket::createVibration(pos, target, 15);

    EXPECT_EQ(packet.particleType(), ParticleTypeId::Vibration);
}

TEST(ParticlePacketVibrationTest, CreateVibrationBlock_SetsPosition)
{
    Vector3 pos(10.0f, 64.0f, -20.0f);
    BlockPos target(100, 70, -50);

    auto packet = ParticlePacket::createVibration(pos, target, 15);

    EXPECT_DOUBLE_EQ(packet.x(), 10.0);
    EXPECT_DOUBLE_EQ(packet.y(), 64.0);
    EXPECT_DOUBLE_EQ(packet.z(), -20.0);
}

TEST(ParticlePacketVibrationTest, CreateVibrationBlock_ZeroVelocityAndOffset)
{
    Vector3 pos(0.0f, 0.0f, 0.0f);
    BlockPos target(50, 100, 150);

    auto packet = ParticlePacket::createVibration(pos, target, 10);

    EXPECT_FLOAT_EQ(packet.velocityX(), 0.0f);
    EXPECT_FLOAT_EQ(packet.velocityY(), 0.0f);
    EXPECT_FLOAT_EQ(packet.velocityZ(), 0.0f);
    EXPECT_FLOAT_EQ(packet.offsetX(), 0.0f);
    EXPECT_FLOAT_EQ(packet.offsetY(), 0.0f);
    EXPECT_FLOAT_EQ(packet.offsetZ(), 0.0f);
}

TEST(ParticlePacketVibrationTest, CreateVibrationBlock_CountIsOne)
{
    Vector3 pos(0.0f, 0.0f, 0.0f);
    BlockPos target(50, 100, 150);

    auto packet = ParticlePacket::createVibration(pos, target, 10);

    EXPECT_EQ(packet.count(), 1u);
}

TEST(ParticlePacketVibrationTest, CreateVibrationBlock_HasOptionalData)
{
    Vector3 pos(0.0f, 0.0f, 0.0f);
    BlockPos target(50, 100, 150);

    auto packet = ParticlePacket::createVibration(pos, target, 10);

    // 振动粒子包应包含可选数据
    EXPECT_FALSE(packet.optionalData().empty());
    // 格式: VarInt(0) + i64 packedBlockPos + VarInt arrivalInTicks
    // 最少 1 + 8 + 1 = 10 字节
    EXPECT_GE(packet.optionalData().size(), 10u);
}

// ==================== createVibration (Entity) 工厂方法测试 ====================

TEST(ParticlePacketVibrationTest, CreateVibrationEntity_SetsCorrectType)
{
    Vector3 pos(10.0f, 64.0f, -20.0f);

    auto packet = ParticlePacket::createVibration(pos, EntityInstanceId(42), 1.5f, 15);

    EXPECT_EQ(packet.particleType(), ParticleTypeId::Vibration);
}

TEST(ParticlePacketVibrationTest, CreateVibrationEntity_HasOptionalData)
{
    Vector3 pos(0.0f, 0.0f, 0.0f);

    auto packet = ParticlePacket::createVibration(pos, EntityInstanceId(42), 1.5f, 10);

    EXPECT_FALSE(packet.optionalData().empty());
    // 格式: VarInt(1) + VarInt entityId + f32 yOffset + VarInt arrivalInTicks
    // 最少 1 + 1 + 4 + 1 = 7 字节
    EXPECT_GE(packet.optionalData().size(), 7u);
}

// ==================== isVibrationParticle 测试 ====================

TEST(ParticlePacketVibrationTest, IsVibrationParticle_TrueForVibrationWithOptionalData)
{
    Vector3 pos(0.0f, 0.0f, 0.0f);
    BlockPos target(50, 100, 150);

    auto packet = ParticlePacket::createVibration(pos, target, 10);

    EXPECT_TRUE(packet.isVibrationParticle());
}

TEST(ParticlePacketVibrationTest, IsVibrationParticle_FalseForVibrationWithoutOptionalData)
{
    ParticlePacket packet(ParticleTypeId::Vibration, Vector3(0, 0, 0), Vector3(0, 0, 0), Vector3(0, 0, 0), 1);

    EXPECT_FALSE(packet.isVibrationParticle());
}

TEST(ParticlePacketVibrationTest, IsVibrationParticle_FalseForNonVibrationType)
{
    ParticlePacket packet(ParticleTypeId::Flame, Vector3(0, 0, 0), Vector3(0, 0, 0), Vector3(0, 0, 0), 1);
    std::vector<u8> fakeData(32, 0x00);
    packet.setOptionalData(fakeData);

    EXPECT_FALSE(packet.isVibrationParticle());
}

// ==================== decodeVibrationTarget (Block) 测试 ====================

TEST(ParticlePacketVibrationTest, DecodeVibrationTargetBlock_ReturnsCorrectPosition)
{
    Vector3 pos(10.0f, 64.0f, -20.0f);
    BlockPos target(100, 70, -50);

    auto packet = ParticlePacket::createVibration(pos, target, 15);

    auto decoded = packet.decodeVibrationTarget();
    ASSERT_TRUE(decoded.has_value());

    EXPECT_EQ(decoded->kind, ParticlePacket::VibrationTarget::Kind::Block);
    EXPECT_EQ(decoded->blockPos.x, 100);
    EXPECT_EQ(decoded->blockPos.y, 70);
    EXPECT_EQ(decoded->blockPos.z, -50);
}

TEST(ParticlePacketVibrationTest, DecodeVibrationTargetBlock_NegativeCoordinates)
{
    Vector3 pos(0.0f, 0.0f, 0.0f);
    BlockPos target(-100, -64, -200);

    auto packet = ParticlePacket::createVibration(pos, target, 8);

    auto decoded = packet.decodeVibrationTarget();
    ASSERT_TRUE(decoded.has_value());

    EXPECT_EQ(decoded->kind, ParticlePacket::VibrationTarget::Kind::Block);
    EXPECT_EQ(decoded->blockPos.x, -100);
    EXPECT_EQ(decoded->blockPos.y, -64);
    EXPECT_EQ(decoded->blockPos.z, -200);
}

TEST(ParticlePacketVibrationTest, DecodeVibrationTargetBlock_ZeroPosition)
{
    Vector3 pos(50.0f, 50.0f, 50.0f);
    BlockPos target(0, 0, 0);

    auto packet = ParticlePacket::createVibration(pos, target, 5);

    auto decoded = packet.decodeVibrationTarget();
    ASSERT_TRUE(decoded.has_value());

    EXPECT_EQ(decoded->blockPos.x, 0);
    EXPECT_EQ(decoded->blockPos.y, 0);
    EXPECT_EQ(decoded->blockPos.z, 0);
}

TEST(ParticlePacketVibrationTest, DecodeVibrationTargetBlock_LargeCoordinates)
{
    // MC Java BlockPos.asLong 的水平坐标范围为 ±33554432 (2^25)，Y 范围为 ±2048 (2^11)
    // 30000000 在 X/Z 范围内，256 在 Y 范围内
    Vector3 pos(0.0f, 0.0f, 0.0f);
    BlockPos target(30000000, 256, 30000000);

    auto packet = ParticlePacket::createVibration(pos, target, 100);

    auto decoded = packet.decodeVibrationTarget();
    ASSERT_TRUE(decoded.has_value());

    EXPECT_EQ(decoded->blockPos.x, 30000000);
    EXPECT_EQ(decoded->blockPos.y, 256);
    EXPECT_EQ(decoded->blockPos.z, 30000000);
}

TEST(ParticlePacketVibrationTest, DecodeVibrationTargetBlock_ReturnsNulloptForNonVibration)
{
    ParticlePacket packet(ParticleTypeId::Flame, Vector3(0, 0, 0), Vector3(0, 0, 0), Vector3(0, 0, 0), 1);

    auto decoded = packet.decodeVibrationTarget();
    EXPECT_FALSE(decoded.has_value());
}

TEST(ParticlePacketVibrationTest, DecodeVibrationTargetBlock_ReturnsNulloptForEmptyOptionalData)
{
    ParticlePacket packet(ParticleTypeId::Vibration, Vector3(0, 0, 0), Vector3(0, 0, 0), Vector3(0, 0, 0), 1);

    auto decoded = packet.decodeVibrationTarget();
    EXPECT_FALSE(decoded.has_value());
}

// ==================== decodeVibrationTarget (Entity) 测试 ====================

TEST(ParticlePacketVibrationTest, DecodeVibrationTargetEntity_ReturnsCorrectEntityInfo)
{
    Vector3 pos(10.0f, 64.0f, -20.0f);

    auto packet = ParticlePacket::createVibration(pos, EntityInstanceId(42), 1.5f, 15);

    auto decoded = packet.decodeVibrationTarget();
    ASSERT_TRUE(decoded.has_value());

    EXPECT_EQ(decoded->kind, ParticlePacket::VibrationTarget::Kind::Entity);
    EXPECT_EQ(decoded->entityId, EntityInstanceId(42));
    EXPECT_FLOAT_EQ(decoded->yOffset, 1.5f);
}

TEST(ParticlePacketVibrationTest, DecodeVibrationTargetEntity_ZeroEntityIdAndOffset)
{
    Vector3 pos(0.0f, 0.0f, 0.0f);

    auto packet = ParticlePacket::createVibration(pos, EntityInstanceId(0), 0.0f, 5);

    auto decoded = packet.decodeVibrationTarget();
    ASSERT_TRUE(decoded.has_value());

    EXPECT_EQ(decoded->kind, ParticlePacket::VibrationTarget::Kind::Entity);
    EXPECT_EQ(decoded->entityId, EntityInstanceId(0));
    EXPECT_FLOAT_EQ(decoded->yOffset, 0.0f);
}

TEST(ParticlePacketVibrationTest, DecodeVibrationTargetEntity_LargeEntityId)
{
    Vector3 pos(0.0f, 0.0f, 0.0f);

    // VarInt 可表示的最大 32 位有符号整数
    auto packet = ParticlePacket::createVibration(pos, EntityInstanceId(2000000000), 2.0f, 10);

    auto decoded = packet.decodeVibrationTarget();
    ASSERT_TRUE(decoded.has_value());

    EXPECT_EQ(decoded->entityId, EntityInstanceId(2000000000));
    EXPECT_FLOAT_EQ(decoded->yOffset, 2.0f);
}

// ==================== decodeVibrationArrivalInTicks 测试 ====================

TEST(ParticlePacketVibrationTest, DecodeVibrationArrivalInTicks_ReturnsCorrectTicks_Block)
{
    Vector3 pos(10.0f, 64.0f, -20.0f);
    BlockPos target(100, 70, -50);

    auto packet = ParticlePacket::createVibration(pos, target, 15);

    auto decoded = packet.decodeVibrationArrivalInTicks();
    ASSERT_TRUE(decoded.has_value());

    EXPECT_EQ(decoded.value(), 15);
}

TEST(ParticlePacketVibrationTest, DecodeVibrationArrivalInTicks_ReturnsCorrectTicks_Entity)
{
    Vector3 pos(10.0f, 64.0f, -20.0f);

    auto packet = ParticlePacket::createVibration(pos, EntityInstanceId(42), 1.5f, 20);

    auto decoded = packet.decodeVibrationArrivalInTicks();
    ASSERT_TRUE(decoded.has_value());

    EXPECT_EQ(decoded.value(), 20);
}

TEST(ParticlePacketVibrationTest, DecodeVibrationArrivalInTicks_SingleTick)
{
    Vector3 pos(0.0f, 0.0f, 0.0f);
    BlockPos target(1, 1, 1);

    auto packet = ParticlePacket::createVibration(pos, target, 1);

    auto decoded = packet.decodeVibrationArrivalInTicks();
    ASSERT_TRUE(decoded.has_value());

    EXPECT_EQ(decoded.value(), 1);
}

TEST(ParticlePacketVibrationTest, DecodeVibrationArrivalInTicks_LargeTicks)
{
    Vector3 pos(0.0f, 0.0f, 0.0f);
    BlockPos target(50, 100, 150);

    auto packet = ParticlePacket::createVibration(pos, target, 1000000);

    auto decoded = packet.decodeVibrationArrivalInTicks();
    ASSERT_TRUE(decoded.has_value());

    EXPECT_EQ(decoded.value(), 1000000);
}

TEST(ParticlePacketVibrationTest, DecodeVibrationArrivalInTicks_ReturnsNulloptForNonVibration)
{
    ParticlePacket packet(ParticleTypeId::Flame, Vector3(0, 0, 0), Vector3(0, 0, 0), Vector3(0, 0, 0), 1);

    auto decoded = packet.decodeVibrationArrivalInTicks();
    EXPECT_FALSE(decoded.has_value());
}

TEST(ParticlePacketVibrationTest, DecodeVibrationArrivalInTicks_ReturnsNulloptForEmptyOptionalData)
{
    ParticlePacket packet(ParticleTypeId::Vibration, Vector3(0, 0, 0), Vector3(0, 0, 0), Vector3(0, 0, 0), 1);

    auto decoded = packet.decodeVibrationArrivalInTicks();
    EXPECT_FALSE(decoded.has_value());
}

// ==================== 振动粒子序列化/反序列化测试 ====================

TEST(ParticlePacketVibrationTest, SerializeDeserialize_VibrationPacket_Block)
{
    Vector3 pos(10.0f, 64.0f, -20.0f);
    BlockPos target(100, 70, -50);

    auto original = ParticlePacket::createVibration(pos, target, 15);

    auto result = original.serialize();
    ASSERT_TRUE(result.success()) << result.error().message();

    ParticlePacket deserialized;
    auto deserResult = deserialized.deserialize(result.value().data(), result.value().size());
    ASSERT_TRUE(deserResult.success()) << deserResult.error().message();

    EXPECT_EQ(deserialized.particleType(), ParticleTypeId::Vibration);
    EXPECT_DOUBLE_EQ(deserialized.x(), 10.0);
    EXPECT_DOUBLE_EQ(deserialized.y(), 64.0);
    EXPECT_DOUBLE_EQ(deserialized.z(), -20.0);
    EXPECT_FLOAT_EQ(deserialized.velocityX(), 0.0f);
    EXPECT_FLOAT_EQ(deserialized.velocityY(), 0.0f);
    EXPECT_FLOAT_EQ(deserialized.velocityZ(), 0.0f);
    EXPECT_FLOAT_EQ(deserialized.offsetX(), 0.0f);
    EXPECT_FLOAT_EQ(deserialized.offsetY(), 0.0f);
    EXPECT_FLOAT_EQ(deserialized.offsetZ(), 0.0f);
    EXPECT_EQ(deserialized.count(), 1u);

    EXPECT_TRUE(deserialized.isVibrationParticle());

    auto decodedTarget = deserialized.decodeVibrationTarget();
    ASSERT_TRUE(decodedTarget.has_value());
    EXPECT_EQ(decodedTarget->kind, ParticlePacket::VibrationTarget::Kind::Block);
    EXPECT_EQ(decodedTarget->blockPos.x, 100);
    EXPECT_EQ(decodedTarget->blockPos.y, 70);
    EXPECT_EQ(decodedTarget->blockPos.z, -50);

    auto decodedTicks = deserialized.decodeVibrationArrivalInTicks();
    ASSERT_TRUE(decodedTicks.has_value());
    EXPECT_EQ(decodedTicks.value(), 15);
}

TEST(ParticlePacketVibrationTest, SerializeDeserialize_VibrationPacket_Entity)
{
    Vector3 pos(10.0f, 64.0f, -20.0f);

    auto original = ParticlePacket::createVibration(pos, EntityInstanceId(42), 1.5f, 20);

    auto result = original.serialize();
    ASSERT_TRUE(result.success()) << result.error().message();

    ParticlePacket deserialized;
    auto deserResult = deserialized.deserialize(result.value().data(), result.value().size());
    ASSERT_TRUE(deserResult.success()) << deserResult.error().message();

    EXPECT_TRUE(deserialized.isVibrationParticle());

    auto decodedTarget = deserialized.decodeVibrationTarget();
    ASSERT_TRUE(decodedTarget.has_value());
    EXPECT_EQ(decodedTarget->kind, ParticlePacket::VibrationTarget::Kind::Entity);
    EXPECT_EQ(decodedTarget->entityId, EntityInstanceId(42));
    EXPECT_FLOAT_EQ(decodedTarget->yOffset, 1.5f);

    auto decodedTicks = deserialized.decodeVibrationArrivalInTicks();
    ASSERT_TRUE(decodedTicks.has_value());
    EXPECT_EQ(decodedTicks.value(), 20);
}

TEST(ParticlePacketVibrationTest, SerializeDeserialize_VibrationPacket_NegativeTarget)
{
    Vector3 pos(0.0f, 0.0f, 0.0f);
    BlockPos target(-100, -64, -200);

    auto original = ParticlePacket::createVibration(pos, target, 8);

    auto result = original.serialize();
    ASSERT_TRUE(result.success()) << result.error().message();

    ParticlePacket deserialized;
    auto deserResult = deserialized.deserialize(result.value().data(), result.value().size());
    ASSERT_TRUE(deserResult.success()) << deserResult.error().message();

    auto decodedTarget = deserialized.decodeVibrationTarget();
    ASSERT_TRUE(decodedTarget.has_value());
    EXPECT_EQ(decodedTarget->blockPos.x, -100);
    EXPECT_EQ(decodedTarget->blockPos.y, -64);
    EXPECT_EQ(decodedTarget->blockPos.z, -200);

    auto decodedTicks = deserialized.decodeVibrationArrivalInTicks();
    ASSERT_TRUE(decodedTicks.has_value());
    EXPECT_EQ(decodedTicks.value(), 8);
}

TEST(ParticlePacketVibrationTest, SerializeDeserialize_VibrationPacket_ZeroTarget)
{
    Vector3 pos(10.0f, 20.0f, 30.0f);
    BlockPos target(0, 0, 0);

    auto original = ParticlePacket::createVibration(pos, target, 5);

    auto result = original.serialize();
    ASSERT_TRUE(result.success()) << result.error().message();

    ParticlePacket deserialized;
    auto deserResult = deserialized.deserialize(result.value().data(), result.value().size());
    ASSERT_TRUE(deserResult.success()) << deserResult.error().message();

    auto decodedTarget = deserialized.decodeVibrationTarget();
    ASSERT_TRUE(decodedTarget.has_value());
    EXPECT_EQ(decodedTarget->blockPos.x, 0);
    EXPECT_EQ(decodedTarget->blockPos.y, 0);
    EXPECT_EQ(decodedTarget->blockPos.z, 0);

    auto decodedTicks = deserialized.decodeVibrationArrivalInTicks();
    ASSERT_TRUE(decodedTicks.has_value());
    EXPECT_EQ(decodedTicks.value(), 5);
}

// ==================== 振动粒子数据完整性测试 ====================

TEST(ParticlePacketVibrationTest, VibrationDataRoundTrip_MultipleBlockPackets)
{
    struct TestCase {
        Vector3 pos;
        BlockPos target;
        i32 ticks;
    };

    std::vector<TestCase> cases = {
        {Vector3(0.0f, 0.0f, 0.0f), BlockPos(10, 20, 30), 5},
        {Vector3(100.0f, -64.0f, 200.0f), BlockPos(-50, 300, -100), 20},
        {Vector3(1.5f, 2.5f, 3.5f), BlockPos(0, 0, 0), 1},
    };

    for (const auto& tc : cases) {
        auto original = ParticlePacket::createVibration(tc.pos, tc.target, tc.ticks);
        auto result = original.serialize();
        ASSERT_TRUE(result.success()) << "Serialization failed for pos=(" << tc.pos.x << "," << tc.pos.y << ","
                                      << tc.pos.z << ")";

        ParticlePacket deserialized;
        auto deserResult = deserialized.deserialize(result.value().data(), result.value().size());
        ASSERT_TRUE(deserResult.success()) << "Deserialization failed";

        auto decodedTarget = deserialized.decodeVibrationTarget();
        ASSERT_TRUE(decodedTarget.has_value());
        ASSERT_EQ(decodedTarget->kind, ParticlePacket::VibrationTarget::Kind::Block);
        EXPECT_EQ(decodedTarget->blockPos.x, tc.target.x);
        EXPECT_EQ(decodedTarget->blockPos.y, tc.target.y);
        EXPECT_EQ(decodedTarget->blockPos.z, tc.target.z);

        auto decodedTicks = deserialized.decodeVibrationArrivalInTicks();
        ASSERT_TRUE(decodedTicks.has_value());
        EXPECT_EQ(decodedTicks.value(), tc.ticks);
    }
}

TEST(ParticlePacketVibrationTest, VibrationDataRoundTrip_MultipleEntityPackets)
{
    struct TestCase {
        Vector3 pos;
        EntityInstanceId entityId;
        f32 yOffset;
        i32 ticks;
    };

    std::vector<TestCase> cases = {
        {Vector3(0.0f, 0.0f, 0.0f), EntityInstanceId(1), 0.0f, 5},
        {Vector3(100.0f, -64.0f, 200.0f), EntityInstanceId(12345), 2.5f, 20},
        {Vector3(1.5f, 2.5f, 3.5f), EntityInstanceId(2000000000), -1.0f, 1},
    };

    for (const auto& tc : cases) {
        auto original = ParticlePacket::createVibration(tc.pos, tc.entityId, tc.yOffset, tc.ticks);
        auto result = original.serialize();
        ASSERT_TRUE(result.success()) << "Serialization failed for entityId=" << tc.entityId;

        ParticlePacket deserialized;
        auto deserResult = deserialized.deserialize(result.value().data(), result.value().size());
        ASSERT_TRUE(deserResult.success()) << "Deserialization failed";

        auto decodedTarget = deserialized.decodeVibrationTarget();
        ASSERT_TRUE(decodedTarget.has_value());
        ASSERT_EQ(decodedTarget->kind, ParticlePacket::VibrationTarget::Kind::Entity);
        EXPECT_EQ(decodedTarget->entityId, tc.entityId);
        EXPECT_FLOAT_EQ(decodedTarget->yOffset, tc.yOffset);

        auto decodedTicks = deserialized.decodeVibrationArrivalInTicks();
        ASSERT_TRUE(decodedTicks.has_value());
        EXPECT_EQ(decodedTicks.value(), tc.ticks);
    }
}

// ==================== BlockPos.asLong/fromLong 位打包测试 ====================
// 验证 packedBlockPos 的位布局与 MC Java BlockPos.asLong 一致

TEST(ParticlePacketVibrationTest, PackedBlockPos_BitLayout_MatchesMinecraft)
{
    // 验证 packed long 的位布局：X(26位,bit38-63) | Z(26位,bit12-37) | Y(12位,bit0-11)
    // 取一组坐标，打包后检查各个字段是否能通过位移提取
    BlockPos pos(123456, -64, -789012);
    i64 packed = pos.asLong();

    // 通过 fromLong 还原应得到相同的坐标
    BlockPos restored = BlockPos::fromLong(packed);
    EXPECT_EQ(restored.x, 123456);
    EXPECT_EQ(restored.y, -64);
    EXPECT_EQ(restored.z, -789012);

    // 直接验证 getXFromLong/getYFromLong/getZFromLong
    EXPECT_EQ(BlockPos::getXFromLong(packed), 123456);
    EXPECT_EQ(BlockPos::getYFromLong(packed), -64);
    EXPECT_EQ(BlockPos::getZFromLong(packed), -789012);
}

TEST(ParticlePacketVibrationTest, PackedBlockPos_ZeroCoordinates)
{
    BlockPos pos(0, 0, 0);
    i64 packed = pos.asLong();

    EXPECT_EQ(packed, 0);
    EXPECT_EQ(BlockPos::fromLong(packed), pos);
}

TEST(ParticlePacketVibrationTest, PackedBlockPos_NegativeCoordinates)
{
    BlockPos pos(-1, -1, -1);
    i64 packed = pos.asLong();

    BlockPos restored = BlockPos::fromLong(packed);
    EXPECT_EQ(restored.x, -1);
    EXPECT_EQ(restored.y, -1);
    EXPECT_EQ(restored.z, -1);
}
