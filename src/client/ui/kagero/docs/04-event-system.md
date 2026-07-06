# 事件系统

Kagero 事件系统提供类型安全的事件分发机制，支持事件优先级、事件过滤和自动取消订阅。

## 概述

事件系统包含以下核心组件：

| 组件 | 描述 |
|------|------|
| `Event` | 事件基类 |
| `EventBus` | 全局事件总线 |
| `EventSubscription` | RAII 订阅管理器 |
| 输入事件 | 鼠标、键盘等输入事件 |
| UI 事件 | 焦点、值变化等 UI 事件 |
| Widget 事件 | 按钮、滑块等组件事件 |

### 命名空间

```cpp
#include "kagero/event/Event.hpp"
#include "kagero/event/EventBus.hpp"
#include "kagero/event/InputEvents.hpp"
#include "kagero/event/UIEvents.hpp"
#include "kagero/event/WidgetEvents.hpp"

using namespace mc::client::ui::kagero::event;
```

## Event 基类

所有事件类型都继承自 `Event` 基类：

```cpp
class Event {
public:
    virtual ~Event() = default;
    
    // 获取事件类型
    virtual EventType getType() const = 0;
    
    // 获取事件名称
    virtual const char* getName() const = 0;
    
    // 检查是否被取消
    bool isCancelled() const;
    
    // 取消事件
    void cancel();
    
    // 事件时间戳
    u64 timestamp() const;
    
    // 是否是气泡事件
    virtual bool bubbles() const;
    
    // 是否可取消
    virtual bool isCancellable() const;
    
    // 事件目标
    void* target() const;
    void* currentTarget() const;
};
```

### 事件类型枚举

```cpp
enum class EventType : u32 {
    // 输入事件
    MouseClick = 1,
    MouseRelease = 2,
    MouseDrag = 3,
    MouseScroll = 4,
    MouseMove = 5,
    MouseEnter = 6,
    MouseLeave = 7,
    
    KeyPress = 10,
    KeyRelease = 11,
    KeyRepeat = 12,
    CharInput = 13,
    
    // 焦点事件
    FocusGained = 20,
    FocusLost = 21,
    
    // 值变化事件
    ValueChange = 30,
    TextChange = 31,
    
    // 组件事件
    WidgetInit = 40,
    WidgetResize = 41,
    WidgetShow = 42,
    WidgetHide = 43,
    WidgetEnable = 44,
    WidgetDisable = 45,

    // 自定义事件
    Custom = 1000
};
```

## EventBus

全局事件总线，采用单例模式。

### 订阅事件

```cpp
EventBus& bus = EventBus::instance();

// 订阅事件
auto subId = bus.subscribe<MouseClickEvent>([](const MouseClickEvent& e) {
    std::cout << "Click at (" << e.x() << ", " << e.y() << ")" << std::endl;
});

// 订阅事件（带优先级）
auto highPriorityId = bus.subscribe<MouseClickEvent>([](const MouseClickEvent& e) {
    // 优先处理
}, 100);  // 数值越大越先执行
```

### 发布事件

```cpp
// 发布事件
bus.publish(MouseClickEvent(100, 200, 0));

// 发布事件（移动语义）
bus.publish(MouseClickEvent(100, 200, 0));
```

### 取消订阅

```cpp
// 取消订阅
bus.unsubscribe(subId);

// 清除所有处理器
bus.clear();

// 获取处理器数量
size_t count = bus.handlerCount<MouseClickEvent>();
```

### 事件过滤器

```cpp
// 添加事件过滤器
u64 filterId = bus.addFilter([](const Event& e) {
    // 返回 true 继续处理，返回 false 阻止处理
    if (e.getType() == EventType::MouseClick) {
        const auto& click = static_cast<const MouseClickEvent&>(e);
        return click.x() > 0 && click.y() > 0;  // 只处理有效坐标
    }
    return true;
});

// 移除过滤器
bus.removeFilter(filterId);
```

