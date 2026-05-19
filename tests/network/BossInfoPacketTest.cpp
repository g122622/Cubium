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

#include "network/packet/BossInfoPacket.hpp"
#include "util/text/StringTextComponent.hpp"
#include "util/text/TextStyle.hpp"
#include <gtest/gtest.h>

using namespace mc::network;
using namespace mc::text;
using mc::f32;
using mc::u64;
using mc::u8;

// ==================== BossInfoPacket 基础测试 ====================

class BossInfoPacketTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        testUuid = 12345678901234ULL;
        testText = R"({"text":"Dragon Health","color":"red"})";
    }

    u64 testUuid;
    std::string testText;
};

TEST_F(BossInfoPacketTest, DefaultConstruction)
{
    BossInfoPacket packet;
    EXPECT_EQ(packet.action(), BossInfoAction::Add);
    EXPECT_EQ(packet.uuid(), 0ULL);
    EXPECT_EQ(packet.percent(), 1.0f);
    EXPECT_EQ(packet.color(), 0u);
    EXPECT_EQ(packet.overlay(), 0u);
    EXPECT_FALSE(packet.darkenSky());
    EXPECT_FALSE(packet.playEndBossMusic());
    EXPECT_FALSE(packet.createFog());
}

TEST_F(BossInfoPacketTest, CreateAddPacket)
{
    auto name = std::make_unique<StringTextComponent>("Ender Dragon");
    auto packet = BossInfoPacket::add(testUuid, std::move(name), 0.75f, 2, 0, true, true, false);

    EXPECT_EQ(packet.action(), BossInfoAction::Add);
    EXPECT_EQ(packet.uuid(), testUuid);
    EXPECT_EQ(packet.percent(), 0.75f);
    EXPECT_EQ(packet.color(), 2u);
    EXPECT_EQ(packet.overlay(), 0u);
    EXPECT_TRUE(packet.darkenSky());
    EXPECT_TRUE(packet.playEndBossMusic());
    EXPECT_FALSE(packet.createFog());
}

TEST_F(BossInfoPacketTest, CreateRemovePacket)
{
    auto packet = BossInfoPacket::remove(testUuid);

    EXPECT_EQ(packet.action(), BossInfoAction::Remove);
    EXPECT_EQ(packet.uuid(), testUuid);
}

TEST_F(BossInfoPacketTest, CreateUpdatePercentPacket)
{
    auto packet = BossInfoPacket::updatePercent(testUuid, 0.5f);

    EXPECT_EQ(packet.action(), BossInfoAction::UpdatePercent);
    EXPECT_EQ(packet.uuid(), testUuid);
    EXPECT_EQ(packet.percent(), 0.5f);
}

TEST_F(BossInfoPacketTest, CreateUpdateNamePacket)
{
    auto name = std::make_unique<StringTextComponent>("New Boss Name");
    auto packet = BossInfoPacket::updateName(testUuid, std::move(name));

    EXPECT_EQ(packet.action(), BossInfoAction::UpdateName);
    EXPECT_EQ(packet.uuid(), testUuid);
    EXPECT_TRUE(packet.nameJson().find("New Boss Name") != std::string::npos);
}

TEST_F(BossInfoPacketTest, CreateUpdateStylePacket)
{
    auto packet = BossInfoPacket::updateStyle(testUuid, 3, 2);

    EXPECT_EQ(packet.action(), BossInfoAction::UpdateStyle);
    EXPECT_EQ(packet.uuid(), testUuid);
    EXPECT_EQ(packet.color(), 3u);
    EXPECT_EQ(packet.overlay(), 2u);
}

TEST_F(BossInfoPacketTest, CreateUpdatePropertiesPacket)
{
    auto packet = BossInfoPacket::updateProperties(testUuid, true, false, true);

    EXPECT_EQ(packet.action(), BossInfoAction::UpdateProperties);
    EXPECT_EQ(packet.uuid(), testUuid);
    EXPECT_TRUE(packet.darkenSky());
    EXPECT_FALSE(packet.playEndBossMusic());
    EXPECT_TRUE(packet.createFog());
}

// ==================== BossInfoPacket 序列化测试 ====================

class BossInfoPacketSerializeTest : public ::testing::Test {
protected:
    void SetUp() override { testUuid = 12345678901234ULL; }

    u64 testUuid;
};

