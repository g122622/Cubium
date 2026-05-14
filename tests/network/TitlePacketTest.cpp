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

#include "network/packet/TitlePacket.hpp"
#include "util/text/StringTextComponent.hpp"
#include "util/text/TextStyle.hpp"
#include <gtest/gtest.h>

using namespace mc::network;
using namespace mc::text;
using mc::i32;
using mc::u16;
using mc::u8;

// ==================== TitlePacket 基础测试 ====================

class TitlePacketTest : public ::testing::Test {
protected:
    void SetUp() override { testText = R"({"text":"Hello World","color":"red"})"; }

    std::string testText;
};

TEST_F(TitlePacketTest, DefaultConstruction)
{
    TitlePacket packet;
    EXPECT_EQ(packet.action(), TitleAction::Clear);
    EXPECT_FALSE(packet.text().has_value());
    EXPECT_EQ(packet.fadeIn(), -1);
    EXPECT_EQ(packet.stay(), -1);
    EXPECT_EQ(packet.fadeOut(), -1);
}

TEST_F(TitlePacketTest, CreateTitle)
{
    auto packet = TitlePacket::createTitle(testText);

    EXPECT_EQ(packet.action(), TitleAction::Title);
    EXPECT_TRUE(packet.text().has_value());
    EXPECT_EQ(packet.text().value(), testText);
}

TEST_F(TitlePacketTest, CreateTitleFromComponent)
{
    auto component = std::make_unique<StringTextComponent>("Hello World");
    Style style;
    style.setColor(TextFormatting::Red);
    component->setStyle(style);

    auto packet = TitlePacket::createTitle(*component);

    EXPECT_EQ(packet.action(), TitleAction::Title);
    EXPECT_TRUE(packet.text().has_value());
    // 验证JSON包含正确的文本
    EXPECT_TRUE(packet.text()->find("Hello World") != std::string::npos);
}

TEST_F(TitlePacketTest, CreateSubtitle)
{
    auto packet = TitlePacket::createSubtitle(testText);

    EXPECT_EQ(packet.action(), TitleAction::Subtitle);
    EXPECT_TRUE(packet.text().has_value());
    EXPECT_EQ(packet.text().value(), testText);
}

TEST_F(TitlePacketTest, CreateSubtitleFromComponent)
{
    auto component = std::make_unique<StringTextComponent>("Subtitle Text");
    auto packet = TitlePacket::createSubtitle(*component);

    EXPECT_EQ(packet.action(), TitleAction::Subtitle);
    EXPECT_TRUE(packet.text().has_value());
    EXPECT_TRUE(packet.text()->find("Subtitle Text") != std::string::npos);
}

TEST_F(TitlePacketTest, CreateActionbar)
{
    auto packet = TitlePacket::createActionbar(testText);

    EXPECT_EQ(packet.action(), TitleAction::Actionbar);
    EXPECT_TRUE(packet.text().has_value());
    EXPECT_EQ(packet.text().value(), testText);
}

TEST_F(TitlePacketTest, CreateActionbarFromComponent)
{
    auto component = std::make_unique<StringTextComponent>("Action Bar");
    auto packet = TitlePacket::createActionbar(*component);

    EXPECT_EQ(packet.action(), TitleAction::Actionbar);
    EXPECT_TRUE(packet.text().has_value());
    EXPECT_TRUE(packet.text()->find("Action Bar") != std::string::npos);
}

TEST_F(TitlePacketTest, CreateTimes)
{
    i32 fadeIn = 10;
    i32 stay = 70;
    i32 fadeOut = 20;

    auto packet = TitlePacket::createTimes(fadeIn, stay, fadeOut);

    EXPECT_EQ(packet.action(), TitleAction::Times);
    EXPECT_FALSE(packet.text().has_value());
    EXPECT_EQ(packet.fadeIn(), fadeIn);
    EXPECT_EQ(packet.stay(), stay);
    EXPECT_EQ(packet.fadeOut(), fadeOut);
}

