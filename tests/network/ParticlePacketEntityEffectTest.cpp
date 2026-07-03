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

// ==================== createEntityEffect 工厂方法测试 ====================

TEST(ParticlePacketEntityEffectTest, CreateEntityEffect_SetsCorrectType)
{
    Vector3 pos(10.0f, 64.0f, -20.0f);
    Vector3 vel(0.1f, 0.2f, 0.3f);
    Vector3 offset(0.5f, 0.5f, 0.5f);

    auto packet = ParticlePacket::createEntityEffect(pos, vel, offset, 5, 0xFFFF0000);

    EXPECT_EQ(packet.particleType(), ParticleTypeId::EntityEffect);
}

TEST(ParticlePacketEntityEffectTest, CreateEntityEffect_SetsPosition)
{
    Vector3 pos(10.0f, 64.0f, -20.0f);
    Vector3 vel(0.1f, 0.2f, 0.3f);
    Vector3 offset(0.5f, 0.5f, 0.5f);

    auto packet = ParticlePacket::createEntityEffect(pos, vel, offset, 5, 0xFFFF0000);

    EXPECT_DOUBLE_EQ(packet.x(), 10.0);
    EXPECT_DOUBLE_EQ(packet.y(), 64.0);
    EXPECT_DOUBLE_EQ(packet.z(), -20.0);
}

TEST(ParticlePacketEntityEffectTest, CreateEntityEffect_SetsVelocityAndOffset)
{
    Vector3 pos(0.0f, 0.0f, 0.0f);
    Vector3 vel(0.1f, 0.2f, 0.3f);
    Vector3 offset(0.5f, 0.5f, 0.5f);

    auto packet = ParticlePacket::createEntityEffect(pos, vel, offset, 5, 0xFFFF0000);

    EXPECT_FLOAT_EQ(packet.velocityX(), 0.1f);
    EXPECT_FLOAT_EQ(packet.velocityY(), 0.2f);
    EXPECT_FLOAT_EQ(packet.velocityZ(), 0.3f);
    EXPECT_FLOAT_EQ(packet.offsetX(), 0.5f);
    EXPECT_FLOAT_EQ(packet.offsetY(), 0.5f);
    EXPECT_FLOAT_EQ(packet.offsetZ(), 0.5f);
}

TEST(ParticlePacketEntityEffectTest, CreateEntityEffect_SetsCount)
{
    Vector3 pos(0, 0, 0);
    Vector3 vel(0, 0, 0);
    Vector3 offset(0, 0, 0);

    auto packet = ParticlePacket::createEntityEffect(pos, vel, offset, 10, 0xFFFF0000);

    EXPECT_EQ(packet.count(), 10u);
}

TEST(ParticlePacketEntityEffectTest, CreateEntityEffect_HasOptionalData)
{
    Vector3 pos(0, 0, 0);
    Vector3 vel(0, 0, 0);
    Vector3 offset(0, 0, 0);

    auto packet = ParticlePacket::createEntityEffect(pos, vel, offset, 1, 0xFFFF0000);

    EXPECT_FALSE(packet.optionalData().empty());
    // i32 color = 4 bytes
    EXPECT_GE(packet.optionalData().size(), 4u);
}

// ==================== isEntityEffectParticle 测试 ====================

TEST(ParticlePacketEntityEffectTest, IsEntityEffectParticle_TrueWithOptionalData)
{
    Vector3 pos(0, 0, 0);
    Vector3 vel(0, 0, 0);
    Vector3 offset(0, 0, 0);

    auto packet = ParticlePacket::createEntityEffect(pos, vel, offset, 1, 0xFFFF0000);

    EXPECT_TRUE(packet.isEntityEffectParticle());
}

TEST(ParticlePacketEntityEffectTest, IsEntityEffectParticle_FalseWithoutOptionalData)
{
    ParticlePacket packet(ParticleTypeId::EntityEffect, Vector3(0, 0, 0), Vector3(0, 0, 0), Vector3(0, 0, 0), 1);

    EXPECT_FALSE(packet.isEntityEffectParticle());
}

TEST(ParticlePacketEntityEffectTest, IsEntityEffectParticle_FalseForNonEntityEffectType)
{
    ParticlePacket packet(ParticleTypeId::Flame, Vector3(0, 0, 0), Vector3(0, 0, 0), Vector3(0, 0, 0), 1);
    std::vector<u8> fakeData(4, 0x00);
    packet.setOptionalData(fakeData);

    EXPECT_FALSE(packet.isEntityEffectParticle());
}

