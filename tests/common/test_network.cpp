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

#include <gtest/gtest.h>

#include "common/network/packet/Packet.hpp"
#include "common/network/packet/PacketSerializer.hpp"
#include "common/network/packet/ProtocolPackets.hpp"

using namespace mc::network;
using namespace mc;

// ============================================================================
// PacketSerializer 测试
// ============================================================================

TEST(PacketSerializer, WriteReadU8)
{
    PacketSerializer serializer;
    serializer.writeU8(0x12);
    serializer.writeU8(0xFF);

    EXPECT_EQ(serializer.size(), 2u);

    auto result1 = serializer.readU8();
    EXPECT_TRUE(result1.success());
    EXPECT_EQ(result1.value(), 0x12);

    auto result2 = serializer.readU8();
    EXPECT_TRUE(result2.success());
    EXPECT_EQ(result2.value(), 0xFF);
}

TEST(PacketSerializer, WriteReadU16)
{
    PacketSerializer serializer;
    serializer.writeU16(0x1234);
    serializer.writeU16(0xFFFF);

    EXPECT_EQ(serializer.size(), 4u);

    auto result1 = serializer.readU16();
    EXPECT_TRUE(result1.success());
    EXPECT_EQ(result1.value(), 0x1234);

    auto result2 = serializer.readU16();
    EXPECT_TRUE(result2.success());
    EXPECT_EQ(result2.value(), 0xFFFF);
}

TEST(PacketSerializer, WriteReadU32)
{
    PacketSerializer serializer;
    serializer.writeU32(0x12345678);
    serializer.writeU32(0xFFFFFFFF);

    auto result1 = serializer.readU32();
    EXPECT_TRUE(result1.success());
    EXPECT_EQ(result1.value(), 0x12345678u);

    auto result2 = serializer.readU32();
    EXPECT_TRUE(result2.success());
    EXPECT_EQ(result2.value(), 0xFFFFFFFFu);
}

TEST(PacketSerializer, WriteReadU64)
{
    PacketSerializer serializer;
    serializer.writeU64(0x0123456789ABCDEF);
    serializer.writeU64(0xFFFFFFFFFFFFFFFF);

    auto result1 = serializer.readU64();
    EXPECT_TRUE(result1.success());
    EXPECT_EQ(result1.value(), 0x0123456789ABCDEFull);

    auto result2 = serializer.readU64();
    EXPECT_TRUE(result2.success());
    EXPECT_EQ(result2.value(), 0xFFFFFFFFFFFFFFFFull);
}

TEST(PacketSerializer, WriteReadFloat)
{
    PacketSerializer serializer;
    serializer.writeF32(3.14159f);
    serializer.writeF32(-1234.5f);

    auto result1 = serializer.readF32();
    EXPECT_TRUE(result1.success());
    EXPECT_NEAR(result1.value(), 3.14159f, 0.0001f);

    auto result2 = serializer.readF32();
    EXPECT_TRUE(result2.success());
    EXPECT_NEAR(result2.value(), -1234.5f, 0.0001f);
}

TEST(PacketSerializer, WriteReadDouble)
{
    PacketSerializer serializer;
    serializer.writeF64(3.14159265358979);
    serializer.writeF64(-1234.5678);

    auto result1 = serializer.readF64();
    EXPECT_TRUE(result1.success());
    EXPECT_NEAR(result1.value(), 3.14159265358979, 0.00000001);

    auto result2 = serializer.readF64();
    EXPECT_TRUE(result2.success());
    EXPECT_NEAR(result2.value(), -1234.5678, 0.0001);
}

TEST(PacketSerializer, WriteReadBool)
{
    PacketSerializer serializer;
    serializer.writeBool(true);
    serializer.writeBool(false);

    auto result1 = serializer.readBool();
    EXPECT_TRUE(result1.success());
    EXPECT_TRUE(result1.value());

    auto result2 = serializer.readBool();
    EXPECT_TRUE(result2.success());
    EXPECT_FALSE(result2.value());
}

