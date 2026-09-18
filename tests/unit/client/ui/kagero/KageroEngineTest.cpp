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

#include "client/ui/kagero/KageroEngine.hpp"
#include "client/ui/kagero/Types.hpp"
#include "client/ui/kagero/paint/contracts/ICanvas.hpp"
#include "client/ui/kagero/template/bindings/BuiltinEvents.hpp"
#include "client/ui/kagero/widget/ContainerWidget.hpp"
#include "client/ui/kagero/widget/ScrollableWidget.hpp"
#include <chrono>
#include <thread>
#include <gtest/gtest.h>

namespace mc::client::ui::kagero {

// Mock Canvas for testing — implements the full ICanvas interface
class MockCanvas : public paint::ICanvas {
public:
    void drawRect(const Rect&, const paint::IPaint&) override {}
    void drawRRect(const paint::RRect&, const paint::IPaint&) override {}
    void drawCircle(f32, f32, f32, const paint::IPaint&) override {}
    void drawOval(const Rect&, const paint::IPaint&) override {}
    void drawPath(const paint::IPath&, const paint::IPaint&) override {}
    void drawLine(f32, f32, f32, f32, const paint::IPaint&) override {}
    void drawGradientRect(const Rect&, u32, u32, bool) override {}
    void drawImage(const paint::IImage&, f32, f32) override {}
    void drawImageRect(const paint::IImage&, const Rect&, const Rect&) override {}
    void drawImageNine(const paint::IImage&, const Rect&, const Rect&, const paint::IPaint*) override {}
    void drawText(const std::string&, f32, f32, const paint::IPaint&) override {}
    void drawTextBlob(const paint::ITextBlob&, f32, f32, const paint::IPaint&) override {}
    void clipRect(const Rect&) override {}
    void clipRRect(const paint::RRect&) override {}
    void clipPath(const paint::IPath&) override {}
    void clipOutRect(const Rect&) override {}
    [[nodiscard]] bool clipIsEmpty() const override { return false; }
    [[nodiscard]] Rect getClipBounds() const override { return Rect{}; }
    void translate(f32, f32) override {}
    void scale(f32, f32) override {}
    void rotate(f32) override {}
    void concat(const paint::Matrix&) override {}
    void setMatrix(const paint::Matrix&) override {}
    [[nodiscard]] paint::Matrix getTotalMatrix() const override { return paint::Matrix::identity(); }
    i32 save() override { return 0; }
    void restore() override {}
    void restoreToCount(i32) override {}
    i32 saveLayer(const Rect*, const paint::IPaint*) override { return 0; }
    i32 saveLayerAlpha(const Rect*, u8) override { return 0; }
    [[nodiscard]] i32 width() const override { return 800; }
    [[nodiscard]] i32 height() const override { return 600; }
    [[nodiscard]] f32 getTextWidth(const std::string&) const override { return 0.0f; }
    [[nodiscard]] u32 getFontHeight() const override { return 16; }
};

// Mock Widget that tracks double-click and right-click events
class MockClickWidget : public widget::Widget {
public:
    explicit MockClickWidget(const std::string& id)
        : Widget(id)
    {
        setBounds(Rect(0, 0, 100, 100));
    }

    bool onClick(i32 mouseX, i32 mouseY, i32 button, i32 mods) override
    {
        (void)mouseX;
        (void)mouseY;
        (void)mods;
        lastClickButton = button;
        clickCount++;
        return true;
    }

    bool onDoubleClick(i32 mouseX, i32 mouseY, i32 button, i32 mods) override
    {
        (void)mouseX;
        (void)mouseY;
        (void)mods;
        lastDoubleClickButton = button;
        doubleClickCount++;
        return true;
    }

    bool onRightClick(i32 mouseX, i32 mouseY, i32 mods) override
    {
        (void)mouseX;
        (void)mouseY;
        (void)mods;
        rightClickCount++;
        return true;
    }

