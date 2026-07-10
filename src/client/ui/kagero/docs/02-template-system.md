# 模板系统

Kagero 模板系统提供声明式的 UI 定义方式，支持零内联脚本、编译时验证和增量更新。

## 概述

模板系统包含以下核心组件：

```
template/
├── core/           # 核心配置和错误处理
│   ├── TemplateConfig.hpp
│   └── TemplateError.hpp
├── parser/         # 词法分析和语法分析
│   ├── Lexer.hpp
│   ├── Parser.hpp
│   └── Ast.hpp
├── binder/         # 绑定上下文
│   └── BindingContext.hpp
├── compiler/       # 模板编译器  
│   └── TemplateCompiler.hpp
├── runtime/        # 运行时实例
│   └── TemplateInstance.hpp
└── bindings/       # 内置组件和事件
    ├── BuiltinWidgets.hpp
    └── BuiltinEvents.hpp
```

## 基本用法

### 1. 初始化模板系统

```cpp
#include "kagero/template/Template.hpp"

// 在程序启动时调用一次
mc::client::ui::kagero::tpl::initializeTemplateSystem();
```

### 2. 编译模板

```cpp
#include "kagero/template/Template.hpp"
#include "kagero/template/compiler/TemplateCompiler.hpp"

using namespace mc::client::ui::kagero::tpl;

TemplateCompiler compiler;

// 启用严格模式（禁止动态特性）
compiler.setStrictMode(true);

// 编译模板字符串
auto compiled = compiler.compile(R"(
    <screen id="main">
        <text id="title" text="Welcome"/>
        <button id="btn_start" text="Start" on:click="onStart"/>
    </screen>
)");

// 检查编译错误
if (compiled->hasErrors()) {
    for (const auto& error : compiled->errors()) {
        std::cerr << "Error at line " << error.location.line 
                  << ": " << error.message << std::endl;
    }
    return;
}

// 从文件编译
auto compiledFromFile = compiler.compileFile("ui/main_screen.xml");
```

### 3. 创建绑定上下文

```cpp
#include "kagero/template/binder/BindingContext.hpp"
#include "kagero/state/StateStore.hpp"
#include "kagero/event/EventBus.hpp"

using namespace mc::client::ui::kagero::state;
using namespace mc::client::ui::kagero::event;
using namespace mc::client::ui::kagero::tpl::binder;

StateStore& store = StateStore::instance();
EventBus& eventBus = EventBus::instance();

BindingContext ctx(store, eventBus);
```

### 4. 暴露变量和回调

```cpp
// 暴露只读变量
std::string playerName = "Steve";
ctx.expose("player.name", &playerName);

// 暴露可写变量
i32 health = 100;
ctx.exposeWritable("player.health", &health);

// 暴露 Reactive 状态
Reactive<f32> volume(0.8f);
ctx.exposeReactive("settings.volume", volume);

// 暴露回调函数
ctx.exposeCallback("onStartGame", [](Widget* w, const Event& e) {
    std::cout << "Game started!" << std::endl;
});

// 暴露简单回调
ctx.exposeSimpleCallback("onCancel", []() {
    std::cout << "Cancelled" << std::endl;
});
```

### 5. 实例化模板

```cpp
#include "kagero/template/runtime/TemplateInstance.hpp"

using namespace mc::client::ui::kagero::tpl::runtime;

TemplateInstance instance(compiled.get(), ctx);

// 注册默认工厂
instance.registerDefaultFactories();
instance.registerDefaultAttributeSetters();
instance.registerDefaultEventBinders();

// 实例化 Widget 树
auto root = instance.instantiate();
if (root) {
    // 将 root 添加到屏幕
}
```

### 6. 更新绑定

```cpp
// 修改状态
playerName = "Alex";

// 通知变化
ctx.notifyChange("player.name");

// 更新所有绑定
instance.updateBindings();

// 或只更新特定路径
instance.updateBinding("player.name");
```

## 模板语法

### 基本结构

```xml
<screen id="main" width="800" height="600">
    <!-- 子元素 -->
</screen>
```

### 静态属性

```xml
<text id="title" 
      x="100" y="50" 
      text="Hello World" 
      color="#FFFFFF" 
      shadow="true"/>
```

### 绑定属性

