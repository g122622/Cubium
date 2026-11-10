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

#include "util/text/TextStyle.hpp"
#include "core/Types.hpp"
#include "util/text/StyleNbt.hpp"
#include "util/text/TextEvents.hpp"
#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

using namespace mc::text;

// ============================================================================
// TextFormatting 测试
// ============================================================================

class TextFormattingTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(TextFormattingTest, GetFormattingColor)
{
    // 颜色代码
    EXPECT_EQ(getFormattingColor(TextFormatting::Black), 0xFF000000u);
    EXPECT_EQ(getFormattingColor(TextFormatting::Red), 0xFFFF5555u);
    EXPECT_EQ(getFormattingColor(TextFormatting::White), 0xFFFFFFFFu);
    EXPECT_EQ(getFormattingColor(TextFormatting::Gold), 0xFFFFAA00u);

    // 样式代码返回白色
    EXPECT_EQ(getFormattingColor(TextFormatting::Bold), 0xFFFFFFFFu);
    EXPECT_EQ(getFormattingColor(TextFormatting::Italic), 0xFFFFFFFFu);

    // None 返回白色
    EXPECT_EQ(getFormattingColor(TextFormatting::None), 0xFFFFFFFFu);
}

TEST_F(TextFormattingTest, IsColor)
{
    // 颜色代码
    EXPECT_TRUE(isColor(TextFormatting::Black));
    EXPECT_TRUE(isColor(TextFormatting::Red));
    EXPECT_TRUE(isColor(TextFormatting::White));
    EXPECT_TRUE(isColor(TextFormatting::Aqua));

    // 样式代码
    EXPECT_FALSE(isColor(TextFormatting::Bold));
    EXPECT_FALSE(isColor(TextFormatting::Italic));
    EXPECT_FALSE(isColor(TextFormatting::Underline));
    EXPECT_FALSE(isColor(TextFormatting::Reset));

    // None
    EXPECT_FALSE(isColor(TextFormatting::None));
}

TEST_F(TextFormattingTest, IsStyle)
{
    // 样式代码
    EXPECT_TRUE(isStyle(TextFormatting::Bold));
    EXPECT_TRUE(isStyle(TextFormatting::Italic));
    EXPECT_TRUE(isStyle(TextFormatting::Underline));
    EXPECT_TRUE(isStyle(TextFormatting::Strikethrough));
    EXPECT_TRUE(isStyle(TextFormatting::Obfuscated));

    // 颜色代码
    EXPECT_FALSE(isStyle(TextFormatting::Red));
    EXPECT_FALSE(isStyle(TextFormatting::White));

    // Reset 和 None
    EXPECT_FALSE(isStyle(TextFormatting::Reset));
    EXPECT_FALSE(isStyle(TextFormatting::None));
}

TEST_F(TextFormattingTest, FromCode)
{
    // 数字代码
    EXPECT_EQ(fromCode('0'), TextFormatting::Black);
    EXPECT_EQ(fromCode('9'), TextFormatting::Blue);
    EXPECT_EQ(fromCode('a'), TextFormatting::Green);
    EXPECT_EQ(fromCode('f'), TextFormatting::White);

    // 样式代码
    EXPECT_EQ(fromCode('l'), TextFormatting::Bold);
    EXPECT_EQ(fromCode('o'), TextFormatting::Italic);
    EXPECT_EQ(fromCode('k'), TextFormatting::Obfuscated);
    EXPECT_EQ(fromCode('r'), TextFormatting::Reset);

    // 大写代码
    EXPECT_EQ(fromCode('A'), TextFormatting::Green);
    EXPECT_EQ(fromCode('L'), TextFormatting::Bold);

    // 无效代码
    EXPECT_EQ(fromCode('x'), TextFormatting::None);
    EXPECT_EQ(fromCode('z'), TextFormatting::None);
}

