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
 * @file TextFieldWidgetTest.cpp
 * @brief TextFieldWidget单元测试
 */

#include "client/ui/kagero/widget/TextFieldWidget.hpp"
#include "client/ui/Font.hpp"
#include "client/ui/Glyph.hpp"
#include "client/ui/kagero/Types.hpp"
#include <gtest/gtest.h>

using namespace mc::client::ui::kagero;
using namespace mc::client::ui::kagero::widget;
using namespace mc::client::Colors;
using namespace mc;

class InspectableTextFieldWidget : public TextFieldWidget {
public:
    using TextFieldWidget::TextFieldWidget;

    [[nodiscard]] i32 scrollOffset() const { return m_scrollOffset; }
};

// ==================== 构造函数测试 ====================

TEST(TextFieldWidgetTest, DefaultConstructor)
{
    TextFieldWidget textField;
    EXPECT_TRUE(textField.id().empty());
    EXPECT_TRUE(textField.text().empty());
    EXPECT_EQ(32, textField.maxLength());
}

TEST(TextFieldWidgetTest, ConstructorWithBounds)
{
    TextFieldWidget textField("txt_name", 10, 20, 200, 24);

    EXPECT_EQ("txt_name", textField.id());
    EXPECT_EQ(10, textField.x());
    EXPECT_EQ(20, textField.y());
    EXPECT_EQ(200, textField.width());
    EXPECT_EQ(24, textField.height());
    EXPECT_TRUE(textField.isEnabled());
    EXPECT_TRUE(textField.canLoseFocus());
    EXPECT_TRUE(textField.drawBackground());
}

// ==================== 文本操作测试 ====================

TEST(TextFieldWidgetTest, SetText)
{
    TextFieldWidget textField("test", 0, 0, 100, 20);

    textField.setText("Hello");
    EXPECT_EQ("Hello", textField.text());

    textField.setText("World");
    EXPECT_EQ("World", textField.text());
}

TEST(TextFieldWidgetTest, SetMaxLength)
{
    TextFieldWidget textField("test", 0, 0, 100, 20);
    textField.setMaxLength(10);

    EXPECT_EQ(10, textField.maxLength());

    // 设置长文本会被截断
    textField.setText("This is a very long text");
    EXPECT_EQ(10, textField.text().length());
}

TEST(TextFieldWidgetTest, SetPlaceholder)
{
    TextFieldWidget textField("test", 0, 0, 100, 20);

    textField.setPlaceholder("Enter name...");
    EXPECT_EQ("Enter name...", textField.placeholder());
}

// ==================== 光标操作测试 ====================

TEST(TextFieldWidgetTest, SetCursorPosition)
{
    TextFieldWidget textField("test", 0, 0, 100, 20);
    textField.setText("Hello");

    EXPECT_EQ(5, textField.cursorPosition()); // 默认在末尾

    textField.setCursorPosition(2);
    EXPECT_EQ(2, textField.cursorPosition());

    textField.setCursorPositionStart();
    EXPECT_EQ(0, textField.cursorPosition());

    textField.setCursorPositionEnd();
    EXPECT_EQ(5, textField.cursorPosition());

    // 超出范围约束
    textField.setCursorPosition(100);
    EXPECT_EQ(5, textField.cursorPosition());

    textField.setCursorPosition(-1);
    EXPECT_EQ(0, textField.cursorPosition());
}

TEST(TextFieldWidgetTest, MoveCursorBy)
{
    TextFieldWidget textField("test", 0, 0, 100, 20);
    textField.setText("Hello");
    textField.setCursorPositionStart();

    textField.moveCursorBy(2);
    EXPECT_EQ(2, textField.cursorPosition());

    textField.moveCursorBy(-1);
    EXPECT_EQ(1, textField.cursorPosition());
}

// ==================== 选择操作测试 ====================

TEST(TextFieldWidgetTest, SelectAll)
{
    TextFieldWidget textField("test", 0, 0, 100, 20);
    textField.setText("Hello World");

    textField.selectAll();

    EXPECT_TRUE(textField.hasSelection());
    EXPECT_EQ("Hello World", textField.getSelectedText());
}

TEST(TextFieldWidgetTest, ClearSelection)
{
    TextFieldWidget textField("test", 0, 0, 100, 20);
    textField.setText("Hello World");
    textField.selectAll();

    EXPECT_TRUE(textField.hasSelection());

    textField.clearSelection();
    EXPECT_FALSE(textField.hasSelection());
}

TEST(TextFieldWidgetTest, GetSelectedText)
{
    TextFieldWidget textField("test", 0, 0, 100, 20);
    textField.setText("Hello World");

    // 手动设置选择（通过移动光标）
    textField.setCursorPosition(0);
    // 使用selectAll来创建选择
    textField.selectAll();

    EXPECT_EQ("Hello World", textField.getSelectedText());
}

