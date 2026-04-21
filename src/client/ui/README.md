# src/client/ui - 用户界面模块

本目录包含 Minecraft Reborn 客户端的完整用户界面系统，包括字体渲染、Kagero UI 框架和 Minecraft 特定的 UI 组件。

## 目录结构

```
ui/
├── Font.hpp                          # 字体类定义
├── Font.cpp                          # 字体实现
├── FontRenderer.hpp                  # 文本渲染器
├── FontRenderer.cpp                  # 文本渲染实现
├── FontTextureAtlas.hpp              # 字形纹理图集
├── FontTextureAtlas.cpp              # 纹理图集实现
├── Glyph.hpp                         # 字形数据结构
├── DefaultAsciiFont.hpp              # 默认ASCII位图字体
├── DefaultAsciiFont.cpp              # 默认字体实现
├── TridentCanvas.hpp                 # Trident渲染器画布实现
├── TridentCanvas.cpp                 # 画布实现
├── kagero/                           # Kagero UI框架
│   ├── KageroEngine.hpp              # UI引擎核心
│   ├── KageroEngine.cpp              # 引擎实现
│   ├── Types.hpp                     # 基础类型定义
│   ├── README.md                     # 框架文档
│   ├── docs/                         # 详细文档
│   │   ├── README.md                 # 文档索引
│   │   ├── 01-quick-start.md         # 快速开始
│   │   ├── 02-template-system.md     # 模板系统
│   │   ├── 03-state-system.md        # 状态系统
│   │   ├── 04-event-system.md        # 事件系统
│   │   └── 05-built-in-widgets.md    # 内置组件
│   ├── event/                        # 事件系统
│   │   ├── Event.hpp                 # 事件基类
│   │   ├── EventBus.hpp              # 事件总线
│   │   ├── InputEvents.hpp           # 输入事件
│   │   ├── UIEvents.hpp              # UI事件
│   │   └── WidgetEvents.hpp          # 组件事件
│   ├── state/                        # 状态管理
│   │   ├── ReactiveState.hpp         # 响应式状态
│   │   ├── StateBinding.hpp          # 状态绑定
│   │   ├── StateObserver.hpp         # 状态观察者
│   │   └── StateStore.hpp            # 全局状态存储
│   ├── layout/                       # 布局系统
│   │   ├── LayoutSystem.hpp          # 布局系统入口
│   │   ├── core/                     # 核心组件
│   │   │   ├── LayoutEngine.hpp      # 布局引擎
│   │   │   ├── LayoutResult.hpp      # 布局结果
│   │   │   └── MeasureSpec.hpp       # 测量规格
│   │   ├── algorithms/               # 布局算法
│   │   │   ├── FlexLayout.hpp        # 弹性布局
│   │   │   ├── GridLayout.hpp        # 网格布局
│   │   │   └── AnchorLayout.hpp      # 锚点布局
│   │   ├── constraints/              # 布局约束
│   │   │   └── LayoutConstraints.hpp
│   │   └── integration/              # Widget集成
│   │       ├── WidgetLayoutAdaptor.hpp
│   │       └── WidgetLayoutAdaptor.cpp
│   ├── paint/                        # 绘制抽象层
│   │   ├── Color.hpp                 # 颜色定义
│   │   ├── Geometry.hpp              # 几何类型
│   │   ├── PaintContext.hpp          # 绘制上下文
│   │   ├── PaintContext.cpp
│   │   ├── TextureImage.hpp          # 纹理图像
│   │   ├── TextureImage.cpp
│   │   └── contracts/                # 接口定义
│   │       ├── ICanvas.hpp           # 画布接口
│   │       ├── IImage.hpp            # 图像接口
│   │       ├── IPaint.hpp            # 画笔接口
│   │       ├── IPath.hpp             # 路径接口
│   │       ├── ISurface.hpp          # 绘图表面接口
│   │       ├── ITextBlob.hpp         # 文本块接口
│   │       └── ITypeface.hpp         # 字体接口
│   ├── template/                     # 模板系统
│   │   ├── Template.hpp              # 模板入口
│   │   ├── TemplateSystem.hpp        # 模板系统API
│   │   ├── TemplateSystem.cpp
│   │   ├── parser/                   # 解析器
│   │   │   ├── Lexer.hpp             # 词法分析
│   │   │   ├── Lexer.cpp
│   │   │   ├── Parser.hpp            # 语法分析
│   │   │   ├── Parser.cpp
│   │   │   ├── Ast.hpp               # 抽象语法树
│   │   │   ├── Ast.cpp
│   │   │   ├── AstVisitor.hpp        # AST访问者
│   │   │   └── AstVisitor.cpp
│   │   ├── compiler/                 # 编译器
│   │   │   ├── TemplateCompiler.hpp
│   │   │   └── TemplateCompiler.cpp
│   │   ├── binder/                   # 绑定上下文
│   │   │   ├── BindingContext.hpp
│   │   │   └── BindingContext.cpp
│   │   ├── runtime/                  # 运行时
│   │   │   ├── TemplateInstance.hpp
│   │   │   ├── TemplateInstance.cpp
│   │   │   ├── UpdateScheduler.hpp
│   │   │   └── UpdateScheduler.cpp
│   │   ├── core/                     # 核心配置
│   │   │   ├── TemplateConfig.hpp
│   │   │   └── TemplateError.hpp
│   │   └── bindings/                 # 内置绑定
│   │       ├── BuiltinWidgets.hpp
│   │       ├── BuiltinWidgets.cpp
│   │       ├── BuiltinEvents.hpp
│   │       └── BuiltinEvents.cpp
│   └── widget/                       # Widget组件
│       ├── Widget.hpp                # Widget基类
│       ├── ButtonWidget.hpp          # 按钮组件
│       ├── CheckboxWidget.hpp        # 复选框组件
│       ├── ContainerWidget.hpp       # 容器组件
│       ├── ContainerWidget.cpp
│       ├── IWidgetContainer.hpp      # 容器接口
│       ├── ListWidget.hpp            # 列表组件
│       ├── ScrollableWidget.hpp      # 滚动容器
│       ├── SliderWidget.hpp          # 滑块组件
│       ├── SlotWidget.hpp            # 物品槽组件
│       ├── TextFieldWidget.hpp       # 文本输入框
│       ├── TextWidget.hpp            # 文本组件
│       └── Viewport3DWidget.hpp      # 3D视口组件
├── minecraft/                        # Minecraft特定UI
│   ├── MinecraftUIContext.hpp        # Minecraft UI上下文
│   ├── MinecraftUIContext.cpp
│   ├── resources/                    # UI资源
│   │   ├── MinecraftTypeface.hpp     # Minecraft字体
│   │   ├── MinecraftTypeface.cpp
│   │   ├── ResourceProvider.hpp      # 资源提供者
│   │   └── ResourceProvider.cpp
│   ├── screens/                      # 游戏屏幕
│   │   ├── README.md
│   │   ├── Screen.hpp                # 屏幕基类
│   │   ├── Screen.cpp
│   │   ├── ScreenManager.hpp         # 屏幕管理器
│   │   ├── ScreenManager.cpp
│   │   ├── AbstractContainerScreen.hpp # 容器屏幕基类
│   │   ├── ContainerScreen.hpp       # 容器屏幕
│   │   ├── ContainerScreen.cpp
│   │   ├── InventoryScreen.hpp       # 背包屏幕
│   │   ├── InventoryScreen.cpp
│   │   ├── MainMenuScreen.hpp        # 主菜单屏幕
│   │   ├── MainMenuScreen.cpp
│   │   ├── OptionsScreen.hpp         # 选项屏幕
│   │   ├── OptionsScreen.cpp
│   │   ├── PauseScreen.hpp           # 暂停屏幕
│   │   ├── PauseScreen.cpp
│   │   ├── DebugScreenWidget.hpp     # 调试屏幕组件
│   │   ├── DebugScreenWidget.cpp
│   │   ├── CraftingScreen.hpp        # 合成屏幕
│   │   ├── CraftingScreen.cpp
│   │   └── CreativeScreen.hpp/cpp    # 创造模式物品库屏幕
│   ├── targetinfo/                   # 准星目标信息覆盖层
│   │   ├── TargetInfo.hpp            # 目标快照和格式化辅助
│   │   ├── TargetInfoResolver.hpp    # 方块/实体命中解析
│   │   └── TargetInfoWidget.hpp      # HUD 目标提示 Widget
│   ├── templates/                    # UI模板
│   │   ├── main_menu.tpl             # 主菜单模板
│   │   ├── pause_menu.tpl            # 暂停菜单模板
│   │   ├── options.tpl               # 选项模板
│   │   └── inventory.tpl             # 背包模板
│   └── widgets/                      # Minecraft特定组件
│       ├── ChatWidget.hpp            # 聊天组件
│       ├── ChatWidget.cpp
│       ├── CrosshairWidget.hpp       # 准星组件
│       ├── CrosshairWidget.cpp
│       ├── ExperienceBar.hpp         # 经验条
│       ├── ExperienceBar.cpp
│       ├── HealthBarWidget.hpp       # 生命值条
│       ├── HealthBarWidget.cpp
│       ├── HotbarWidget.hpp          # 快捷栏
│       ├── HotbarWidget.cpp
│       ├── HudWidget.hpp             # HUD组件
│       ├── HudWidget.cpp
│       ├── HungerBarWidget.hpp       # 饥饿值条
│       ├── HungerBarWidget.cpp
│       ├── InventorySlot.hpp         # 背包槽位
│       ├── InventorySlot.cpp
│       ├── ScreenStackWidget.hpp     # 屏幕栈组件
│       ├── ScreenStackWidget.cpp
│       ├── SlotWidget.hpp            # 槽位组件
│       ├── SlotWidget.cpp
│       ├── Viewport3DWidget.hpp      # 3D视口
│       └── Viewport3DWidget.cpp
└── screen/                           # 屏幕系统（旧版兼容）
    ├── AbstractContainerScreen.hpp
    ├── CraftingScreen.hpp
    ├── CraftingScreen.cpp
    ├── ScreenManager.hpp
    └── ScreenManager.cpp
```

