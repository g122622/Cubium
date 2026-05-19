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
 * @file LayoutEngineTest.cpp
 * @brief LayoutEngine 和相关核心组件单元测试
 *
 * 测试覆盖率目标：95%+
 *
 * 注意：本测试文件专注于不依赖具体Widget的核心组件测试
 */

#include "client/ui/kagero/layout/core/LayoutEngine.hpp"
#include "client/ui/kagero/layout/constraints/LayoutConstraints.hpp"
#include "client/ui/kagero/layout/core/LayoutResult.hpp"
#include "client/ui/kagero/layout/core/MeasureSpec.hpp"
#include <limits>
#include <gtest/gtest.h>

using namespace mc::client::ui::kagero;
using namespace mc::client::ui::kagero::layout;
using mc::i32;

// ============================================================================
// LayoutResult 测试
// ============================================================================

class LayoutResultTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(LayoutResultTest, DefaultConstruction)
{
    LayoutResult result;

    EXPECT_EQ(result.bounds.x, 0);
    EXPECT_EQ(result.bounds.y, 0);
    EXPECT_EQ(result.bounds.width, 0);
    EXPECT_EQ(result.bounds.height, 0);
    EXPECT_TRUE(result.needsRepaint);
    EXPECT_FALSE(result.needsRelayout);
}

TEST_F(LayoutResultTest, ParameterizedConstruction)
{
    LayoutResult result(10, 20, 100, 50);

    EXPECT_EQ(result.bounds.x, 10);
    EXPECT_EQ(result.bounds.y, 20);
    EXPECT_EQ(result.bounds.width, 100);
    EXPECT_EQ(result.bounds.height, 50);
}

TEST_F(LayoutResultTest, IsValid)
{
    LayoutResult valid(0, 0, 100, 50);
    EXPECT_TRUE(valid.isValid());

    LayoutResult zeroSize(0, 0, 0, 0);
    EXPECT_TRUE(zeroSize.isValid()); // 零尺寸也是有效的

    LayoutResult negativeWidth(0, 0, -10, 50);
    EXPECT_FALSE(negativeWidth.isValid());

    LayoutResult negativeHeight(0, 0, 100, -5);
    EXPECT_FALSE(negativeHeight.isValid());
}

TEST_F(LayoutResultTest, CenterXY)
{
    LayoutResult result(0, 0, 100, 50);

    EXPECT_EQ(result.centerX(), 50);
    EXPECT_EQ(result.centerY(), 25);
}

TEST_F(LayoutResultTest, RightBottom)
{
    LayoutResult result(10, 20, 100, 50);

    EXPECT_EQ(result.right(), 110); // x + width
    EXPECT_EQ(result.bottom(), 70); // y + height
}

TEST_F(LayoutResultTest, Contains)
{
    LayoutResult result(10, 20, 100, 50);

    EXPECT_TRUE(result.contains(10, 20));   // 左上角
    EXPECT_TRUE(result.contains(50, 45));   // 中心
    EXPECT_TRUE(result.contains(109, 69));  // 右下角（不含）
    EXPECT_FALSE(result.contains(10, 19));  // 上边界外
    EXPECT_FALSE(result.contains(9, 20));   // 左边界外
    EXPECT_FALSE(result.contains(110, 20)); // 右边界外
    EXPECT_FALSE(result.contains(10, 70));  // 下边界外
}

TEST_F(LayoutResultTest, Merged)
{
    LayoutResult result1(0, 0, 100, 50);
    LayoutResult result2(50, 25, 100, 50);

    LayoutResult merged = result1.merged(result2);

    EXPECT_EQ(merged.bounds.x, 0);
    EXPECT_EQ(merged.bounds.y, 0);
    EXPECT_EQ(merged.bounds.width, 150); // max(right) - min(x)
    EXPECT_EQ(merged.bounds.height, 75); // max(bottom) - min(y)
}

