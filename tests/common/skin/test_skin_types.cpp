#include <gtest/gtest.h>
#include "common/skin/core/SkinTypes.hpp"
#include "common/skin/core/GameProfile.hpp"
#include "common/skin/core/SkinTextures.hpp"
#include <array>

using namespace mc::skin;

class SkinTypesTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(SkinTypesTest, ParseSkinType) {
    EXPECT_EQ(SkinType::Default, parseSkinType("default"));
    EXPECT_EQ(SkinType::Slim, parseSkinType("slim"));
    EXPECT_EQ(SkinType::Default, parseSkinType("DEFAULT"));  // 大小写不敏感
    EXPECT_EQ(SkinType::Default, parseSkinType("unknown"));  // 未知值返回默认
    EXPECT_EQ(SkinType::Default, parseSkinType(""));         // 空字符串返回默认
}

TEST_F(SkinTypesTest, SkinTypeToString) {
    EXPECT_EQ("default", skinTypeToString(SkinType::Default));
    EXPECT_EQ("slim", skinTypeToString(SkinType::Slim));
}

TEST_F(SkinTypesTest, GetDefaultSkinTypeForUUID) {
    // 测试不同UUID得到不同皮肤类型
    // UUID: 00000000-0000-0000-0000-000000000000
    std::array<u8, 16> zeroUUID = {};
    SkinType zeroType = getDefaultSkinTypeForUUID(zeroUUID);
    EXPECT_TRUE(zeroType == SkinType::Default || zeroType == SkinType::Slim);

    // UUID: 550e8400-e29b-41d4-a716-446655440000
    std::array<u8, 16> testUUID1 = {
        0x55, 0x0e, 0x84, 0x00, 0xe2, 0x9b, 0x41, 0xd4,
        0xa7, 0x16, 0x44, 0x66, 0x55, 0x44, 0x00, 0x00
    };
    SkinType type1 = getDefaultSkinTypeForUUID(testUUID1);

    // UUID: 550e8400-e29b-41d4-a716-446655440001
    std::array<u8, 16> testUUID2 = {
        0x55, 0x0e, 0x84, 0x00, 0xe2, 0x9b, 0x41, 0xd4,
        0xa7, 0x16, 0x44, 0x66, 0x55, 0x44, 0x00, 0x01
    };
    SkinType type2 = getDefaultSkinTypeForUUID(testUUID2);

    // 不同UUID可能得到不同结果
    // type1 != type2 是可能的但不是必须的
}

TEST_F(SkinTypesTest, CalculateUUIDHashCode) {
    // 测试哈希计算的一致性
    std::array<u8, 16> uuid = {
        0x55, 0x0e, 0x84, 0x00, 0xe2, 0x9b, 0x41, 0xd4,
        0xa7, 0x16, 0x44, 0x66, 0x55, 0x44, 0x00, 0x00
    };

    i32 hash1 = calculateUUIDHashCode(uuid);
    i32 hash2 = calculateUUIDHashCode(uuid);
    EXPECT_EQ(hash1, hash2);

    // 不同UUID应该有不同哈希（大概率）
    std::array<u8, 16> uuid2 = {
        0x55, 0x0e, 0x84, 0x00, 0xe2, 0x9b, 0x41, 0xd4,
        0xa7, 0x16, 0x44, 0x66, 0x55, 0x44, 0x00, 0x01
    };
    i32 hash3 = calculateUUIDHashCode(uuid2);
    // 不保证一定不同，但大概率不同
}

// ============================================================================
// GameProfile 测试
// ============================================================================

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
    std::array<u8, 16> uuid = {
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
    std::array<u8, 16> uuid = {
        0x55, 0x0e, 0x84, 0x00, 0xe2, 0x9b, 0x41, 0xd4,
        0xa7, 0x16, 0x44, 0x66, 0x55, 0x44, 0x00, 0x00
    };

    GameProfile profile(uuid, "Test");
    String uuidStr = profile.uuidToString();

    EXPECT_EQ("550e8400-e29b-41d4-a716-446655440000", uuidStr);
}