## EventSubscription

RAII 订阅管理器，自动管理订阅生命周期。

```cpp
// 自动管理订阅
{
    EventSubscription<MouseClickEvent> subscription([](const MouseClickEvent& e) {
        // 处理点击
    });
    
    // 检查是否有效
    bool valid = subscription.valid();
    
    // 获取订阅 ID
    EventBus::HandlerId id = subscription.id();
    
    // 离开作用域自动取消订阅
}

// 手动取消订阅
{
    EventSubscription<MouseClickEvent> subscription([](const MouseClickEvent& e) {});
    subscription.unsubscribe();  // 手动取消
}
```

## 输入事件

### 鼠标事件

#### MouseClickEvent

鼠标点击事件：

```cpp
MouseClickEvent event(100, 200, 0, 1);  // x, y, button, clicks

event.x();          // X 坐标
event.y();          // Y 坐标
event.button();     // 鼠标按钮（0=左键，1=右键，2=中键）
event.clicks();     // 点击次数

event.isLeftButton();    // 是否左键
event.isRightButton();   // 是否右键
event.isDoubleClick();   // 是否双击
```

#### MouseReleaseEvent

鼠标释放事件：

```cpp
MouseReleaseEvent event(100, 200, 0);  // x, y, button

event.x();          // X 坐标
event.y();          // Y 坐标
event.button();     // 鼠标按钮
```

#### MouseDragEvent

鼠标拖动事件：

```cpp
MouseDragEvent event(100, 200, 10, 5, 0);  // x, y, deltaX, deltaY, button

event.x();          // 当前 X 坐标
event.y();          // 当前 Y 坐标
event.deltaX();     // X 方向移动量
event.deltaY();     // Y 方向移动量
event.button();     // 鼠标按钮
```

#### MouseScrollEvent

鼠标滚轮事件：

```cpp
MouseScrollEvent event(100, 200, 0.0, 1.0);  // x, y, deltaX, deltaY

event.x();          // X 坐标
event.y();          // Y 坐标
event.deltaX();     // 水平滚动量
event.deltaY();     // 垂直滚动量

event.isScrollUp();   // 是否向上滚动
event.isScrollDown(); // 是否向下滚动
```

#### MouseMoveEvent

鼠标移动事件：

```cpp
MouseMoveEvent event(100, 200, 10, 5);  // x, y, deltaX, deltaY

event.x();          // 当前 X 坐标
event.y();          // 当前 Y 坐标
event.deltaX();     // X 方向移动量
event.deltaY();     // Y 方向移动量
```

#### MouseEnterEvent / MouseLeaveEvent

鼠标进入/离开事件：

```cpp
MouseEnterEvent event(100, 200);  // x, y
MouseLeaveEvent event(100, 200);  // x, y

event.x();          // X 坐标
event.y();          // Y 坐标
```

### 键盘事件

#### KeyEvent

键盘按键事件：

```cpp
KeyEvent event(65, 0, 1, 0);  // key, scanCode, action, mods

event.key();        // 键码（GLFW 键码）
event.scanCode();   // 扫描码
event.action();     // 动作（0=释放，1=按下，2=重复）
event.mods();       // 修饰键状态

event.isPressed();   // 是否按下
event.isReleased();  // 是否释放
event.isRepeat();    // 是否重复

event.hasShift();    // 是否按住 Shift
event.hasControl();  // 是否按住 Ctrl
event.hasAlt();      // 是否按住 Alt
event.hasSuper();    // 是否按住 Super/Win
```

#### CharInputEvent

字符输入事件：

```cpp
CharInputEvent event(0x4E);  // Unicode 码点

event.codePoint();  // Unicode 码点

std::string utf8 = event.toUtf8();  // 转换为 UTF-8 字符串
```

## UI 事件

### 焦点事件

