#include <atomic>
#include <chrono>
#include <thread>
#include <vector>
#include <gtest/gtest.h>

#include "client/ui/kagero/event/Event.hpp"
#include "client/ui/kagero/event/EventBus.hpp"
#include "client/ui/kagero/event/InputEvents.hpp"
#include "client/ui/kagero/event/UIEvents.hpp"
#include "client/ui/kagero/event/WidgetEvents.hpp"

using namespace mc::client::ui::kagero::event;
using mc::i32;
using mc::u32;
using mc::u64;

// ==================== Event Base Tests ====================

class EventTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(EventTest, EventTypeValues)
{
    // 验证事件类型枚举值与文档一致
    EXPECT_EQ(static_cast<u32>(EventType::MouseClick), 1u);
    EXPECT_EQ(static_cast<u32>(EventType::MouseRelease), 2u);
    EXPECT_EQ(static_cast<u32>(EventType::MouseDrag), 3u);
    EXPECT_EQ(static_cast<u32>(EventType::MouseScroll), 4u);
    EXPECT_EQ(static_cast<u32>(EventType::MouseMove), 5u);
    EXPECT_EQ(static_cast<u32>(EventType::MouseEnter), 6u);
    EXPECT_EQ(static_cast<u32>(EventType::MouseLeave), 7u);

    EXPECT_EQ(static_cast<u32>(EventType::KeyPress), 10u);
    EXPECT_EQ(static_cast<u32>(EventType::KeyRelease), 11u);
    EXPECT_EQ(static_cast<u32>(EventType::KeyRepeat), 12u);
    EXPECT_EQ(static_cast<u32>(EventType::CharInput), 13u);

    EXPECT_EQ(static_cast<u32>(EventType::FocusGained), 20u);
    EXPECT_EQ(static_cast<u32>(EventType::FocusLost), 21u);

    EXPECT_EQ(static_cast<u32>(EventType::ValueChange), 30u);
    EXPECT_EQ(static_cast<u32>(EventType::TextChange), 31u);

    EXPECT_EQ(static_cast<u32>(EventType::WidgetInit), 40u);
    EXPECT_EQ(static_cast<u32>(EventType::WidgetResize), 41u);
    EXPECT_EQ(static_cast<u32>(EventType::WidgetShow), 42u);
    EXPECT_EQ(static_cast<u32>(EventType::WidgetHide), 43u);
    EXPECT_EQ(static_cast<u32>(EventType::WidgetEnable), 44u);
    EXPECT_EQ(static_cast<u32>(EventType::WidgetDisable), 45u);

    EXPECT_EQ(static_cast<u32>(EventType::Custom), 1000u);
}

TEST_F(EventTest, EventCancellation)
{
    MouseClickEvent event(100, 200, 0, 1);

    // 初始状态
    EXPECT_FALSE(event.isCancelled());

    // 取消事件
    event.cancel();
    EXPECT_TRUE(event.isCancelled());
}

TEST_F(EventTest, EventTimestamp)
{
    MouseClickEvent event(100, 200, 0, 1);

    // 默认时间戳为0
    EXPECT_EQ(event.timestamp(), 0u);

    // 设置时间戳
    event.setTimestamp(12345);
    EXPECT_EQ(event.timestamp(), 12345u);
}

TEST_F(EventTest, EventBubbles)
{
    MouseClickEvent clickEvent(100, 200, 0);
    FocusGainedEvent focusGained;
    FocusLostEvent focusLost;

    // 默认事件应该冒泡
    EXPECT_TRUE(clickEvent.bubbles());

    // 焦点事件不应该冒泡
    EXPECT_FALSE(focusGained.bubbles());
    EXPECT_FALSE(focusLost.bubbles());
}

TEST_F(EventTest, EventCancellable)
{
    MouseClickEvent event(100, 200, 0);

    // 默认事件应该可取消
    EXPECT_TRUE(event.isCancellable());
}

TEST_F(EventTest, EventResultUsage)
{
    // 测试 EventResult 结构体
    EventResult result;
    EXPECT_FALSE(result.handled);
    EXPECT_FALSE(result.cancelled);

    // 链式调用
    result.setHandled().setCancelled();
    EXPECT_TRUE(result.handled);
    EXPECT_TRUE(result.cancelled);

    // 单独设置
    EventResult result2;
    result2.setHandled(true);
    EXPECT_TRUE(result2.handled);
    EXPECT_FALSE(result2.cancelled);

    result2.setCancelled(false);
    EXPECT_FALSE(result2.cancelled);
}

TEST_F(EventTest, EventFilterUsage)
{
    // 测试 EventFilter 类型定义
    i32 callCount = 0;
    EventFilter filter = [&callCount](const Event& e) {
        callCount++;
        return e.getType() != EventType::MouseClick;
    };

    MouseClickEvent clickEvent(100, 200, 0);
    MouseMoveEvent moveEvent(100, 200, 0, 0);

    EXPECT_FALSE(filter(clickEvent)); // MouseClick 被过滤
    EXPECT_TRUE(filter(moveEvent));   // MouseMove 通过
    EXPECT_EQ(callCount, 2);
}

