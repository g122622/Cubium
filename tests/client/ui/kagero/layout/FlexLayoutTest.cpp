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
 * @file FlexLayoutTest.cpp
 * @brief FlexLayout 弹性布局算法单元测试
 *
 * 测试覆盖率目标：95%+
 *
 * 测试场景：
 * - FlexConfig 测试
 * - FlexLayout 基础测试
 * - FlexLine 测试
 * - 辅助函数测试
 */

#include "client/ui/kagero/layout/algorithms/FlexLayout.hpp"
#include "client/ui/kagero/layout/constraints/LayoutConstraints.hpp"
#include "client/ui/kagero/layout/core/MeasureSpec.hpp"
#include <limits>
#include <gtest/gtest.h>

using namespace mc::client::ui::kagero;
using namespace mc::client::ui::kagero::layout;
using mc::i32;

// ============================================================================
// FlexConfig 测试
// ============================================================================

class FlexConfigTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(FlexConfigTest, DefaultValues)
{
    FlexConfig config;

    EXPECT_EQ(config.direction, Direction::Row);
    EXPECT_EQ(config.justifyContent, JustifyContent::Start);
    EXPECT_EQ(config.alignItems, Align::Stretch);
    EXPECT_EQ(config.wrap, Wrap::NoWrap);
    EXPECT_EQ(config.gap, 0);
}

TEST_F(FlexConfigTest, IsHorizontal)
{
    FlexConfig config;

    config.direction = Direction::Row;
    EXPECT_TRUE(config.isHorizontal());

    config.direction = Direction::RowReverse;
    EXPECT_TRUE(config.isHorizontal());

    config.direction = Direction::Column;
    EXPECT_FALSE(config.isHorizontal());

    config.direction = Direction::ColumnReverse;
    EXPECT_FALSE(config.isHorizontal());
}

TEST_F(FlexConfigTest, IsVertical)
{
    FlexConfig config;

    config.direction = Direction::Column;
    EXPECT_TRUE(config.isVertical());

    config.direction = Direction::ColumnReverse;
    EXPECT_TRUE(config.isVertical());

    config.direction = Direction::Row;
    EXPECT_FALSE(config.isVertical());

    config.direction = Direction::RowReverse;
    EXPECT_FALSE(config.isVertical());
}

TEST_F(FlexConfigTest, IsReverse)
{
    FlexConfig config;

    config.direction = Direction::Row;
    EXPECT_FALSE(config.isReverse());

    config.direction = Direction::RowReverse;
    EXPECT_TRUE(config.isReverse());

    config.direction = Direction::Column;
    EXPECT_FALSE(config.isReverse());

    config.direction = Direction::ColumnReverse;
    EXPECT_TRUE(config.isReverse());
}

TEST_F(FlexConfigTest, ShouldWrap)
{
    FlexConfig config;

    config.wrap = Wrap::NoWrap;
    EXPECT_FALSE(config.shouldWrap());

    config.wrap = Wrap::Wrap;
    EXPECT_TRUE(config.shouldWrap());

    config.wrap = Wrap::WrapReverse;
    EXPECT_TRUE(config.shouldWrap());
}

// ============================================================================
// FlexLayout 基础测试
// ============================================================================

class FlexLayoutTest : public ::testing::Test {
protected:
    void SetUp() override { layout = std::make_unique<FlexLayout>(); }

    void TearDown() override { layout.reset(); }

    std::unique_ptr<FlexLayout> layout;
};

TEST_F(FlexLayoutTest, DefaultConfig)
{
    EXPECT_EQ(layout->direction(), Direction::Row);
    EXPECT_EQ(layout->justifyContent(), JustifyContent::Start);
    EXPECT_EQ(layout->alignItems(), Align::Stretch);
    EXPECT_EQ(layout->wrap(), Wrap::NoWrap);
    EXPECT_EQ(layout->gap(), 0);
}

TEST_F(FlexLayoutTest, SetDirection)
{
    layout->setDirection(Direction::Column);
    EXPECT_EQ(layout->direction(), Direction::Column);

    layout->setDirection(Direction::RowReverse);
    EXPECT_EQ(layout->direction(), Direction::RowReverse);
}

TEST_F(FlexLayoutTest, SetJustifyContent)
{
    layout->setJustifyContent(JustifyContent::Center);
    EXPECT_EQ(layout->justifyContent(), JustifyContent::Center);

    layout->setJustifyContent(JustifyContent::SpaceBetween);
    EXPECT_EQ(layout->justifyContent(), JustifyContent::SpaceBetween);
}