```cpp
FocusGainedEvent focusGained;  // 获得焦点
FocusLostEvent focusLost;      // 失去焦点

// 这两个事件不会冒泡
focusGained.bubbles();  // false
focusLost.bubbles();    // false
```

### 组件事件

```cpp
// 初始化事件
WidgetInitEvent initEvent;

// 尺寸改变事件
WidgetResizeEvent resizeEvent(100, 100, 200, 150);  // oldWidth, oldHeight, newWidth, newHeight
resizeEvent.oldWidth();
resizeEvent.oldHeight();
resizeEvent.newWidth();
resizeEvent.newHeight();

// 显示/隐藏事件
WidgetShowEvent showEvent;
WidgetHideEvent hideEvent;

// 启用/禁用事件
WidgetEnableEvent enableEvent;
WidgetDisableEvent disableEvent;
```

### 屏幕事件

```cpp
// 屏幕打开
ScreenOpenEvent openEvent("main_menu");
openEvent.screenId();  // "main_menu"

// 屏幕关闭
ScreenCloseEvent closeEvent("main_menu");
closeEvent.screenId();  // "main_menu"

// 屏幕切换
ScreenChangeEvent changeEvent("main_menu", "game");
changeEvent.fromScreen();  // "main_menu"
changeEvent.toScreen();    // "game"
```

## Widget 事件

### 值变化事件

```cpp
// 通用值变化事件
ValueChangeEvent<i32> valueEvent(10, 20);  // oldValue, newValue
valueEvent.oldValue();  // 10
valueEvent.newValue();  // 20

// 文本变化事件
TextChangeEvent textEvent("old", "new");
textEvent.oldText();  // "old"
textEvent.newText();  // "new"
```

### 按钮事件

```cpp
ButtonClickEvent clickEvent(button, 0);  // button widget, buttonIndex
clickEvent.buttonIndex();  // 鼠标按钮索引
```

### 滑块事件

```cpp
// 滑块值变化（f64）
using SliderValueChangeEvent = ValueChangeEvent<f64>;
```

### 复选框事件

```cpp
// 复选框状态变化（bool）
using CheckboxChangeEvent = ValueChangeEvent<bool>;
```

### 选择事件

```cpp
SelectionEvent selectEvent(0, 2);  // oldIndex, newIndex
selectEvent.oldIndex();    // 0
selectEvent.newIndex();    // 2
selectEvent.hasSelection(); // true (newIndex >= 0)

// 多选事件
MultiSelectionEvent multiSelect({0, 1}, {1, 2, 3});
multiSelect.oldIndices();  // {0, 1}
multiSelect.newIndices();  // {1, 2, 3}
```

### 物品槽事件

```cpp
SlotClickEvent slotEvent(0, 0, false);  // slotIndex, button, shiftHeld
slotEvent.slotIndex();    // 0
slotEvent.button();       // 0
slotEvent.isShiftHeld();  // false
slotEvent.isLeftClick();  // true
slotEvent.isRightClick(); // false
```

### 拖拽事件

```cpp
// DragStartEvent：x, y, button, mods（mods 默认为 0）
DragStartEvent dragStart(100, 200, 0, 0x0003);  // 左键 + Shift+Ctrl
dragStart.x();
dragStart.y();
dragStart.button();       // 0（左键）
dragStart.mods();         // 0x0003（Shift | Control）
dragStart.isLeftButton();  // true

// DragEndEvent：x, y, button, dropped
DragEndEvent dragEnd(150, 250, 0, true);  // 左键，被外部取消
dragEnd.x();
dragEnd.y();
dragEnd.button();      // 0（左键）
dragEnd.wasDropped();  // true
```

`button` 取值与 GLFW 一致（0=左键，1=右键，2=中键），`mods` 为 `KeyMods` 位掩码（参见 `Types.hpp`）。这两个字段会由 `BuiltinEvents` 的 `dragStart`/`dragEnd` 处理器透传到 `Widget::onDragStart(x, y, button, mods)` 与 `Widget::onDragEnd(x, y, button, dropped)`，与 `KageroEngine` 在 `handleClick`/`handleRelease` 中直接调用 Widget 虚方法的语义保持一致。

