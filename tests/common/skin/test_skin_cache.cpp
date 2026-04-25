#include <gtest/gtest.h>
#include "common/skin/manager/SkinCache.hpp"
#include "common/skin/network/PlayerSkinInfo.hpp"
#include "common/skin/network/SkinPackets.hpp"
#include "common/skin/core/SkinTextures.hpp"
#include "common/entity/entities/player/PlayerModelPart.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/network/packet/PacketSerializer.hpp"
#include "common/core/Types.hpp"
#include <filesystem>
#include <fstream>
#include <ctime>

using namespace mc::skin;

class SkinCacheTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 使用临时目录
        testDir_ = "./test_skin_cache_" + std::to_string(std::time(nullptr));
        cache_ = std::make_unique<SkinCache>(testDir_);
    }

    void TearDown() override {
        cache_.reset();

        // 清理临时目录
        std::error_code ec;
        std::filesystem::remove_all(testDir_, ec);
    }

    mc::String testDir_;
    std::unique_ptr<SkinCache> cache_;
};

TEST_F(SkinCacheTest, Initialize) {
    auto result = cache_->initialize();
    EXPECT_TRUE(result.success());
}

TEST_F(SkinCacheTest, SaveAndReadSkin) {
    auto initResult = cache_->initialize();
    ASSERT_TRUE(initResult.success());

    mc::String hash = "abc123def456";
    std::vector<mc::u8> testData(64 * 64 * 4, 0xAB);  // 64x64 RGBA

    // 保存
    auto saveResult = cache_->saveSkin(hash, testData);
    EXPECT_TRUE(saveResult.success());

    // 检查存在
    EXPECT_TRUE(cache_->hasSkin(hash));

    // 读取
    auto readResult = cache_->readSkin(hash);
    EXPECT_TRUE(readResult.success());
    EXPECT_EQ(testData.size(), readResult.value().size());
    EXPECT_EQ(0xAB, readResult.value()[0]);

    // 清理
    EXPECT_TRUE(cache_->removeSkin(hash));
    EXPECT_FALSE(cache_->hasSkin(hash));
}

TEST_F(SkinCacheTest, GenerateSkinLocation) {
    auto initResult = cache_->initialize();
    ASSERT_TRUE(initResult.success());

    mc::String hash = "abc123def456";
    mc::ResourceLocation location = cache_->generateSkinLocation(hash);

    EXPECT_EQ("minecraft:skins/ab/abc123def456", location.toString());
}

TEST_F(SkinCacheTest, CacheCount) {
    auto initResult = cache_->initialize();
    ASSERT_TRUE(initResult.success());

    EXPECT_EQ(0u, cache_->cacheCount());

    std::vector<mc::u8> testData(64 * 64 * 4, 0);

    cache_->saveSkin("hash1", testData);
    EXPECT_EQ(1u, cache_->cacheCount());

    cache_->saveSkin("hash2", testData);
    EXPECT_EQ(2u, cache_->cacheCount());

    cache_->removeSkin("hash1");
    EXPECT_EQ(1u, cache_->cacheCount());
}

TEST_F(SkinCacheTest, ClearAll) {
    auto initResult = cache_->initialize();
    ASSERT_TRUE(initResult.success());

    std::vector<mc::u8> testData(64 * 64 * 4, 0);

    cache_->saveSkin("hash1", testData);
    cache_->saveSkin("hash2", testData);
    EXPECT_GT(cache_->cacheCount(), 0u);

    cache_->clearAll();
    EXPECT_EQ(0u, cache_->cacheCount());
}

// ============================================================================
// PlayerSkinInfo 测试
// ============================================================================

class PlayerSkinInfoTest : public ::testing::Test {
protected:
    void SetUp() override {
        uuid_ = GameProfile::parseUUID("550e8400-e29b-41d4-a716-446655440000");
        profile_ = std::make_unique<GameProfile>(uuid_, "TestPlayer");
        info_ = std::make_unique<PlayerSkinInfo>(*profile_);
    }

    std::array<mc::u8, 16> uuid_;
    std::unique_ptr<GameProfile> profile_;
    std::unique_ptr<PlayerSkinInfo> info_;
};

TEST_F(PlayerSkinInfoTest, BasicProperties) {
    EXPECT_EQ("TestPlayer", info_->name());

    const auto& infoUuid = info_->uuid();
    for (size_t i = 0; i < 16; ++i) {
        EXPECT_EQ(uuid_[i], infoUuid[i]) << "UUID mismatch at index " << i;
    }
}

TEST_F(PlayerSkinInfoTest, LoadState) {
    EXPECT_EQ(SkinLoadState::NotLoaded, info_->loadState());

    info_->setLoadState(SkinLoadState::Loading);
    EXPECT_EQ(SkinLoadState::Loading, info_->loadState());
    EXPECT_TRUE(info_->isLoading());

    info_->setLoadState(SkinLoadState::Loaded);
    EXPECT_EQ(SkinLoadState::Loaded, info_->loadState());
    EXPECT_TRUE(info_->isLoaded());

    info_->setLoadState(SkinLoadState::Failed);
    EXPECT_TRUE(info_->isFailed());

    info_->setLoadState(SkinLoadState::UsingDefault);
    EXPECT_TRUE(info_->isUsingDefault());
}