TEST_F(EventTest, EventTarget)
{
    MouseClickEvent event(100, 200, 0);

    // 默认目标为空
    EXPECT_EQ(event.target(), nullptr);
    EXPECT_EQ(event.currentTarget(), nullptr);

    // 设置目标
    int dummyTarget = 42;
    event.setTarget(&dummyTarget);
    EXPECT_EQ(event.target(), &dummyTarget);

    event.setCurrentTarget(&dummyTarget);
    EXPECT_EQ(event.currentTarget(), &dummyTarget);
}

// ==================== Input Events Tests ====================

class InputEventsTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(InputEventsTest, MouseClickEvent)
{
    MouseClickEvent event(100, 200, 0, 2);

    EXPECT_EQ(event.getType(), EventType::MouseClick);
    EXPECT_STREQ(event.getName(), "MouseClick");
    EXPECT_EQ(event.x(), 100);
    EXPECT_EQ(event.y(), 200);
    EXPECT_EQ(event.button(), 0);
    EXPECT_EQ(event.clicks(), 2);

    EXPECT_TRUE(event.isLeftButton());
    EXPECT_FALSE(event.isRightButton());
    EXPECT_TRUE(event.isDoubleClick());
}

TEST_F(InputEventsTest, MouseClickEventRightButton)
{
    MouseClickEvent event(100, 200, 1, 1);

    EXPECT_FALSE(event.isLeftButton());
    EXPECT_TRUE(event.isRightButton());
    EXPECT_FALSE(event.isDoubleClick());
}

TEST_F(InputEventsTest, MouseClickEventMiddleButton)
{
    MouseClickEvent event(100, 200, 2, 1);

    EXPECT_FALSE(event.isLeftButton());
    EXPECT_FALSE(event.isRightButton());
}

TEST_F(InputEventsTest, MouseReleaseEvent)
{
    MouseReleaseEvent event(150, 250, 0);

    EXPECT_EQ(event.getType(), EventType::MouseRelease);
    EXPECT_STREQ(event.getName(), "MouseRelease");
    EXPECT_EQ(event.x(), 150);
    EXPECT_EQ(event.y(), 250);
    EXPECT_EQ(event.button(), 0);
}

TEST_F(InputEventsTest, MouseDragEvent)
{
    MouseDragEvent event(100, 200, 10, 5, 0);

    EXPECT_EQ(event.getType(), EventType::MouseDrag);
    EXPECT_STREQ(event.getName(), "MouseDrag");
    EXPECT_EQ(event.x(), 100);
    EXPECT_EQ(event.y(), 200);
    EXPECT_EQ(event.deltaX(), 10);
    EXPECT_EQ(event.deltaY(), 5);
    EXPECT_EQ(event.button(), 0);
}

TEST_F(InputEventsTest, MouseScrollEvent)
{
    MouseScrollEvent event(100, 200, 0.0, 1.0);

    EXPECT_EQ(event.getType(), EventType::MouseScroll);
    EXPECT_STREQ(event.getName(), "MouseScroll");
    EXPECT_EQ(event.x(), 100);
    EXPECT_EQ(event.y(), 200);
    EXPECT_DOUBLE_EQ(event.deltaX(), 0.0);
    EXPECT_DOUBLE_EQ(event.deltaY(), 1.0);

    // deltaY > 0 表示向上滚动
    EXPECT_TRUE(event.isScrollUp());
    EXPECT_FALSE(event.isScrollDown());
}

TEST_F(InputEventsTest, MouseScrollEventDown)
{
    MouseScrollEvent event(100, 200, 0.0, -1.0);

    // deltaY < 0 表示向下滚动
    EXPECT_FALSE(event.isScrollUp());
    EXPECT_TRUE(event.isScrollDown());
}

TEST_F(InputEventsTest, MouseMoveEvent)
{
    MouseMoveEvent event(100, 200, 10, 5);

    EXPECT_EQ(event.getType(), EventType::MouseMove);
    EXPECT_STREQ(event.getName(), "MouseMove");
    EXPECT_EQ(event.x(), 100);
    EXPECT_EQ(event.y(), 200);
    EXPECT_EQ(event.deltaX(), 10);
    EXPECT_EQ(event.deltaY(), 5);
}

TEST_F(InputEventsTest, MouseEnterEvent)
{
    MouseEnterEvent event(100, 200);

    EXPECT_EQ(event.getType(), EventType::MouseEnter);
    EXPECT_STREQ(event.getName(), "MouseEnter");
    EXPECT_EQ(event.x(), 100);
    EXPECT_EQ(event.y(), 200);
}

TEST_F(InputEventsTest, MouseLeaveEvent)
{
    MouseLeaveEvent event(100, 200);

    EXPECT_EQ(event.getType(), EventType::MouseLeave);
    EXPECT_STREQ(event.getName(), "MouseLeave");
    EXPECT_EQ(event.x(), 100);
    EXPECT_EQ(event.y(), 200);
}

TEST_F(InputEventsTest, KeyEvent)
{
    // 按下事件
    KeyEvent pressEvent(65, 0, 1, 0); // GLFW_KEY_A
    EXPECT_EQ(pressEvent.getType(), EventType::KeyPress);
    EXPECT_STREQ(pressEvent.getName(), "KeyPress");
    EXPECT_EQ(pressEvent.key(), 65);
    EXPECT_EQ(pressEvent.scanCode(), 0);
    EXPECT_EQ(pressEvent.action(), 1);
    EXPECT_EQ(pressEvent.mods(), 0);
    EXPECT_TRUE(pressEvent.isPressed());
    EXPECT_FALSE(pressEvent.isReleased());
    EXPECT_FALSE(pressEvent.isRepeat());

    // 释放事件
    KeyEvent releaseEvent(65, 0, 0, 0);
    EXPECT_EQ(releaseEvent.getType(), EventType::KeyRelease);
    EXPECT_TRUE(releaseEvent.isReleased());
    EXPECT_FALSE(releaseEvent.isPressed());

    // 重复事件
    KeyEvent repeatEvent(65, 0, 2, 0);
    EXPECT_EQ(repeatEvent.getType(), EventType::KeyRepeat);
    EXPECT_TRUE(repeatEvent.isRepeat());
}