TEST_F(BossInfoPacketSerializeTest, SerializeDeserializeAddPacket)
{
    auto name = std::make_unique<StringTextComponent>("Test Boss");
    Style style;
    style.setColor(TextFormatting::Red);
    name->setStyle(style);

    auto original = BossInfoPacket::add(testUuid, std::move(name), 0.8f, 2, 1, true, false, true);

    auto result = original.serialize();
    ASSERT_TRUE(result.success()) << result.error().message();

    const auto& data = result.value();
    EXPECT_GT(data.size(), 0u);

    BossInfoPacket deserialized;
    auto deserResult = deserialized.deserialize(data.data(), data.size());
    EXPECT_TRUE(deserResult.success()) << deserResult.error().message();

    EXPECT_EQ(deserialized.action(), BossInfoAction::Add);
    EXPECT_EQ(deserialized.uuid(), testUuid);
    EXPECT_FLOAT_EQ(deserialized.percent(), 0.8f);
    EXPECT_EQ(deserialized.color(), 2u);
    EXPECT_EQ(deserialized.overlay(), 1u);
    EXPECT_TRUE(deserialized.darkenSky());
    EXPECT_FALSE(deserialized.playEndBossMusic());
    EXPECT_TRUE(deserialized.createFog());
    EXPECT_TRUE(deserialized.nameJson().find("Test Boss") != std::string::npos);
}

TEST_F(BossInfoPacketSerializeTest, SerializeDeserializeRemovePacket)
{
    auto original = BossInfoPacket::remove(testUuid);

    auto result = original.serialize();
    ASSERT_TRUE(result.success()) << result.error().message();

    BossInfoPacket deserialized;
    auto deserResult = deserialized.deserialize(result.value().data(), result.value().size());
    EXPECT_TRUE(deserResult.success()) << deserResult.error().message();

    EXPECT_EQ(deserialized.action(), BossInfoAction::Remove);
    EXPECT_EQ(deserialized.uuid(), testUuid);
}

TEST_F(BossInfoPacketSerializeTest, SerializeDeserializeUpdatePercentPacket)
{
    auto original = BossInfoPacket::updatePercent(testUuid, 0.25f);

    auto result = original.serialize();
    ASSERT_TRUE(result.success()) << result.error().message();

    BossInfoPacket deserialized;
    auto deserResult = deserialized.deserialize(result.value().data(), result.value().size());
    EXPECT_TRUE(deserResult.success()) << deserResult.error().message();

    EXPECT_EQ(deserialized.action(), BossInfoAction::UpdatePercent);
    EXPECT_EQ(deserialized.uuid(), testUuid);
    EXPECT_FLOAT_EQ(deserialized.percent(), 0.25f);
}

TEST_F(BossInfoPacketSerializeTest, SerializeDeserializeUpdateNamePacket)
{
    auto name = std::make_unique<StringTextComponent>("Updated Name");
    auto original = BossInfoPacket::updateName(testUuid, std::move(name));

    auto result = original.serialize();
    ASSERT_TRUE(result.success()) << result.error().message();

    BossInfoPacket deserialized;
    auto deserResult = deserialized.deserialize(result.value().data(), result.value().size());
    EXPECT_TRUE(deserResult.success()) << deserResult.error().message();

    EXPECT_EQ(deserialized.action(), BossInfoAction::UpdateName);
    EXPECT_EQ(deserialized.uuid(), testUuid);
    EXPECT_TRUE(deserialized.nameJson().find("Updated Name") != std::string::npos);
}

TEST_F(BossInfoPacketSerializeTest, SerializeDeserializeUpdateStylePacket)
{
    auto original = BossInfoPacket::updateStyle(testUuid, 5, 3);

    auto result = original.serialize();
    ASSERT_TRUE(result.success()) << result.error().message();

    BossInfoPacket deserialized;
    auto deserResult = deserialized.deserialize(result.value().data(), result.value().size());
    EXPECT_TRUE(deserResult.success()) << deserResult.error().message();

    EXPECT_EQ(deserialized.action(), BossInfoAction::UpdateStyle);
    EXPECT_EQ(deserialized.uuid(), testUuid);
    EXPECT_EQ(deserialized.color(), 5u);
    EXPECT_EQ(deserialized.overlay(), 3u);
}

