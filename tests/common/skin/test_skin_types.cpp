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
    // zeroUUID 的 hashCode = 0，floorMod(0, 18) = 0，索引 0 是 slim/alex
    std::array<mc::u8, 16> zeroUUID = {};
    SkinType zeroType = getDefaultSkinTypeForUUID(zeroUUID);
    EXPECT_EQ(zeroType, SkinType::Slim);

    // 确保结果与 getDefaultSkinVariantForUUID 一致
    const DefaultSkinVariant& zeroVariant = getDefaultSkinVariantForUUID(zeroUUID);
    EXPECT_EQ(zeroType, zeroVariant.skinType);

    std::array<mc::u8, 16> testUUID1 = {
        0x55, 0x0e, 0x84, 0x00, 0xe2, 0x9b, 0x41, 0xd4, 0xa7, 0x16, 0x44, 0x66, 0x55, 0x44, 0x00, 0x00};
    SkinType type1 = getDefaultSkinTypeForUUID(testUUID1);
    const DefaultSkinVariant& variant1 = getDefaultSkinVariantForUUID(testUUID1);
    EXPECT_EQ(type1, variant1.skinType);
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
// 18 种默认皮肤测试
// ============================================================================

TEST_F(SkinTypesTest, DefaultSkinVariantsCount)
{
    const auto& variants = getDefaultSkinVariants();
    EXPECT_EQ(variants.size(), DEFAULT_SKIN_COUNT);
    EXPECT_EQ(variants.size(), 18u);
}

TEST_F(SkinTypesTest, DefaultSkinVariantsOrder)
{
    const auto& variants = getDefaultSkinVariants();

    // 前 9 个应为 slim
    for (size_t i = 0; i < 9; ++i) {
        EXPECT_EQ(variants[i].skinType, SkinType::Slim) << "Variant " << i << " should be slim";
        EXPECT_EQ(variants[i].index, i);
    }

    // 后 9 个应为 wide (Default)
    for (size_t i = 9; i < 18; ++i) {
        EXPECT_EQ(variants[i].skinType, SkinType::Default) << "Variant " << i << " should be wide";
        EXPECT_EQ(variants[i].index, i);
    }
}

TEST_F(SkinTypesTest, DefaultSkinVariantsNames)
{
    const auto& variants = getDefaultSkinVariants();

    // 9 个皮肤名称（slim 和 wide 使用相同名称）
    const char* expectedNames[] = {"alex", "ari", "efe", "kai", "makena", "noor", "steve", "sunny", "zuri"};

    for (size_t i = 0; i < 9; ++i) {
        EXPECT_STREQ(variants[i].name, expectedNames[i]) << "Slim variant " << i;
        EXPECT_STREQ(variants[i + 9].name, expectedNames[i]) << "Wide variant " << i;
    }
}

TEST_F(SkinTypesTest, DefaultSkinVariantTextureLocations)
{
    const auto& variants = getDefaultSkinVariants();

    // 检查 slim 变体路径
    EXPECT_EQ(variants[0].textureLocation(), mc::ResourceLocation("minecraft:textures/entity/player/slim/alex.png"));
    EXPECT_EQ(variants[6].textureLocation(), mc::ResourceLocation("minecraft:textures/entity/player/slim/steve.png"));

    // 检查 wide 变体路径
    EXPECT_EQ(variants[9].textureLocation(), mc::ResourceLocation("minecraft:textures/entity/player/wide/alex.png"));
    EXPECT_EQ(variants[15].textureLocation(), mc::ResourceLocation("minecraft:textures/entity/player/wide/steve.png"));
}

TEST_F(SkinTypesTest, GetDefaultSkinVariantForUUID)
{
    std::array<mc::u8, 16> zeroUUID = {};
    const DefaultSkinVariant& variant = getDefaultSkinVariantForUUID(zeroUUID);

    // zeroUUID 的 hashCode 为 0，floorMod(0, 18) = 0，所以应选择索引 0 (slim/alex)
    EXPECT_EQ(variant.index, 0u);
    EXPECT_EQ(variant.skinType, SkinType::Slim);
    EXPECT_STREQ(variant.name, "alex");
}

TEST_F(SkinTypesTest, GetDefaultSkinVariantForUUIDDeterministic)
{
    std::array<mc::u8, 16> uuid = {
        0x55, 0x0e, 0x84, 0x00, 0xe2, 0x9b, 0x41, 0xd4, 0xa7, 0x16, 0x44, 0x66, 0x55, 0x44, 0x00, 0x00};

    const DefaultSkinVariant& v1 = getDefaultSkinVariantForUUID(uuid);
    const DefaultSkinVariant& v2 = getDefaultSkinVariantForUUID(uuid);
    EXPECT_EQ(v1.index, v2.index);
    EXPECT_STREQ(v1.name, v2.name);
    EXPECT_EQ(v1.skinType, v2.skinType);
}

TEST_F(SkinTypesTest, GetDefaultSkinVariantForUUIDNegativeHash)
{
    // 构造一个产生负 hashCode 的 UUID，验证 floorMod 行为
    // hashCode = mostHigh ^ mostLow ^ leastHigh ^ leastLow
    // 如果 mostSigBits = 0x8000000000000000，则 mostHigh = 0x80000000（负数）
    std::array<mc::u8, 16> negativeUUID = {};
    negativeUUID[0] = 0x80; // 设置 mostSigBits 的最高位
    const DefaultSkinVariant& variant = getDefaultSkinVariantForUUID(negativeUUID);

    // 验证索引在有效范围内 [0, 17]
    EXPECT_LT(variant.index, DEFAULT_SKIN_COUNT);
}

TEST_F(SkinTypesTest, GetCanonicalDefaultSkin)
{
    const DefaultSkinVariant& canonical = getCanonicalDefaultSkin();
    EXPECT_EQ(canonical.index, 6u);
    EXPECT_EQ(canonical.skinType, SkinType::Slim);
    EXPECT_STREQ(canonical.name, "steve");
}

TEST_F(SkinTypesTest, GetDefaultSkinVariantConsistentWithType)
{
    // 验证 getDefaultSkinVariantForUUID 与 getDefaultSkinTypeForUUID 一致
    std::array<mc::u8, 16> uuid1 = {
        0x55, 0x0e, 0x84, 0x00, 0xe2, 0x9b, 0x41, 0xd4, 0xa7, 0x16, 0x44, 0x66, 0x55, 0x44, 0x00, 0x00};
    std::array<mc::u8, 16> uuid2 = {
        0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef, 0xfe, 0xdc, 0xba, 0x98, 0x76, 0x54, 0x32, 0x10};

    const DefaultSkinVariant& variant1 = getDefaultSkinVariantForUUID(uuid1);
    SkinType type1 = getDefaultSkinTypeForUUID(uuid1);
    EXPECT_EQ(variant1.skinType, type1);

    const DefaultSkinVariant& variant2 = getDefaultSkinVariantForUUID(uuid2);
    SkinType type2 = getDefaultSkinTypeForUUID(uuid2);
    EXPECT_EQ(variant2.skinType, type2);
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
