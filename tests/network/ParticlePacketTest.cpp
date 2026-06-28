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

#include "network/packet/ParticlePacket.hpp"
#include "client/renderer/trident/particle/ParticleTypes.hpp"
#include "common/util/math/Vector3.hpp"
#include <cmath>
#include <gtest/gtest.h>

using namespace mc::network;
using namespace mc::client::renderer::trident::particle;
using mc::f32;
using mc::f64;
using mc::u16;
using mc::u32;
using mc::u8;
using mc::Vector3;

// ==================== ParticlePacket 基础测试 ====================

class ParticlePacketTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        testType = ParticleTypeId::Flame;
        testPos = Vector3(100.5f, 64.0f, -200.25f);
        testVelocity = Vector3(0.1f, 0.2f, 0.3f);
        testOffset = Vector3(0.5f, 0.5f, 0.5f);
        testCount = 10;
    }

    ParticleTypeId testType;
    Vector3 testPos;
    Vector3 testVelocity;
    Vector3 testOffset;
    u32 testCount;
};

TEST_F(ParticlePacketTest, DefaultConstruction)
{
    ParticlePacket packet;
    EXPECT_EQ(packet.particleType(), ParticleTypeId::Invalid);
    EXPECT_EQ(packet.x(), 0.0);
    EXPECT_EQ(packet.y(), 0.0);
    EXPECT_EQ(packet.z(), 0.0);
    EXPECT_EQ(packet.count(), 1u);
    EXPECT_TRUE(packet.optionalData().empty());
}

TEST_F(ParticlePacketTest, ParameterizedConstruction)
{
    ParticlePacket packet(testType, testPos, testVelocity, testOffset, testCount);

    EXPECT_EQ(packet.particleType(), testType);
    EXPECT_DOUBLE_EQ(packet.x(), static_cast<f64>(testPos.x));
    EXPECT_DOUBLE_EQ(packet.y(), static_cast<f64>(testPos.y));
    EXPECT_DOUBLE_EQ(packet.z(), static_cast<f64>(testPos.z));
    EXPECT_FLOAT_EQ(packet.velocityX(), testVelocity.x);
    EXPECT_FLOAT_EQ(packet.velocityY(), testVelocity.y);
    EXPECT_FLOAT_EQ(packet.velocityZ(), testVelocity.z);
    EXPECT_FLOAT_EQ(packet.offsetX(), testOffset.x);
    EXPECT_FLOAT_EQ(packet.offsetY(), testOffset.y);
    EXPECT_FLOAT_EQ(packet.offsetZ(), testOffset.z);
    EXPECT_EQ(packet.count(), testCount);
}

TEST_F(ParticlePacketTest, SettersAndGetters)
{
    ParticlePacket packet;

    packet.setParticleType(ParticleTypeId::Smoke);
    EXPECT_EQ(packet.particleType(), ParticleTypeId::Smoke);

    packet.setPosition(50.0f, 70.0f, 100.0f);
    EXPECT_DOUBLE_EQ(packet.x(), 50.0);
    EXPECT_DOUBLE_EQ(packet.y(), 70.0);
    EXPECT_DOUBLE_EQ(packet.z(), 100.0);

    Vector3 newPos(25.5f, 30.0f, 45.75f);
    packet.setPosition(newPos);
    EXPECT_DOUBLE_EQ(packet.x(), static_cast<f64>(newPos.x));
    EXPECT_DOUBLE_EQ(packet.y(), static_cast<f64>(newPos.y));
    EXPECT_DOUBLE_EQ(packet.z(), static_cast<f64>(newPos.z));

    packet.setVelocity(1.0f, 2.0f, 3.0f);
    EXPECT_FLOAT_EQ(packet.velocityX(), 1.0f);
    EXPECT_FLOAT_EQ(packet.velocityY(), 2.0f);
    EXPECT_FLOAT_EQ(packet.velocityZ(), 3.0f);

    packet.setOffset(0.1f, 0.2f, 0.3f);
    EXPECT_FLOAT_EQ(packet.offsetX(), 0.1f);
    EXPECT_FLOAT_EQ(packet.offsetY(), 0.2f);
    EXPECT_FLOAT_EQ(packet.offsetZ(), 0.3f);

    packet.setCount(25);
    EXPECT_EQ(packet.count(), 25u);

    std::vector<u8> data = {0x01, 0x02, 0x03};
    packet.setOptionalData(data);
    EXPECT_EQ(packet.optionalData().size(), 3u);
    EXPECT_EQ(packet.optionalData()[0], 0x01);
}