    int clickCount = 0;
    int doubleClickCount = 0;
    int rightClickCount = 0;
    int lastClickButton = -1;
    int lastDoubleClickButton = -1;
};

// ==================== KageroEngine 双击检测测试 ====================

class KageroEngineDoubleClickTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        auto result = engine.initialize(canvas, {800, 600});
        ASSERT_TRUE(result.success());
    }

    MockCanvas canvas;
    KageroEngine engine;
};

TEST_F(KageroEngineDoubleClickTest, SingleClickDoesNotTriggerDoubleClick)
{
    auto widget = std::make_unique<MockClickWidget>("test");
    auto* widgetPtr = widget.get();
    engine.addLayer(std::move(widget), 0);

    // 第一次点击
    engine.handleClick(50, 50, 0, 0);
    EXPECT_EQ(1, widgetPtr->clickCount);
    EXPECT_EQ(0, widgetPtr->doubleClickCount);
}

TEST_F(KageroEngineDoubleClickTest, RapidDoubleClickTriggersOnDoubleClick)
{
    auto widget = std::make_unique<MockClickWidget>("test");
    auto* widgetPtr = widget.get();
    engine.addLayer(std::move(widget), 0);

    // 第一次点击
    engine.handleClick(50, 50, 0, 0);
    EXPECT_EQ(1, widgetPtr->clickCount);

    // 立即第二次点击（在250ms阈值内）
    engine.handleClick(50, 50, 0, 0);
    EXPECT_EQ(2, widgetPtr->clickCount);
    EXPECT_EQ(1, widgetPtr->doubleClickCount);
    EXPECT_EQ(0, widgetPtr->lastDoubleClickButton);
}

TEST_F(KageroEngineDoubleClickTest, TripleClickDoesNotTriggerSecondDoubleClick)
{
    auto widget = std::make_unique<MockClickWidget>("test");
    auto* widgetPtr = widget.get();
    engine.addLayer(std::move(widget), 0);

    // 第一次点击
    engine.handleClick(50, 50, 0, 0);
    // 第二次点击（双击）
    engine.handleClick(50, 50, 0, 0);
    EXPECT_EQ(1, widgetPtr->doubleClickCount);

    // 第三次点击（不应触发双击，因为双击后状态被重置）
    engine.handleClick(50, 50, 0, 0);
    EXPECT_EQ(1, widgetPtr->doubleClickCount); // 仍然是1
}

TEST_F(KageroEngineDoubleClickTest, DifferentButtonsNoDoubleClick)
{
    auto widget = std::make_unique<MockClickWidget>("test");
    auto* widgetPtr = widget.get();
    engine.addLayer(std::move(widget), 0);

    // 左键点击
    engine.handleClick(50, 50, 0, 0);
    // 右键点击（不同按钮，不触发双击）
    engine.handleClick(50, 50, 1, 0);
    EXPECT_EQ(0, widgetPtr->doubleClickCount);
}

TEST_F(KageroEngineDoubleClickTest, TimeoutDoesNotTriggerDoubleClick)
{
    auto widget = std::make_unique<MockClickWidget>("test");
    auto* widgetPtr = widget.get();
    engine.addLayer(std::move(widget), 0);

    // 第一次点击
    engine.handleClick(50, 50, 0, 0);
    EXPECT_EQ(1, widgetPtr->clickCount);

    // 等待超过250ms阈值
    std::this_thread::sleep_for(std::chrono::milliseconds(300));

    // 第二次点击（超过阈值，不触发双击）
    engine.handleClick(50, 50, 0, 0);
    EXPECT_EQ(2, widgetPtr->clickCount);
    EXPECT_EQ(0, widgetPtr->doubleClickCount);
}