TEST(PacketSerializer, WriteReadString)
{
    PacketSerializer serializer;
    serializer.writeString("Hello");
    serializer.writeString("World");

    auto result1 = serializer.readString();
    EXPECT_TRUE(result1.success());
    EXPECT_EQ(result1.value(), "Hello");

    auto result2 = serializer.readString();
    EXPECT_TRUE(result2.success());
    EXPECT_EQ(result2.value(), "World");
}

TEST(PacketSerializer, WriteReadBytes)
{
    PacketSerializer serializer;
    std::vector<mc::u8> data = {0x01, 0x02, 0x03, 0x04, 0x05};
    serializer.writeBytes(data);

    auto result = serializer.readBytes(5);
    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value().size(), 5u);
    EXPECT_EQ(result.value()[0], 0x01);
    EXPECT_EQ(result.value()[4], 0x05);
}

TEST(PacketSerializer, OutOfBounds)
{
    PacketSerializer serializer;
    serializer.writeU8(0x12);

    // 读取成功
    auto result1 = serializer.readU8();
    EXPECT_TRUE(result1.success());

    // 读取失败 - 没有数据了
    auto result2 = serializer.readU8();
    EXPECT_FALSE(result2.success());
    EXPECT_EQ(result2.error().code(), mc::ErrorCode::OutOfBounds);
}

TEST(PacketSerializer, ResetRead)
{
    PacketSerializer serializer;
    serializer.writeU8(0x12);
    serializer.writeU8(0x34);

    // 读取一次
    auto result1 = serializer.readU8();
    EXPECT_TRUE(result1.success());
    EXPECT_EQ(result1.value(), 0x12);

    // 重置读位置
    serializer.resetRead();

    // 再次读取应该从头开始
    auto result2 = serializer.readU8();
    EXPECT_TRUE(result2.success());
    EXPECT_EQ(result2.value(), 0x12);
}

// ============================================================================
// KeepAlivePacket 测试
// ============================================================================

TEST(KeepAlivePacket, SerializeDeserialize)
{
    KeepAlivePacket original;
    original.setTimestamp(1234567890);
    original.setFlags(0x0001);

    auto serializeResult = original.serialize();
    EXPECT_TRUE(serializeResult.success());

    const auto& data = serializeResult.value();
    EXPECT_GE(data.size(), PACKET_HEADER_SIZE);

    KeepAlivePacket deserialized;
    auto deserializeResult = deserialized.deserialize(data.data(), data.size());
    EXPECT_TRUE(deserializeResult.success());
    EXPECT_EQ(deserialized.timestamp(), 1234567890);
    EXPECT_EQ(deserialized.flags(), 0x0001);
}

TEST(KeepAlivePacket, PacketTooSmall)
{
    std::vector<mc::u8> smallData(PACKET_HEADER_SIZE - 1, 0x00);

    KeepAlivePacket packet;
    auto result = packet.deserialize(smallData.data(), smallData.size());
    EXPECT_FALSE(result.success());
    EXPECT_EQ(result.error().code(), mc::ErrorCode::InvalidArgument);
}

TEST(PlayerTryUseItemOnBlockPacket, SerializeDeserialize)
{
    PlayerTryUseItemOnBlockPacket original(10, 64, -3, Direction::Up, 0.25f, 0.75f, 0.5f, 0);

    PacketSerializer serializer;
    original.serialize(serializer);

    PacketDeserializer deserializer(serializer.data(), serializer.size());
    auto result = PlayerTryUseItemOnBlockPacket::deserialize(deserializer);
    ASSERT_TRUE(result.success());

    const auto& packet = result.value();
    EXPECT_EQ(packet.x(), 10);
    EXPECT_EQ(packet.y(), 64);
    EXPECT_EQ(packet.z(), -3);
    EXPECT_EQ(packet.face(), Direction::Up);
    EXPECT_FLOAT_EQ(packet.hitX(), 0.25f);
    EXPECT_FLOAT_EQ(packet.hitY(), 0.75f);
    EXPECT_FLOAT_EQ(packet.hitZ(), 0.5f);
    EXPECT_EQ(packet.hand(), 0);
}

// ============================================================================
// DisconnectPacket 测试
// ============================================================================

