# 状态系统

Kagero 状态系统提供响应式的状态管理，支持自动追踪变化并通知订阅者。

## 概述

状态系统包含三个核心组件：

| 组件 | 描述 |
|------|------|
| `StateStore` | 全局状态存储，键值对形式，支持订阅 |
| `Reactive<T>` | 响应式包装器，自动通知观察者 |
| `Binding<T>` | 双向绑定工具，连接状态与组件 |

### 命名空间

```cpp
#include "kagero/state/StateStore.hpp"
#include "kagero/state/ReactiveState.hpp"
#include "kagero/state/StateBinding.hpp"
#include "kagero/state/StateObserver.hpp"

using namespace mc::client::ui::kagero::state;
```

## StateStore

全局状态存储器，采用单例模式。

### 基本操作

```cpp
StateStore& store = StateStore::instance();

// 设置值
store.set<i32>("player.health", 100);
store.set<std::string>("player.name", "Steve");
store.set<f32>("settings.volume", 0.8f);
store.set<bool>("ui.paused", false);

// 获取值（带默认值）
i32 health = store.get<i32>("player.health", 0);
std::string name = store.get<std::string>("player.name", "Unknown");

// 检查键是否存在
bool exists = store.has("player.health");

// 删除键
store.remove("player.health");

// 清空所有状态
store.clear();

// 获取所有键
std::vector<std::string> keys = store.keys();
```

### 订阅状态变化

```cpp
// 订阅状态变化
u64 subId = store.subscribe("player.health", []() {
    i32 newHealth = StateStore::instance().get<i32>("player.health");
    std::cout << "Health changed to: " << newHealth << std::endl;
});

// 取消订阅
store.unsubscribe(subId);

// 取消某键的所有订阅
store.unsubscribeAll("player.health");
```

### 动作分发

类似 Redux 的动作模式：

```cpp
// 定义动作
auto incrementHealth = [](StateStore& store) {
    i32 current = store.get<i32>("player.health", 0);
    store.set<i32>("player.health", current + 10);
};

// 派发动作
store.dispatch(incrementHealth);
```

### 中间件

```cpp
// 添加中间件
store.addMiddleware([](const std::string& key, const std::any& value, StateStore& store) {
    // 记录状态变化
    std::cout << "State changed: " << key << std::endl;
});

// 清除中间件
store.clearMiddlewares();
```

### 批量更新

```cpp
// 批量更新（延迟通知）
store.batchUpdate([](StateStore& store) {
    store.set<i32>("player.health", 100);
    store.set<i32>("player.maxHealth", 100);
    store.set<std::string>("player.name", "Alex");
    // 所有订阅者在批量更新结束后才收到通知
});
```

## Reactive

响应式状态包装器，自动追踪变化并通知观察者。

### 基本用法

```cpp
// 创建响应式状态
Reactive<i32> count(0);
Reactive<std::string> name("Steve");
Reactive<bool> visible(true);

// 获取值
i32 value = count.get();
const std::string& nameRef = name.get();

// 设置值（自动通知观察者）
count.set(10);
name.set("Alex");

// 隐式转换
i32 v = count;  // 自动调用 get()

// 赋值操作
count = 20;     // 自动调用 set()
```

### 观察者模式

```cpp
Reactive<i32> health(100);

// 添加观察者
auto observerId = health.observe([](i32 oldValue, i32 newValue) {
    std::cout << "Health: " << oldValue << " -> " << newValue << std::endl;
});

// 修改值触发观察者
health.set(80);  // 输出: Health: 100 -> 80

// 移除观察者
health.removeObserver(observerId);

// 清除所有观察者
health.clearObservers();

// 获取观察者数量
size_t count = health.observerCount();
```

### 修改器模式

```cpp
Reactive<std::vector<i32>> numbers;

// 使用 modify 修改复杂值
numbers.modify([](std::vector<i32>& vec) {
    vec.push_back(1);
    vec.push_back(2);
    vec.push_back(3);
});

// 强制通知（即使值未变）
numbers.forceNotify();
```