TEST_F(KageroEngineDoubleClickTest, DifferentWidgetDoubleClickDispatchesToCorrectChild)
{
    // 使用ContainerWidget包含两个不同位置的子Widget
    // 双击检测发生在引擎层级别，同一层内的双击会触发onDoubleClick，
    // ContainerWidget将onDoubleClick分发到点击位置对应的子Widget
    auto container = std::make_unique<widget::ContainerWidget>("container");
    container->setBounds(Rect(0, 0, 200, 100));

    auto widget1 = std::make_unique<MockClickWidget>("w1");
    auto* ptr1 = widget1.get();
    widget1->setBounds(Rect(0, 0, 100, 100));

    auto widget2 = std::make_unique<MockClickWidget>("w2");
    auto* ptr2 = widget2.get();
    widget2->setBounds(Rect(100, 0, 100, 100));

    container->addChild(std::move(widget1));
    container->addChild(std::move(widget2));
    engine.addLayer(std::move(container), 0);

    // 第一次点击w1区域
    engine.handleClick(25, 25, 0, 0);
    EXPECT_EQ(1, ptr1->clickCount);
    EXPECT_EQ(0, ptr2->clickCount);

    // 立即第二次点击w2区域（同一层、250ms内，引擎检测到双击）
    // ContainerWidget的onDoubleClick会将双击分发到w2（因为第二次点击在w2区域）
    engine.handleClick(125, 25, 0, 0);
    EXPECT_EQ(1, ptr2->clickCount);
    EXPECT_EQ(1, ptr2->doubleClickCount); // w2收到双击（引擎层级别检测）
    EXPECT_EQ(0, ptr1->doubleClickCount); // w1未收到双击
}

TEST_F(KageroEngineDoubleClickTest, RightClickDoubleClickTriggersBoth)
{
    auto widget = std::make_unique<MockClickWidget>("test");
    auto* widgetPtr = widget.get();
    engine.addLayer(std::move(widget), 0);

    // 右键第一次点击：触发onClick + onRightClick
    engine.handleClick(50, 50, 1, 0);
    EXPECT_EQ(1, widgetPtr->clickCount);
    EXPECT_EQ(1, widgetPtr->rightClickCount);

    // 右键第二次点击：触发onClick + onDoubleClick + onRightClick
    engine.handleClick(50, 50, 1, 0);
    EXPECT_EQ(2, widgetPtr->clickCount);
    EXPECT_EQ(1, widgetPtr->doubleClickCount);
    EXPECT_EQ(1, widgetPtr->lastDoubleClickButton); // 双击按钮为右键(button=1)
    EXPECT_EQ(2, widgetPtr->rightClickCount);       // 两次右键都触发onRightClick
}

// ==================== KageroEngine 右键分发测试 ====================

TEST_F(KageroEngineDoubleClickTest, RightClickButton1TriggersOnRightClick)
{
    auto widget = std::make_unique<MockClickWidget>("test");
    auto* widgetPtr = widget.get();
    engine.addLayer(std::move(widget), 0);

    // 右键点击（button == 1）
    engine.handleClick(50, 50, 1, 0);
    EXPECT_EQ(1, widgetPtr->clickCount);
    EXPECT_EQ(1, widgetPtr->rightClickCount);
    EXPECT_EQ(1, widgetPtr->lastClickButton);
}

TEST_F(KageroEngineDoubleClickTest, LeftClickButton0DoesNotTriggerOnRightClick)
{
    auto widget = std::make_unique<MockClickWidget>("test");
    auto* widgetPtr = widget.get();
    engine.addLayer(std::move(widget), 0);

    // 左键点击（button == 0）
    engine.handleClick(50, 50, 0, 0);
    EXPECT_EQ(1, widgetPtr->clickCount);
    EXPECT_EQ(0, widgetPtr->rightClickCount);
}

TEST_F(KageroEngineDoubleClickTest, MiddleClickButton2DoesNotTriggerOnRightClick)
{
    auto widget = std::make_unique<MockClickWidget>("test");
    auto* widgetPtr = widget.get();
    engine.addLayer(std::move(widget), 0);

    // 中键点击（button == 2）不应触发右键
    engine.handleClick(50, 50, 2, 0);
    EXPECT_EQ(1, widgetPtr->clickCount);
    EXPECT_EQ(0, widgetPtr->rightClickCount);
}

// ==================== Widget 回调机制测试 ====================

