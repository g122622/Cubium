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

// ============================================================================
// ParticlePacket createBlock / isBlockParticle / decodeBlockStateId 测试
// ============================================================================
//
// 验证方块粒子（携带方块状态 ID）的工厂方法、判断与解码逻辑。
// 对应 BreezeEntity::emitGroundParticles / emitJumpTrailParticles 在服务端
// 通过 ParticlePacket::createBlock 编码 blockStateId，客户端通过
// decodeBlockStateId 解码后调用 BlockRegistry::getBlockState 还原。

// ==================== createBlock 工厂方法测试 ====================

TEST(ParticlePacketBlockTest, CreateBlock_SetsCorrectType)
{
    Vector3 pos(10.0f, 64.0f, -20.0f);
    Vector3 vel(0.0f, 0.0f, 0.0f);
    Vector3 offset(0.0f, 0.0f, 0.0f);

    auto packet = ParticlePacket::createBlock(ParticleTypeId::Block, pos, vel, offset, 3, 1u);

    EXPECT_EQ(packet.particleType(), ParticleTypeId::Block);
}

TEST(ParticlePacketBlockTest, CreateBlock_SetsPosition)
{
    Vector3 pos(10.5f, 64.0f, -20.25f);
    Vector3 vel(0.0f, 0.0f, 0.0f);
    Vector3 offset(0.0f, 0.0f, 0.0f);

    auto packet = ParticlePacket::createBlock(ParticleTypeId::Block, pos, vel, offset, 3, 1u);

    EXPECT_DOUBLE_EQ(packet.x(), 10.5);
    EXPECT_DOUBLE_EQ(packet.y(), 64.0);
    EXPECT_DOUBLE_EQ(packet.z(), -20.25);
}

TEST(ParticlePacketBlockTest, CreateBlock_SetsVelocityAndOffset)
{
    Vector3 pos(0.0f, 0.0f, 0.0f);
    Vector3 vel(0.1f, 0.2f, 0.3f);
    Vector3 offset(0.5f, 0.5f, 0.5f);

    auto packet = ParticlePacket::createBlock(ParticleTypeId::Block, pos, vel, offset, 3, 1u);

    EXPECT_FLOAT_EQ(packet.velocityX(), 0.1f);
    EXPECT_FLOAT_EQ(packet.velocityY(), 0.2f);
    EXPECT_FLOAT_EQ(packet.velocityZ(), 0.3f);
    EXPECT_FLOAT_EQ(packet.offsetX(), 0.5f);
    EXPECT_FLOAT_EQ(packet.offsetY(), 0.5f);
    EXPECT_FLOAT_EQ(packet.offsetZ(), 0.5f);
}

TEST(ParticlePacketBlockTest, CreateBlock_SetsCount)
{
    auto packet = ParticlePacket::createBlock(
        ParticleTypeId::Block, Vector3(0, 0, 0), Vector3(0, 0, 0), Vector3(0, 0, 0), 20, 1u);

    EXPECT_EQ(packet.count(), 20u);
}

TEST(ParticlePacketBlockTest, CreateBlock_EncodesBlockStateIdInOptionalData)
{
    // blockStateId = 1（石头默认状态）
    auto packet =
        ParticlePacket::createBlock(ParticleTypeId::Block, Vector3(0, 0, 0), Vector3(0, 0, 0), Vector3(0, 0, 0), 1, 1u);

    // VarInt 编码 1 = 0x01（1 字节）
    ASSERT_FALSE(packet.optionalData().empty());
    EXPECT_EQ(packet.optionalData().size(), 1u);
    EXPECT_EQ(packet.optionalData()[0], 0x01);
}

TEST(ParticlePacketBlockTest, CreateBlock_LargeBlockStateIdEncodesAsMultiByteVarInt)
{
    // blockStateId = 300，VarInt 编码为多字节
    // 300 = 0b100101100
    // VarInt: 低 7 位 0101100 (0x2C) | 高位续 1 → 0xAC
    //         剩余 0000010 (0x02) | 高位停 0 → 0x02
    // 总计 2 字节: 0xAC 0x02
    auto packet = ParticlePacket::createBlock(
        ParticleTypeId::Block, Vector3(0, 0, 0), Vector3(0, 0, 0), Vector3(0, 0, 0), 1, 300u);

    ASSERT_EQ(packet.optionalData().size(), 2u);
    EXPECT_EQ(packet.optionalData()[0], 0xAC);
    EXPECT_EQ(packet.optionalData()[1], 0x02);
}

// ==================== isBlockParticle 判断测试 ====================