TEST(DisconnectPacket, SerializeDeserialize)
{
    DisconnectPacket original;
    original.setReason("Server shutdown");
    original.setFlags(0x0002);

    auto serializeResult = original.serialize();
    EXPECT_TRUE(serializeResult.success());

    const auto& data = serializeResult.value();
    EXPECT_GE(data.size(), PACKET_HEADER_SIZE);

    DisconnectPacket deserialized;
    auto deserializeResult = deserialized.deserialize(data.data(), data.size());
    EXPECT_TRUE(deserializeResult.success());
    EXPECT_EQ(deserialized.reason(), "Server shutdown");
    EXPECT_EQ(deserialized.flags(), 0x0002);
}

// ============================================================================
// PacketDeserializer 测试
// ============================================================================

TEST(PacketDeserializer, ReadOperations)
{
    PacketSerializer serializer;
    serializer.writeU8(0x12);
    serializer.writeU16(0x1234);
    serializer.writeU32(0x12345678);

    PacketDeserializer deserializer(serializer.buffer());

    auto result1 = deserializer.readU8();
    EXPECT_TRUE(result1.success());
    EXPECT_EQ(result1.value(), 0x12);

    auto result2 = deserializer.readU16();
    EXPECT_TRUE(result2.success());
    EXPECT_EQ(result2.value(), 0x1234);

    auto result3 = deserializer.readU32();
    EXPECT_TRUE(result3.success());
    EXPECT_EQ(result3.value(), 0x12345678u);
}

TEST(PacketDeserializer, HasRemaining)
{
    PacketSerializer serializer;
    serializer.writeU32(0x12345678);

    PacketDeserializer deserializer(serializer.buffer());

    EXPECT_TRUE(deserializer.hasRemaining(4));
    EXPECT_TRUE(deserializer.hasRemaining(2));
    EXPECT_FALSE(deserializer.hasRemaining(5));

    (void)deserializer.readU32();
    EXPECT_FALSE(deserializer.hasRemaining(1));
}

// ============================================================================
// VarInt/VarLong 测试
// ============================================================================

TEST(PacketSerializer, WriteReadVarInt)
{
    PacketSerializer serializer;

    // 测试各种值
    serializer.writeVarInt(0);
    serializer.writeVarInt(1);
    serializer.writeVarInt(127);
    serializer.writeVarInt(128);
    serializer.writeVarInt(16383);
    serializer.writeVarInt(16384);
    serializer.writeVarInt(2097151);
    serializer.writeVarInt(2097152);
    serializer.writeVarInt(-1);
    serializer.writeVarInt(-2147483648); // INT_MIN

    auto result0 = serializer.readVarInt();
    EXPECT_TRUE(result0.success());
    EXPECT_EQ(result0.value(), 0);

    auto result1 = serializer.readVarInt();
    EXPECT_TRUE(result1.success());
    EXPECT_EQ(result1.value(), 1);

    auto result127 = serializer.readVarInt();
    EXPECT_TRUE(result127.success());
    EXPECT_EQ(result127.value(), 127);

    auto result128 = serializer.readVarInt();
    EXPECT_TRUE(result128.success());
    EXPECT_EQ(result128.value(), 128);

    auto result16383 = serializer.readVarInt();
    EXPECT_TRUE(result16383.success());
    EXPECT_EQ(result16383.value(), 16383);

    auto result16384 = serializer.readVarInt();
    EXPECT_TRUE(result16384.success());
    EXPECT_EQ(result16384.value(), 16384);

    auto result2097151 = serializer.readVarInt();
    EXPECT_TRUE(result2097151.success());
    EXPECT_EQ(result2097151.value(), 2097151);

    auto result2097152 = serializer.readVarInt();
    EXPECT_TRUE(result2097152.success());
    EXPECT_EQ(result2097152.value(), 2097152);

    auto resultNeg1 = serializer.readVarInt();
    EXPECT_TRUE(resultNeg1.success());
    EXPECT_EQ(resultNeg1.value(), -1);

    auto resultMin = serializer.readVarInt();
    EXPECT_TRUE(resultMin.success());
    EXPECT_EQ(resultMin.value(), -2147483647 - 1);
}