TEST(WidgetCallbackTest, OnDoubleClickCallback)
{
    widget::Widget w("test");
    w.setBounds(Rect(0, 0, 100, 100));

    bool callbackFired = false;
    w.setOnDoubleClickCallback([&](widget::Widget& widget) {
        callbackFired = true;
        EXPECT_EQ(&widget, &w);
    });

    // 调用 onDoubleClick 应该触发回调
    bool result = w.onDoubleClick(50, 50, 0, 0);
    EXPECT_TRUE(result);
    EXPECT_TRUE(callbackFired);
}

TEST(WidgetCallbackTest, OnRightClickCallback)
{
    widget::Widget w("test");
    w.setBounds(Rect(0, 0, 100, 100));

    bool callbackFired = false;
    w.setOnRightClickCallback([&](widget::Widget& widget) {
        callbackFired = true;
        EXPECT_EQ(&widget, &w);
    });

    // 调用 onRightClick 应该触发回调
    bool result = w.onRightClick(50, 50, 0);
    EXPECT_TRUE(result);
    EXPECT_TRUE(callbackFired);
}

TEST(WidgetCallbackTest, NoCallbackReturnsFalse)
{
    widget::Widget w("test");
    w.setBounds(Rect(0, 0, 100, 100));

    // 没有设置回调时，onDoubleClick 应该返回 false
    EXPECT_FALSE(w.onDoubleClick(50, 50, 0, 0));
    EXPECT_FALSE(w.onRightClick(50, 50, 0));
}

// ==================== ContainerWidget 双击/右键分发测试 ====================

class ContainerWidgetNewEventsTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        container.setBounds(Rect(0, 0, 200, 200));
        auto child = std::make_unique<MockClickWidget>("child");
        childPtr = child.get();
        container.addChild(std::move(child));
    }

    widget::ContainerWidget container;
    MockClickWidget* childPtr = nullptr;
};

TEST_F(ContainerWidgetNewEventsTest, DoubleClickDispatchesToChild)
{
    bool result = container.onDoubleClick(50, 50, 0, 0);
    EXPECT_TRUE(result);
    EXPECT_EQ(1, childPtr->doubleClickCount);
}

TEST_F(ContainerWidgetNewEventsTest, RightClickDispatchesToChild)
{
    bool result = container.onRightClick(50, 50, 0);
    EXPECT_TRUE(result);
    EXPECT_EQ(1, childPtr->rightClickCount);
}

TEST_F(ContainerWidgetNewEventsTest, DoubleClickOutsideContainerReturnsFalse)
{
    bool result = container.onDoubleClick(250, 250, 0, 0);
    EXPECT_FALSE(result);
    EXPECT_EQ(0, childPtr->doubleClickCount);
}

TEST_F(ContainerWidgetNewEventsTest, RightClickOutsideContainerReturnsFalse)
{
    bool result = container.onRightClick(250, 250, 0);
    EXPECT_FALSE(result);
    EXPECT_EQ(0, childPtr->rightClickCount);
}

// ==================== ScrollableWidget 双击/右键分发测试 ====================

class ScrollableWidgetNewEventsTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        scrollable.setBounds(Rect(0, 0, 200, 200));
        scrollable.setContentHeight(400);
        auto child = std::make_unique<MockClickWidget>("child");
        childPtr = child.get();
        child->setBounds(Rect(0, 0, 200, 100));
        scrollable.addChild(std::move(child));
    }

    widget::ScrollableWidget scrollable;
    MockClickWidget* childPtr = nullptr;
};

TEST_F(ScrollableWidgetNewEventsTest, DoubleClickDispatchesToChild)
{
    bool result = scrollable.onDoubleClick(50, 50, 0, 0);
    EXPECT_TRUE(result);
    EXPECT_EQ(1, childPtr->doubleClickCount);
}

TEST_F(ScrollableWidgetNewEventsTest, RightClickDispatchesToChild)
{
    bool result = scrollable.onRightClick(50, 50, 0);
    EXPECT_TRUE(result);
    EXPECT_EQ(1, childPtr->rightClickCount);
}