TEST(ParticlePacketBlockTest, IsBlockParticle_BlockTypeWithOptionalData_ReturnsTrue)
{
    auto packet =
        ParticlePacket::createBlock(ParticleTypeId::Block, Vector3(0, 0, 0), Vector3(0, 0, 0), Vector3(0, 0, 0), 1, 1u);

    EXPECT_TRUE(packet.isBlockParticle());
}

TEST(ParticlePacketBlockTest, IsBlockParticle_BlockMarkerTypeWithOptionalData_ReturnsTrue)
{
    auto packet = ParticlePacket::createBlock(
        ParticleTypeId::BlockMarker, Vector3(0, 0, 0), Vector3(0, 0, 0), Vector3(0, 0, 0), 1, 1u);

    EXPECT_TRUE(packet.isBlockParticle());
}

TEST(ParticlePacketBlockTest, IsBlockParticle_FallingDustTypeWithOptionalData_ReturnsTrue)
{
    auto packet = ParticlePacket::createBlock(
        ParticleTypeId::FallingDust, Vector3(0, 0, 0), Vector3(0, 0, 0), Vector3(0, 0, 0), 1, 1u);

    EXPECT_TRUE(packet.isBlockParticle());
}

TEST(ParticlePacketBlockTest, IsBlockParticle_DustPillarTypeWithOptionalData_ReturnsTrue)
{
    auto packet = ParticlePacket::createBlock(
        ParticleTypeId::DustPillar, Vector3(0, 0, 0), Vector3(0, 0, 0), Vector3(0, 0, 0), 1, 1u);

    EXPECT_TRUE(packet.isBlockParticle());
}

TEST(ParticlePacketBlockTest, IsBlockParticle_BlockCrumbleTypeWithOptionalData_ReturnsTrue)
{
    auto packet = ParticlePacket::createBlock(
        ParticleTypeId::BlockCrumble, Vector3(0, 0, 0), Vector3(0, 0, 0), Vector3(0, 0, 0), 1, 1u);

    EXPECT_TRUE(packet.isBlockParticle());
}

TEST(ParticlePacketBlockTest, IsBlockParticle_BlockTypeWithoutOptionalData_ReturnsFalse)
{
    // Block 类型但未设置 optionalData
    ParticlePacket packet(ParticleTypeId::Block, Vector3(0, 0, 0), Vector3(0, 0, 0), Vector3(0, 0, 0), 1);
    EXPECT_FALSE(packet.isBlockParticle());
}

TEST(ParticlePacketBlockTest, IsBlockParticle_NonBlockTypeWithOptionalData_ReturnsFalse)
{
    // Flame 类型不属于 requiresBlockState，即使有 optionalData 也不是 Block 粒子
    ParticlePacket packet(ParticleTypeId::Flame, Vector3(0, 0, 0), Vector3(0, 0, 0), Vector3(0, 0, 0), 1);
    std::vector<u8> fakeData = {0x01};
    packet.setOptionalData(fakeData);

    EXPECT_FALSE(packet.isBlockParticle());
}

// ==================== decodeBlockStateId 解码测试 ====================

TEST(ParticlePacketBlockTest, DecodeBlockStateId_SmallId_ReturnsCorrectValue)
{
    auto packet =
        ParticlePacket::createBlock(ParticleTypeId::Block, Vector3(0, 0, 0), Vector3(0, 0, 0), Vector3(0, 0, 0), 1, 1u);

    auto stateId = packet.decodeBlockStateId();
    ASSERT_TRUE(stateId.has_value());
    EXPECT_EQ(stateId.value(), 1u);
}

TEST(ParticlePacketBlockTest, DecodeBlockStateId_LargeId_ReturnsCorrectValue)
{
    auto packet = ParticlePacket::createBlock(
        ParticleTypeId::Block, Vector3(0, 0, 0), Vector3(0, 0, 0), Vector3(0, 0, 0), 1, 300u);

    auto stateId = packet.decodeBlockStateId();
    ASSERT_TRUE(stateId.has_value());
    EXPECT_EQ(stateId.value(), 300u);
}

TEST(ParticlePacketBlockTest, DecodeBlockStateId_ZeroId_ReturnsZero)
{
    auto packet =
        ParticlePacket::createBlock(ParticleTypeId::Block, Vector3(0, 0, 0), Vector3(0, 0, 0), Vector3(0, 0, 0), 1, 0u);

    auto stateId = packet.decodeBlockStateId();
    ASSERT_TRUE(stateId.has_value());
    EXPECT_EQ(stateId.value(), 0u);
}

