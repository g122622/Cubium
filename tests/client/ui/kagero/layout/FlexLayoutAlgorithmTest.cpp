/**
 * @file FlexLayoutAlgorithmTest.cpp
 * @brief FlexLayout 算法详细测试
 *
 * 测试覆盖率目标：95%+
 *
 * 测试场景：
 * - 单元素布局
 * - 多元素水平布局
 * - 多元素垂直布局
 * - 主轴对齐测试
 * - 交叉轴对齐测试
 * - 弹性增长/缩小测试
 * - 换行和反向布局测试
 */

#include <gtest/gtest.h>
#include "client/ui/kagero/layout/algorithms/FlexLayout.hpp"
#include "client/ui/kagero/layout/core/MeasureSpec.hpp"
#include "client/ui/kagero/layout/constraints/LayoutConstraints.hpp"
#include "client/ui/kagero/layout/integration/WidgetLayoutAdaptor.hpp"
#include "client/ui/kagero/widget/Widget.hpp"
#include <memory>
#include <vector>

using namespace mc::client::ui::kagero;
using namespace mc::client::ui::kagero::layout;
using namespace mc::client::ui::kagero::widget;
using mc::i32;

// ============================================================================
// Mock Widget 类
// ============================================================================

/**
 * @brief 简单的 Mock Widget，用于布局测试
 */
class TestWidget : public Widget {
public:
    explicit TestWidget(const std::string& id = "", i32 w = 100, i32 h = 50)
        : Widget(id) {
        setSize(w, h);
    }

    void paint(PaintContext& /*ctx*/) override {
        // Mock implementation - do nothing
    }
};

// ============================================================================
// 测试辅助函数
// ============================================================================

/**
 * @brief 创建一个固定尺寸的适配器
 */
inline std::pair<std::unique_ptr<TestWidget>, std::unique_ptr<WidgetLayoutAdaptor>>
createTestAdaptor(const std::string& id, i32 width, i32 height, const LayoutConstraints& constraints = LayoutConstraints{}) {
    auto widget = std::make_unique<TestWidget>(id, width, height);
    auto adaptor = std::make_unique<WidgetLayoutAdaptor>(widget.get());
    adaptor->setConstraints(constraints);

    // 设置自定义测量函数，返回固定尺寸
    adaptor->setMeasureFunc([width, height](WidgetLayoutAdaptor*, const MeasureSpec&, const MeasureSpec&) {
        return Size(width, height);
    });

    return {std::move(widget), std::move(adaptor)};
}

// ============================================================================
// 测试基类
// ============================================================================

class FlexLayoutDetailedTest : public ::testing::Test {
protected:
    void SetUp() override {
        layout = std::make_unique<FlexLayout>();
    }

    void TearDown() override {
        layout.reset();
        widgets.clear();
        adaptors.clear();
    }

    /**
     * @brief 添加一个测试 Widget
     */
    WidgetLayoutAdaptor* addWidget(i32 width, i32 height,
                                   const LayoutConstraints& constraints = LayoutConstraints{}) {
        std::string id = std::string("widget_") + std::to_string(idCounter++);
        auto [widget, adaptor] = createTestAdaptor(id, width, height, constraints);
        auto* ptr = adaptor.get();
        widgets.push_back(std::move(widget));
        adaptors.push_back(std::move(adaptor));
        return ptr;
    }

    /**
     * @brief 获取所有适配器指针
     */
    std::vector<WidgetLayoutAdaptor*> getAdaptors() {
        std::vector<WidgetLayoutAdaptor*> result;
        for (const auto& adaptor : adaptors) {
            result.push_back(adaptor.get());
        }
        return result;
    }

    std::unique_ptr<FlexLayout> layout;
    std::vector<std::unique_ptr<TestWidget>> widgets;
    std::vector<std::unique_ptr<WidgetLayoutAdaptor>> adaptors;
    int idCounter = 0;
};

// ============================================================================
// 单元素布局测试
// ============================================================================

TEST_F(FlexLayoutDetailedTest, SingleElementBasic) {
    addWidget(100, 50);
    auto children = getAdaptors();

    Rect container(0, 0, 400, 300);
    auto results = layout->compute(container, children);

    ASSERT_EQ(results.size(), 1u);
    EXPECT_EQ(results[0].bounds.width, 100);
    EXPECT_EQ(results[0].bounds.height, 50);
    EXPECT_EQ(results[0].bounds.x, 0);
    EXPECT_EQ(results[0].bounds.y, 0);
}