## 模块说明

### 1. 字体系统 (`Font*.hpp/cpp`, `Glyph.hpp`)

字体系统负责文本渲染，参考 Minecraft 的字体实现。

**核心组件：**

| 文件 | 职责 |
|------|------|
| `Glyph.hpp` | 字形数据结构，包含UV坐标、度量数据、`GuiVertex`顶点结构 |
| `Font.hpp/cpp` | 字体类，管理字形纹理图集和字形缓存 |
| `FontRenderer.hpp/cpp` | 文本渲染器，支持阴影、粗体、斜体、颜色、UTF-8 |
| `FontTextureAtlas.hpp/cpp` | 字形纹理图集，使用二叉树打包算法动态打包字形 |
| `DefaultAsciiFont.hpp/cpp` | 内置ASCII位图字体（5x7点阵），用于调试 |

**字形提供者接口：**
- `IGlyphProvider`: 字形加载抽象接口
- `BitmapGlyphProvider`: Minecraft格式位图字体加载器

**文本样式：**
- 阴影、粗体、斜体、删除线、下划线
- 颜色支持（ARGB格式，包含MC聊天颜色常量）

### 2. Kagero UI 框架 (`kagero/`)

Kagero（陽炎）是一个现代化的 UI 框架，采用声明式模板、响应式状态管理和组件化架构。