## 自定义事件

创建自定义事件：

```cpp
// 定义自定义事件
class PlayerDeathEvent : public Event {
public:
    explicit PlayerDeathEvent(i32 playerId, const std::string& cause)
        : m_playerId(playerId), m_cause(cause) {}
    
    EventType getType() const override { return EventType::Custom; }
    const char* getName() const override { return "PlayerDeath"; }
    
    i32 playerId() const { return m_playerId; }
    const std::string& cause() const { return m_cause; }
    
private:
    i32 m_playerId;
    std::string m_cause;
};

// 使用自定义事件
EventBus::instance().subscribe<PlayerDeathEvent>([](const PlayerDeathEvent& e) {
    std::cout << "Player " << e.playerId() << " died: " << e.cause() << std::endl;
});

EventBus::instance().publish(PlayerDeathEvent(1, "Fall damage"));
```

## 最佳实践

### 1. 使用 EventSubscription 管理订阅

```cpp
// 推荐：使用 RAII 管理订阅
class MyWidget : public Widget {
    EventSubscription<MouseClickEvent> m_clickSubscription;
    
    void init() override {
        m_clickSubscription = EventSubscription<MouseClickEvent>(
            [this](const MouseClickEvent& e) {
                handleClick(e);
            }
        );
    }
    
    // 析构时自动取消订阅
};
```

### 2. 事件优先级

```cpp
// 高优先级处理器先执行
bus.subscribe<MouseClickEvent>([](const MouseClickEvent& e) {
    // 最高优先级，先执行
}, 1000);

bus.subscribe<MouseClickEvent>([](const MouseClickEvent& e) {
    // 普通优先级
}, 0);

bus.subscribe<MouseClickEvent>([](const MouseClickEvent& e) {
    // 低优先级，最后执行
}, -100);
```

### 3. 事件取消

```cpp
bus.subscribe<MouseClickEvent>([](const MouseClickEvent& e) {
    // 取消事件，阻止后续处理器执行
    e.cancel();
});

bus.subscribe<MouseClickEvent>([](const MouseClickEvent& e) {
    // 检查是否已取消
    if (e.isCancelled()) {
        return;  // 事件已被取消
    }
    // 处理事件
});
```

### 4. 事件过滤

```cpp
// 全局过滤器，只处理特定条件的事件
bus.addFilter([](const Event& e) {
    if (e.getType() == EventType::MouseClick) {
        const auto& click = static_cast<const MouseClickEvent&>(e);
        // 只处理左键点击
        return click.isLeftButton();
    }
    return true;
});
```

### 5. 批量订阅

```cpp
class InputHandler {
public:
    void init() {
        // 使用 EventSubscription 管理多个订阅
        m_subscriptions.push_back(
            EventSubscription<MouseClickEvent>([this](const auto& e) { onClick(e); })
        );
        m_subscriptions.push_back(
            EventSubscription<KeyEvent>([this](const auto& e) { onKey(e); })
        );
    }
    
private:
    std::vector<std::variant<
        EventSubscription<MouseClickEvent>,
        EventSubscription<KeyEvent>
    >> m_subscriptions;
};
```

### 6. 线程安全

EventBus 内部使用互斥锁保护，可以在多线程环境中安全使用：

```cpp
// 线程安全的事件发布
std::thread([]() {
    EventBus::instance().publish(MouseClickEvent(100, 200, 0));
}).detach();
```

## API 参考

### EventResult

事件处理结果结构体：

```cpp
struct EventResult {
    bool handled = false;       // 是否被处理
    bool cancelled = false;     // 是否被取消

    EventResult& setHandled(bool value = true);
    EventResult& setCancelled(bool value = true);
};
```