使用 `bind:` 前缀绑定到状态：

```xml
<text id="playerName" bind:text="player.name"/>
<image bind:visible="player.alive"/>
```

> **注意**：当前版本仅支持简单路径绑定（如 `player.name`、`settings.volume`）。
> 表达式绑定（如 `bind:text="'Health: ' + player.health"`）暂不支持。
> 如需组合文本，请在 C++ 端预处理后暴露完整字符串。

### 事件绑定

使用 `on:` 前缀绑定事件处理：

```xml
<button id="btn_start" text="Start" on:click="onStartGame"/>
<text id="link" on:click="openLink('https://example.com')"/>
```

### 条件渲染

使用 `if:` 属性条件渲染：

```xml
<panel id="hud" if:condition="game.isPlaying">
    <text bind:text="player.health"/>
</panel>
```

### 循环渲染

使用 `for:` 属性循环渲染：

```xml
<list id="inventory">
    <slot for:item="slot in player.inventory" 
          bind:item="slot.item"/>
</list>
```

### 样式属性

```xml
<button id="btn"
        width="200" height="40"
        margin="10,5" padding="5"
        background-color="#404040"
        border-color="#606060"
        corner-radius="4"/>
```

### Flex 布局

容器组件支持 Flex 弹性布局，可自动排列子元素：

```xml
<!-- 垂直排列按钮，间距 10px -->
<container id="menu" layout="flex-column" gap="10" align-items="center" size="200,120">
    <button id="btn1" text="Option A" size="200,20"/>
    <button id="btn2" text="Option B" size="200,20"/>
    <button id="btn3" text="Option C" size="200,20"/>
</container>
```

**布局类型** (`layout` 属性)：

| 值 | 描述 |
|----|------|
| `flex` / `flex-row` | 水平排列（从左到右） |
| `flex-column` | 垂直排列（从上到下） |
| `flex-row-reverse` | 水平反向排列（从右到左） |
| `flex-column-reverse` | 垂直反向排列（从下到上） |
| `flex-center` | 垂直居中排列（Column + Center justify + Center align） |
| `grid` | 网格布局 |
| `anchor` | 锚点布局 |

**Flex 子属性**：

| 属性 | 描述 | 值 |
|------|------|-----|
| `gap` | 子元素间距 | 整数（像素），如 `10` |
| `align-items` | 交叉轴对齐 | `start` / `center` / `end` / `stretch` / `baseline` |
| `justify-content` | 主轴对齐 | `start` / `center` / `end` / `space-between` / `space-around` / `space-evenly` |

**Grid 子属性**（仅 `<grid>` 标签）：

| 属性 | 描述 | 值 | 默认 / 钳制 |
|------|------|-----|-------------|
| `cols` | 列数 | 整数 | 默认 `1`；非数字回退到 `1`；<1 钳制为 `1` |
| `rows` | 行数 | 整数 | 默认 `0`（自动推算为 `ceil(childCount/cols)`） |
| `gap` | 同时设置列间距与行间距 | 整数（像素） | 默认 `0`；<0 钳制为 `0` |

```xml
<!-- 9 列 3 行、间距 2px 的物品栏网格 -->
<grid id="inventory" cols="9" rows="3" gap="2" pos="10,10" size="178,70">
    <slot id="slot_0" index="0" size="18,18"/>
    <!-- ... -->
</grid>
```

**screen 默认布局**：`<screen>` 标签默认使用 `flex-center` 布局（垂直居中排列），无需手动设置。

### 百分比尺寸

`size` 和 `pos` 属性支持百分比值，相对于父容器尺寸计算：

```xml
<!-- 占满父容器 -->
<container id="overlay" size="100%,100%" background-color="#B4000000"/>

<!-- 居中且宽度为父容器的 50% -->
<container id="panel" size="50%,80%"/>
```

- 像素值和百分比值可混合使用：`size="50%,200"`
- 百分比在模板实例化后、父容器尺寸已知时解析

## 内置组件

模板系统支持以下内置组件标签：

