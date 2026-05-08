# 快速开始

欢迎使用 Kagero UI 引擎！本指南将帮助您快速上手，了解如何创建和管理用户界面。

## 简介

Kagero（陽炎，かげろう）是 Minecraft Reborn 项目的现代化 UI 引擎，采用声明式模板、响应式状态管理和组件化架构。

核心特性：
- **声明式模板** - XML 风格的模板语法，零内联脚本
- **响应式状态** - 自动追踪状态变化并更新 UI
- **事件总线** - 类型安全的事件分发系统
- **组件化架构** - 可复用的 Widget 组件库
- **Flex 布局** - 类似 CSS Flexbox 的现代布局系统

## 基本概念

### 命名空间

所有 Kagero 类型位于 `mc::client::ui::kagero` 命名空间下：

```cpp
using namespace mc::client::ui::kagero;
using namespace mc::client::ui::kagero::widget;
using namespace mc::client::ui::kagero::state;
using namespace mc::client::ui::kagero::event;
```

### Widget 基类

所有 UI 组件都继承自 `Widget` 基类：

```cpp
#include "widget/Widget.hpp"

class MyWidget : public mc::client::ui::kagero::widget::Widget {
public:
    void paint(PaintContext& ctx) override {
        // 绘制逻辑
    }

    bool onClick(i32 mouseX, i32 mouseY, i32 button) override {
        // 点击处理
        return true;
    }
};
```

### 矩形与坐标

Kagero 使用 `Rect` 结构表示矩形区域：

```cpp
Rect rect(10, 20, 200, 100);  // x=10, y=20, width=200, height=100

// 常用方法
rect.contains(50, 50);        // 检查点是否在矩形内
rect.intersects(other);       // 检查两矩形是否相交
rect.centerX(), rect.centerY(); // 获取中心点
```

## 创建简单界面

### 方式一：代码创建

```cpp
#include "kagero/widget/ButtonWidget.hpp"
#include "kagero/widget/TextWidget.hpp"
#include "kagero/widget/ContainerWidget.hpp"

using namespace mc::client::ui::kagero::widget;

// 创建容器
auto container = std::make_unique<ContainerWidget>("main");
container->setBounds(Rect(0, 0, 800, 600));

// 创建标题文本
auto title = std::make_unique<TextWidget>("title", 300, 50, 200, 30);
title->setText("Hello Kagero!");
title->setColor(Colors::WHITE);
title->setShadow(true);

// 创建按钮
auto button = std::make_unique<ButtonWidget>(
    "btn_start",              // ID
    300, 200,                 // 位置
    200, 40,                  // 尺寸
    "Start Game",             // 文本
    [](ButtonWidget& btn) {   // 点击回调
        // 处理点击
    }
);

// 添加到容器
container->addChild(std::move(title));
container->addChild(std::move(button));
```

### 方式二：模板创建

```cpp
#include "kagero/template/Template.hpp"
#include "kagero/state/StateStore.hpp"

using namespace mc::client::ui::kagero::tpl;
using namespace mc::client::ui::kagero::state;

// 初始化模板系统
initializeTemplateSystem();

// 编译模板
TemplateCompiler compiler;
auto compiled = compiler.compile(R"(
    <screen id="main" width="800" height="600">
        <text id="title" x="300" y="50" text="Hello Kagero!" 
              color="#FFFFFF" shadow="true"/>
        <button id="btn_start" x="300" y="200" width="200" height="40"
                text="Start Game" on:click="onStartGame"/>
    </screen>
)");

// 创建绑定上下文
StateStore& store = StateStore::instance();
EventBus& eventBus = EventBus::instance();
BindingContext ctx(store, eventBus);

// 注册回调
ctx.exposeCallback("onStartGame", [](Widget* w, const Event& e) {
    // 处理点击
});

// 实例化模板
TemplateInstance instance(compiled.get(), ctx);
auto root = instance.instantiate();
```

## 状态管理

### 使用 StateStore

```cpp
#include "kagero/state/StateStore.hpp"

using namespace mc::client::ui::kagero::state;

StateStore& store = StateStore::instance();

// 设置状态
store.set<i32>("player.health", 100);
store.set<std::string>("player.name", "Steve");

// 获取状态
i32 health = store.get<i32>("player.health", 0);
std::string name = store.get<std::string>("player.name", "Unknown");

// 订阅状态变化
u64 subId = store.subscribe("player.health", []() {
    i32 newHealth = StateStore::instance().get<i32>("player.health");
    // 更新 UI
});

// 取消订阅
store.unsubscribe(subId);
```

### 使用 Reactive

```cpp
#include "kagero/state/ReactiveState.hpp"

using namespace mc::client::ui::kagero::state;

// 创建响应式状态
Reactive<i32> health(100);
Reactive<std::string> name("Steve");

// 观察变化
health.observe([](i32 oldValue, i32 newValue) {
    std::cout << "Health: " << oldValue << " -> " << newValue << std::endl;
});

// 修改值会触发观察者
health.set(80);  // 输出: Health: 100 -> 80
```

## 事件处理

### 使用 EventBus

```cpp
#include "kagero/event/EventBus.hpp"
#include "kagero/event/InputEvents.hpp"

using namespace mc::client::ui::kagero::event;

EventBus& bus = EventBus::instance();

// 订阅事件
auto subId = bus.subscribe<MouseClickEvent>([](const MouseClickEvent& e) {
    std::cout << "Click at (" << e.x() << ", " << e.y() << ")" << std::endl;
});

// 发布事件
bus.publish(MouseClickEvent(100, 200, 0));

// 取消订阅
bus.unsubscribe(subId);
```

### 使用 EventSubscription (RAII)

```cpp
// 自动管理订阅生命周期
{
    EventSubscription<MouseClickEvent> subscription([](const MouseClickEvent& e) {
        // 处理事件
    });
    // 离开作用域自动取消订阅
}
```

## 布局系统

### Flex 布局

```cpp
#include "kagero/layout/LayoutSystem.hpp"

using namespace mc::client::ui::kagero::layout;

// 配置 Flex 布局
FlexConfig config;
config.direction = Direction::Row;           // 水平排列
config.justifyContent = JustifyContent::Center; // 居中对齐
config.alignItems = Align::Center;           // 交叉轴居中
config.gap = 10;                              // 元素间距

// 执行布局
auto& engine = LayoutEngine::instance();
engine.layoutFlex(containerAdaptor, Rect(0, 0, 800, 600), config);
```

### 预设配置

```cpp
// 水平居中
auto config = centerRowFlexConfig();

// 垂直居中
auto config = centerColumnFlexConfig();

// 两端对齐
auto config = spaceBetweenFlexConfig();
```

## 下一步

- [模板系统](./02-template-system.md) - 学习声明式模板语法
- [状态系统](./03-state-system.md) - 深入了解响应式状态管理
- [事件系统](./04-event-system.md) - 掌握事件处理机制
- [内置组件](./05-built-in-widgets.md) - 浏览可用的 Widget 组件