TEST(PacketSerializer, VarIntSize)
{
    // VarInt 编码后的大小应该是可变的
    PacketSerializer serializer;

    // 0-127: 1字节
    size_t start = serializer.size();
    serializer.writeVarInt(0);
    EXPECT_EQ(serializer.size() - start, 1u);

    serializer.clear();
    start = serializer.size();
    serializer.writeVarInt(127);
    EXPECT_EQ(serializer.size() - start, 1u);

    // 128-16383: 2字节
    serializer.clear();
    start = serializer.size();
    serializer.writeVarInt(128);
    EXPECT_EQ(serializer.size() - start, 2u);

    serializer.clear();
    start = serializer.size();
    serializer.writeVarInt(16383);
    EXPECT_EQ(serializer.size() - start, 2u);

    // 16384-2097151: 3字节
    serializer.clear();
    start = serializer.size();
    serializer.writeVarInt(16384);
    EXPECT_EQ(serializer.size() - start, 3u);
}

TEST(PacketSerializer, WriteReadVarLong)
{
    PacketSerializer serializer;

    serializer.writeVarLong(0);
    serializer.writeVarLong(9223372036854775807LL); // LONG_MAX
    serializer.writeVarLong(-1);
    serializer.writeVarLong(-9223372036854775807LL - 1); // LONG_MIN

    auto result0 = serializer.readVarLong();
    EXPECT_TRUE(result0.success());
    EXPECT_EQ(result0.value(), 0);

    auto resultMax = serializer.readVarLong();
    EXPECT_TRUE(resultMax.success());
    EXPECT_EQ(resultMax.value(), 9223372036854775807LL);

    auto resultNeg1 = serializer.readVarLong();
    EXPECT_TRUE(resultNeg1.success());
    EXPECT_EQ(resultNeg1.value(), -1);

    auto resultMin = serializer.readVarLong();
    EXPECT_TRUE(resultMin.success());
    EXPECT_EQ(resultMin.value(), -9223372036854775807LL - 1);
}

// ============================================================================
// ProtocolPackets 测试
// ============================================================================

TEST(PlayerPosition, SerializeDeserialize)
{
    PlayerPosition pos1(100.5, 64.0, -200.25, 90.0f, 45.0f, true);

    PacketSerializer ser;
    pos1.serialize(ser);

    PacketDeserializer deser(ser.buffer());
    auto result = PlayerPosition::deserialize(deser);

    EXPECT_TRUE(result.success());
    EXPECT_DOUBLE_EQ(result.value().x, 100.5);
    EXPECT_DOUBLE_EQ(result.value().y, 64.0);
    EXPECT_DOUBLE_EQ(result.value().z, -200.25);
    EXPECT_FLOAT_EQ(result.value().yaw, 90.0f);
    EXPECT_FLOAT_EQ(result.value().pitch, 45.0f);
    EXPECT_TRUE(result.value().onGround);
}

TEST(LoginRequestPacket, SerializeDeserialize)
{
    LoginRequestPacket original("TestPlayer", 753);

    PacketSerializer ser;
    original.serialize(ser);

    PacketDeserializer deser(ser.buffer());
    auto result = LoginRequestPacket::deserialize(deser);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value().username(), "TestPlayer");
    EXPECT_EQ(result.value().protocolVersion(), 753);
}

TEST(LoginRequestPacket, InvalidUsername)
{
    // 空用户名
    LoginRequestPacket emptyUsername("", 753);
    PacketSerializer ser;
    emptyUsername.serialize(ser);
    PacketDeserializer deser(ser.buffer());
    auto result = LoginRequestPacket::deserialize(deser);
    EXPECT_FALSE(result.success());
}

