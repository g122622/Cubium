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
 * @file LayoutConstraintsTest.cpp
 * @brief LayoutConstraints 单元测试
 *
 * 测试覆盖率目标：95%+
 */

#include "client/ui/kagero/layout/constraints/LayoutConstraints.hpp"
#include "client/ui/kagero/layout/core/MeasureSpec.hpp"
#include <limits>
#include <gtest/gtest.h>

using namespace mc::client::ui::kagero;
using namespace mc::client::ui::kagero::layout;
using mc::i32;

class LayoutConstraintsTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

// ============================================================================
// 默认构造测试
// ============================================================================

TEST_F(LayoutConstraintsTest, DefaultConstruction)
{
    LayoutConstraints c;

    EXPECT_EQ(c.minWidth, 0);
    EXPECT_EQ(c.minHeight, 0);
    EXPECT_EQ(c.preferredWidth, -1); // -1 表示无偏好
    EXPECT_EQ(c.preferredHeight, -1);
    EXPECT_EQ(c.maxWidth, std::numeric_limits<i32>::max());
    EXPECT_EQ(c.maxHeight, std::numeric_limits<i32>::max());
    EXPECT_TRUE(c.enabled);
    EXPECT_EQ(c.aspectRatio, 0.0f);
}

// ============================================================================
// 工厂方法测试
// ============================================================================

TEST_F(LayoutConstraintsTest, Fixed)
{
    LayoutConstraints c = LayoutConstraints::fixed(100, 200);

    EXPECT_EQ(c.preferredWidth, 100);
    EXPECT_EQ(c.preferredHeight, 200);
    EXPECT_EQ(c.minWidth, 100);
    EXPECT_EQ(c.minHeight, 200);
    EXPECT_EQ(c.maxWidth, 100);
    EXPECT_EQ(c.maxHeight, 200);
}

TEST_F(LayoutConstraintsTest, Flexible)
{
    LayoutConstraints c = LayoutConstraints::flexible(50, 30);

    EXPECT_EQ(c.minWidth, 50);
    EXPECT_EQ(c.minHeight, 30);
    EXPECT_EQ(c.preferredWidth, -1);
    EXPECT_EQ(c.preferredHeight, -1);
    EXPECT_FLOAT_EQ(c.flex.grow, 1.0f);
}

TEST_F(LayoutConstraintsTest, WrapContent)
{
    LayoutConstraints c = LayoutConstraints::wrapContent();

    EXPECT_EQ(c.minWidth, 0);
    EXPECT_EQ(c.minHeight, 0);
    EXPECT_EQ(c.preferredWidth, -1);
    EXPECT_EQ(c.preferredHeight, -1);
    EXPECT_FLOAT_EQ(c.flex.grow, 0.0f);
}

TEST_F(LayoutConstraintsTest, FillParent)
{
    LayoutConstraints c = LayoutConstraints::fillParent();

    EXPECT_FLOAT_EQ(c.flex.grow, 1.0f);
    EXPECT_EQ(c.alignSelf, Align::Stretch);
}

// ============================================================================
// resolveWidth/resolveHeight 测试
// ============================================================================

TEST_F(LayoutConstraintsTest, ResolveWidthExactly)
{
    LayoutConstraints c;
    c.preferredWidth = 100;
    MeasureSpec spec = MeasureSpec::MakeExactly(200);

    // Exactly模式：返回规格尺寸，但受约束限制
    MeasureSpec result = c.resolveWidth(spec);
    EXPECT_TRUE(result.isExactly());
    EXPECT_EQ(result.size, 200); // 减去边距后
}

TEST_F(LayoutConstraintsTest, ResolveWidthExactlyWithMargin)
{
    LayoutConstraints c;
    c.preferredWidth = 100;
    c.margin = Margin(10, 5); // 左右各10，共20

    MeasureSpec spec = MeasureSpec::MakeExactly(200);
    MeasureSpec result = c.resolveWidth(spec);

    // 可用宽度 = 200 - 20 = 180
    EXPECT_TRUE(result.isExactly());
    EXPECT_EQ(result.size, 180);
}