TEST_F(TextFormattingTest, ToCode)
{
    // 颜色代码
    EXPECT_EQ(toCode(TextFormatting::Black), '0');
    EXPECT_EQ(toCode(TextFormatting::Red), 'c');
    EXPECT_EQ(toCode(TextFormatting::White), 'f');

    // 样式代码
    EXPECT_EQ(toCode(TextFormatting::Bold), 'l');
    EXPECT_EQ(toCode(TextFormatting::Italic), 'o');
    EXPECT_EQ(toCode(TextFormatting::Reset), 'r');

    // None
    EXPECT_EQ(toCode(TextFormatting::None), '\0');
}

TEST_F(TextFormattingTest, FromName)
{
    // 颜色名称
    EXPECT_EQ(fromName("red"), TextFormatting::Red);
    EXPECT_EQ(fromName("dark_blue"), TextFormatting::DarkBlue);
    EXPECT_EQ(fromName("gold"), TextFormatting::Gold);
    EXPECT_EQ(fromName("white"), TextFormatting::White);

    // 样式名称
    EXPECT_EQ(fromName("bold"), TextFormatting::Bold);
    EXPECT_EQ(fromName("italic"), TextFormatting::Italic);

    // 大写名称
    EXPECT_EQ(fromName("RED"), TextFormatting::Red);
    EXPECT_EQ(fromName("BOLD"), TextFormatting::Bold);

    // 无效名称
    EXPECT_EQ(fromName("invalid"), TextFormatting::None);
}

TEST_F(TextFormattingTest, ToName)
{
    EXPECT_EQ(toName(TextFormatting::Red), "red");
    EXPECT_EQ(toName(TextFormatting::DarkBlue), "dark_blue");
    EXPECT_EQ(toName(TextFormatting::Bold), "bold");
    EXPECT_EQ(toName(TextFormatting::None), "");
}

// ============================================================================
// Style 测试
// ============================================================================

class StyleTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(StyleTest, DefaultValues)
{
    Style style;

    EXPECT_FALSE(style.getColor().has_value());
    EXPECT_FALSE(style.isBold());
    EXPECT_FALSE(style.isItalic());
    EXPECT_FALSE(style.isUnderlined());
    EXPECT_FALSE(style.isStrikethrough());
    EXPECT_FALSE(style.isObfuscated());
    EXPECT_FALSE(style.hasEvents());
}

TEST_F(StyleTest, SetColor)
{
    Style style;

    style.setColor(TextFormatting::Red);
    EXPECT_TRUE(style.getColor().has_value());
    EXPECT_EQ(*style.getColor(), TextFormatting::Red);
    EXPECT_EQ(style.getColorARGB(), 0xFFFF5555u);

    style.setColor(std::nullopt);
    EXPECT_FALSE(style.getColor().has_value());
    EXPECT_EQ(style.getColorARGB(), 0xFFFFFFFFu);
}

TEST_F(StyleTest, SetStyles)
{
    Style style;

    style.setBold(true);
    EXPECT_TRUE(style.isBold());

    style.setItalic(true);
    EXPECT_TRUE(style.isItalic());

    style.setUnderlined(true);
    EXPECT_TRUE(style.isUnderlined());

    style.setStrikethrough(true);
    EXPECT_TRUE(style.isStrikethrough());

    style.setObfuscated(true);
    EXPECT_TRUE(style.isObfuscated());
}

TEST_F(StyleTest, MergeWithParent)
{
    // 父样式：红色 + 粗体
    Style parent;
    parent.setColor(TextFormatting::Red);
    parent.setBold(true);

    // 子样式：斜体
    Style child;
    child.setItalic(true);

    // 合并
    Style merged = child.mergeWithParent(parent);
    EXPECT_EQ(*merged.getColor(), TextFormatting::Red);
    EXPECT_TRUE(merged.isBold());
    EXPECT_TRUE(merged.isItalic());
    EXPECT_FALSE(merged.isUnderlined());

    // 子样式覆盖颜色
    Style childWithColor;
    childWithColor.setColor(TextFormatting::Blue);
    Style merged2 = childWithColor.mergeWithParent(parent);
    EXPECT_EQ(*merged2.getColor(), TextFormatting::Blue);
    EXPECT_TRUE(merged2.isBold());
}

