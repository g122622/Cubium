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
 * @file ScrollableWidgetTest.cpp
 * @brief ScrollableWidget单元测试
 */

#include "client/ui/kagero/widget/ScrollableWidget.hpp"
#include "client/ui/Glyph.hpp"
#include "client/ui/kagero/Types.hpp"
#include "client/ui/kagero/widget/TextWidget.hpp"
#include <gtest/gtest.h>

using namespace mc::client::ui::kagero;
using namespace mc::client::ui::kagero::widget;
using namespace mc;

// ==================== 构造函数测试 ====================

TEST(ScrollableWidgetTest, DefaultConstructor)
{
    ScrollableWidget scrollable;
    EXPECT_TRUE(scrollable.id().empty());
    EXPECT_EQ(0, scrollable.scrollX());
    EXPECT_EQ(0, scrollable.scrollY());
}

TEST(ScrollableWidgetTest, ConstructorWithBounds)
{
    ScrollableWidget scrollable("scroll_list", 10, 20, 300, 400);

    EXPECT_EQ("scroll_list", scrollable.id());
    EXPECT_EQ(10, scrollable.x());
    EXPECT_EQ(20, scrollable.y());
    EXPECT_EQ(300, scrollable.width());
    EXPECT_EQ(400, scrollable.height());
}

// ==================== 内容尺寸测试 ====================

TEST(ScrollableWidgetTest, SetContentWidth)
{
    ScrollableWidget scrollable("test", 0, 0, 300, 400);

    scrollable.setContentWidth(500);
    EXPECT_EQ(500, scrollable.contentWidth());
}

TEST(ScrollableWidgetTest, SetContentHeight)
{
    ScrollableWidget scrollable("test", 0, 0, 300, 400);

    scrollable.setContentHeight(1000);
    EXPECT_EQ(1000, scrollable.contentHeight());
}

TEST(ScrollableWidgetTest, SetContentSize)
{
    ScrollableWidget scrollable("test", 0, 0, 300, 400);

    scrollable.setContentSize(500, 1000);
    EXPECT_EQ(500, scrollable.contentWidth());
    EXPECT_EQ(1000, scrollable.contentHeight());
}

// ==================== 滚动位置测试 ====================

TEST(ScrollableWidgetTest, SetScrollX)
{
    ScrollableWidget scrollable("test", 0, 0, 300, 400);
    scrollable.setContentSize(500, 1000);

    scrollable.setScrollX(100);
    EXPECT_EQ(100, scrollable.scrollX());

    // 超出范围约束: maxScrollX = contentWidth - visibleWidth()
    // visibleWidth() = width - scrollbarWidth = 300 - 6 = 294
    // maxScrollX = 500 - 294 = 206
    scrollable.setScrollX(300);
    EXPECT_EQ(206, scrollable.scrollX());

    scrollable.setScrollX(-10);
    EXPECT_EQ(0, scrollable.scrollX());
}

TEST(ScrollableWidgetTest, SetScrollY)
{
    ScrollableWidget scrollable("test", 0, 0, 300, 400);
    scrollable.setContentHeight(1000);

    scrollable.setScrollY(200);
    EXPECT_EQ(200, scrollable.scrollY());

    // 超出范围约束
    scrollable.setScrollY(700);
    EXPECT_EQ(600, scrollable.scrollY()); // 1000 - 400 = 600 max

    scrollable.setScrollY(-10);
    EXPECT_EQ(0, scrollable.scrollY());
}

TEST(ScrollableWidgetTest, ScrollBy)
{
    ScrollableWidget scrollable("test", 0, 0, 300, 400);
    scrollable.setContentHeight(1000);

    scrollable.scrollBy(100);
    EXPECT_EQ(100, scrollable.scrollY());

    scrollable.scrollBy(50);
    EXPECT_EQ(150, scrollable.scrollY());

    scrollable.scrollBy(-100);
    EXPECT_EQ(50, scrollable.scrollY());
}

TEST(ScrollableWidgetTest, ScrollByX)
{
    ScrollableWidget scrollable("test", 0, 0, 300, 400);
    scrollable.setContentSize(500, 1000);

    scrollable.scrollByX(50);
    EXPECT_EQ(50, scrollable.scrollX());

    scrollable.scrollByX(-30);
    EXPECT_EQ(20, scrollable.scrollX());
}

TEST(ScrollableWidgetTest, ScrollToTop)
{
    ScrollableWidget scrollable("test", 0, 0, 300, 400);
    scrollable.setContentHeight(1000);
    scrollable.setScrollY(500);

    scrollable.scrollToTop();
    EXPECT_EQ(0, scrollable.scrollY());
}

TEST(ScrollableWidgetTest, ScrollToBottom)
{
    ScrollableWidget scrollable("test", 0, 0, 300, 400);
    scrollable.setContentHeight(1000);

    scrollable.scrollToBottom();
    EXPECT_EQ(600, scrollable.scrollY()); // 1000 - 400 = 600
}

TEST(ScrollableWidgetTest, ScrollTo)
{
    ScrollableWidget scrollable("test", 0, 0, 300, 400);
    scrollable.setContentHeight(1000);

    scrollable.scrollTo(300);
    EXPECT_EQ(300, scrollable.scrollY());
}

// ==================== scrollIntoView测试 ====================

TEST(ScrollableWidgetTest, ScrollIntoViewAbove)
{
    ScrollableWidget scrollable("test", 0, 0, 300, 400);
    scrollable.setContentHeight(1000);
    scrollable.setScrollY(200);

    // 滚动到顶部附近
    scrollable.scrollIntoView(50, 50);
    EXPECT_EQ(50, scrollable.scrollY());
}