TEST_F(ParticlePacketTest, VectorGetters)
{
    ParticlePacket packet(testType, testPos, testVelocity, testOffset, testCount);

    Vector3 pos = packet.position();
    EXPECT_FLOAT_EQ(pos.x, testPos.x);
    EXPECT_FLOAT_EQ(pos.y, testPos.y);
    EXPECT_FLOAT_EQ(pos.z, testPos.z);

    Vector3 vel = packet.velocity();
    EXPECT_FLOAT_EQ(vel.x, testVelocity.x);
    EXPECT_FLOAT_EQ(vel.y, testVelocity.y);
    EXPECT_FLOAT_EQ(vel.z, testVelocity.z);

    Vector3 off = packet.offset();
    EXPECT_FLOAT_EQ(off.x, testOffset.x);
    EXPECT_FLOAT_EQ(off.y, testOffset.y);
    EXPECT_FLOAT_EQ(off.z, testOffset.z);
}

// ==================== ParticlePacket 序列化测试 ====================

class ParticlePacketSerializeTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        testType = ParticleTypeId::Crit;
        testPos = Vector3(123.45f, 67.89f, -234.56f);
        testVelocity = Vector3(0.5f, -0.3f, 0.1f);
        testOffset = Vector3(1.0f, 2.0f, 3.0f);
        testCount = 5;
    }

    ParticleTypeId testType;
    Vector3 testPos;
    Vector3 testVelocity;
    Vector3 testOffset;
    u32 testCount;
};

TEST_F(ParticlePacketSerializeTest, SerializeDeserializeBasic)
{
    ParticlePacket original(testType, testPos, testVelocity, testOffset, testCount);

    auto result = original.serialize();
    ASSERT_TRUE(result.success()) << result.error().message();

    const auto& data = result.value();
    EXPECT_GT(data.size(), 0u);

    ParticlePacket deserialized;
    auto deserResult = deserialized.deserialize(data.data(), data.size());
    EXPECT_TRUE(deserResult.success()) << deserResult.error().message();

    EXPECT_EQ(deserialized.particleType(), testType);
    EXPECT_DOUBLE_EQ(deserialized.x(), static_cast<f64>(testPos.x));
    EXPECT_DOUBLE_EQ(deserialized.y(), static_cast<f64>(testPos.y));
    EXPECT_DOUBLE_EQ(deserialized.z(), static_cast<f64>(testPos.z));
    EXPECT_FLOAT_EQ(deserialized.velocityX(), testVelocity.x);
    EXPECT_FLOAT_EQ(deserialized.velocityY(), testVelocity.y);
    EXPECT_FLOAT_EQ(deserialized.velocityZ(), testVelocity.z);
    EXPECT_FLOAT_EQ(deserialized.offsetX(), testOffset.x);
    EXPECT_FLOAT_EQ(deserialized.offsetY(), testOffset.y);
    EXPECT_FLOAT_EQ(deserialized.offsetZ(), testOffset.z);
    EXPECT_EQ(deserialized.count(), testCount);
}

TEST_F(ParticlePacketSerializeTest, SerializeDeserializeWithOptionalData)
{
    ParticlePacket original(testType, testPos, testVelocity, testOffset, testCount);

    std::vector<u8> optionalData = {0x01, 0x02, 0x03, 0x04, 0x05};
    original.setOptionalData(optionalData);

    auto result = original.serialize();
    ASSERT_TRUE(result.success()) << result.error().message();

    ParticlePacket deserialized;
    auto deserResult = deserialized.deserialize(result.value().data(), result.value().size());
    EXPECT_TRUE(deserResult.success()) << deserResult.error().message();

    EXPECT_EQ(deserialized.optionalData().size(), optionalData.size());
    EXPECT_EQ(deserialized.optionalData(), optionalData);
}

