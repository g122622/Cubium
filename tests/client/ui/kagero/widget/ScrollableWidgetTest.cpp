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

    // 内容高度未设置（默认0），垂直滚动条不可见，所以不减去滚动条宽度
    EXPECT_EQ(300, scrollable.visibleWidth());

    // 设置内容高度使垂直滚动条可见
    scrollable.setContentHeight(1000);
    // 300 - 0 (padding) - 10 (scrollbar) = 290
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

// ==================== 水平滚动条测试 ====================

TEST(ScrollableWidgetTest, ShowHorizontalScrollbar)
{
    ScrollableWidget scrollable("test", 0, 0, 300, 400);

    // 默认显示水平滚动条
    EXPECT_TRUE(scrollable.showHorizontalScrollbar());

    scrollable.setShowHorizontalScrollbar(false);
    EXPECT_FALSE(scrollable.showHorizontalScrollbar());
}

TEST(ScrollableWidgetTest, HorizontalScrollRatio)
{
    ScrollableWidget scrollable("test", 0, 0, 300, 400);
    scrollable.setContentSize(600, 800);

    // 初始时水平滚动比例为0
    EXPECT_DOUBLE_EQ(0.0, scrollable.horizontalScrollRatio());

    // 滚动到中间位置
    scrollable.setScrollX(150);
    // maxScrollX = 600 - 300 + 6 (scrollbarWidth) = 306...
    // visibleWidth = 300 - 6 (scrollbarWidth) = 294
    // maxScrollX = 600 - 294 = 306
    // ratio = 150 / 306 ≈ 0.490
    EXPECT_NEAR(0.490, scrollable.horizontalScrollRatio(), 0.01);

    // 滚动到最右
    scrollable.scrollToRight();
    EXPECT_DOUBLE_EQ(1.0, scrollable.horizontalScrollRatio());
}

TEST(ScrollableWidgetTest, ScrollToLeftRight)
{
    ScrollableWidget scrollable("test", 0, 0, 300, 400);
    scrollable.setContentSize(600, 800);

    // 滚动到最右
    scrollable.scrollToRight();
    EXPECT_GT(scrollable.scrollX(), 0);

    // 滚动到最左
    scrollable.scrollToLeft();
    EXPECT_EQ(0, scrollable.scrollX());
}

TEST(ScrollableWidgetTest, ScrollXIntoView)
{
    ScrollableWidget scrollable("test", 0, 0, 300, 400);
    scrollable.setContentSize(600, 800);
    scrollable.setScrollX(100);

    // 目标在视口左侧
    scrollable.scrollXIntoView(20, 50);
    EXPECT_EQ(20, scrollable.scrollX());

    // 目标在视口右侧
    scrollable.scrollXIntoView(500, 50);
    EXPECT_GT(scrollable.scrollX(), 100);

    // 目标已在视口中
    scrollable.setScrollX(100);
    i32 savedX = scrollable.scrollX();
    scrollable.scrollXIntoView(150, 30);
    EXPECT_EQ(savedX, scrollable.scrollX());
}

TEST(ScrollableWidgetTest, VisibleWidthWithBothScrollbars)
{
    ScrollableWidget scrollable("test", 0, 0, 300, 400);
    scrollable.setShowScrollbar(true);
    scrollable.setShowHorizontalScrollbar(true);
    scrollable.setScrollbarWidth(6);
    scrollable.setContentSize(600, 1000); // 内容超出两个方向

    // 两个滚动条都可见时，visibleWidth 需要减去垂直滚动条宽度
    // visibleHeightRaw = 400, contentHeight = 1000 > 400, 所以垂直滚动条可见
    // visibleWidthRaw = 300, contentWidth = 600 > 300 - 6 = 294, 所以水平滚动条可见
    // visibleWidth = 300 - 6 (垂直滚动条) = 294
    EXPECT_EQ(294, scrollable.visibleWidth());
}

TEST(ScrollableWidgetTest, VisibleHeightWithBothScrollbars)
{
    ScrollableWidget scrollable("test", 0, 0, 300, 400);
    scrollable.setShowScrollbar(true);
    scrollable.setShowHorizontalScrollbar(true);
    scrollable.setScrollbarWidth(6);
    scrollable.setContentSize(600, 1000); // 内容超出两个方向

    // 两个滚动条都可见时，visibleHeight 需要减去水平滚动条高度
    // visibleWidthRaw = 300, contentWidth = 600 > 300, 所以水平滚动条可见
    // visibleHeightRaw = 400, contentHeight = 1000 > 400 - 6 = 394, 所以垂直滚动条可见
    // visibleHeight = 400 - 6 (水平滚动条) = 394
    EXPECT_EQ(394, scrollable.visibleHeight());
}