TEST_F(InputEventsTest, KeyEventModifiers)
{
    KeyEvent event(65, 0, 1, 0x01); // Shift
    EXPECT_TRUE(event.hasShift());
    EXPECT_FALSE(event.hasControl());
    EXPECT_FALSE(event.hasAlt());
    EXPECT_FALSE(event.hasSuper());

    KeyEvent event2(65, 0, 1, 0x02); // Control
    EXPECT_FALSE(event2.hasShift());
    EXPECT_TRUE(event2.hasControl());

    KeyEvent event3(65, 0, 1, 0x04); // Alt
    EXPECT_TRUE(event3.hasAlt());

    KeyEvent event4(65, 0, 1, 0x08); // Super
    EXPECT_TRUE(event4.hasSuper());

    KeyEvent event5(65, 0, 1, 0x05); // Shift + Alt
    EXPECT_TRUE(event5.hasShift());
    EXPECT_TRUE(event5.hasAlt());
}

TEST_F(InputEventsTest, CharInputEvent)
{
    CharInputEvent event(0x4E); // 'N'

    EXPECT_EQ(event.getType(), EventType::CharInput);
    EXPECT_STREQ(event.getName(), "CharInput");
    EXPECT_EQ(event.codePoint(), 0x4Eu);

    // 测试 UTF-8 转换 - ASCII 字符
    CharInputEvent asciiEvent(0x41); // 'A'
    EXPECT_EQ(asciiEvent.toUtf8(), "A");

    // 测试 UTF-8 转换 - 中文
    CharInputEvent chineseEvent(0x4E2D); // '中'
    std::string utf8 = chineseEvent.toUtf8();
    EXPECT_EQ(utf8.size(), 3u); // 中文字符 UTF-8 编码为 3 字节
}

TEST_F(InputEventsTest, CharInputEventUtf8Conversion)
{
    // 测试各种 Unicode 范围的 UTF-8 编码

    // U+0000 - U+007F: 1 字节
    CharInputEvent e1(0x7F);
    EXPECT_EQ(e1.toUtf8().size(), 1u);

    // U+0080 - U+07FF: 2 字节
    CharInputEvent e2(0x07FF);
    EXPECT_EQ(e2.toUtf8().size(), 2u);

    // U+0800 - U+FFFF: 3 字节
    CharInputEvent e3(0xFFFF);
    EXPECT_EQ(e3.toUtf8().size(), 3u);

    // U+10000 - U+10FFFF: 4 字节
    CharInputEvent e4(0x10000);
    EXPECT_EQ(e4.toUtf8().size(), 4u);
}

// ==================== UI Events Tests ====================

class UIEventsTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(UIEventsTest, FocusGainedEvent)
{
    FocusGainedEvent event;

    EXPECT_EQ(event.getType(), EventType::FocusGained);
    EXPECT_STREQ(event.getName(), "FocusGained");
    EXPECT_FALSE(event.bubbles());
}

TEST_F(UIEventsTest, FocusLostEvent)
{
    FocusLostEvent event;

    EXPECT_EQ(event.getType(), EventType::FocusLost);
    EXPECT_STREQ(event.getName(), "FocusLost");
    EXPECT_FALSE(event.bubbles());
}

TEST_F(UIEventsTest, WidgetInitEvent)
{
    WidgetInitEvent event;

    EXPECT_EQ(event.getType(), EventType::WidgetInit);
    EXPECT_STREQ(event.getName(), "WidgetInit");
    EXPECT_FALSE(event.bubbles());
}

TEST_F(UIEventsTest, WidgetResizeEvent)
{
    WidgetResizeEvent event(100, 100, 200, 150);

    EXPECT_EQ(event.getType(), EventType::WidgetResize);
    EXPECT_STREQ(event.getName(), "WidgetResize");
    EXPECT_EQ(event.oldWidth(), 100);
    EXPECT_EQ(event.oldHeight(), 100);
    EXPECT_EQ(event.newWidth(), 200);
    EXPECT_EQ(event.newHeight(), 150);
}

TEST_F(UIEventsTest, WidgetShowHideEvents)
{
    WidgetShowEvent showEvent;
    EXPECT_EQ(showEvent.getType(), EventType::WidgetShow);
    EXPECT_STREQ(showEvent.getName(), "WidgetShow");
    EXPECT_TRUE(showEvent.bubbles()); // WidgetShowEvent 应该冒泡

    WidgetHideEvent hideEvent;
    EXPECT_EQ(hideEvent.getType(), EventType::WidgetHide);
    EXPECT_STREQ(hideEvent.getName(), "WidgetHide");
    EXPECT_TRUE(hideEvent.bubbles()); // WidgetHideEvent 应该冒泡
}