**命名空间：** `mc::client::ui::kagero`

#### 2.1 核心引擎 (`KageroEngine.hpp/cpp`)

统一管理所有UI组件的渲染、更新和事件分发。采用分层架构，每层是一个Widget。

```cpp
// 使用示例
auto engine = std::make_unique<KageroEngine>();
engine->initialize(*canvas, {1920, 1080});
engine->addLayer(std::make_unique<CrosshairWidget>(), 0);
engine->addLayer(std::make_unique<HudWidget>(), 10);
engine->render();
engine->update(deltaTime);
```

#### 2.2 Widget 系统 (`widget/`)

所有UI组件的基类，提供：
- 生命周期管理（`init`, `tick`, `paint`）
- 事件处理（`onClick`, `onDrag`, `onScroll`, `onKey`, `onChar`）
- 布局属性（`position`, `size`, `anchor`）
- 状态管理（`visible`, `active`, `hovered`, `focused`）

**内置组件：**

| 组件 | 说明 |
|------|------|
| `Widget` | 基类组件 |
| `ContainerWidget` | 容器组件，支持子组件管理 |
| `TextWidget` | 文本显示，支持对齐、阴影、换行 |
| `ButtonWidget` | 按钮组件，支持点击回调、悬停提示、禁用状态 |
| `ImageButtonWidget` | 图片按钮 |
| `SliderWidget` | 滑块组件，支持拖动、滚轮、键盘控制 |
| `IntSliderWidget` | 整数滑块 |
| `TextFieldWidget` | 文本输入框，支持选择、复制粘贴、验证 |
| `CheckboxWidget` | 复选框 |
| `ListWidget` | 列表组件 |
| `ScrollableWidget` | 滚动容器 |
| `SlotWidget` | 物品槽组件 |
| `Viewport3DWidget` | 3D视口组件 |

