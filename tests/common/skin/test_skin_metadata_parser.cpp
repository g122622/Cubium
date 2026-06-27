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

#include "common/skin/core/GameProfile.hpp"
#include "common/skin/core/SkinTextures.hpp"
#include "common/skin/core/SkinTypes.hpp"
#include "common/skin/parser/SkinMetadataParser.hpp"
#include <cstdint>
#include <string>
#include <gtest/gtest.h>

using namespace mc;
using namespace mc::skin;

// ============================================================================
// 辅助函数
// ============================================================================

/**
 * @brief 创建标准的 textures 属性 JSON 并进行 Base64 编码
 *
 * @param skinUrl 皮肤 URL（空字符串表示不包含 SKIN）
 * @param skinModel 皮肤模型类型（"default" 或 "slim"）
 * @param capeUrl 披风 URL（空字符串表示不包含 CAPE）
 * @param elytraUrl 鞘翅 URL（空字符串表示不包含 ELYTRA）
 * @return Base64 编码的 textures 属性值
 */
static std::string makeTexturesBase64(const std::string& skinUrl,
    const std::string& skinModel = "default",
    const std::string& capeUrl = "",
    const std::string& elytraUrl = "")
{
    // 手动构建 JSON 字符串
    std::string json = R"({"textures":{)";

    bool first = true;

    if (!skinUrl.empty()) {
        json += "\"SKIN\":{\"url\":\"" + skinUrl + "\"";
        if (skinModel == "slim") {
            json += ",\"metadata\":{\"model\":\"slim\"}";
        }
        json += "}";
        first = false;
    }

    if (!capeUrl.empty()) {
        if (!first) json += ",";
        json += "\"CAPE\":{\"url\":\"" + capeUrl + "\"}";
        first = false;
    }

    if (!elytraUrl.empty()) {
        if (!first) json += ",";
        json += "\"ELYTRA\":{\"url\":\"" + elytraUrl + "\"}";
        first = false;
    }

    json += "}}";

    // Base64 编码
    static const char base64Chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string result;
    result.reserve(((json.size() + 2) / 3) * 4);

    int i = 0;
    int len = static_cast<int>(json.size());
    while (i < len) {
        u32 octet_a = i < len ? static_cast<u8>(json[i++]) : 0;
        u32 octet_b = i < len ? static_cast<u8>(json[i++]) : 0;
        u32 octet_c = i < len ? static_cast<u8>(json[i++]) : 0;

        u32 triple = (octet_a << 16) | (octet_b << 8) | octet_c;

        result += base64Chars[(triple >> 18) & 0x3F];
        result += base64Chars[(triple >> 12) & 0x3F];
        result += (i - 2 < len) ? base64Chars[(triple >> 6) & 0x3F] : '=';
        result += (i - 1 < len) ? base64Chars[triple & 0x3F] : '=';
    }

    return result;
}

// ============================================================================
// SkinMetadataParser 测试
// ============================================================================

class SkinMetadataParserTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

// ---------- parse 测试 ----------

TEST_F(SkinMetadataParserTest, ParseRejectsNonTexturesProperty)
{
    GameProfileProperty prop("not_textures", "dGVzdA=="); // "test" in base64
    auto result = SkinMetadataParser::parse(prop);
    EXPECT_FALSE(result.success());
}

TEST_F(SkinMetadataParserTest, ParseSkinOnly)
{
    std::string base64 = makeTexturesBase64("http://textures.minecraft.net/texture/abc123");
    GameProfileProperty prop("textures", base64);
    auto result = SkinMetadataParser::parse(prop);
    ASSERT_TRUE(result.success());

    SkinTextures textures = result.value();
    EXPECT_TRUE(textures.hasSkin());
    EXPECT_FALSE(textures.hasCape());
    EXPECT_FALSE(textures.hasElytra());
    EXPECT_EQ(textures.skinUrl().value_or(""), "http://textures.minecraft.net/texture/abc123");
    EXPECT_EQ(textures.skinHash().value_or(""), "abc123");
    EXPECT_EQ(textures.skinType(), SkinType::Default);
}

TEST_F(SkinMetadataParserTest, ParseSkinWithSlimModel)
{
    std::string base64 = makeTexturesBase64("http://textures.minecraft.net/texture/slim123", "slim");
    GameProfileProperty prop("textures", base64);
    auto result = SkinMetadataParser::parse(prop);
    ASSERT_TRUE(result.success());

    SkinTextures textures = result.value();
    EXPECT_TRUE(textures.hasSkin());
    EXPECT_EQ(textures.skinType(), SkinType::Slim);
}