| 标签名 | 描述 |
|--------|------|
| `screen` | 屏幕容器 |
| `container` | 通用容器 |
| `text` | 文本显示 |
| `button` | 按钮 |
| `image` | 图片 |
| `textfield` | 文本输入框 |
| `slider` | 滑块 |
| `checkbox` | 复选框 |
| `list` | 列表容器 |
| `scrollable` | 可滚动容器 |
| `slot` | 物品槽 |
| `viewport3d` | 3D 视口 |
| `grid` | 网格容器 |

### 组件示例

```xml
<!-- 文本 -->
<text id="title" text="Title" color="#FFFFFF" shadow="true" 
      x="10" y="10" width="200" height="30"/>

<!-- 按钮 -->
<button id="btn" text="Click Me" 
        x="10" y="50" width="120" height="24"
        on:click="onButtonClick"/>

<!-- 文本输入框 -->
<textfield id="input" placeholder="Enter text..."
           x="10" y="90" width="200" height="24"
           max-length="32"
           on:change="onTextChanged"/>

<!-- 滑块 -->
<slider id="volume" min="0" max="100" value="80"
        x="10" y="130" width="200" height="20"
        on:change="onVolumeChanged"/>

<!-- 复选框 -->
<checkbox id="option" text="Enable Feature"
          x="10" y="170" checked="false"
          on:change="onOptionChanged"/>
```

## 内置事件

| 事件名 | 描述 |
|--------|------|
| `click` | 鼠标点击 |
| `doubleClick` | 双击 |
| `rightClick` | 右键点击 |
| `mouseEnter` | 鼠标进入 |
| `mouseLeave` | 鼠标离开 |
| `scroll` | 滚轮滚动 |
| `keyDown` | 键盘按下 |
| `keyUp` | 键盘释放 |
| `focus` | 获得焦点 |
| `blur` | 失去焦点 |
| `change` | 值变化 |
| `input` | 输入变化 |

## 绑定上下文 API

### Value 类型

`Value` 类用于运行时存储任意类型的值：

```cpp
Value v1(true);           // 布尔值
Value v2(42);             // 整数
Value v3(3.14f);          // 浮点数
Value v4("hello");        // 字符串

// 类型检查
v1.isBool();    // true
v2.isInteger(); // true
v3.isFloat();   // true
v4.isString();  // true

// 类型转换
i32 i = v2.toInteger();
f32 f = v3.toFloat();
bool b = v1.toBool();
std::string s = v4.asString();
```

### BindingContext 方法

```cpp
// 暴露变量
ctx.expose("key", &variable);
ctx.exposeWritable("key", &variable);
ctx.exposeReactive("key", reactiveVar);

// 暴露回调
ctx.exposeCallback("name", callback);
ctx.exposeSimpleCallback("name", callback);

// 解析绑定
Value value = ctx.resolveBinding("player.name");

// 设置绑定
ctx.setBinding("player.name", Value("Alex"));

// 检查路径
bool exists = ctx.hasPath("player.name");
bool writable = ctx.isWritable("player.name");

// 订阅变化
u64 subId = ctx.subscribe("player.name", [](const std::string& path, const Value& newValue) {
    // 处理变化
});

// 取消订阅
ctx.unsubscribe(subId);

// 循环变量
ctx.setLoopVariable("item", itemValue);
ctx.clearLoopVariable("item");
```

### 绑定路径解析规则

`resolveBinding(path)` 按以下顺序解析路径：

1. **循环变量**（`$var` 或 `$var.field`）：优先匹配当前 `loopVar`/`loopValue`，未命中再查 `m_loopVariables` 表。
2. **暴露变量**（`expose` / `exposeWritable` / `exposeReactive`）：路径完全匹配 `m_exposedVars` 中的键。
3. **StateStore 扁平键**：路径完全匹配 StateStore 中的键（如 `"player.name"`），直接通过 `Value::fromAny` 转换后返回。
4. **StateStore 嵌套路径**：以 `.` 与 `[index]` 拆分路径，第一段作为根键查 StateStore，根键的 `std::any` 通过 `Value::fromAny` 转为 `Value`，再逐层 `getProperty` / `getElement`。

**嵌套路径示例：**