// ==================== decodeEntityEffectColor 测试 ====================

TEST(ParticlePacketEntityEffectTest, DecodeEntityEffectColor_ReturnsCorrectColor)
{
    Vector3 pos(0, 0, 0);
    Vector3 vel(0, 0, 0);
    Vector3 offset(0, 0, 0);

    // MC 原版 BellBlockEntity 起始颜色 16700985（0x00FFED79）
    auto packet = ParticlePacket::createEntityEffect(pos, vel, offset, 1, 16700990u);

    auto color = packet.decodeEntityEffectColor();
    ASSERT_TRUE(color.has_value());
    EXPECT_EQ(color.value(), 16700990u);
}

TEST(ParticlePacketEntityEffectTest, DecodeEntityEffectColor_ReturnsNulloptForNonEntityEffect)
{
    ParticlePacket packet(ParticleTypeId::Flame, Vector3(0, 0, 0), Vector3(0, 0, 0), Vector3(0, 0, 0), 1);

    EXPECT_FALSE(packet.decodeEntityEffectColor().has_value());
}

TEST(ParticlePacketEntityEffectTest, DecodeEntityEffectColor_ReturnsNulloptForEmptyOptionalData)
{
    ParticlePacket packet(ParticleTypeId::EntityEffect, Vector3(0, 0, 0), Vector3(0, 0, 0), Vector3(0, 0, 0), 1);

    EXPECT_FALSE(packet.decodeEntityEffectColor().has_value());
}

// ==================== EntityEffect 序列化/反序列化测试 ====================

TEST(ParticlePacketEntityEffectTest, SerializeDeserialize_EntityEffectPacket)
{
    Vector3 pos(10.0f, 64.0f, -20.0f);
    Vector3 vel(0.1f, 0.2f, 0.3f);
    Vector3 offset(0.5f, 0.5f, 0.5f);

    auto original = ParticlePacket::createEntityEffect(pos, vel, offset, 5, 0xFF123456);

    auto result = original.serialize();
    ASSERT_TRUE(result.success()) << result.error().message();

    ParticlePacket deserialized;
    auto deserResult = deserialized.deserialize(result.value().data(), result.value().size());
    ASSERT_TRUE(deserResult.success()) << deserResult.error().message();

    EXPECT_EQ(deserialized.particleType(), ParticleTypeId::EntityEffect);
    EXPECT_DOUBLE_EQ(deserialized.x(), 10.0);
    EXPECT_DOUBLE_EQ(deserialized.y(), 64.0);
    EXPECT_DOUBLE_EQ(deserialized.z(), -20.0);
    EXPECT_EQ(deserialized.count(), 5u);

    EXPECT_TRUE(deserialized.isEntityEffectParticle());

    auto decodedColor = deserialized.decodeEntityEffectColor();
    ASSERT_TRUE(decodedColor.has_value());
    EXPECT_EQ(decodedColor.value(), 0xFF123456u);
}

// ==================== BellBlockEntity 颜色递增场景测试 ====================

TEST(ParticlePacketEntityEffectTest, BellBlockEntity_ColorIncrementSequence)
{
    // 模拟 BellBlockEntity._showBellParticles 的颜色递增序列：
    //   起始 colorCounter = 16700985
    //   每次发射粒子前 colorCounter += 5
    //   第一个粒子颜色 = 16700990, 第二个 = 16700995, ...
    Vector3 pos(0, 0, 0);
    Vector3 vel(0, 0, 0);
    Vector3 offset(0, 0, 0);

    i32 colorCounter = 16700985;
    for (i32 k = 0; k < 15; ++k) {
        colorCounter += 5;
        const u32 expectedColor = static_cast<u32>(colorCounter);

        auto packet = ParticlePacket::createEntityEffect(pos, vel, offset, 1, expectedColor);

        auto result = packet.serialize();
        ASSERT_TRUE(result.success()) << result.error().message();

        ParticlePacket deserialized;
        auto deserResult = deserialized.deserialize(result.value().data(), result.value().size());
        ASSERT_TRUE(deserResult.success()) << deserResult.error().message();

        auto decodedColor = deserialized.decodeEntityEffectColor();
        ASSERT_TRUE(decodedColor.has_value());
        EXPECT_EQ(decodedColor.value(), expectedColor);
    }
}
