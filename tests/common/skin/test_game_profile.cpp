#include <gtest/gtest.h>
#include "common/skin/core/GameProfile.hpp"
#include "common/skin/core/SkinTypes.hpp"
#include <array>

using namespace mc::skin;

class GameProfileTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(GameProfileTest, DefaultConstruction) {
    GameProfile profile;
    EXPECT_FALSE(profile.hasValidUUID());
    EXPECT_TRUE(profile.name().empty());
    EXPECT_TRUE(profile.properties().empty());
}

TEST_F(GameProfileTest, UUIDConstruction) {
    std::array<mc::u8, 16> uuid = {
        0x55, 0x0e, 0x84, 0x00, 0xe2, 0x9b, 0x41, 0xd4,
        0xa7, 0x16, 0x44, 0x66, 0x55, 0x44, 0x00, 0x00
    };

    GameProfile profile(uuid, "TestPlayer");
    EXPECT_TRUE(profile.hasValidUUID());
    EXPECT_EQ("TestPlayer", profile.name());

    for (size_t i = 0; i < 16; ++i) {
        EXPECT_EQ(uuid[i], profile.uuid()[i]);
    }
}

TEST_F(GameProfileTest, UUIDToString) {
    std::array<mc::u8, 16> uuid = {
        0x55, 0x0e, 0x84, 0x00, 0xe2, 0x9b, 0x41, 0xd4,
        0xa7, 0x16, 0x44, 0x66, 0x55, 0x44, 0x00, 0x00
    };

    GameProfile profile(uuid, "Test");
    mc::std::string uuidStr = profile.uuidToString();

    EXPECT_EQ("550e8400-e29b-41d4-a716-446655440000", uuidStr);
}

TEST_F(GameProfileTest, UUIDToStringNoDashes) {
    std::array<mc::u8, 16> uuid = {
        0x55, 0x0e, 0x84, 0x00, 0xe2, 0x9b, 0x41, 0xd4,
        0xa7, 0x16, 0x44, 0x66, 0x55, 0x44, 0x00, 0x00
    };

    GameProfile profile(uuid, "Test");
    mc::std::string uuidStr = profile.uuidToStringNoDashes();

    EXPECT_EQ("550e8400e29b41d4a716446655440000", uuidStr);
}

TEST_F(GameProfileTest, ParseUUID) {
    mc::std::string uuidStr1 = "550e8400-e29b-41d4-a716-446655440000";
    auto uuid1 = GameProfile::parseUUID(uuidStr1);

    std::array<mc::u8, 16> expected = {
        0x55, 0x0e, 0x84, 0x00, 0xe2, 0x9b, 0x41, 0xd4,
        0xa7, 0x16, 0x44, 0x66, 0x55, 0x44, 0x00, 0x00
    };

    for (size_t i = 0; i < 16; ++i) {
        EXPECT_EQ(expected[i], uuid1[i]);
    }

    // Test without dashes
    mc::std::string uuidStr2 = "550e8400e29b41d4a716446655440000";
    auto uuid2 = GameProfile::parseUUID(uuidStr2);

    for (size_t i = 0; i < 16; ++i) {
        EXPECT_EQ(expected[i], uuid2[i]);
    }
}

TEST_F(GameProfileTest, ParseInvalidUUID) {
    mc::std::string invalidStr = "invalid-uuid";
    auto uuid = GameProfile::parseUUID(invalidStr);

    // Should return all zeros
    for (size_t i = 0; i < 16; ++i) {
        EXPECT_EQ(0, uuid[i]);
    }
}

TEST_F(GameProfileTest, PropertyManagement) {
    GameProfile profile;

    profile.addProperty({"textures", "base64data", "signature"});
    EXPECT_TRUE(profile.hasTextures());
    EXPECT_EQ(1u, profile.properties().size());

    const GameProfileProperty* prop = profile.getTexturesProperty();
    ASSERT_NE(nullptr, prop);
    EXPECT_EQ("textures", prop->name);
    EXPECT_EQ("base64data", prop->value);
    EXPECT_TRUE(prop->hasSignature());
    EXPECT_EQ("signature", prop->signature.value());

    const GameProfileProperty* notFound = profile.getProperty("nonexistent");
    EXPECT_EQ(nullptr, notFound);

    profile.addProperty({"textures", "newvalue"});
    prop = profile.getTexturesProperty();
    ASSERT_NE(nullptr, prop);
    EXPECT_EQ("newvalue", prop->value);
    EXPECT_FALSE(prop->hasSignature());
}

TEST_F(GameProfileTest, UUIDHashCode) {
    std::array<mc::u8, 16> uuid = {
        0x55, 0x0e, 0x84, 0x00, 0xe2, 0x9b, 0x41, 0xd4,
        0xa7, 0x16, 0x44, 0x66, 0x55, 0x44, 0x00, 0x00
    };

    GameProfile profile(uuid, "Test");
    mc::i32 hash1 = profile.uuidHashCode();
    mc::i32 hash2 = profile.uuidHashCode();

    EXPECT_EQ(hash1, hash2);
}

TEST_F(GameProfileTest, SetName) {
    GameProfile profile;
    profile.setName("Player1");
    EXPECT_EQ("Player1", profile.name());
}

TEST_F(GameProfileTest, SetUUID) {
    std::array<mc::u8, 16> uuid = {
        0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
        0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10
    };

    GameProfile profile;
    profile.setUUID(uuid);
    EXPECT_TRUE(profile.hasValidUUID());

    for (size_t i = 0; i < 16; ++i) {
        EXPECT_EQ(uuid[i], profile.uuid()[i]);
    }
}