TEST_F(UIEventsTest, WidgetEnableDisableEvents)
{
    WidgetEnableEvent enableEvent;
    EXPECT_EQ(enableEvent.getType(), EventType::WidgetEnable);
    EXPECT_STREQ(enableEvent.getName(), "WidgetEnable");
    EXPECT_TRUE(enableEvent.bubbles()); // WidgetEnableEvent 应该冒泡

    WidgetDisableEvent disableEvent;
    EXPECT_EQ(disableEvent.getType(), EventType::WidgetDisable);
    EXPECT_STREQ(disableEvent.getName(), "WidgetDisable");
    EXPECT_TRUE(disableEvent.bubbles()); // WidgetDisableEvent 应该冒泡
}

TEST_F(UIEventsTest, ScreenOpenEvent)
{
    ScreenOpenEvent event("main_menu");

    EXPECT_EQ(event.getType(), EventType::Custom);
    EXPECT_STREQ(event.getName(), "ScreenOpen");
    EXPECT_EQ(event.screenId(), "main_menu");
}

TEST_F(UIEventsTest, ScreenCloseEvent)
{
    ScreenCloseEvent event("pause_menu");

    EXPECT_EQ(event.getType(), EventType::Custom);
    EXPECT_STREQ(event.getName(), "ScreenClose");
    EXPECT_EQ(event.screenId(), "pause_menu");
}

TEST_F(UIEventsTest, ScreenChangeEvent)
{
    ScreenChangeEvent event("main_menu", "game");

    EXPECT_EQ(event.getType(), EventType::Custom);
    EXPECT_STREQ(event.getName(), "ScreenChange");
    EXPECT_EQ(event.fromScreen(), "main_menu");
    EXPECT_EQ(event.toScreen(), "game");
}

// ==================== Widget Events Tests ====================

class WidgetEventsTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(WidgetEventsTest, ValueChangeEvent)
{
    ValueChangeEvent<i32> event(10, 20);

    EXPECT_EQ(event.getType(), EventType::ValueChange);
    EXPECT_STREQ(event.getName(), "ValueChange");
    EXPECT_EQ(event.oldValue(), 10);
    EXPECT_EQ(event.newValue(), 20);
}

TEST_F(WidgetEventsTest, ValueChangeEventWithSource)
{
    int source = 42;
    ValueChangeEvent<i32> event(10, 20, &source);

    EXPECT_EQ(event.target(), &source);
}

TEST_F(WidgetEventsTest, TextChangeEvent)
{
    TextChangeEvent event("old text", "new text");

    EXPECT_EQ(event.getType(), EventType::TextChange);
    EXPECT_STREQ(event.getName(), "TextChange");
    EXPECT_EQ(event.oldText(), "old text");
    EXPECT_EQ(event.newText(), "new text");
}

TEST_F(WidgetEventsTest, ButtonClickEvent)
{
    int button = 42;
    ButtonClickEvent event(&button, 0);

    EXPECT_EQ(event.getType(), EventType::MouseClick);
    EXPECT_STREQ(event.getName(), "ButtonClick");
    EXPECT_EQ(event.buttonIndex(), 0);
    EXPECT_EQ(event.target(), &button);
}

TEST_F(WidgetEventsTest, SliderValueChangeEvent)
{
    SliderValueChangeEvent event(0.5, 0.75);

    EXPECT_EQ(event.getType(), EventType::ValueChange);
    EXPECT_DOUBLE_EQ(event.oldValue(), 0.5);
    EXPECT_DOUBLE_EQ(event.newValue(), 0.75);
}

TEST_F(WidgetEventsTest, CheckboxChangeEvent)
{
    CheckboxChangeEvent event(false, true);

    EXPECT_EQ(event.getType(), EventType::ValueChange);
    EXPECT_EQ(event.oldValue(), false);
    EXPECT_EQ(event.newValue(), true);
}

TEST_F(WidgetEventsTest, SelectionEvent)
{
    SelectionEvent event(0, 2);

    EXPECT_EQ(event.getType(), EventType::ValueChange);
    EXPECT_STREQ(event.getName(), "Selection");
    EXPECT_EQ(event.oldIndex(), 0);
    EXPECT_EQ(event.newIndex(), 2);
    EXPECT_TRUE(event.hasSelection());
}

TEST_F(WidgetEventsTest, SelectionEventNoSelection)
{
    SelectionEvent event(2, -1);

    EXPECT_EQ(event.newIndex(), -1);
    EXPECT_FALSE(event.hasSelection());
}

TEST_F(WidgetEventsTest, MultiSelectionEvent)
{
    std::vector<i32> oldIndices = {0, 1};
    std::vector<i32> newIndices = {1, 2, 3};
    MultiSelectionEvent event(oldIndices, newIndices);

    EXPECT_EQ(event.getType(), EventType::ValueChange);
    EXPECT_STREQ(event.getName(), "MultiSelection");
    EXPECT_EQ(event.oldIndices(), oldIndices);
    EXPECT_EQ(event.newIndices(), newIndices);
}

TEST_F(WidgetEventsTest, SlotClickEvent)
{
    SlotClickEvent event(5, 0, true);

    EXPECT_EQ(event.getType(), EventType::MouseClick);
    EXPECT_STREQ(event.getName(), "SlotClick");
    EXPECT_EQ(event.slotIndex(), 5);
    EXPECT_EQ(event.button(), 0);
    EXPECT_TRUE(event.isShiftHeld());
    EXPECT_TRUE(event.isLeftClick());
    EXPECT_FALSE(event.isRightClick());
}

