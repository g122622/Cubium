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

// ==================== createDust 工厂方法测试 ====================

TEST(ParticlePacketDustTest, CreateDust_SetsCorrectType)
{
    Vector3 pos(10.0f, 64.0f, -20.0f);
    Vector3 vel(0.1f, 0.2f, 0.3f);
    Vector3 offset(0.5f, 0.5f, 0.5f);

    auto packet = ParticlePacket::createDust(ParticleTypeId::Dust, pos, vel, offset, 5, 0xFFFF0000, 1.0f);

    EXPECT_EQ(packet.particleType(), ParticleTypeId::Dust);
}

TEST(ParticlePacketDustTest, CreateDust_SetsPosition)
{
    Vector3 pos(10.0f, 64.0f, -20.0f);
    Vector3 vel(0.1f, 0.2f, 0.3f);
    Vector3 offset(0.5f, 0.5f, 0.5f);

    auto packet = ParticlePacket::createDust(ParticleTypeId::Dust, pos, vel, offset, 5, 0xFFFF0000, 1.0f);

    EXPECT_DOUBLE_EQ(packet.x(), 10.0);
    EXPECT_DOUBLE_EQ(packet.y(), 64.0);
    EXPECT_DOUBLE_EQ(packet.z(), -20.0);
}

TEST(ParticlePacketDustTest, CreateDust_SetsVelocityAndOffset)
{
    Vector3 pos(0.0f, 0.0f, 0.0f);
    Vector3 vel(0.1f, 0.2f, 0.3f);
    Vector3 offset(0.5f, 0.5f, 0.5f);

    auto packet = ParticlePacket::createDust(ParticleTypeId::Dust, pos, vel, offset, 5, 0xFFFF0000, 1.0f);

    EXPECT_FLOAT_EQ(packet.velocityX(), 0.1f);
    EXPECT_FLOAT_EQ(packet.velocityY(), 0.2f);
    EXPECT_FLOAT_EQ(packet.velocityZ(), 0.3f);
    EXPECT_FLOAT_EQ(packet.offsetX(), 0.5f);
    EXPECT_FLOAT_EQ(packet.offsetY(), 0.5f);
    EXPECT_FLOAT_EQ(packet.offsetZ(), 0.5f);
}

TEST(ParticlePacketDustTest, CreateDust_SetsCount)
{
    Vector3 pos(0, 0, 0);
    Vector3 vel(0, 0, 0);
    Vector3 offset(0, 0, 0);

    auto packet = ParticlePacket::createDust(ParticleTypeId::Dust, pos, vel, offset, 10, 0xFFFF0000, 1.0f);

    EXPECT_EQ(packet.count(), 10u);
}

TEST(ParticlePacketDustTest, CreateDust_HasOptionalData)
{
    Vector3 pos(0, 0, 0);
    Vector3 vel(0, 0, 0);
    Vector3 offset(0, 0, 0);

    auto packet = ParticlePacket::createDust(ParticleTypeId::Dust, pos, vel, offset, 1, 0xFFFF0000, 1.0f);

    EXPECT_FALSE(packet.optionalData().empty());
    // i32 color + f32 scale = 8 bytes
    EXPECT_GE(packet.optionalData().size(), 8u);
}

TEST(ParticlePacketDustTest, CreateDust_RedstoneType)
{
    Vector3 pos(0, 0, 0);
    Vector3 vel(0, 0, 0);
    Vector3 offset(0, 0, 0);

    auto packet = ParticlePacket::createDust(ParticleTypeId::Redstone, pos, vel, offset, 1, 0xFFFF0000, 1.0f);

    EXPECT_EQ(packet.particleType(), ParticleTypeId::Redstone);
    EXPECT_TRUE(packet.isDustParticle());
}

// ==================== isDustParticle 测试 ====================

TEST(ParticlePacketDustTest, IsDustParticle_TrueForDustWithOptionalData)
{
    Vector3 pos(0, 0, 0);
    Vector3 vel(0, 0, 0);
    Vector3 offset(0, 0, 0);

    auto packet = ParticlePacket::createDust(ParticleTypeId::Dust, pos, vel, offset, 1, 0xFFFF0000, 1.0f);

    EXPECT_TRUE(packet.isDustParticle());
}

TEST(ParticlePacketDustTest, IsDustParticle_FalseForDustWithoutOptionalData)
{
    ParticlePacket packet(ParticleTypeId::Dust, Vector3(0, 0, 0), Vector3(0, 0, 0), Vector3(0, 0, 0), 1);

    EXPECT_FALSE(packet.isDustParticle());
}

TEST(ParticlePacketDustTest, IsDustParticle_FalseForNonDustType)
{
    ParticlePacket packet(ParticleTypeId::Flame, Vector3(0, 0, 0), Vector3(0, 0, 0), Vector3(0, 0, 0), 1);
    std::vector<u8> fakeData(8, 0x00);
    packet.setOptionalData(fakeData);

    EXPECT_FALSE(packet.isDustParticle());
}