TEST_F(TitlePacketTest, CreateClear)
{
    auto packet = TitlePacket::createClear();

    EXPECT_EQ(packet.action(), TitleAction::Clear);
    EXPECT_FALSE(packet.text().has_value());
}

TEST_F(TitlePacketTest, CreateReset)
{
    auto packet = TitlePacket::createReset();

    EXPECT_EQ(packet.action(), TitleAction::Reset);
    EXPECT_FALSE(packet.text().has_value());
}

TEST_F(TitlePacketTest, SettersAndGetters)
{
    TitlePacket packet;

    packet.setAction(TitleAction::Title);
    EXPECT_EQ(packet.action(), TitleAction::Title);

    packet.setText("New Text");
    EXPECT_TRUE(packet.text().has_value());
    EXPECT_EQ(packet.text().value(), "New Text");

    packet.setTimes(5, 10, 15);
    EXPECT_EQ(packet.fadeIn(), 5);
    EXPECT_EQ(packet.stay(), 10);
    EXPECT_EQ(packet.fadeOut(), 15);
}

// ==================== TitlePacket 序列化测试 ====================

class TitlePacketSerializeTest : public ::testing::Test {
protected:
    void SetUp() override { testText = R"({"text":"Test Title","bold":true})"; }

    std::string testText;
};

TEST_F(TitlePacketSerializeTest, SerializeDeserializeTitle)
{
    auto original = TitlePacket::createTitle(testText);

    auto result = original.serialize();
    ASSERT_TRUE(result.success()) << result.error().message();

    const auto& data = result.value();
    EXPECT_GT(data.size(), 0u);

    TitlePacket deserialized;
    auto deserResult = deserialized.deserialize(data.data(), data.size());
    EXPECT_TRUE(deserResult.success()) << deserResult.error().message();

    EXPECT_EQ(deserialized.action(), TitleAction::Title);
    EXPECT_TRUE(deserialized.text().has_value());
    EXPECT_EQ(deserialized.text().value(), testText);
}

TEST_F(TitlePacketSerializeTest, SerializeDeserializeSubtitle)
{
    auto original = TitlePacket::createSubtitle(testText);

    auto result = original.serialize();
    ASSERT_TRUE(result.success()) << result.error().message();

    TitlePacket deserialized;
    auto deserResult = deserialized.deserialize(result.value().data(), result.value().size());
    EXPECT_TRUE(deserResult.success()) << deserResult.error().message();

    EXPECT_EQ(deserialized.action(), TitleAction::Subtitle);
    EXPECT_TRUE(deserialized.text().has_value());
    EXPECT_EQ(deserialized.text().value(), testText);
}

TEST_F(TitlePacketSerializeTest, SerializeDeserializeActionbar)
{
    auto original = TitlePacket::createActionbar(testText);

    auto result = original.serialize();
    ASSERT_TRUE(result.success()) << result.error().message();

    TitlePacket deserialized;
    auto deserResult = deserialized.deserialize(result.value().data(), result.value().size());
    EXPECT_TRUE(deserResult.success()) << deserResult.error().message();

    EXPECT_EQ(deserialized.action(), TitleAction::Actionbar);
    EXPECT_TRUE(deserialized.text().has_value());
    EXPECT_EQ(deserialized.text().value(), testText);
}

TEST_F(TitlePacketSerializeTest, SerializeDeserializeTimes)
{
    i32 fadeIn = 10;
    i32 stay = 70;
    i32 fadeOut = 20;

    auto original = TitlePacket::createTimes(fadeIn, stay, fadeOut);

    auto result = original.serialize();
    ASSERT_TRUE(result.success()) << result.error().message();

    TitlePacket deserialized;
    auto deserResult = deserialized.deserialize(result.value().data(), result.value().size());
    EXPECT_TRUE(deserResult.success()) << deserResult.error().message();

    EXPECT_EQ(deserialized.action(), TitleAction::Times);
    EXPECT_FALSE(deserialized.text().has_value());
    EXPECT_EQ(deserialized.fadeIn(), fadeIn);
    EXPECT_EQ(deserialized.stay(), stay);
    EXPECT_EQ(deserialized.fadeOut(), fadeOut);
}

