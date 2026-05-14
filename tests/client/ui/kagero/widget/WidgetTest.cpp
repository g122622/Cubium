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
 * @file WidgetTest.cpp
 * @brief Widget基类单元测试
 */

#include "client/ui/kagero/widget/Widget.hpp"
#include "client/ui/Glyph.hpp"
#include "client/ui/kagero/Types.hpp"
#include <gtest/gtest.h>

using namespace mc::client::ui::kagero;
using namespace mc::client::ui::kagero::widget;
using namespace mc;

/**
 * @brief 测试用Widget实现
 */
class TestWidget : public Widget {
public:
    TestWidget() = default;
    explicit TestWidget(std::string id)
        : Widget(std::move(id))
    {}

    void paint(PaintContext& ctx) override
    {
        (void)ctx;
        m_paintCalled = true;
    }

    bool wasPaintCalled() const { return m_paintCalled; }
    void resetFlags() { m_paintCalled = false; }

private:
    bool m_paintCalled = false;
};

// ==================== 构造函数测试 ====================

TEST(WidgetTest, DefaultConstructor)
{
    TestWidget widget;
    EXPECT_TRUE(widget.id().empty());
    EXPECT_TRUE(widget.isVisible());
    EXPECT_TRUE(widget.isActive());
    EXPECT_FALSE(widget.isHovered());
    EXPECT_FALSE(widget.isFocused());
    EXPECT_EQ(0, widget.x());
    EXPECT_EQ(0, widget.y());
    EXPECT_EQ(0, widget.width());
    EXPECT_EQ(0, widget.height());
}

TEST(WidgetTest, ConstructorWithId)
{
    TestWidget widget("test_widget");
    EXPECT_EQ("test_widget", widget.id());
}

// ==================== 位置和尺寸测试 ====================

TEST(WidgetTest, SetPosition)
{
    TestWidget widget;
    widget.setPosition(10, 20);

    EXPECT_EQ(10, widget.x());
    EXPECT_EQ(20, widget.y());
}

TEST(WidgetTest, SetSize)
{
    TestWidget widget;
    widget.setSize(100, 200);

    EXPECT_EQ(100, widget.width());
    EXPECT_EQ(200, widget.height());
}

TEST(WidgetTest, SetBounds)
{
    TestWidget widget;
    widget.setBounds(Rect(10, 20, 100, 200));

    EXPECT_EQ(10, widget.x());
    EXPECT_EQ(20, widget.y());
    EXPECT_EQ(100, widget.width());
    EXPECT_EQ(200, widget.height());
    EXPECT_EQ(110, widget.bounds().right());
    EXPECT_EQ(220, widget.bounds().bottom());
}

// ==================== 可见性和状态测试 ====================

TEST(WidgetTest, SetVisible)
{
    TestWidget widget;
    EXPECT_TRUE(widget.isVisible());

    widget.setVisible(false);
    EXPECT_FALSE(widget.isVisible());

    widget.setVisible(true);
    EXPECT_TRUE(widget.isVisible());
}

TEST(WidgetTest, SetActive)
{
    TestWidget widget;
    EXPECT_TRUE(widget.isActive());

    widget.setActive(false);
    EXPECT_FALSE(widget.isActive());
    EXPECT_TRUE(widget.isDisabled());

    widget.setActive(true);
    EXPECT_TRUE(widget.isActive());
    EXPECT_FALSE(widget.isDisabled());
}

TEST(WidgetTest, SetFocused)
{
    TestWidget widget;
    EXPECT_FALSE(widget.isFocused());

    widget.setFocused(true);
    EXPECT_TRUE(widget.isFocused());

    widget.setFocused(false);
    EXPECT_FALSE(widget.isFocused());
}

TEST(WidgetTest, SetHovered)
{
    TestWidget widget;
    EXPECT_FALSE(widget.isHovered());

    widget.setHovered(true);
    EXPECT_TRUE(widget.isHovered());

    widget.setHovered(false);
    EXPECT_FALSE(widget.isHovered());
}

