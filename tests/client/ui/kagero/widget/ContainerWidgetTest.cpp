/**
 * @file ContainerWidgetTest.cpp
 * @brief ContainerWidget单元测试
 */

#include <gtest/gtest.h>
#include "client/ui/kagero/widget/ContainerWidget.hpp"
#include "client/ui/kagero/widget/ButtonWidget.hpp"
#include "client/ui/kagero/widget/TextWidget.hpp"
#include "client/ui/kagero/Types.hpp"
#include "client/ui/Glyph.hpp"

using namespace mc::client::ui::kagero;
using namespace mc::client::ui::kagero::widget;
using namespace mc;

// ==================== 构造函数测试 ====================

TEST(ContainerWidgetTest, DefaultConstructor) {
    ContainerWidget container;
    EXPECT_TRUE(container.id().empty());
    EXPECT_EQ(0u, container.childCount());
    EXPECT_EQ(0u, container.widgetCount());
}

TEST(ContainerWidgetTest, ConstructorWithId) {
    ContainerWidget container("main");
    EXPECT_EQ("main", container.id());
}

// ==================== 子组件管理测试 ====================

TEST(ContainerWidgetTest, AddChild) {
    ContainerWidget container("main");

    auto child = std::make_unique<TextWidget>("child1", 0, 0, 100, 20);
    container.addChild(std::move(child));

    EXPECT_EQ(1u, container.childCount());
    EXPECT_EQ(1u, container.widgetCount());
}

TEST(ContainerWidgetTest, AddWidget) {
    ContainerWidget container("main");

    auto child = std::make_unique<TextWidget>("child1", 0, 0, 100, 20);
    container.addWidget(std::move(child));

    EXPECT_EQ(1u, container.childCount());
    EXPECT_EQ(1u, container.widgetCount());
}

TEST(ContainerWidgetTest, FindChild) {
    ContainerWidget container("main");

    container.addChild(std::make_unique<TextWidget>("child1", 0, 0, 100, 20));
    container.addChild(std::make_unique<TextWidget>("child2", 0, 30, 100, 20));

    Widget* found = container.findChild("child1");
    ASSERT_NE(found, nullptr);
    EXPECT_EQ("child1", found->id());

    found = container.findChild("child2");
    ASSERT_NE(found, nullptr);
    EXPECT_EQ("child2", found->id());

    found = container.findChild("nonexistent");
    EXPECT_EQ(found, nullptr);
}

TEST(ContainerWidgetTest, FindWidgetById) {
    ContainerWidget container("main");

    container.addWidget(std::make_unique<TextWidget>("child1", 0, 0, 100, 20));

    Widget* found = container.findWidgetById("child1");
    ASSERT_NE(found, nullptr);
    EXPECT_EQ("child1", found->id());
}

TEST(ContainerWidgetTest, RemoveChild) {
    ContainerWidget container("main");

    container.addChild(std::make_unique<TextWidget>("child1", 0, 0, 100, 20));
    container.addChild(std::make_unique<TextWidget>("child2", 0, 30, 100, 20));

    EXPECT_EQ(2u, container.childCount());

    bool removed = container.removeChild("child1");
    EXPECT_TRUE(removed);
    EXPECT_EQ(1u, container.childCount());

    removed = container.removeChild("nonexistent");
    EXPECT_FALSE(removed);
    EXPECT_EQ(1u, container.childCount());
}

TEST(ContainerWidgetTest, ClearChildren) {
    ContainerWidget container("main");

    container.addChild(std::make_unique<TextWidget>("child1", 0, 0, 100, 20));
    container.addChild(std::make_unique<TextWidget>("child2", 0, 30, 100, 20));

    EXPECT_EQ(2u, container.childCount());

    container.clearChildren();
    EXPECT_EQ(0u, container.childCount());
}

TEST(ContainerWidgetTest, ClearWidgets) {
    ContainerWidget container("main");

    container.addWidget(std::make_unique<TextWidget>("child1", 0, 0, 100, 20));
    container.addWidget(std::make_unique<TextWidget>("child2", 0, 30, 100, 20));

    EXPECT_EQ(2u, container.widgetCount());

    container.clearWidgets();
    EXPECT_EQ(0u, container.widgetCount());
}

// ==================== 父容器测试 ====================

TEST(ContainerWidgetTest, ChildParent) {
    ContainerWidget container("main");

    auto child = std::make_unique<TextWidget>("child1", 0, 0, 100, 20);
    TextWidget* childPtr = child.get();
    container.addChild(std::move(child));

    EXPECT_EQ(&container, childPtr->parent());
}

// ==================== 位置检测测试 ====================