#### 2.3 事件系统 (`event/`)

类型安全的事件分发系统。

```cpp
// 订阅事件
auto id = EventBus::instance().subscribe<ClickEvent>([](const ClickEvent& e) {
    // 处理点击事件
});

// 发布事件
ClickEvent event(100, 200, 0);
EventBus::instance().publish(event);

// 取消订阅
EventBus::instance().unsubscribe(id);
```

**事件类型：**
- 输入事件：`MouseClick`, `MouseRelease`, `MouseDrag`, `MouseScroll`, `KeyPress`, `CharInput`
- 焦点事件：`FocusGained`, `FocusLost`
- 值变化事件：`ValueChange`, `TextChange`
- 组件事件：`WidgetInit`, `WidgetResize`, `WidgetShow`, `WidgetHide`

#### 2.4 状态管理 (`state/`)

响应式状态管理，自动追踪状态变化并更新UI。

```cpp
// 响应式状态
Reactive<i32> count(0);
count.observe([](i32 oldValue, i32 newValue) {
    std::cout << "Count changed from " << oldValue << " to " << newValue << std::endl;
});
count.set(10); // 触发观察者

// 全局状态存储
StateStore& store = StateStore::instance();
store.set("playerHealth", 20);
i32 health = store.get<i32>("playerHealth");

// 双向绑定
Binding<i32> binding = Binding<i32>::fromReactive(count);
```

**核心类：**
- `Reactive<T>`: 响应式包装器
- `StateStore`: 全局状态存储（单例）
- `Binding<T>`: 双向绑定
- `Computed<T>`: 计算属性

#### 2.5 布局系统 (`layout/`)

类似 CSS Flexbox 的现代布局系统。

```cpp
// Flex布局配置
FlexConfig config;
config.direction = Direction::Row;
config.justifyContent = JustifyContent::Center;
config.alignItems = Align::Center;
config.gap = 10;

// 使用布局引擎
auto& engine = LayoutEngine::instance();
engine.layoutFlex(containerAdaptor, Rect(0, 0, 800, 600), config);
```

**支持布局：**
- `FlexLayout`: 弹性布局（类似Flexbox）
- `GridLayout`: 网格布局
- `AnchorLayout`: 锚点布局

#### 2.6 模板系统 (`template/`)

声明式XML风格模板语法。

```xml
<screen id="main" width="800" height="600">
    <text id="title" x="300" y="50" text="Hello Kagero!"/>
    <button id="btn_start" x="300" y="200" width="200" height="40"
            text="Start Game" on:click="onStart"/>
</screen>
```

**模板系统组件：**
- `Lexer`: 词法分析器
- `Parser`: 语法分析器
- `Ast`: 抽象语法树
- `TemplateCompiler`: 模板编译器
- `TemplateInstance`: 模板实例化
- `BindingContext`: 绑定上下文

#### 2.7 绘制抽象层 (`paint/`)

平台无关的绘图接口，类似 Chromium 的 Skia。

