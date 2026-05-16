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
#include "common/skin/core/SkinTypes.hpp"
#include <array>
#include <gtest/gtest.h>

using namespace mc::skin;

class GameProfileTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(GameProfileTest, DefaultConstruction)
{
    GameProfile profile;
    EXPECT_FALSE(profile.hasValidUUID());
    EXPECT_TRUE(profile.name().empty());
    EXPECT_TRUE(profile.properties().empty());
}

TEST_F(GameProfileTest, UUIDConstruction)
{
    std::array<mc::u8, 16> uuid = {
        0x55, 0x0e, 0x84, 0x00, 0xe2, 0x9b, 0x41, 0xd4, 0xa7, 0x16, 0x44, 0x66, 0x55, 0x44, 0x00, 0x00};

    GameProfile profile(uuid, "TestPlayer");
    EXPECT_TRUE(profile.hasValidUUID());
    EXPECT_EQ("TestPlayer", profile.name());

    for (size_t i = 0; i < 16; ++i) {
        EXPECT_EQ(uuid[i], profile.uuid()[i]);
    }
}

TEST_F(GameProfileTest, UUIDToString)
{
    std::array<mc::u8, 16> uuid = {
        0x55, 0x0e, 0x84, 0x00, 0xe2, 0x9b, 0x41, 0xd4, 0xa7, 0x16, 0x44, 0x66, 0x55, 0x44, 0x00, 0x00};

    GameProfile profile(uuid, "Test");
    std::string uuidStr = profile.uuidToString();

    EXPECT_EQ("550e8400-e29b-41d4-a716-446655440000", uuidStr);
}

TEST_F(GameProfileTest, UUIDToStringNoDashes)
{
    std::array<mc::u8, 16> uuid = {
        0x55, 0x0e, 0x84, 0x00, 0xe2, 0x9b, 0x41, 0xd4, 0xa7, 0x16, 0x44, 0x66, 0x55, 0x44, 0x00, 0x00};

    GameProfile profile(uuid, "Test");
    std::string uuidStr = profile.uuidToStringNoDashes();

    EXPECT_EQ("550e8400e29b41d4a716446655440000", uuidStr);
}

TEST_F(GameProfileTest, ParseUUID)
{
    std::string uuidStr1 = "550e8400-e29b-41d4-a716-446655440000";
    auto uuid1 = GameProfile::parseUUID(uuidStr1);

    std::array<mc::u8, 16> expected = {
        0x55, 0x0e, 0x84, 0x00, 0xe2, 0x9b, 0x41, 0xd4, 0xa7, 0x16, 0x44, 0x66, 0x55, 0x44, 0x00, 0x00};

    for (size_t i = 0; i < 16; ++i) {
        EXPECT_EQ(expected[i], uuid1[i]);
    }

    // Test without dashes
    std::string uuidStr2 = "550e8400e29b41d4a716446655440000";
    auto uuid2 = GameProfile::parseUUID(uuidStr2);

    for (size_t i = 0; i < 16; ++i) {
        EXPECT_EQ(expected[i], uuid2[i]);
    }
}

TEST_F(GameProfileTest, ParseInvalidUUID)
{
    std::string invalidStr = "invalid-uuid";
    auto uuid = GameProfile::parseUUID(invalidStr);

    // Should return all zeros
    for (size_t i = 0; i < 16; ++i) {
        EXPECT_EQ(0, uuid[i]);
    }
}