TEST_F(WidgetEventsTest, SlotClickEventRightClick)
{
    SlotClickEvent event(5, 1, false);

    EXPECT_FALSE(event.isLeftClick());
    EXPECT_TRUE(event.isRightClick());
}

TEST_F(WidgetEventsTest, ContainerCloseEvent)
{
    int container = 42;
    ContainerCloseEvent event(&container);

    EXPECT_EQ(event.getType(), EventType::Custom);
    EXPECT_STREQ(event.getName(), "ContainerClose");
    EXPECT_EQ(event.target(), &container);
}

TEST_F(WidgetEventsTest, FormSubmitEvent)
{
    FormSubmitEvent event("login_form");

    EXPECT_EQ(event.getType(), EventType::Custom);
    EXPECT_STREQ(event.getName(), "FormSubmit");
    EXPECT_EQ(event.formId(), "login_form");
}

TEST_F(WidgetEventsTest, DragStartEvent)
{
    DragStartEvent event(100, 200);

    EXPECT_EQ(event.getType(), EventType::Custom);
    EXPECT_STREQ(event.getName(), "DragStart");
    EXPECT_EQ(event.x(), 100);
    EXPECT_EQ(event.y(), 200);
}

TEST_F(WidgetEventsTest, DragEndEvent)
{
    DragEndEvent event(150, 250, true);

    EXPECT_EQ(event.getType(), EventType::Custom);
    EXPECT_STREQ(event.getName(), "DragEnd");
    EXPECT_EQ(event.x(), 150);
    EXPECT_EQ(event.y(), 250);
    EXPECT_TRUE(event.wasDropped());
}

// ==================== EventBus Tests ====================

class EventBusTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 每个测试前清空事件总线
        EventBus::instance().clear();
    }

    void TearDown() override { EventBus::instance().clear(); }
};

TEST_F(EventBusTest, SubscribeAndPublish)
{
    i32 clickCount = 0;

    auto id =
        EventBus::instance().subscribe<MouseClickEvent>([&clickCount](const MouseClickEvent& e) { clickCount++; });

    EXPECT_NE(id, 0u);

    MouseClickEvent event(100, 200, 0);
    EventBus::instance().publish(event);

    EXPECT_EQ(clickCount, 1);
}

TEST_F(EventBusTest, Unsubscribe)
{
    i32 clickCount = 0;

    auto id =
        EventBus::instance().subscribe<MouseClickEvent>([&clickCount](const MouseClickEvent& e) { clickCount++; });

    EventBus::instance().publish(MouseClickEvent(100, 200, 0));
    EXPECT_EQ(clickCount, 1);

    // 取消订阅
    bool result = EventBus::instance().unsubscribe(id);
    EXPECT_TRUE(result);

    // 再次发布，处理器不应被调用
    EventBus::instance().publish(MouseClickEvent(100, 200, 0));
    EXPECT_EQ(clickCount, 1); // 还是 1
}

TEST_F(EventBusTest, UnsubscribeInvalidId)
{
    bool result = EventBus::instance().unsubscribe(99999);
    EXPECT_FALSE(result);
}

TEST_F(EventBusTest, MultipleSubscribers)
{
    i32 count1 = 0, count2 = 0, count3 = 0;

    auto id1 = EventBus::instance().subscribe<MouseClickEvent>([&count1](const MouseClickEvent&) { count1++; });
    auto id2 = EventBus::instance().subscribe<MouseClickEvent>([&count2](const MouseClickEvent&) { count2++; });
    auto id3 = EventBus::instance().subscribe<MouseClickEvent>([&count3](const MouseClickEvent&) { count3++; });

    EventBus::instance().publish(MouseClickEvent(100, 200, 0));

    EXPECT_EQ(count1, 1);
    EXPECT_EQ(count2, 1);
    EXPECT_EQ(count3, 1);

    EventBus::instance().unsubscribe(id1);
    EventBus::instance().unsubscribe(id2);
    EventBus::instance().unsubscribe(id3);
}

TEST_F(EventBusTest, Priority)
{
    std::vector<i32> order;

    auto id1 = EventBus::instance().subscribe<MouseClickEvent>([&order](const MouseClickEvent&) { order.push_back(1); },
        0 // 默认优先级
    );
    auto id2 = EventBus::instance().subscribe<MouseClickEvent>([&order](const MouseClickEvent&) { order.push_back(2); },
        100 // 高优先级
    );
    auto id3 = EventBus::instance().subscribe<MouseClickEvent>([&order](const MouseClickEvent&) { order.push_back(3); },
        -100 // 低优先级
    );

    EventBus::instance().publish(MouseClickEvent(100, 200, 0));

    // 应该按优先级顺序执行：2 -> 1 -> 3
    ASSERT_EQ(order.size(), 3u);
    EXPECT_EQ(order[0], 2); // 高优先级
    EXPECT_EQ(order[1], 1); // 中优先级
    EXPECT_EQ(order[2], 3); // 低优先级

    EventBus::instance().unsubscribe(id1);
    EventBus::instance().unsubscribe(id2);
    EventBus::instance().unsubscribe(id3);
}