```cpp
// 在 StateStore 中存对象
std::unordered_map<std::string, Value> player;
player["name"] = Value("Steve");
player["health"] = Value(100);
StateStore::instance().set("player", player);

// 模板中可以直接用嵌套路径绑定
// <text bind:text="player.name"/>
// <text bind:text="player.health"/>

// 数组同理
std::vector<Value> inventory = { Value("sword"), Value("apple") };
StateStore::instance().set("player.inventory", inventory);
// <text bind:text="player.inventory[0]"/>
```

> **注意**：StateStore 中同时存在 `"player"` 与 `"player.name"` 时，`resolveBinding("player.name")` 会命中扁平键 `"player.name"`，跳过嵌套解析。两种写法不可混用。
>
> **Value::fromAny 支持的类型**：`bool`、`i32`、`i64`、`u32`、`f32`、`f64`、`std::string`、`const char*`、`Value`、`std::vector<Value>`、`std::unordered_map<std::string, Value>`。其他类型静默返回 `Null`。`i64`/`u32`/`f64` 会被窄化（代码中已标注 TODO，待 Value 类增加原生支持后消除）。

详细行为见 `template/binder/README.md`。

## 更新调度器

`UpdateScheduler` 管理增量更新，避免频繁刷新整个模板。
`TemplateInstance` 内部持有一个 `UpdateScheduler` 实例，通过回调驱动 `updateBinding(path)`：

```cpp
// TemplateInstance 内部已自动配置调度器回调
// 外部通过 instance.scheduler() 可访问调度器进行配置
auto& scheduler = instance.scheduler();

// 配置
scheduler.setBatchDelay(16);    // 批量更新延迟（毫秒）
scheduler.setMaxBatchSize(100); // 最大批量大小
scheduler.setDeferredUpdate(true);

// 状态变更通知：入队路径，由调度器按延迟/优先级统一执行
instance.notifyStateChange("player.health");

// 每帧推进调度器（由 TemplateScreen::tick 自动调用）
instance.tick(currentMs);

// 立即执行所有待处理任务（无视延迟，用于屏幕关闭/强制刷新）
instance.flushPending();

// 全量刷新绑定（立即执行待处理任务 + 重新应用所有绑定计划）
instance.refresh();
```

### 时间模型与延迟语义

- 外部通过 `tick(currentMs)` 推进调度器内部时钟（毫秒级时间戳，通常来自 `util::TimeUtils::getCurrentTimeMs()`）
- `schedule()` 时根据 `m_deferredUpdate` 与 `m_batchDelayMs` 计算任务到期时间 `dueTimeMs`
- `executePending()` / `tick()` 只执行 `dueTimeMs <= 当前时间` 的任务
- `flush()` 无视延迟立即执行所有待处理任务（用于屏幕关闭、强制刷新等场景）

### 批量延迟语义（trailing-edge debounce）

- 同一路径的多次 `schedule` 会被 `_deduplicatePaths()` 合并，只保留最新任务
- 启用 `deferredUpdate` 时，每次 `schedule` 会把该路径的到期时间推迟 `now + batchDelayMs`
- 这意味着连续高频更新只会被处理一次（最后一次），避免重复刷新

## 最佳实践

### 1. 模板组织

将模板存储在独立文件中：

```
resources/ui/
├── screens/
│   ├── main_menu.xml
│   ├── pause_menu.xml
│   └── inventory.xml
├── widgets/
│   ├── button.xml
│   └── slot.xml
└── styles/
    └── common.xml
```

### 2. 状态路径命名

使用清晰的状态路径命名：

```cpp
// 推荐
"player.health"         // 玩家生命值
"player.inventory[0]"   // 玩家物品槽
"settings.volume"       // 音量设置
"ui.dialog.visible"     // 对话框可见性

// 不推荐
"h"                     // 太短
"getPlayerHealth"       // 不应使用函数名
```

### 3. 严格模式

生产环境启用严格模式：

```cpp
TemplateCompiler compiler;
compiler.setStrictMode(true);  // 禁止所有动态特性
```

### 4. 错误处理

始终检查编译错误：

```cpp
auto compiled = compiler.compile(source);
if (compiled->hasErrors()) {
    for (const auto& error : compiled->errors()) {
        // 记录或显示错误
    }
    return;
}
```

### 5. 性能优化

- 使用 `UpdateScheduler` 进行批量更新
- 避免频繁调用 `updateBindings()`
- 使用 `updateBinding(path)` 更新特定绑定
- 利用 `notifyChange()` 只更新变化的状态