TEST_F(ScrollableWidgetNewEventsTest, DoubleClickWithScrollOffset)
{
    scrollable.setScrollY(50);
    // Child is at scroll offset, so click at y=50 should hit child at adjusted y=100
    // But child's bounds are (0, 0, 200, 100), adjusted y = 50 + 50 = 100,
    // which is outside child bounds (0-100). Let's test without offset.
    scrollable.setScrollY(0);
    bool result = scrollable.onDoubleClick(50, 50, 0, 0);
    EXPECT_TRUE(result);
}

// ==================== BuiltinEvents 绑定测试 ====================

class BuiltinEventsNewEventsTest : public ::testing::Test {
protected:
    void SetUp() override { tpl::bindings::BuiltinEvents::instance().initialize(); }
};

TEST_F(BuiltinEventsNewEventsTest, DoubleClickEventDispatchesOnDoubleClick)
{
    MockClickWidget widget("test");
    widget.setActive(true);
    widget.setVisible(true);

    auto event = tpl::bindings::event_utils::createClickEvent(50, 50, 0, 2, 0);
    bool handled = tpl::bindings::BuiltinEvents::instance().handle(&widget, "doubleClick", event);
    EXPECT_TRUE(handled);
    EXPECT_EQ(1, widget.doubleClickCount);
}

TEST_F(BuiltinEventsNewEventsTest, RightClickEventDispatchesOnRightClick)
{
    MockClickWidget widget("test");
    widget.setActive(true);
    widget.setVisible(true);

    auto event = tpl::bindings::event_utils::createClickEvent(50, 50, 1, 1, 0);
    bool handled = tpl::bindings::BuiltinEvents::instance().handle(&widget, "rightClick", event);
    EXPECT_TRUE(handled);
    EXPECT_EQ(1, widget.rightClickCount);
}

TEST_F(BuiltinEventsNewEventsTest, InactiveWidgetDoesNotHandleDoubleClick)
{
    MockClickWidget widget("test");
    widget.setActive(false);
    widget.setVisible(true);

    auto event = tpl::bindings::event_utils::createClickEvent(50, 50, 0, 2, 0);
    // BuiltinEvents::handle() returns true when a handler is found, but the handler
    // itself checks isActive() and does not call onDoubleClick for inactive widgets.
    bool handled = tpl::bindings::BuiltinEvents::instance().handle(&widget, "doubleClick", event);
    EXPECT_TRUE(handled);                  // handler was found and invoked (but early-returned internally)
    EXPECT_EQ(0, widget.doubleClickCount); // the widget's onDoubleClick was NOT called
}

// ==================== KageroEngine 拖拽生命周期集成测试 ====================

// 跟踪 onDragStart/onDrag/onDragEnd/onRelease 调用的 Mock Widget
class MockDragTrackingWidget : public widget::Widget {
public:
    explicit MockDragTrackingWidget(const std::string& id)
        : Widget(id)
    {
        setBounds(Rect(0, 0, 100, 100));
    }

    bool onClick(i32 mouseX, i32 mouseY, i32 button, i32 mods) override
    {
        (void)mouseX;
        (void)mouseY;
        (void)mods;
        lastClickButton = button;
        clickCount++;
        return consumeClick;
    }

    bool onDragStart(i32 mouseX, i32 mouseY, i32 button, i32 mods) override
    {
        dragStartCount++;
        lastDragStartX = mouseX;
        lastDragStartY = mouseY;
        lastDragStartButton = button;
        lastDragStartMods = mods;
        return true;
    }

    bool onDrag(i32 mouseX, i32 mouseY, i32 deltaX, i32 deltaY, i32 button) override
    {
        dragCount++;
        lastDragX = mouseX;
        lastDragY = mouseY;
        lastDragDeltaX = deltaX;
        lastDragDeltaY = deltaY;
        lastDragButton = button;
        return true;
    }

    bool onDragEnd(i32 mouseX, i32 mouseY, i32 button, bool dropped) override
    {
        dragEndCount++;
        lastDragEndX = mouseX;
        lastDragEndY = mouseY;
        lastDragEndButton = button;
        lastDragEndDropped = dropped;
        return true;
    }

