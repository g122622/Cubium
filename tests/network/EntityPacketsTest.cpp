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

#include "network/packet/EntityPackets.hpp"
#include <gtest/gtest.h>

using namespace mc::network;
using mc::u8;

// ==================== SpawnEntityPacket Tests ====================

class SpawnEntityPacketTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        packet.setEntityId(12345);
        packet.setEntityTypeId("minecraft:item");
        packet.setPosition(100.5f, 64.0f, -200.25f);
        packet.setRotation(45.0f, 30.0f);
        packet.setVelocity(100, -50, 200);

        std::array<u8, 16> uuid = {
            0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10};
        packet.setUuid(uuid);
    }

    SpawnEntityPacket packet;
};

TEST_F(SpawnEntityPacketTest, SerializeDeserialize)
{
    auto result = packet.serialize();
    ASSERT_TRUE(result.success());

    const auto& data = result.value();
    EXPECT_GT(data.size(), sizeof(PacketHeader));

    SpawnEntityPacket packet2;
    auto result2 = packet2.deserialize(data.data(), data.size());
    EXPECT_TRUE(result2.success());

    EXPECT_EQ(packet2.entityId(), 12345u);
    EXPECT_EQ(packet2.entityTypeId(), "minecraft:item");
    EXPECT_FLOAT_EQ(packet2.x(), 100.5f);
    EXPECT_FLOAT_EQ(packet2.y(), 64.0f);
    EXPECT_FLOAT_EQ(packet2.z(), -200.25f);
    EXPECT_FLOAT_EQ(packet2.yaw(), 45.0f);
    EXPECT_FLOAT_EQ(packet2.pitch(), 30.0f);
    EXPECT_EQ(packet2.velocityX(), 100);
    EXPECT_EQ(packet2.velocityY(), -50);
    EXPECT_EQ(packet2.velocityZ(), 200);
}

TEST_F(SpawnEntityPacketTest, PacketType)
{
    EXPECT_EQ(packet.type(), PacketType::SpawnEntity);
}

// ==================== SpawnMobPacket Tests ====================

class SpawnMobPacketTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        packet.setEntityId(54321);
        packet.setEntityTypeId("minecraft:pig");
        packet.setPosition(50.0f, 70.0f, 100.0f);
        packet.setRotation(0.0f, 0.0f, 0.0f); // yaw, pitch, headYaw
        packet.setVelocity(0, 0, 0);

        std::array<u8, 16> uuid = {
            0xAB, 0xCD, 0xEF, 0x01, 0x23, 0x45, 0x67, 0x89, 0x9A, 0xBC, 0xDE, 0xF0, 0x12, 0x34, 0x56, 0x78};
        packet.setUuid(uuid);

        std::vector<u8> metadata = {0x01, 0x02, 0x03, static_cast<u8>(0xFF)}; // 示例元数据
        packet.setMetadata(metadata);
    }

    SpawnMobPacket packet;
};

TEST_F(SpawnMobPacketTest, SerializeDeserialize)
{
    auto result = packet.serialize();
    ASSERT_TRUE(result.success());

    const auto& data = result.value();

    SpawnMobPacket packet2;
    auto result2 = packet2.deserialize(data.data(), data.size());
    EXPECT_TRUE(result2.success());

    EXPECT_EQ(packet2.entityId(), 54321u);
    EXPECT_EQ(packet2.entityTypeId(), "minecraft:pig");
    EXPECT_FLOAT_EQ(packet2.x(), 50.0f);
    EXPECT_FLOAT_EQ(packet2.y(), 70.0f);
    EXPECT_FLOAT_EQ(packet2.z(), 100.0f);
    EXPECT_EQ(packet2.metadata().size(), 4u);
    EXPECT_EQ(packet2.metadata()[0], 0x01);
    EXPECT_EQ(packet2.metadata()[3], 0xFF);
}

TEST_F(SpawnMobPacketTest, PacketType)
{
    EXPECT_EQ(packet.type(), PacketType::SpawnMob);
}

// ==================== EntityVelocityPacket Tests ====================

TEST(EntityVelocityPacketTest, SerializeDeserialize)
{
    EntityVelocityPacket packet;
    packet.setEntityId(100);
    packet.setVelocity(1000, -500, 2000);

    auto result = packet.serialize();
    ASSERT_TRUE(result.success());

    EntityVelocityPacket packet2;
    auto result2 = packet2.deserialize(result.value().data(), result.value().size());
    EXPECT_TRUE(result2.success());

    EXPECT_EQ(packet2.entityId(), 100u);
    EXPECT_EQ(packet2.velocityX(), 1000);
    EXPECT_EQ(packet2.velocityY(), -500);
    EXPECT_EQ(packet2.velocityZ(), 2000);
}