TEST_F(PlayerSkinInfoTest, SkinTextures) {
    mc::ResourceLocation skinLoc("minecraft:skins/test");
    info_->setSkinLocation(skinLoc);

    EXPECT_TRUE(info_->getSkinLocation() == skinLoc);

    mc::ResourceLocation capeLoc("minecraft:capes/test");
    info_->setCapeLocation(capeLoc);

    auto cape = info_->getCapeLocation();
    EXPECT_TRUE(cape.has_value());
    EXPECT_EQ(capeLoc, cape.value());
}

TEST_F(PlayerSkinInfoTest, SkinType) {
    // 默认根据 UUID 确定
    SkinType type = info_->getSkinType();
    EXPECT_TRUE(type == SkinType::Default || type == SkinType::Slim);

    // 设置皮肤纹理后，使用纹理的类型
    SkinTextures textures;
    textures.setSkinType(SkinType::Slim);
    info_->setSkinTextures(textures);

    EXPECT_EQ(SkinType::Slim, info_->getSkinType());
}

TEST_F(PlayerSkinInfoTest, ModelParts) {
    mc::u8 parts = info_->modelParts();
    EXPECT_EQ(0x7F, parts);  // 默认显示所有部件（除披风外）

    // 设置特定部件
    info_->setModelPartEnabled(mc::PlayerModelPart::Cape, true);
    EXPECT_TRUE(info_->isWearing(mc::PlayerModelPart::Cape));

    info_->setModelPartEnabled(mc::PlayerModelPart::Cape, false);
    EXPECT_FALSE(info_->isWearing(mc::PlayerModelPart::Cape));

    // 批量设置
    info_->setModelParts(0x00);
    EXPECT_EQ(0x00, info_->modelParts());
}

TEST_F(PlayerSkinInfoTest, DefaultSkinLocation) {
    mc::ResourceLocation defaultSkin = info_->getDefaultSkinLocation();
    EXPECT_TRUE(defaultSkin.toString().find("steve") != mc::String::npos ||
                defaultSkin.toString().find("alex") != mc::String::npos);
}

// ============================================================================
// SkinPackets 测试
// ============================================================================

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
    EXPECT_TRUE(entry.displayName.has_value() == false);
}

TEST_F(SkinPacketsTest, PlayerListEntrySerialization) {
    std::array<mc::u8, 16> uuid = {
        0x55, 0x0e, 0x84, 0x00, 0xe2, 0x9b, 0x41, 0xd4,
        0xa7, 0x16, 0x44, 0x66, 0x55, 0x44, 0x00, 0x00
    };

    PlayerListEntry entry;
    entry.uuid = uuid;
    entry.name = "TestPlayer";
    entry.gameMode = mc::GameMode::Creative;
    entry.ping = 100;
    entry.properties.push_back({"textures", "base64data", "signature"});

    // 序列化
    mc::network::PacketSerializer ser;
    entry.serialize(ser, PlayerListAction::AddPlayer);

    // 反序列化
    auto buffer = ser.buffer();
    mc::network::PacketDeserializer deser(buffer.data(), buffer.size());

    auto result = PlayerListEntry::deserialize(deser, PlayerListAction::AddPlayer);
    ASSERT_TRUE(result.success());

    const PlayerListEntry& decoded = result.value();

    for (size_t i = 0; i < 16; ++i) {
        EXPECT_EQ(uuid[i], decoded.uuid[i]);
    }
    EXPECT_EQ("TestPlayer", decoded.name);
    EXPECT_EQ(mc::GameMode::Creative, decoded.gameMode);
    EXPECT_EQ(100, decoded.ping);
    EXPECT_EQ(1u, decoded.properties.size());
    EXPECT_EQ("textures", decoded.properties[0].name);
    EXPECT_EQ("base64data", decoded.properties[0].value);
}

TEST_F(SkinPacketsTest, PlayerListItemPacketAddPlayer) {
    PlayerListItemPacket packet(PlayerListAction::AddPlayer);

    std::array<mc::u8, 16> uuid = GameProfile::parseUUID("550e8400-e29b-41d4-a716-446655440000");
    GameProfile profile(uuid, "TestPlayer");
    profile.addProperty({"textures", "dGVzdA=="});  // base64 of "test"

    PlayerListEntry entry = PlayerListEntry::createAdd(profile, mc::GameMode::Survival, 50);
    packet.addEntry(entry);

    // 序列化
    auto serializeResult = packet.serialize();
    ASSERT_TRUE(serializeResult.success());

    // 反序列化
    PlayerListItemPacket decoded;
    auto deserializeResult = decoded.deserialize(
        serializeResult.value().data(),
        serializeResult.value().size()
    );
    ASSERT_TRUE(deserializeResult.success());

    EXPECT_EQ(PlayerListAction::AddPlayer, decoded.action());
    EXPECT_EQ(1u, decoded.entries().size());

    const PlayerListEntry& decodedEntry = decoded.entries()[0];
    for (size_t i = 0; i < 16; ++i) {
        EXPECT_EQ(uuid[i], decodedEntry.uuid[i]);
    }
    EXPECT_EQ("TestPlayer", decodedEntry.name);
    EXPECT_EQ(mc::GameMode::Survival, decodedEntry.gameMode);
    EXPECT_EQ(50, decodedEntry.ping);
}