// ==================== decodeDustColor/Scale 测试 ====================

TEST(ParticlePacketDustTest, DecodeDustColor_ReturnsCorrectColor)
{
    Vector3 pos(0, 0, 0);
    Vector3 vel(0, 0, 0);
    Vector3 offset(0, 0, 0);

    auto packet = ParticlePacket::createDust(ParticleTypeId::Dust, pos, vel, offset, 1, 0xFF00FF00, 2.0f);

    auto color = packet.decodeDustColor();
    ASSERT_TRUE(color.has_value());
    EXPECT_EQ(color.value(), 0xFF00FF00u);
}

TEST(ParticlePacketDustTest, DecodeDustScale_ReturnsCorrectScale)
{
    Vector3 pos(0, 0, 0);
    Vector3 vel(0, 0, 0);
    Vector3 offset(0, 0, 0);

    auto packet = ParticlePacket::createDust(ParticleTypeId::Dust, pos, vel, offset, 1, 0xFFFF0000, 2.5f);

    auto scale = packet.decodeDustScale();
    ASSERT_TRUE(scale.has_value());
    EXPECT_FLOAT_EQ(scale.value(), 2.5f);
}

TEST(ParticlePacketDustTest, DecodeDustColor_ReturnsNulloptForNonDust)
{
    ParticlePacket packet(ParticleTypeId::Flame, Vector3(0, 0, 0), Vector3(0, 0, 0), Vector3(0, 0, 0), 1);

    EXPECT_FALSE(packet.decodeDustColor().has_value());
}

TEST(ParticlePacketDustTest, DecodeDustScale_ReturnsNulloptForEmptyOptionalData)
{
    ParticlePacket packet(ParticleTypeId::Dust, Vector3(0, 0, 0), Vector3(0, 0, 0), Vector3(0, 0, 0), 1);

    EXPECT_FALSE(packet.decodeDustScale().has_value());
}

// ==================== 灰尘粒子序列化/反序列化测试 ====================

TEST(ParticlePacketDustTest, SerializeDeserialize_DustPacket)
{
    Vector3 pos(10.0f, 64.0f, -20.0f);
    Vector3 vel(0.1f, 0.2f, 0.3f);
    Vector3 offset(0.5f, 0.5f, 0.5f);

    auto original = ParticlePacket::createDust(ParticleTypeId::Dust, pos, vel, offset, 5, 0xFF123456, 3.0f);

    auto result = original.serialize();
    ASSERT_TRUE(result.success()) << result.error().message();

    ParticlePacket deserialized;
    auto deserResult = deserialized.deserialize(result.value().data(), result.value().size());
    ASSERT_TRUE(deserResult.success()) << deserResult.error().message();

    EXPECT_EQ(deserialized.particleType(), ParticleTypeId::Dust);
    EXPECT_DOUBLE_EQ(deserialized.x(), 10.0);
    EXPECT_DOUBLE_EQ(deserialized.y(), 64.0);
    EXPECT_DOUBLE_EQ(deserialized.z(), -20.0);
    EXPECT_EQ(deserialized.count(), 5u);

    EXPECT_TRUE(deserialized.isDustParticle());

    auto decodedColor = deserialized.decodeDustColor();
    ASSERT_TRUE(decodedColor.has_value());
    EXPECT_EQ(decodedColor.value(), 0xFF123456u);

    auto decodedScale = deserialized.decodeDustScale();
    ASSERT_TRUE(decodedScale.has_value());
    EXPECT_FLOAT_EQ(decodedScale.value(), 3.0f);
}

// ==================== createDustColorTransition 测试 ====================

TEST(ParticlePacketDustTest, CreateDustColorTransition_SetsCorrectType)
{
    Vector3 pos(0, 0, 0);
    Vector3 vel(0, 0, 0);
    Vector3 offset(0, 0, 0);

    auto packet = ParticlePacket::createDustColorTransition(pos, vel, offset, 1, 0xFF39E5C0, 0xFFFF0000, 1.0f);

    EXPECT_EQ(packet.particleType(), ParticleTypeId::DustColorTransition);
}

TEST(ParticlePacketDustTest, CreateDustColorTransition_HasOptionalData)
{
    Vector3 pos(0, 0, 0);
    Vector3 vel(0, 0, 0);
    Vector3 offset(0, 0, 0);

    auto packet = ParticlePacket::createDustColorTransition(pos, vel, offset, 1, 0xFF39E5C0, 0xFFFF0000, 1.0f);

    EXPECT_FALSE(packet.optionalData().empty());
    // 2*i32 + f32 = 12 bytes
    EXPECT_GE(packet.optionalData().size(), 12u);
}

// ==================== isDustColorTransitionParticle 测试 ====================