TEST_F(LayoutResultTest, MergedWithInvalid)
{
    LayoutResult valid(0, 0, 100, 50);
    LayoutResult invalid;
    invalid.bounds.width = -10; // 无效

    LayoutResult merged1 = valid.merged(invalid);
    EXPECT_TRUE(merged1.isValid());
    EXPECT_EQ(merged1.bounds.width, 100);

    LayoutResult merged2 = invalid.merged(valid);
    EXPECT_TRUE(merged2.isValid());
    EXPECT_EQ(merged2.bounds.width, 100);
}

// ============================================================================
// LayoutContext 测试
// ============================================================================

class LayoutContextTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(LayoutContextTest, DefaultConstruction)
{
    LayoutContext ctx;

    EXPECT_EQ(ctx.availableWidth(), 0);
    EXPECT_EQ(ctx.availableHeight(), 0);
    EXPECT_EQ(ctx.depth(), 0);
}

TEST_F(LayoutContextTest, ParameterizedConstruction)
{
    LayoutContext ctx(400, 300);

    EXPECT_EQ(ctx.availableWidth(), 400);
    EXPECT_EQ(ctx.availableHeight(), 300);
}

TEST_F(LayoutContextTest, Setters)
{
    LayoutContext ctx;

    ctx.setAvailableWidth(800);
    ctx.setAvailableHeight(600);
    ctx.setDepth(2);

    EXPECT_EQ(ctx.availableWidth(), 800);
    EXPECT_EQ(ctx.availableHeight(), 600);
    EXPECT_EQ(ctx.depth(), 2);
}

TEST_F(LayoutContextTest, CreateChildContext)
{
    LayoutContext parent(400, 300);
    parent.setDepth(1);

    LayoutContext child = parent.createChildContext(200, 150);

    EXPECT_EQ(child.availableWidth(), 200);
    EXPECT_EQ(child.availableHeight(), 150);
    EXPECT_EQ(child.depth(), 2); // 深度+1
}

TEST_F(LayoutContextTest, IsValid)
{
    LayoutContext valid(400, 300);
    EXPECT_TRUE(valid.isValid());

    LayoutContext invalid(-10, 300);
    EXPECT_FALSE(invalid.isValid());

    LayoutContext invalid2(400, -5);
    EXPECT_FALSE(invalid2.isValid());
}

// ============================================================================
// LayoutStats 测试
// ============================================================================

class LayoutStatsTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(LayoutStatsTest, DefaultValues)
{
    LayoutStats stats;

    EXPECT_EQ(stats.totalWidgets, 0);
    EXPECT_EQ(stats.relayoutedWidgets, 0);
    EXPECT_EQ(stats.totalTimeMs, 0.0);
    EXPECT_EQ(stats.maxDepth, 0);
    EXPECT_EQ(stats.measureCount, 0);
    EXPECT_EQ(stats.layoutCount, 0);
}

TEST_F(LayoutStatsTest, Reset)
{
    LayoutStats stats;
    stats.totalWidgets = 100;
    stats.relayoutedWidgets = 50;
    stats.totalTimeMs = 5.0;
    stats.maxDepth = 10;
    stats.measureCount = 200;
    stats.layoutCount = 100;

    stats.reset();

    EXPECT_EQ(stats.totalWidgets, 0);
    EXPECT_EQ(stats.relayoutedWidgets, 0);
    EXPECT_EQ(stats.totalTimeMs, 0.0);
    EXPECT_EQ(stats.maxDepth, 0);
    EXPECT_EQ(stats.measureCount, 0);
    EXPECT_EQ(stats.layoutCount, 0);
}

// ============================================================================
// LayoutEngine 测试
// ============================================================================

class LayoutEngineTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        engine = &LayoutEngine::instance();
        engine->resetStats();
    }

    void TearDown() override { engine->resetStats(); }

    LayoutEngine* engine;
};

TEST_F(LayoutEngineTest, Instance)
{
    LayoutEngine& instance1 = LayoutEngine::instance();
    LayoutEngine& instance2 = LayoutEngine::instance();

    EXPECT_EQ(&instance1, &instance2); // 单例
}