TEST_F(LayoutConstraintsTest, ResolveWidthAtMost)
{
    LayoutConstraints c;
    c.preferredWidth = 100;

    MeasureSpec spec = MeasureSpec::MakeAtMost(200);
    MeasureSpec result = c.resolveWidth(spec);

    EXPECT_TRUE(result.isAtMost());
    EXPECT_EQ(result.size, 100);
}

TEST_F(LayoutConstraintsTest, ResolveWidthAtMostWithConstraint)
{
    LayoutConstraints c;
    c.preferredWidth = 150;
    c.maxWidth = 120; // 最大宽度限制

    MeasureSpec spec = MeasureSpec::MakeAtMost(200);
    MeasureSpec result = c.resolveWidth(spec);

    // 期望值被maxWidth限制
    EXPECT_TRUE(result.isAtMost());
    EXPECT_EQ(result.size, 120);
}

TEST_F(LayoutConstraintsTest, ResolveWidthUnspecified)
{
    LayoutConstraints c;
    c.preferredWidth = 100;

    MeasureSpec spec = MeasureSpec::MakeUnspecified();
    MeasureSpec result = c.resolveWidth(spec);

    EXPECT_TRUE(result.isAtMost());
    EXPECT_EQ(result.size, 100);
}

TEST_F(LayoutConstraintsTest, ResolveHeightExactly)
{
    LayoutConstraints c;
    c.preferredHeight = 150;

    MeasureSpec spec = MeasureSpec::MakeExactly(300);
    MeasureSpec result = c.resolveHeight(spec);

    EXPECT_TRUE(result.isExactly());
    EXPECT_EQ(result.size, 300);
}

TEST_F(LayoutConstraintsTest, ResolveHeightAtMost)
{
    LayoutConstraints c;
    c.preferredHeight = 100;

    MeasureSpec spec = MeasureSpec::MakeAtMost(200);
    MeasureSpec result = c.resolveHeight(spec);

    EXPECT_TRUE(result.isAtMost());
    EXPECT_EQ(result.size, 100);
}

// ============================================================================
// 辅助方法测试
// ============================================================================

TEST_F(LayoutConstraintsTest, HasPreferredSize)
{
    LayoutConstraints c1;
    EXPECT_FALSE(c1.hasPreferredSize());

    LayoutConstraints c2;
    c2.preferredWidth = 100;
    EXPECT_FALSE(c2.hasPreferredSize()); // 还需要 preferredHeight

    c2.preferredHeight = 50;
    EXPECT_TRUE(c2.hasPreferredSize());
}

TEST_F(LayoutConstraintsTest, HasMinConstraints)
{
    LayoutConstraints c;
    EXPECT_FALSE(c.hasMinConstraints());

    c.minWidth = 10;
    EXPECT_TRUE(c.hasMinConstraints());

    c.minWidth = 0;
    c.minHeight = 10;
    EXPECT_TRUE(c.hasMinConstraints());
}

TEST_F(LayoutConstraintsTest, HasMaxConstraints)
{
    LayoutConstraints c;
    EXPECT_FALSE(c.hasMaxConstraints());

    c.maxWidth = 1000;
    EXPECT_TRUE(c.hasMaxConstraints());

    c.maxWidth = std::numeric_limits<i32>::max();
    c.maxHeight = 800;
    EXPECT_TRUE(c.hasMaxConstraints());
}

TEST_F(LayoutConstraintsTest, ClampWidth)
{
    LayoutConstraints c;
    c.minWidth = 50;
    c.maxWidth = 200;

    EXPECT_EQ(c.clampWidth(30), 50);   // 低于最小值
    EXPECT_EQ(c.clampWidth(100), 100); // 在范围内
    EXPECT_EQ(c.clampWidth(300), 200); // 超过最大值
}

TEST_F(LayoutConstraintsTest, ClampHeight)
{
    LayoutConstraints c;
    c.minHeight = 30;
    c.maxHeight = 150;

    EXPECT_EQ(c.clampHeight(20), 30);
    EXPECT_EQ(c.clampHeight(80), 80);
    EXPECT_EQ(c.clampHeight(200), 150);
}

TEST_F(LayoutConstraintsTest, TotalWidth)
{
    LayoutConstraints c;
    c.margin = Margin(10, 20); // 左右各10

    EXPECT_EQ(c.totalWidth(100), 120); // 100 + 20
}