## Computed

计算属性，基于其他响应式状态自动计算值。

```cpp
Reactive<i32> a(10);
Reactive<i32> b(20);

// 创建计算属性
Computed<i32> sum([&a, &b]() {
    return a.get() + b.get();
});

// 获取计算值
i32 result = sum.get();  // 30

// 当依赖变化时，需要手动标记为脏
a.set(15);
sum.markDirty();         // 标记需要重新计算
result = sum.get();      // 35（重新计算）
```

## Binding

双向绑定工具，连接响应式状态和组件属性。

### 创建绑定

```cpp
Reactive<i32> volume(80);

// 从 Reactive 创建双向绑定
Binding<i32> volumeBinding = Binding<i32>::fromReactive(volume);

// 创建只读绑定
Binding<i32> readOnlyBinding = Binding<i32>::readOnly([]() {
    return 42;
});

// 创建双向绑定
Binding<i32> twoWayBinding = Binding<i32>::twoWay(
    []() { return volume.get(); },
    [](const i32& v) { volume.set(v); }
);

// 创建常量绑定
Binding<i32> constBinding = Binding<i32>::constant(100);
```

### 使用绑定

```cpp
Binding<i32> binding = Binding<i32>::fromReactive(volume);

// 获取值
i32 value = binding.get();

// 设置值（如果可写）
if (binding.isWritable()) {
    binding.set(50);
}

// 隐式转换
i32 v = binding;
```

## 状态绑定工具

`binding` 命名空间提供便捷的状态绑定工具。

```cpp
#include "kagero/state/StateBinding.hpp"

using namespace mc::client::ui::kagero::state::binding;
```

### bind 函数

```cpp
// 创建绑定点
StateBindingPoint point = bind<i32>("player.health");

// 获取和设置值
i32 health = point.get<i32>();
point.set(100);

// 绑定更新回调
u64 subId = point.bind([]() {
    // 状态变化时调用
});

// 解除绑定
point.unbind(subId);
```

### bindReactive 函数

```cpp
Reactive<i32> health(100);

// 创建双向同步
StateBindingPoint point = bindReactive(health, "player.health");

// 现在 Reactive 和 StateStore 自动同步
health.set(80);  // StateStore 也会更新
StateStore::instance().set<i32>("player.health", 60);  // Reactive 也会更新
```

### watch 函数

```cpp
// 监视状态变化（带旧值和新值）
u64 watchId = watch<i32>("player.health", [](const i32& oldVal, const i32& newVal) {
    std::cout << "Health: " << oldVal << " -> " << newVal << std::endl;
});

// 取消监视
unwatch(watchId);

// 监视多个状态
std::vector<u64> watchIds = watchAll({"player.health", "player.mana"}, []() {
    std::cout << "Health or mana changed" << std::endl;
});

// 取消多个监视
unwatchAll(watchIds);
```

## StateScope

自动管理订阅生命周期的作用域类。

```cpp
{
    StateScope scope;
    
    // 订阅自动添加到作用域
    scope.subscribe("player.health", []() {
        // 处理变化
    });
    
    scope.watch<i32>("player.mana", [](const i32& oldVal, const i32& newVal) {
        // 处理变化
    });
    
    // 离开作用域时自动取消所有订阅
}
```

## StateContext

组件级别的状态管理器。

```cpp
StateContext ctx;

// 获取和设置值（代理到 StateStore）
ctx.set<i32>("health", 100);
i32 health = ctx.get<i32>("health");

// 创建响应式状态（生命周期由 ctx 管理）
Reactive<i32>& mana = ctx.reactive<i32>("mana", 50);
mana.set(60);

// 获取作用域
StateScope& scope = ctx.scope();
```

## 观察者辅助类

### AutoObserver

自动管理观察者生命周期：

