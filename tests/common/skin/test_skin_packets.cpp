#include <gtest/gtest.h>
#include "common/skin/network/SkinPackets.hpp"
#include "common/skin/core/GameProfile.hpp"
#include "common/network/packet/PacketSerializer.hpp"
#include <array>

using namespace mc::skin;

class SkinPacketsTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(SkinPacketsTest, PlayerListEntryConstruction) {
    std::array<mc::u8, 16> uuid = GameProfile::parseUUID("550e8400-e29b-41d4-a716-446655440000");
    GameProfile profile(uuid, "TestPlayer");

    PlayerListEntry entry = PlayerListEntry::createAdd(profile, mc::GameMode::Survival, 50);

    for (size_t i = 0; i < 16; ++i) {
        EXPECT_EQ(uuid[i], entry.uuid[i]);
    }
    EXPECT_EQ("TestPlayer", entry.name);
    EXPECT_EQ(mc::GameMode::Survival, entry.gameMode);
    EXPECT_EQ(50, entry.ping);
    EXPECT_FALSE(entry.displayName.has_value());
}

TEST_F(SkinPacketsTest, PlayerListEntryCreateRemove) {
    std::array<mc::u8, 16> uuid = GameProfile::parseUUID("550e8400-e29b-41d4-a716-446655440000");

    PlayerListEntry entry = PlayerListEntry::createRemove(uuid);

    for (size_t i = 0; i < 16; ++i) {
        EXPECT_EQ(uuid[i], entry.uuid[i]);
    }
    // Remove 操作只需要 UUID
}

TEST_F(SkinPacketsTest, PlayerListEntryCreateUpdateLatency) {
    std::array<mc::u8, 16> uuid = GameProfile::parseUUID("550e8400-e29b-41d4-a716-446655440000");

    PlayerListEntry entry = PlayerListEntry::createUpdateLatency(uuid, 200);

    for (size_t i = 0; i < 16; ++i) {
        EXPECT_EQ(uuid[i], entry.uuid[i]);
    }
    EXPECT_EQ(200, entry.ping);
}

TEST_F(SkinPacketsTest, PlayerListEntryCreateUpdateGameMode) {
    std::array<mc::u8, 16> uuid = GameProfile::parseUUID("550e8400-e29b-41d4-a716-446655440000");

    PlayerListEntry entry = PlayerListEntry::createUpdateGameMode(uuid, mc::GameMode::Creative);

    for (size_t i = 0; i < 16; ++i) {
        EXPECT_EQ(uuid[i], entry.uuid[i]);
    }
    EXPECT_EQ(mc::GameMode::Creative, entry.gameMode);
}

TEST_F(SkinPacketsTest, PlayerListItemPacketConstruction) {
    PlayerListItemPacket packet(PlayerListAction::AddPlayer);
    EXPECT_EQ(PlayerListAction::AddPlayer, packet.action());
    EXPECT_TRUE(packet.entries().empty());
}

TEST_F(SkinPacketsTest, PlayerListItemPacketAddEntry) {
    PlayerListItemPacket packet(PlayerListAction::AddPlayer);

    std::array<mc::u8, 16> uuid = GameProfile::parseUUID("550e8400-e29b-41d4-a716-446655440000");
    GameProfile profile(uuid, "TestPlayer");

    PlayerListEntry entry = PlayerListEntry::createAdd(profile, mc::GameMode::Survival, 50);
    packet.addEntry(entry);

    EXPECT_EQ(1u, packet.entries().size());
    EXPECT_EQ("TestPlayer", packet.entries()[0].name);
}

TEST_F(SkinPacketsTest, PlayerListItemPacketSerializeAddPlayer) {
    PlayerListItemPacket packet(PlayerListAction::AddPlayer);

    std::array<mc::u8, 16> uuid = GameProfile::parseUUID("550e8400-e29b-41d4-a716-446655440000");
    GameProfile profile(uuid, "TestPlayer");
    profile.addProperty({"textures", "dGVzdA=="});  // base64 of "test"

    PlayerListEntry entry = PlayerListEntry::createAdd(profile, mc::GameMode::Survival, 50);
    packet.addEntry(entry);

    auto serializeResult = packet.serialize();
    ASSERT_TRUE(serializeResult.success());
    EXPECT_FALSE(serializeResult.value().empty());
}

TEST_F(SkinPacketsTest, PlayerListItemPacketSerializeRemovePlayer) {
    PlayerListItemPacket packet(PlayerListAction::RemovePlayer);

    std::array<mc::u8, 16> uuid = GameProfile::parseUUID("550e8400-e29b-41d4-a716-446655440000");

    PlayerListEntry entry = PlayerListEntry::createRemove(uuid);
    packet.addEntry(entry);

    auto serializeResult = packet.serialize();
    ASSERT_TRUE(serializeResult.success());
    EXPECT_FALSE(serializeResult.value().empty());
}

TEST_F(SkinPacketsTest, PlayerListItemPacketMultipleEntries) {
    PlayerListItemPacket packet(PlayerListAction::AddPlayer);

    std::array<mc::u8, 16> uuid1 = GameProfile::parseUUID("550e8400-e29b-41d4-a716-446655440000");
    std::array<mc::u8, 16> uuid2 = GameProfile::parseUUID("550e8400-e29b-41d4-a716-446655440001");

    GameProfile profile1(uuid1, "Player1");
    GameProfile profile2(uuid2, "Player2");

    packet.addEntry(PlayerListEntry::createAdd(profile1, mc::GameMode::Survival, 10));
    packet.addEntry(PlayerListEntry::createAdd(profile2, mc::GameMode::Creative, 20));

    EXPECT_EQ(2u, packet.entries().size());

    EXPECT_EQ("Player1", packet.entries()[0].name);
    EXPECT_EQ(mc::GameMode::Survival, packet.entries()[0].gameMode);
    EXPECT_EQ(10, packet.entries()[0].ping);

    EXPECT_EQ("Player2", packet.entries()[1].name);
    EXPECT_EQ(mc::GameMode::Creative, packet.entries()[1].gameMode);
    EXPECT_EQ(20, packet.entries()[1].ping);
}

TEST_F(SkinPacketsTest, PlayerListActionValues) {
    EXPECT_EQ(0, static_cast<int>(PlayerListAction::AddPlayer));
    EXPECT_EQ(1, static_cast<int>(PlayerListAction::UpdateGameMode));
    EXPECT_EQ(2, static_cast<int>(PlayerListAction::UpdateLatency));
    EXPECT_EQ(3, static_cast<int>(PlayerListAction::UpdateDisplayName));
    EXPECT_EQ(4, static_cast<int>(PlayerListAction::RemovePlayer));
}