// ==================== EntityTeleportPacket Tests ====================

TEST(EntityTeleportPacketTest, SerializeDeserialize)
{
    EntityTeleportPacket packet;
    packet.setEntityId(200);
    packet.setPosition(123.45f, 64.0f, -789.0f);
    packet.setRotation(90.0f, 45.0f);
    packet.setOnGround(true);

    auto result = packet.serialize();
    ASSERT_TRUE(result.success());

    EntityTeleportPacket packet2;
    auto result2 = packet2.deserialize(result.value().data(), result.value().size());
    EXPECT_TRUE(result2.success());

    EXPECT_EQ(packet2.entityId(), 200u);
    EXPECT_FLOAT_EQ(packet2.x(), 123.45f);
    EXPECT_FLOAT_EQ(packet2.y(), 64.0f);
    EXPECT_FLOAT_EQ(packet2.z(), -789.0f);
    EXPECT_FLOAT_EQ(packet2.yaw(), 90.0f);
    EXPECT_FLOAT_EQ(packet2.pitch(), 45.0f);
    EXPECT_TRUE(packet2.onGround());
}

// ==================== EntityDestroyPacket Tests ====================

TEST(EntityDestroyPacketTest, SerializeDeserialize)
{
    EntityDestroyPacket packet;
    packet.addEntityId(1);
    packet.addEntityId(2);
    packet.addEntityId(3);

    auto result = packet.serialize();
    ASSERT_TRUE(result.success());

    EntityDestroyPacket packet2;
    auto result2 = packet2.deserialize(result.value().data(), result.value().size());
    EXPECT_TRUE(result2.success());

    EXPECT_EQ(packet2.entityIds().size(), 3u);
    EXPECT_EQ(packet2.entityIds()[0], 1u);
    EXPECT_EQ(packet2.entityIds()[1], 2u);
    EXPECT_EQ(packet2.entityIds()[2], 3u);
}

TEST(EntityDestroyPacketTest, EmptyList)
{
    EntityDestroyPacket packet;

    auto result = packet.serialize();
    ASSERT_TRUE(result.success());

    EntityDestroyPacket packet2;
    auto result2 = packet2.deserialize(result.value().data(), result.value().size());
    EXPECT_TRUE(result2.success());

    EXPECT_TRUE(packet2.entityIds().empty());
}

// ==================== EntityAnimationPacket Tests ====================

TEST(EntityAnimationPacketTest, SerializeDeserialize)
{
    EntityAnimationPacket packet;
    packet.setEntityId(300);
    packet.setAnimation(EntityAnimationPacket::Animation::SwingMainHand);

    auto result = packet.serialize();
    ASSERT_TRUE(result.success());

    EntityAnimationPacket packet2;
    auto result2 = packet2.deserialize(result.value().data(), result.value().size());
    EXPECT_TRUE(result2.success());

    EXPECT_EQ(packet2.entityId(), 300u);
    EXPECT_EQ(packet2.animation(), EntityAnimationPacket::Animation::SwingMainHand);
}

TEST(EntityAnimationPacketTest, AllAnimationTypes)
{
    EntityAnimationPacket packet;
    packet.setEntityId(1);

    auto testAnimation = [&](EntityAnimationPacket::Animation anim) {
        packet.setAnimation(anim);
        auto result = packet.serialize();
        ASSERT_TRUE(result.success());

        EntityAnimationPacket packet2;
        auto result2 = packet2.deserialize(result.value().data(), result.value().size());
        EXPECT_TRUE(result2.success());
        EXPECT_EQ(packet2.animation(), anim);
    };

    testAnimation(EntityAnimationPacket::Animation::SwingMainHand);
    testAnimation(EntityAnimationPacket::Animation::TakeDamage);
    testAnimation(EntityAnimationPacket::Animation::LeaveBed);
    testAnimation(EntityAnimationPacket::Animation::SwingOffHand);
    testAnimation(EntityAnimationPacket::Animation::CriticalEffect);
    testAnimation(EntityAnimationPacket::Animation::MagicCriticalEffect);
}

// ==================== EntityMovePacket Tests ====================