TEST_F(BossInfoPacketSerializeTest, SerializeDeserializeUpdatePropertiesPacket)
{
    auto original = BossInfoPacket::updateProperties(testUuid, false, true, false);

    auto result = original.serialize();
    ASSERT_TRUE(result.success()) << result.error().message();

    BossInfoPacket deserialized;
    auto deserResult = deserialized.deserialize(result.value().data(), result.value().size());
    EXPECT_TRUE(deserResult.success()) << deserResult.error().message();

    EXPECT_EQ(deserialized.action(), BossInfoAction::UpdateProperties);
    EXPECT_EQ(deserialized.uuid(), testUuid);
    EXPECT_FALSE(deserialized.darkenSky());
    EXPECT_TRUE(deserialized.playEndBossMusic());
    EXPECT_FALSE(deserialized.createFog());
}

TEST_F(BossInfoPacketSerializeTest, SerializeDeserializeAllFlagsEnabled)
{
    auto name = std::make_unique<StringTextComponent>("Full Boss");
    auto original = BossInfoPacket::add(testUuid, std::move(name), 1.0f, 6, 4, true, true, true);

    auto result = original.serialize();
    ASSERT_TRUE(result.success()) << result.error().message();

    BossInfoPacket deserialized;
    auto deserResult = deserialized.deserialize(result.value().data(), result.value().size());
    EXPECT_TRUE(deserResult.success()) << deserResult.error().message();

    EXPECT_TRUE(deserialized.darkenSky());
    EXPECT_TRUE(deserialized.playEndBossMusic());
    EXPECT_TRUE(deserialized.createFog());
}

TEST_F(BossInfoPacketSerializeTest, SerializeDeserializeZeroPercent)
{
    auto original = BossInfoPacket::updatePercent(testUuid, 0.0f);

    auto result = original.serialize();
    ASSERT_TRUE(result.success()) << result.error().message();

    BossInfoPacket deserialized;
    auto deserResult = deserialized.deserialize(result.value().data(), result.value().size());
    EXPECT_TRUE(deserResult.success()) << deserResult.error().message();

    EXPECT_FLOAT_EQ(deserialized.percent(), 0.0f);
}

TEST_F(BossInfoPacketSerializeTest, SerializeDeserializeFullPercent)
{
    auto original = BossInfoPacket::updatePercent(testUuid, 1.0f);

    auto result = original.serialize();
    ASSERT_TRUE(result.success()) << result.error().message();

    BossInfoPacket deserialized;
    auto deserResult = deserialized.deserialize(result.value().data(), result.value().size());
    EXPECT_TRUE(deserResult.success()) << deserResult.error().message();

    EXPECT_FLOAT_EQ(deserialized.percent(), 1.0f);
}

// ==================== BossInfoPacket 所有操作类型测试 ====================

TEST_F(BossInfoPacketSerializeTest, AllActionTypesSerialize)
{
    std::vector<BossInfoAction> actions = {BossInfoAction::Add,
        BossInfoAction::Remove,
        BossInfoAction::UpdatePercent,
        BossInfoAction::UpdateName,
        BossInfoAction::UpdateStyle,
        BossInfoAction::UpdateProperties};

    for (auto action : actions) {
        BossInfoPacket packet;
        // 手动设置必要的字段用于测试
        switch (action) {
            case BossInfoAction::Add: {
                auto name = std::make_unique<StringTextComponent>("Test");
                packet = BossInfoPacket::add(testUuid, std::move(name), 0.5f, 0, 0, false, false, false);
                break;
            }
            case BossInfoAction::Remove:
                packet = BossInfoPacket::remove(testUuid);
                break;
            case BossInfoAction::UpdatePercent:
                packet = BossInfoPacket::updatePercent(testUuid, 0.5f);
                break;
            case BossInfoAction::UpdateName: {
                auto name = std::make_unique<StringTextComponent>("Test");
                packet = BossInfoPacket::updateName(testUuid, std::move(name));
                break;
            }
            case BossInfoAction::UpdateStyle:
                packet = BossInfoPacket::updateStyle(testUuid, 0, 0);
                break;
            case BossInfoAction::UpdateProperties:
                packet = BossInfoPacket::updateProperties(testUuid, false, false, false);
                break;
        }

        auto result = packet.serialize();
        ASSERT_TRUE(result.success()) << "Failed to serialize action " << static_cast<int>(action);

        BossInfoPacket deserialized;
        auto deserResult = deserialized.deserialize(result.value().data(), result.value().size());
        ASSERT_TRUE(deserResult.success()) << "Failed to deserialize action " << static_cast<int>(action);
        EXPECT_EQ(deserialized.action(), action);
    }
}

// ==================== BossInfoPacket 错误处理测试 ====================