TEST(ParticlePacketBlockTest, DecodeBlockStateId_MaxU32_ReturnsCorrectValue)
{
    // VarInt 编码 u32 最大值需要 5 字节
    const u32 maxStateId = 0xFFFFFFFFu;
    auto packet = ParticlePacket::createBlock(
        ParticleTypeId::Block, Vector3(0, 0, 0), Vector3(0, 0, 0), Vector3(0, 0, 0), 1, maxStateId);

    auto stateId = packet.decodeBlockStateId();
    ASSERT_TRUE(stateId.has_value());
    EXPECT_EQ(stateId.value(), maxStateId);
}

TEST(ParticlePacketBlockTest, DecodeBlockStateId_NotBlockParticle_ReturnsNullopt)
{
    // 非 Block 粒子类型，decodeBlockStateId 应返回 nullopt
    ParticlePacket packet(ParticleTypeId::Flame, Vector3(0, 0, 0), Vector3(0, 0, 0), Vector3(0, 0, 0), 1);
    std::vector<u8> fakeData = {0x01};
    packet.setOptionalData(fakeData);

    auto stateId = packet.decodeBlockStateId();
    EXPECT_FALSE(stateId.has_value());
}

TEST(ParticlePacketBlockTest, DecodeBlockStateId_BlockTypeWithoutOptionalData_ReturnsNullopt)
{
    // Block 类型但无 optionalData，isBlockParticle 为 false，返回 nullopt
    ParticlePacket packet(ParticleTypeId::Block, Vector3(0, 0, 0), Vector3(0, 0, 0), Vector3(0, 0, 0), 1);

    auto stateId = packet.decodeBlockStateId();
    EXPECT_FALSE(stateId.has_value());
}

// ==================== 序列化往返测试 ====================

TEST(ParticlePacketBlockTest, SerializeDeserialize_PreservesBlockStateId)
{
    const u32 testStateId = 12345u;
    auto original = ParticlePacket::createBlock(ParticleTypeId::Block,
        Vector3(10.0f, 64.0f, -20.0f),
        Vector3(0.1f, 0.2f, 0.3f),
        Vector3(0.5f, 0.5f, 0.5f),
        5,
        testStateId);

    auto serializeResult = original.serialize();
    ASSERT_TRUE(serializeResult.success()) << serializeResult.error().message();

    ParticlePacket deserialized;
    auto deserResult = deserialized.deserialize(serializeResult.value().data(), serializeResult.value().size());
    ASSERT_TRUE(deserResult.success()) << deserResult.error().message();

    EXPECT_EQ(deserialized.particleType(), ParticleTypeId::Block);
    EXPECT_TRUE(deserialized.isBlockParticle());

    auto decodedStateId = deserialized.decodeBlockStateId();
    ASSERT_TRUE(decodedStateId.has_value());
    EXPECT_EQ(decodedStateId.value(), testStateId);
}

TEST(ParticlePacketBlockTest, SerializeDeserialize_AllBlockParticleTypes)
{
    // 测试所有 requiresBlockState 返回 true 的粒子类型
    const ParticleTypeId blockTypes[] = {
        ParticleTypeId::Block,
        ParticleTypeId::BlockMarker,
        ParticleTypeId::FallingDust,
        ParticleTypeId::DustPillar,
        ParticleTypeId::BlockCrumble,
    };

    for (auto type : blockTypes) {
        auto original = ParticlePacket::createBlock(
            type, Vector3(10.0f, 64.0f, -20.0f), Vector3(0, 0, 0), Vector3(0, 0, 0), 3, 42u);

        auto serializeResult = original.serialize();
        ASSERT_TRUE(serializeResult.success()) << "Failed to serialize type " << static_cast<int>(type);

        ParticlePacket deserialized;
        auto deserResult = deserialized.deserialize(serializeResult.value().data(), serializeResult.value().size());
        ASSERT_TRUE(deserResult.success()) << "Failed to deserialize type " << static_cast<int>(type);

        EXPECT_TRUE(deserialized.isBlockParticle()) << "isBlockParticle failed for type " << static_cast<int>(type);

        auto stateId = deserialized.decodeBlockStateId();
        ASSERT_TRUE(stateId.has_value()) << "decodeBlockStateId failed for type " << static_cast<int>(type);
        EXPECT_EQ(stateId.value(), 42u) << "Wrong stateId for type " << static_cast<int>(type);
    }
}

// ==================== BreezeEntity 使用场景模拟测试 ====================