TEST_F(FlexLayoutDetailedTest, SingleElementCenter) {
    layout->setJustifyContent(JustifyContent::Center);
    layout->setAlignItems(Align::Center);

    addWidget(100, 50);
    auto children = getAdaptors();

    Rect container(0, 0, 400, 300);
    auto results = layout->compute(container, children);

    ASSERT_EQ(results.size(), 1u);
    EXPECT_EQ(results[0].bounds.width, 100);
    EXPECT_EQ(results[0].bounds.height, 50);
    // 水平居中: (400 - 100) / 2 = 150
    EXPECT_EQ(results[0].bounds.x, 150);
    // 垂直居中: (300 - 50) / 2 = 125
    EXPECT_EQ(results[0].bounds.y, 125);
}

TEST_F(FlexLayoutDetailedTest, SingleElementEnd) {
    layout->setJustifyContent(JustifyContent::End);
    layout->setAlignItems(Align::End);

    addWidget(100, 50);
    auto children = getAdaptors();

    Rect container(0, 0, 400, 300);
    auto results = layout->compute(container, children);

    ASSERT_EQ(results.size(), 1u);
    EXPECT_EQ(results[0].bounds.x, 300);  // 400 - 100
    EXPECT_EQ(results[0].bounds.y, 250);  // 300 - 50
}

// ============================================================================
// 多元素水平布局测试
// ============================================================================

TEST_F(FlexLayoutDetailedTest, MultipleElementsHorizontal) {
    addWidget(100, 50);
    addWidget(100, 50);
    addWidget(100, 50);
    auto children = getAdaptors();

    Rect container(0, 0, 400, 300);
    auto results = layout->compute(container, children);

    ASSERT_EQ(results.size(), 3u);

    EXPECT_EQ(results[0].bounds.width, 100);
    EXPECT_EQ(results[1].bounds.width, 100);
    EXPECT_EQ(results[2].bounds.width, 100);

    EXPECT_EQ(results[0].bounds.x, 0);
    EXPECT_EQ(results[1].bounds.x, 100);
    EXPECT_EQ(results[2].bounds.x, 200);
}

TEST_F(FlexLayoutDetailedTest, MultipleElementsWithGap) {
    layout->setGap(10);

    addWidget(100, 50);
    addWidget(100, 50);
    addWidget(100, 50);
    auto children = getAdaptors();

    Rect container(0, 0, 400, 300);
    auto results = layout->compute(container, children);

    ASSERT_EQ(results.size(), 3u);

    EXPECT_EQ(results[0].bounds.x, 0);
    EXPECT_EQ(results[1].bounds.x, 110);  // 100 + 10
    EXPECT_EQ(results[2].bounds.x, 220);  // 100 + 10 + 100 + 10
}

// ============================================================================
// 垂直布局测试
// ============================================================================

TEST_F(FlexLayoutDetailedTest, VerticalLayout) {
    layout->setDirection(Direction::Column);
    layout->setAlignItems(Align::Start);

    addWidget(100, 50);
    addWidget(100, 50);
    addWidget(100, 50);
    auto children = getAdaptors();

    Rect container(0, 0, 400, 300);
    auto results = layout->compute(container, children);

    ASSERT_EQ(results.size(), 3u);

    EXPECT_EQ(results[0].bounds.y, 0);
    EXPECT_EQ(results[1].bounds.y, 50);
    EXPECT_EQ(results[2].bounds.y, 100);
}

TEST_F(FlexLayoutDetailedTest, VerticalLayoutWithGap) {
    layout->setDirection(Direction::Column);
    layout->setGap(10);
    layout->setAlignItems(Align::Start);

    addWidget(100, 50);
    addWidget(100, 50);
    auto children = getAdaptors();

    Rect container(0, 0, 400, 300);
    auto results = layout->compute(container, children);

    ASSERT_EQ(results.size(), 2u);

    EXPECT_EQ(results[0].bounds.y, 0);
    EXPECT_EQ(results[1].bounds.y, 60);  // 50 + 10
}

// ============================================================================
// 主轴对齐测试
// ============================================================================