TEST(ParticlePacketDustTest, IsDustColorTransition_TrueWithOptionalData)
{
    Vector3 pos(0, 0, 0);
    Vector3 vel(0, 0, 0);
    Vector3 offset(0, 0, 0);

    auto packet = ParticlePacket::createDustColorTransition(pos, vel, offset, 1, 0xFF39E5C0, 0xFFFF0000, 1.0f);

    EXPECT_TRUE(packet.isDustColorTransitionParticle());
}

TEST(ParticlePacketDustTest, IsDustColorTransition_FalseWithoutOptionalData)
{
    ParticlePacket packet(ParticleTypeId::DustColorTransition, Vector3(0, 0, 0), Vector3(0, 0, 0), Vector3(0, 0, 0), 1);

    EXPECT_FALSE(packet.isDustColorTransitionParticle());
}

// ==================== decodeDustColorTransition 测试 ====================

TEST(ParticlePacketDustTest, DecodeDustColorTransition_FromColor)
{
    Vector3 pos(0, 0, 0);
    Vector3 vel(0, 0, 0);
    Vector3 offset(0, 0, 0);

    auto packet = ParticlePacket::createDustColorTransition(pos, vel, offset, 1, 0xFF39E5C0, 0xFFFF0000, 1.0f);

    auto fromColor = packet.decodeDustColorTransitionFromColor();
    ASSERT_TRUE(fromColor.has_value());
    EXPECT_EQ(fromColor.value(), 0xFF39E5C0u);
}

TEST(ParticlePacketDustTest, DecodeDustColorTransition_ToColor)
{
    Vector3 pos(0, 0, 0);
    Vector3 vel(0, 0, 0);
    Vector3 offset(0, 0, 0);

    auto packet = ParticlePacket::createDustColorTransition(pos, vel, offset, 1, 0xFF39E5C0, 0xFFFF0000, 1.0f);

    auto toColor = packet.decodeDustColorTransitionToColor();
    ASSERT_TRUE(toColor.has_value());
    EXPECT_EQ(toColor.value(), 0xFFFF0000u);
}

TEST(ParticlePacketDustTest, DecodeDustColorTransition_Scale)
{
    Vector3 pos(0, 0, 0);
    Vector3 vel(0, 0, 0);
    Vector3 offset(0, 0, 0);

    auto packet = ParticlePacket::createDustColorTransition(pos, vel, offset, 1, 0xFF39E5C0, 0xFFFF0000, 2.5f);

    auto scale = packet.decodeDustColorTransitionScale();
    ASSERT_TRUE(scale.has_value());
    EXPECT_FLOAT_EQ(scale.value(), 2.5f);
}

TEST(ParticlePacketDustTest, DecodeDustColorTransition_ReturnsNulloptForNonTransition)
{
    ParticlePacket packet(ParticleTypeId::Dust, Vector3(0, 0, 0), Vector3(0, 0, 0), Vector3(0, 0, 0), 1);

    EXPECT_FALSE(packet.decodeDustColorTransitionFromColor().has_value());
    EXPECT_FALSE(packet.decodeDustColorTransitionToColor().has_value());
    EXPECT_FALSE(packet.decodeDustColorTransitionScale().has_value());
}

// ==================== DustColorTransition 序列化/反序列化测试 ====================

TEST(ParticlePacketDustTest, SerializeDeserialize_DustColorTransitionPacket)
{
    Vector3 pos(10.0f, 64.0f, -20.0f);
    Vector3 vel(0.1f, 0.2f, 0.3f);
    Vector3 offset(0.5f, 0.5f, 0.5f);

    auto original = ParticlePacket::createDustColorTransition(pos, vel, offset, 3, 0xFF39E5C0, 0xFFFF0000, 2.0f);

    auto result = original.serialize();
    ASSERT_TRUE(result.success()) << result.error().message();

    ParticlePacket deserialized;
    auto deserResult = deserialized.deserialize(result.value().data(), result.value().size());
    ASSERT_TRUE(deserResult.success()) << deserResult.error().message();

    EXPECT_EQ(deserialized.particleType(), ParticleTypeId::DustColorTransition);
    EXPECT_DOUBLE_EQ(deserialized.x(), 10.0);
    EXPECT_DOUBLE_EQ(deserialized.y(), 64.0);
    EXPECT_DOUBLE_EQ(deserialized.z(), -20.0);
    EXPECT_EQ(deserialized.count(), 3u);

    EXPECT_TRUE(deserialized.isDustColorTransitionParticle());

    auto fromColor = deserialized.decodeDustColorTransitionFromColor();
    ASSERT_TRUE(fromColor.has_value());
    EXPECT_EQ(fromColor.value(), 0xFF39E5C0u);

    auto toColor = deserialized.decodeDustColorTransitionToColor();
    ASSERT_TRUE(toColor.has_value());
    EXPECT_EQ(toColor.value(), 0xFFFF0000u);

    auto scale = deserialized.decodeDustColorTransitionScale();
    ASSERT_TRUE(scale.has_value());
    EXPECT_FLOAT_EQ(scale.value(), 2.0f);
}