TEST(BossInfoPacketErrorTest, DeserializeEmptyData)
{
    BossInfoPacket packet;
    auto result = packet.deserialize(nullptr, 0);
    EXPECT_FALSE(result.success());
}

TEST(BossInfoPacketErrorTest, DeserializeTruncatedData)
{
    auto original =
        BossInfoPacket::add(12345ULL, std::make_unique<StringTextComponent>("Test"), 0.5f, 0, 0, false, false, false);
    auto result = original.serialize();
    ASSERT_TRUE(result.success());

    auto data = result.value();
    // 截断数据 (至少需要 9 字节: UUID + Action)
    if (data.size() > 10) {
        data.resize(5);

        BossInfoPacket deserialized;
        auto deserResult = deserialized.deserialize(data.data(), data.size());
        EXPECT_FALSE(deserResult.success());
    }
}

TEST(BossInfoPacketErrorTest, DeserializeMinimumValidData)
{
    // 最小有效数据: Remove 包 (UUID + Action = 9 字节)
    auto original = BossInfoPacket::remove(12345ULL);
    auto result = original.serialize();
    ASSERT_TRUE(result.success());

    BossInfoPacket deserialized;
    auto deserResult = deserialized.deserialize(result.value().data(), result.value().size());
    EXPECT_TRUE(deserResult.success());
    EXPECT_EQ(deserialized.action(), BossInfoAction::Remove);
}

// ==================== BossInfoPacket 与 ITextComponent 集成测试 ====================

TEST(BossInfoPacketComponentTest, CreateFromSimpleText)
{
    auto component = std::make_unique<StringTextComponent>("Simple Boss Name");
    auto packet = BossInfoPacket::add(1ULL, std::move(component), 1.0f, 0, 0, false, false, false);

    EXPECT_EQ(packet.action(), BossInfoAction::Add);
    EXPECT_TRUE(packet.nameJson().find("Simple Boss Name") != std::string::npos);
}

TEST(BossInfoPacketComponentTest, CreateFromStyledText)
{
    auto component = std::make_unique<StringTextComponent>("Styled Boss");
    Style style;
    style.setColor(TextFormatting::Gold);
    style.setBold(true);
    style.setItalic(true);
    component->setStyle(style);

    auto packet = BossInfoPacket::add(1ULL, std::move(component), 1.0f, 0, 0, false, false, false);

    EXPECT_TRUE(packet.nameJson().find("gold") != std::string::npos);
    EXPECT_TRUE(packet.nameJson().find("bold") != std::string::npos);
    EXPECT_TRUE(packet.nameJson().find("italic") != std::string::npos);
}

TEST(BossInfoPacketComponentTest, CreateFromNestedText)
{
    auto mainText = std::make_unique<StringTextComponent>("Main ");
    Style mainStyle;
    mainStyle.setColor(TextFormatting::Red);
    mainText->setStyle(mainStyle);

    auto extraText = std::make_unique<StringTextComponent>("Extra");
    Style extraStyle;
    extraStyle.setColor(TextFormatting::Blue);
    extraText->setStyle(extraStyle);
    mainText->append(std::move(extraText));

    auto packet = BossInfoPacket::add(1ULL, std::move(mainText), 1.0f, 0, 0, false, false, false);

    EXPECT_TRUE(packet.nameJson().find("Main") != std::string::npos);
    EXPECT_TRUE(packet.nameJson().find("Extra") != std::string::npos);
}

// ==================== BossInfoPacket 颜色和样式测试 ====================

TEST(BossInfoPacketColorTest, AllColors)
{
    // 测试所有 BossInfoColor 值
    for (u8 color = 0; color <= 6; ++color) {
        auto packet = BossInfoPacket::updateStyle(1ULL, color, 0);

        auto result = packet.serialize();
        ASSERT_TRUE(result.success()) << "Failed to serialize color " << color;

        BossInfoPacket deserialized;
        auto deserResult = deserialized.deserialize(result.value().data(), result.value().size());
        ASSERT_TRUE(deserResult.success()) << "Failed to deserialize color " << color;
        EXPECT_EQ(deserialized.color(), color);
    }
}

TEST(BossInfoPacketOverlayTest, AllOverlays)
{
    // 测试所有 BossInfoOverlay 值
    for (u8 overlay = 0; overlay <= 4; ++overlay) {
        auto packet = BossInfoPacket::updateStyle(1ULL, 0, overlay);

        auto result = packet.serialize();
        ASSERT_TRUE(result.success()) << "Failed to serialize overlay " << overlay;

        BossInfoPacket deserialized;
        auto deserResult = deserialized.deserialize(result.value().data(), result.value().size());
        ASSERT_TRUE(deserResult.success()) << "Failed to deserialize overlay " << overlay;
        EXPECT_EQ(deserialized.overlay(), overlay);
    }
}