TEST_F(FlexLayoutDetailedTest, JustifyContentCenter) {
    layout->setJustifyContent(JustifyContent::Center);
    layout->setAlignItems(Align::Start);

    addWidget(100, 50);
    addWidget(100, 50);
    auto children = getAdaptors();

    Rect container(0, 0, 400, 300);
    auto results = layout->compute(container, children);

    ASSERT_EQ(results.size(), 2u);

    // 总宽度 200，剩余空间 200，居中偏移 = 100
    EXPECT_EQ(results[0].bounds.x, 100);
    EXPECT_EQ(results[1].bounds.x, 200);
}

TEST_F(FlexLayoutDetailedTest, JustifyContentEnd) {
    layout->setJustifyContent(JustifyContent::End);
    layout->setAlignItems(Align::Start);

    addWidget(100, 50);
    addWidget(100, 50);
    auto children = getAdaptors();

    Rect container(0, 0, 400, 300);
    auto results = layout->compute(container, children);

    ASSERT_EQ(results.size(), 2u);

    EXPECT_EQ(results[0].bounds.x, 200);
    EXPECT_EQ(results[1].bounds.x, 300);
}

TEST_F(FlexLayoutDetailedTest, JustifyContentSpaceBetween) {
    layout->setJustifyContent(JustifyContent::SpaceBetween);
    layout->setAlignItems(Align::Start);

    addWidget(100, 50);
    addWidget(100, 50);
    addWidget(100, 50);
    auto children = getAdaptors();

    Rect container(0, 0, 500, 300);
    auto results = layout->compute(container, children);

    ASSERT_EQ(results.size(), 3u);

    EXPECT_EQ(results[0].bounds.x, 0);
    EXPECT_EQ(results[1].bounds.x, 200);
    EXPECT_EQ(results[2].bounds.x, 400);
}

TEST_F(FlexLayoutDetailedTest, JustifyContentSpaceAround) {
    layout->setJustifyContent(JustifyContent::SpaceAround);
    layout->setAlignItems(Align::Start);

    addWidget(100, 50);
    addWidget(100, 50);
    auto children = getAdaptors();

    Rect container(0, 0, 400, 300);
    auto results = layout->compute(container, children);

    ASSERT_EQ(results.size(), 2u);

    EXPECT_EQ(results[0].bounds.x, 50);
    EXPECT_EQ(results[1].bounds.x, 250);
}

// ============================================================================
// 交叉轴对齐测试
// ============================================================================

TEST_F(FlexLayoutDetailedTest, AlignItemsStart) {
    layout->setAlignItems(Align::Start);

    addWidget(100, 50);
    addWidget(100, 80);
    auto children = getAdaptors();

    Rect container(0, 0, 400, 300);
    auto results = layout->compute(container, children);

    ASSERT_EQ(results.size(), 2u);

    EXPECT_EQ(results[0].bounds.y, 0);
    EXPECT_EQ(results[1].bounds.y, 0);
    EXPECT_EQ(results[0].bounds.height, 50);
    EXPECT_EQ(results[1].bounds.height, 80);
}

TEST_F(FlexLayoutDetailedTest, AlignItemsCenter) {
    layout->setAlignItems(Align::Center);

    addWidget(100, 50);
    addWidget(100, 80);
    auto children = getAdaptors();

    Rect container(0, 0, 400, 300);
    auto results = layout->compute(container, children);

    ASSERT_EQ(results.size(), 2u);

    // Flexbox: 先在行内对齐，然后整行在容器中居中
    // 行高 80（最大高度）
    // 容器高度 300，居中偏移 = (300 - 80) / 2 = 110
    // child1 高度80，在行内居中偏移 = (80-80)/2 = 0，最终 y = 0 + 110 = 110
    // child0 高度50，在行内居中偏移 = (80-50)/2 = 15，最终 y = 15 + 110 = 125
    EXPECT_EQ(results[0].bounds.y, 125);
    EXPECT_EQ(results[1].bounds.y, 110);
}

TEST_F(FlexLayoutDetailedTest, AlignItemsEnd) {
    layout->setAlignItems(Align::End);

    addWidget(100, 50);
    addWidget(100, 80);
    auto children = getAdaptors();

    Rect container(0, 0, 400, 300);
    auto results = layout->compute(container, children);

    ASSERT_EQ(results.size(), 2u);

    // Flexbox: 先在行内对齐，然后整行在容器中底部对齐
    // 行高 80（最大高度）
    // 容器高度 300，底部偏移 = 300 - 80 = 220
    // child0 高度50，在行内底部对齐偏移 = 80 - 50 = 30，最终 y = 30 + 220 = 250
    // child1 高度80，在行内底部对齐偏移 = 80 - 80 = 0，最终 y = 0 + 220 = 220
    EXPECT_EQ(results[0].bounds.y, 250);
    EXPECT_EQ(results[1].bounds.y, 220);
}