// ==================== 包含和碰撞测试 ====================

TEST(WidgetTest, Contains)
{
    TestWidget widget;
    widget.setBounds(Rect(10, 20, 100, 50));

    EXPECT_TRUE(widget.contains(10, 20));   // 左上角
    EXPECT_TRUE(widget.contains(50, 40));   // 中心
    EXPECT_TRUE(widget.contains(109, 69));  // 右下角前一个像素
    EXPECT_FALSE(widget.contains(110, 70)); // 右下角（不含）
    EXPECT_FALSE(widget.contains(9, 20));   // 左边外
    EXPECT_FALSE(widget.contains(10, 19));  // 上边外
}

TEST(WidgetTest, IsMouseOver)
{
    TestWidget widget;
    widget.setBounds(Rect(10, 20, 100, 50));
    widget.setActive(true);
    widget.setVisible(true);

    EXPECT_TRUE(widget.isMouseOver(50, 40));
    EXPECT_FALSE(widget.isMouseOver(200, 200));

    // 不可见时不响应
    widget.setVisible(false);
    EXPECT_FALSE(widget.isMouseOver(50, 40));

    // 不激活时不响应
    widget.setVisible(true);
    widget.setActive(false);
    EXPECT_FALSE(widget.isMouseOver(50, 40));
}

// ==================== 锚点和边距测试 ====================

TEST(WidgetTest, SetAnchor)
{
    TestWidget widget;
    widget.setAnchor(Anchor::Center);

    EXPECT_EQ(Anchor::Center, widget.anchor());
}

TEST(WidgetTest, SetMargin)
{
    TestWidget widget;
    widget.setMargin(Margin(10, 20, 30, 40));

    EXPECT_EQ(10, widget.margin().left);
    EXPECT_EQ(20, widget.margin().top);
    EXPECT_EQ(30, widget.margin().right);
    EXPECT_EQ(40, widget.margin().bottom);
    EXPECT_EQ(40, widget.margin().horizontal());
    EXPECT_EQ(60, widget.margin().vertical());
}

TEST(WidgetTest, SetPadding)
{
    TestWidget widget;
    widget.setPadding(Padding(10, 20, 30, 40));

    EXPECT_EQ(10, widget.padding().left);
    EXPECT_EQ(20, widget.padding().top);
    EXPECT_EQ(30, widget.padding().right);
    EXPECT_EQ(40, widget.padding().bottom);
    EXPECT_EQ(40, widget.padding().horizontal());
    EXPECT_EQ(60, widget.padding().vertical());
}

// ==================== 透明度测试 ====================

TEST(WidgetTest, SetAlpha)
{
    TestWidget widget;
    EXPECT_FLOAT_EQ(1.0f, widget.alpha());

    widget.setAlpha(0.5f);
    EXPECT_FLOAT_EQ(0.5f, widget.alpha());

    widget.setAlpha(0.0f);
    EXPECT_FLOAT_EQ(0.0f, widget.alpha());
}

// ==================== Z索引测试 ====================

TEST(WidgetTest, SetZIndex)
{
    TestWidget widget;
    EXPECT_EQ(0, widget.zIndex());

    widget.setZIndex(10);
    EXPECT_EQ(10, widget.zIndex());

    widget.setZIndex(-5);
    EXPECT_EQ(-5, widget.zIndex());
}

// ==================== 父容器测试 ====================

TEST(WidgetTest, SetParent)
{
    TestWidget widget;
    EXPECT_EQ(nullptr, widget.parent());

    IWidgetContainer* container = reinterpret_cast<IWidgetContainer*>(0x1234);
    widget.setParent(container);
    EXPECT_EQ(container, widget.parent());
}

// ==================== ID设置测试 ====================

TEST(WidgetTest, SetId)
{
    TestWidget widget;
    widget.setId("new_id");

    EXPECT_EQ("new_id", widget.id());
}