**核心接口：**
- `ICanvas`: 画布接口（绘制矩形、圆形、路径、文本、图像）
- `IPaint`: 画笔接口（颜色、样式）
- `IImage`: 图像接口
- `IPath`: 路径接口
- `ITextBlob`: 文本块接口
- `ITypeface`: 字体接口

**实现：**
- `TridentCanvas`: Vulkan渲染器实现

### 3. Minecraft UI (`minecraft/`)

Minecraft 特定的UI实现。

#### 3.1 MinecraftUIContext

提供Minecraft UI系统的业务逻辑支持：
- 状态绑定（玩家生命值、饥饿值等）
- 事件绑定（点击、关闭等）
- 资源管理（纹理图集、字体等）

#### 3.2 屏幕系统 (`screens/`)

| 屏幕 | 说明 |
|------|------|
| `Screen` | 屏幕基类 |
| `ScreenManager` | 屏幕栈管理 |
| `MainMenuScreen` | 主菜单 |
| `PauseScreen` | 暂停菜单 |
| `OptionsScreen` | 选项屏幕 |
| `InventoryScreen` | 背包屏幕 |
| `ContainerScreen` | 容器屏幕 |
| `CraftingScreen` | 合成屏幕 |
| `CreativeScreen` | 创造模式物品库屏幕 |
| `DebugScreenWidget` | 调试信息（F3屏幕） |

#### 3.3 HUD组件 (`widgets/`)

| 组件 | 说明 |
|------|------|
| `HudWidget` | HUD主组件，渲染快捷栏、生命值、饥饿值、经验条 |
| `HotbarWidget` | 快捷栏 |
| `HealthBarWidget` | 生命值条 |
| `HungerBarWidget` | 饥饿值条 |
| `ExperienceBar` | 经验条 |
| `CrosshairWidget` | 准星 |
| `ChatWidget` | 聊天组件 |
| `InventorySlot` | 背包槽位 |
| `SlotWidget` | 槽位组件 |
| `ScreenStackWidget` | 屏幕栈组件 |

## 模块关系图

```
┌─────────────────────────────────────────────────────────────────┐
│                         ClientApplication                        │
└─────────────────────────────────────────────────────────────────┘
                                 │
                                 ▼
┌─────────────────────────────────────────────────────────────────┐
│                          ScreenManager                           │
│                    (屏幕栈管理、事件分发)                          │
└─────────────────────────────────────────────────────────────────┘
                                 │
              ┌──────────────────┼──────────────────┐
              ▼                  ▼                  ▼
┌──────────────────┐ ┌──────────────────┐ ┌──────────────────┐
│  MinecraftUIContext │ │    KageroEngine   │ │   FontRenderer   │
│  (MC业务绑定)       │ │   (UI引擎核心)     │ │   (文本渲染)      │
└──────────────────┘ └──────────────────┘ └──────────────────┘
              │                  │                  │
              ▼                  ▼                  ▼
┌──────────────────┐ ┌──────────────────┐ ┌──────────────────┐
│    Templates      │ │     Widgets       │ │      Font        │
│   (UI模板)         │ │    (UI组件)        │ │   (字体系统)      │
└──────────────────┘ └──────────────────┘ └──────────────────┘
              │                  │                  │
              ▼                  ▼                  ▼
┌──────────────────┐ ┌──────────────────┐ ┌──────────────────┐
│ BindingContext    │ │    State/Event    │ │ FontTextureAtlas │
│   (绑定上下文)      │ │  (状态/事件系统)    │ │   (字形图集)      │
└──────────────────┘ └──────────────────┘ └──────────────────┘
                                 │
                                 ▼
                     ┌──────────────────┐
                     │   TridentCanvas   │
                     │  (Vulkan绘制实现)   │
                     └──────────────────┘
                                 │
                                 ▼
                     ┌──────────────────┐
                     │   GuiRenderer     │
                     │  (GUI渲染器)       │
                     └──────────────────┘
```

## 整体职责

1. **字体渲染**：加载位图字体，动态打包字形纹理，渲染带样式的文本
2. **UI框架**：提供声明式模板、响应式状态、事件分发、Flex布局
3. **组件库**：提供按钮、文本、滑块、输入框等可复用组件
4. **Minecraft UI**：实现游戏特定的HUD、屏幕、菜单

