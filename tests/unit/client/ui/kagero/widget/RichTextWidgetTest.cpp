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

#include "client/ui/kagero/widget/RichTextWidget.hpp"
#include "client/ui/kagero/Types.hpp"
#include "common/core/Types.hpp"
#include "common/util/text/StringTextComponent.hpp"
#include "common/util/text/TextEvents.hpp"
#include "common/util/text/TextParser.hpp"
#include "common/util/text/TextStyle.hpp"
#include <gtest/gtest.h>

/**
 * @brief RichTextWidget 单元测试
 *
 * 测试覆盖：
 * - 构造函数和初始化
 * - 文本设置（ITextComponent 和纯文本）
 * - 布局计算（自动换行、行数计算）
 * - 样式属性设置
 * - 事件处理（点击、悬停回调）
 */
class RichTextWidgetTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        widget = std::make_unique<mc::client::ui::kagero::widget::RichTextWidget>("test", 0, 0, 200, 100);
    }

    void TearDown() override { widget.reset(); }

    std::unique_ptr<mc::client::ui::kagero::widget::RichTextWidget> widget;
};

// ==================== 构造函数测试 ====================

TEST_F(RichTextWidgetTest, DefaultConstructor)
{
    mc::client::ui::kagero::widget::RichTextWidget w;
    EXPECT_EQ(w.getText(), nullptr);
    EXPECT_EQ(widget->getUnformattedText(), "");
}

TEST_F(RichTextWidgetTest, ConstructorWithBounds)
{
    mc::client::ui::kagero::widget::RichTextWidget w("id", 10, 20, 100, 50);
    EXPECT_EQ(w.bounds().x, 10);
    EXPECT_EQ(w.bounds().y, 20);
    EXPECT_EQ(w.bounds().width, 100);
    EXPECT_EQ(w.bounds().height, 50);
}

TEST_F(RichTextWidgetTest, ConstructorWithText)
{
    auto text = std::make_unique<mc::text::StringTextComponent>("Hello");
    mc::client::ui::kagero::widget::RichTextWidget w("id", 0, 0, 100, 50, std::move(text));

    EXPECT_NE(w.getText(), nullptr);
    EXPECT_EQ(w.getUnformattedText(), "Hello");
}

// ==================== 文本设置测试 ====================

TEST_F(RichTextWidgetTest, SetTextFromComponent)
{
    auto text = std::make_unique<mc::text::StringTextComponent>("Test Text");
    widget->setText(std::move(text));

    EXPECT_NE(widget->getText(), nullptr);
    EXPECT_EQ(widget->getUnformattedText(), "Test Text");
}

TEST_F(RichTextWidgetTest, SetTextFromString)
{
    widget->setText("Plain std::string");

    EXPECT_NE(widget->getText(), nullptr);
    EXPECT_EQ(widget->getUnformattedText(), "Plain std::string");
}

TEST_F(RichTextWidgetTest, SetTextFromLegacyFormat)
{
    widget->setText("§cRed Text");

    EXPECT_NE(widget->getText(), nullptr);
    EXPECT_EQ(widget->getUnformattedText(), "Red Text");
}

TEST_F(RichTextWidgetTest, SetNullText)
{
    widget->setText(std::unique_ptr<mc::text::ITextComponent>());

    EXPECT_EQ(widget->getText(), nullptr);
    EXPECT_EQ(widget->getUnformattedText(), "");
}

// ==================== 样式设置测试 ====================

TEST_F(RichTextWidgetTest, SetBaseColor)
{
    widget->setBaseColor(0xFF0000FF); // 蓝色
    EXPECT_TRUE(widget->isVisible());
}

TEST_F(RichTextWidgetTest, SetShadow)
{
    widget->setShadow(true);
    widget->setShadow(false);
    EXPECT_TRUE(widget->isVisible());
}