TEST_F(ParticlePacketSerializeTest, SerializeDeserializeEmptyOptionalData)
{
    ParticlePacket original(testType, testPos, testVelocity, testOffset, testCount);

    auto result = original.serialize();
    ASSERT_TRUE(result.success()) << result.error().message();

    ParticlePacket deserialized;
    auto deserResult = deserialized.deserialize(result.value().data(), result.value().size());
    EXPECT_TRUE(deserResult.success()) << deserResult.error().message();

    EXPECT_TRUE(deserialized.optionalData().empty());
}

TEST_F(ParticlePacketSerializeTest, SerializeDeserializeSingleParticle)
{
    auto original =
        ParticlePacket::createSingle(ParticleTypeId::Heart, Vector3(10.0f, 20.0f, 30.0f), Vector3(0.0f, 0.0f, 0.0f));

    auto result = original.serialize();
    ASSERT_TRUE(result.success()) << result.error().message();

    ParticlePacket deserialized;
    auto deserResult = deserialized.deserialize(result.value().data(), result.value().size());
    EXPECT_TRUE(deserResult.success()) << deserResult.error().message();

    EXPECT_EQ(deserialized.particleType(), ParticleTypeId::Heart);
    EXPECT_EQ(deserialized.count(), 1u);
    EXPECT_FLOAT_EQ(deserialized.offsetX(), 0.0f);
    EXPECT_FLOAT_EQ(deserialized.offsetY(), 0.0f);
    EXPECT_FLOAT_EQ(deserialized.offsetZ(), 0.0f);
}

TEST_F(ParticlePacketSerializeTest, SerializeDeserializeLargeCount)
{
    ParticlePacket original(testType, testPos, testVelocity, testOffset, 1000);

    auto result = original.serialize();
    ASSERT_TRUE(result.success()) << result.error().message();

    ParticlePacket deserialized;
    auto deserResult = deserialized.deserialize(result.value().data(), result.value().size());
    EXPECT_TRUE(deserResult.success()) << deserResult.error().message();

    EXPECT_EQ(deserialized.count(), 1000u);
}

TEST_F(ParticlePacketSerializeTest, SerializeDeserializeZeroOffset)
{
    ParticlePacket original(testType, testPos, testVelocity, Vector3(0.0f, 0.0f, 0.0f), 1);

    auto result = original.serialize();
    ASSERT_TRUE(result.success()) << result.error().message();

    ParticlePacket deserialized;
    auto deserResult = deserialized.deserialize(result.value().data(), result.value().size());
    EXPECT_TRUE(deserResult.success()) << deserResult.error().message();

    EXPECT_FLOAT_EQ(deserialized.offsetX(), 0.0f);
    EXPECT_FLOAT_EQ(deserialized.offsetY(), 0.0f);
    EXPECT_FLOAT_EQ(deserialized.offsetZ(), 0.0f);
}

// ==================== ParticlePacket 错误处理测试 ====================

TEST(ParticlePacketErrorTest, DeserializeTooSmallData)
{
    ParticlePacket packet;
    u8 smallData[] = {0x01};

    auto result = packet.deserialize(smallData, sizeof(smallData));
    EXPECT_FALSE(result.success());
}

TEST(ParticlePacketErrorTest, DeserializeInvalidParticleType)
{
    ParticlePacket original(ParticleTypeId::Flame, Vector3(0, 0, 0), Vector3(0, 0, 0), Vector3(0, 0, 0), 1);
    auto result = original.serialize();
    ASSERT_TRUE(result.success());

    auto data = result.value();
    // 修改粒子类型为无效值 (255)
    data[0] = 0xFF;
    data[1] = 0x01;

    ParticlePacket deserialized;
    auto deserResult = deserialized.deserialize(data.data(), data.size());
    EXPECT_FALSE(deserResult.success());
}