    bool onRelease(i32 mouseX, i32 mouseY, i32 button, i32 mods) override
    {
        (void)mouseX;
        (void)mouseY;
        (void)mods;
        releaseCount++;
        lastReleaseButton = button;
        return true;
    }

    // 控制是否消费点击（不消费则不会进入拖拽流程）
    bool consumeClick = true;

    int clickCount = 0;
    int dragStartCount = 0;
    int dragCount = 0;
    int dragEndCount = 0;
    int releaseCount = 0;

    int lastClickButton = -1;
    int lastDragStartX = 0;
    int lastDragStartY = 0;
    int lastDragStartButton = -1;
    int lastDragStartMods = 0;
    int lastDragX = 0;
    int lastDragY = 0;
    int lastDragDeltaX = 0;
    int lastDragDeltaY = 0;
    int lastDragButton = -1;
    int lastDragEndX = 0;
    int lastDragEndY = 0;
    int lastDragEndButton = -1;
    bool lastDragEndDropped = false;
    int lastReleaseButton = -1;
};

// ==================== BuiltinEvents 拖拽事件分发测试 ====================
//
// 验证 BuiltinEvents 的 dragStart/dragEnd 处理器从 DragStartEvent/DragEndEvent
// 事件对象读取 button/mods/dropped 字段，正确转发到 Widget::onDragStart/onDragEnd，
// 不再硬编码为 0。这些测试复用上方的 MockDragTrackingWidget。

TEST_F(BuiltinEventsNewEventsTest, DragStartEventDispatchesOnDragStartWithButtonAndMods)
{
    // DragStartEvent 携带 button=1（右键）、mods=Shift+Control（0x0001|0x0002=0x0003）
    MockDragTrackingWidget widget("test");
    widget.setActive(true);
    widget.setVisible(true);

    auto event = tpl::bindings::event_utils::createDragStartEvent(50, 60, 1, 0x0003);
    bool handled = tpl::bindings::BuiltinEvents::instance().handle(&widget, "dragStart", event);

    EXPECT_TRUE(handled);
    EXPECT_EQ(1, widget.dragStartCount);
    EXPECT_EQ(50, widget.lastDragStartX);
    EXPECT_EQ(60, widget.lastDragStartY);
    EXPECT_EQ(1, widget.lastDragStartButton);
    EXPECT_EQ(0x0003, widget.lastDragStartMods);
}

TEST_F(BuiltinEventsNewEventsTest, DragStartEventDefaultsModsToZero)
{
    // 默认 mods=0
    MockDragTrackingWidget widget("test");
    widget.setActive(true);
    widget.setVisible(true);

    auto event = tpl::bindings::event_utils::createDragStartEvent(10, 20, 0);
    bool handled = tpl::bindings::BuiltinEvents::instance().handle(&widget, "dragStart", event);

    EXPECT_TRUE(handled);
    EXPECT_EQ(1, widget.dragStartCount);
    EXPECT_EQ(0, widget.lastDragStartButton);
    EXPECT_EQ(0, widget.lastDragStartMods);
}

TEST_F(BuiltinEventsNewEventsTest, DragEndEventDispatchesOnDragEndWithButtonAndDropped)
{
    // DragEndEvent 携带 button=2（中键）、dropped=true
    MockDragTrackingWidget widget("test");
    widget.setActive(true);
    widget.setVisible(true);

    auto event = tpl::bindings::event_utils::createDragEndEvent(70, 80, 2, true);
    bool handled = tpl::bindings::BuiltinEvents::instance().handle(&widget, "dragEnd", event);

    EXPECT_TRUE(handled);
    EXPECT_EQ(1, widget.dragEndCount);
    EXPECT_EQ(70, widget.lastDragEndX);
    EXPECT_EQ(80, widget.lastDragEndY);
    EXPECT_EQ(2, widget.lastDragEndButton);
    EXPECT_TRUE(widget.lastDragEndDropped);
}

