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

// ==================== createTrail 工厂方法测试 ====================

TEST(ParticlePacketTrailTest, CreateTrail_SetsCorrectType)
{
    Vector3 pos(10.0f, 64.0f, -20.0f);
    Vector3d target(100.5, 70.0, -50.25);

    auto packet = ParticlePacket::createTrail(pos, target, 0xFFFFFFFF, 10);

    EXPECT_EQ(packet.particleType(), ParticleTypeId::Trail);
}

TEST(ParticlePacketTrailTest, CreateTrail_SetsPosition)
{
    Vector3 pos(10.0f, 64.0f, -20.0f);
    Vector3d target(100.5, 70.0, -50.25);

    auto packet = ParticlePacket::createTrail(pos, target, 0xFFFFFFFF, 10);

    EXPECT_DOUBLE_EQ(packet.x(), 10.0);
    EXPECT_DOUBLE_EQ(packet.y(), 64.0);
    EXPECT_DOUBLE_EQ(packet.z(), -20.0);
}

TEST(ParticlePacketTrailTest, CreateTrail_ZeroVelocityAndOffset)
{
    Vector3 pos(0.0f, 0.0f, 0.0f);
    Vector3d target(50.0, 100.0, 150.0);

    auto packet = ParticlePacket::createTrail(pos, target, 0xFFFF0000, 5);

    EXPECT_FLOAT_EQ(packet.velocityX(), 0.0f);
    EXPECT_FLOAT_EQ(packet.velocityY(), 0.0f);
    EXPECT_FLOAT_EQ(packet.velocityZ(), 0.0f);
    EXPECT_FLOAT_EQ(packet.offsetX(), 0.0f);
    EXPECT_FLOAT_EQ(packet.offsetY(), 0.0f);
    EXPECT_FLOAT_EQ(packet.offsetZ(), 0.0f);
}

TEST(ParticlePacketTrailTest, CreateTrail_CountIsOne)
{
    Vector3 pos(0.0f, 0.0f, 0.0f);
    Vector3d target(50.0, 100.0, 150.0);

    auto packet = ParticlePacket::createTrail(pos, target, 0xFFFFFFFF, 10);

    EXPECT_EQ(packet.count(), 1u);
}

TEST(ParticlePacketTrailTest, CreateTrail_HasOptionalData)
{
    Vector3 pos(0.0f, 0.0f, 0.0f);
    Vector3d target(50.0, 100.0, 150.0);

    auto packet = ParticlePacket::createTrail(pos, target, 0xFFFFFFFF, 10);

    EXPECT_FALSE(packet.optionalData().empty());
    // 最少需要 3*f64 + i32 + VarInt = 28+ 字节
    EXPECT_GE(packet.optionalData().size(), 28u);
}

// ==================== isTrailParticle 测试 ====================

TEST(ParticlePacketTrailTest, IsTrailParticle_TrueForTrailWithOptionalData)
{
    Vector3 pos(0.0f, 0.0f, 0.0f);
    Vector3d target(50.0, 100.0, 150.0);

    auto packet = ParticlePacket::createTrail(pos, target, 0xFFFFFFFF, 10);

    EXPECT_TRUE(packet.isTrailParticle());
}

TEST(ParticlePacketTrailTest, IsTrailParticle_FalseForTrailWithoutOptionalData)
{
    ParticlePacket packet(ParticleTypeId::Trail, Vector3(0, 0, 0), Vector3(0, 0, 0), Vector3(0, 0, 0), 1);

    EXPECT_FALSE(packet.isTrailParticle());
}

TEST(ParticlePacketTrailTest, IsTrailParticle_FalseForNonTrailType)
{
    ParticlePacket packet(ParticleTypeId::Flame, Vector3(0, 0, 0), Vector3(0, 0, 0), Vector3(0, 0, 0), 1);
    std::vector<u8> fakeData(32, 0x00);
    packet.setOptionalData(fakeData);

    EXPECT_FALSE(packet.isTrailParticle());
}

// ==================== decodeTrailTarget/Color/Duration 测试 ====================

TEST(ParticlePacketTrailTest, DecodeTrailTarget_ReturnsCorrectPosition)
{
    Vector3 pos(10.0f, 64.0f, -20.0f);
    Vector3d target(100.5, 70.0, -50.25);

    auto packet = ParticlePacket::createTrail(pos, target, 0xFFFF0000, 15);

    auto decoded = packet.decodeTrailTarget();
    ASSERT_TRUE(decoded.has_value());

    EXPECT_DOUBLE_EQ(decoded->x, 100.5);
    EXPECT_DOUBLE_EQ(decoded->y, 70.0);
    EXPECT_DOUBLE_EQ(decoded->z, -50.25);
}

TEST(ParticlePacketTrailTest, DecodeTrailColor_ReturnsCorrectColor)
{
    Vector3 pos(0.0f, 0.0f, 0.0f);
    Vector3d target(50.0, 100.0, 150.0);

    auto packet = ParticlePacket::createTrail(pos, target, 0xFF00FF00, 10);

    auto color = packet.decodeTrailColor();
    ASSERT_TRUE(color.has_value());
    EXPECT_EQ(color.value(), 0xFF00FF00u);
}