TEST(ParticlePacketErrorTest, DeserializeTruncatedOptionalData)
{
    ParticlePacket original(ParticleTypeId::Flame, Vector3(0, 0, 0), Vector3(0, 0, 0), Vector3(0, 0, 0), 1);
    std::vector<u8> optData = {0x01, 0x02, 0x03, 0x04, 0x05};
    original.setOptionalData(optData);

    auto result = original.serialize();
    ASSERT_TRUE(result.success());

    auto data = result.value();
    data.resize(data.size() - 3);

    ParticlePacket deserialized;
    auto deserResult = deserialized.deserialize(data.data(), data.size());
    EXPECT_FALSE(deserResult.success());
}

// ==================== ParticlePacket 工厂方法测试 ====================

TEST(ParticlePacketFactoryTest, CreateMethod)
{
    auto packet = ParticlePacket::create(ParticleTypeId::LargeExplosion,
        Vector3(100.0f, 50.0f, 75.0f),
        Vector3(0.0f, 0.0f, 0.0f),
        Vector3(2.0f, 2.0f, 2.0f),
        50);

    EXPECT_EQ(packet.particleType(), ParticleTypeId::LargeExplosion);
    EXPECT_DOUBLE_EQ(packet.x(), 100.0);
    EXPECT_DOUBLE_EQ(packet.y(), 50.0);
    EXPECT_DOUBLE_EQ(packet.z(), 75.0);
    EXPECT_FLOAT_EQ(packet.offsetX(), 2.0f);
    EXPECT_FLOAT_EQ(packet.offsetY(), 2.0f);
    EXPECT_FLOAT_EQ(packet.offsetZ(), 2.0f);
    EXPECT_EQ(packet.count(), 50u);
}

TEST(ParticlePacketFactoryTest, CreateSingleMethod)
{
    auto packet =
        ParticlePacket::createSingle(ParticleTypeId::Bubble, Vector3(10.0f, 20.0f, 30.0f), Vector3(0.0f, 0.1f, 0.0f));

    EXPECT_EQ(packet.particleType(), ParticleTypeId::Bubble);
    EXPECT_DOUBLE_EQ(packet.x(), 10.0);
    EXPECT_DOUBLE_EQ(packet.y(), 20.0);
    EXPECT_DOUBLE_EQ(packet.z(), 30.0);
    EXPECT_FLOAT_EQ(packet.velocityY(), 0.1f);
    EXPECT_EQ(packet.count(), 1u);
    EXPECT_FLOAT_EQ(packet.offsetX(), 0.0f);
    EXPECT_FLOAT_EQ(packet.offsetY(), 0.0f);
    EXPECT_FLOAT_EQ(packet.offsetZ(), 0.0f);
}

// ==================== 所有粒子类型测试 ====================

TEST(ParticlePacketAllTypesTest, AllProtocolParticleTypesSerializeDeserialize)
{
    // 测试所有 MC 协议粒子类型（0~114）的序列化/反序列化
    // 协议粒子的枚举值与协议 ID 一致，序列化/反序列化应完全保持一致
    for (u16 i = 0; i < mc::particle::PROTOCOL_PARTICLE_TYPE_COUNT; ++i) {
        ParticleTypeId type = static_cast<ParticleTypeId>(i);

        ParticlePacket original(type, Vector3(0, 0, 0), Vector3(0, 0, 0), Vector3(0, 0, 0), 1);
        auto result = original.serialize();

        ASSERT_TRUE(result.success()) << "Failed to serialize particle type " << i;

        ParticlePacket deserialized;
        auto deserResult = deserialized.deserialize(result.value().data(), result.value().size());
        ASSERT_TRUE(deserResult.success()) << "Failed to deserialize particle type " << i;
        EXPECT_EQ(deserialized.particleType(), type) << "Type mismatch for particle ID " << i;
    }
}