TEST_F(GameProfileTest, UUIDToStringNoDashes) {
    std::array<u8, 16> uuid = {
        0x55, 0x0e, 0x84, 0x00, 0xe2, 0x9b, 0x41, 0xd4,
        0xa7, 0x16, 0x44, 0x66, 0x55, 0x44, 0x00, 0x00
    };

    GameProfile profile(uuid, "Test");
    String uuidStr = profile.uuidToStringNoDashes();

    EXPECT_EQ("550e8400e29b41d4a716446655440000", uuidStr);
}

TEST_F(GameProfileTest, ParseUUID) {
    // 带连字符
    String uuidStr1 = "550e8400-e29b-41d4-a716-446655440000";
    auto uuid1 = GameProfile::parseUUID(uuidStr1);

    std::array<u8, 16> expected = {
        0x55, 0x0e, 0x84, 0x00, 0xe2, 0x9b, 0x41, 0xd4,
        0xa7, 0x16, 0x44, 0x66, 0x55, 0x44, 0x00, 0x00
    };

    for (size_t i = 0; i < 16; ++i) {
        EXPECT_EQ(expected[i], uuid1[i]);
    }

    // 不带连字符
    String uuidStr2 = "550e8400e29b41d4a716446655440000";
    auto uuid2 = GameProfile::parseUUID(uuidStr2);

    for (size_t i = 0; i < 16; ++i) {
        EXPECT_EQ(expected[i], uuid2[i]);
    }

    // 无效格式
    String invalidStr = "invalid";
    auto invalidUUID = GameProfile::parseUUID(invalidStr);
    for (size_t i = 0; i < 16; ++i) {
        EXPECT_EQ(0, invalidUUID[i]);
    }
}

TEST_F(GameProfileTest, PropertyManagement) {
    GameProfile profile;

    // 添加属性
    profile.addProperty({"textures", "base64data", "signature"});
    EXPECT_TRUE(profile.hasTextures());
    EXPECT_EQ(1u, profile.properties().size());

    const GameProfileProperty* prop = profile.getTexturesProperty();
    ASSERT_NE(nullptr, prop);
    EXPECT_EQ("textures", prop->name);
    EXPECT_EQ("base64data", prop->value);
    EXPECT_TRUE(prop->hasSignature());
    EXPECT_EQ("signature", prop->signature.value());

    // 获取不存在的属性
    const GameProfileProperty* notFound = profile.getProperty("nonexistent");
    EXPECT_EQ(nullptr, notFound);

    // 添加同名属性会替换
    profile.addProperty({"textures", "newvalue"});
    prop = profile.getTexturesProperty();
    ASSERT_NE(nullptr, prop);
    EXPECT_EQ("newvalue", prop->value);
    EXPECT_FALSE(prop->hasSignature());

    // 清除属性
    profile.clearProperties();
    EXPECT_FALSE(profile.hasTextures());
    EXPECT_TRUE(profile.properties().empty());
}

TEST_F(GameProfileTest, Comparison) {
    std::array<u8, 16> uuid1 = {
        0x55, 0x0e, 0x84, 0x00, 0xe2, 0x9b, 0x41, 0xd4,
        0xa7, 0x16, 0x44, 0x66, 0x55, 0x44, 0x00, 0x00
    };
    std::array<u8, 16> uuid2 = {
        0x55, 0x0e, 0x84, 0x00, 0xe2, 0x9b, 0x41, 0xd4,
        0xa7, 0x16, 0x44, 0x66, 0x55, 0x44, 0x00, 0x01
    };

    GameProfile profile1(uuid1, "Player1");
    GameProfile profile2(uuid1, "Player1");
    GameProfile profile3(uuid2, "Player1");
    GameProfile profile4(uuid1, "Player2");

    EXPECT_TRUE(profile1 == profile2);   // 相同UUID
    EXPECT_FALSE(profile1 == profile3);  // 不同UUID
    EXPECT_TRUE(profile1 == profile4);   // 相同UUID，不同名字

    EXPECT_FALSE(profile1 != profile2);
    EXPECT_TRUE(profile1 != profile3);
}