// ==================== 性能测试 ====================

TEST(BossInfoPacketPerfTest, SerializeDeserializePerformance)
{
    u64 testUuid = 12345678901234ULL;
    auto name = std::make_unique<StringTextComponent>("Performance Test Boss");
    auto packet = BossInfoPacket::add(testUuid, std::move(name), 0.5f, 2, 1, true, false, true);

    for (int i = 0; i < 1000; ++i) {
        auto result = packet.serialize();
        ASSERT_TRUE(result.success());
    }

    auto result = packet.serialize();
    for (int i = 0; i < 1000; ++i) {
        BossInfoPacket received;
        auto deserResult = received.deserialize(result.value().data(), result.value().size());
        ASSERT_TRUE(deserResult.success());
    }
}

// ==================== Boss 栏场景模拟测试 ====================

TEST(BossInfoPacketScenarioTest, SimulateBossSpawn)
{
    // 模拟 Boss 生成
    auto name = std::make_unique<StringTextComponent>("Ender Dragon");
    Style style;
    style.setColor(TextFormatting::DarkPurple);
    name->setStyle(style);

    auto packet = BossInfoPacket::add(12345ULL, std::move(name), 1.0f, 5, 0, true, true, false);

    EXPECT_EQ(packet.action(), BossInfoAction::Add);
    EXPECT_FLOAT_EQ(packet.percent(), 1.0f);
    EXPECT_TRUE(packet.darkenSky());
    EXPECT_TRUE(packet.playEndBossMusic());

    auto result = packet.serialize();
    ASSERT_TRUE(result.success());
}

TEST(BossInfoPacketScenarioTest, SimulateBossHealthUpdate)
{
    // 模拟 Boss 血量更新
    for (float percent = 1.0f; percent >= 0.0f; percent -= 0.1f) {
        auto packet = BossInfoPacket::updatePercent(12345ULL, percent);

        auto result = packet.serialize();
        ASSERT_TRUE(result.success());

        BossInfoPacket deserialized;
        auto deserResult = deserialized.deserialize(result.value().data(), result.value().size());
        ASSERT_TRUE(deserResult.success());
        EXPECT_FLOAT_EQ(deserialized.percent(), percent);
    }
}

TEST(BossInfoPacketScenarioTest, SimulateBossDeath)
{
    // 模拟 Boss 死亡
    // 先设置血量为 0
    auto healthPacket = BossInfoPacket::updatePercent(12345ULL, 0.0f);
    EXPECT_EQ(healthPacket.action(), BossInfoAction::UpdatePercent);

    // 然后移除 Boss 栏
    auto removePacket = BossInfoPacket::remove(12345ULL);
    EXPECT_EQ(removePacket.action(), BossInfoAction::Remove);

    auto result = removePacket.serialize();
    ASSERT_TRUE(result.success());
}

TEST(BossInfoPacketScenarioTest, SimulateCustomBossbarCreate)
{
    // 模拟 /bossbar add 命令
    auto name = std::make_unique<StringTextComponent>("Custom Boss Bar");
    auto packet = BossInfoPacket::add(99999ULL, std::move(name), 0.0f, 6, 0, false, false, false);

    EXPECT_EQ(packet.action(), BossInfoAction::Add);
    EXPECT_FLOAT_EQ(packet.percent(), 0.0f);
    EXPECT_EQ(packet.color(), 6u); // White

    auto result = packet.serialize();
    ASSERT_TRUE(result.success());
}

TEST(BossInfoPacketScenarioTest, SimulateCustomBossbarUpdate)
{
    // 模拟 /bossbar set 命令序列
    u64 bossUuid = 99999ULL;

    // 设置名称
    auto namePacket = BossInfoPacket::updateName(bossUuid, std::make_unique<StringTextComponent>("New Name"));
    EXPECT_EQ(namePacket.action(), BossInfoAction::UpdateName);

    // 设置值（百分比）
    auto percentPacket = BossInfoPacket::updatePercent(bossUuid, 0.5f);
    EXPECT_EQ(percentPacket.action(), BossInfoAction::UpdatePercent);

    // 设置颜色
    auto stylePacket = BossInfoPacket::updateStyle(bossUuid, 2, 0); // Red color
    EXPECT_EQ(stylePacket.action(), BossInfoAction::UpdateStyle);

    // 设置属性
    auto propsPacket = BossInfoPacket::updateProperties(bossUuid, true, false, false);
    EXPECT_EQ(propsPacket.action(), BossInfoAction::UpdateProperties);

    // 验证都能序列化和反序列化
    for (auto& packet : {namePacket, percentPacket, stylePacket, propsPacket}) {
        auto result = packet.serialize();
        ASSERT_TRUE(result.success());
    }
}