TEST(ParticlePacketAllTypesTest, InternalExtensionParticlesMapToProtocolId)
{
    // 内部扩展粒子（115~123）序列化时映射到协议粒子 ID，
    // 反序列化后得到的是映射后的协议粒子类型，而非原始内部类型
    struct Mapping {
        ParticleTypeId internal;
        ParticleTypeId mapped;
    };

    Mapping mappings[] = {
        {ParticleTypeId::Breaking, ParticleTypeId::Block},
        {ParticleTypeId::Barrier, ParticleTypeId::Block},
        {ParticleTypeId::Light, ParticleTypeId::Block},
        {ParticleTypeId::Redstone, ParticleTypeId::Dust},
        {ParticleTypeId::LargeExplosion, ParticleTypeId::HugeExplosion},
        {ParticleTypeId::ItemPickup, ParticleTypeId::Poof},
        {ParticleTypeId::DrippingCherryLeaves, ParticleTypeId::CherryLeaves},
        {ParticleTypeId::FallingCherryLeaves, ParticleTypeId::CherryLeaves},
        {ParticleTypeId::LandingCherryLeaves, ParticleTypeId::CherryLeaves},
    };

    for (const auto& m : mappings) {
        ParticlePacket original(m.internal, Vector3(10, 20, 30), Vector3(0, 0, 0), Vector3(1, 1, 1), 5);
        auto result = original.serialize();
        ASSERT_TRUE(result.success()) << "Failed to serialize internal type " << static_cast<int>(m.internal);

        ParticlePacket deserialized;
        auto deserResult = deserialized.deserialize(result.value().data(), result.value().size());
        ASSERT_TRUE(deserResult.success()) << "Failed to deserialize internal type " << static_cast<int>(m.internal);
        EXPECT_EQ(deserialized.particleType(), m.mapped)
            << "Internal type " << static_cast<int>(m.internal) << " should map to " << static_cast<int>(m.mapped);
    }
}

// ==================== 粒子同步集成测试 ====================

TEST(ParticleSyncIntegrationTest, ServerToClientTransmission)
{
    // 模拟服务器创建粒子包
    ParticlePacket serverPacket(ParticleTypeId::Flame,
        Vector3(100.0f, 64.0f, 200.0f),
        Vector3(0.0f, 0.02f, 0.0f),
        Vector3(0.5f, 0.5f, 0.5f),
        10);

    // 模拟服务器序列化
    auto serializeResult = serverPacket.serialize();
    ASSERT_TRUE(serializeResult.success()) << "Server serialization failed";

    const auto& networkData = serializeResult.value();
    EXPECT_GT(networkData.size(), 0u);

    // 模拟客户端反序列化
    ParticlePacket clientPacket;
    auto deserializeResult = clientPacket.deserialize(networkData.data(), networkData.size());
    ASSERT_TRUE(deserializeResult.success()) << "Client deserialization failed";

    // 验证数据完整性
    EXPECT_EQ(clientPacket.particleType(), serverPacket.particleType());
    EXPECT_DOUBLE_EQ(clientPacket.x(), serverPacket.x());
    EXPECT_DOUBLE_EQ(clientPacket.y(), serverPacket.y());
    EXPECT_DOUBLE_EQ(clientPacket.z(), serverPacket.z());
    EXPECT_FLOAT_EQ(clientPacket.velocityX(), serverPacket.velocityX());
    EXPECT_FLOAT_EQ(clientPacket.velocityY(), serverPacket.velocityY());
    EXPECT_FLOAT_EQ(clientPacket.velocityZ(), serverPacket.velocityZ());
    EXPECT_FLOAT_EQ(clientPacket.offsetX(), serverPacket.offsetX());
    EXPECT_FLOAT_EQ(clientPacket.offsetY(), serverPacket.offsetY());
    EXPECT_FLOAT_EQ(clientPacket.offsetZ(), serverPacket.offsetZ());
    EXPECT_EQ(clientPacket.count(), serverPacket.count());
}

TEST(ParticleSyncIntegrationTest, MultiplePacketsInSequence)
{
    std::vector<ParticleTypeId> types = {ParticleTypeId::Flame,
        ParticleTypeId::Smoke,
        ParticleTypeId::HugeExplosion,
        ParticleTypeId::Bubble,
        ParticleTypeId::Heart};

    for (auto type : types) {
        ParticlePacket packet(type, Vector3(0, 0, 0), Vector3(0, 0, 0), Vector3(1, 1, 1), 5);
        auto result = packet.serialize();
        ASSERT_TRUE(result.success()) << "Failed for type " << static_cast<int>(type);

        ParticlePacket received;
        auto deserResult = received.deserialize(result.value().data(), result.value().size());
        ASSERT_TRUE(deserResult.success()) << "Failed to deserialize type " << static_cast<int>(type);
        EXPECT_EQ(received.particleType(), type);
    }
}