TEST(ScrollableWidgetTest, HorizontalScrollbarDragOnClick)
{
    ScrollableWidget scrollable("test", 0, 0, 300, 400);
    scrollable.setContentSize(600, 800);
    scrollable.setActive(true);
    scrollable.setVisible(true);

    // 点击水平滚动条区域（底部）
    // 水平滚动条在 y = bottom - scrollbarWidth = 400 - 6 = 394
    bool result = scrollable.onClick(150, 397, 0, 0);
    EXPECT_TRUE(result);
}

TEST(ScrollableWidgetTest, OnScrollHorizontal)
{
    ScrollableWidget scrollable("test", 0, 0, 300, 400);
    scrollable.setContentSize(600, 800);
    scrollable.setActive(true);
    scrollable.setVisible(true);

    // 默认情况下（Shift未按下），滚轮应垂直滚动
    scrollable.onScroll(50, 50, -1.0);
    EXPECT_EQ(20, scrollable.scrollY()); // 默认速度20
    EXPECT_EQ(0, scrollable.scrollX());  // 水平不滚动
}

TEST(ScrollableWidgetTest, OnKeyHorizontal)
{
    ScrollableWidget scrollable("test", 0, 0, 300, 400);
    scrollable.setContentSize(600, 800);
    scrollable.setActive(true);
    scrollable.setVisible(true);
    scrollable.setFocused(true);

    // 右键水平滚动
    scrollable.onKey(Keys::Right, 0, static_cast<i32>(KeyAction::Press), 0);
    EXPECT_EQ(20, scrollable.scrollX());
    EXPECT_EQ(0, scrollable.scrollY()); // 垂直不滚动

    // 左键水平滚动
    scrollable.onKey(Keys::Left, 0, static_cast<i32>(KeyAction::Press), 0);
    EXPECT_EQ(0, scrollable.scrollX());

    // Home键滚动到顶部
    scrollable.setScrollY(200);
    scrollable.onKey(Keys::Home, 0, static_cast<i32>(KeyAction::Press), 0);
    EXPECT_EQ(0, scrollable.scrollY());

    // Shift+Home滚动到最左
    scrollable.setScrollX(100);
    scrollable.onKey(Keys::Home, 0, static_cast<i32>(KeyAction::Press), static_cast<i32>(KeyMods::Shift));
    EXPECT_EQ(0, scrollable.scrollX());

    // End键滚动到底部
    scrollable.onKey(Keys::End, 0, static_cast<i32>(KeyAction::Press), 0);
    EXPECT_GT(scrollable.scrollY(), 0);

    // Shift+End滚动到最右
    scrollable.onKey(Keys::End, 0, static_cast<i32>(KeyAction::Press), static_cast<i32>(KeyMods::Shift));
    EXPECT_GT(scrollable.scrollX(), 0);
}

TEST(ScrollableWidgetTest, ClampScrollWithBothAxes)
{
    ScrollableWidget scrollable("test", 0, 0, 300, 400);
    scrollable.setContentSize(600, 1000);

    // 超出范围约束
    scrollable.setScrollX(999);
    scrollable.setScrollY(999);

    // scrollX 应该被限制在 maxScrollX
    EXPECT_LT(scrollable.scrollX(), 999);
    EXPECT_LT(scrollable.scrollY(), 999);
    EXPECT_GE(scrollable.scrollX(), 0);
    EXPECT_GE(scrollable.scrollY(), 0);
}

TEST(ScrollableWidgetTest, NoHorizontalScrollbarWhenContentFits)
{
    ScrollableWidget scrollable("test", 0, 0, 300, 400);
    scrollable.setShowHorizontalScrollbar(true);
    scrollable.setContentSize(200, 800); // 内容宽度小于容器宽度

    // 内容宽度小于可见宽度时，水平滚动不应生效
    // scrollX 应该被 clamp 到 0
    scrollable.setScrollX(100);
    EXPECT_EQ(0, scrollable.scrollX());
}

