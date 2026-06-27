/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to Use, copy, modify, merge, publish, distribute, sublicense, and/or sell
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
#include "network/packet/ParticlePacket.hpp"
#include <gtest/gtest.h>

using namespace mc::network;
using namespace mc::client::renderer::trident::particle;
using mc::f32;
using mc::f64;
using mc::i32;
using mc::u32;
using mc::u8;
using mc::Vector3;
using mc::Vector3d;

// ==================== createVibration 工厂方法测试 ====================

TEST(ParticlePacketVibrationTest, CreateVibration_SetsCorrectType)
{
    Vector3 pos(10.0f, 64.0f, -20.0f);
    Vector3d target(100.5, 70.0, -50.25);

    auto packet = ParticlePacket::createVibration(pos, target, 15);

    EXPECT_EQ(packet.particleType(), ParticleTypeId::Vibration);
}

TEST(ParticlePacketVibrationTest, CreateVibration_SetsPosition)
{
    Vector3 pos(10.0f, 64.0f, -20.0f);
    Vector3d target(100.5, 70.0, -50.25);

    auto packet = ParticlePacket::createVibration(pos, target, 15);

    EXPECT_DOUBLE_EQ(packet.x(), 10.0);
    EXPECT_DOUBLE_EQ(packet.y(), 64.0);
    EXPECT_DOUBLE_EQ(packet.z(), -20.0);
}

TEST(ParticlePacketVibrationTest, CreateVibration_ZeroVelocityAndOffset)
{
    Vector3 pos(0.0f, 0.0f, 0.0f);
    Vector3d target(50.0, 100.0, 150.0);

    auto packet = ParticlePacket::createVibration(pos, target, 10);

    // 振动粒子包速度和偏移应为零
    EXPECT_FLOAT_EQ(packet.velocityX(), 0.0f);
    EXPECT_FLOAT_EQ(packet.velocityY(), 0.0f);
    EXPECT_FLOAT_EQ(packet.velocityZ(), 0.0f);
    EXPECT_FLOAT_EQ(packet.offsetX(), 0.0f);
    EXPECT_FLOAT_EQ(packet.offsetY(), 0.0f);
    EXPECT_FLOAT_EQ(packet.offsetZ(), 0.0f);
}

TEST(ParticlePacketVibrationTest, CreateVibration_CountIsOne)
{
    Vector3 pos(0.0f, 0.0f, 0.0f);
    Vector3d target(50.0, 100.0, 150.0);

    auto packet = ParticlePacket::createVibration(pos, target, 10);

    // 振动粒子包数量始终为 1
    EXPECT_EQ(packet.count(), 1u);
}

TEST(ParticlePacketVibrationTest, CreateVibration_HasOptionalData)
{
    Vector3 pos(0.0f, 0.0f, 0.0f);
    Vector3d target(50.0, 100.0, 150.0);

    auto packet = ParticlePacket::createVibration(pos, target, 10);

    // 振动粒子包应包含可选数据（3 * f64 + VarInt）
    EXPECT_FALSE(packet.optionalData().empty());
    // 最少需要 3 * sizeof(f64) = 24 字节
    EXPECT_GE(packet.optionalData().size(), 24u);
}

// ==================== isVibrationParticle 测试 ====================

TEST(ParticlePacketVibrationTest, IsVibrationParticle_TrueForVibrationWithOptionalData)
{
    Vector3 pos(0.0f, 0.0f, 0.0f);
    Vector3d target(50.0, 100.0, 150.0);

    auto packet = ParticlePacket::createVibration(pos, target, 10);

    EXPECT_TRUE(packet.isVibrationParticle());
}

TEST(ParticlePacketVibrationTest, IsVibrationParticle_FalseForVibrationWithoutOptionalData)
{
    // 纯 Vibration 类型粒子包（无可选数据）不应识别为振动粒子
    ParticlePacket packet(ParticleTypeId::Vibration, Vector3(0, 0, 0), Vector3(0, 0, 0), Vector3(0, 0, 0), 1);

    EXPECT_FALSE(packet.isVibrationParticle());
}

TEST(ParticlePacketVibrationTest, IsVibrationParticle_FalseForNonVibrationType)
{
    // 非 Vibration 类型的粒子包即使有可选数据也不应识别为振动粒子
    ParticlePacket packet(ParticleTypeId::Flame, Vector3(0, 0, 0), Vector3(0, 0, 0), Vector3(0, 0, 0), 1);
    std::vector<u8> fakeData(32, 0x00);
    packet.setOptionalData(fakeData);

    EXPECT_FALSE(packet.isVibrationParticle());
}

// ==================== decodeVibrationTarget 测试 ====================

