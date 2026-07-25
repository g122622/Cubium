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
#include "common/skin/network/SkinPackets.hpp"
#include "common/util/text/ITextComponent.hpp"
#include "common/util/text/StringTextComponent.hpp"
#include "common/util/text/TextStyle.hpp"
#include <array>
#include <gtest/gtest.h>

using namespace mc::skin;

class SkinPacketsTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(SkinPacketsTest, PlayerListEntryConstruction)
{
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

TEST_F(SkinPacketsTest, PlayerListEntryCreateRemove)
{
    std::array<mc::u8, 16> uuid = GameProfile::parseUUID("550e8400-e29b-41d4-a716-446655440000");

    PlayerListEntry entry = PlayerListEntry::createRemove(uuid);

    for (size_t i = 0; i < 16; ++i) {
        EXPECT_EQ(uuid[i], entry.uuid[i]);
    }
    // Remove 操作只需要 UUID
}

TEST_F(SkinPacketsTest, PlayerListEntryCreateUpdateLatency)
{
    std::array<mc::u8, 16> uuid = GameProfile::parseUUID("550e8400-e29b-41d4-a716-446655440000");

    PlayerListEntry entry = PlayerListEntry::createUpdateLatency(uuid, 200);

    for (size_t i = 0; i < 16; ++i) {
        EXPECT_EQ(uuid[i], entry.uuid[i]);
    }
    EXPECT_EQ(200, entry.ping);
}

TEST_F(SkinPacketsTest, PlayerListEntryCreateUpdateGameMode)
{
    std::array<mc::u8, 16> uuid = GameProfile::parseUUID("550e8400-e29b-41d4-a716-446655440000");

    PlayerListEntry entry = PlayerListEntry::createUpdateGameMode(uuid, mc::GameMode::Creative);

    for (size_t i = 0; i < 16; ++i) {
        EXPECT_EQ(uuid[i], entry.uuid[i]);
    }
    EXPECT_EQ(mc::GameMode::Creative, entry.gameMode);
}

TEST_F(SkinPacketsTest, PlayerListActionValues)
{
    EXPECT_EQ(0, static_cast<int>(PlayerListAction::AddPlayer));
    EXPECT_EQ(1, static_cast<int>(PlayerListAction::UpdateGameMode));
    EXPECT_EQ(2, static_cast<int>(PlayerListAction::UpdateLatency));
    EXPECT_EQ(3, static_cast<int>(PlayerListAction::UpdateDisplayName));
    EXPECT_EQ(4, static_cast<int>(PlayerListAction::RemovePlayer));
}

// ============================================================================
// DisplayName ITextComponent 序列化测试
// ============================================================================

TEST_F(SkinPacketsTest, SetDisplayNameFromStringTextComponent)
{
    PlayerListEntry entry;

    mc::text::StringTextComponent text("TestPlayer");
    entry.setDisplayName(text);

    ASSERT_TRUE(entry.displayName.has_value());
    // 验证是有效的 JSON
    nlohmann::json json = nlohmann::json::parse(*entry.displayName);
    EXPECT_EQ(json["text"], "TestPlayer");
}

TEST_F(SkinPacketsTest, SetDisplayNameWithStyle)
{
    PlayerListEntry entry;

    mc::text::StringTextComponent text("ColoredPlayer");
    mc::text::Style style;
    style.setColor(mc::text::TextFormatting::Red);
    style.setBold(true);
    text.setStyle(style);

    entry.setDisplayName(text);

    ASSERT_TRUE(entry.displayName.has_value());
    nlohmann::json json = nlohmann::json::parse(*entry.displayName);
    EXPECT_EQ(json["text"], "ColoredPlayer");
    EXPECT_EQ(json["color"], "red");
    EXPECT_TRUE(json["bold"].get<bool>());
}

TEST_F(SkinPacketsTest, SetDisplayNameWithSiblings)
{
    PlayerListEntry entry;

    auto text = std::make_unique<mc::text::StringTextComponent>("Hello ");
    mc::text::Style style;
    style.setColor(mc::text::TextFormatting::Yellow);
    text->setStyle(style);

    auto sibling = std::make_unique<mc::text::StringTextComponent>("World");
    mc::text::Style siblingStyle;
    siblingStyle.setColor(mc::text::TextFormatting::Green);
    sibling->setStyle(siblingStyle);
    text->append(std::move(sibling));

    entry.setDisplayName(*text);

    ASSERT_TRUE(entry.displayName.has_value());
    nlohmann::json json = nlohmann::json::parse(*entry.displayName);
    EXPECT_EQ(json["text"], "Hello ");
    EXPECT_EQ(json["color"], "yellow");
    EXPECT_TRUE(json.contains("extra"));
    EXPECT_EQ(json["extra"][0]["text"], "World");
    EXPECT_EQ(json["extra"][0]["color"], "green");
}

TEST_F(SkinPacketsTest, GetDisplayNameAsText)
{
    PlayerListEntry entry;

    mc::text::StringTextComponent text("TestPlayer");
    mc::text::Style style;
    style.setColor(mc::text::TextFormatting::Gold);
    text.setStyle(style);

    entry.setDisplayName(text);

    auto parsed = entry.getDisplayNameAsText();
    ASSERT_NE(parsed, nullptr);
    EXPECT_EQ(parsed->getUnformattedText(), "TestPlayer");
    EXPECT_EQ(parsed->getStyle().getColor(), mc::text::TextFormatting::Gold);
}

TEST_F(SkinPacketsTest, GetDisplayNameAsTextEmpty)
{
    PlayerListEntry entry;

    // displayName 为空时返回 nullptr
    auto parsed = entry.getDisplayNameAsText();
    EXPECT_EQ(parsed, nullptr);
}

TEST_F(SkinPacketsTest, GetDisplayNameAsTextInvalidJson)
{
    PlayerListEntry entry;
    entry.displayName = "invalid json {";

    // 无效 JSON 时返回 nullptr（不抛异常）
    auto parsed = entry.getDisplayNameAsText();
    EXPECT_EQ(parsed, nullptr);
}

TEST_F(SkinPacketsTest, SerializeTextStaticMethod)
{
    mc::text::StringTextComponent text("Hello World");

    std::string json = PlayerListEntry::serializeText(text);

    nlohmann::json parsed = nlohmann::json::parse(json);
    EXPECT_EQ(parsed["text"], "Hello World");
}

TEST_F(SkinPacketsTest, CreateUpdateDisplayName)
{
    std::array<mc::u8, 16> uuid = GameProfile::parseUUID("550e8400-e29b-41d4-a716-446655440000");

    // 有显示名
    PlayerListEntry entry1 = PlayerListEntry::createUpdateDisplayName(uuid, "{\"text\":\"CustomName\"}");
    for (size_t i = 0; i < 16; ++i) {
        EXPECT_EQ(uuid[i], entry1.uuid[i]);
    }
    ASSERT_TRUE(entry1.displayName.has_value());
    EXPECT_EQ(*entry1.displayName, "{\"text\":\"CustomName\"}");

    // 无显示名（清除）
    PlayerListEntry entry2 = PlayerListEntry::createUpdateDisplayName(uuid, std::nullopt);
    EXPECT_FALSE(entry2.displayName.has_value());
}