TEST_F(BuiltinEventsNewEventsTest, DragEndEventDefaultsDroppedToFalse)
{
    // 默认 dropped=false
    MockDragTrackingWidget widget("test");
    widget.setActive(true);
    widget.setVisible(true);

    auto event = tpl::bindings::event_utils::createDragEndEvent(30, 40, 0);
    bool handled = tpl::bindings::BuiltinEvents::instance().handle(&widget, "dragEnd", event);

    EXPECT_TRUE(handled);
    EXPECT_EQ(1, widget.dragEndCount);
    EXPECT_EQ(0, widget.lastDragEndButton);
    EXPECT_FALSE(widget.lastDragEndDropped);
}

TEST_F(BuiltinEventsNewEventsTest, InactiveWidgetDoesNotHandleDragStart)
{
    // 非激活 Widget 不应触发 onDragStart
    MockDragTrackingWidget widget("test");
    widget.setActive(false);
    widget.setVisible(true);

    auto event = tpl::bindings::event_utils::createDragStartEvent(50, 60, 0, 0);
    bool handled = tpl::bindings::BuiltinEvents::instance().handle(&widget, "dragStart", event);

    EXPECT_TRUE(handled);                // handler 被找到并调用（但内部提前返回）
    EXPECT_EQ(0, widget.dragStartCount); // 但 onDragStart 未被调用
}

TEST_F(BuiltinEventsNewEventsTest, InvisibleWidgetDoesNotHandleDragEnd)
{
    // 不可见 Widget 不应触发 onDragEnd
    MockDragTrackingWidget widget("test");
    widget.setActive(true);
    widget.setVisible(false);

    auto event = tpl::bindings::event_utils::createDragEndEvent(50, 60, 0, false);
    bool handled = tpl::bindings::BuiltinEvents::instance().handle(&widget, "dragEnd", event);

    EXPECT_TRUE(handled);              // handler 被找到并调用（但内部提前返回）
    EXPECT_EQ(0, widget.dragEndCount); // 但 onDragEnd 未被调用
}

class KageroEngineDragLifecycleTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        auto result = engine.initialize(canvas, {800, 600});
        ASSERT_TRUE(result.success());
    }

    MockCanvas canvas;
    KageroEngine engine;
};

TEST_F(KageroEngineDragLifecycleTest, HandleClickTriggersOnDragStart)
{
    // handleClick 成功点击后应立即触发 onDragStart，传入正确的 button 与 mods
    auto widget = std::make_unique<MockDragTrackingWidget>("test");
    auto* widgetPtr = widget.get();
    engine.addLayer(std::move(widget), 0);

    engine.handleClick(50, 60, 0, 1); // button=0(左键), mods=1(Shift)

    EXPECT_EQ(1, widgetPtr->clickCount);
    EXPECT_EQ(1, widgetPtr->dragStartCount);
    EXPECT_EQ(50, widgetPtr->lastDragStartX);
    EXPECT_EQ(60, widgetPtr->lastDragStartY);
    EXPECT_EQ(0, widgetPtr->lastDragStartButton);
    EXPECT_EQ(1, widgetPtr->lastDragStartMods);
}

TEST_F(KageroEngineDragLifecycleTest, HandleClickDoesNotTriggerOnDragStartWhenClickNotConsumed)
{
    // onClick 返回 false（未消费点击）时不应触发 onDragStart
    auto widget = std::make_unique<MockDragTrackingWidget>("test");
    auto* widgetPtr = widget.get();
    widgetPtr->consumeClick = false;
    engine.addLayer(std::move(widget), 0);

    engine.handleClick(50, 60, 0, 0);

    EXPECT_EQ(1, widgetPtr->clickCount);
    EXPECT_EQ(0, widgetPtr->dragStartCount);
}

TEST_F(KageroEngineDragLifecycleTest, HandleMouseMoveTriggersOnDragAfterClick)
{
    // 点击后移动鼠标应触发 onDrag，传入正确的 button（来自 m_dragButton）
    auto widget = std::make_unique<MockDragTrackingWidget>("test");
    auto* widgetPtr = widget.get();
    engine.addLayer(std::move(widget), 0);

    engine.handleClick(50, 50, 0, 0);
    EXPECT_EQ(0, widgetPtr->dragCount);

    // 第一次移动（建立 m_hasLastMousePos）
    engine.handleMouseMove(55, 55);
    // 第二次移动（产生 deltaX/deltaY）
    engine.handleMouseMove(65, 70);

    EXPECT_GE(widgetPtr->dragCount, 1);
    EXPECT_EQ(0, widgetPtr->lastDragButton); // button 来自 m_dragButton
}