TEST_F(LayoutEngineTest, RegisterAlgorithm)
{
    auto algorithm = std::make_unique<FlexLayoutAlgorithm>();
    engine->registerAlgorithm("test_algo", std::move(algorithm));

    EXPECT_TRUE(engine->hasAlgorithm("test_algo"));
    EXPECT_NE(engine->getAlgorithm("test_algo"), nullptr);
}

TEST_F(LayoutEngineTest, GetNonexistentAlgorithm)
{
    ILayoutAlgorithm* algo = engine->getAlgorithm("nonexistent");
    EXPECT_EQ(algo, nullptr);

    EXPECT_FALSE(engine->hasAlgorithm("nonexistent"));
}

TEST_F(LayoutEngineTest, BuiltInAlgorithms)
{
    EXPECT_TRUE(engine->hasAlgorithm("flex"));
    EXPECT_TRUE(engine->hasAlgorithm("flex-row"));
    EXPECT_TRUE(engine->hasAlgorithm("flex-column"));
    EXPECT_TRUE(engine->hasAlgorithm("flex-center"));
    EXPECT_TRUE(engine->hasAlgorithm("grid"));
    EXPECT_TRUE(engine->hasAlgorithm("anchor"));
    EXPECT_TRUE(engine->hasAlgorithm("stack"));
}

TEST_F(LayoutEngineTest, DebugVisualize)
{
    engine->setDebugVisualize(true);
    EXPECT_TRUE(engine->isDebugVisualize());

    engine->setDebugVisualize(false);
    EXPECT_FALSE(engine->isDebugVisualize());
}

TEST_F(LayoutEngineTest, Profiling)
{
    engine->setProfiling(true);
    EXPECT_TRUE(engine->isProfiling());

    engine->setProfiling(false);
    EXPECT_FALSE(engine->isProfiling());
}

// ============================================================================
// FlexLayoutAlgorithm 测试
// ============================================================================

class FlexLayoutAlgorithmTest : public ::testing::Test {
protected:
    void SetUp() override { algorithm = std::make_unique<FlexLayoutAlgorithm>(); }

    void TearDown() override { algorithm.reset(); }

    std::unique_ptr<FlexLayoutAlgorithm> algorithm;
};

TEST_F(FlexLayoutAlgorithmTest, Name)
{
    EXPECT_EQ(algorithm->name(), "flex");
}

TEST_F(FlexLayoutAlgorithmTest, Config)
{
    FlexConfig config;
    config.direction = Direction::Column;
    config.gap = 15;
    config.justifyContent = JustifyContent::SpaceAround;

    algorithm->setConfig(config);

    const FlexConfig& retrieved = algorithm->config();
    EXPECT_EQ(retrieved.direction, Direction::Column);
    EXPECT_EQ(retrieved.gap, 15);
    EXPECT_EQ(retrieved.justifyContent, JustifyContent::SpaceAround);
}

TEST_F(FlexLayoutAlgorithmTest, ComputeEmptyChildren)
{
    std::vector<WidgetLayoutAdaptor*> children;
    Rect container(0, 0, 400, 300);

    auto results = algorithm->compute(container, children, LayoutConstraints{});

    EXPECT_TRUE(results.empty());
}

TEST_F(FlexLayoutAlgorithmTest, MeasureEmptyChildren)
{
    std::vector<WidgetLayoutAdaptor*> children;

    Size size = algorithm->measure(MeasureSpec::MakeUnspecified(), MeasureSpec::MakeUnspecified(), children);

    EXPECT_EQ(size.width, 0);
    EXPECT_EQ(size.height, 0);
}

// ============================================================================
// LayoutType 枚举测试
// ============================================================================

TEST(LayoutTypeTest, EnumValues)
{
    EXPECT_EQ(static_cast<int>(LayoutType::None), 0);
    EXPECT_EQ(static_cast<int>(LayoutType::Flex), 1);
    EXPECT_EQ(static_cast<int>(LayoutType::Grid), 2);
    EXPECT_EQ(static_cast<int>(LayoutType::Anchor), 3);
    EXPECT_EQ(static_cast<int>(LayoutType::Stack), 4);
}