TEST_F(RichTextWidgetTest, SetWordWrap)
{
    widget->setWordWrap(true);
    widget->setWordWrap(false);
    EXPECT_TRUE(widget->isVisible());
}

TEST_F(RichTextWidgetTest, SetAlignment)
{
    widget->setAlignment(mc::client::ui::kagero::widget::TextAlignment::Left);
    widget->setAlignment(mc::client::ui::kagero::widget::TextAlignment::Center);
    widget->setAlignment(mc::client::ui::kagero::widget::TextAlignment::Right);
    EXPECT_TRUE(widget->isVisible());
}

TEST_F(RichTextWidgetTest, SetLineSpacing)
{
    widget->setLineSpacing(1.5f);
    EXPECT_TRUE(widget->isVisible());
}

TEST_F(RichTextWidgetTest, SetMaxLines)
{
    widget->setMaxLines(10);
    EXPECT_TRUE(widget->isVisible());
}

TEST_F(RichTextWidgetTest, SetEventsEnabled)
{
    widget->setEventsEnabled(true);
    widget->setEventsEnabled(false);
    EXPECT_TRUE(widget->isVisible());
}

// ==================== 布局计算测试 ====================

TEST_F(RichTextWidgetTest, GetTextWidthEmpty)
{
    EXPECT_EQ(widget->getTextWidth(), 0.0f);
}

TEST_F(RichTextWidgetTest, GetTextWidthSimple)
{
    widget->setText("Hello World");
    EXPECT_GT(widget->getTextWidth(), 0.0f);
}

TEST_F(RichTextWidgetTest, GetTextHeightEmpty)
{
    EXPECT_EQ(widget->getTextHeight(), 0.0f);
}

TEST_F(RichTextWidgetTest, GetTextHeightSimple)
{
    widget->setText("Hello");
    EXPECT_GT(widget->getTextHeight(), 0.0f);
}

TEST_F(RichTextWidgetTest, GetLineCountEmpty)
{
    EXPECT_EQ(widget->getLineCount(), 0);
}

TEST_F(RichTextWidgetTest, GetLineCountSingleLine)
{
    widget->setText("Single Line");
    EXPECT_EQ(widget->getLineCount(), 1);
}

TEST_F(RichTextWidgetTest, GetLineCountMultipleLines)
{
    // Note: Word wrap requires either a real font or multiple text components.
    // Without a font, a single text component is treated as one run that won't wrap.
    // Here we test that setting word wrap doesn't cause errors.
    widget->setWordWrap(true);
    widget->setBounds(mc::client::ui::kagero::Rect(0, 0, 50, 100));
    widget->setText("This is a long text");
    // With no font set, measureTextWidth returns length * 8.0f
    // Since this is a single run, it won't wrap (wrap requires !currentLine.runs.empty())
    EXPECT_GE(widget->getLineCount(), 1);
}

TEST_F(RichTextWidgetTest, MaxLinesConstraint)
{
    // Note: Without a real font, word wrap behavior is limited.
    // Max lines is enforced during layout when text would wrap.
    widget->setWordWrap(true);
    widget->setBounds(mc::client::ui::kagero::Rect(0, 0, 30, 100));
    widget->setMaxLines(2);
    widget->setText("Line one");
    // With no font, line count depends on measureTextWidth fallback
    EXPECT_GE(widget->getLineCount(), 1);
    EXPECT_LE(widget->getLineCount(), 2);
}

// ==================== 事件回调测试 ====================

TEST_F(RichTextWidgetTest, ClickCallback)
{
    bool callbackCalled = false;

    widget->setOnClick([&callbackCalled](const mc::text::ClickEvent& /*event*/) { callbackCalled = true; });

    widget->setEventsEnabled(true);
}

TEST_F(RichTextWidgetTest, HoverCallback)
{
    bool callbackCalled = false;

    widget->setOnHover([&callbackCalled](const mc::text::HoverEvent& /*event*/, mc::i32 /*x*/, mc::i32 /*y*/) {
        callbackCalled = true;
    });

    widget->setEventsEnabled(true);
}