TEST_F(StyleTest, IsEmpty)
{
    Style empty;
    EXPECT_TRUE(empty.isEmpty());

    Style withColor;
    withColor.setColor(TextFormatting::Red);
    EXPECT_FALSE(withColor.isEmpty());

    Style withBold;
    withBold.setBold(true);
    EXPECT_FALSE(withBold.isEmpty());
}

TEST_F(StyleTest, JsonSerialization)
{
    // 序列化
    Style style;
    style.setColor(TextFormatting::Red);
    style.setBold(true);
    style.setItalic(true);

    nlohmann::json json = style.toJson();
    EXPECT_EQ(json["color"], "red");
    EXPECT_TRUE(json["bold"].get<bool>());
    EXPECT_TRUE(json["italic"].get<bool>());

    // 反序列化
    Style parsed = Style::fromJson(json);
    EXPECT_EQ(*parsed.getColor(), TextFormatting::Red);
    EXPECT_TRUE(parsed.isBold());
    EXPECT_TRUE(parsed.isItalic());
    EXPECT_FALSE(parsed.isUnderlined());
}

TEST_F(StyleTest, Equality)
{
    Style style1;
    style1.setColor(TextFormatting::Red);
    style1.setBold(true);

    Style style2;
    style2.setColor(TextFormatting::Red);
    style2.setBold(true);

    Style style3;
    style3.setColor(TextFormatting::Red);
    style3.setItalic(true);

    EXPECT_EQ(style1, style2);
    EXPECT_NE(style1, style3);
}

// ============================================================================
// 阴影色与字体（原版 Style.shadowColor / Style.font）
// ============================================================================

TEST_F(StyleTest, ShadowColorAndFontAreThreeStateFields)
{
    Style style;
    EXPECT_FALSE(style.getShadowColor().has_value());
    EXPECT_FALSE(style.getFont().has_value());

    // 阴影色是 ARGB 打包整数，允许负值（高位 alpha 置 1）。
    style.setShadowColor(-16777216);
    style.setFont("minecraft:uniform");
    EXPECT_EQ(*style.getShadowColor(), -16777216);
    EXPECT_EQ(*style.getFont(), "minecraft:uniform");
    EXPECT_FALSE(style.isEmpty()) << "shadow color alone must make a style non-empty";

    Style fontOnly;
    fontOnly.setFont("minecraft:default");
    EXPECT_FALSE(fontOnly.isEmpty()) << "font alone must make a style non-empty";

    // 相等性必须覆盖这两个新字段，否则两个仅阴影色不同的样式会被判为相等。
    Style same;
    same.setShadowColor(-16777216);
    same.setFont("minecraft:uniform");
    EXPECT_EQ(style, same);
    same.setShadowColor(0);
    EXPECT_NE(style, same);
}

TEST_F(StyleTest, ShadowColorAndFontParticipateInInheritance)
{
    Style parent;
    parent.setShadowColor(0x00FFFFFF);
    parent.setFont("minecraft:uniform");

    Style child;
    child.setBold(true);
    const Style merged = child.mergeWithParent(parent);
    ASSERT_TRUE(merged.getShadowColor().has_value());
    ASSERT_TRUE(merged.getFont().has_value());
    EXPECT_EQ(*merged.getShadowColor(), 0x00FFFFFF);
    EXPECT_EQ(*merged.getFont(), "minecraft:uniform");

    // 子样式显式给出时优先。
    Style overriding;
    overriding.setShadowColor(0);
    overriding.setFont("minecraft:alt");
    const Style overridden = overriding.mergeWithParent(parent);
    EXPECT_EQ(*overridden.getShadowColor(), 0);
    EXPECT_EQ(*overridden.getFont(), "minecraft:alt");

    // applyTo 同样是「本样式优先、other 兜底」。
    const Style applied = parent.applyTo(child);
    EXPECT_EQ(*applied.getShadowColor(), 0x00FFFFFF);
    EXPECT_EQ(*applied.getFont(), "minecraft:uniform");
}