TEST_F(LayoutConstraintsTest, TotalHeight)
{
    LayoutConstraints c;
    c.margin = Margin(0, 15); // 上下各15

    EXPECT_EQ(c.totalHeight(100), 130); // 100 + 30
}

TEST_F(LayoutConstraintsTest, IsLayoutEnabled)
{
    LayoutConstraints c;
    EXPECT_TRUE(c.isLayoutEnabled());

    c.enabled = false;
    EXPECT_FALSE(c.isLayoutEnabled());
}

// ============================================================================
// FlexItem 测试
// ============================================================================

class FlexItemTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(FlexItemTest, DefaultValues)
{
    FlexItem item;

    EXPECT_FLOAT_EQ(item.grow, 0.0f);
    EXPECT_FLOAT_EQ(item.shrink, 1.0f);
    EXPECT_EQ(item.basis, -1);
    EXPECT_EQ(item.alignSelf, Align::Stretch);
    EXPECT_EQ(item.minWidth, 0);
    EXPECT_EQ(item.maxWidth, std::numeric_limits<i32>::max());
    EXPECT_EQ(item.minHeight, 0);
    EXPECT_EQ(item.maxHeight, std::numeric_limits<i32>::max());
}

TEST_F(FlexItemTest, CanGrow)
{
    FlexItem item;
    EXPECT_FALSE(item.canGrow());

    item.grow = 1.0f;
    EXPECT_TRUE(item.canGrow());

    item.grow = 0.5f;
    EXPECT_TRUE(item.canGrow());
}

TEST_F(FlexItemTest, CanShrink)
{
    FlexItem item;
    EXPECT_TRUE(item.canShrink());

    item.shrink = 0.0f;
    EXPECT_FALSE(item.canShrink());
}

TEST_F(FlexItemTest, HasWidthConstraints)
{
    FlexItem item;
    EXPECT_FALSE(item.hasWidthConstraints());

    item.minWidth = 10;
    EXPECT_TRUE(item.hasWidthConstraints());

    item.minWidth = 0;
    item.maxWidth = 100;
    EXPECT_TRUE(item.hasWidthConstraints());
}

TEST_F(FlexItemTest, HasHeightConstraints)
{
    FlexItem item;
    EXPECT_FALSE(item.hasHeightConstraints());

    item.minHeight = 20;
    EXPECT_TRUE(item.hasHeightConstraints());
}

TEST_F(FlexItemTest, ClampWidth)
{
    FlexItem item;
    item.minWidth = 50;
    item.maxWidth = 200;

    EXPECT_EQ(item.clampWidth(30), 50);
    EXPECT_EQ(item.clampWidth(100), 100);
    EXPECT_EQ(item.clampWidth(300), 200);
}

TEST_F(FlexItemTest, ClampHeight)
{
    FlexItem item;
    item.minHeight = 30;
    item.maxHeight = 150;

    EXPECT_EQ(item.clampHeight(20), 30);
    EXPECT_EQ(item.clampHeight(100), 100);
    EXPECT_EQ(item.clampHeight(200), 150);
}

// ============================================================================
// GridItem 测试
// ============================================================================

class GridItemTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(GridItemTest, DefaultValues)
{
    GridItem item;

    // 默认值为 -1 表示需要自动放置
    EXPECT_EQ(item.column, -1);
    EXPECT_EQ(item.row, -1);
    EXPECT_EQ(item.columnSpan, 1);
    EXPECT_EQ(item.rowSpan, 1);
    EXPECT_EQ(item.alignSelf, Align::Stretch);
}

TEST_F(GridItemTest, IsSingleCell)
{
    GridItem item;
    EXPECT_TRUE(item.isSingleCell());

    item.columnSpan = 2;
    EXPECT_FALSE(item.isSingleCell());

    item.columnSpan = 1;
    item.rowSpan = 3;
    EXPECT_FALSE(item.isSingleCell());
}

TEST_F(GridItemTest, ColumnEnd)
{
    GridItem item;
    item.column = 1;
    item.columnSpan = 3;

    EXPECT_EQ(item.columnEnd(), 4);
}