TEST_F(TitlePacketSerializeTest, SerializeDeserializeClear)
{
    auto original = TitlePacket::createClear();

    auto result = original.serialize();
    ASSERT_TRUE(result.success()) << result.error().message();

    TitlePacket deserialized;
    auto deserResult = deserialized.deserialize(result.value().data(), result.value().size());
    EXPECT_TRUE(deserResult.success()) << deserResult.error().message();

    EXPECT_EQ(deserialized.action(), TitleAction::Clear);
    EXPECT_FALSE(deserialized.text().has_value());
}

TEST_F(TitlePacketSerializeTest, SerializeDeserializeReset)
{
    auto original = TitlePacket::createReset();

    auto result = original.serialize();
    ASSERT_TRUE(result.success()) << result.error().message();

    TitlePacket deserialized;
    auto deserResult = deserialized.deserialize(result.value().data(), result.value().size());
    EXPECT_TRUE(deserResult.success()) << deserResult.error().message();

    EXPECT_EQ(deserialized.action(), TitleAction::Reset);
    EXPECT_FALSE(deserialized.text().has_value());
}

TEST_F(TitlePacketSerializeTest, SerializeDeserializeEmptyText)
{
    auto original = TitlePacket::createTitle("");

    auto result = original.serialize();
    ASSERT_TRUE(result.success()) << result.error().message();

    TitlePacket deserialized;
    auto deserResult = deserialized.deserialize(result.value().data(), result.value().size());
    EXPECT_TRUE(deserResult.success()) << deserResult.error().message();

    EXPECT_EQ(deserialized.action(), TitleAction::Title);
    EXPECT_TRUE(deserialized.text().has_value());
    EXPECT_EQ(deserialized.text().value(), "");
}

TEST_F(TitlePacketSerializeTest, SerializeDeserializeLongText)
{
    // 构造一个长JSON文本
    std::string longText = R"({"text":")";
    for (int i = 0; i < 100; ++i) {
        longText += "A";
    }
    longText += R"(","color":"gold"})";

    auto original = TitlePacket::createTitle(longText);

    auto result = original.serialize();
    ASSERT_TRUE(result.success()) << result.error().message();

    TitlePacket deserialized;
    auto deserResult = deserialized.deserialize(result.value().data(), result.value().size());
    EXPECT_TRUE(deserResult.success()) << deserResult.error().message();

    EXPECT_EQ(deserialized.action(), TitleAction::Title);
    EXPECT_TRUE(deserialized.text().has_value());
    EXPECT_EQ(deserialized.text().value(), longText);
}

TEST_F(TitlePacketSerializeTest, SerializeDeserializeZeroTimes)
{
    auto original = TitlePacket::createTimes(0, 0, 0);

    auto result = original.serialize();
    ASSERT_TRUE(result.success()) << result.error().message();

    TitlePacket deserialized;
    auto deserResult = deserialized.deserialize(result.value().data(), result.value().size());
    EXPECT_TRUE(deserResult.success()) << deserResult.error().message();

    EXPECT_EQ(deserialized.fadeIn(), 0);
    EXPECT_EQ(deserialized.stay(), 0);
    EXPECT_EQ(deserialized.fadeOut(), 0);
}

TEST_F(TitlePacketSerializeTest, SerializeDeserializeLargeTimes)
{
    auto original = TitlePacket::createTimes(1000, 5000, 1000);

    auto result = original.serialize();
    ASSERT_TRUE(result.success()) << result.error().message();

    TitlePacket deserialized;
    auto deserResult = deserialized.deserialize(result.value().data(), result.value().size());
    EXPECT_TRUE(deserResult.success()) << deserResult.error().message();

    EXPECT_EQ(deserialized.fadeIn(), 1000);
    EXPECT_EQ(deserialized.stay(), 5000);
    EXPECT_EQ(deserialized.fadeOut(), 1000);
}

// ==================== TitlePacket 错误处理测试 ====================

