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

/**
 * @file StyleArgumentTest.cpp
 * @brief StyleArgumentType 单元测试
 *
 * 覆盖：
 * - SNBT 复合标签解析（布尔/字节/字符串颜色/自定义 RGB/insertion）
 * - 花括号内空白与引号内空白、括号的切分（命令参数按空白分隔，但 SNBT 内部允许空白）
 * - 解析失败（未配平 / 非法 SNBT / 非法颜色）必须抛 CommandError 而不是静默成功
 * - 空样式 `{}`（会被包成非空指针，不能与「没有命令」混淆）
 */

#include <gtest/gtest.h>

#include "common/command/StringReader.hpp"
#include "common/command/arguments/StyleArgument.hpp"
#include "common/command/errors/CommandErrors.hpp"
#include "common/util/text/TextStyle.hpp"

using mc::command::CommandError;
using mc::command::StringReader;
using mc::command::StyleArgumentType;
using mc::command::StylePtr;

namespace {

/// 解析一整段文本（断言游标正好停在末尾）
StylePtr parseAll(StyleArgumentType& parser, StringReader& reader)
{
    auto styleResult = parser.parse(reader);
    EXPECT_TRUE(styleResult.success()) << styleResult.error().toString();
    EXPECT_EQ(reader.getCursor(), reader.getTotalLength());
    if (styleResult.failed()) {
        return nullptr;
    }
    return std::move(styleResult).value();
}

} // namespace

TEST(StyleArgumentTypeTest, ParsesEmptyCompound)
{
    StyleArgumentType parser;
    StringReader reader("{}");
    const auto style = parseAll(parser, reader);

    // 空样式仍然解析成功，且必须是非空指针（否则会被命令树当成「这一层没有命令」）
    ASSERT_NE(style, nullptr);
    EXPECT_TRUE(style->isEmpty());
}

TEST(StyleArgumentTypeTest, ParsesBoldBoolean)
{
    StyleArgumentType parser;
    StringReader reader("{bold:true}");
    const auto style = parseAll(parser, reader);

    ASSERT_NE(style, nullptr);
    EXPECT_EQ(style->bold(), true);
}

TEST(StyleArgumentTypeTest, ParsesByteSuffixExamples)
{
    // 原版示例里两种写法都常见：1b（SNBT 字节）与 true（布尔字面量）
    StyleArgumentType parser;
    StringReader reader("{italic:1b,underlined:0b}");
    const auto style = parseAll(parser, reader);

    ASSERT_NE(style, nullptr);
    EXPECT_EQ(style->italic(), true);
    EXPECT_EQ(style->underlined(), false);
}

TEST(StyleArgumentTypeTest, ParsesNamedColor)
{
    StyleArgumentType parser;
    StringReader reader("{color:\"red\"}");
    const auto style = parseAll(parser, reader);

    ASSERT_NE(style, nullptr);
    EXPECT_EQ(style->getColor(), mc::text::TextFormatting::Red);
}

TEST(StyleArgumentTypeTest, ParsesCustomRgbColor)
{
    StyleArgumentType parser;
    StringReader reader("{color:\"#FF5555\"}");
    const auto style = parseAll(parser, reader);

    ASSERT_NE(style, nullptr);
    const auto& color = style->getTextColor();
    ASSERT_TRUE(color.has_value());
    EXPECT_EQ(color->value(), 0xFF5555);
}

TEST(StyleArgumentTypeTest, AllowsWhitespaceInsideBraces)
{
    // 命令参数按空白分隔，但花括号内的空白属于样式本身（原版 SNBT 允许）
    StyleArgumentType parser;
    StringReader reader("{bold: true, color: \"red\"}");
    const auto style = parseAll(parser, reader);

    ASSERT_NE(style, nullptr);
    EXPECT_EQ(style->bold(), true);
    EXPECT_EQ(style->getColor(), mc::text::TextFormatting::Red);
}

TEST(StyleArgumentTypeTest, QuotesProtectBracesAndSpaces)
{
    // insertion 里带空白与花括号：不能被当作参数结束或括号配平的一部分
    StyleArgumentType parser;
    StringReader reader("{insertion:\"{ not } a brace\"}");
    const auto style = parseAll(parser, reader);

    ASSERT_NE(style, nullptr);
    ASSERT_TRUE(style->getInsertion().has_value());
    EXPECT_EQ(*style->getInsertion(), "{ not } a brace");
}

TEST(StyleArgumentTypeTest, StopsAtTrailingArgument)
{
    // 后面还有别的参数时，只消费样式本身
    StyleArgumentType parser;
    StringReader reader("{bold:true} Steve");
    auto styleResult = parser.parse(reader);

    ASSERT_TRUE(styleResult.success()) << styleResult.error().toString();

    const auto style = std::move(styleResult).value();

    ASSERT_NE(style, nullptr);
    EXPECT_EQ(style->bold(), true);
    EXPECT_EQ(reader.getCursor(), 11);
    EXPECT_EQ(reader.getRemaining(), " Steve");
}

TEST(StyleArgumentTypeTest, RejectsUnbalancedBraces)
{
    StyleArgumentType parser;
    StringReader reader("{bold:true");
    EXPECT_TRUE((parser.parse(reader)).failed());
    // 失败后游标回到起点，便于上层报出准确位置
    EXPECT_EQ(reader.getCursor(), 0);
}

TEST(StyleArgumentTypeTest, RejectsNonCompoundText)
{
    StyleArgumentType parser;
    StringReader reader("bold");
    EXPECT_TRUE((parser.parse(reader)).failed());
}

TEST(StyleArgumentTypeTest, RejectsInvalidColor)
{
    StyleArgumentType parser;
    StringReader reader("{color:\"not-a-color\"}");
    EXPECT_TRUE((parser.parse(reader)).failed());
}

TEST(StyleArgumentTypeTest, ReportsTypeNameAndExamples)
{
    StyleArgumentType parser;
    EXPECT_EQ(parser.getTypeName(), "style");
    EXPECT_FALSE(parser.getExamples().empty());
    // 原版 StyleArgument 是 SingletonArgumentInfo（contextAware），网络元数据无属性
    EXPECT_TRUE(parser.serializeMetadata().empty());
}