TEST(EntityMovePacketTest, SerializeDeserialize)
{
    EntityMovePacket packet;
    packet.setEntityId(400);
    packet.setDelta(100, -50, 200); // 相对移动（1/32 block）
    packet.setRotation(180.0f, 90.0f);
    packet.setOnGround(false);

    auto result = packet.serialize();
    ASSERT_TRUE(result.success());

    EntityMovePacket packet2;
    auto result2 = packet2.deserialize(result.value().data(), result.value().size());
    EXPECT_TRUE(result2.success());

    EXPECT_EQ(packet2.entityId(), 400u);
    EXPECT_EQ(packet2.deltaX(), 100);
    EXPECT_EQ(packet2.deltaY(), -50);
    EXPECT_EQ(packet2.deltaZ(), 200);
    EXPECT_FLOAT_EQ(packet2.yaw(), 180.0f);
    EXPECT_FLOAT_EQ(packet2.pitch(), 90.0f);
    EXPECT_FALSE(packet2.onGround());
}

// ==================== EntityHeadLookPacket Tests ====================

TEST(EntityHeadLookPacketTest, SerializeDeserialize)
{
    EntityHeadLookPacket packet;
    packet.setEntityId(500);
    packet.setHeadYaw(270.0f);

    auto result = packet.serialize();
    ASSERT_TRUE(result.success());

    EntityHeadLookPacket packet2;
    auto result2 = packet2.deserialize(result.value().data(), result.value().size());
    EXPECT_TRUE(result2.success());

    EXPECT_EQ(packet2.entityId(), 500u);
    EXPECT_FLOAT_EQ(packet2.headYaw(), 270.0f);
}

// ==================== EntityStatusPacket Tests ====================

TEST(EntityStatusPacketTest, SerializeDeserialize)
{
    EntityStatusPacket packet;
    packet.setEntityId(600);
    packet.setStatus(EntityStatusPacket::Status::LoveHeart);

    auto result = packet.serialize();
    ASSERT_TRUE(result.success());

    EntityStatusPacket packet2;
    auto result2 = packet2.deserialize(result.value().data(), result.value().size());
    EXPECT_TRUE(result2.success());

    EXPECT_EQ(packet2.entityId(), 600u);
    EXPECT_EQ(packet2.status(), EntityStatusPacket::Status::LoveHeart);
}

TEST(EntityStatusPacketTest, AllStatusTypes)
{
    EntityStatusPacket packet;
    packet.setEntityId(1);

    auto testStatus = [&](EntityStatusPacket::Status status) {
        packet.setStatus(status);
        auto result = packet.serialize();
        ASSERT_TRUE(result.success());

        EntityStatusPacket packet2;
        auto result2 = packet2.deserialize(result.value().data(), result.value().size());
        EXPECT_TRUE(result2.success());
        EXPECT_EQ(packet2.status(), status);
    };

    testStatus(EntityStatusPacket::Status::Hurt);
    testStatus(EntityStatusPacket::Status::Death);
    testStatus(EntityStatusPacket::Status::TamingSucceeded);
    testStatus(EntityStatusPacket::Status::LoveHeart);
    testStatus(EntityStatusPacket::Status::SheepEatGrass);
    testStatus(EntityStatusPacket::Status::ChickenLayEgg);
}

// ==================== EntityMetadataPacket Tests ====================

TEST(EntityMetadataPacketTest, SerializeDeserialize)
{
    EntityMetadataPacket packet;
    packet.setEntityId(700);

    std::vector<u8> metadata = {0x00, 0x01, 0x02, 0x03, 0x04};
    packet.setMetadata(metadata);

    auto result = packet.serialize();
    ASSERT_TRUE(result.success());

    EntityMetadataPacket packet2;
    auto result2 = packet2.deserialize(result.value().data(), result.value().size());
    EXPECT_TRUE(result2.success());

    EXPECT_EQ(packet2.entityId(), 700u);
    EXPECT_EQ(packet2.metadata().size(), 5u);
    EXPECT_EQ(packet2.metadata(), metadata);
}

TEST(EntityMetadataPacketTest, EmptyMetadata)
{
    EntityMetadataPacket packet;
    packet.setEntityId(800);

    auto result = packet.serialize();
    ASSERT_TRUE(result.success());

    EntityMetadataPacket packet2;
    auto result2 = packet2.deserialize(result.value().data(), result.value().size());
    EXPECT_TRUE(result2.success());

    EXPECT_EQ(packet2.entityId(), 800u);
    EXPECT_TRUE(packet2.metadata().empty());
}

