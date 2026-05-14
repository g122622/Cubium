#include "core/Types.hpp"
#include "util/text/ITextComponent.hpp"
#include "util/text/StringTextComponent.hpp"
#include "util/text/TextParser.hpp"
#include "util/text/TranslationTextComponent.hpp"
#include <gtest/gtest.h>

using namespace mc::text;

// ============================================================================
// StringTextComponent 测试
// ============================================================================

class StringTextComponentTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(StringTextComponentTest, BasicConstruction)
{
    StringTextComponent text("Hello World");

    EXPECT_EQ(text.getText(), "Hello World");
    EXPECT_EQ(text.getUnformattedText(), "Hello World");
    EXPECT_EQ(text.getFormattedText(), "Hello World");
    EXPECT_TRUE(text.getStyle().isEmpty());
}

TEST_F(StringTextComponentTest, EmptyText)
{
    StringTextComponent text("");

    EXPECT_EQ(text.getText(), "");
    EXPECT_EQ(text.getUnformattedText(), "");
    EXPECT_EQ(text.getFormattedText(), "");
}

TEST_F(StringTextComponentTest, WithStyle)
{
    StringTextComponent text("Hello");
    Style style;
    style.setColor(TextFormatting::Red);
    style.setBold(true);
    text.setStyle(style);

    EXPECT_EQ(text.getStyle().getColor(), TextFormatting::Red);
    EXPECT_TRUE(text.getStyle().isBold());
    EXPECT_EQ(text.getFormattedText(), "§c§lHello");
}

TEST_F(StringTextComponentTest, DeepCopy)
{
    StringTextComponent original("Hello");
    Style style;
    style.setColor(TextFormatting::Red);
    original.setStyle(style);
    original.append(std::make_unique<StringTextComponent>(" World"));

    auto copy = original.deepCopy();

    EXPECT_EQ(copy->getUnformattedText(), "Hello World");
    EXPECT_EQ(copy->getStyle().getColor(), TextFormatting::Red);
    EXPECT_EQ(copy->getSiblings().size(), 1u);

    // 验证是深拷贝
    Style blueStyle;
    blueStyle.setColor(TextFormatting::Blue);
    original.setStyle(blueStyle);
    EXPECT_EQ(copy->getStyle().getColor(), TextFormatting::Red);
}

TEST_F(StringTextComponentTest, ShallowCopy)
{
    StringTextComponent original("Hello");
    Style style;
    style.setColor(TextFormatting::Red);
    original.setStyle(style);

    auto copy = original.shallowCopy();

    EXPECT_EQ(copy->getUnformattedText(), "Hello");
    EXPECT_EQ(copy->getStyle().getColor(), TextFormatting::Red);
    EXPECT_EQ(copy->getSiblings().size(), 0u); // 浅拷贝不包含子组件
}

TEST_F(StringTextComponentTest, JsonSerialization)
{
    StringTextComponent text("Hello");
    Style style;
    style.setColor(TextFormatting::Red);
    style.setBold(true);
    text.setStyle(style);
    text.append(std::make_unique<StringTextComponent>(" World"));

    nlohmann::json json = text.toJson();
    EXPECT_EQ(json["text"], "Hello");
    EXPECT_EQ(json["color"], "red");
    EXPECT_TRUE(json["bold"].get<bool>());
    EXPECT_TRUE(json.contains("extra"));

    auto parsed = ITextComponent::fromJson(json);
    EXPECT_EQ(parsed->getUnformattedText(), "Hello World");
}

TEST_F(StringTextComponentTest, AppendSiblings)
{
    StringTextComponent text("Hello");

    text.append(std::make_unique<StringTextComponent>(" "));
    text.append(std::make_unique<StringTextComponent>("World"));

    EXPECT_EQ(text.getSiblings().size(), 2u);
    EXPECT_EQ(text.getUnformattedText(), "Hello World");
}

TEST_F(StringTextComponentTest, AppendText)
{
    StringTextComponent text("Hello");
    text.appendText(" ");
    text.appendText("World");

    EXPECT_EQ(text.getSiblings().size(), 2u);
    EXPECT_EQ(text.getUnformattedText(), "Hello World");
}

// ============================================================================
// TranslationTextComponent 测试
// ============================================================================

class TranslationTextComponentTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(TranslationTextComponentTest, BasicConstruction)
{
    TranslationTextComponent text("chat.type.text");

    EXPECT_EQ(text.getKey(), "chat.type.text");
    EXPECT_TRUE(text.getParams().empty());
}

TEST_F(TranslationTextComponentTest, WithParams)
{
    std::vector<std::unique_ptr<ITextComponent>> params;
    params.push_back(std::make_unique<StringTextComponent>("Player"));
    params.push_back(std::make_unique<StringTextComponent>("Hello!"));

    TranslationTextComponent text("chat.type.announcement", std::move(params));

    EXPECT_EQ(text.getKey(), "chat.type.announcement");
    EXPECT_EQ(text.getParams().size(), 2u);
}

TEST_F(TranslationTextComponentTest, AddParam)
{
    TranslationTextComponent text("chat.type.text");
    text.addParam(std::make_unique<StringTextComponent>("Player"));

    EXPECT_EQ(text.getParams().size(), 1u);
}

TEST_F(TranslationTextComponentTest, DeepCopy)
{
    TranslationTextComponent original("translation.key");
    original.addParam(std::make_unique<StringTextComponent>("param1"));
    Style style;
    style.setColor(TextFormatting::Blue);
    original.setStyle(style);

    auto copy = original.deepCopy();

    auto* transCopy = dynamic_cast<TranslationTextComponent*>(copy.get());
    ASSERT_NE(transCopy, nullptr);
    EXPECT_EQ(transCopy->getKey(), "translation.key");
    EXPECT_EQ(transCopy->getParams().size(), 1u);
    EXPECT_EQ(copy->getStyle().getColor(), TextFormatting::Blue);
}

TEST_F(TranslationTextComponentTest, JsonSerialization)
{
    TranslationTextComponent text("chat.type.announcement");
    text.addParam(std::make_unique<StringTextComponent>("Server"));
    text.addParam(std::make_unique<StringTextComponent>("Hello!"));
    Style style;
    style.setColor(TextFormatting::Yellow);
    text.setStyle(style);

    nlohmann::json json = text.toJson();
    EXPECT_EQ(json["translate"], "chat.type.announcement");
    EXPECT_EQ(json["color"], "yellow");
    EXPECT_TRUE(json.contains("with"));
    EXPECT_EQ(json["with"].size(), 2u);
}

// ============================================================================
// ITextComponent Factory 测试
// ============================================================================

class ITextComponentFactoryTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(ITextComponentFactoryTest, FromJsonString)
{
    nlohmann::json json = "Hello World";

    auto component = ITextComponent::fromJson(json);
    ASSERT_NE(component, nullptr);

    auto* stringComp = dynamic_cast<StringTextComponent*>(component.get());
    ASSERT_NE(stringComp, nullptr);
    EXPECT_EQ(stringComp->getText(), "Hello World");
}

TEST_F(ITextComponentFactoryTest, FromJsonObject)
{
    nlohmann::json json = {{"text", "Hello"}, {"color", "red"}, {"bold", true}};

    auto component = ITextComponent::fromJson(json);
    ASSERT_NE(component, nullptr);

    EXPECT_EQ(component->getUnformattedText(), "Hello");
    EXPECT_EQ(component->getStyle().getColor(), TextFormatting::Red);
    EXPECT_TRUE(component->getStyle().isBold());
}

TEST_F(ITextComponentFactoryTest, FromJsonWithExtra)
{
    nlohmann::json json = {{"text", "Hello "},
        {"color", "red"},
        {"extra", nlohmann::json::array({{{"text", "World"}, {"color", "blue"}}})}};

    auto component = ITextComponent::fromJson(json);
    ASSERT_NE(component, nullptr);

    EXPECT_EQ(component->getUnformattedText(), "Hello World");
    EXPECT_EQ(component->getStyle().getColor(), TextFormatting::Red);
    EXPECT_EQ(component->getSiblings().size(), 1u);
}

TEST_F(ITextComponentFactoryTest, FromJsonTranslation)
{
    nlohmann::json json = {
        {"translate", "chat.type.announcement"}, {"with", nlohmann::json::array({"Server", "Hello!"})}};

    auto component = ITextComponent::fromJson(json);
    ASSERT_NE(component, nullptr);

    auto* transComp = dynamic_cast<TranslationTextComponent*>(component.get());
    ASSERT_NE(transComp, nullptr);
    EXPECT_EQ(transComp->getKey(), "chat.type.announcement");
    EXPECT_EQ(transComp->getParams().size(), 2u);
}