TEST_F(SkinPacketsTest, PlayerListItemPacketRemovePlayer) {
    PlayerListItemPacket packet(PlayerListAction::RemovePlayer);

    std::array<mc::u8, 16> uuid = GameProfile::parseUUID("550e8400-e29b-41d4-a716-446655440000");

    PlayerListEntry entry = PlayerListEntry::createRemove(uuid);
    packet.addEntry(entry);

    // 序列化
    auto serializeResult = packet.serialize();
    ASSERT_TRUE(serializeResult.success());

    // 反序列化
    PlayerListItemPacket decoded;
    auto deserializeResult = decoded.deserialize(
        serializeResult.value().data(),
        serializeResult.value().size()
    );
    ASSERT_TRUE(deserializeResult.success());

    EXPECT_EQ(PlayerListAction::RemovePlayer, decoded.action());
    EXPECT_EQ(1u, decoded.entries().size());

    const PlayerListEntry& decodedEntry = decoded.entries()[0];
    for (size_t i = 0; i < 16; ++i) {
        EXPECT_EQ(uuid[i], decodedEntry.uuid[i]);
    }
}

TEST_F(SkinPacketsTest, PlayerListItemPacketMultipleEntries) {
    PlayerListItemPacket packet(PlayerListAction::AddPlayer);

    std::array<mc::u8, 16> uuid1 = GameProfile::parseUUID("550e8400-e29b-41d4-a716-446655440000");
    std::array<mc::u8, 16> uuid2 = GameProfile::parseUUID("550e8400-e29b-41d4-a716-446655440001");

    GameProfile profile1(uuid1, "Player1");
    GameProfile profile2(uuid2, "Player2");

    packet.addEntry(PlayerListEntry::createAdd(profile1, mc::GameMode::Survival, 10));
    packet.addEntry(PlayerListEntry::createAdd(profile2, mc::GameMode::Creative, 20));

    // 序列化
    auto serializeResult = packet.serialize();
    ASSERT_TRUE(serializeResult.success());

    // 反序列化
    PlayerListItemPacket decoded;
    auto deserializeResult = decoded.deserialize(
        serializeResult.value().data(),
        serializeResult.value().size()
    );
    ASSERT_TRUE(deserializeResult.success());

    EXPECT_EQ(PlayerListAction::AddPlayer, decoded.action());
    EXPECT_EQ(2u, decoded.entries().size());

    EXPECT_EQ("Player1", decoded.entries()[0].name);
    EXPECT_EQ(mc::GameMode::Survival, decoded.entries()[0].gameMode);
    EXPECT_EQ(10, decoded.entries()[0].ping);

    EXPECT_EQ("Player2", decoded.entries()[1].name);
    EXPECT_EQ(mc::GameMode::Creative, decoded.entries()[1].gameMode);
    EXPECT_EQ(20, decoded.entries()[1].ping);
}

TEST_F(SkinPacketsTest, PlayerListEntryUpdateLatency) {
    std::array<mc::u8, 16> uuid = GameProfile::parseUUID("550e8400-e29b-41d4-a716-446655440000");

    PlayerListEntry entry = PlayerListEntry::createUpdateLatency(uuid, 200);
    EXPECT_EQ(200, entry.ping);

    // 序列化
    mc::network::PacketSerializer ser;
    entry.serialize(ser, PlayerListAction::UpdateLatency);

    auto buffer = ser.buffer();
    mc::network::PacketDeserializer deser(buffer.data(), buffer.size());

    auto result = PlayerListEntry::deserialize(deser, PlayerListAction::UpdateLatency);
    ASSERT_TRUE(result.success());

    const PlayerListEntry& decoded = result.value();
    for (size_t i = 0; i < 16; ++i) {
        EXPECT_EQ(uuid[i], decoded.uuid[i]);
    }
    EXPECT_EQ(200, decoded.ping);
}

TEST_F(SkinPacketsTest, PlayerListEntryUpdateGameMode) {
    std::array<mc::u8, 16> uuid = GameProfile::parseUUID("550e8400-e29b-41d4-a716-446655440000");

    PlayerListEntry entry = PlayerListEntry::createUpdateGameMode(uuid, mc::GameMode::Spectator);
    EXPECT_EQ(mc::GameMode::Spectator, entry.gameMode);

    // 序列化
    mc::network::PacketSerializer ser;
    entry.serialize(ser, PlayerListAction::UpdateGameMode);

    auto buffer = ser.buffer();
    mc::network::PacketDeserializer deser(buffer.data(), buffer.size());

    auto result = PlayerListEntry::deserialize(deser, PlayerListAction::UpdateGameMode);
    ASSERT_TRUE(result.success());

    const PlayerListEntry& decoded = result.value();
    EXPECT_EQ(mc::GameMode::Spectator, decoded.gameMode);
}