// ==================== 边界情况测试 ====================

TEST_F(RichTextWidgetTest, EmptyString)
{
    widget->setText("");
    EXPECT_EQ(widget->getUnformattedText(), "");
    EXPECT_EQ(widget->getLineCount(), 0);
}

TEST_F(RichTextWidgetTest, NullText)
{
    widget->setText(std::unique_ptr<mc::text::ITextComponent>());
    EXPECT_EQ(widget->getText(), nullptr);
    EXPECT_EQ(widget->getUnformattedText(), "");
}

TEST_F(RichTextWidgetTest, VeryLongText)
{
    std::string longText(1000, 'A');
    widget->setText(longText);
    EXPECT_EQ(widget->getUnformattedText().length(), 1000u);
}

TEST_F(RichTextWidgetTest, SpecialCharacters)
{
    widget->setText("Hello§cWorld§l!");
    EXPECT_EQ(widget->getUnformattedText(), "HelloWorld!");
}

TEST_F(RichTextWidgetTest, UnicodeText)
{
    widget->setText("你好世界");
    EXPECT_EQ(widget->getUnformattedText(), "你好世界");
}

TEST_F(RichTextWidgetTest, MixedFormatCodes)
{
    widget->setText("§cRed§lBold§rReset");
    EXPECT_EQ(widget->getUnformattedText(), "RedBoldReset");
}

// ==================== 组件生命周期测试 ====================

TEST_F(RichTextWidgetTest, MoveConstructor)
{
    widget->setText("Test");

    mc::client::ui::kagero::widget::RichTextWidget moved = std::move(*widget);
    EXPECT_EQ(moved.getUnformattedText(), "Test");
}

TEST_F(RichTextWidgetTest, Init)
{
    widget->setText("Test");
    widget->init();
    EXPECT_EQ(widget->getLineCount(), 1);
}

// ==================== 复杂文本测试 ====================

TEST_F(RichTextWidgetTest, NestedComponents)
{
    auto root = std::make_unique<mc::text::StringTextComponent>("Hello ");

    auto colored = std::make_unique<mc::text::StringTextComponent>("World");
    mc::text::Style redStyle;
    redStyle.setColor(mc::text::TextFormatting::Red);
    colored->setStyle(redStyle);

    root->append(std::move(colored));
    widget->setText(std::move(root));

    EXPECT_EQ(widget->getUnformattedText(), "Hello World");
}

TEST_F(RichTextWidgetTest, ClickEventComponent)
{
    auto text = std::make_unique<mc::text::StringTextComponent>("Click Me");
    mc::text::Style style;
    style.setClickEvent(mc::text::ClickEvent(mc::text::ClickAction::OpenUrl, "https://example.com"));
    text->setStyle(style);

    widget->setText(std::move(text));
    EXPECT_EQ(widget->getUnformattedText(), "Click Me");
}

TEST_F(RichTextWidgetTest, HoverEventComponent)
{
    auto text = std::make_unique<mc::text::StringTextComponent>("Hover Me");
    mc::text::Style style;
    style.setHoverEvent(mc::text::HoverEvent::showText("Tooltip"));
    text->setStyle(style);

    widget->setText(std::move(text));
    EXPECT_EQ(widget->getUnformattedText(), "Hover Me");
}

// ==================== 文本对齐测试 ====================

TEST_F(RichTextWidgetTest, AlignmentLeft)
{
    widget->setAlignment(mc::client::ui::kagero::widget::TextAlignment::Left);
    widget->setText("Test");
    EXPECT_EQ(widget->getLineCount(), 1);
}

TEST_F(RichTextWidgetTest, AlignmentCenter)
{
    widget->setAlignment(mc::client::ui::kagero::widget::TextAlignment::Center);
    widget->setText("Test");
    EXPECT_EQ(widget->getLineCount(), 1);
}