TEST(LoginResponsePacket, SerializeDeserialize)
{
    Uuid testUuid{};
    for (size_t i = 0; i < testUuid.size(); ++i) {
        testUuid[i] = static_cast<u8>(i + 1);
    }
    LoginResponsePacket original(true, 12345, 100, testUuid, "TestPlayer", "Welcome!");

    PacketSerializer ser;
    original.serialize(ser);

    PacketDeserializer deser(ser.buffer());
    auto result = LoginResponsePacket::deserialize(deser);

    EXPECT_TRUE(result.success());
    EXPECT_TRUE(result.value().success());
    EXPECT_EQ(result.value().playerId(), 12345u);
    EXPECT_EQ(result.value().entityId(), 100u);
    EXPECT_EQ(result.value().uuid(), testUuid);
    EXPECT_EQ(result.value().username(), "TestPlayer");
    EXPECT_EQ(result.value().message(), "Welcome!");
}

TEST(PlayerMovePacket, FullPosition)
{
    PlayerPosition pos(100.0, 64.0, 200.0, 45.0f, 30.0f, true);
    PlayerMovePacket original(pos, PlayerMovePacket::MoveType::Full);

    PacketSerializer ser;
    original.serialize(ser);

    PacketDeserializer deser(ser.buffer());
    auto result = PlayerMovePacket::deserialize(deser);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value().type(), PlayerMovePacket::MoveType::Full);
    EXPECT_DOUBLE_EQ(result.value().x(), 100.0);
    EXPECT_DOUBLE_EQ(result.value().y(), 64.0);
    EXPECT_DOUBLE_EQ(result.value().z(), 200.0);
    EXPECT_FLOAT_EQ(result.value().yaw(), 45.0f);
    EXPECT_FLOAT_EQ(result.value().pitch(), 30.0f);
    EXPECT_TRUE(result.value().onGround());
}

TEST(PlayerMovePacket, PositionOnly)
{
    PlayerMovePacket original;
    original.setPosition(PlayerPosition(50.0, 70.0, 100.0, 0.0f, 0.0f, false));
    original.setType(PlayerMovePacket::MoveType::Position);

    PacketSerializer ser;
    original.serialize(ser);

    PacketDeserializer deser(ser.buffer());
    auto result = PlayerMovePacket::deserialize(deser);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value().type(), PlayerMovePacket::MoveType::Position);
    EXPECT_DOUBLE_EQ(result.value().x(), 50.0);
    EXPECT_DOUBLE_EQ(result.value().y(), 70.0);
    EXPECT_DOUBLE_EQ(result.value().z(), 100.0);
    EXPECT_FALSE(result.value().onGround());
}

TEST(PlayerMovePacket, RotationOnly)
{
    PlayerMovePacket original;
    original.setPosition(PlayerPosition(0.0, 0.0, 0.0, 180.0f, 90.0f, true));
    original.setType(PlayerMovePacket::MoveType::Rotation);

    PacketSerializer ser;
    original.serialize(ser);

    PacketDeserializer deser(ser.buffer());
    auto result = PlayerMovePacket::deserialize(deser);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value().type(), PlayerMovePacket::MoveType::Rotation);
    EXPECT_FLOAT_EQ(result.value().yaw(), 180.0f);
    EXPECT_FLOAT_EQ(result.value().pitch(), 90.0f);
    EXPECT_TRUE(result.value().onGround());
}

TEST(TeleportPacket, SerializeDeserialize)
{
    TeleportPacket original(100.0, 64.0, -50.0, 0.0f, 0.0f, 42);

    PacketSerializer ser;
    original.serialize(ser);

    PacketDeserializer deser(ser.buffer());
    auto result = TeleportPacket::deserialize(deser);

    EXPECT_TRUE(result.success());
    EXPECT_DOUBLE_EQ(result.value().x(), 100.0);
    EXPECT_DOUBLE_EQ(result.value().y(), 64.0);
    EXPECT_DOUBLE_EQ(result.value().z(), -50.0);
    EXPECT_FLOAT_EQ(result.value().yaw(), 0.0f);
    EXPECT_FLOAT_EQ(result.value().pitch(), 0.0f);
    EXPECT_EQ(result.value().teleportId(), 42u);
}

TEST(TeleportConfirmPacket, SerializeDeserialize)
{
    TeleportConfirmPacket original(12345);

    PacketSerializer ser;
    original.serialize(ser);

    PacketDeserializer deser(ser.buffer());
    auto result = TeleportConfirmPacket::deserialize(deser);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value().teleportId(), 12345u);
}