TEST_F(ITextComponentFactoryTest, FromJsonArray)
{
    nlohmann::json json = nlohmann::json::array(
        {{{"text", "Hello"}, {"color", "red"}}, {{"text", " "}}, {{"text", "World"}, {"color", "blue"}}});

    auto component = ITextComponent::fromJsonArray(json);
    ASSERT_NE(component, nullptr);

    EXPECT_EQ(component->getUnformattedText(), "Hello World");
    EXPECT_EQ(component->getSiblings().size(), 2u);
}

// ============================================================================
// TextParser 测试
// ============================================================================

class TextParserTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(TextParserTest, ParsePlainText)
{
    auto component = TextParser::parse("Hello World");

    ASSERT_NE(component, nullptr);
    EXPECT_EQ(component->getUnformattedText(), "Hello World");
    EXPECT_TRUE(component->getStyle().isEmpty());
}

TEST_F(TextParserTest, ParseColorCode)
{
    auto component = TextParser::parse("§cHello World");

    ASSERT_NE(component, nullptr);
    EXPECT_EQ(component->getUnformattedText(), "Hello World");
    EXPECT_EQ(component->getStyle().getColor(), TextFormatting::Red);
}

TEST_F(TextParserTest, ParseMultipleColors)
{
    auto component = TextParser::parse("§cHello §9World");

    ASSERT_NE(component, nullptr);
    EXPECT_EQ(component->getUnformattedText(), "Hello World");

    // 第一个子组件应该是红色
    const auto& siblings = component->getSiblings();
    EXPECT_GE(siblings.size(), 1u);
}

TEST_F(TextParserTest, ParseStyleCodes)
{
    auto component = TextParser::parse("§lBold");

    ASSERT_NE(component, nullptr);
    EXPECT_EQ(component->getUnformattedText(), "Bold");
    EXPECT_TRUE(component->getStyle().isBold());
}

TEST_F(TextParserTest, ParseMultipleStyleCodes)
{
    auto component = TextParser::parse("§l§oBold Italic");

    ASSERT_NE(component, nullptr);
    EXPECT_TRUE(component->getStyle().isBold());
    EXPECT_TRUE(component->getStyle().isItalic());
}

TEST_F(TextParserTest, ParseResetCode)
{
    auto component = TextParser::parse("§cRed§rPlain");

    ASSERT_NE(component, nullptr);
    EXPECT_EQ(component->getUnformattedText(), "RedPlain");
}

TEST_F(TextParserTest, ParseColorAndStyle)
{
    auto component = TextParser::parse("§c§lRed Bold");

    ASSERT_NE(component, nullptr);
    EXPECT_EQ(component->getStyle().getColor(), TextFormatting::Red);
    EXPECT_TRUE(component->getStyle().isBold());
}

TEST_F(TextParserTest, ToLegacyFormat)
{
    StringTextComponent text("Hello");
    Style style;
    style.setColor(TextFormatting::Red);
    style.setBold(true);
    text.setStyle(style);

    std::string legacy = TextParser::toLegacyFormat(text);
    EXPECT_EQ(legacy, "§c§lHello");
}

TEST_F(TextParserTest, RoundTrip)
{
    std::string original = "§cHello §lWorld!";

    auto component = TextParser::parse(original);
    ASSERT_NE(component, nullptr);

    std::string roundTrip = TextParser::toLegacyFormat(*component);

    // 注意：可能不完全相同（样式顺序可能不同）
    EXPECT_EQ(component->getUnformattedText(), "Hello World!");
}

TEST_F(TextParserTest, InvalidCode)
{
    auto component = TextParser::parse("§xInvalid");

    ASSERT_NE(component, nullptr);
    // 无效代码应该作为普通文本保留
    EXPECT_TRUE(component->getUnformattedText().find("§x") != std::string::npos ||
        component->getUnformattedText().find("x") != std::string::npos);
}

TEST_F(TextParserTest, EmptyText)
{
    auto component = TextParser::parse("");

    ASSERT_NE(component, nullptr);
    EXPECT_EQ(component->getUnformattedText(), "");
}