TEST_F(GridItemTest, RowEnd)
{
    GridItem item;
    item.row = 2;
    item.rowSpan = 2;

    EXPECT_EQ(item.rowEnd(), 4);
}

// ============================================================================
// AnchorConstraints 测试
// ============================================================================

class AnchorConstraintsTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(AnchorConstraintsTest, DefaultValues)
{
    AnchorConstraints ac;

    EXPECT_FALSE(ac.left.has_value());
    EXPECT_FALSE(ac.top.has_value());
    EXPECT_FALSE(ac.right.has_value());
    EXPECT_FALSE(ac.bottom.has_value());
    EXPECT_FALSE(ac.centerX);
    EXPECT_FALSE(ac.centerY);
    EXPECT_EQ(ac.offsetX, 0);
    EXPECT_EQ(ac.offsetY, 0);
}

TEST_F(AnchorConstraintsTest, IsTopLeft)
{
    AnchorConstraints ac;
    EXPECT_FALSE(ac.isTopLeft());

    ac.left = 10;
    EXPECT_FALSE(ac.isTopLeft());

    ac.top = 20;
    EXPECT_TRUE(ac.isTopLeft());

    ac.right = 100;
    EXPECT_FALSE(ac.isTopLeft()); // 有right约束，不是纯左上角
}

TEST_F(AnchorConstraintsTest, IsStretchHorizontal)
{
    AnchorConstraints ac;
    EXPECT_FALSE(ac.isStretchHorizontal());

    ac.left = 10;
    EXPECT_FALSE(ac.isStretchHorizontal());

    ac.right = 20;
    EXPECT_TRUE(ac.isStretchHorizontal());
}

TEST_F(AnchorConstraintsTest, IsStretchVertical)
{
    AnchorConstraints ac;
    EXPECT_FALSE(ac.isStretchVertical());

    ac.top = 10;
    ac.bottom = 20;
    EXPECT_TRUE(ac.isStretchVertical());
}

TEST_F(AnchorConstraintsTest, HasPercentPosition)
{
    AnchorConstraints ac;
    EXPECT_FALSE(ac.hasPercentPosition());

    ac.leftPercent = 0.5f;
    EXPECT_TRUE(ac.hasPercentPosition());

    ac.leftPercent.reset();
    ac.topPercent = 0.3f;
    EXPECT_TRUE(ac.hasPercentPosition());
}

// ============================================================================
// 枚举类型测试
// ============================================================================

TEST(LayoutEnumTest, AlignValues)
{
    // 确保枚举值正确
    EXPECT_EQ(static_cast<int>(Align::Start), 0);
    EXPECT_EQ(static_cast<int>(Align::Center), 1);
    EXPECT_EQ(static_cast<int>(Align::End), 2);
    EXPECT_EQ(static_cast<int>(Align::Stretch), 3);
    EXPECT_EQ(static_cast<int>(Align::Baseline), 4);
}

TEST(LayoutEnumTest, DirectionValues)
{
    EXPECT_EQ(static_cast<int>(Direction::Row), 0);
    EXPECT_EQ(static_cast<int>(Direction::RowReverse), 1);
    EXPECT_EQ(static_cast<int>(Direction::Column), 2);
    EXPECT_EQ(static_cast<int>(Direction::ColumnReverse), 3);
}

TEST(LayoutEnumTest, JustifyContentValues)
{
    EXPECT_EQ(static_cast<int>(JustifyContent::Start), 0);
    EXPECT_EQ(static_cast<int>(JustifyContent::Center), 1);
    EXPECT_EQ(static_cast<int>(JustifyContent::End), 2);
    EXPECT_EQ(static_cast<int>(JustifyContent::SpaceBetween), 3);
    EXPECT_EQ(static_cast<int>(JustifyContent::SpaceAround), 4);
    EXPECT_EQ(static_cast<int>(JustifyContent::SpaceEvenly), 5);
}

TEST(LayoutEnumTest, WrapValues)
{
    EXPECT_EQ(static_cast<int>(Wrap::NoWrap), 0);
    EXPECT_EQ(static_cast<int>(Wrap::Wrap), 1);
    EXPECT_EQ(static_cast<int>(Wrap::WrapReverse), 2);
}
