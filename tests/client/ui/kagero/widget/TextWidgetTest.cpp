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
 * @file TextWidgetTest.cpp
 * @brief TextWidget单元测试
 */

#include "client/ui/kagero/widget/TextWidget.hpp"
#include "client/ui/Font.hpp"
#include "client/ui/Glyph.hpp"
#include "client/ui/kagero/Types.hpp"
#include <gtest/gtest.h>

using namespace mc::client::ui::kagero;
using namespace mc::client::ui::kagero::widget;
using namespace mc::client::Colors;
using namespace mc;

// ==================== 构造函数测试 ====================

TEST(TextWidgetTest, DefaultConstructor)
{
    TextWidget text;
    EXPECT_TRUE(text.id().empty());
    EXPECT_TRUE(text.text().empty());
    EXPECT_EQ(TextAlignment::Left, text.alignment());
}

TEST(TextWidgetTest, ConstructorWithBounds)
{
    TextWidget text("lbl_title", 10, 20, 200, 30);

    EXPECT_EQ("lbl_title", text.id());
    EXPECT_EQ(10, text.x());
    EXPECT_EQ(20, text.y());
    EXPECT_EQ(200, text.width());
    EXPECT_EQ(30, text.height());
    EXPECT_TRUE(text.text().empty());
}

TEST(TextWidgetTest, ConstructorWithText)
{
    TextWidget text("lbl_title", 10, 20, 200, 30, "Hello World");

    EXPECT_EQ("lbl_title", text.id());
    EXPECT_EQ("Hello World", text.text());
}

// ==================== 文本属性测试 ====================

TEST(TextWidgetTest, SetText)
{
    TextWidget text("test", 0, 0, 100, 20);
    EXPECT_TRUE(text.text().empty());

    text.setText("Hello");
    EXPECT_EQ("Hello", text.text());

    text.setText("World");
    EXPECT_EQ("World", text.text());
}

TEST(TextWidgetTest, SetColor)
{
    TextWidget text("test", 0, 0, 100, 20);

    text.setColor(RED);
    EXPECT_EQ(RED, text.color());
}

TEST(TextWidgetTest, SetShadow)
{
    TextWidget text("test", 0, 0, 100, 20);

    EXPECT_TRUE(text.hasShadow()); // 默认有阴影

    text.setShadow(false);
    EXPECT_FALSE(text.hasShadow());

    text.setShadow(true);
    EXPECT_TRUE(text.hasShadow());
}

TEST(TextWidgetTest, SetShadowColor)
{
    TextWidget text("test", 0, 0, 100, 20);

    text.setShadowColor(BLACK);
    EXPECT_EQ(BLACK, text.shadowColor());
}

TEST(TextWidgetTest, SetAlignment)
{
    TextWidget text("test", 0, 0, 100, 20);

    EXPECT_EQ(TextAlignment::Left, text.alignment());

    text.setAlignment(TextAlignment::Center);
    EXPECT_EQ(TextAlignment::Center, text.alignment());

    text.setAlignment(TextAlignment::Right);
    EXPECT_EQ(TextAlignment::Right, text.alignment());
}

// ==================== 换行和行数测试 ====================

TEST(TextWidgetTest, SetMaxLines)
{
    TextWidget text("test", 0, 0, 100, 20);

    EXPECT_EQ(0, text.maxLines()); // 默认无限制

    text.setMaxLines(3);
    EXPECT_EQ(3, text.maxLines());
}

TEST(TextWidgetTest, SetWordWrap)
{
    TextWidget text("test", 0, 0, 100, 20);

    EXPECT_FALSE(text.wordWrap()); // 默认不换行

    text.setWordWrap(true);
    EXPECT_TRUE(text.wordWrap());

    text.setWordWrap(false);
    EXPECT_FALSE(text.wordWrap());
}

TEST(TextWidgetTest, SetLineHeight)
{
    TextWidget text("test", 0, 0, 100, 20);

    EXPECT_EQ(9, text.lineHeight()); // 默认MC字体高度

    text.setLineHeight(12);
    EXPECT_EQ(12, text.lineHeight());
}