// ==================== Error Handling Tests ====================

TEST(EntityPacketsErrorTest, SpawnEntityPacketTooSmall)
{
    SpawnEntityPacket packet;
    mc::u8 smallData[] = {0x01, 0x02}; // 数据太小

    auto result = packet.deserialize(smallData, sizeof(smallData));
    EXPECT_FALSE(result.success());
}

TEST(EntityPacketsErrorTest, SpawnMobPacketTooSmall)
{
    SpawnMobPacket packet;
    mc::u8 smallData[] = {0x01, 0x02, 0x03}; // 数据太小

    auto result = packet.deserialize(smallData, sizeof(smallData));
    EXPECT_FALSE(result.success());
}

TEST(EntityPacketsErrorTest, EntityVelocityPacketTooSmall)
{
    EntityVelocityPacket packet;
    mc::u8 smallData[] = {0x01}; // 数据太小

    auto result = packet.deserialize(smallData, sizeof(smallData));
    EXPECT_FALSE(result.success());
}

TEST(EntityPacketsErrorTest, EntityTeleportPacketTooSmall)
{
    EntityTeleportPacket packet;
    mc::u8 smallData[] = {0x01, 0x02, 0x03, 0x04}; // 数据太小

    auto result = packet.deserialize(smallData, sizeof(smallData));
    EXPECT_FALSE(result.success());
}

// ==================== PlayerInputPacket Tests ====================

TEST(PlayerInputPacketTest, SerializeDeserialize)
{
    PlayerInputPacket packet;
    packet.setStrafeSpeed(0.5f);
    packet.setForwardSpeed(1.0f);
    packet.setJumping(true);
    packet.setSneaking(false);

    auto result = packet.serialize();
    ASSERT_TRUE(result.success());

    PlayerInputPacket packet2;
    auto result2 = packet2.deserialize(result.value().data(), result.value().size());
    EXPECT_TRUE(result2.success());

    EXPECT_FLOAT_EQ(packet2.strafeSpeed(), 0.5f);
    EXPECT_FLOAT_EQ(packet2.forwardSpeed(), 1.0f);
    EXPECT_TRUE(packet2.isJumping());
    EXPECT_FALSE(packet2.isSneaking());
}

TEST(PlayerInputPacketTest, AllFlagsCombinations)
{
    PlayerInputPacket packet;
    packet.setStrafeSpeed(-0.75f);
    packet.setForwardSpeed(-0.25f);

    // Test all combinations of jumping and sneaking
    for (int jumping = 0; jumping <= 1; jumping++) {
        for (int sneaking = 0; sneaking <= 1; sneaking++) {
            packet.setJumping(jumping == 1);
            packet.setSneaking(sneaking == 1);

            auto result = packet.serialize();
            ASSERT_TRUE(result.success());

            PlayerInputPacket packet2;
            auto result2 = packet2.deserialize(result.value().data(), result.value().size());
            EXPECT_TRUE(result2.success());
            EXPECT_EQ(packet2.isJumping(), jumping == 1);
            EXPECT_EQ(packet2.isSneaking(), sneaking == 1);
        }
    }
}

TEST(PlayerInputPacketTest, PacketType)
{
    PlayerInputPacket packet;
    EXPECT_EQ(packet.type(), PacketType::PlayerInput);
}

// ==================== MoveVehiclePacket Tests ====================

TEST(MoveVehiclePacketTest, SerializeDeserialize)
{
    MoveVehiclePacket packet;
    packet.setPosition(100.5, 64.0, -200.25);
    packet.setRotation(45.0f, 30.0f);

    auto result = packet.serialize();
    ASSERT_TRUE(result.success());

    MoveVehiclePacket packet2;
    auto result2 = packet2.deserialize(result.value().data(), result.value().size());
    EXPECT_TRUE(result2.success());

    EXPECT_DOUBLE_EQ(packet2.x(), 100.5);
    EXPECT_DOUBLE_EQ(packet2.y(), 64.0);
    EXPECT_DOUBLE_EQ(packet2.z(), -200.25);
    EXPECT_FLOAT_EQ(packet2.yaw(), 45.0f);
    EXPECT_FLOAT_EQ(packet2.pitch(), 30.0f);
}

TEST(MoveVehiclePacketTest, PacketType)
{
    MoveVehiclePacket packet;
    EXPECT_EQ(packet.type(), PacketType::MoveVehicle);
}

// ==================== VehicleMovePacket Tests ====================