// ==================== 写入和删除测试 ====================

TEST(TextFieldWidgetTest, WriteText)
{
    TextFieldWidget textField("test", 0, 0, 100, 20);

    textField.writeText("Hello");
    EXPECT_EQ("Hello", textField.text());

    textField.writeText(" World");
    EXPECT_EQ("Hello World", textField.text());
}

TEST(TextFieldWidgetTest, WriteTextWithSelection)
{
    TextFieldWidget textField("test", 0, 0, 100, 20);
    textField.setText("Hello World");
    textField.selectAll();

    textField.writeText("Hi");
    EXPECT_EQ("Hi", textField.text());
}

TEST(TextFieldWidgetTest, DeleteFromCursorBackspace)
{
    TextFieldWidget textField("test", 0, 0, 100, 20);
    textField.setText("Hello");
    textField.setEnabled(true);
    textField.setFocused(true);
    textField.setCursorPositionEnd();

    // 删除最后一个字符
    textField.onKey(259, 0, 1, 0); // GLFW_KEY_BACKSPACE
    EXPECT_EQ("Hell", textField.text());
}

TEST(TextFieldWidgetTest, DeleteFromCursorDelete)
{
    TextFieldWidget textField("test", 0, 0, 100, 20);
    textField.setText("Hello");
    textField.setEnabled(true);
    textField.setFocused(true);
    textField.setCursorPositionStart();

    // 删除第一个字符
    textField.onKey(261, 0, 1, 0); // GLFW_KEY_DELETE
    EXPECT_EQ("ello", textField.text());
}

TEST(TextFieldWidgetTest, DeleteSelectedText)
{
    TextFieldWidget textField("test", 0, 0, 100, 20);
    textField.setText("Hello World");
    textField.selectAll();

    textField.deleteSelectedText();
    EXPECT_EQ("", textField.text());
}

// ==================== 字符输入测试 ====================

TEST(TextFieldWidgetTest, OnCharInput)
{
    TextFieldWidget textField("test", 0, 0, 100, 20);
    textField.setEnabled(true);
    textField.setFocused(true);

    textField.onChar('H');
    textField.onChar('i');
    EXPECT_EQ("Hi", textField.text());
}

TEST(TextFieldWidgetTest, OnCharInputDisabled)
{
    TextFieldWidget textField("test", 0, 0, 100, 20);
    textField.setEnabled(false);
    textField.setFocused(true);

    textField.onChar('H');
    EXPECT_TRUE(textField.text().empty());
}

TEST(TextFieldWidgetTest, OnCharInputControlChars)
{
    TextFieldWidget textField("test", 0, 0, 100, 20);
    textField.setEnabled(true);
    textField.setFocused(true);

    // 控制字符不应被输入
    textField.onChar(0);   // NULL
    textField.onChar(8);   // Backspace
    textField.onChar(127); // DEL

    EXPECT_TRUE(textField.text().empty());
}

// ==================== 验证器测试 ====================

TEST(TextFieldWidgetTest, ValidatorAcceptsValidInput)
{
    TextFieldWidget textField("test", 0, 0, 100, 20);
    textField.setValidator([](const std::string& text) {
        // 只允许数字
        for (char c : text) {
            if (!std::isdigit(c)) return false;
        }
        return true;
    });

    textField.setText("12345");
    EXPECT_EQ("12345", textField.text());
}

TEST(TextFieldWidgetTest, ValidatorRejectsInvalidInput)
{
    TextFieldWidget textField("test", 0, 0, 100, 20);
    textField.setValidator([](const std::string& text) {
        // 只允许数字
        for (char c : text) {
            if (!std::isdigit(c)) return false;
        }
        return true;
    });

    textField.setText("123"); // 有效
    EXPECT_EQ("123", textField.text());

    textField.setText("abc");           // 无效，应该被拒绝
    EXPECT_EQ("123", textField.text()); // 保持原值
}

// ==================== 回调测试 ====================

TEST(TextFieldWidgetTest, TextChangedCallback)
{
    TextFieldWidget textField("test", 0, 0, 100, 20);

    std::string lastText;
    int callCount = 0;
    textField.setTextChangedCallback([&lastText, &callCount](const std::string& text) {
        lastText = text;
        ++callCount;
    });

    textField.setText("Hello");
    EXPECT_EQ("Hello", lastText);
    EXPECT_EQ(1, callCount);

    textField.setText("World");
    EXPECT_EQ("World", lastText);
    EXPECT_EQ(2, callCount);
}

// ==================== 状态测试 ====================

