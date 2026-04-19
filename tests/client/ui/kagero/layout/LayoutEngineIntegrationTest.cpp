/**
 * @file LayoutEngineIntegrationTest.cpp
 * @brief Kagero 布局适配器与布局引擎的集成测试
 */

#include <gtest/gtest.h>

#include "client/ui/kagero/layout/core/LayoutEngine.hpp"
#include "client/ui/kagero/layout/integration/WidgetLayoutAdaptor.hpp"
#include "client/ui/kagero/widget/ContainerWidget.hpp"

#include <memory>

namespace mc::client::ui::kagero::layout {

using mc::i32;

namespace {

/**
 * @brief 创建指定尺寸的测试 Widget
 *
 * 该辅助函数只用于测试，直接设置 Widget 的初始尺寸，方便布局计算。
 */
std::unique_ptr<widget::Widget> makeWidget(const char* id, i32 width, i32 height) {
    auto widget = std::make_unique<widget::Widget>(id);
    widget->setSize(width, height);
    return widget;
}

/**
 * @brief 向测试容器追加两个子 Widget
 *
 * 容器实例由调用方持有，避免依赖容器类型的移动语义。
 */
void addTwoWidgets(
    widget::ContainerWidget& container,
    const char* firstId,
    i32 firstWidth,
    i32 firstHeight,
    const char* secondId,
    i32 secondWidth,
    i32 secondHeight
) {
    container.addWidget(makeWidget(firstId, firstWidth, firstHeight));
    container.addWidget(makeWidget(secondId, secondWidth, secondHeight));
}

} // namespace

TEST(LayoutAdaptorIntegrationTest, GetChildren_EnumeratesContainerWidgets) {
    widget::ContainerWidget container("root");
    addTwoWidgets(container, "child_a", 24, 12, "child_b", 18, 30);
    WidgetLayoutAdaptor adaptor(&container);

    const auto children = adaptor.getChildren();

    ASSERT_EQ(children.size(), 2u);
    EXPECT_EQ(children[0]->id(), "child_a");
    EXPECT_EQ(children[1]->id(), "child_b");
    EXPECT_EQ(children[0]->currentSize().width, 24);
    EXPECT_EQ(children[0]->currentSize().height, 12);
    EXPECT_EQ(children[1]->currentSize().width, 18);
    EXPECT_EQ(children[1]->currentSize().height, 30);
    EXPECT_EQ(children[0]->depth(), 1);
    EXPECT_EQ(children[1]->depth(), 1);
}

TEST(LayoutEngineIntegrationTest, LayoutFlex_AppliesHorizontalSpacing) {
    widget::ContainerWidget container("root");
    addTwoWidgets(container, "left", 30, 20, "right", 40, 10);
    WidgetLayoutAdaptor adaptor(&container);

    FlexConfig config;
    config.direction = Direction::Row;
    config.gap = 5;
    config.justifyContent = JustifyContent::Start;
    config.alignItems = Align::Start;

    LayoutEngine::instance().layoutFlex(&adaptor, Rect(0, 0, 120, 40), config);

    const auto& children = container.widgets();
    ASSERT_EQ(children.size(), 2u);
    EXPECT_EQ(container.bounds().x, 0);
    EXPECT_EQ(container.bounds().y, 0);
    EXPECT_EQ(container.bounds().width, 120);
    EXPECT_EQ(container.bounds().height, 40);
    EXPECT_EQ(children[0]->bounds().x, 0);
    EXPECT_EQ(children[0]->bounds().y, 0);
    EXPECT_EQ(children[0]->bounds().width, 30);
    EXPECT_EQ(children[0]->bounds().height, 20);
    EXPECT_EQ(children[1]->bounds().x, 35);
    EXPECT_EQ(children[1]->bounds().y, 0);
    EXPECT_EQ(children[1]->bounds().width, 40);
    EXPECT_EQ(children[1]->bounds().height, 10);
}

TEST(LayoutEngineIntegrationTest, LayoutWithGrid_UsesGridAlgorithm) {
    widget::ContainerWidget container("root");
    addTwoWidgets(container, "top", 20, 10, "bottom", 15, 12);
    WidgetLayoutAdaptor adaptor(&container);

    LayoutEngine::instance().layoutWith("grid", &adaptor, Rect(0, 0, 80, 60));

    const auto& children = container.widgets();
    ASSERT_EQ(children.size(), 2u);
    EXPECT_EQ(children[0]->bounds().x, 0);
    EXPECT_EQ(children[0]->bounds().y, 0);
    EXPECT_EQ(children[0]->bounds().width, 80);
    EXPECT_EQ(children[0]->bounds().height, 30);
    EXPECT_EQ(children[1]->bounds().x, 0);
    EXPECT_EQ(children[1]->bounds().y, 30);
    EXPECT_EQ(children[1]->bounds().width, 80);
    EXPECT_EQ(children[1]->bounds().height, 30);
}

TEST(LayoutEngineIntegrationTest, LayoutWithStack_UsesColumnLayout) {
    widget::ContainerWidget container("root");
    addTwoWidgets(container, "first", 20, 10, "second", 15, 12);
    WidgetLayoutAdaptor adaptor(&container);

    LayoutEngine::instance().layoutWith("stack", &adaptor, Rect(0, 0, 80, 60));

    const auto& children = container.widgets();
    ASSERT_EQ(children.size(), 2u);
    EXPECT_EQ(children[0]->bounds().x, 0);
    EXPECT_EQ(children[0]->bounds().y, 0);
    EXPECT_EQ(children[0]->bounds().width, 80);
    EXPECT_EQ(children[1]->bounds().x, 0);
    EXPECT_EQ(children[1]->bounds().y, 10);
}

} // namespace mc::client::ui::kagero::layout