// ============================================================================
// Flex Grow 测试
// ============================================================================

TEST_F(FlexLayoutDetailedTest, FlexGrow) {
    LayoutConstraints c1;
    addWidget(100, 50, c1);

    LayoutConstraints c2;
    c2.flex.grow = 1.0f;
    addWidget(100, 50, c2);

    LayoutConstraints c3;
    addWidget(100, 50, c3);

    auto children = getAdaptors();

    Rect container(0, 0, 400, 300);
    auto results = layout->compute(container, children);

    ASSERT_EQ(results.size(), 3u);

    EXPECT_EQ(results[0].bounds.width, 100);
    EXPECT_EQ(results[2].bounds.width, 100);
    EXPECT_GE(results[1].bounds.width, 100);
}

TEST_F(FlexLayoutDetailedTest, FlexGrowMultiple) {
    LayoutConstraints c1;
    c1.flex.grow = 1.0f;
    addWidget(100, 50, c1);

    LayoutConstraints c2;
    c2.flex.grow = 2.0f;
    addWidget(100, 50, c2);

    auto children = getAdaptors();

    Rect container(0, 0, 400, 300);
    auto results = layout->compute(container, children);

    ASSERT_EQ(results.size(), 2u);

    // 总宽度应该接近 400（由于整数除法可能有 1 像素误差）
    int totalWidth = results[0].bounds.width + results[1].bounds.width;
    EXPECT_GE(totalWidth, 399);
    EXPECT_LE(totalWidth, 400);

    // 检查两个元素宽度比大约是 1:2（考虑整数舍入）
    // widget0: 100 + (200 * 1/3) ≈ 166
    // widget1: 100 + (200 * 2/3) ≈ 233
    EXPECT_GE(results[0].bounds.width, 165);
    EXPECT_LE(results[0].bounds.width, 167);
    EXPECT_GE(results[1].bounds.width, 232);
    EXPECT_LE(results[1].bounds.width, 234);
}

// ============================================================================
// 换行测试
// ============================================================================

TEST_F(FlexLayoutDetailedTest, WrapMultipleLines) {
    layout->setWrap(Wrap::Wrap);
    layout->setAlignItems(Align::Start);

    addWidget(150, 50);
    addWidget(150, 50);
    addWidget(150, 50);
    addWidget(150, 50);
    auto children = getAdaptors();

    Rect container(0, 0, 350, 300);
    auto results = layout->compute(container, children);

    ASSERT_EQ(results.size(), 4u);

    EXPECT_LT(results[0].bounds.y, results[2].bounds.y);
    EXPECT_EQ(results[0].bounds.y, results[1].bounds.y);
    EXPECT_EQ(results[2].bounds.y, results[3].bounds.y);
}

// ============================================================================
// 反向布局测试
// ============================================================================

TEST_F(FlexLayoutDetailedTest, RowReverse) {
    layout->setDirection(Direction::RowReverse);

    addWidget(100, 50);
    addWidget(100, 50);
    addWidget(100, 50);
    auto children = getAdaptors();

    Rect container(0, 0, 400, 300);
    auto results = layout->compute(container, children);

    ASSERT_EQ(results.size(), 3u);

    EXPECT_GT(results[0].bounds.x, results[1].bounds.x);
    EXPECT_GT(results[1].bounds.x, results[2].bounds.x);
}

TEST_F(FlexLayoutDetailedTest, ColumnReverse) {
    layout->setDirection(Direction::ColumnReverse);

    addWidget(100, 50);
    addWidget(100, 50);
    addWidget(100, 50);
    auto children = getAdaptors();

    Rect container(0, 0, 400, 300);
    auto results = layout->compute(container, children);

    ASSERT_EQ(results.size(), 3u);

    EXPECT_GT(results[0].bounds.y, results[1].bounds.y);
    EXPECT_GT(results[1].bounds.y, results[2].bounds.y);
}

// ============================================================================
// 内边距测试
// ============================================================================