TEST_F(EventBusTest, EventCancellation)
{
    std::vector<i32> order;

    // 第一个处理器取消事件，后续处理器不应该执行
    auto id1 = EventBus::instance().subscribe<MouseClickEvent>(
        [&order](const MouseClickEvent& e) {
            order.push_back(1);
            e.cancel(); // 取消事件
        },
        100 // 高优先级
    );
    auto id2 = EventBus::instance().subscribe<MouseClickEvent>(
        [&order](const MouseClickEvent& e) {
            if (!e.isCancelled()) {
                order.push_back(2);
            }
        },
        50 // 中优先级
    );
    auto id3 = EventBus::instance().subscribe<MouseClickEvent>(
        [&order](const MouseClickEvent& e) {
            if (!e.isCancelled()) {
                order.push_back(3);
            }
        },
        0 // 低优先级
    );

    MouseClickEvent event(100, 200, 0);
    EventBus::instance().publish(event);

    // 只有第一个处理器应该执行，因为事件被取消
    ASSERT_EQ(order.size(), 1u);
    EXPECT_EQ(order[0], 1);

    EventBus::instance().unsubscribe(id1);
    EventBus::instance().unsubscribe(id2);
    EventBus::instance().unsubscribe(id3);
}

TEST_F(EventBusTest, EventFilter)
{
    i32 clickCount = 0;

    // 添加过滤器：只处理 x > 50 的事件
    auto filterId = EventBus::instance().addFilter([](const Event& e) {
        if (e.getType() == EventType::MouseClick) {
            const auto& click = static_cast<const MouseClickEvent&>(e);
            return click.x() > 50;
        }
        return true;
    });

    auto subId =
        EventBus::instance().subscribe<MouseClickEvent>([&clickCount](const MouseClickEvent&) { clickCount++; });

    // x = 10，应该被过滤
    EventBus::instance().publish(MouseClickEvent(10, 200, 0));
    EXPECT_EQ(clickCount, 0);

    // x = 100，应该通过
    EventBus::instance().publish(MouseClickEvent(100, 200, 0));
    EXPECT_EQ(clickCount, 1);

    EventBus::instance().removeFilter(filterId);
    EventBus::instance().unsubscribe(subId);
}

TEST_F(EventBusTest, RemoveFilter)
{
    i32 clickCount = 0;

    auto filterId = EventBus::instance().addFilter([](const Event&) { return false; } // 过滤所有事件
    );

    auto subId =
        EventBus::instance().subscribe<MouseClickEvent>([&clickCount](const MouseClickEvent&) { clickCount++; });

    EventBus::instance().publish(MouseClickEvent(100, 200, 0));
    EXPECT_EQ(clickCount, 0);

    // 移除过滤器
    bool result = EventBus::instance().removeFilter(filterId);
    EXPECT_TRUE(result);

    EventBus::instance().publish(MouseClickEvent(100, 200, 0));
    EXPECT_EQ(clickCount, 1);

    EventBus::instance().unsubscribe(subId);
}

TEST_F(EventBusTest, RemoveInvalidFilter)
{
    bool result = EventBus::instance().removeFilter(99999);
    EXPECT_FALSE(result);
}

TEST_F(EventBusTest, AddFilterWithId)
{
    i32 clickCount = 0;

    // 使用 addFilterWithId 添加过滤器
    EventBus::instance().addFilterWithId(42, [](const Event&) { return false; });

    auto subId =
        EventBus::instance().subscribe<MouseClickEvent>([&clickCount](const MouseClickEvent&) { clickCount++; });

    EventBus::instance().publish(MouseClickEvent(100, 200, 0));
    EXPECT_EQ(clickCount, 0); // 过滤器阻止了事件

    // 移除过滤器
    bool result = EventBus::instance().removeFilter(42);
    EXPECT_TRUE(result);

    EventBus::instance().publish(MouseClickEvent(100, 200, 0));
    EXPECT_EQ(clickCount, 1);

    EventBus::instance().unsubscribe(subId);
}

TEST_F(EventBusTest, MultipleFilters)
{
    i32 clickCount = 0;

    // 添加多个过滤器
    auto filterId1 = EventBus::instance().addFilter([](const Event& e) {
        if (e.getType() == EventType::MouseClick) {
            const auto& click = static_cast<const MouseClickEvent&>(e);
            return click.x() > 0; // x > 0
        }
        return true;
    });
    auto filterId2 = EventBus::instance().addFilter([](const Event& e) {
        if (e.getType() == EventType::MouseClick) {
            const auto& click = static_cast<const MouseClickEvent&>(e);
            return click.y() > 0; // y > 0
        }
        return true;
    });

    auto subId =
        EventBus::instance().subscribe<MouseClickEvent>([&clickCount](const MouseClickEvent&) { clickCount++; });

    // x=0, y=100 - 被第一个过滤器阻止
    EventBus::instance().publish(MouseClickEvent(0, 100, 0));
    EXPECT_EQ(clickCount, 0);

    // x=100, y=0 - 被第二个过滤器阻止
    EventBus::instance().publish(MouseClickEvent(100, 0, 0));
    EXPECT_EQ(clickCount, 0);

    // x=100, y=100 - 通过两个过滤器
    EventBus::instance().publish(MouseClickEvent(100, 100, 0));
    EXPECT_EQ(clickCount, 1);

    EventBus::instance().removeFilter(filterId1);
    EventBus::instance().removeFilter(filterId2);
    EventBus::instance().unsubscribe(subId);
}