TEST(ParticlePacketTrailTest, DecodeTrailDuration_ReturnsCorrectDuration)
{
    Vector3 pos(0.0f, 0.0f, 0.0f);
    Vector3d target(50.0, 100.0, 150.0);

    auto packet = ParticlePacket::createTrail(pos, target, 0xFFFFFFFF, 42);

    auto duration = packet.decodeTrailDuration();
    ASSERT_TRUE(duration.has_value());
    EXPECT_EQ(duration.value(), 42);
}

TEST(ParticlePacketTrailTest, DecodeTrailTarget_NegativeCoordinates)
{
    Vector3 pos(0.0f, 0.0f, 0.0f);
    Vector3d target(-100.5, -64.0, -200.75);

    auto packet = ParticlePacket::createTrail(pos, target, 0xFFFFFFFF, 10);

    auto decoded = packet.decodeTrailTarget();
    ASSERT_TRUE(decoded.has_value());

    EXPECT_DOUBLE_EQ(decoded->x, -100.5);
    EXPECT_DOUBLE_EQ(decoded->y, -64.0);
    EXPECT_DOUBLE_EQ(decoded->z, -200.75);
}

TEST(ParticlePacketTrailTest, DecodeTrailTarget_ReturnsNulloptForNonTrail)
{
    ParticlePacket packet(ParticleTypeId::Flame, Vector3(0, 0, 0), Vector3(0, 0, 0), Vector3(0, 0, 0), 1);

    EXPECT_FALSE(packet.decodeTrailTarget().has_value());
}

TEST(ParticlePacketTrailTest, DecodeTrailColor_ReturnsNulloptForEmptyOptionalData)
{
    ParticlePacket packet(ParticleTypeId::Trail, Vector3(0, 0, 0), Vector3(0, 0, 0), Vector3(0, 0, 0), 1);

    EXPECT_FALSE(packet.decodeTrailColor().has_value());
}

TEST(ParticlePacketTrailTest, DecodeTrailDuration_ReturnsNulloptForEmptyOptionalData)
{
    ParticlePacket packet(ParticleTypeId::Trail, Vector3(0, 0, 0), Vector3(0, 0, 0), Vector3(0, 0, 0), 1);

    EXPECT_FALSE(packet.decodeTrailDuration().has_value());
}

// ==================== 轨迹粒子序列化/反序列化测试 ====================

TEST(ParticlePacketTrailTest, SerializeDeserialize_TrailPacket)
{
    Vector3 pos(10.0f, 64.0f, -20.0f);
    Vector3d target(100.5, 70.0, -50.25);

    auto original = ParticlePacket::createTrail(pos, target, 0xFF00FF00, 20);

    auto result = original.serialize();
    ASSERT_TRUE(result.success()) << result.error().message();

    ParticlePacket deserialized;
    auto deserResult = deserialized.deserialize(result.value().data(), result.value().size());
    ASSERT_TRUE(deserResult.success()) << deserResult.error().message();

    EXPECT_EQ(deserialized.particleType(), ParticleTypeId::Trail);
    EXPECT_DOUBLE_EQ(deserialized.x(), 10.0);
    EXPECT_DOUBLE_EQ(deserialized.y(), 64.0);
    EXPECT_DOUBLE_EQ(deserialized.z(), -20.0);
    EXPECT_EQ(deserialized.count(), 1u);

    EXPECT_TRUE(deserialized.isTrailParticle());

    auto decodedTarget = deserialized.decodeTrailTarget();
    ASSERT_TRUE(decodedTarget.has_value());
    EXPECT_DOUBLE_EQ(decodedTarget->x, 100.5);
    EXPECT_DOUBLE_EQ(decodedTarget->y, 70.0);
    EXPECT_DOUBLE_EQ(decodedTarget->z, -50.25);

    auto decodedColor = deserialized.decodeTrailColor();
    ASSERT_TRUE(decodedColor.has_value());
    EXPECT_EQ(decodedColor.value(), 0xFF00FF00u);

    auto decodedDuration = deserialized.decodeTrailDuration();
    ASSERT_TRUE(decodedDuration.has_value());
    EXPECT_EQ(decodedDuration.value(), 20);
}

TEST(ParticlePacketTrailTest, SerializeDeserialize_TrailPacket_NegativeTarget)
{
    Vector3 pos(0.0f, 0.0f, 0.0f);
    Vector3d target(-100.5, -64.0, -200.75);

    auto original = ParticlePacket::createTrail(pos, target, 0xFFFF0000, 8);

    auto result = original.serialize();
    ASSERT_TRUE(result.success()) << result.error().message();

    ParticlePacket deserialized;
    auto deserResult = deserialized.deserialize(result.value().data(), result.value().size());
    ASSERT_TRUE(deserResult.success()) << deserResult.error().message();

    auto decodedTarget = deserialized.decodeTrailTarget();
    ASSERT_TRUE(decodedTarget.has_value());
    EXPECT_DOUBLE_EQ(decodedTarget->x, -100.5);
    EXPECT_DOUBLE_EQ(decodedTarget->y, -64.0);
    EXPECT_DOUBLE_EQ(decodedTarget->z, -200.75);

    auto decodedColor = deserialized.decodeTrailColor();
    ASSERT_TRUE(decodedColor.has_value());
    EXPECT_EQ(decodedColor.value(), 0xFFFF0000u);

    auto decodedDuration = deserialized.decodeTrailDuration();
    ASSERT_TRUE(decodedDuration.has_value());
    EXPECT_EQ(decodedDuration.value(), 8);
}