### EventHandler

事件处理器类型定义：

```cpp
template<typename EventT>
using EventHandler = std::function<void(const EventT&)>;
```

### EventFilter

事件过滤器类型定义：

```cpp
using EventFilter = std::function<bool(const Event&)>;
```

返回 `true` 继续处理，返回 `false` 阻止处理。

### EventBus 完整 API

```cpp
class EventBus {
public:
    using HandlerId = u64;

    // 获取单例实例
    static EventBus& instance();

    // 订阅事件（返回订阅 ID）
    template<typename EventT>
    HandlerId subscribe(EventHandler<EventT> handler, i32 priority = 0);

    // 取消订阅
    bool unsubscribe(HandlerId id);

    // 发布事件
    template<typename EventT>
    void publish(const EventT& event);

    // 添加事件过滤器（返回过滤器 ID）
    HandlerId addFilter(EventFilter filter);

    // 添加事件过滤器（指定 ID，已废弃）
    void addFilterWithId(HandlerId id, EventFilter filter);

    // 移除事件过滤器
    bool removeFilter(HandlerId id);

    // 清除所有处理器和过滤器
    void clear();

    // 获取特定类型事件的处理器数量
    template<typename EventT>
    size_t handlerCount() const;
};
```

### SimpleEvent 模板

用于快速创建携带数据的事件类型：

```cpp
template<typename T, EventType Type>
class SimpleEvent : public Event {
public:
    explicit SimpleEvent(T data);

    EventType getType() const override;
    const char* getName() const override;

    const T& data() const;  // 只读访问
    T& data();               // 可变访问
};

// 使用示例
using IntValueEvent = SimpleEvent<i32, EventType::ValueChange>;
IntValueEvent event(42);
```

## 事件冒泡行为

不同事件的冒泡行为如下：

| 事件类型 | 是否冒泡 | 说明 |
|----------|----------|------|
| MouseClickEvent | 是 | 默认行为 |
| MouseReleaseEvent | 是 | 默认行为 |
| MouseDragEvent | 是 | 默认行为 |
| MouseScrollEvent | 是 | 默认行为 |
| MouseMoveEvent | 是 | 默认行为 |
| MouseEnterEvent | 是 | 默认行为 |
| MouseLeaveEvent | 是 | 默认行为 |
| KeyEvent | 是 | 默认行为 |
| CharInputEvent | 是 | 默认行为 |
| FocusGainedEvent | 否 | 焦点事件不冒泡 |
| FocusLostEvent | 否 | 焦点事件不冒泡 |
| WidgetInitEvent | 否 | 初始化事件不冒泡 |
| WidgetResizeEvent | 是 | 默认行为 |
| WidgetShowEvent | 是 | 默认行为 |
| WidgetHideEvent | 是 | 默认行为 |
| WidgetEnableEvent | 是 | 默认行为 |
| WidgetDisableEvent | 是 | 默认行为 |
| ValueChangeEvent | 是 | 默认行为 |
| TextChangeEvent | 是 | 默认行为 |
| 自定义事件 | 视情况 | 默认冒泡 |

## 注意事项

### 事件取消

事件取消是通过 `cancel()` 方法实现的。由于事件处理器接收的是 `const` 引用，`cancel()` 方法被声明为 `const`，内部使用 `mutable` 成员变量：

```cpp
// 正确用法
bus.subscribe<MouseClickEvent>([](const MouseClickEvent& e) {
    e.cancel();  // 可以在 const 上下文中调用
});
```

取消事件后，EventBus 会检查 `isCancelled()` 状态，停止调用后续处理器。

### 线程安全

EventBus 内部使用 `std::mutex` 保护所有操作，包括：
- 订阅/取消订阅
- 发布事件
- 添加/移除过滤器

可以在多线程环境中安全使用，但要注意：
- 处理器内部不应执行耗时操作
- 处理器内部不应死锁等待另一个线程的事件处理