TEST(ParticlePacketVibrationTest, DecodeVibrationTarget_ReturnsCorrectPosition)
{
    Vector3 pos(10.0f, 64.0f, -20.0f);
    Vector3d target(100.5, 70.0, -50.25);

    auto packet = ParticlePacket::createVibration(pos, target, 15);

    auto decoded = packet.decodeVibrationTarget();
    ASSERT_TRUE(decoded.has_value());

    EXPECT_DOUBLE_EQ(decoded->x, 100.5);
    EXPECT_DOUBLE_EQ(decoded->y, 70.0);
    EXPECT_DOUBLE_EQ(decoded->z, -50.25);
}

TEST(ParticlePacketVibrationTest, DecodeVibrationTarget_NegativeCoordinates)
{
    Vector3 pos(0.0f, 0.0f, 0.0f);
    Vector3d target(-100.5, -64.0, -200.75);

    auto packet = ParticlePacket::createVibration(pos, target, 8);

    auto decoded = packet.decodeVibrationTarget();
    ASSERT_TRUE(decoded.has_value());

    EXPECT_DOUBLE_EQ(decoded->x, -100.5);
    EXPECT_DOUBLE_EQ(decoded->y, -64.0);
    EXPECT_DOUBLE_EQ(decoded->z, -200.75);
}

TEST(ParticlePacketVibrationTest, DecodeVibrationTarget_ZeroPosition)
{
    Vector3 pos(50.0f, 50.0f, 50.0f);
    Vector3d target(0.0, 0.0, 0.0);

    auto packet = ParticlePacket::createVibration(pos, target, 5);

    auto decoded = packet.decodeVibrationTarget();
    ASSERT_TRUE(decoded.has_value());

    EXPECT_DOUBLE_EQ(decoded->x, 0.0);
    EXPECT_DOUBLE_EQ(decoded->y, 0.0);
    EXPECT_DOUBLE_EQ(decoded->z, 0.0);
}

TEST(ParticlePacketVibrationTest, DecodeVibrationTarget_LargeCoordinates)
{
    Vector3 pos(0.0f, 0.0f, 0.0f);
    Vector3d target(30000000.0, 256.0, 30000000.0);

    auto packet = ParticlePacket::createVibration(pos, target, 100);

    auto decoded = packet.decodeVibrationTarget();
    ASSERT_TRUE(decoded.has_value());

    EXPECT_DOUBLE_EQ(decoded->x, 30000000.0);
    EXPECT_DOUBLE_EQ(decoded->y, 256.0);
    EXPECT_DOUBLE_EQ(decoded->z, 30000000.0);
}

TEST(ParticlePacketVibrationTest, DecodeVibrationTarget_ReturnsNulloptForNonVibration)
{
    ParticlePacket packet(ParticleTypeId::Flame, Vector3(0, 0, 0), Vector3(0, 0, 0), Vector3(0, 0, 0), 1);

    auto decoded = packet.decodeVibrationTarget();
    EXPECT_FALSE(decoded.has_value());
}

TEST(ParticlePacketVibrationTest, DecodeVibrationTarget_ReturnsNulloptForEmptyOptionalData)
{
    // Vibration 类型但没有可选数据
    ParticlePacket packet(ParticleTypeId::Vibration, Vector3(0, 0, 0), Vector3(0, 0, 0), Vector3(0, 0, 0), 1);

    auto decoded = packet.decodeVibrationTarget();
    EXPECT_FALSE(decoded.has_value());
}

// ==================== decodeVibrationArrivalInTicks 测试 ====================

TEST(ParticlePacketVibrationTest, DecodeVibrationArrivalInTicks_ReturnsCorrectTicks)
{
    Vector3 pos(10.0f, 64.0f, -20.0f);
    Vector3d target(100.5, 70.0, -50.25);

    auto packet = ParticlePacket::createVibration(pos, target, 15);

    auto decoded = packet.decodeVibrationArrivalInTicks();
    ASSERT_TRUE(decoded.has_value());

    EXPECT_EQ(decoded.value(), 15);
}

TEST(ParticlePacketVibrationTest, DecodeVibrationArrivalInTicks_SingleTick)
{
    Vector3 pos(0.0f, 0.0f, 0.0f);
    Vector3d target(1.0, 1.0, 1.0);

    auto packet = ParticlePacket::createVibration(pos, target, 1);

    auto decoded = packet.decodeVibrationArrivalInTicks();
    ASSERT_TRUE(decoded.has_value());

    EXPECT_EQ(decoded.value(), 1);
}

