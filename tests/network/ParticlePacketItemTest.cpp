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
#include "common/item/core/ItemStack.hpp"
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
// ParticlePacket createItem / isItemParticle / decodeItemStack 测试
// ============================================================================
//
// 验证物品粒子（携带 ItemStack）的工厂方法、判断与解码逻辑。
// 对应物品破碎、史莱姆弹跳、雪球击中等场景，服务端通过
// ParticlePacket::createItem 编码 ItemStack，客户端通过 decodeItemStack 还原。

// ==================== createItem 工厂方法测试 ====================

TEST(ParticlePacketItemTest, CreateItem_SetsCorrectType)
{
    Vector3 pos(10.0f, 64.0f, -20.0f);
    Vector3 vel(0.0f, 0.0f, 0.0f);
    Vector3 offset(0.0f, 0.0f, 0.0f);
    mc::ItemStack itemStack;

    auto packet = ParticlePacket::createItem(ParticleTypeId::Item, pos, vel, offset, 3, itemStack);

    EXPECT_EQ(packet.particleType(), ParticleTypeId::Item);
}

TEST(ParticlePacketItemTest, CreateItem_SetsPosition)
{
    Vector3 pos(10.5f, 64.0f, -20.25f);
    Vector3 vel(0.0f, 0.0f, 0.0f);
    Vector3 offset(0.0f, 0.0f, 0.0f);
    mc::ItemStack itemStack;

    auto packet = ParticlePacket::createItem(ParticleTypeId::Item, pos, vel, offset, 3, itemStack);

    EXPECT_DOUBLE_EQ(packet.x(), 10.5);
    EXPECT_DOUBLE_EQ(packet.y(), 64.0);
    EXPECT_DOUBLE_EQ(packet.z(), -20.25);
}

TEST(ParticlePacketItemTest, CreateItem_SetsVelocityAndOffset)
{
    Vector3 pos(0.0f, 0.0f, 0.0f);
    Vector3 vel(0.1f, 0.2f, 0.3f);
    Vector3 offset(0.5f, 0.5f, 0.5f);
    mc::ItemStack itemStack;

    auto packet = ParticlePacket::createItem(ParticleTypeId::Item, pos, vel, offset, 3, itemStack);

    EXPECT_FLOAT_EQ(packet.velocityX(), 0.1f);
    EXPECT_FLOAT_EQ(packet.velocityY(), 0.2f);
    EXPECT_FLOAT_EQ(packet.velocityZ(), 0.3f);
    EXPECT_FLOAT_EQ(packet.offsetX(), 0.5f);
    EXPECT_FLOAT_EQ(packet.offsetY(), 0.5f);
    EXPECT_FLOAT_EQ(packet.offsetZ(), 0.5f);
}

TEST(ParticlePacketItemTest, CreateItem_SetsCount)
{
    mc::ItemStack itemStack;
    auto packet = ParticlePacket::createItem(
        ParticleTypeId::Item, Vector3(0, 0, 0), Vector3(0, 0, 0), Vector3(0, 0, 0), 20, itemStack);

    EXPECT_EQ(packet.count(), 20u);
}

TEST(ParticlePacketItemTest, CreateItem_EncodesItemStackInOptionalData)
{
    // 空 ItemStack 序列化为单字节 0x00（writeBool(false)）
    mc::ItemStack itemStack;
    auto packet = ParticlePacket::createItem(
        ParticleTypeId::Item, Vector3(0, 0, 0), Vector3(0, 0, 0), Vector3(0, 0, 0), 1, itemStack);

    ASSERT_FALSE(packet.optionalData().empty());
    // 空 ItemStack 序列化后应为 1 字节 (0x00)
    EXPECT_EQ(packet.optionalData().size(), 1u);
    EXPECT_EQ(packet.optionalData()[0], 0x00);
}

// ==================== isItemParticle 判断测试 ====================

TEST(ParticlePacketItemTest, IsItemParticle_ItemTypeWithOptionalData_ReturnsTrue)
{
    mc::ItemStack itemStack;
    auto packet = ParticlePacket::createItem(
        ParticleTypeId::Item, Vector3(0, 0, 0), Vector3(0, 0, 0), Vector3(0, 0, 0), 1, itemStack);

    EXPECT_TRUE(packet.isItemParticle());
}

TEST(ParticlePacketItemTest, IsItemParticle_ItemSlimeTypeWithOptionalData_ReturnsTrue)
{
    mc::ItemStack itemStack;
    auto packet = ParticlePacket::createItem(
        ParticleTypeId::ItemSlime, Vector3(0, 0, 0), Vector3(0, 0, 0), Vector3(0, 0, 0), 1, itemStack);

    EXPECT_TRUE(packet.isItemParticle());
}

TEST(ParticlePacketItemTest, IsItemParticle_ItemCobwebTypeWithOptionalData_ReturnsTrue)
{
    mc::ItemStack itemStack;
    auto packet = ParticlePacket::createItem(
        ParticleTypeId::ItemCobweb, Vector3(0, 0, 0), Vector3(0, 0, 0), Vector3(0, 0, 0), 1, itemStack);

    EXPECT_TRUE(packet.isItemParticle());
}

TEST(ParticlePacketItemTest, IsItemParticle_ItemSnowballTypeWithOptionalData_ReturnsTrue)
{
    mc::ItemStack itemStack;
    auto packet = ParticlePacket::createItem(
        ParticleTypeId::ItemSnowball, Vector3(0, 0, 0), Vector3(0, 0, 0), Vector3(0, 0, 0), 1, itemStack);

    EXPECT_TRUE(packet.isItemParticle());
}