TEST(TextFieldWidgetTest, SetEnabled)
{
    TextFieldWidget textField("test", 0, 0, 100, 20);

    EXPECT_TRUE(textField.isEnabled());

    textField.setEnabled(false);
    EXPECT_FALSE(textField.isEnabled());

    textField.setEnabled(true);
    EXPECT_TRUE(textField.isEnabled());
}

TEST(TextFieldWidgetTest, SetCanLoseFocus)
{
    TextFieldWidget textField("test", 0, 0, 100, 20);

    EXPECT_TRUE(textField.canLoseFocus());

    textField.setCanLoseFocus(false);
    EXPECT_FALSE(textField.canLoseFocus());

    textField.setCanLoseFocus(true);
    EXPECT_TRUE(textField.canLoseFocus());
}

TEST(TextFieldWidgetTest, SetDrawBackground)
{
    TextFieldWidget textField("test", 0, 0, 100, 20);

    EXPECT_TRUE(textField.drawBackground());

    textField.setDrawBackground(false);
    EXPECT_FALSE(textField.drawBackground());
}

TEST(TextFieldWidgetTest, SetTextColor)
{
    TextFieldWidget textField("test", 0, 0, 100, 20);

    textField.setTextColor(RED);
    EXPECT_EQ(RED, textField.textColor());
}

// ==================== 颜色测试 ====================

TEST(TextFieldWidgetTest, SetDisabledTextColor)
{
    TextFieldWidget textField("test", 0, 0, 100, 20);

    textField.setDisabledTextColor(GRAY);
    EXPECT_EQ(GRAY, textField.disabledTextColor());
}

// ==================== 键盘导航测试 ====================

TEST(TextFieldWidgetTest, KeyHomeEnd)
{
    TextFieldWidget textField("test", 0, 0, 100, 20);
    textField.setText("Hello World");
    textField.setEnabled(true);
    textField.setFocused(true);

    textField.setCursorPosition(5);
    EXPECT_EQ(5, textField.cursorPosition());

    // Home键
    textField.onKey(268, 0, 1, 0); // GLFW_KEY_HOME
    EXPECT_EQ(0, textField.cursorPosition());

    // End键
    textField.onKey(269, 0, 1, 0);             // GLFW_KEY_END
    EXPECT_EQ(11, textField.cursorPosition()); // "Hello World".length()
}

TEST(TextFieldWidgetTest, KeyLeftRight)
{
    TextFieldWidget textField("test", 0, 0, 100, 20);
    textField.setText("Hello");
    textField.setEnabled(true);
    textField.setFocused(true);
    textField.setCursorPositionStart();

    EXPECT_EQ(0, textField.cursorPosition());

    // 右键
    textField.onKey(262, 0, 1, 0); // GLFW_KEY_RIGHT
    EXPECT_EQ(1, textField.cursorPosition());

    // 左键
    textField.onKey(263, 0, 1, 0); // GLFW_KEY_LEFT
    EXPECT_EQ(0, textField.cursorPosition());

    // 左键不能超出边界
    textField.onKey(263, 0, 1, 0);
    EXPECT_EQ(0, textField.cursorPosition());
}

// ==================== canWrite测试 ====================

TEST(TextFieldWidgetTest, CanWrite)
{
    TextFieldWidget textField("test", 0, 0, 100, 20);

    // 不可见不可写
    textField.setVisible(false);
    EXPECT_FALSE(textField.canWrite());

    // 可见但不聚焦不可写
    textField.setVisible(true);
    textField.setFocused(false);
    EXPECT_FALSE(textField.canWrite());

    // 可见且聚焦但禁用不可写
    textField.setFocused(true);
    textField.setEnabled(false);
    EXPECT_FALSE(textField.canWrite());

    // 可见、聚焦、启用才可写
    textField.setEnabled(true);
    EXPECT_TRUE(textField.canWrite());
}

TEST(TextFieldWidgetTest, OnClick_SetsCursorPositionByMouseX)
{
    TextFieldWidget textField("test", 0, 0, 100, 20);
    textField.setText("ABCD");

    // 点击到第二个字符边界，光标应落在索引 1。
    EXPECT_TRUE(textField.onClick(9, 10, 0));
    EXPECT_EQ(1, textField.cursorPosition());
}

TEST(TextFieldWidgetTest, WriteText_ReplacesSelectionAndRespectsMaxLength)
{
    TextFieldWidget textField("test", 0, 0, 100, 20);
    textField.setText("ABCD");
    textField.setCursorPosition(2);
    textField.setSelectionPosition(4);
    textField.setMaxLength(5);

    textField.writeText("WXYZ");

    EXPECT_EQ("ABWXY", textField.text());
    EXPECT_EQ(5, static_cast<i32>(textField.text().size()));
    EXPECT_EQ(5, textField.cursorPosition());
    EXPECT_FALSE(textField.hasSelection());
}