TEST_F(FlexLayoutDetailedTest, ContainerPadding) {
    LayoutConstraints containerConstraints;
    containerConstraints.padding = Padding(20, 10);

    addWidget(100, 50);
    addWidget(100, 50);
    auto children = getAdaptors();

    Rect container(0, 0, 400, 300);
    auto results = layout->compute(container, children, containerConstraints);

    ASSERT_EQ(results.size(), 2u);

    EXPECT_GE(results[0].bounds.x, 20);
    EXPECT_GE(results[0].bounds.y, 10);
}

// ============================================================================
// 测量测试
// ============================================================================

TEST_F(FlexLayoutDetailedTest, MeasureHorizontal) {
    addWidget(100, 50);
    addWidget(100, 80);
    auto children = getAdaptors();

    Size size = layout->measure(
        MeasureSpec::MakeUnspecified(),
        MeasureSpec::MakeUnspecified(),
        children
    );

    EXPECT_EQ(size.width, 200);
    EXPECT_EQ(size.height, 80);
}

TEST_F(FlexLayoutDetailedTest, MeasureVertical) {
    layout->setDirection(Direction::Column);

    addWidget(100, 50);
    addWidget(150, 80);
    auto children = getAdaptors();

    Size size = layout->measure(
        MeasureSpec::MakeUnspecified(),
        MeasureSpec::MakeUnspecified(),
        children
    );

    EXPECT_EQ(size.width, 150);
    EXPECT_EQ(size.height, 130);
}

TEST_F(FlexLayoutDetailedTest, MeasureWithGap) {
    layout->setGap(10);

    addWidget(100, 50);
    addWidget(100, 50);
    addWidget(100, 50);
    auto children = getAdaptors();

    Size size = layout->measure(
        MeasureSpec::MakeUnspecified(),
        MeasureSpec::MakeUnspecified(),
        children
    );

    EXPECT_EQ(size.width, 320);
    EXPECT_EQ(size.height, 50);
}

TEST_F(FlexLayoutDetailedTest, MeasureWithExactlySpec) {
    addWidget(100, 50);
    addWidget(100, 50);
    auto children = getAdaptors();

    Size size = layout->measure(
        MeasureSpec::MakeExactly(500),
        MeasureSpec::MakeExactly(400),
        children
    );

    EXPECT_EQ(size.width, 500);
    EXPECT_EQ(size.height, 400);
}

// ============================================================================
// 边界情况测试
// ============================================================================

TEST_F(FlexLayoutDetailedTest, EmptyChildrenCompute) {
    std::vector<WidgetLayoutAdaptor*> children;
    Rect container(0, 0, 400, 300);

    auto results = layout->compute(container, children);
    EXPECT_TRUE(results.empty());
}

TEST_F(FlexLayoutDetailedTest, EmptyChildrenMeasure) {
    std::vector<WidgetLayoutAdaptor*> children;

    Size size = layout->measure(
        MeasureSpec::MakeUnspecified(),
        MeasureSpec::MakeUnspecified(),
        children
    );

    EXPECT_EQ(size.width, 0);
    EXPECT_EQ(size.height, 0);
}

TEST_F(FlexLayoutDetailedTest, EmptyChildrenMeasureExactly) {
    std::vector<WidgetLayoutAdaptor*> children;

    Size size = layout->measure(
        MeasureSpec::MakeExactly(400),
        MeasureSpec::MakeExactly(300),
        children
    );

    // Exactly规格应该返回规格指定的尺寸
    EXPECT_EQ(size.width, 400);
    EXPECT_EQ(size.height, 300);
}

TEST_F(FlexLayoutDetailedTest, DisabledChild) {
    LayoutConstraints c1;
    addWidget(100, 50, c1);

    LayoutConstraints c2;
    c2.enabled = false;
    addWidget(100, 50, c2);

    auto children = getAdaptors();

    Rect container(0, 0, 400, 300);
    auto results = layout->compute(container, children);

    ASSERT_EQ(results.size(), 2u);
}

TEST_F(FlexLayoutDetailedTest, ZeroSizeContainer) {
    addWidget(100, 50);
    auto children = getAdaptors();

    Rect container(0, 0, 0, 0);
    auto results = layout->compute(container, children);

    ASSERT_EQ(results.size(), 1u);
}

TEST_F(FlexLayoutDetailedTest, LargeGap) {
    layout->setGap(1000);

    addWidget(100, 50);
    addWidget(100, 50);
    auto children = getAdaptors();

    Rect container(0, 0, 400, 300);
    auto results = layout->compute(container, children);

    ASSERT_EQ(results.size(), 2u);
}