TEST(ParticlePacketItemTest, IsItemParticle_ItemTypeWithoutOptionalData_ReturnsFalse)
{
    // Item 类型但未设置 optionalData
    ParticlePacket packet(ParticleTypeId::Item, Vector3(0, 0, 0), Vector3(0, 0, 0), Vector3(0, 0, 0), 1);
    EXPECT_FALSE(packet.isItemParticle());
}

TEST(ParticlePacketItemTest, IsItemParticle_NonItemTypeWithOptionalData_ReturnsFalse)
{
    // Flame 类型不属于 requiresItemData，即使有 optionalData 也不是 Item 粒子
    ParticlePacket packet(ParticleTypeId::Flame, Vector3(0, 0, 0), Vector3(0, 0, 0), Vector3(0, 0, 0), 1);
    std::vector<u8> fakeData = {0x00};
    packet.setOptionalData(fakeData);

    EXPECT_FALSE(packet.isItemParticle());
}

// ==================== decodeItemStack 解码测试 ====================

TEST(ParticlePacketItemTest, DecodeItemStack_EmptyItemStack_ReturnsEmpty)
{
    mc::ItemStack itemStack;
    auto packet = ParticlePacket::createItem(
        ParticleTypeId::Item, Vector3(0, 0, 0), Vector3(0, 0, 0), Vector3(0, 0, 0), 1, itemStack);

    auto decoded = packet.decodeItemStack();
    ASSERT_TRUE(decoded.has_value());
    EXPECT_TRUE(decoded->isEmpty());
}

TEST(ParticlePacketItemTest, DecodeItemStack_NotItemParticle_ReturnsNullopt)
{
    // 非 Item 粒子类型，decodeItemStack 应返回 nullopt
    ParticlePacket packet(ParticleTypeId::Flame, Vector3(0, 0, 0), Vector3(0, 0, 0), Vector3(0, 0, 0), 1);
    std::vector<u8> fakeData = {0x00};
    packet.setOptionalData(fakeData);

    auto decoded = packet.decodeItemStack();
    EXPECT_FALSE(decoded.has_value());
}

TEST(ParticlePacketItemTest, DecodeItemStack_ItemTypeWithoutOptionalData_ReturnsNullopt)
{
    // Item 类型但无 optionalData，isItemParticle 为 false，返回 nullopt
    ParticlePacket packet(ParticleTypeId::Item, Vector3(0, 0, 0), Vector3(0, 0, 0), Vector3(0, 0, 0), 1);

    auto decoded = packet.decodeItemStack();
    EXPECT_FALSE(decoded.has_value());
}

// ==================== 序列化往返测试 ====================

TEST(ParticlePacketItemTest, SerializeDeserialize_PreservesItemStack)
{
    mc::ItemStack originalItem;
    auto original = ParticlePacket::createItem(ParticleTypeId::Item,
        Vector3(10.0f, 64.0f, -20.0f),
        Vector3(0.1f, 0.2f, 0.3f),
        Vector3(0.5f, 0.5f, 0.5f),
        5,
        originalItem);

    auto serializeResult = original.serialize();
    ASSERT_TRUE(serializeResult.success()) << serializeResult.error().message();

    ParticlePacket deserialized;
    auto deserResult = deserialized.deserialize(serializeResult.value().data(), serializeResult.value().size());
    ASSERT_TRUE(deserResult.success()) << deserResult.error().message();

    EXPECT_EQ(deserialized.particleType(), ParticleTypeId::Item);
    EXPECT_TRUE(deserialized.isItemParticle());

    auto decoded = deserialized.decodeItemStack();
    ASSERT_TRUE(decoded.has_value());
    EXPECT_TRUE(decoded->isEmpty());
}

TEST(ParticlePacketItemTest, SerializeDeserialize_AllItemParticleTypes)
{
    // 测试所有 requiresItemData 返回 true 的粒子类型
    const ParticleTypeId itemTypes[] = {
        ParticleTypeId::Item,
        ParticleTypeId::ItemSlime,
        ParticleTypeId::ItemCobweb,
        ParticleTypeId::ItemSnowball,
    };

    mc::ItemStack itemStack;
    for (auto type : itemTypes) {
        auto original = ParticlePacket::createItem(
            type, Vector3(10.0f, 64.0f, -20.0f), Vector3(0, 0, 0), Vector3(0, 0, 0), 3, itemStack);

        auto serializeResult = original.serialize();
        ASSERT_TRUE(serializeResult.success()) << "Failed to serialize type " << static_cast<int>(type);

        ParticlePacket deserialized;
        auto deserResult = deserialized.deserialize(serializeResult.value().data(), serializeResult.value().size());
        ASSERT_TRUE(deserResult.success()) << "Failed to deserialize type " << static_cast<int>(type);

        EXPECT_TRUE(deserialized.isItemParticle()) << "isItemParticle failed for type " << static_cast<int>(type);

        auto decoded = deserialized.decodeItemStack();
        ASSERT_TRUE(decoded.has_value()) << "decodeItemStack failed for type " << static_cast<int>(type);
        EXPECT_TRUE(decoded->isEmpty()) << "ItemStack not empty for type " << static_cast<int>(type);
    }
}

// ==================== 边界场景测试 ====================

TEST(ParticlePacketItemTest, CreateItem_ThenModifyOptionalData_BreaksItemParticleFlag)
{
    // 验证 isItemParticle 依赖于 optionalData 非空
    mc::ItemStack itemStack;
    auto packet = ParticlePacket::createItem(
        ParticleTypeId::Item, Vector3(0, 0, 0), Vector3(0, 0, 0), Vector3(0, 0, 0), 1, itemStack);

    EXPECT_TRUE(packet.isItemParticle());

    // 清空 optionalData
    packet.setOptionalData({});
    EXPECT_FALSE(packet.isItemParticle());
}
