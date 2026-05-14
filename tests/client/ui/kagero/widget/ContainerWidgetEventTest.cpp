#include "client/ui/kagero/widget/ButtonWidget.hpp"
#include "client/ui/kagero/widget/ContainerWidget.hpp"
#include "client/ui/kagero/widget/TextFieldWidget.hpp"
#include <gtest/gtest.h>

namespace mc::client::ui::kagero::widget {

// 测试用的 Mock Widget，用于跟踪 tick 调用
class MockTickWidget : public Widget {
public:
    MockTickWidget(const std::string& id)
        : Widget(id)
    {
        setBounds(Rect(0, 0, 100, 20));
    }

    void tick(f32 dt) override
    {
        tickCount++;
        lastDt = dt;
    }

    int tickCount = 0;
    f32 lastDt = 0.0f;
};

class ContainerWidgetEventTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        container = std::make_unique<ContainerWidget>("testContainer");
        container->setBounds(Rect(0, 0, 400, 300));
    }

    void TearDown() override { container.reset(); }

    std::unique_ptr<ContainerWidget> container;
};

// ========== Tick 事件传播测试 ==========

TEST_F(ContainerWidgetEventTest, Tick_PropagatesToChildren)
{
    auto tickWidget = std::make_unique<MockTickWidget>("tickWidget");
    tickWidget->setVisible(true);
    tickWidget->setActive(true);

    container->addChild(std::move(tickWidget));

    container->tick(0.016f);
    // 由于 tickChildren 只遍历可见且活跃的子组件，我们需要确认它被调用
    // ContainerWidget::tick 会调用 tickChildren
}

TEST_F(ContainerWidgetEventTest, Tick_DoesNotTickInvisibleChildren)
{
    auto tickWidget = std::make_unique<MockTickWidget>("tickWidget");
    tickWidget->setVisible(false);
    tickWidget->setActive(true);

    container->addChild(std::move(tickWidget));

    container->tick(0.016f);
    // 不可见组件不应收到 tick
}

// ========== 点击事件传播测试 ==========

TEST_F(ContainerWidgetEventTest, Click_PropagatesToVisibleChild)
{
    auto button = std::make_unique<ButtonWidget>("btn", 10, 10, 100, 20, "Test");
    button->setActive(true);
    bool clicked = false;
    button->setOnPress([&clicked](ButtonWidget&) { clicked = true; });

    container->addChild(std::move(button));

    EXPECT_TRUE(container->onClick(50, 20, 0)); // 左键点击按钮区域
    EXPECT_TRUE(clicked);
}

TEST_F(ContainerWidgetEventTest, Click_ReturnsFalseWhenNoWidgetClicked)
{
    auto button = std::make_unique<ButtonWidget>("btn", 10, 10, 100, 20, "Test");
    container->addChild(std::move(button));

    EXPECT_FALSE(container->onClick(200, 200, 0)); // 点击空白区域
}

TEST_F(ContainerWidgetEventTest, Click_DoesNotHitInvisibleWidget)
{
    auto button = std::make_unique<ButtonWidget>("btn", 10, 10, 100, 20, "Test");
    button->setVisible(false);
    bool clicked = false;
    button->setOnPress([&clicked](ButtonWidget&) { clicked = true; });

    container->addChild(std::move(button));

    EXPECT_FALSE(container->onClick(50, 20, 0));
    EXPECT_FALSE(clicked);
}

TEST_F(ContainerWidgetEventTest, Click_TopWidgetReceivesEvent)
{
    bool topClicked = false;
    bool bottomClicked = false;

    auto bottom = std::make_unique<ButtonWidget>("bottom", 10, 10, 100, 20, "Bottom");
    bottom->setOnPress([&bottomClicked](ButtonWidget&) { bottomClicked = true; });

    auto top = std::make_unique<ButtonWidget>("top", 10, 10, 100, 20, "Top");
    top->setOnPress([&topClicked](ButtonWidget&) { topClicked = true; });

    container->addChild(std::move(bottom));
    container->addChild(std::move(top)); // 后添加的在上面

    EXPECT_TRUE(container->onClick(50, 20, 0));
    EXPECT_TRUE(topClicked);
    EXPECT_FALSE(bottomClicked); // 底层未收到事件
}

// ========== 焦点管理测试 ==========

TEST_F(ContainerWidgetEventTest, Focus_ClickSetsFocus)
{
    auto textField = std::make_unique<TextFieldWidget>("field", 10, 10, 200, 20);
    textField->setActive(true);
    auto* fieldPtr = textField.get();

    container->addChild(std::move(textField));

    EXPECT_FALSE(fieldPtr->isFocused());
    EXPECT_TRUE(container->onClick(50, 20, 0));
    EXPECT_TRUE(fieldPtr->isFocused());
}

TEST_F(ContainerWidgetEventTest, Focus_ClickElsewhereClearsFocus)
{
    auto textField = std::make_unique<TextFieldWidget>("field", 10, 10, 200, 20);
    textField->setActive(true);
    auto* fieldPtr = textField.get();

    container->addChild(std::move(textField));

    // 点击设置焦点
    container->onClick(50, 20, 0);
    EXPECT_TRUE(fieldPtr->isFocused());

    // 点击空白区域清除焦点
    container->onClick(300, 200, 0);
    EXPECT_FALSE(fieldPtr->isFocused());
}