TEST(ParticlePacketBlockTest, BreezeEmitGroundParticlesScenario_SingleBlockState)
{
    // 模拟旋风人 emitGroundParticles 场景：
    // 发射 1 + nextInt(1) 个 Block 粒子，携带同一种方块状态
    // 在服务端通过 broadcastBlockParticleInRange 广播，客户端解码还原
    const u32 stoneStateId = 1u;  // 石头默认状态 ID
    const u32 particleCount = 2u; // 1 + nextInt(1) 最大值

    auto packet = ParticlePacket::createBlock(ParticleTypeId::Block,
        Vector3(0.5f, 64.0f, 0.5f),
        Vector3(0.0f, 0.0f, 0.0f),
        Vector3(0.0f, 0.0f, 0.0f),
        particleCount,
        stoneStateId);

    EXPECT_TRUE(packet.isBlockParticle());

    auto decodedStateId = packet.decodeBlockStateId();
    ASSERT_TRUE(decodedStateId.has_value());
    EXPECT_EQ(decodedStateId.value(), stoneStateId);
    EXPECT_EQ(packet.count(), particleCount);
}

TEST(ParticlePacketBlockTest, BreezeEmitJumpTrailParticlesScenario_ThreeParticles)
{
    // 模拟旋风人 emitJumpTrailParticles 场景：
    // 前 5 tick 每 tick 发射 3 个 Block 粒子
    const u32 blockStateId = 10u; // 假设的方块状态 ID
    const u32 particleCount = 3u;

    for (i32 tick = 1; tick <= 5; ++tick) {
        Vector3 pos(0.5f, 64.0f + tick * 0.1f, 0.5f); // 模拟实体上升
        auto packet = ParticlePacket::createBlock(ParticleTypeId::Block,
            pos,
            Vector3(0.0f, 0.0f, 0.0f),
            Vector3(0.0f, 0.0f, 0.0f),
            particleCount,
            blockStateId);

        EXPECT_TRUE(packet.isBlockParticle());
        EXPECT_EQ(packet.count(), particleCount);

        auto decodedStateId = packet.decodeBlockStateId();
        ASSERT_TRUE(decodedStateId.has_value());
        EXPECT_EQ(decodedStateId.value(), blockStateId);
    }
}

TEST(ParticlePacketBlockTest, BreezeSlideParticlesScenario_TwentyParticles)
{
    // 模拟旋风人 Sliding Pose 场景：每 tick 发射 20 个 Block 粒子
    const u32 blockStateId = 5u;
    const u32 particleCount = 20u; // SLIDE_PARTICLES_AMOUNT

    auto packet = ParticlePacket::createBlock(ParticleTypeId::Block,
        Vector3(0.0f, 64.0f, 0.0f),
        Vector3(0.0f, 0.0f, 0.0f),
        Vector3(0.0f, 0.0f, 0.0f),
        particleCount,
        blockStateId);

    EXPECT_EQ(packet.count(), particleCount);
    EXPECT_TRUE(packet.isBlockParticle());

    auto decodedStateId = packet.decodeBlockStateId();
    ASSERT_TRUE(decodedStateId.has_value());
    EXPECT_EQ(decodedStateId.value(), blockStateId);
}

// ==================== 边界场景测试 ====================

TEST(ParticlePacketBlockTest, Deserialize_TruncatedBlockStateData_ReturnsError)
{
    // 构造一个 Block 粒子包，然后截断 optionalData 部分
    auto original = ParticlePacket::createBlock(
        ParticleTypeId::Block, Vector3(0, 0, 0), Vector3(0, 0, 0), Vector3(0, 0, 0), 1, 300u);

    auto serializeResult = original.serialize();
    ASSERT_TRUE(serializeResult.success());

    auto data = serializeResult.value();
    // 截断最后 1 字节（300 的 VarInt 是 2 字节，截断后只剩 1 字节续位）
    data.resize(data.size() - 1);

    ParticlePacket deserialized;
    auto result = deserialized.deserialize(data.data(), data.size());
    // 反序列化可能成功（因为 optionalData 长度由剩余字节决定），
    // 但 decodeBlockStateId 应返回 nullopt 或错误的值
    if (result.success()) {
        // 即使反序列化成功，VarInt 解码也应失败
        auto stateId = deserialized.decodeBlockStateId();
        EXPECT_FALSE(stateId.has_value());
    }
}

TEST(ParticlePacketBlockTest, CreateBlock_ThenModifyOptionalData_BreaksBlockParticleFlag)
{
    // 验证 isBlockParticle 依赖于 optionalData 非空
    auto packet =
        ParticlePacket::createBlock(ParticleTypeId::Block, Vector3(0, 0, 0), Vector3(0, 0, 0), Vector3(0, 0, 0), 1, 1u);

    EXPECT_TRUE(packet.isBlockParticle());

    // 清空 optionalData
    packet.setOptionalData({});
    EXPECT_FALSE(packet.isBlockParticle());
}