TEST_F(FlexLayoutTest, SetAlignItems)
{
    layout->setAlignItems(Align::Center);
    EXPECT_EQ(layout->alignItems(), Align::Center);

    layout->setAlignItems(Align::End);
    EXPECT_EQ(layout->alignItems(), Align::End);
}

TEST_F(FlexLayoutTest, SetWrap)
{
    layout->setWrap(Wrap::Wrap);
    EXPECT_EQ(layout->wrap(), Wrap::Wrap);
}

TEST_F(FlexLayoutTest, SetGap)
{
    layout->setGap(10);
    EXPECT_EQ(layout->gap(), 10);

    layout->setGap(20);
    EXPECT_EQ(layout->gap(), 20);
}

TEST_F(FlexLayoutTest, SetConfig)
{
    FlexConfig config;
    config.direction = Direction::Column;
    config.justifyContent = JustifyContent::SpaceAround;
    config.alignItems = Align::Center;
    config.gap = 15;

    layout->setConfig(config);

    EXPECT_EQ(layout->direction(), Direction::Column);
    EXPECT_EQ(layout->justifyContent(), JustifyContent::SpaceAround);
    EXPECT_EQ(layout->alignItems(), Align::Center);
    EXPECT_EQ(layout->gap(), 15);
}

TEST_F(FlexLayoutTest, EmptyChildren)
{
    std::vector<WidgetLayoutAdaptor*> children;
    Rect container(0, 0, 400, 300);

    auto results = layout->compute(container, children);

    EXPECT_TRUE(results.empty());
}

TEST_F(FlexLayoutTest, MeasureEmptyChildren)
{
    std::vector<WidgetLayoutAdaptor*> children;

    Size size = layout->measure(MeasureSpec::MakeExactly(400), MeasureSpec::MakeExactly(300), children);

    // Exactly规格应该返回规格指定的尺寸
    EXPECT_EQ(size.width, 400);
    EXPECT_EQ(size.height, 300);
}

// ============================================================================
// FlexLine 测试
// ============================================================================

class FlexLineTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(FlexLineTest, DefaultValues)
{
    FlexLine line;

    EXPECT_EQ(line.mainSize, 0);
    EXPECT_EQ(line.crossSize, 0);
    EXPECT_EQ(line.offsetX, 0);
    EXPECT_EQ(line.offsetY, 0);
    EXPECT_FLOAT_EQ(line.totalGrow, 0.0f);
    EXPECT_FLOAT_EQ(line.totalShrink, 0.0f);
    EXPECT_TRUE(line.items.empty());
    EXPECT_TRUE(line.indices.empty());
}

TEST_F(FlexLineTest, ItemCount)
{
    FlexLine line;
    EXPECT_EQ(line.itemCount(), 0u);
}

// ============================================================================
// 辅助函数测试
// ============================================================================

TEST(FlexHelperTest, DefaultFlexConfig)
{
    FlexConfig config = defaultFlexConfig();

    EXPECT_EQ(config.direction, Direction::Row);
    EXPECT_EQ(config.justifyContent, JustifyContent::Start);
    EXPECT_EQ(config.alignItems, Align::Stretch);
    EXPECT_EQ(config.wrap, Wrap::NoWrap);
    EXPECT_EQ(config.gap, 0);
}

TEST(FlexHelperTest, CenterRowFlexConfig)
{
    FlexConfig config = centerRowFlexConfig();

    EXPECT_EQ(config.direction, Direction::Row);
    EXPECT_EQ(config.justifyContent, JustifyContent::Center);
    EXPECT_EQ(config.alignItems, Align::Center);
}

TEST(FlexHelperTest, CenterColumnFlexConfig)
{
    FlexConfig config = centerColumnFlexConfig();

    EXPECT_EQ(config.direction, Direction::Column);
    EXPECT_EQ(config.justifyContent, JustifyContent::Center);
    EXPECT_EQ(config.alignItems, Align::Center);
}

TEST(FlexHelperTest, SpaceBetweenFlexConfig)
{
    FlexConfig config = spaceBetweenFlexConfig();

    EXPECT_EQ(config.direction, Direction::Row);
    EXPECT_EQ(config.justifyContent, JustifyContent::SpaceBetween);
    EXPECT_EQ(config.alignItems, Align::Center);
}