TEST(VehicleMovePacketTest, SerializeDeserialize)
{
    VehicleMovePacket packet;
    packet.setPosition(123.45, 67.89, -456.78);
    packet.setRotation(180.0f, -45.0f);

    auto result = packet.serialize();
    ASSERT_TRUE(result.success());

    VehicleMovePacket packet2;
    auto result2 = packet2.deserialize(result.value().data(), result.value().size());
    EXPECT_TRUE(result2.success());

    EXPECT_DOUBLE_EQ(packet2.x(), 123.45);
    EXPECT_DOUBLE_EQ(packet2.y(), 67.89);
    EXPECT_DOUBLE_EQ(packet2.z(), -456.78);
    EXPECT_FLOAT_EQ(packet2.yaw(), 180.0f);
    EXPECT_FLOAT_EQ(packet2.pitch(), -45.0f);
}

TEST(VehicleMovePacketTest, PacketType)
{
    VehicleMovePacket packet;
    EXPECT_EQ(packet.type(), PacketType::VehicleMove);
}

// ==================== EntityActionPacket Tests ====================

TEST(EntityActionPacketTest, SerializeDeserialize)
{
    EntityActionPacket packet;
    packet.setEntityId(12345);
    packet.setAction(mc::network::EntityActionType::StartRidingJump);
    packet.setAuxData(50); // Jump power 50%

    auto result = packet.serialize();
    ASSERT_TRUE(result.success());

    EntityActionPacket packet2;
    auto result2 = packet2.deserialize(result.value().data(), result.value().size());
    EXPECT_TRUE(result2.success());

    EXPECT_EQ(packet2.entityId(), 12345u);
    EXPECT_EQ(packet2.action(), mc::network::EntityActionType::StartRidingJump);
    EXPECT_EQ(packet2.auxData(), 50);
}

TEST(EntityActionPacketTest, AllActionTypes)
{
    EntityActionPacket packet;
    packet.setEntityId(1);

    // Test PressShiftKey
    packet.setAction(mc::network::EntityActionType::PressShiftKey);
    packet.setAuxData(0);
    auto result1 = packet.serialize();
    ASSERT_TRUE(result1.success());
    EntityActionPacket packet1;
    EXPECT_TRUE(packet1.deserialize(result1.value().data(), result1.value().size()).success());
    EXPECT_EQ(packet1.action(), mc::network::EntityActionType::PressShiftKey);

    // Test StartRidingJump
    packet.setAction(mc::network::EntityActionType::StartRidingJump);
    packet.setAuxData(50);
    auto result2 = packet.serialize();
    ASSERT_TRUE(result2.success());
    EntityActionPacket packet2;
    EXPECT_TRUE(packet2.deserialize(result2.value().data(), result2.value().size()).success());
    EXPECT_EQ(packet2.action(), mc::network::EntityActionType::StartRidingJump);
    EXPECT_EQ(packet2.auxData(), 50);

    // Test StopRidingJump
    packet.setAction(mc::network::EntityActionType::StopRidingJump);
    auto result3 = packet.serialize();
    ASSERT_TRUE(result3.success());
    EntityActionPacket packet3;
    EXPECT_TRUE(packet3.deserialize(result3.value().data(), result3.value().size()).success());
    EXPECT_EQ(packet3.action(), mc::network::EntityActionType::StopRidingJump);

    // Test StartFallFlying
    packet.setAction(mc::network::EntityActionType::StartFallFlying);
    auto result4 = packet.serialize();
    ASSERT_TRUE(result4.success());
    EntityActionPacket packet4;
    EXPECT_TRUE(packet4.deserialize(result4.value().data(), result4.value().size()).success());
    EXPECT_EQ(packet4.action(), mc::network::EntityActionType::StartFallFlying);
}

TEST(EntityActionPacketTest, PacketType)
{
    EntityActionPacket packet;
    EXPECT_EQ(packet.type(), PacketType::EntityAction);
}

TEST(EntityActionPacketTest, InvalidActionType)
{
    // Create serialized data with invalid action type
    mc::network::PacketSerializer ser;
    ser.writeVarInt(12345); // entity id
    ser.writeVarInt(999);   // invalid action type
    ser.writeVarInt(0);     // aux data

    EntityActionPacket packet;
    auto result = packet.deserialize(ser.buffer().data(), ser.buffer().size());
    EXPECT_FALSE(result.success()); // Should fail for invalid action type
}

// ==================== SteerBoatPacket Tests ====================