TEST(TitlePacketErrorTest, DeserializeEmptyData)
{
    TitlePacket packet;
    auto result = packet.deserialize(nullptr, 0);
    EXPECT_FALSE(result.success());
}

TEST(TitlePacketErrorTest, DeserializeTruncatedData)
{
    auto original = TitlePacket::createTitle("Test");
    auto result = original.serialize();
    ASSERT_TRUE(result.success());

    auto data = result.value();
    // 截断数据
    data.resize(data.size() / 2);

    TitlePacket deserialized;
    auto deserResult = deserialized.deserialize(data.data(), data.size());
    EXPECT_FALSE(deserResult.success());
}

TEST(TitlePacketErrorTest, DeserializeInvalidActionType)
{
    // 手动构造一个无效的动作类型
    mc::network::PacketSerializer serializer;
    serializer.writeVarInt(99); // 无效的动作类型

    std::vector<u8> data(serializer.data(), serializer.data() + serializer.size());

    TitlePacket packet;
    auto result = packet.deserialize(data.data(), data.size());
    EXPECT_FALSE(result.success());
}

// ==================== TitlePacket 与 ITextComponent 集成测试 ====================

TEST(TitlePacketComponentTest, CreateFromSimpleText)
{
    auto component = std::make_unique<StringTextComponent>("Simple Text");
    auto packet = TitlePacket::createTitle(*component);

    EXPECT_EQ(packet.action(), TitleAction::Title);
    EXPECT_TRUE(packet.text().has_value());
    EXPECT_TRUE(packet.text()->find("Simple Text") != std::string::npos);
}

TEST(TitlePacketComponentTest, CreateFromStyledText)
{
    auto component = std::make_unique<StringTextComponent>("Styled Text");
    Style style;
    style.setColor(TextFormatting::Gold);
    style.setBold(true);
    style.setItalic(true);
    component->setStyle(style);

    auto packet = TitlePacket::createTitle(*component);

    EXPECT_TRUE(packet.text().has_value());
    // 验证JSON包含样式信息
    EXPECT_TRUE(packet.text()->find("gold") != std::string::npos);
    EXPECT_TRUE(packet.text()->find("bold") != std::string::npos);
    EXPECT_TRUE(packet.text()->find("italic") != std::string::npos);
}

TEST(TitlePacketComponentTest, CreateFromNestedText)
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

    auto packet = TitlePacket::createTitle(*mainText);

    EXPECT_TRUE(packet.text().has_value());
    // 验证JSON包含嵌套组件
    EXPECT_TRUE(packet.text()->find("Main") != std::string::npos);
    EXPECT_TRUE(packet.text()->find("Extra") != std::string::npos);
}

// ==================== TitlePacket 所有动作类型测试 ====================

TEST(TitlePacketAllActionsTest, AllActionTypesSerialize)
{
    // 测试所有动作类型
    std::vector<TitleAction> actions = {TitleAction::Title,
        TitleAction::Subtitle,
        TitleAction::Actionbar,
        TitleAction::Times,
        TitleAction::Clear,
        TitleAction::Reset};

    for (auto action : actions) {
        TitlePacket packet;
        packet.setAction(action);

        if (action == TitleAction::Title || action == TitleAction::Subtitle || action == TitleAction::Actionbar) {
            packet.setText("Test");
        } else if (action == TitleAction::Times) {
            packet.setTimes(10, 70, 20);
        }

        auto result = packet.serialize();
        ASSERT_TRUE(result.success()) << "Failed to serialize action " << static_cast<int>(action);

        TitlePacket deserialized;
        auto deserResult = deserialized.deserialize(result.value().data(), result.value().size());
        ASSERT_TRUE(deserResult.success()) << "Failed to deserialize action " << static_cast<int>(action);
        EXPECT_EQ(deserialized.action(), action);
    }
}

// ==================== 性能测试 ====================