TEST(ParticleSyncIntegrationTest, ParticleWithBlockData)
{
    ParticlePacket blockParticle(
        ParticleTypeId::Block, Vector3(10, 20, 30), Vector3(0, 0, 0), Vector3(0.1f, 0.1f, 0.1f), 20);

    // 设置方块状态数据
    std::vector<u8> blockData = {0x01, 0x02};
    blockParticle.setOptionalData(blockData);

    auto result = blockParticle.serialize();
    ASSERT_TRUE(result.success());

    ParticlePacket received;
    auto deserResult = received.deserialize(result.value().data(), result.value().size());
    ASSERT_TRUE(deserResult.success());

    EXPECT_EQ(received.particleType(), ParticleTypeId::Block);
    EXPECT_EQ(received.optionalData().size(), 2u);
    EXPECT_EQ(received.optionalData(), blockData);
}

TEST(ParticleSyncIntegrationTest, ParticleWithRedstoneData)
{
    ParticlePacket redstoneParticle(ParticleTypeId::Dust, Vector3(0, 0, 0), Vector3(0, 0, 0), Vector3(0, 0, 0), 1);

    // 设置红石颜色数据
    std::vector<u8> colorData;
    f32 r = 1.0f, g = 0.0f, b = 0.0f, scale = 1.0f;
    const u8* rBytes = reinterpret_cast<const u8*>(&r);
    const u8* gBytes = reinterpret_cast<const u8*>(&g);
    const u8* bBytes = reinterpret_cast<const u8*>(&b);
    const u8* sBytes = reinterpret_cast<const u8*>(&scale);
    colorData.insert(colorData.end(), rBytes, rBytes + sizeof(f32));
    colorData.insert(colorData.end(), gBytes, gBytes + sizeof(f32));
    colorData.insert(colorData.end(), bBytes, bBytes + sizeof(f32));
    colorData.insert(colorData.end(), sBytes, sBytes + sizeof(f32));

    redstoneParticle.setOptionalData(colorData);

    auto result = redstoneParticle.serialize();
    ASSERT_TRUE(result.success());

    ParticlePacket received;
    auto deserResult = received.deserialize(result.value().data(), result.value().size());
    ASSERT_TRUE(deserResult.success());

    EXPECT_EQ(received.particleType(), ParticleTypeId::Dust);
    EXPECT_EQ(received.optionalData().size(), sizeof(f32) * 4);
}

// ==================== 性能测试 ====================

TEST(ParticlePacketPerfTest, SerializeDeserializePerformance)
{
    ParticlePacket packet(ParticleTypeId::Flame, Vector3(0, 0, 0), Vector3(0, 0, 0), Vector3(1, 1, 1), 10);

    for (int i = 0; i < 1000; ++i) {
        auto result = packet.serialize();
        ASSERT_TRUE(result.success());
    }

    auto result = packet.serialize();
    for (int i = 0; i < 1000; ++i) {
        ParticlePacket received;
        auto deserResult = received.deserialize(result.value().data(), result.value().size());
        ASSERT_TRUE(deserResult.success());
    }
}

TEST(ParticlePacketPerfTest, LargeDataHandling)
{
    ParticlePacket packet(ParticleTypeId::Flame, Vector3(0, 0, 0), Vector3(0, 0, 0), Vector3(1, 1, 1), 100);

    std::vector<u8> largeData(1000, 0xAB);
    packet.setOptionalData(largeData);

    auto result = packet.serialize();
    ASSERT_TRUE(result.success());

    ParticlePacket received;
    auto deserResult = received.deserialize(result.value().data(), result.value().size());
    ASSERT_TRUE(deserResult.success());
    EXPECT_EQ(received.optionalData().size(), 1000u);
}