TEST(SteerBoatPacketTest, SerializeDeserialize_BothFalse)
{
    SteerBoatPacket packet;
    packet.setPaddleState(false, false);

    auto result = packet.serialize();
    ASSERT_TRUE(result.success());

    SteerBoatPacket packet2;
    auto result2 = packet2.deserialize(result.value().data(), result.value().size());
    EXPECT_TRUE(result2.success());

    EXPECT_FALSE(packet2.leftPaddle());
    EXPECT_FALSE(packet2.rightPaddle());
}

TEST(SteerBoatPacketTest, SerializeDeserialize_LeftOnly)
{
    SteerBoatPacket packet;
    packet.setPaddleState(true, false);

    auto result = packet.serialize();
    ASSERT_TRUE(result.success());

    SteerBoatPacket packet2;
    auto result2 = packet2.deserialize(result.value().data(), result.value().size());
    EXPECT_TRUE(result2.success());

    EXPECT_TRUE(packet2.leftPaddle());
    EXPECT_FALSE(packet2.rightPaddle());
}

TEST(SteerBoatPacketTest, SerializeDeserialize_RightOnly)
{
    SteerBoatPacket packet;
    packet.setPaddleState(false, true);

    auto result = packet.serialize();
    ASSERT_TRUE(result.success());

    SteerBoatPacket packet2;
    auto result2 = packet2.deserialize(result.value().data(), result.value().size());
    EXPECT_TRUE(result2.success());

    EXPECT_FALSE(packet2.leftPaddle());
    EXPECT_TRUE(packet2.rightPaddle());
}

TEST(SteerBoatPacketTest, SerializeDeserialize_BothTrue)
{
    SteerBoatPacket packet;
    packet.setPaddleState(true, true);

    auto result = packet.serialize();
    ASSERT_TRUE(result.success());

    SteerBoatPacket packet2;
    auto result2 = packet2.deserialize(result.value().data(), result.value().size());
    EXPECT_TRUE(result2.success());

    EXPECT_TRUE(packet2.leftPaddle());
    EXPECT_TRUE(packet2.rightPaddle());
}

TEST(SteerBoatPacketTest, SetLeftPaddle)
{
    SteerBoatPacket packet;
    packet.setLeftPaddle(true);
    packet.setRightPaddle(false);

    EXPECT_TRUE(packet.leftPaddle());
    EXPECT_FALSE(packet.rightPaddle());
}

TEST(SteerBoatPacketTest, SetRightPaddle)
{
    SteerBoatPacket packet;
    packet.setLeftPaddle(false);
    packet.setRightPaddle(true);

    EXPECT_FALSE(packet.leftPaddle());
    EXPECT_TRUE(packet.rightPaddle());
}

TEST(SteerBoatPacketTest, PacketType)
{
    SteerBoatPacket packet;
    EXPECT_EQ(packet.type(), PacketType::SteerBoat);
}

TEST(SteerBoatPacketTest, DefaultValues)
{
    SteerBoatPacket packet;
    EXPECT_FALSE(packet.leftPaddle());
    EXPECT_FALSE(packet.rightPaddle());
}

TEST(SteerBoatPacketTest, Deserialize_TooSmall)
{
    SteerBoatPacket packet;
    mc::u8 smallData[] = {0x00}; // 只有一个字节，无法读取两个 bool

    auto result = packet.deserialize(smallData, sizeof(smallData));
    // 一个 bool 只需要一个字节，但需要读取两个 bool，所以应该失败
    EXPECT_FALSE(result.success());
}

TEST(SteerBoatPacketTest, Deserialize_EmptyData)
{
    SteerBoatPacket packet;
    mc::u8 emptyData[] = {};

    auto result = packet.deserialize(emptyData, 0);
    EXPECT_FALSE(result.success()); // 空数据应该失败
}

TEST(SteerBoatPacketTest, SerializeDeserialize_MultipleTimes)
{
    SteerBoatPacket packet;

    // 测试多次序列化/反序列化
    for (int i = 0; i < 4; i++) {
        bool left = (i & 1) != 0;
        bool right = (i & 2) != 0;

        packet.setPaddleState(left, right);
        auto result = packet.serialize();
        ASSERT_TRUE(result.success());

        SteerBoatPacket packet2;
        auto result2 = packet2.deserialize(result.value().data(), result.value().size());
        ASSERT_TRUE(result2.success());

        EXPECT_EQ(packet2.leftPaddle(), left);
        EXPECT_EQ(packet2.rightPaddle(), right);
    }
}