TEST_F(StyleTest, ShadowColorAndFontSurviveJsonRoundTrip)
{
    Style style;
    style.setShadowColor(-16777216);
    style.setFont("minecraft:uniform");

    const nlohmann::json json = style.toJson();
    // 原版 `Style.Serializer.MAP_CODEC`：shadow_color 是整数，font 是字符串。
    ASSERT_TRUE(json.contains("shadow_color"));
    ASSERT_TRUE(json["shadow_color"].is_number_integer());
    EXPECT_EQ(json["shadow_color"].get<mc::i32>(), -16777216);
    ASSERT_TRUE(json.contains("font"));
    EXPECT_EQ(json["font"].get<std::string>(), "minecraft:uniform");

    const Style parsed = Style::fromJson(json);
    EXPECT_EQ(parsed, style);
}

// 这两个字段此前完全缺失，读入带它们的文本再写回会静默丢字段——属于数据丢失而非显示差异。
TEST_F(StyleTest, ShadowColorAndFontSurviveNbtRoundTrip)
{
    Style style;
    style.setColor(TextFormatting::Red);
    style.setShadowColor(-16777216);
    style.setFont("minecraft:uniform");

    mc::nbt::tags::compound_tag tag;
    styleToNbt(tag, style);

    // compound_tag::get<T> 直接返回底层值（int_tag → int、string_tag → std::string）。
    EXPECT_EQ(tag.get<mc::nbt::tags::int_tag>("shadow_color"), -16777216);
    EXPECT_EQ(tag.get<mc::nbt::tags::string_tag>("font"), "minecraft:uniform");

    const auto parsed = styleFromNbt(tag);
    ASSERT_TRUE(parsed.success()) << parsed.error().message();
    EXPECT_EQ(parsed.value(), style);
}

// ============================================================================
// ClickEvent 测试
// ============================================================================

class ClickEventTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(ClickEventTest, BasicOperations)
{
    ClickEvent event(ClickAction::RunCommand, "/help");

    EXPECT_EQ(event.getAction(), ClickAction::RunCommand);
    EXPECT_EQ(event.getValue(), "/help");
    EXPECT_TRUE(event.isValid());

    ClickEvent emptyEvent;
    EXPECT_FALSE(emptyEvent.isValid());
}

TEST_F(ClickEventTest, JsonSerialization)
{
    ClickEvent event(ClickAction::OpenUrl, "https://example.com");
    nlohmann::json json = event.toJson();

    EXPECT_EQ(json["action"], "open_url");
    EXPECT_EQ(json["value"], "https://example.com");

    ClickEvent parsed = ClickEvent::fromJson(json);
    EXPECT_EQ(parsed.getAction(), ClickAction::OpenUrl);
    EXPECT_EQ(parsed.getValue(), "https://example.com");
}

// ============================================================================
// HoverEvent 测试
// ============================================================================

class HoverEventTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(HoverEventTest, BasicOperations)
{
    HoverEvent event = HoverEvent::showText("Hello World");

    EXPECT_EQ(event.getAction(), HoverAction::ShowText);
    EXPECT_EQ(event.getValue(), "Hello World");
    EXPECT_TRUE(event.isValid());

    HoverEvent emptyEvent;
    EXPECT_FALSE(emptyEvent.isValid());
}

TEST_F(HoverEventTest, JsonSerialization)
{
    HoverEvent event = HoverEvent::showText("Test hover");
    nlohmann::json json = event.toJson();

    EXPECT_EQ(json["action"], "show_text");
    EXPECT_EQ(json["value"], "Test hover");

    HoverEvent parsed = HoverEvent::fromJson(json);
    EXPECT_EQ(parsed.getAction(), HoverAction::ShowText);
    EXPECT_EQ(parsed.getValue(), "Test hover");
}

// ============================================================================
// GetStyleCodes 测试
// ============================================================================

TEST_F(StyleTest, GetStyleCodes)
{
    Style style;
    style.setColor(TextFormatting::Red);
    style.setBold(true);

    std::string codes = getStyleCodes(style);
    EXPECT_EQ(codes, "§c§l");

    Style style2;
    style2.setColor(TextFormatting::Blue);
    style2.setItalic(true);
    style2.setUnderlined(true);

    std::string codes2 = getStyleCodes(style2);
    EXPECT_EQ(codes2, "§9§o§n");
}