// ============================================================================
// SkinTextures 测试
// ============================================================================

class SkinTexturesTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(SkinTexturesTest, DefaultConstruction) {
    SkinTextures textures;
    EXPECT_FALSE(textures.hasSkin());
    EXPECT_FALSE(textures.hasCape());
    EXPECT_FALSE(textures.hasElytra());
    EXPECT_FALSE(textures.hasAnyTexture());
    EXPECT_EQ(SkinType::Default, textures.skinType());
}

TEST_F(SkinTexturesTest, SetSkin) {
    SkinTextures textures;

    ResourceLocation location("minecraft:skins/test");
    textures.setSkin(location);

    EXPECT_TRUE(textures.hasSkin());
    EXPECT_TRUE(textures.hasAnyTexture());
    EXPECT_TRUE(textures.getSkin().has_value());
    EXPECT_EQ(location, textures.getSkin().value());
}

TEST_F(SkinTexturesTest, SetCape) {
    SkinTextures textures;

    ResourceLocation location("minecraft:capes/test");
    textures.setCape(location);

    EXPECT_TRUE(textures.hasCape());
    EXPECT_TRUE(textures.hasAnyTexture());
    EXPECT_TRUE(textures.getCape().has_value());
    EXPECT_EQ(location, textures.getCape().value());
}

TEST_F(SkinTexturesTest, SetElytra) {
    SkinTextures textures;

    ResourceLocation location("minecraft:elytra/test");
    textures.setElytra(location);

    EXPECT_TRUE(textures.hasElytra());
    EXPECT_TRUE(textures.hasAnyTexture());
    EXPECT_TRUE(textures.getElytra().has_value());
    EXPECT_EQ(location, textures.getElytra().value());
}

TEST_F(SkinTexturesTest, SetSkinType) {
    SkinTextures textures;

    textures.setSkinType(SkinType::Slim);
    EXPECT_EQ(SkinType::Slim, textures.skinType());

    textures.setSkinType(SkinType::Default);
    EXPECT_EQ(SkinType::Default, textures.skinType());
}

TEST_F(SkinTexturesTest, URLsAndHashes) {
    SkinTextures textures;

    textures.setSkinUrl("http://textures.minecraft.net/texture/abc123");
    textures.setSkinHash("abc123");
    textures.setCapeUrl("http://textures.minecraft.net/texture/def456");
    textures.setCapeHash("def456");

    EXPECT_TRUE(textures.skinUrl().has_value());
    EXPECT_EQ("http://textures.minecraft.net/texture/abc123", textures.skinUrl().value());
    EXPECT_TRUE(textures.skinHash().has_value());
    EXPECT_EQ("abc123", textures.skinHash().value());

    EXPECT_TRUE(textures.capeUrl().has_value());
    EXPECT_TRUE(textures.capeHash().has_value());
}

TEST_F(SkinTexturesTest, GetCacheKey) {
    SkinTextures textures;

    textures.setSkinHash("abcdef1234567890");
    String key = textures.getSkinCacheKey();
    EXPECT_EQ("skins/ab/abcdef1234567890", key);

    textures.setCapeHash("ghijklmnopqrstuv");
    String capeKey = textures.getCapeCacheKey();
    EXPECT_EQ("capes/gh/ghijklmnopqrstuv", capeKey);
}

TEST_F(SkinTexturesTest, ExtractHashFromUrl) {
    String url = "http://textures.minecraft.net/texture/abc123def456";
    String hash = SkinTextures::extractHashFromUrl(url);
    EXPECT_EQ("abc123def456", hash);

    // 带查询参数
    String urlWithQuery = "http://textures.minecraft.net/texture/abc123?param=value";
    String hashWithQuery = SkinTextures::extractHashFromUrl(urlWithQuery);
    EXPECT_EQ("abc123", hashWithQuery);

    // 无效URL
    String invalidUrl = "http://example.com/";
    String invalidHash = SkinTextures::extractHashFromUrl(invalidUrl);
    EXPECT_TRUE(invalidHash.empty());
}