TEST(SimpleKeepAlive, SerializeDeserialize)
{
    SimpleKeepAlive original(9876543210ULL);

    PacketSerializer ser;
    original.serialize(ser);

    PacketDeserializer deser(ser.buffer());
    auto result = SimpleKeepAlive::deserialize(deser);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value().id(), 9876543210ULL);
}

TEST(ChunkDataPacket, SerializeDeserialize)
{
    std::vector<u8> chunkData(1024, 0xAB);
    ChunkDataPacket original(10, -5, 0, std::move(chunkData)); // dimension=0 (主世界)

    PacketSerializer ser;
    original.serialize(ser);

    PacketDeserializer deser(ser.buffer());
    auto result = ChunkDataPacket::deserialize(deser);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value().x(), 10);
    EXPECT_EQ(result.value().z(), -5);
    EXPECT_EQ(result.value().dimension(), 0);
    EXPECT_EQ(result.value().size(), 1024u);
}

TEST(UnloadChunkPacket, SerializeDeserialize)
{
    UnloadChunkPacket original(15, -20, 0); // dimension=0 (主世界)

    PacketSerializer ser;
    original.serialize(ser);

    PacketDeserializer deser(ser.buffer());
    auto result = UnloadChunkPacket::deserialize(deser);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value().x(), 15);
    EXPECT_EQ(result.value().z(), -20);
    EXPECT_EQ(result.value().dimension(), 0);
}

TEST(PlayerSpawnPacket, SerializeDeserialize)
{
    PlayerPosition pos(100.0, 64.0, 200.0, 0.0f, 0.0f, true);
    PlayerSpawnPacket original(12345, "OtherPlayer", pos);

    PacketSerializer ser;
    original.serialize(ser);

    PacketDeserializer deser(ser.buffer());
    auto result = PlayerSpawnPacket::deserialize(deser);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value().playerId(), 12345u);
    EXPECT_EQ(result.value().username(), "OtherPlayer");
    EXPECT_DOUBLE_EQ(result.value().position().x, 100.0);
}

TEST(PlayerDespawnPacket, SerializeDeserialize)
{
    PlayerDespawnPacket original(12345);

    PacketSerializer ser;
    original.serialize(ser);

    PacketDeserializer deser(ser.buffer());
    auto result = PlayerDespawnPacket::deserialize(deser);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value().playerId(), 12345u);
}

TEST(BlockUpdatePacket, SerializeDeserialize)
{
    BlockUpdatePacket original(100, 64, -200, 42u); // 使用 blockStateId

    PacketSerializer ser;
    original.serialize(ser);

    PacketDeserializer deser(ser.buffer());
    auto result = BlockUpdatePacket::deserialize(deser);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value().x(), 100);
    EXPECT_EQ(result.value().y(), 64);
    EXPECT_EQ(result.value().z(), -200);
    EXPECT_EQ(result.value().blockStateId(), 42u);
}

TEST(BlockInteractionPacket, SerializeDeserialize)
{
    BlockInteractionPacket original(BlockInteractionAction::StartDestroyBlock, 12, 70, -5, Direction::North);

    PacketSerializer ser;
    original.serialize(ser);

    PacketDeserializer deser(ser.buffer());
    auto result = BlockInteractionPacket::deserialize(deser);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value().action(), BlockInteractionAction::StartDestroyBlock);
    EXPECT_EQ(result.value().x(), 12);
    EXPECT_EQ(result.value().y(), 70);
    EXPECT_EQ(result.value().z(), -5);
    EXPECT_EQ(result.value().face(), Direction::North);
}

TEST(ChatMessagePacket, SerializeDeserialize)
{
    ChatMessagePacket original("Hello, world!", 12345);

    PacketSerializer ser;
    original.serialize(ser);

    PacketDeserializer deser(ser.buffer());
    auto result = ChatMessagePacket::deserialize(deser);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value().message(), "Hello, world!");
    EXPECT_EQ(result.value().senderId(), 12345u);
}