TEST(ParticlePacketVibrationTest, DecodeVibrationArrivalInTicks_LargeTicks)
{
    Vector3 pos(0.0f, 0.0f, 0.0f);
    Vector3d target(50.0, 100.0, 150.0);

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

TEST(ParticlePacketVibrationTest, SerializeDeserialize_VibrationPacket)
{
    Vector3 pos(10.0f, 64.0f, -20.0f);
    Vector3d target(100.5, 70.0, -50.25);

    auto original = ParticlePacket::createVibration(pos, target, 15);

    // 序列化
    auto result = original.serialize();
    ASSERT_TRUE(result.success()) << result.error().message();

    // 反序列化
    ParticlePacket deserialized;
    auto deserResult = deserialized.deserialize(result.value().data(), result.value().size());
    ASSERT_TRUE(deserResult.success()) << deserResult.error().message();

    // 验证基本字段
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

    // 验证可选数据完整
    EXPECT_TRUE(deserialized.isVibrationParticle());

    // 验证振动目标位置
    auto decodedTarget = deserialized.decodeVibrationTarget();
    ASSERT_TRUE(decodedTarget.has_value());
    EXPECT_DOUBLE_EQ(decodedTarget->x, 100.5);
    EXPECT_DOUBLE_EQ(decodedTarget->y, 70.0);
    EXPECT_DOUBLE_EQ(decodedTarget->z, -50.25);

    // 验证到达 tick 数
    auto decodedTicks = deserialized.decodeVibrationArrivalInTicks();
    ASSERT_TRUE(decodedTicks.has_value());
    EXPECT_EQ(decodedTicks.value(), 15);
}

TEST(ParticlePacketVibrationTest, SerializeDeserialize_VibrationPacket_NegativeTarget)
{
    Vector3 pos(0.0f, 0.0f, 0.0f);
    Vector3d target(-100.5, -64.0, -200.75);

    auto original = ParticlePacket::createVibration(pos, target, 8);

    auto result = original.serialize();
    ASSERT_TRUE(result.success()) << result.error().message();

    ParticlePacket deserialized;
    auto deserResult = deserialized.deserialize(result.value().data(), result.value().size());
    ASSERT_TRUE(deserResult.success()) << deserResult.error().message();

    auto decodedTarget = deserialized.decodeVibrationTarget();
    ASSERT_TRUE(decodedTarget.has_value());
    EXPECT_DOUBLE_EQ(decodedTarget->x, -100.5);
    EXPECT_DOUBLE_EQ(decodedTarget->y, -64.0);
    EXPECT_DOUBLE_EQ(decodedTarget->z, -200.75);

    auto decodedTicks = deserialized.decodeVibrationArrivalInTicks();
    ASSERT_TRUE(decodedTicks.has_value());
    EXPECT_EQ(decodedTicks.value(), 8);
}

TEST(ParticlePacketVibrationTest, SerializeDeserialize_VibrationPacket_ZeroTarget)
{
    Vector3 pos(10.0f, 20.0f, 30.0f);
    Vector3d target(0.0, 0.0, 0.0);

    auto original = ParticlePacket::createVibration(pos, target, 5);

    auto result = original.serialize();
    ASSERT_TRUE(result.success()) << result.error().message();

    ParticlePacket deserialized;
    auto deserResult = deserialized.deserialize(result.value().data(), result.value().size());
    ASSERT_TRUE(deserResult.success()) << deserResult.error().message();

    auto decodedTarget = deserialized.decodeVibrationTarget();
    ASSERT_TRUE(decodedTarget.has_value());
    EXPECT_DOUBLE_EQ(decodedTarget->x, 0.0);
    EXPECT_DOUBLE_EQ(decodedTarget->y, 0.0);
    EXPECT_DOUBLE_EQ(decodedTarget->z, 0.0);

    auto decodedTicks = deserialized.decodeVibrationArrivalInTicks();
    ASSERT_TRUE(decodedTicks.has_value());
    EXPECT_EQ(decodedTicks.value(), 5);
}

// ==================== 振动粒子数据完整性测试 ====================

TEST(ParticlePacketVibrationTest, VibrationDataRoundTrip_MultiplePackets)
{
    // 多个振动粒子包的序列化/反序列化应保持数据完整
    struct TestCase {
        Vector3 pos;
        Vector3d target;
        i32 ticks;
    };

    std::vector<TestCase> cases = {
        {Vector3(0.0f, 0.0f, 0.0f), Vector3d(10.0, 20.0, 30.0), 5},
        {Vector3(100.0f, -64.0f, 200.0f), Vector3d(-50.5, 300.0, -100.25), 20},
        {Vector3(1.5f, 2.5f, 3.5f), Vector3d(0.001, 0.002, 0.003), 1},
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
        EXPECT_DOUBLE_EQ(decodedTarget->x, tc.target.x);
        EXPECT_DOUBLE_EQ(decodedTarget->y, tc.target.y);
        EXPECT_DOUBLE_EQ(decodedTarget->z, tc.target.z);

        auto decodedTicks = deserialized.decodeVibrationArrivalInTicks();
        ASSERT_TRUE(decodedTicks.has_value());
        EXPECT_EQ(decodedTicks.value(), tc.ticks);
    }
}