// ==================== 事件处理测试 ====================

TEST(WidgetTest, OnClickReturnsFalse)
{
    TestWidget widget;
    widget.setBounds(Rect(0, 0, 100, 100));

    // 默认实现返回false
    EXPECT_FALSE(widget.onClick(50, 50, 0));
}

TEST(WidgetTest, OnReleaseReturnsFalse)
{
    TestWidget widget;
    widget.setBounds(Rect(0, 0, 100, 100));

    EXPECT_FALSE(widget.onRelease(50, 50, 0));
}

TEST(WidgetTest, OnDragReturnsFalse)
{
    TestWidget widget;
    widget.setBounds(Rect(0, 0, 100, 100));

    EXPECT_FALSE(widget.onDrag(50, 50, 10, 10));
}

TEST(WidgetTest, OnScrollReturnsFalse)
{
    TestWidget widget;
    widget.setBounds(Rect(0, 0, 100, 100));

    EXPECT_FALSE(widget.onScroll(50, 50, 1.0));
}

TEST(WidgetTest, OnKeyReturnsFalse)
{
    TestWidget widget;

    EXPECT_FALSE(widget.onKey(65, 0, 1, 0)); // 'A' key
}

TEST(WidgetTest, OnCharReturnsFalse)
{
    TestWidget widget;

    EXPECT_FALSE(widget.onChar('A'));
}

// ==================== Rect测试 ====================

TEST(RectTest, DefaultConstructor)
{
    Rect rect;
    EXPECT_EQ(0, rect.x);
    EXPECT_EQ(0, rect.y);
    EXPECT_EQ(0, rect.width);
    EXPECT_EQ(0, rect.height);
}

TEST(RectTest, ParameterConstructor)
{
    Rect rect(10, 20, 100, 50);

    EXPECT_EQ(10, rect.x);
    EXPECT_EQ(20, rect.y);
    EXPECT_EQ(100, rect.width);
    EXPECT_EQ(50, rect.height);
    EXPECT_EQ(110, rect.right());
    EXPECT_EQ(70, rect.bottom());
    EXPECT_EQ(60, rect.centerX());
    EXPECT_EQ(45, rect.centerY());
}

TEST(RectTest, Contains)
{
    Rect rect(10, 20, 100, 50);

    EXPECT_TRUE(rect.contains(10, 20));    // 左上角
    EXPECT_TRUE(rect.contains(50, 40));    // 中心
    EXPECT_TRUE(rect.contains(109, 69));   // 右下角前一个像素
    EXPECT_FALSE(rect.contains(110, 70));  // 右下角（不含）
    EXPECT_FALSE(rect.contains(9, 20));    // 左边外
    EXPECT_FALSE(rect.contains(200, 200)); // 远外
}

TEST(RectTest, Intersects)
{
    Rect rect1(0, 0, 100, 100);
    Rect rect2(50, 50, 100, 100);
    Rect rect3(200, 200, 100, 100);

    EXPECT_TRUE(rect1.intersects(rect2));
    EXPECT_FALSE(rect1.intersects(rect3));
}

TEST(RectTest, Intersection)
{
    Rect rect1(0, 0, 100, 100);
    Rect rect2(50, 50, 100, 100);

    Rect result = rect1.intersection(rect2);
    EXPECT_EQ(50, result.x);
    EXPECT_EQ(50, result.y);
    EXPECT_EQ(50, result.width);
    EXPECT_EQ(50, result.height);
}

TEST(RectTest, IsValid)
{
    Rect valid(0, 0, 100, 100);
    Rect invalid1(0, 0, 0, 100);
    Rect invalid2(0, 0, 100, 0);

    EXPECT_TRUE(valid.isValid());
    EXPECT_FALSE(invalid1.isValid());
    EXPECT_FALSE(invalid2.isValid());
}

// ==================== Margin测试 ====================