TEST(TextWidgetTest, SetScale)
{
    TextWidget text("test", 0, 0, 100, 20);

    EXPECT_FLOAT_EQ(1.0f, text.scale());

    text.setScale(1.5f);
    EXPECT_FLOAT_EQ(1.5f, text.scale());

    text.setScale(0.5f);
    EXPECT_FLOAT_EQ(0.5f, text.scale());
}

// ==================== 文本尺寸测试 ====================

TEST(TextWidgetTest, GetTextWidth)
{
    TextWidget text("test", 0, 0, 100, 20);
    text.setText("Hello");

    // 假设每个字符宽度为8像素
    f32 width = text.getTextWidth();
    EXPECT_FLOAT_EQ(5.0f * 8.0f, width);

    text.setScale(2.0f);
    width = text.getTextWidth();
    EXPECT_FLOAT_EQ(5.0f * 8.0f * 2.0f, width);
}

TEST(TextWidgetTest, GetTextWidthWithFont)
{
    mc::client::Font font;
    TextWidget text("test", 0, 0, 100, 20);
    text.setFont(&font);
    text.setText("Hello");

    EXPECT_FLOAT_EQ(5.0f * 4.0f, text.getTextWidth());
}

TEST(TextWidgetTest, GetTextHeight)
{
    TextWidget text("test", 0, 0, 100, 20);
    text.setText("Hello");
    text.setLineHeight(10);

    f32 height = text.getTextHeight();
    EXPECT_FLOAT_EQ(10.0f, height);

    text.setScale(2.0f);
    height = text.getTextHeight();
    EXPECT_FLOAT_EQ(20.0f, height);
}

TEST(TextWidgetTest, GetLineCount)
{
    TextWidget text("test", 0, 0, 100, 20);

    text.setText("Hello");
    EXPECT_EQ(1, text.getLineCount());

    text.setText("Hello\nWorld");
    EXPECT_EQ(2, text.getLineCount());

    text.setText("Hello\nWorld\nTest");
    EXPECT_EQ(3, text.getLineCount());

    text.setText(""); // 空文本
    EXPECT_EQ(0, text.getLineCount());
}

TEST(TextWidgetTest, GetLine)
{
    TextWidget text("test", 0, 0, 100, 20);
    text.setText("Hello\nWorld\nTest");

    EXPECT_EQ("Hello", text.getLine(0));
    EXPECT_EQ("World", text.getLine(1));
    EXPECT_EQ("Test", text.getLine(2));
    EXPECT_EQ("", text.getLine(3)); // 超出范围
}

// ==================== 可见性测试 ====================

TEST(TextWidgetTest, InvisibleNotRendered)
{
    TextWidget text("test", 0, 0, 100, 20, "Hello");
    text.setVisible(false);

    // 验证不可见时paint应该跳过绘制
    // 实际测试需要mock PaintContext验证绘制调用
    EXPECT_FALSE(text.isVisible());
}

// ==================== 字体测试 ====================

TEST(TextWidgetTest, SetFont)
{
    TextWidget text("test", 0, 0, 100, 20);

    EXPECT_EQ(nullptr, text.font());

    void* mockFont = reinterpret_cast<void*>(0x1234);
    text.setFont(mockFont);
    EXPECT_EQ(mockFont, text.font());
}

// ==================== TextAlignment枚举测试 ====================

TEST(TextAlignmentTest, EnumValues)
{
    EXPECT_EQ(0, static_cast<u8>(TextAlignment::Left));
    EXPECT_EQ(1, static_cast<u8>(TextAlignment::Center));
    EXPECT_EQ(2, static_cast<u8>(TextAlignment::Right));
}

TEST(TextWidgetTest, GetLineCountWithWordWrap)
{
    mc::client::Font font;
    TextWidget text("test", 0, 0, 8, 20);
    text.setFont(&font);
    text.setWordWrap(true);
    text.setText("AB CD");

    EXPECT_EQ(2, text.getLineCount());
    EXPECT_EQ("AB", text.getLine(0));
    EXPECT_EQ("CD", text.getLine(1));
}