TEST(TitlePacketPerfTest, SerializeDeserializePerformance)
{
    auto packet = TitlePacket::createTitle("Performance Test Title");

    for (int i = 0; i < 1000; ++i) {
        auto result = packet.serialize();
        ASSERT_TRUE(result.success());
    }

    auto result = packet.serialize();
    for (int i = 0; i < 1000; ++i) {
        TitlePacket received;
        auto deserResult = received.deserialize(result.value().data(), result.value().size());
        ASSERT_TRUE(deserResult.success());
    }
}

TEST(TitlePacketPerfTest, AllActionTypesPerformance)
{
    for (int i = 0; i < 100; ++i) {
        // Title
        auto titlePacket = TitlePacket::createTitle("Title");
        auto result1 = titlePacket.serialize();
        ASSERT_TRUE(result1.success());

        // Subtitle
        auto subtitlePacket = TitlePacket::createSubtitle("Subtitle");
        auto result2 = subtitlePacket.serialize();
        ASSERT_TRUE(result2.success());

        // Actionbar
        auto actionbarPacket = TitlePacket::createActionbar("Actionbar");
        auto result3 = actionbarPacket.serialize();
        ASSERT_TRUE(result3.success());

        // Times
        auto timesPacket = TitlePacket::createTimes(10, 70, 20);
        auto result4 = timesPacket.serialize();
        ASSERT_TRUE(result4.success());

        // Clear
        auto clearPacket = TitlePacket::createClear();
        auto result5 = clearPacket.serialize();
        ASSERT_TRUE(result5.success());

        // Reset
        auto resetPacket = TitlePacket::createReset();
        auto result6 = resetPacket.serialize();
        ASSERT_TRUE(result6.success());
    }
}

// ==================== 命令场景模拟测试 ====================

TEST(TitlePacketCommandTest, SimulateTitleCommand)
{
    // 模拟 /title @a title {"text":"Welcome","color":"yellow"}
    std::string jsonText = R"({"text":"Welcome","color":"yellow"})";
    auto packet = TitlePacket::createTitle(jsonText);

    EXPECT_EQ(packet.action(), TitleAction::Title);
    EXPECT_TRUE(packet.text().has_value());

    auto result = packet.serialize();
    ASSERT_TRUE(result.success());

    // 验证序列化后可以正确反序列化
    TitlePacket received;
    auto deserResult = received.deserialize(result.value().data(), result.value().size());
    EXPECT_TRUE(deserResult.success());
    EXPECT_EQ(received.action(), TitleAction::Title);
    EXPECT_EQ(received.text().value(), jsonText);
}

TEST(TitlePacketCommandTest, SimulateTimesCommand)
{
    // 模拟 /title @a times 10 70 20
    auto packet = TitlePacket::createTimes(10, 70, 20);

    EXPECT_EQ(packet.action(), TitleAction::Times);
    EXPECT_EQ(packet.fadeIn(), 10);
    EXPECT_EQ(packet.stay(), 70);
    EXPECT_EQ(packet.fadeOut(), 20);

    auto result = packet.serialize();
    ASSERT_TRUE(result.success());
}

TEST(TitlePacketCommandTest, SimulateClearCommand)
{
    // 模拟 /title @a clear
    auto packet = TitlePacket::createClear();

    EXPECT_EQ(packet.action(), TitleAction::Clear);

    auto result = packet.serialize();
    ASSERT_TRUE(result.success());
}

TEST(TitlePacketCommandTest, SimulateResetCommand)
{
    // 模拟 /title @a reset
    auto packet = TitlePacket::createReset();

    EXPECT_EQ(packet.action(), TitleAction::Reset);

    auto result = packet.serialize();
    ASSERT_TRUE(result.success());
}

TEST(TitlePacketCommandTest, SimulateSubtitleCommand)
{
    // 模拟 /title @a subtitle {"text":"A Subtitle","color":"gray"}
    std::string jsonText = R"({"text":"A Subtitle","color":"gray"})";
    auto packet = TitlePacket::createSubtitle(jsonText);

    EXPECT_EQ(packet.action(), TitleAction::Subtitle);
    EXPECT_TRUE(packet.text().has_value());

    auto result = packet.serialize();
    ASSERT_TRUE(result.success());
}