TEST(MarginTest, SingleValue)
{
    Margin margin(10);

    EXPECT_EQ(10, margin.left);
    EXPECT_EQ(10, margin.top);
    EXPECT_EQ(10, margin.right);
    EXPECT_EQ(10, margin.bottom);
    EXPECT_EQ(20, margin.horizontal());
    EXPECT_EQ(20, margin.vertical());
}

TEST(MarginTest, TwoValues)
{
    Margin margin(10, 20);

    EXPECT_EQ(10, margin.left);
    EXPECT_EQ(20, margin.top);
    EXPECT_EQ(10, margin.right);
    EXPECT_EQ(20, margin.bottom);
    EXPECT_EQ(20, margin.horizontal());
    EXPECT_EQ(40, margin.vertical());
}

TEST(MarginTest, FourValues)
{
    Margin margin(10, 20, 30, 40);

    EXPECT_EQ(10, margin.left);
    EXPECT_EQ(20, margin.top);
    EXPECT_EQ(30, margin.right);
    EXPECT_EQ(40, margin.bottom);
    EXPECT_EQ(40, margin.horizontal());
    EXPECT_EQ(60, margin.vertical());
}

// ==================== Padding测试 ====================

TEST(PaddingTest, SingleValue)
{
    Padding padding(10);

    EXPECT_EQ(10, padding.left);
    EXPECT_EQ(10, padding.top);
    EXPECT_EQ(10, padding.right);
    EXPECT_EQ(10, padding.bottom);
    EXPECT_EQ(20, padding.horizontal());
    EXPECT_EQ(20, padding.vertical());
}

TEST(PaddingTest, TwoValues)
{
    Padding padding(10, 20);

    EXPECT_EQ(10, padding.left);
    EXPECT_EQ(20, padding.top);
    EXPECT_EQ(10, padding.right);
    EXPECT_EQ(20, padding.bottom);
    EXPECT_EQ(20, padding.horizontal());
    EXPECT_EQ(40, padding.vertical());
}

TEST(PaddingTest, FourValues)
{
    Padding padding(10, 20, 30, 40);

    EXPECT_EQ(10, padding.left);
    EXPECT_EQ(20, padding.top);
    EXPECT_EQ(30, padding.right);
    EXPECT_EQ(40, padding.bottom);
    EXPECT_EQ(40, padding.horizontal());
    EXPECT_EQ(60, padding.vertical());
}

// ==================== Anchor测试 ====================

TEST(WidgetTest, VisualStyleProperties)
{
    TestWidget widget;

    EXPECT_EQ(mc::client::Colors::fromARGB(0, 0, 0, 0), widget.backgroundColor());
    EXPECT_EQ(mc::client::Colors::fromARGB(0, 0, 0, 0), widget.borderColor());
    EXPECT_EQ(0, widget.cornerRadius());

    widget.setBackgroundColor(mc::client::Colors::RED);
    widget.setBorderColor(mc::client::Colors::GREEN);
    widget.setCornerRadius(6);

    EXPECT_EQ(mc::client::Colors::RED, widget.backgroundColor());
    EXPECT_EQ(mc::client::Colors::GREEN, widget.borderColor());
    EXPECT_EQ(6, widget.cornerRadius());
}
TEST(AnchorTest, AllAnchors)
{
    EXPECT_EQ(0, static_cast<u8>(Anchor::TopLeft));
    EXPECT_EQ(1, static_cast<u8>(Anchor::TopCenter));
    EXPECT_EQ(2, static_cast<u8>(Anchor::TopRight));
    EXPECT_EQ(3, static_cast<u8>(Anchor::CenterLeft));
    EXPECT_EQ(4, static_cast<u8>(Anchor::Center));
    EXPECT_EQ(5, static_cast<u8>(Anchor::CenterRight));
    EXPECT_EQ(6, static_cast<u8>(Anchor::BottomLeft));
    EXPECT_EQ(7, static_cast<u8>(Anchor::BottomCenter));
    EXPECT_EQ(8, static_cast<u8>(Anchor::BottomRight));
}