// ==================== 特殊字符和 Unicode 测试 ====================

TEST(BossInfoPacketSpecialCharTest, UnicodeCharacters)
{
    // 测试 Unicode 字符（中文、日文、表情符号等）
    auto name = std::make_unique<StringTextComponent>("Boss 名称 🐉 ドラゴン");
    auto packet = BossInfoPacket::add(1ULL, std::move(name), 1.0f, 0, 0, false, false, false);

    EXPECT_TRUE(packet.nameJson().find("Boss") != std::string::npos);

    auto result = packet.serialize();
    ASSERT_TRUE(result.success());

    BossInfoPacket deserialized;
    auto deserResult = deserialized.deserialize(result.value().data(), result.value().size());
    EXPECT_TRUE(deserResult.success());
}

TEST(BossInfoPacketSpecialCharTest, JsonEscapeSequences)
{
    // 测试 JSON 转义序列
    auto name = std::make_unique<StringTextComponent>("Line1\nLine2\tTabbed");
    auto packet = BossInfoPacket::add(1ULL, std::move(name), 1.0f, 0, 0, false, false, false);

    auto result = packet.serialize();
    ASSERT_TRUE(result.success());

    BossInfoPacket deserialized;
    auto deserResult = deserialized.deserialize(result.value().data(), result.value().size());
    EXPECT_TRUE(deserResult.success());
}

// ==================== 边界条件测试 ====================

TEST(BossInfoPacketBoundaryTest, MaxUuid)
{
    u64 maxUuid = UINT64_MAX;
    auto packet = BossInfoPacket::remove(maxUuid);

    EXPECT_EQ(packet.uuid(), maxUuid);

    auto result = packet.serialize();
    ASSERT_TRUE(result.success());

    BossInfoPacket deserialized;
    auto deserResult = deserialized.deserialize(result.value().data(), result.value().size());
    EXPECT_TRUE(deserResult.success());
    EXPECT_EQ(deserialized.uuid(), maxUuid);
}

TEST(BossInfoPacketBoundaryTest, ZeroUuid)
{
    u64 zeroUuid = 0;
    auto packet = BossInfoPacket::remove(zeroUuid);

    EXPECT_EQ(packet.uuid(), zeroUuid);

    auto result = packet.serialize();
    ASSERT_TRUE(result.success());

    BossInfoPacket deserialized;
    auto deserResult = deserialized.deserialize(result.value().data(), result.value().size());
    EXPECT_TRUE(deserResult.success());
    EXPECT_EQ(deserialized.uuid(), zeroUuid);
}

TEST(BossInfoPacketBoundaryTest, RapidUpdateSequence)
{
    // 模拟快速连续发送不同类型的 Boss 栏包
    u64 uuid = 12345ULL;

    // Add
    auto addPacket =
        BossInfoPacket::add(uuid, std::make_unique<StringTextComponent>("Boss"), 1.0f, 0, 0, false, false, false);
    auto addResult = addPacket.serialize();
    ASSERT_TRUE(addResult.success());

    // Multiple percent updates
    for (int i = 0; i < 10; ++i) {
        auto percentPacket = BossInfoPacket::updatePercent(uuid, 1.0f - i * 0.1f);
        auto result = percentPacket.serialize();
        ASSERT_TRUE(result.success());
    }

    // Update name
    auto namePacket = BossInfoPacket::updateName(uuid, std::make_unique<StringTextComponent>("Updated"));
    auto nameResult = namePacket.serialize();
    ASSERT_TRUE(nameResult.success());

    // Update style
    auto stylePacket = BossInfoPacket::updateStyle(uuid, 2, 1);
    auto styleResult = stylePacket.serialize();
    ASSERT_TRUE(styleResult.success());

    // Remove
    auto removePacket = BossInfoPacket::remove(uuid);
    auto removeResult = removePacket.serialize();
    ASSERT_TRUE(removeResult.success());
}