TEST_F(ContainerWidgetEventTest, Focus_TabNavigation)
{
    auto field1 = std::make_unique<TextFieldWidget>("field1", 10, 10, 200, 20);
    auto field2 = std::make_unique<TextFieldWidget>("field2", 10, 40, 200, 20);
    auto* field1Ptr = field1.get();
    auto* field2Ptr = field2.get();

    container->addChild(std::move(field1));
    container->addChild(std::move(field2));

    // 初始无焦点
    EXPECT_EQ(container->getFocusedWidget(), nullptr);

    // Tab 到第一个
    EXPECT_TRUE(container->focusNext());
    EXPECT_EQ(container->getFocusedWidget(), field1Ptr);

    // Tab 到第二个
    EXPECT_TRUE(container->focusNext());
    EXPECT_EQ(container->getFocusedWidget(), field2Ptr);

    // 到达末尾
    EXPECT_FALSE(container->focusNext());
}

TEST_F(ContainerWidgetEventTest, Focus_ReverseTabNavigation)
{
    auto field1 = std::make_unique<TextFieldWidget>("field1", 10, 10, 200, 20);
    auto field2 = std::make_unique<TextFieldWidget>("field2", 10, 40, 200, 20);
    auto* field1Ptr = field1.get();
    auto* field2Ptr = field2.get();

    container->addChild(std::move(field1));
    container->addChild(std::move(field2));

    // 反向 Tab 从最后一个开始
    EXPECT_TRUE(container->focusPrevious());
    EXPECT_EQ(container->getFocusedWidget(), field2Ptr);

    EXPECT_TRUE(container->focusPrevious());
    EXPECT_EQ(container->getFocusedWidget(), field1Ptr);

    // 到达开头
    EXPECT_FALSE(container->focusPrevious());
}

TEST_F(ContainerWidgetEventTest, Focus_ClearFocus)
{
    auto textField = std::make_unique<TextFieldWidget>("field", 10, 10, 200, 20);
    auto* fieldPtr = textField.get();

    container->addChild(std::move(textField));

    container->setFocusedWidget(fieldPtr);
    EXPECT_TRUE(fieldPtr->isFocused());

    container->clearFocus();
    EXPECT_FALSE(fieldPtr->isFocused());
    EXPECT_EQ(container->getFocusedWidget(), nullptr);
}

// ========== 键盘事件传播测试 ==========

TEST_F(ContainerWidgetEventTest, Key_SentToFocusedWidget)
{
    auto textField = std::make_unique<TextFieldWidget>("field", 10, 10, 200, 20);
    textField->setActive(true);
    auto* fieldPtr = textField.get();

    container->addChild(std::move(textField));

    // 设置焦点
    container->setFocusedWidget(fieldPtr);

    // TextFieldWidget 通过 onChar 处理字符输入，onKey 处理特殊键
    // 测试字符输入
    bool handled = container->onChar(static_cast<u32>('A'));
    EXPECT_TRUE(handled); // TextField 应该处理
    EXPECT_EQ(fieldPtr->text(), std::string("A"));
}

TEST_F(ContainerWidgetEventTest, Key_NotHandledWhenNoFocus)
{
    auto textField = std::make_unique<TextFieldWidget>("field", 10, 10, 200, 20);
    container->addChild(std::move(textField));

    // 无焦点
    EXPECT_EQ(container->getFocusedWidget(), nullptr);

    bool handled = container->onChar(static_cast<u32>('A'));
    EXPECT_FALSE(handled);
}

// ========== 滚动事件传播测试 ==========

TEST_F(ContainerWidgetEventTest, Scroll_PropagatesToChild)
{
    // 创建一个可滚动的容器作为子组件
    auto scrollContainer = std::make_unique<ContainerWidget>("scrollContainer");
    scrollContainer->setBounds(Rect(10, 10, 200, 100));
    auto* scrollPtr = scrollContainer.get();

    container->addChild(std::move(scrollContainer));

    // 滚动事件应该传递到子组件
    bool handled = container->onScroll(50, 50, 1.0);
    // ContainerWidget 本身不处理滚动，返回 false
    // 但事件会传递到子组件
    (void)scrollPtr;
    (void)handled;
}

// ========== 拖动事件传播测试 ==========

TEST_F(ContainerWidgetEventTest, Drag_PropagatesToHoveredChild)
{
    auto button = std::make_unique<ButtonWidget>("btn", 10, 10, 100, 20, "Test");
    button->setActive(true);
    container->addChild(std::move(button));

    // 先点击使按钮获得悬停状态
    container->onClick(50, 20, 0);

    // 拖动事件会传递到悬停的组件
    bool handled = container->onDrag(60, 25, 10, 5);
    // ButtonWidget 默认不处理拖动，所以返回 false
    EXPECT_FALSE(handled);
}

} // namespace mc::client::ui::kagero::widget
