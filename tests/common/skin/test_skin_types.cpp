#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/skin/core/GameProfile.hpp"
#include "common/skin/core/SkinTextures.hpp"
#include "common/skin/core/SkinTypes.hpp"
#include <array>
#include <gtest/gtest.h>

using namespace mc::skin;

class SkinTypesTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(SkinTypesTest, ParseSkinType)
{
    EXPECT_EQ(SkinType::Default, parseSkinType("default"));
    EXPECT_EQ(SkinType::Slim, parseSkinType("slim"));
    EXPECT_EQ(SkinType::Default, parseSkinType("DEFAULT"));
    EXPECT_EQ(SkinType::Default, parseSkinType("unknown"));
    EXPECT_EQ(SkinType::Default, parseSkinType(""));
}

TEST_F(SkinTypesTest, SkinTypeToString)
{
    EXPECT_EQ("default", skinTypeToString(SkinType::Default));
    EXPECT_EQ("slim", skinTypeToString(SkinType::Slim));
}

TEST_F(SkinTypesTest, GetDefaultSkinTypeForUUID)
{
    std::array<mc::u8, 16> zeroUUID = {};
    SkinType zeroType = getDefaultSkinTypeForUUID(zeroUUID);
    EXPECT_TRUE(zeroType == SkinType::Default || zeroType == SkinType::Slim);

    std::array<mc::u8, 16> testUUID1 = {
        0x55, 0x0e, 0x84, 0x00, 0xe2, 0x9b, 0x41, 0xd4, 0xa7, 0x16, 0x44, 0x66, 0x55, 0x44, 0x00, 0x00};
    SkinType type1 = getDefaultSkinTypeForUUID(testUUID1);
    EXPECT_TRUE(type1 == SkinType::Default || type1 == SkinType::Slim);
}

TEST_F(SkinTypesTest, CalculateUUIDHashCode)
{
    std::array<mc::u8, 16> uuid = {
        0x55, 0x0e, 0x84, 0x00, 0xe2, 0x9b, 0x41, 0xd4, 0xa7, 0x16, 0x44, 0x66, 0x55, 0x44, 0x00, 0x00};

    mc::i32 hash1 = calculateUUIDHashCode(uuid);
    mc::i32 hash2 = calculateUUIDHashCode(uuid);
    EXPECT_EQ(hash1, hash2);
}

// ============================================================================
// SkinTextures 测试
// ============================================================================

class SkinTexturesTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(SkinTexturesTest, DefaultConstruction)
{
    SkinTextures textures;
    EXPECT_FALSE(textures.hasSkin());
    EXPECT_FALSE(textures.hasCape());
    EXPECT_FALSE(textures.hasElytra());
    EXPECT_FALSE(textures.hasAnyTexture());
    EXPECT_EQ(SkinType::Default, textures.skinType());
}

TEST_F(SkinTexturesTest, SetSkin)
{
    SkinTextures textures;

    mc::ResourceLocation location("minecraft:skins/test");
    textures.setSkin(location);

    EXPECT_TRUE(textures.hasSkin());
    EXPECT_TRUE(textures.hasAnyTexture());
    EXPECT_TRUE(textures.getSkin().has_value());
    EXPECT_EQ(location, textures.getSkin().value());
}

TEST_F(SkinTexturesTest, SetCape)
{
    SkinTextures textures;

    mc::ResourceLocation location("minecraft:capes/test");
    textures.setCape(location);

    EXPECT_TRUE(textures.hasCape());
    EXPECT_TRUE(textures.hasAnyTexture());
    EXPECT_TRUE(textures.getCape().has_value());
    EXPECT_EQ(location, textures.getCape().value());
}

TEST_F(SkinTexturesTest, SetSkinType)
{
    SkinTextures textures;

    textures.setSkinType(SkinType::Slim);
    EXPECT_EQ(SkinType::Slim, textures.skinType());

    textures.setSkinType(SkinType::Default);
    EXPECT_EQ(SkinType::Default, textures.skinType());
}

TEST_F(SkinTexturesTest, URLsAndHashes)
{
    SkinTextures textures;

    textures.setSkinUrl("http://textures.minecraft.net/texture/abc123");
    textures.setSkinHash("abc123");

    EXPECT_TRUE(textures.skinUrl().has_value());
    EXPECT_EQ("http://textures.minecraft.net/texture/abc123", textures.skinUrl().value());
    EXPECT_TRUE(textures.skinHash().has_value());
    EXPECT_EQ("abc123", textures.skinHash().value());
}

TEST_F(SkinTexturesTest, ExtractHashFromUrl)
{
    std::string url = "http://textures.minecraft.net/texture/abc123def456";
    std::string hash = SkinTextures::extractHashFromUrl(url);
    EXPECT_EQ("abc123def456", hash);

    std::string urlWithQuery = "http://textures.minecraft.net/texture/abc123?param=value";
    std::string hashWithQuery = SkinTextures::extractHashFromUrl(urlWithQuery);
    EXPECT_EQ("abc123", hashWithQuery);
}