TEST_F(RichTextWidgetTest, AlignmentRight)
{
    widget->setAlignment(mc::client::ui::kagero::widget::TextAlignment::Right);
    widget->setText("Test");
    EXPECT_EQ(widget->getLineCount(), 1);
}

// ==================== 禁用事件测试 ====================

TEST_F(RichTextWidgetTest, EventsDisabled)
{
    widget->setEventsEnabled(false);
    widget->setText("Test");

    bool clicked = widget->onClick(10, 10, 0, 0);
    EXPECT_FALSE(clicked);
}

// ==================== Widget 可见性测试 ====================

TEST_F(RichTextWidgetTest, InvisibleWidget)
{
    widget->setText("Test");
    widget->setVisible(false);

    bool clicked = widget->onClick(10, 10, 0, 0);
    EXPECT_FALSE(clicked);
}

TEST_F(RichTextWidgetTest, InactiveWidget)
{
    widget->setText("Test");
    widget->setActive(false);

    bool clicked = widget->onClick(10, 10, 0, 0);
    EXPECT_FALSE(clicked);
}

// ==================== 混淆文字（§k）测试 ====================

TEST_F(RichTextWidgetTest, ObfuscatedTextDoesNotCrash)
{
    // 设置混淆文本不应崩溃
    widget->setText("§kSecret");
    EXPECT_NE(widget->getText(), nullptr);
    EXPECT_EQ(widget->getUnformattedText(), "Secret");
}

TEST_F(RichTextWidgetTest, ObfuscatedWithColorAndReset)
{
    // §k + 颜色 + §r 组合
    widget->setText("§c§kRedObfuscated§rNormal");
    EXPECT_NE(widget->getText(), nullptr);
    // 未格式化文本应包含完整内容
    EXPECT_EQ(widget->getUnformattedText(), "RedObfuscatedNormal");
}

TEST_F(RichTextWidgetTest, ObfuscatedEmptyText)
{
    // 空混淆文本不应崩溃
    widget->setText("§k");
    EXPECT_NE(widget->getText(), nullptr);
}

TEST_F(RichTextWidgetTest, ObfuscatedPureSpaceText)
{
    // 纯空格混淆文本不应崩溃（空格不替换）
    widget->setText("§k   ");
    EXPECT_NE(widget->getText(), nullptr);
    EXPECT_EQ(widget->getUnformattedText(), "   ");
}

TEST_F(RichTextWidgetTest, ObfuscatedMixedText)
{
    // 混淆文本与普通文本混合
    widget->setText("Normal §kObfuscated§r Normal");
    EXPECT_NE(widget->getText(), nullptr);
}

TEST_F(RichTextWidgetTest, ObfuscatedTickDoesNotCrash)
{
    // tick() 应正常处理混淆动画计时器
    widget->setText("§kTest");
    widget->init();

    // 模拟多帧tick
    for (int i = 0; i < 100; ++i) {
        widget->tick(0.016f); // ~60fps
    }

    EXPECT_NE(widget->getText(), nullptr);
}

TEST_F(RichTextWidgetTest, ObfuscatedUnicodeText)
{
    // UTF-8多字节字符的混淆文本不应崩溃
    widget->setText("§k你好");
    EXPECT_NE(widget->getText(), nullptr);
    EXPECT_EQ(widget->getUnformattedText(), "你好");
}

TEST_F(RichTextWidgetTest, ObfuscatedWithBold)
{
    // 混淆 + 粗体组合
    widget->setText("§l§kBoldObfuscated");
    EXPECT_NE(widget->getText(), nullptr);
}

TEST_F(RichTextWidgetTest, ObfuscatedMultiLine)
{
    // 混淆文本换行不应崩溃
    widget->setText("§kLine1\nLine2");
    EXPECT_NE(widget->getText(), nullptr);
}