TEST(TitlePacketCommandTest, SimulateActionbarCommand)
{
    // 模拟 /title @a actionbar {"text":"Action Bar Message"}
    std::string jsonText = R"({"text":"Action Bar Message"})";
    auto packet = TitlePacket::createActionbar(jsonText);

    EXPECT_EQ(packet.action(), TitleAction::Actionbar);
    EXPECT_TRUE(packet.text().has_value());

    auto result = packet.serialize();
    ASSERT_TRUE(result.success());
}

// ==================== 特殊字符和 Unicode 测试 ====================

TEST(TitlePacketSpecialCharTest, UnicodeCharacters)
{
    // 测试 Unicode 字符（中文、日文、表情符号等）
    std::string unicodeText = R"({"text":"你好世界 🌍 Привет мир"})";
    auto packet = TitlePacket::createTitle(unicodeText);

    EXPECT_EQ(packet.action(), TitleAction::Title);
    EXPECT_TRUE(packet.text().has_value());
    EXPECT_EQ(packet.text().value(), unicodeText);

    auto result = packet.serialize();
    ASSERT_TRUE(result.success());

    TitlePacket deserialized;
    auto deserResult = deserialized.deserialize(result.value().data(), result.value().size());
    EXPECT_TRUE(deserResult.success());
    EXPECT_EQ(deserialized.text().value(), unicodeText);
}

TEST(TitlePacketSpecialCharTest, JsonEscapeSequences)
{
    // 测试 JSON 转义序列
    std::string escapedText = R"({"text":"Line1\nLine2\tTabbed\"Quoted\""})";
    auto packet = TitlePacket::createTitle(escapedText);

    EXPECT_EQ(packet.action(), TitleAction::Title);

    auto result = packet.serialize();
    ASSERT_TRUE(result.success());

    TitlePacket deserialized;
    auto deserResult = deserialized.deserialize(result.value().data(), result.value().size());
    EXPECT_TRUE(deserResult.success());
    EXPECT_EQ(deserialized.text().value(), escapedText);
}

TEST(TitlePacketSpecialCharTest, MinecraftFormattingCodes)
{
    // 测试 Minecraft 格式代码（§ 符号）
    std::string mcText = R"({"text":"§cRed Text §lBold§r Reset"})";
    auto packet = TitlePacket::createTitle(mcText);

    EXPECT_EQ(packet.action(), TitleAction::Title);

    auto result = packet.serialize();
    ASSERT_TRUE(result.success());

    TitlePacket deserialized;
    auto deserResult = deserialized.deserialize(result.value().data(), result.value().size());
    EXPECT_TRUE(deserResult.success());
    EXPECT_EQ(deserialized.text().value(), mcText);
}

TEST(TitlePacketSpecialCharTest, ComplexJsonWithMultipleComponents)
{
    // 测试复杂的 JSON 文本组件（带 extra 数组）
    std::string complexText =
        R"({"text":"Main","color":"red","extra":[{"text":" ","color":"white"},{"text":"Extra","color":"gold","bold":true}]})";
    auto packet = TitlePacket::createTitle(complexText);

    EXPECT_EQ(packet.action(), TitleAction::Title);

    auto result = packet.serialize();
    ASSERT_TRUE(result.success());

    TitlePacket deserialized;
    auto deserResult = deserialized.deserialize(result.value().data(), result.value().size());
    EXPECT_TRUE(deserResult.success());
    EXPECT_EQ(deserialized.text().value(), complexText);
}

TEST(TitlePacketSpecialCharTest, SpecialMinecraftText)
{
    // 测试 Minecraft 特殊文本（如翻译键、记分板、选择器）
    std::string specialText = R"({"translate":"death.attack.player","with":["Player1","Player2"]})";
    auto packet = TitlePacket::createTitle(specialText);

    EXPECT_EQ(packet.action(), TitleAction::Title);

    auto result = packet.serialize();
    ASSERT_TRUE(result.success());

    TitlePacket deserialized;
    auto deserResult = deserialized.deserialize(result.value().data(), result.value().size());
    EXPECT_TRUE(deserResult.success());
    EXPECT_EQ(deserialized.text().value(), specialText);
}