TEST_F(EventBusTest, HandlerCount)
{
    EXPECT_EQ(EventBus::instance().handlerCount<MouseClickEvent>(), 0u);

    auto id1 = EventBus::instance().subscribe<MouseClickEvent>([](const MouseClickEvent&) {});
    EXPECT_EQ(EventBus::instance().handlerCount<MouseClickEvent>(), 1u);

    auto id2 = EventBus::instance().subscribe<MouseClickEvent>([](const MouseClickEvent&) {});
    EXPECT_EQ(EventBus::instance().handlerCount<MouseClickEvent>(), 2u);

    EventBus::instance().unsubscribe(id1);
    EXPECT_EQ(EventBus::instance().handlerCount<MouseClickEvent>(), 1u);

    EventBus::instance().unsubscribe(id2);
    EXPECT_EQ(EventBus::instance().handlerCount<MouseClickEvent>(), 0u);
}

TEST_F(EventBusTest, Clear)
{
    auto id1 = EventBus::instance().subscribe<MouseClickEvent>([](const MouseClickEvent&) {});
    auto filterId = EventBus::instance().addFilter([](const Event&) { return true; });

    EXPECT_EQ(EventBus::instance().handlerCount<MouseClickEvent>(), 1u);

    EventBus::instance().clear();

    EXPECT_EQ(EventBus::instance().handlerCount<MouseClickEvent>(), 0u);
}

TEST_F(EventBusTest, DifferentEventTypes)
{
    i32 clickCount = 0, keyCount = 0;

    auto clickId =
        EventBus::instance().subscribe<MouseClickEvent>([&clickCount](const MouseClickEvent&) { clickCount++; });
    auto keyId = EventBus::instance().subscribe<KeyEvent>([&keyCount](const KeyEvent&) { keyCount++; });

    EventBus::instance().publish(MouseClickEvent(100, 200, 0));
    EventBus::instance().publish(KeyEvent(65, 0, 1, 0));

    EXPECT_EQ(clickCount, 1);
    EXPECT_EQ(keyCount, 1);

    EventBus::instance().unsubscribe(clickId);
    EventBus::instance().unsubscribe(keyId);
}

// ==================== EventSubscription Tests ====================

class EventSubscriptionTest : public ::testing::Test {
protected:
    void SetUp() override { EventBus::instance().clear(); }

    void TearDown() override { EventBus::instance().clear(); }
};

TEST_F(EventSubscriptionTest, AutoUnsubscribe)
{
    i32 clickCount = 0;

    {
        EventSubscription<MouseClickEvent> subscription([&clickCount](const MouseClickEvent&) { clickCount++; });

        EXPECT_TRUE(subscription.valid());
        EXPECT_NE(subscription.id(), 0u);

        EventBus::instance().publish(MouseClickEvent(100, 200, 0));
        EXPECT_EQ(clickCount, 1);
    } // 离开作用域，自动取消订阅

    EventBus::instance().publish(MouseClickEvent(100, 200, 0));
    EXPECT_EQ(clickCount, 1); // 还是 1，说明处理器已被移除
}

TEST_F(EventSubscriptionTest, ManualUnsubscribe)
{
    i32 clickCount = 0;

    EventSubscription<MouseClickEvent> subscription([&clickCount](const MouseClickEvent&) { clickCount++; });

    EXPECT_TRUE(subscription.valid());

    subscription.unsubscribe();
    EXPECT_FALSE(subscription.valid());

    EventBus::instance().publish(MouseClickEvent(100, 200, 0));
    EXPECT_EQ(clickCount, 0);
}

TEST_F(EventSubscriptionTest, MoveSemantics)
{
    i32 clickCount = 0;

    EventSubscription<MouseClickEvent> subscription1([&clickCount](const MouseClickEvent&) { clickCount++; });

    EXPECT_TRUE(subscription1.valid());
    auto id = subscription1.id();

    // 移动构造
    EventSubscription<MouseClickEvent> subscription2(std::move(subscription1));

    EXPECT_FALSE(subscription1.valid()); // 移动后无效
    EXPECT_TRUE(subscription2.valid());
    EXPECT_EQ(subscription2.id(), id); // ID 保持不变

    EventBus::instance().publish(MouseClickEvent(100, 200, 0));
    EXPECT_EQ(clickCount, 1);
}

TEST_F(EventSubscriptionTest, MoveAssignment)
{
    i32 clickCount = 0;

    EventSubscription<MouseClickEvent> subscription1([&clickCount](const MouseClickEvent&) { clickCount++; });

    EventSubscription<MouseClickEvent> subscription2([](const MouseClickEvent&) {} // 空处理器
    );

    auto id1 = subscription1.id();
    auto id2 = subscription2.id();

    // 移动赋值
    subscription2 = std::move(subscription1);

    EXPECT_FALSE(subscription1.valid()); // 移动后无效
    EXPECT_TRUE(subscription2.valid());
    EXPECT_EQ(subscription2.id(), id1); // 使用 id1

    EventBus::instance().publish(MouseClickEvent(100, 200, 0));
    EXPECT_EQ(clickCount, 1);
}

TEST_F(EventSubscriptionTest, Priority)
{
    std::vector<i32> order;

    EventSubscription<MouseClickEvent> sub1([&order](const MouseClickEvent&) { order.push_back(1); }, 0);
    EventSubscription<MouseClickEvent> sub2([&order](const MouseClickEvent&) { order.push_back(2); }, 100);

    EventBus::instance().publish(MouseClickEvent(100, 200, 0));

    ASSERT_EQ(order.size(), 2u);
    EXPECT_EQ(order[0], 2); // 高优先级先执行
    EXPECT_EQ(order[1], 1);
}