## 输入和输出

### 输入

| 输入类型 | 来源 | 说明 |
|----------|------|------|
| 用户输入 | GLFW | 鼠标点击、移动、滚轮、键盘输入 |
| 状态数据 | 游戏逻辑 | 玩家生命值、饥饿值、经验、物品栏 |
| 模板文件 | `minecraft/templates/*.tpl` | XML风格的UI定义 |
| 资源文件 | 资源包 | 字体纹理、GUI纹理 |

### 输出

| 输出类型 | 目标 | 说明 |
|----------|------|------|
| 顶点数据 | GuiRenderer | GUI顶点缓冲 |
| 事件回调 | 游戏逻辑 | 按钮点击、值变化等 |
| 状态更新 | 游戏逻辑 | 玩家操作结果 |

## 依赖项

### 外部依赖

| 库 | 用途 |
|-----|------|
| `GLFW` | 窗口和输入 |
| `Vulkan` | 图形渲染 |
| `spdlog` | 日志 |

### 内部依赖

| 模块 | 用途 |
|------|------|
| `common/core` | 基础类型、错误处理 |
| `common/resource` | 资源加载 |
| `client/renderer/api` | 渲染接口 |
| `client/renderer/trident/gui` | GUI渲染器 |

## 使用方法

### 1. 初始化字体系统

```cpp
#include "client/ui/Font.hpp"
#include "client/ui/FontRenderer.hpp"

mc::client::Font font;
font.initialize(256);  // 256x256纹理图集
font.addProvider(std::make_unique<mc::client::BitmapGlyphProvider>());

mc::client::FontRenderer fontRenderer;
fontRenderer.initialize(&font);
```

### 2. 创建UI

```cpp
#include "client/ui/kagero/KageroEngine.hpp"
#include "client/ui/kagero/widget/ButtonWidget.hpp"

using namespace mc::client::ui::kagero;

// 创建引擎
KageroEngine engine;
engine.initialize(canvas, {1920, 1080});

// 添加按钮
auto button = std::make_unique<widget::ButtonWidget>(
    "btn_start", 100, 100, 200, 40, "Start Game"
);
button->setOnPress([](widget::ButtonWidget& btn) {
    std::cout << "Button pressed!" << std::endl;
});
engine.addLayer(std::move(button), 10);

// 主循环
engine.render();
engine.update(deltaTime);
```

### 3. 使用模板系统

```cpp
#include "client/ui/kagero/template/TemplateSystem.hpp"
#include "client/ui/kagero/template/compiler/TemplateCompiler.hpp"

using namespace mc::client::ui::kagero;

// 初始化
tpl::initializeTemplateSystem();

// 编译模板
tpl::compiler::TemplateCompiler compiler;
auto compiled = compiler.compile(R"(
    <screen id="main" width="800" height="600">
        <text id="title" text="Hello World!"/>
    </screen>
)");

// 实例化
state::StateStore& store = state::StateStore::instance();
event::EventBus& eventBus = event::EventBus::instance();
tpl::binder::BindingContext ctx(store, eventBus);

tpl::runtime::TemplateInstance instance(compiled.get(), ctx);
instance.registerDefaultFactories();
auto root = instance.instantiate();
```

### 4. 渲染HUD

```cpp
#include "client/ui/minecraft/widgets/HudWidget.hpp"

auto hud = std::make_unique<mc::client::ui::minecraft::widgets::HudWidget>();
hud->setPlayer(player);
hud->setGuiRenderer(&guiRenderer);
hud->setIconsAtlas(&iconsAtlas);
hud->setWidgetsAtlas(&widgetsAtlas);
```

## 容易踩的坑

### 1. 字形图集已满

**问题**：`FontTextureAtlas::addGlyph` 返回错误。

**原因**：字形纹理图集空间不足，无法容纳新字形。

**解决方案**：
- 增大图集大小：`font.initialize(512)` 
- 预加载常用字符
- 分多个图集

### 2. 事件订阅未取消

**问题**：内存泄漏或野指针回调。