TEST(ContainerWidgetTest, GetWidgetAt) {
    ContainerWidget container("main");
    container.setBounds(Rect(0, 0, 300, 300));

    container.addChild(std::make_unique<TextWidget>("child1", 0, 0, 100, 100));
    container.addChild(std::make_unique<TextWidget>("child2", 100, 0, 100, 100));
    container.addChild(std::make_unique<TextWidget>("child3", 200, 0, 100, 100));

    Widget* found = container.getWidgetAt(50, 50);
    ASSERT_NE(found, nullptr);
    EXPECT_EQ("child1", found->id());

    found = container.getWidgetAt(150, 50);
    ASSERT_NE(found, nullptr);
    EXPECT_EQ("child2", found->id());

    found = container.getWidgetAt(250, 50);
    ASSERT_NE(found, nullptr);
    EXPECT_EQ("child3", found->id());

    found = container.getWidgetAt(400, 50);
    EXPECT_EQ(found, nullptr);
}

TEST(ContainerWidgetTest, GetWidgetAtInvisible) {
    ContainerWidget container("main");
    container.setBounds(Rect(0, 0, 300, 300));

    auto child1 = std::make_unique<TextWidget>("child1", 0, 0, 100, 100);
    child1->setVisible(false);
    container.addChild(std::move(child1));

    // 不可见组件不应被检测到
    Widget* found = container.getWidgetAt(50, 50);
    EXPECT_EQ(found, nullptr);
}

TEST(ContainerWidgetTest, GetWidgetAtInactive) {
    ContainerWidget container("main");
    container.setBounds(Rect(0, 0, 300, 300));

    auto child1 = std::make_unique<TextWidget>("child1", 0, 0, 100, 100);
    child1->setActive(false);
    container.addChild(std::move(child1));

    // 不激活组件不应被检测到
    Widget* found = container.getWidgetAt(50, 50);
    EXPECT_EQ(found, nullptr);
}

// ==================== 层级测试 ====================

TEST(ContainerWidgetTest, BringToFront) {
    ContainerWidget container("main");

    container.addChild(std::make_unique<TextWidget>("child1", 0, 0, 100, 100));
    container.addChild(std::make_unique<TextWidget>("child2", 0, 0, 100, 100));
    container.addChild(std::make_unique<TextWidget>("child3", 0, 0, 100, 100));

    // 初始顺序: child1, child2, child3
    // getWidgetAt从后往前遍历，所以应返回child3
    Widget* found = container.getWidgetAt(50, 50);
    ASSERT_NE(found, nullptr);
    EXPECT_EQ("child3", found->id());

    // 将child1移到前面
    Widget* child1 = container.findChild("child1");
    container.bringToFront(child1);

    // 现在child1应该在最上层
    found = container.getWidgetAt(50, 50);
    ASSERT_NE(found, nullptr);
    EXPECT_EQ("child1", found->id());
}

TEST(ContainerWidgetTest, SendToBack) {
    ContainerWidget container("main");

    container.addChild(std::make_unique<TextWidget>("child1", 0, 0, 100, 100));
    container.addChild(std::make_unique<TextWidget>("child2", 0, 0, 100, 100));
    container.addChild(std::make_unique<TextWidget>("child3", 0, 0, 100, 100));

    // 初始顺序: child1, child2, child3
    Widget* found = container.getWidgetAt(50, 50);
    EXPECT_EQ("child3", found->id());

    // 将child3移到最后面
    Widget* child3 = container.findChild("child3");
    container.sendToBack(child3);

    // 现在child2应该在最上层
    found = container.getWidgetAt(50, 50);
    EXPECT_EQ("child2", found->id());
}

// ==================== 遍历测试 ====================

TEST(ContainerWidgetTest, ForEachWidget) {
    ContainerWidget container("main");

    container.addChild(std::make_unique<TextWidget>("child1", 0, 0, 100, 100));
    container.addChild(std::make_unique<TextWidget>("child2", 0, 0, 100, 100));
    container.addChild(std::make_unique<TextWidget>("child3", 0, 0, 100, 100));

    std::vector<std::string> ids;
    container.forEachWidget([&ids](Widget& widget) {
        ids.push_back(widget.id());
    });

    EXPECT_EQ(3u, ids.size());
    EXPECT_EQ("child1", ids[0]);
    EXPECT_EQ("child2", ids[1]);
    EXPECT_EQ("child3", ids[2]);
}

TEST(ContainerWidgetTest, ForEachWidgetConst) {
    ContainerWidget container("main");

    container.addChild(std::make_unique<TextWidget>("child1", 0, 0, 100, 100));
    container.addChild(std::make_unique<TextWidget>("child2", 0, 0, 100, 100));

    const ContainerWidget& constContainer = container;
    int count = 0;
    constContainer.forEachWidget([&count](const Widget& widget) {
        (void)widget;
        ++count;
    });

    EXPECT_EQ(2, count);
}

// ==================== 移除子组件指针测试 ====================

TEST(ContainerWidgetTest, RemoveWidgetPointer) {
    ContainerWidget container("main");

    auto child = std::make_unique<TextWidget>("child1", 0, 0, 100, 100);
    Widget* childPtr = child.get();
    container.addChild(std::move(child));

    EXPECT_EQ(1u, container.childCount());

    container.removeWidget(childPtr);
    EXPECT_EQ(0u, container.childCount());
}