// ==================== Thread Safety Tests ====================

class EventBusThreadSafetyTest : public ::testing::Test {
protected:
    void SetUp() override { EventBus::instance().clear(); }

    void TearDown() override { EventBus::instance().clear(); }
};

TEST_F(EventBusThreadSafetyTest, ConcurrentPublish)
{
    std::atomic<i32> clickCount{0};

    auto id = EventBus::instance().subscribe<MouseClickEvent>([&clickCount](const MouseClickEvent&) { clickCount++; });

    const int numThreads = 4;
    const int eventsPerThread = 100;

    std::vector<std::thread> threads;
    for (int i = 0; i < numThreads; ++i) {
        threads.emplace_back([&eventsPerThread]() {
            for (int j = 0; j < eventsPerThread; ++j) {
                EventBus::instance().publish(MouseClickEvent(100, 200, 0));
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    EXPECT_EQ(clickCount.load(), numThreads * eventsPerThread);

    EventBus::instance().unsubscribe(id);
}

TEST_F(EventBusThreadSafetyTest, ConcurrentSubscribeUnsubscribe)
{
    const int numThreads = 4;
    const int opsPerThread = 50;

    std::vector<std::thread> threads;
    std::vector<std::vector<u64>> ids(static_cast<size_t>(numThreads));

    for (size_t i = 0; i < static_cast<size_t>(numThreads); ++i) {
        threads.emplace_back([&ids, i, opsPerThread]() {
            for (int j = 0; j < opsPerThread; ++j) {
                auto id = EventBus::instance().subscribe<MouseClickEvent>([](const MouseClickEvent&) {});
                ids[i].push_back(id);
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    // 取消订阅
    for (size_t i = 0; i < static_cast<size_t>(numThreads); ++i) {
        for (auto id : ids[i]) {
            EventBus::instance().unsubscribe(id);
        }
    }

    EXPECT_EQ(EventBus::instance().handlerCount<MouseClickEvent>(), 0u);
}

// ==================== Custom Event Tests ====================

class CustomEventTest : public ::testing::Test {
protected:
    void SetUp() override { EventBus::instance().clear(); }

    void TearDown() override { EventBus::instance().clear(); }
};

// 自定义事件类
class PlayerDeathEvent : public Event {
public:
    explicit PlayerDeathEvent(i32 playerId, const std::string& cause)
        : m_playerId(playerId)
        , m_cause(cause)
    {}

    EventType getType() const override { return EventType::Custom; }
    const char* getName() const override { return "PlayerDeath"; }

    i32 playerId() const { return m_playerId; }
    const std::string& cause() const { return m_cause; }

private:
    i32 m_playerId;
    std::string m_cause;
};

TEST_F(CustomEventTest, CustomEventSubscription)
{
    i32 lastPlayerId = 0;
    std::string lastCause;

    auto id = EventBus::instance().subscribe<PlayerDeathEvent>([&lastPlayerId, &lastCause](const PlayerDeathEvent& e) {
        lastPlayerId = e.playerId();
        lastCause = e.cause();
    });

    EventBus::instance().publish(PlayerDeathEvent(1, "Fall damage"));

    EXPECT_EQ(lastPlayerId, 1);
    EXPECT_EQ(lastCause, "Fall damage");

    EventBus::instance().unsubscribe(id);
}

TEST_F(CustomEventTest, CustomEventCancellation)
{
    PlayerDeathEvent event(1, "Fall damage");

    EXPECT_FALSE(event.isCancelled());
    EXPECT_TRUE(event.isCancellable());

    event.cancel();
    EXPECT_TRUE(event.isCancelled());
}

// ==================== SimpleEvent Tests ====================

TEST_F(CustomEventTest, SimpleEventUsage)
{
    i32 value = 0;

    auto id = EventBus::instance().subscribe<SimpleEvent<i32, EventType::ValueChange>>(
        [&value](const SimpleEvent<i32, EventType::ValueChange>& e) { value = e.data(); });

    EventBus::instance().publish(SimpleEvent<i32, EventType::ValueChange>(42));

    EXPECT_EQ(value, 42);

    EventBus::instance().unsubscribe(id);
}

TEST_F(CustomEventTest, SimpleEventMutableData)
{
    // 测试 SimpleEvent 的可变数据访问
    SimpleEvent<std::string, EventType::Custom> event(std::string("test"));

    EXPECT_EQ(event.data(), "test");

    // 可变引用访问
    event.data() = "modified";
    EXPECT_EQ(event.data(), "modified");
}

TEST_F(CustomEventTest, SimpleEventWithStruct)
{
    struct TestData {
        i32 x;
        i32 y;
    };

    auto id = EventBus::instance().subscribe<SimpleEvent<TestData, EventType::Custom>>(
        [](const SimpleEvent<TestData, EventType::Custom>& e) {
            // 处理结构体数据
            EXPECT_EQ(e.data().x, 10);
            EXPECT_EQ(e.data().y, 20);
        });

    EventBus::instance().publish(SimpleEvent<TestData, EventType::Custom>({10, 20}));

    EventBus::instance().unsubscribe(id);
}