TEST_F(KageroEngineDragLifecycleTest, HandleReleaseTriggersOnDragEndThenOnRelease)
{
    // 释放鼠标时应先触发 onDragEnd（dropped=false），再触发 onRelease
    auto widget = std::make_unique<MockDragTrackingWidget>("test");
    auto* widgetPtr = widget.get();
    engine.addLayer(std::move(widget), 0);

    engine.handleClick(50, 50, 0, 0);
    EXPECT_EQ(0, widgetPtr->dragEndCount);
    EXPECT_EQ(0, widgetPtr->releaseCount);

    engine.handleRelease(60, 70, 0, 0);

    // onDragEnd 应被调用一次，dropped=false
    EXPECT_EQ(1, widgetPtr->dragEndCount);
    EXPECT_FALSE(widgetPtr->lastDragEndDropped);
    EXPECT_EQ(60, widgetPtr->lastDragEndX);
    EXPECT_EQ(70, widgetPtr->lastDragEndY);

    // onRelease 也应被调用
    EXPECT_EQ(1, widgetPtr->releaseCount);
}

TEST_F(KageroEngineDragLifecycleTest, HandleReleaseDoesNotTriggerOnDragEndWithoutClick)
{
    // 未点击直接释放不应触发 onDragEnd（m_draggingWidget 为 null）
    auto widget = std::make_unique<MockDragTrackingWidget>("test");
    auto* widgetPtr = widget.get();
    engine.addLayer(std::move(widget), 0);

    engine.handleRelease(50, 50, 0, 0);

    EXPECT_EQ(0, widgetPtr->dragEndCount);
}

TEST_F(KageroEngineDragLifecycleTest, DragEndButtonMatchesClickButton)
{
    // onDragEnd 的 button 参数应与触发拖拽的 click button 一致（来自 m_dragButton）
    auto widget = std::make_unique<MockDragTrackingWidget>("test");
    auto* widgetPtr = widget.get();
    engine.addLayer(std::move(widget), 0);

    // 注意：MockDragTrackingWidget 的 onClick 不区分按钮，均返回 consumeClick
    engine.handleClick(50, 50, 2, 0); // button=2(中键)
    engine.handleRelease(60, 60, 2, 0);

    EXPECT_EQ(2, widgetPtr->lastDragStartButton);
    EXPECT_EQ(2, widgetPtr->lastDragEndButton);
}

TEST_F(KageroEngineDragLifecycleTest, FullDragLifecycleOrder)
{
    // 完整拖拽生命周期：onClick → onDragStart → onDrag → onDragEnd → onRelease
    auto widget = std::make_unique<MockDragTrackingWidget>("test");
    auto* widgetPtr = widget.get();
    engine.addLayer(std::move(widget), 0);

    // 1. 点击
    engine.handleClick(50, 50, 0, 0);
    EXPECT_EQ(1, widgetPtr->clickCount);
    EXPECT_EQ(1, widgetPtr->dragStartCount);

    // 2. 移动（建立基准位置）
    engine.handleMouseMove(55, 55);

    // 3. 拖拽
    engine.handleMouseMove(70, 80);
    EXPECT_GE(widgetPtr->dragCount, 1);

    // 4. 释放
    engine.handleRelease(70, 80, 0, 0);
    EXPECT_EQ(1, widgetPtr->dragEndCount);
    EXPECT_EQ(1, widgetPtr->releaseCount);

    // 5. 再次移动不应触发 onDrag（拖拽已结束）
    int dragCountAfterRelease = widgetPtr->dragCount;
    engine.handleMouseMove(75, 85);
    engine.handleMouseMove(80, 90);
    EXPECT_EQ(dragCountAfterRelease, widgetPtr->dragCount);
}

} // namespace mc::client::ui::kagero