**原因**：订阅事件后忘记取消订阅。

**解决方案**：
```cpp
// 使用RAII管理订阅
tpl::event::EventSubscription<ClickEvent> subscription([](const ClickEvent& e) {
    // 处理事件
});
// 离开作用域自动取消订阅
```

### 3. Widget生命周期

**问题**：访问已销毁的Widget。

**原因**：Widget被移除后仍持有指针。

**解决方案**：
- 使用 `std::unique_ptr` 管理Widget生命周期
- 不要持有Widget的裸指针，使用 `Widget::WeakPtr`
- 在 `onClose()` 回调中清理引用

### 4. 状态更新循环

**问题**：状态更新触发观察者，观察者又更新状态，形成死循环。

**解决方案**：
```cpp
Reactive<i32> count(0);
count.observe([&count](i32 oldVal, i32 newVal) {
    if (newVal > 100) {
        count.set(100);  // 这会再次触发观察者！
    }
});

// 正确做法：检查是否需要更新
count.observe([](i32 oldVal, i32 newVal) {
    if (newVal > 100) {
        // 不直接修改，而是发送事件
        EventBus::instance().publish(ValueClampEvent(100));
    }
});
```

### 5. 布局未更新

**问题**：Widget位置或大小不正确。

**原因**：未在窗口大小改变时重新布局。

**解决方案**：
```cpp
void onResize(i32 width, i32 height) override {
    // 重新计算布局
    engine.resize(width, height);
    // 或手动更新
    widget->setBounds(Rect(x, y, newWidth, newHeight));
}
```

### 6. 模板编译错误

**问题**：模板编译失败，抛出 `TemplateError`。

**常见原因**：
- 未闭合的标签
- 无效的属性名
- 绑定表达式语法错误

**解决方案**：
```cpp
try {
    auto compiled = compiler.compile(templateStr);
} catch (const tpl::core::TemplateError& e) {
    spdlog::error("Template compilation failed: {}", e.what());
}
```

### 7. TridentCanvas限制

**问题**：圆角矩形、圆形等显示为普通矩形。

**原因**：`TridentCanvas` 是简化实现，不支持所有绘图操作。

**已知限制**：
- `drawRRect`: 退化为普通矩形
- `drawCircle`/`drawOval`: 退化为边界矩形
- `drawPath`: 忽略
- `clipOutRect`: 忽略并记录警告
- `saveLayer`/`saveLayerAlpha`: 只保存裁剪和变换状态，不支持离屏渲染

### 8. 线程安全

**问题**：在非主线程访问UI组件。

**原因**：UI组件不是线程安全的。

**解决方案**：
- 所有UI操作必须在主线程执行
- 使用事件总线跨线程通信
- 使用 `std::function` + `std::bind` 在主线程调度执行

## 测试用例

| 测试文件 | 测试内容 |
|----------|----------|
| `tests/ui/kagero/event/event_test.cpp` | 事件系统：EventBus订阅/发布、事件过滤、优先级 |
| `tests/ui/kagero/template/template_test.cpp` | 模板系统：词法分析、语法分析、编译、实例化 |

### 测试命令

```powershell
# 运行UI测试
./build/bin/Release/mc_tests.exe --gtest_filter="UI*"

# 运行特定测试
./build/bin/Release/mc_tests.exe --gtest_filter="UI.EventBusTest.*"
./build/bin/Release/mc_tests.exe --gtest_filter="UI.TemplateTest.*"
```

## 开发准则

1. **代码、文档、单测三者同步**：修改 Kagero 框架时必须同步更新文档和测试
2. **命名空间**：所有 Kagero 组件在 `mc::client::ui::kagero` 命名空间
3. **Widget基类**：所有组件继承 `Widget` 基类，实现 `paint()` 方法
4. **事件处理**：返回 `true` 表示事件已处理，阻止传播
5. **状态更新**：通过 `Reactive<T>` 或 `StateStore` 管理状态，避免直接修改

## 参考资料

- Minecraft 1.16.5 `net.minecraft.client.gui` 包
- CSS Flexbox 规范
- Chromium Skia 图形库
- Flutter 框架设计