TEST(ScrollableWidgetTest, ScrollIntoViewBelow)
{
    ScrollableWidget scrollable("test", 0, 0, 300, 400);
    scrollable.setContentHeight(1000);
    scrollable.setScrollY(200);

    // 滚动到底部附近
    scrollable.scrollIntoView(700, 50);
    EXPECT_EQ(350, scrollable.scrollY()); // 700 + 50 - 400 = 350
}

TEST(ScrollableWidgetTest, ScrollIntoViewVisible)
{
    ScrollableWidget scrollable("test", 0, 0, 300, 400);
    scrollable.setContentHeight(1000);
    scrollable.setScrollY(200);

    // 已经可见，不改变滚动位置
    scrollable.scrollIntoView(300, 50);
    EXPECT_EQ(200, scrollable.scrollY());
}

TEST(ScrollableWidgetTest, ScrollIntoViewWidget)
{
    ScrollableWidget scrollable("test", 0, 0, 300, 400);
    scrollable.setContentHeight(1000);
    scrollable.setScrollY(500);

    auto child = std::make_unique<TextWidget>("child", 10, 100, 200, 50);
    TextWidget* childPtr = child.get();
    scrollable.addChild(std::move(child));

    // scrollable的bounds.y是0，child的y是100，所以childTop = 100 - 0 = 100
    scrollable.scrollIntoView(static_cast<Widget*>(childPtr));

    // 应该滚动到让child可见
    EXPECT_LT(scrollable.scrollY(), 500);
}

// ==================== 滚动条测试 ====================

TEST(ScrollableWidgetTest, ShowScrollbar)
{
    ScrollableWidget scrollable("test", 0, 0, 300, 400);

    EXPECT_TRUE(scrollable.showScrollbar());

    scrollable.setShowScrollbar(false);
    EXPECT_FALSE(scrollable.showScrollbar());
}

TEST(ScrollableWidgetTest, ScrollbarWidth)
{
    ScrollableWidget scrollable("test", 0, 0, 300, 400);

    EXPECT_EQ(6, scrollable.scrollbarWidth());

    scrollable.setScrollbarWidth(10);
    EXPECT_EQ(10, scrollable.scrollbarWidth());
}

TEST(ScrollableWidgetTest, ScrollSpeed)
{
    ScrollableWidget scrollable("test", 0, 0, 300, 400);

    EXPECT_DOUBLE_EQ(20.0, scrollable.scrollSpeed());

    scrollable.setScrollSpeed(30.0);
    EXPECT_DOUBLE_EQ(30.0, scrollable.scrollSpeed());
}

// ==================== 可见区域测试 ====================

TEST(ScrollableWidgetTest, VisibleWidth)
{
    ScrollableWidget scrollable("test", 0, 0, 300, 400);
    scrollable.setShowScrollbar(true);
    scrollable.setScrollbarWidth(10);

    // 300 - 0 - 10 = 290
    EXPECT_EQ(290, scrollable.visibleWidth());

    scrollable.setShowScrollbar(false);
    EXPECT_EQ(300, scrollable.visibleWidth());
}

TEST(ScrollableWidgetTest, VisibleHeight)
{
    ScrollableWidget scrollable("test", 0, 0, 300, 400);

    EXPECT_EQ(400, scrollable.visibleHeight());

    scrollable.setPadding(Padding(10, 20, 10, 20));
    // 400 - 20 - 20 = 360
    EXPECT_EQ(360, scrollable.visibleHeight());
}

TEST(ScrollableWidgetTest, ScrollRatio)
{
    ScrollableWidget scrollable("test", 0, 0, 300, 400);
    scrollable.setContentHeight(1000);

    EXPECT_DOUBLE_EQ(0.0, scrollable.scrollRatio());

    scrollable.setScrollY(300);
    EXPECT_DOUBLE_EQ(0.5, scrollable.scrollRatio());

    scrollable.scrollToBottom();
    EXPECT_DOUBLE_EQ(1.0, scrollable.scrollRatio());
}

// ==================== 滚动事件测试 ====================

TEST(ScrollableWidgetTest, OnScroll)
{
    ScrollableWidget scrollable("test", 0, 0, 300, 400);
    scrollable.setContentHeight(1000);
    scrollable.setActive(true);
    scrollable.setVisible(true);

    // 向下滚动
    scrollable.onScroll(50, 50, -1.0);
    EXPECT_EQ(20, scrollable.scrollY()); // 默认速度20

    // 向上滚动
    scrollable.onScroll(50, 50, 1.0);
    EXPECT_EQ(0, scrollable.scrollY());
}

TEST(ScrollableWidgetTest, OnScrollInactive)
{
    ScrollableWidget scrollable("test", 0, 0, 300, 400);
    scrollable.setContentHeight(1000);
    scrollable.setActive(false);

    bool result = scrollable.onScroll(50, 50, -1.0);
    EXPECT_FALSE(result);
    EXPECT_EQ(0, scrollable.scrollY());
}

// ==================== 子组件测试 ====================

TEST(ScrollableWidgetTest, AddChild)
{
    ScrollableWidget scrollable("test", 0, 0, 300, 400);

    auto child = std::make_unique<TextWidget>("child1", 0, 0, 100, 20);
    scrollable.addChild(std::move(child));

    EXPECT_EQ(1u, scrollable.childCount());
}

TEST(ScrollableWidgetTest, FindChild)
{
    ScrollableWidget scrollable("test", 0, 0, 300, 400);

    scrollable.addChild(std::make_unique<TextWidget>("child1", 0, 0, 100, 20));

    Widget* found = scrollable.findChild("child1");
    ASSERT_NE(found, nullptr);
    EXPECT_EQ("child1", found->id());
}