TEST_F(SkinMetadataParserTest, ParseSkinWithCapeAndElytra)
{
    std::string base64 = makeTexturesBase64("http://textures.minecraft.net/texture/skin456",
        "default",
        "http://textures.minecraft.net/texture/cape789",
        "http://textures.minecraft.net/texture/elytra012");

    GameProfileProperty prop("textures", base64);
    auto result = SkinMetadataParser::parse(prop);
    ASSERT_TRUE(result.success());

    SkinTextures textures = result.value();
    EXPECT_TRUE(textures.hasSkin());
    EXPECT_TRUE(textures.hasCape());
    EXPECT_TRUE(textures.hasElytra());
    EXPECT_EQ(textures.skinHash().value_or(""), "skin456");
    EXPECT_EQ(textures.capeHash().value_or(""), "cape789");
    EXPECT_EQ(textures.elytraHash().value_or(""), "elytra012");
}

TEST_F(SkinMetadataParserTest, ParseCapeOnly)
{
    std::string base64 = makeTexturesBase64("", "default", "http://textures.minecraft.net/texture/capeonly");
    GameProfileProperty prop("textures", base64);
    auto result = SkinMetadataParser::parse(prop);
    ASSERT_TRUE(result.success());

    SkinTextures textures = result.value();
    EXPECT_FALSE(textures.hasSkin());
    EXPECT_TRUE(textures.hasCape());
    EXPECT_FALSE(textures.hasElytra());
}

// ---------- parseJson 测试 ----------

TEST_F(SkinMetadataParserTest, ParseJsonMissingTexturesKey)
{
    auto result = SkinMetadataParser::parseJson("{}");
    EXPECT_FALSE(result.success());
}

TEST_F(SkinMetadataParserTest, ParseJsonInvalidTexturesType)
{
    auto result = SkinMetadataParser::parseJson("{\"textures\":42}");
    EXPECT_FALSE(result.success());
}

TEST_F(SkinMetadataParserTest, ParseJsonEmptyTexturesObject)
{
    auto result = SkinMetadataParser::parseJson("{\"textures\":{}}");
    ASSERT_TRUE(result.success());

    SkinTextures textures = result.value();
    EXPECT_FALSE(textures.hasSkin());
    EXPECT_FALSE(textures.hasCape());
    EXPECT_FALSE(textures.hasElytra());
}

TEST_F(SkinMetadataParserTest, ParseJsonSkinWithMetadata)
{
    std::string json = R"({
        "textures": {
            "SKIN": {
                "url": "http://textures.minecraft.net/texture/abcdef1234567890abcdef1234567890abcdef1234567890abcdef1234567890",
                "metadata": {
                    "model": "slim"
                }
            }
        }
    })";

    auto result = SkinMetadataParser::parseJson(json);
    ASSERT_TRUE(result.success());

    SkinTextures textures = result.value();
    EXPECT_TRUE(textures.hasSkin());
    EXPECT_EQ(textures.skinType(), SkinType::Slim);
}

TEST_F(SkinMetadataParserTest, ParseJsonSkinWithDefaultModel)
{
    std::string json = R"({
        "textures": {
            "SKIN": {
                "url": "http://textures.minecraft.net/texture/abcdef1234567890abcdef1234567890abcdef1234567890abcdef1234567890",
                "metadata": {
                    "model": "default"
                }
            }
        }
    })";

    auto result = SkinMetadataParser::parseJson(json);
    ASSERT_TRUE(result.success());

    SkinTextures textures = result.value();
    EXPECT_TRUE(textures.hasSkin());
    EXPECT_EQ(textures.skinType(), SkinType::Default);
}

TEST_F(SkinMetadataParserTest, ParseJsonSkinWithoutMetadata)
{
    std::string json = R"({
        "textures": {
            "SKIN": {
                "url": "http://textures.minecraft.net/texture/abcdef1234567890abcdef1234567890abcdef1234567890abcdef1234567890"
            }
        }
    })";

    auto result = SkinMetadataParser::parseJson(json);
    ASSERT_TRUE(result.success());

    SkinTextures textures = result.value();
    EXPECT_TRUE(textures.hasSkin());
    // 无 metadata 时默认为 Default
    EXPECT_EQ(textures.skinType(), SkinType::Default);
}

TEST_F(SkinMetadataParserTest, ParseJsonSkinWithInvalidUrl)
{
    std::string json = R"({
        "textures": {
            "SKIN": {
                "url": 42
            }
        }
    })";

    auto result = SkinMetadataParser::parseJson(json);
    ASSERT_TRUE(result.success());

    SkinTextures textures = result.value();
    // url 不是字符串时，SKIN 不应被设置
    EXPECT_FALSE(textures.hasSkin());
}

TEST_F(SkinMetadataParserTest, ParseJsonInvalidJson)
{
    auto result = SkinMetadataParser::parseJson("not json at all");
    EXPECT_FALSE(result.success());
}