## 实现限制

### 当前版本限制

以下功能在当前版本中尚未完全实现：

| 功能 | 状态 | 说明 |
|------|------|------|
| 表达式绑定 | 暂不支持 | `bind:text="'Health: ' + player.health"` 需在 C++ 端预处理 |
| 计算属性 | 暂不支持 | 无法定义派生状态 |
| 样式属性部分功能 | 部分支持 | `margin`、`padding`、`background-color`、`border-color`、`corner-radius` 已注册 setter 但 Widget 类尚无对应属性 |
| `change`/`input` 事件 | 部分支持 | 已注册事件处理器，但 `TextFieldWidget`、`SliderWidget`、`CheckboxWidget` 需实现对应事件触发 |

### 样式属性支持状态

| 属性 | 解析 | Widget 属性 | 备注 |
|------|------|-------------|------|
| `x` | ✅ | ✅ | 位置 X 坐标 |
| `y` | ✅ | ✅ | 位置 Y 坐标 |
| `width` | ✅ | ✅ | 宽度 |
| `height` | ✅ | ✅ | 高度 |
| `text` | ✅ | ✅ | 文本内容 |
| `color` | ✅ | ✅ | 文本颜色 |
| `visible` | ✅ | ✅ | 可见性 |
| `enabled` | ✅ | ✅ | 启用状态 |
| `shadow` | ✅ | ✅ | 文本阴影 |
| `scale` | ✅ | ✅ | 缩放 |
| `align` | ✅ | ✅ | 文本对齐 |
| `id` | ✅ | ✅ | 元素标识 |
| `margin` | ✅ | ⏳ | 已解析，待 Widget 实现 |
| `padding` | ✅ | ⏳ | 已解析，待 Widget 实现 |
| `background-color` | ✅ | ⏳ | 已解析，待 Widget 实现 |
| `border-color` | ✅ | ⏳ | 已解析，待 Widget 实现 |
| `corner-radius` | ✅ | ⏳ | 已解析，待 Widget 实现 |
| `disabled` | ✅ | ✅ | 禁用状态（等价于 `enabled="false"`）|
| `checked` | ✅ | ✅ | 复选框选中状态 |
| `value` | ✅ | ✅ | 滑块/输入框值 |
| `min`/`max` | ✅ | ✅ | 滑块范围 |
| `placeholder` | ✅ | ✅ | 输入框占位文本 |
| `max-length` | ✅ | ✅ | 输入最大长度 |
| `pos` | ✅ | ✅ | 位置（支持百分比如 "50%,0"） |
| `size` | ✅ | ✅ | 尺寸（支持百分比如 "100%,100%"） |
| `layout` | ✅ | ✅ | 布局类型（flex-column 等） |
| `gap` | ✅ | ✅ | Flex 子元素间距 |
| `align-items` | ✅ | ✅ | 交叉轴对齐 |
| `justify-content` | ✅ | ✅ | 主轴对齐 |
| `background-color` | ✅ | ✅ | 背景色（screen 和 container 均支持） |

### Widget 特定事件支持

| 事件 | 通用支持 | Widget 特定触发 |
|------|----------|-----------------|
| `click` | ✅ | ✅ |
| `doubleClick` | ✅ | ✅ |
| `rightClick` | ✅ | ✅ |
| `mouseEnter` | ✅ | ✅ |
| `mouseLeave` | ✅ | ✅ |
| `scroll` | ✅ | ✅ |
| `keyDown` | ✅ | ✅ |
| `keyUp` | ✅ | ✅ |
| `focus` | ✅ | ✅ |
| `blur` | ✅ | ✅ |
| `change` | ✅ | ⏳ | 需 Widget 实现值变化检测 |
| `input` | ✅ | ⏳ | 需 TextFieldWidget 实现输入检测 |

### 未来计划

以下功能计划在后续版本中实现：

1. **表达式绑定**：支持简单的算术和字符串运算
2. **计算属性**：支持基于其他状态自动计算的属性
3. **双向绑定**：`bind双向:text="player.name"` 自动同步输入
4. **条件表达式**：`if:condition="player.health > 50"`
5. **样式系统**：完整的外边距、内边距、边框、圆角支持