// ==================== 边界条件测试 ====================

TEST(TitlePacketBoundaryTest, NegativeTimes)
{
    // 测试负数时间值（MC 1.16.5 使用负值表示使用默认值）
    auto packet = TitlePacket::createTimes(-1, -1, -1);

    EXPECT_EQ(packet.action(), TitleAction::Times);
    EXPECT_EQ(packet.fadeIn(), -1);
    EXPECT_EQ(packet.stay(), -1);
    EXPECT_EQ(packet.fadeOut(), -1);

    auto result = packet.serialize();
    ASSERT_TRUE(result.success());

    TitlePacket deserialized;
    auto deserResult = deserialized.deserialize(result.value().data(), result.value().size());
    EXPECT_TRUE(deserResult.success());
    EXPECT_EQ(deserialized.fadeIn(), -1);
    EXPECT_EQ(deserialized.stay(), -1);
    EXPECT_EQ(deserialized.fadeOut(), -1);
}

TEST(TitlePacketBoundaryTest, MaxIntTimes)
{
    // 测试最大整数值
    auto packet = TitlePacket::createTimes(2147483647, 2147483647, 2147483647);

    EXPECT_EQ(packet.action(), TitleAction::Times);
    EXPECT_EQ(packet.fadeIn(), 2147483647);
    EXPECT_EQ(packet.stay(), 2147483647);
    EXPECT_EQ(packet.fadeOut(), 2147483647);

    auto result = packet.serialize();
    ASSERT_TRUE(result.success());

    TitlePacket deserialized;
    auto deserResult = deserialized.deserialize(result.value().data(), result.value().size());
    EXPECT_TRUE(deserResult.success());
    EXPECT_EQ(deserialized.fadeIn(), 2147483647);
    EXPECT_EQ(deserialized.stay(), 2147483647);
    EXPECT_EQ(deserialized.fadeOut(), 2147483647);
}

TEST(TitlePacketBoundaryTest, ActionbarWithEmptyText)
{
    // 测试空文本的动作栏
    auto packet = TitlePacket::createActionbar("");

    EXPECT_EQ(packet.action(), TitleAction::Actionbar);
    EXPECT_TRUE(packet.text().has_value());
    EXPECT_EQ(packet.text().value(), "");

    auto result = packet.serialize();
    ASSERT_TRUE(result.success());

    TitlePacket deserialized;
    auto deserResult = deserialized.deserialize(result.value().data(), result.value().size());
    EXPECT_TRUE(deserResult.success());
    EXPECT_EQ(deserialized.action(), TitleAction::Actionbar);
    EXPECT_TRUE(deserialized.text().has_value());
    EXPECT_EQ(deserialized.text().value(), "");
}

TEST(TitlePacketBoundaryTest, RapidActionSequence)
{
    // 模拟快速连续发送不同类型的标题包
    std::vector<TitlePacket> packets;

    packets.push_back(TitlePacket::createTimes(10, 70, 20));
    packets.push_back(TitlePacket::createTitle(R"({"text":"Title 1"})"));
    packets.push_back(TitlePacket::createSubtitle(R"({"text":"Subtitle 1"})"));
    packets.push_back(TitlePacket::createClear());
    packets.push_back(TitlePacket::createTitle(R"({"text":"Title 2"})"));
    packets.push_back(TitlePacket::createReset());
    packets.push_back(TitlePacket::createActionbar(R"({"text":"Action Bar"})"));

    // 序列化和反序列化所有包
    for (auto& packet : packets) {
        auto result = packet.serialize();
        ASSERT_TRUE(result.success());

        TitlePacket deserialized;
        auto deserResult = deserialized.deserialize(result.value().data(), result.value().size());
        EXPECT_TRUE(deserResult.success());
        EXPECT_EQ(deserialized.action(), packet.action());
    }
}