TEST_F(GameProfileTest, PropertyManagement)
{
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

TEST_F(GameProfileTest, UUIDHashCode)
{
    std::array<mc::u8, 16> uuid = {
        0x55, 0x0e, 0x84, 0x00, 0xe2, 0x9b, 0x41, 0xd4, 0xa7, 0x16, 0x44, 0x66, 0x55, 0x44, 0x00, 0x00};

    GameProfile profile(uuid, "Test");
    mc::i32 hash1 = profile.uuidHashCode();
    mc::i32 hash2 = profile.uuidHashCode();

    EXPECT_EQ(hash1, hash2);
}

TEST_F(GameProfileTest, SetName)
{
    GameProfile profile;
    profile.setName("Player1");
    EXPECT_EQ("Player1", profile.name());
}

TEST_F(GameProfileTest, SetUUID)
{
    std::array<mc::u8, 16> uuid = {
        0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10};

    GameProfile profile;
    profile.setUUID(uuid);
    EXPECT_TRUE(profile.hasValidUUID());

    for (size_t i = 0; i < 16; ++i) {
        EXPECT_EQ(uuid[i], profile.uuid()[i]);
    }
}

// ============================================================================
// JSON 序列化测试
// ============================================================================

TEST_F(GameProfileTest, ToJson_BasicProfile)
{
    std::array<mc::u8, 16> uuid = {
        0x55, 0x0e, 0x84, 0x00, 0xe2, 0x9b, 0x41, 0xd4, 0xa7, 0x16, 0x44, 0x66, 0x55, 0x44, 0x00, 0x00};

    GameProfile profile(uuid, "TestPlayer");
    nlohmann::json json = profile.toJson();

    EXPECT_TRUE(json.is_object());
    EXPECT_EQ("TestPlayer", json["Name"].get<std::string>());
    EXPECT_EQ("550e8400-e29b-41d4-a716-446655440000", json["Id"].get<std::string>());
    EXPECT_FALSE(json.contains("Properties")); // 无属性时不包含 Properties 字段
}

TEST_F(GameProfileTest, ToJson_EmptyProfile)
{
    GameProfile profile; // 空 UUID 和名称
    nlohmann::json json = profile.toJson();

    EXPECT_TRUE(json.is_object());
    EXPECT_FALSE(json.contains("Name"));    // 空名称不写入
    EXPECT_FALSE(json.contains("Id"));      // 无效 UUID 不写入
    EXPECT_FALSE(json.contains("Properties"));
}

TEST_F(GameProfileTest, ToJson_WithProperties)
{
    std::array<mc::u8, 16> uuid = {
        0x55, 0x0e, 0x84, 0x00, 0xe2, 0x9b, 0x41, 0xd4, 0xa7, 0x16, 0x44, 0x66, 0x55, 0x44, 0x00, 0x00};

    GameProfile profile(uuid, "PlayerWithSkin");
    profile.addProperty({"textures", "base64EncodedTextureData", "textureSignature"});

    nlohmann::json json = profile.toJson();

    EXPECT_TRUE(json.contains("Properties"));
    EXPECT_TRUE(json["Properties"].is_object());
    EXPECT_TRUE(json["Properties"].contains("textures"));

    const auto& textures = json["Properties"]["textures"];
    EXPECT_TRUE(textures.is_array());
    EXPECT_EQ(1u, textures.size());
    EXPECT_EQ("base64EncodedTextureData", textures[0]["Value"].get<std::string>());
    EXPECT_EQ("textureSignature", textures[0]["Signature"].get<std::string>());
}

TEST_F(GameProfileTest, ToJson_PropertyWithoutSignature)
{
    std::array<mc::u8, 16> uuid = {
        0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10};

    GameProfile profile(uuid, "OfflinePlayer");
    profile.addProperty({"textures", "base64Data"}); // 无签名

    nlohmann::json json = profile.toJson();

    EXPECT_TRUE(json["Properties"]["textures"][0].contains("Value"));
    EXPECT_FALSE(json["Properties"]["textures"][0].contains("Signature"));
}

TEST_F(GameProfileTest, FromJson_StringUUID)
{
    nlohmann::json json = {
        {"Name", "TestPlayer"},
        {"Id", "550e8400-e29b-41d4-a716-446655440000"}
    };

    auto result = GameProfile::fromJson(json);
    ASSERT_TRUE(result.success());

    const GameProfile& profile = result.value();
    EXPECT_EQ("TestPlayer", profile.name());
    EXPECT_TRUE(profile.hasValidUUID());

    std::array<mc::u8, 16> expectedUUID = {
        0x55, 0x0e, 0x84, 0x00, 0xe2, 0x9b, 0x41, 0xd4, 0xa7, 0x16, 0x44, 0x66, 0x55, 0x44, 0x00, 0x00};
    for (size_t i = 0; i < 16; ++i) {
        EXPECT_EQ(expectedUUID[i], profile.uuid()[i]);
    }
}

TEST_F(GameProfileTest, FromJson_IntArrayUUID)
{
    // MC NBT 格式：UUID 存储为 4 个 int
    nlohmann::json json = {
        {"Name", "IntArrayUUID"},
        {"Id", {1430752512, -490278444, -1488612266, 1149116928}} // 对应 UUID
    };

    auto result = GameProfile::fromJson(json);
    ASSERT_TRUE(result.success());

    const GameProfile& profile = result.value();
    EXPECT_EQ("IntArrayUUID", profile.name());
    EXPECT_TRUE(profile.hasValidUUID());
}

TEST_F(GameProfileTest, FromJson_WithProperties)
{
    nlohmann::json json = {
        {"Name", "PlayerWithSkin"},
        {"Id", "550e8400-e29b-41d4-a716-446655440000"},
        {"Properties",
         {{"textures",
           nlohmann::json::array({{{"Value", "base64Data"}, {"Signature", "signatureData"}}})}}}
    };

    auto result = GameProfile::fromJson(json);
    ASSERT_TRUE(result.success());

    const GameProfile& profile = result.value();
    EXPECT_TRUE(profile.hasTextures());

    const GameProfileProperty* prop = profile.getTexturesProperty();
    ASSERT_NE(nullptr, prop);
    EXPECT_EQ("base64Data", prop->value);
    EXPECT_TRUE(prop->hasSignature());
    EXPECT_EQ("signatureData", prop->signature.value());
}

TEST_F(GameProfileTest, FromJson_InvalidInput)
{
    // 非 JSON 对象
    nlohmann::json json1 = "not an object";
    auto result1 = GameProfile::fromJson(json1);
    EXPECT_FALSE(result1.success());

    // 数组
    nlohmann::json json2 = nlohmann::json::array();
    auto result2 = GameProfile::fromJson(json2);
    EXPECT_FALSE(result2.success());
}

TEST_F(GameProfileTest, JsonRoundTrip)
{
    std::array<mc::u8, 16> uuid = {
        0x55, 0x0e, 0x84, 0x00, 0xe2, 0x9b, 0x41, 0xd4, 0xa7, 0x16, 0x44, 0x66, 0x55, 0x44, 0x00, 0x00};

    GameProfile original(uuid, "RoundTripPlayer");
    original.addProperty({"textures", "textureValue", "textureSignature"});

    // 序列化
    nlohmann::json json = original.toJson();

    // 反序列化
    auto result = GameProfile::fromJson(json);
    ASSERT_TRUE(result.success());

    const GameProfile& restored = result.value();

    // 验证
    EXPECT_EQ(original.name(), restored.name());
    EXPECT_EQ(original.uuidToString(), restored.uuidToString());
    EXPECT_TRUE(restored.hasTextures());

    const GameProfileProperty* prop = restored.getTexturesProperty();
    ASSERT_NE(nullptr, prop);
    EXPECT_EQ("textureValue", prop->value);
    EXPECT_EQ("textureSignature", prop->signature.value());
}