TEST(ScrollableWidgetTest, HorizontalScrollbarDragAndRelease)
{
    ScrollableWidget scrollable("test", 0, 0, 300, 400);
    scrollable.setContentSize(600, 800);
    scrollable.setActive(true);
    scrollable.setVisible(true);

    // 水平滚动条在底部 y = bottom - scrollbarWidth = 400 - 6 = 394
    // 内容宽度600 > 可见宽度294，水平滚动条可见

    // 点击水平滚动条区域开始拖动
    bool clicked = scrollable.onClick(150, 397, 0, 0);
    EXPECT_TRUE(clicked);

    // 释放鼠标结束拖动
    bool released = scrollable.onRelease(150, 397, 0, 0);
    EXPECT_TRUE(released);
}

TEST(ScrollableWidgetTest, HorizontalScrollbarDragScrolls)
{
    ScrollableWidget scrollable("test", 0, 0, 300, 400);
    scrollable.setContentSize(600, 800);
    scrollable.setActive(true);
    scrollable.setVisible(true);

    // 点击水平滚动条开始拖动
    scrollable.onClick(150, 397, 0, 0);

    // 拖动应改变水平滚动位置
    bool dragged = scrollable.onDrag(200, 397, 50, 0, 0);
    EXPECT_TRUE(dragged);
    // 拖动后scrollX应该有变化（deltaX=50应导致水平滚动）
    EXPECT_GT(scrollable.scrollX(), 0);
}

TEST(ScrollableWidgetTest, VerticalScrollbarDragAndRelease)
{
    ScrollableWidget scrollable("test", 0, 0, 300, 400);
    scrollable.setContentHeight(1000);
    scrollable.setActive(true);
    scrollable.setVisible(true);

    // 点击垂直滚动条开始拖动
    bool clicked = scrollable.onClick(297, 100, 0, 0);
    EXPECT_TRUE(clicked);

    // 释放鼠标结束拖动
    bool released = scrollable.onRelease(297, 100, 0, 0);
    EXPECT_TRUE(released);
}

TEST(ScrollableWidgetTest, ScrollbarClickDoesNotReachChildren)
{
    ScrollableWidget scrollable("test", 0, 0, 300, 400);
    scrollable.setContentSize(600, 1000);
    scrollable.setActive(true);
    scrollable.setVisible(true);

    // 点击垂直滚动条区域应被滚动条消费，不传递给子组件
    bool clicked = scrollable.onClick(297, 100, 0, 0);
    EXPECT_TRUE(clicked);

    // 点击水平滚动条区域也应被滚动条消费
    bool clickedH = scrollable.onClick(150, 397, 0, 0);
    EXPECT_TRUE(clickedH);
}

TEST(ScrollableWidgetTest, ScrollByXClampsCorrectly)
{
    ScrollableWidget scrollable("test", 0, 0, 300, 400);
    scrollable.setContentSize(600, 800);

    // 正常滚动
    scrollable.scrollByX(50);
    EXPECT_EQ(50, scrollable.scrollX());

    // 滚动超过最大值应被限制
    scrollable.scrollByX(999);
    // maxScrollX = 600 - 294 = 306 (294 = visibleWidth with scrollbar)
    EXPECT_EQ(306, scrollable.scrollX());

    // 负滚动应被限制到0
    scrollable.scrollByX(-999);
    EXPECT_EQ(0, scrollable.scrollX());
}

TEST(ScrollableWidgetTest, ShiftKeyTracking)
{
    ScrollableWidget scrollable("test", 0, 0, 300, 400);
    scrollable.setContentSize(600, 800);
    scrollable.setActive(true);
    scrollable.setVisible(true);
    scrollable.setFocused(true);

    // 按下Shift应更新m_shiftHeld但不消费事件
    bool result = scrollable.onKey(Keys::LeftShift, 0, static_cast<i32>(KeyAction::Press), 0);
    EXPECT_FALSE(result); // Shift事件不被消费

    // 释放Shift应重置m_shiftHeld
    result = scrollable.onKey(Keys::LeftShift, 0, static_cast<i32>(KeyAction::Release), 0);
    EXPECT_FALSE(result); // Release事件也不被消费
}

TEST(ScrollableWidgetTest, OnKeyUnfocusedDoesNotScroll)
{
    ScrollableWidget scrollable("test", 0, 0, 300, 400);
    scrollable.setContentSize(600, 800);
    scrollable.setActive(true);
    scrollable.setVisible(true);
    // 未设置焦点

    bool result = scrollable.onKey(Keys::Right, 0, static_cast<i32>(KeyAction::Press), 0);
    EXPECT_FALSE(result);
    EXPECT_EQ(0, scrollable.scrollX()); // 未聚焦时不应该滚动
}