TEST_F(SkinMetadataParserTest, ParseJsonAllTextureTypes)
{
    std::string json = R"({
        "textures": {
            "SKIN": {
                "url": "http://textures.minecraft.net/texture/skinhash0000000000000000000000000000000000000000000000000000",
                "metadata": { "model": "slim" }
            },
            "CAPE": {
                "url": "http://textures.minecraft.net/texture/capehash0000000000000000000000000000000000000000000000000000"
            },
            "ELYTRA": {
                "url": "http://textures.minecraft.net/texture/elytrahash00000000000000000000000000000000000000000000000000"
            }
        }
    })";

    auto result = SkinMetadataParser::parseJson(json);
    ASSERT_TRUE(result.success());

    SkinTextures textures = result.value();
    EXPECT_TRUE(textures.hasSkin());
    EXPECT_TRUE(textures.hasCape());
    EXPECT_TRUE(textures.hasElytra());
    EXPECT_EQ(textures.skinType(), SkinType::Slim);
}

// ---------- parseBase64 测试 ----------

TEST_F(SkinMetadataParserTest, ParseBase64InvalidData)
{
    auto result = SkinMetadataParser::parseBase64("!!!invalid!!!");
    EXPECT_FALSE(result.success());
}

TEST_F(SkinMetadataParserTest, ParseBase64ValidData)
{
    // {"textures":{"SKIN":{"url":"http://textures.minecraft.net/texture/test"}}}
    // Base64 of: {"textures":{"SKIN":{"url":"http://textures.minecraft.net/texture/test"}}}
    std::string json = R"({"textures":{"SKIN":{"url":"http://textures.minecraft.net/texture/test"}}})";
    // 手动 Base64 编码
    std::string base64 = makeTexturesBase64("http://textures.minecraft.net/texture/test");

    auto result = SkinMetadataParser::parseBase64(base64);
    ASSERT_TRUE(result.success());

    SkinTextures textures = result.value();
    EXPECT_TRUE(textures.hasSkin());
}

// ---------- SignatureState / verifySignature 测试 ----------

TEST_F(SkinMetadataParserTest, SignatureStateUnsignedWithoutSignature)
{
    // 没有 signature 的 property 应该返回 UNSIGNED
    GameProfileProperty prop("textures", "dGVzdA==");
    EXPECT_EQ(SkinMetadataParser::getSignatureState(prop), SignatureState::Unsigned);
    EXPECT_TRUE(SkinMetadataParser::verifySignature(prop));
}

TEST_F(SkinMetadataParserTest, SignatureStateWithSignatureReturnsUnsignedWhenNoCrypto)
{
    // 有 signature 但没有加密库支持时，降级为 UNSIGNED（不是 INVALID）
    GameProfileProperty prop("textures", "dGVzdA==", "c29tZXNpZ25hdHVyZQ==");
    SignatureState state = SkinMetadataParser::getSignatureState(prop);
    // 由于没有 RSA 验证能力，当前实现降级为 UNSIGNED
    EXPECT_EQ(state, SignatureState::Unsigned);
    // UNSIGNED 视为有效
    EXPECT_TRUE(SkinMetadataParser::verifySignature(prop));
}

TEST_F(SkinMetadataParserTest, SignatureStateWithEmptySignatureValue)
{
    // 空签名字符串（Base64 解码为空）应该返回 INVALID
    GameProfileProperty prop("textures", "dGVzdA==", "");
    // 空字符串有值但 base64 解码为空
    SignatureState state = SkinMetadataParser::getSignatureState(prop);
    // hasSignature() 返回 true（空字符串也有值），但解码为空 → INVALID
    EXPECT_EQ(state, SignatureState::Invalid);
    EXPECT_FALSE(SkinMetadataParser::verifySignature(prop));
}

TEST_F(SkinMetadataParserTest, SignatureStateWithInvalidBase64Signature)
{
    // 无效的 Base64 签名应该返回 INVALID
    GameProfileProperty prop("textures", "dGVzdA==", "!!!invalid-base64!!!");
    SignatureState state = SkinMetadataParser::getSignatureState(prop);
    // Base64 解码失败 → 空字节 → INVALID
    EXPECT_EQ(state, SignatureState::Invalid);
    EXPECT_FALSE(SkinMetadataParser::verifySignature(prop));
}

// ---------- SignatureState 枚举值测试 ----------

TEST_F(SkinMetadataParserTest, SignatureStateEnumValues)
{
    // 确保枚举值符合预期
    EXPECT_EQ(static_cast<mc::u8>(SignatureState::Unsigned), 0);
    EXPECT_EQ(static_cast<mc::u8>(SignatureState::Invalid), 1);
    EXPECT_EQ(static_cast<mc::u8>(SignatureState::Signed), 2);
}

// ---------- Hash 提取测试 ----------

TEST_F(SkinMetadataParserTest, HashExtractionFromUrl)
{
    std::string json = R"({
        "textures": {
            "SKIN": {
                "url": "http://textures.minecraft.net/texture/abcdef1234567890abcdef1234567890abcdef1234567890abcdef1234567890"
            }
        }
    })";

    auto result = SkinMetadataParser::parseJson(json);
    ASSERT_TRUE(result.success());

    SkinTextures textures = result.value();
    EXPECT_TRUE(textures.hasSkin());
    EXPECT_EQ(textures.skinHash().value_or(""), "abcdef1234567890abcdef1234567890abcdef1234567890abcdef1234567890");
}