```cpp
Reactive<i32> health(100);

{
    AutoObserver<i32> observer(health, [](const i32& value) {
        std::cout << "Health: " << value << std::endl;
    });
    
    health.set(80);  // 输出: Health: 80
    
    // 离开作用域自动移除观察者
}
```

### MultiStateObserver

观察多个状态：

```cpp
Reactive<i32> health(100);
Reactive<i32> mana(50);

MultiStateObserver observer;
observer.observe(health);
observer.observe(mana);
observer.setCallback([]() {
    std::cout << "Some state changed" << std::endl;
});

health.set(80);  // 触发回调
mana.set(40);    // 触发回调

observer.clear();  // 清除所有观察
```

### DebouncedObserver

延迟触发观察者，避免频繁更新：

```cpp
Reactive<std::string> searchText("");

// 300ms 延迟
DebouncedObserver<std::string> observer(searchText, [](const std::string& text) {
    // 执行搜索
    performSearch(text);
}, 300);

searchText.set("h");
searchText.set("he");
searchText.set("hel");
searchText.set("hell");
searchText.set("hello");
// 只有在用户停止输入 300ms 后才执行搜索

// 每帧调用 update() 检查延迟
observer.update();

// 立即触发（跳过延迟）
observer.flush();
```

## Selector

状态选择器，从状态存储中选择和转换数据。

```cpp
// 创建选择器
Selector<i32> totalHealth([](const StateStore& store) {
    i32 base = store.get<i32>("player.baseHealth", 0);
    i32 bonus = store.get<i32>("player.healthBonus", 0);
    return base + bonus;
});

// 执行选择
i32 total = totalHealth.select();

// 订阅选择结果变化
u64 subId = totalHealth.subscribe([](const i32& value) {
    std::cout << "Total health: " << value << std::endl;
});
```

## 最佳实践

### 1. 状态路径命名规范

```cpp
// 使用点分隔的层次结构
"player.health"           // 玩家状态
"player.inventory[0]"     // 数组元素
"settings.audio.volume"   // 设置
"ui.dialog.visible"       // UI 状态

// 避免过于复杂的路径
// 不推荐: "game.world.entities[0].components.health.current"
```

### 2. 选择合适的状态容器

```cpp
// 全局共享状态 -> StateStore
StateStore::instance().set("game.paused", true);

// 组件局部状态 -> Reactive
Reactive<bool> expanded(false);

// 需要双向绑定 -> Binding
Binding<i32> sliderBinding = Binding<i32>::fromReactive(volume);
```

### 3. 避免循环依赖

```cpp
// 错误：循环依赖
Reactive<i32> a(10);
Reactive<i32> b(20);

a.observe([&b](const i32&, const i32& newVal) {
    b.set(newVal * 2);  // a 变化 -> b 变化
});

b.observe([&a](const i32&, const i32& newVal) {
    a.set(newVal / 2);  // b 变化 -> a 变化 -> 无限循环！
});
```

### 4. 使用 StateScope 管理生命周期

```cpp
class MyWidget : public Widget {
    StateScope m_stateScope;
    
    void init() override {
        m_stateScope.subscribe("player.health", [this]() {
            updateHealthDisplay();
        });
    }
    
    // 析构时自动取消订阅
};
```

### 5. 批量更新优化性能

```cpp
// 错误：多次触发通知
store.set("a", 1);
store.set("b", 2);
store.set("c", 3);
// 每次调用都触发订阅者

// 正确：批量更新
store.batchUpdate([](StateStore& store) {
    store.set("a", 1);
    store.set("b", 2);
    store.set("c", 3);
});
// 只在结束时触发一次通知
```

### 6. 使用 DebouncedObserver 处理频繁更新

```cpp
// 搜索输入框
Reactive<std::string> searchText;
DebouncedObserver<std::string> searchObserver(
    searchText, 
    [](const std::string& text) { performSearch(text); },
    300  // 300ms 延迟
);

// 在游戏循环中调用
void tick(f32 dt) {
    searchObserver.update();
}
```
