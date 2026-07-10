# Kagero UI 引擎

**Kagero**（陽炎，かげろう）是 Cubium 项目的现代化 UI 引擎，采用声明式模板、响应式状态管理和组件化架构。

## 目录结构

```
kagero/
├── Types.hpp                    # 基础类型定义（Rect, Margin, Padding, Anchor等）
├── KageroEngine.hpp/cpp         # UI引擎核心类，统一管理所有UI组件（含双击检测、右键分发）
├── README.md                    # 本文档
│
├── event/                       # 事件系统
│   ├── Event.hpp                # 事件基类、EventType枚举、SimpleEvent模板
│   ├── EventBus.hpp             # 事件总线，支持订阅/发布、优先级、过滤
│   ├── InputEvents.hpp          # 输入事件（鼠标、键盘）
│   ├── UIEvents.hpp             # UI事件（焦点、屏幕切换）
│   └── WidgetEvents.hpp         # Widget事件（值变化、按钮点击、滑动条等）
│
├── state/                       # 状态管理
│   ├── ReactiveState.hpp        # 响应式状态包装器（Reactive<T>）、Computed<T>、Binding<T>
│   ├── StateStore.hpp           # 全局状态存储，支持订阅、中间件、批量更新
│   ├── StateBinding.hpp         # 状态绑定工具、StateScope、StateContext
│   └── StateObserver.hpp        # 观察者辅助类
│
├── widget/                      # Widget组件
│   ├── Widget.hpp               # Widget基类，所有UI组件的基类（含onDoubleClick/onRightClick/onDragStart/onDragEnd虚方法、Tooltip支持）
│   ├── IWidgetContainer.hpp     # 容器接口，CRTP模板
│   ├── ContainerWidget.hpp/cpp  # 通用容器组件
│   ├── ButtonWidget.hpp         # 按钮组件（ButtonWidget, ImageButtonWidget）
│   ├── Tooltip.hpp              # Tooltip数据类（多行文本、最大宽度）
│   ├── TooltipRenderer.hpp      # Tooltip渲染工具（MC风格背景/边框/文字）
│   ├── TextWidget.hpp           # 文本显示组件
│   ├── RichTextWidget.hpp       # 富文本组件，支持ITextComponent渲染
│   ├── TextFieldWidget.hpp      # 文本输入框组件
│   ├── CheckboxWidget.hpp       # 复选框组件
│   ├── SliderWidget.hpp         # 滑动条组件
│   ├── ListWidget.hpp           # 列表组件（支持框架级双击回调）
│   ├── ScrollableWidget.hpp     # 可滚动容器组件
│   ├── SlotWidget.hpp           # 物品槽组件（用于背包等）
│   ├── Viewport3DWidget.hpp     # 3D视口组件（用于物品预览等）
│   ├── ImageWidget.hpp          # 图片组件（来自精灵图集，支持 auto 尺寸/着色/UV）
│   └── PaintContext.hpp/cpp     # 绘图上下文，封装ICanvas操作
│
├── layout/                      # 布局系统
│   ├── LayoutSystem.hpp         # 布局系统统一入口
│   ├── core/
│   │   ├── MeasureSpec.hpp      # 测量规格（UNSPECIFIED, EXACTLY, AT_MOST）
│   │   ├── LayoutResult.hpp     # 布局结果（位置、尺寸、FlexItem配置）
│   │   └── LayoutEngine.hpp/cpp # 布局引擎，支持Flex/Grid/Anchor布局
│   ├── constraints/
│   │   └── LayoutConstraints.hpp # 布局约束（最小/最大尺寸）
│   ├── algorithms/
│   │   ├── FlexLayout.hpp/cpp   # 弹性布局算法（类似CSS Flexbox）
│   │   ├── GridLayout.hpp/cpp   # 网格布局算法
│   │   └── AnchorLayout.hpp/cpp # 锚点布局算法
│   └── integration/
│       └── WidgetLayoutAdaptor.hpp/cpp # Widget适配器，连接Widget和布局引擎
│
├── template/                    # 模板系统
│   ├── Template.hpp             # 模板入口文件
│   ├── TemplateSystem.hpp/cpp   # 模板系统初始化/关闭
│   ├── core/
│   │   ├── TemplateConfig.hpp   # 模板配置（版本、特性开关）
│   │   └── TemplateError.hpp    # 模板错误类型
│   ├── parser/
│   │   ├── Lexer.hpp/cpp        # 词法分析器
│   │   ├── Parser.hpp/cpp       # 语法分析器
│   │   ├── Ast.hpp/cpp          # 抽象语法树定义
│   │   └── AstVisitor.hpp/cpp   # AST访问者模式
│   ├── compiler/
│   │   └── TemplateCompiler.hpp/cpp # 模板编译器（AST -> 可执行模板）
│   ├── binder/
│   │   └── BindingContext.hpp/cpp   # 绑定上下文（状态绑定、事件回调）
│   ├── runtime/
│   │   ├── TemplateInstance.hpp/cpp # 模板实例化运行时
│   │   └── UpdateScheduler.hpp/cpp  # 更新调度器（批量更新、防抖）
│   └── bindings/
│       ├── BuiltinWidgets.hpp/cpp   # 内置Widget工厂
│       └── BuiltinEvents.hpp/cpp    # 内置事件绑定器
│
├── paint/                       # 绘制抽象层
│   ├── Color.hpp                # 颜色定义和工具函数
│   ├── Geometry.hpp/cpp         # 几何类型（Rect, RRect, Point, Size, Matrix）及变换
│   ├── PaintContext.hpp/cpp     # 绘图上下文（封装ICanvas）
│   ├── TextureImage.hpp/cpp     # 纹理图像封装
│   └── contracts/
│       ├── ICanvas.hpp          # 画布接口（绘图操作）
│       ├── IPaint.hpp           # 画笔接口（样式配置）
│       ├── IPath.hpp            # 路径接口（矢量图形）
│       ├── IImage.hpp           # 图像接口
│       ├── ISurface.hpp         # 绘图表面接口
│       ├── ITypeface.hpp        # 字体接口
│       └── ITextBlob.hpp        # 文本块接口
│
└── docs/                        # 文档目录
    ├── README.md                # 文档索引
    ├── 01-quick-start.md        # 快速开始
    ├── 02-template-system.md    # 模板系统详解
    ├── 03-state-system.md       # 状态系统详解
    ├── 04-event-system.md       # 事件系统详解
    └── 05-built-in-widgets.md   # 内置组件详解
```

## 内部模块关系

```
┌─────────────────────────────────────────────────────────────────┐
│                        KageroEngine                              │
│                      (UI引擎入口)                                │
└─────────────────────────────────────────────────────────────────┘
                              │
        ┌─────────────────────┼─────────────────────┐
        ▼                     ▼                     ▼
┌───────────────┐    ┌───────────────┐    ┌───────────────┐
│    Widget     │◄───│    Layout     │    │    Event      │
│   (组件层)    │    │   (布局层)    │    │   (事件层)    │
└───────┬───────┘    └───────────────┘    └───────────────┘
        │                    ▲                     ▲
        ▼                    │                     │
┌───────────────┐    ┌───────┴───────┐    ┌───────┴───────┐
│    Paint      │    │    State      │    │   Template    │
│   (绘制层)    │    │   (状态层)    │    │   (模板层)    │
└───────────────┘    └───────────────┘    └───────────────┘
```

**模块职责：**
- **Widget** - 所有UI组件的基类和具体实现，处理用户交互和渲染
- **Layout** - 提供Flex/Grid/Anchor等布局算法，计算组件位置和尺寸
- **Event** - 发布-订阅模式的事件系统，支持类型安全的事件处理
- **State** - 响应式状态管理，支持数据绑定和自动更新
- **Template** - 声明式UI定义，XML模板编译和实例化
- **Paint** - 平台无关的绘制抽象接口，封装ICanvas/IPaint/IPath

## 双击与右键事件系统

Kagero引擎在框架层面统一实现了双击检测和右键事件分发，参考MC Java版 `MouseHandler` 的双击检测机制。

### 事件分发流程

```
GLFW鼠标回调 → InputManager → ClientApplication → KageroEngine::handleClick()
                                                         │
                                                         ├─ onClick() → Widget::onClick()
                                                         ├─ 双击检测（250ms内同Widget同按钮）
                                                         │   └─ onDoubleClick() → Widget::onDoubleClick()
                                                         └─ 右键分发（button == 1）
                                                             └─ onRightClick() → Widget::onRightClick()
```

### KageroEngine 双击检测

- **阈值**：250ms（`DOUBLE_CLICK_THRESHOLD_MS`）
- **条件**：同一Widget、同一鼠标按钮、250ms内的第二次点击
- **状态**：`m_lastClickWidget`、`m_lastClickButton`、`m_lastClickTimeMs`
- **重置**：双击触发后状态立即重置，三击不会触发第二次双击

### 右键点击行为

右键点击（button == 1）时，`onClick` 和 `onRightClick` **都会触发**，这是有意为之的设计：
- `onClick(x, y, 1, mods)` — 通用点击事件（button=1表示右键）
- `onRightClick(x, y, mods)` — 专用右键事件

组件应在 `onClick` 中检查 `button` 参数来区分左右键，或仅处理左键点击（button == 0）。
右键双击同样遵循双击检测机制：250ms内同一Widget同一右键连续点击会触发 `onDoubleClick(x, y, 1, mods)`。

### Widget 虚方法

| 方法 | 说明 | 默认行为 |
|------|------|----------|
| `onDoubleClick(x, y, button, mods)` | 双击事件 | 调用 `m_onDoubleClickCallback`，无回调返回false |
| `onRightClick(x, y, mods)` | 右键事件 | 调用 `m_onRightClickCallback`，无回调返回false |

### Widget 回调设置

```cpp
widget.setOnDoubleClickCallback([](Widget& w) { /* 双击处理 */ });
widget.setOnRightClickCallback([](Widget& w) { /* 右键处理 */ });
```

### 容器分发

ContainerWidget 和 ScrollableWidget 均覆写了 `onDoubleClick`/`onRightClick`，自动将事件分发到子Widget：
- ContainerWidget：遍历子Widget找到命中目标
- ScrollableWidget：调整滚动偏移后分发到子Widget

### ListWidget 双击

ListWidget 的 `onDoubleClick` 覆写会将双击事件分发到列表项（`IListItem::onDoubleClick`），并触发 `m_onDoubleClick` 回调（签名：`void(size_t index, IListItem*)`），然后调用基类 `Widget::onDoubleClick` 以触发模板回调。

## 拖拽事件系统

Kagero 引擎在框架层面统一实现了拖拽开始/进行/结束的事件分发，参考 MC Java 版 `ContainerEventHandler` 的拖拽协议：无阈值、点击命中即开始拖拽、鼠标释放即结束拖拽。

### 事件分发流程

```
GLFW鼠标回调 → InputManager → ClientApplication → KageroEngine::handleClick()
                                                         │
                                                         ├─ onClick() → Widget::onClick()
                                                         │   └─ 命中后立即触发 onDragStart()
                                                         │
GLFW鼠标移动 → KageroEngine::handleMouseMove()
                  └─ onDrag() → Widget::onDrag()（拖拽过程中持续触发）

GLFW鼠标释放 → KageroEngine::handleRelease()
                  ├─ onDragEnd() → Widget::onDragEnd()（先触发，dropped=false）
                  └─ onRelease() → Widget::onRelease()
```

### KageroEngine 拖拽协议

- **无阈值**：点击命中后立即进入拖拽状态，无最小移动距离要求
- **按钮记录**：`m_dragButton` 记录触发拖拽的鼠标按钮，用于 `onDrag`/`onDragEnd` 分发
- **修饰键记录**：`m_dragMods` 记录拖拽开始时的修饰键状态
- **焦点锁定**：拖拽期间所有 `onDrag` 事件分发到 `m_draggingWidget`，即使鼠标移出组件区域
- **正常结束**：鼠标释放时先触发 `onDragEnd(dropped=false)`，再触发 `onRelease`
- **丢弃标志**：`dropped=true` 表示拖拽被外部取消（焦点丢失等），当前未启用

### Widget 虚方法

| 方法 | 说明 | 默认行为 |
|------|------|----------|
| `onDragStart(x, y, button, mods)` | 拖拽开始事件 | 返回 false |
| `onDrag(x, y, deltaX, deltaY, button)` | 拖拽进行事件（持续触发） | 返回 false |
| `onDragEnd(x, y, button, dropped)` | 拖拽结束事件 | 返回 false |

### 容器分发

ContainerWidget 和 ScrollableWidget 均覆写了 `onDragStart`/`onDrag`/`onDragEnd`，自动将事件分发到子 Widget：
- ContainerWidget：通过 `handleDragStartInChildren`/`handleDragInChildren`/`handleDragEndInChildren` 分发
- ScrollableWidget：优先处理滚动条拖拽（`m_draggingScrollbar`/`m_draggingHorizontalScrollbar`），否则调整滚动偏移后分发到子 Widget

### 已实现拖拽的组件

| 组件 | 拖拽行为 |
|------|----------|
| `SliderWidget` | `onClick` 设置 `m_dragging`，`onDrag` 实时更新值，`onDragEnd` 触发最终 `m_onValueChanged` |
| `ScrollableWidget` | `onClick` 设置滚动条拖拽标志，`onDrag` 移动滚动位置，`onDragEnd` 清除标志 |
| `Viewport3DWidget` | `onClick` 设置 `m_dragging`，`onDrag` 旋转视角，`onDragEnd` 清除 `m_dragging` |

### BuiltinEvents 拖拽事件

模板系统可通过 `bind:dragStart`/`bind:drag`/`bind:dragEnd` 绑定拖拽事件：
- `dragStart` → `BuiltinEvents` 调用 `widget->onDragStart()`
- `drag` → `BuiltinEvents` 调用 `widget->onDrag()`
- `dragEnd` → `BuiltinEvents` 调用 `widget->onDragEnd()`

事件对象：`DragStartEvent(x, y, button, mods)`、`MouseDragEvent(x, y, deltaX, deltaY, button)`、`DragEndEvent(x, y, button, dropped)`。

其中 `button` 取值与 GLFW 一致（0=左键，1=右键，2=中键），`mods` 为 `KeyMods` 位掩码，`dropped` 表示拖拽是否被外部取消。

## Tooltip 系统

Widget 基类内置了 Minecraft 风格的 Tooltip 支持，参考 MC Java 版的 `WidgetTooltipHolder` + `Tooltip` 设计模式。

### 核心 API

| 方法 | 说明 |
|------|------|
| `setTooltip(const Tooltip&)` | 设置 Tooltip 数据 |
| `setTooltip(const string&)` | 设置单行 Tooltip（便捷方法） |
| `clearTooltip()` | 清除 Tooltip |
| `tooltip()` | 获取当前 Tooltip |
| `hasTooltip()` | 检查是否设置了 Tooltip |
| `setTooltipDelay(i32 ms)` | 设置显示延迟（毫秒） |
| `tooltipDelay()` | 获取显示延迟 |
| `refreshTooltip(PaintContext&, f32, f32)` | 刷新并渲染 Tooltip（在 paint 末尾调用） |

### 使用方式

```cpp
// 单行 Tooltip
button->setTooltip("保存");

// 多行 Tooltip
button->setTooltip(Tooltip::create("保存", "将当前进度保存到存档"));

// 延迟显示（参考 MC Java 版的 setTooltipDelay）
button->setTooltipDelay(500); // 悬停500ms后显示

// 在 paint() 末尾调用 refreshTooltip
void paint(PaintContext& ctx) override {
    // ... 组件自身渲染 ...
    refreshTooltip(ctx, static_cast<f32>(ctx.canvas().width()),
                        static_cast<f32>(ctx.canvas().height()));
}
```

### 渲染风格

TooltipRenderer 使用 Minecraft 风格的渲染：
- 背景：半透明深色 (`0xF0100010`)
- 边框：紫色 (`0x505000FF`)
- 文本：白色 (`0xFFFFFFFF`)
- 内边距：4px
- 鼠标偏移：12px
- 自动翻转：超出屏幕边界时翻转到鼠标左方或上方

### 延迟机制

参考 MC Java 版的 `WidgetTooltipHolder.refreshTooltipForNextRenderPass()`：
- 鼠标进入组件时记录开始时间
- 延迟期间不渲染 Tooltip
- 鼠标离开时重置计时状态
- 默认延迟为 0（立即显示）

### 模板绑定

模板系统中可通过 `bind:doubleClick` 和 `bind:rightClick` 绑定事件：
- `doubleClick` → `BuiltinEvents` 调用 `widget->onDoubleClick()`
- `rightClick` → `BuiltinEvents` 调用 `widget->onRightClick()`
- `TemplateInstance` 通过 `setOnDoubleClickCallback`/`setOnRightClickCallback` 桥接模板回调

## 上下游外部依赖关系

**被依赖方（谁使用了Kagero）：**
- `src/client/` - 客户端主程序通过 KageroEngine 创建和管理所有UI界面
- `src/client/screen/` - 各种Screen（主菜单、暂停菜单、背包界面等）使用Widget构建

**依赖方（Kagero使用了谁）：**
- `common/core/Types.hpp` - 基础类型定义（i32, u32, String等）
- `common/core/Result.hpp` - 错误处理
- `common/util/text/Utf8.hpp` - UTF-8 编码/解码/迭代工具（所有文本Widget依赖此模块处理多字节字符）
- `common/input/KeyBinding.hpp` - 平台无关键码常量（Keys命名空间），Widget组件通过此模块替代硬编码GLFW键码
- `client/ui/Font.hpp` - 字体渲染和字形查找
- `client/ui/Glyph.hpp` - 字形渲染
- `client/renderer/api/` - 渲染抽象接口（ICanvas实现）
- GLFW - 窗口/输入事件（仅KageroEngine层接收GLFW回调，Widget层不直接依赖GLFW键码）

## 容易踩的坑

### 1. EventBus 线程安全

EventBus本身是线程安全的，但回调可能在任意线程执行，需要保护共享数据：
```cpp
EventBus::instance().subscribe<MouseClickEvent>([](const MouseClickEvent& e) {
    std::lock_guard<std::mutex> lock(sharedDataMutex);
    sharedData.clickCount++;
});
```

### 2. Reactive 循环依赖

双向绑定可能导致无限循环，需要用值比较避免：
```cpp
a.observe([&b](i32, i32 newVal) { if (b.get() != newVal) b.set(newVal); });
b.observe([&a](i32, i32 newVal) { if (a.get() != newVal) a.set(newVal); });
// 或直接使用 binding::bindReactive 自动处理
```

### 3. Widget 生命周期

不要在回调中捕获Widget的裸指针，Widget可能已被销毁。使用智能指针或确保生命周期：
```cpp
auto button = std::make_shared<ButtonWidget>(...);
button->setOnPress([weakButton = std::weak_ptr(button)]() {
    if (auto btn = weakButton.lock()) { btn->setText("Clicked"); }
});
```

### 4. 布局时机

Widget添加到容器后，布局结果可能还未计算完成。需要手动触发布局或使用布局引擎：
```cpp
container->addChild(button);
layout::LayoutEngine::instance().layout(adaptor, Rect(0, 0, 800, 600));
i32 width = button->width();  // 现在是正确的
```

### 5. 模板绑定路径错误

绑定路径不存在时运行时会静默失败，确保StateStore中有对应的键：
```cpp
store.set("player.name", std::string("Steve"));
// 然后 bind:text="player.name" 才能正常工作
```

### 6. 状态批量更新

多次单独更新会触发多次订阅者回调，使用批量更新避免：
```cpp
store.batchUpdate([](StateStore& s) {
    s.set("player.health", 90);
    s.set("player.mana", 50);
    s.set("player.stamina", 100);
});
// 只触发1次回调，避免多次重绘
```

### 7. ContainerWidget 焦点管理

ContainerWidget的Tab导航需要在子组件中正确设置tabIndex，否则focusNext/focusPrevious可能不按预期工作。

### 8. PaintContext 状态栈

save()/restore()必须配对使用，否则会导致绘图状态错乱。建议使用RAII包装器。

### 9. 模板回调注册

通过BindingContext::exposeCallback注册的回调必须在使用TemplateInstance之前完成，否则模板中的事件绑定会找不到回调。

### 10. ScrollableWidget 水平与垂直滚动

ScrollableWidget同时支持水平和垂直双向滚动，提供完整的滚动条交互：

**核心API：**
- `setContentSize(width, height)` / `setContentWidth(width)` / `setContentHeight(height)` 设置内容尺寸
- `setShowScrollbar(bool)` / `setShowHorizontalScrollbar(bool)` 控制滚动条可见性
- `scrollBy(deltaY)` / `scrollByX(deltaX)` 增量滚动
- `scrollToTop()` / `scrollToBottom()` / `scrollToLeft()` / `scrollToRight()` 滚动到边界
- `scrollIntoView(y, height)` / `scrollXIntoView(x, width)` 滚动到指定区域可见
- `horizontalScrollRatio()` / `scrollRatio()` 获取滚动比例（0.0-1.0）

**交互方式：**
- **鼠标拖拽**：点击并拖拽垂直滚动条（右侧）或水平滚动条（底部）
- **鼠标滚轮**：垂直滚动；Shift+滚轮水平滚动（需组件获取焦点后Shift键状态才可追踪）
- **键盘**：↑↓键垂直滚动、←→键水平滚动、PageUp/PageDown垂直翻页、Home/End滚动到首尾（Shift+Home/End水平方向）

**注意事项：**
- 必须正确设置内容尺寸，否则滚动条不会出现或滚动范围不正确
- 当两轴滚动条同时可见时，右下角区域会预留间隙避免重叠
- `visibleWidth()` 和 `visibleHeight()` 会自动扣除对应方向滚动条的占用空间
- Shift+滚轮水平滚动需要组件获取焦点后才能正确追踪Shift键状态

### 11. TextFieldWidget UTF-8 注意事项

TextFieldWidget 内部所有位置/索引操作基于 **码点**（而非字节偏移），使用 `util::text::Utf8.hpp` 工具函数进行码点索引与字节偏移之间的转换。修改此组件时务必注意：
- `m_cursorPosition` 和 `m_selectionEnd` 是码点索引，不是字节偏移
- 字符串截断/删除操作必须通过 `utf8CodepointToByteOffset` 转换后再操作 `std::string`
- `measureTextWidth`、`_measurePrefixWidth`、`positionFromTextOffset` 均按码点迭代
- 光标闪烁使用 `m_cursorBlinkTimer`（秒）+ `m_cursorVisible` 布尔值，周期 500ms
- 选区高亮使用 `m_selectionColor`（默认半透明蓝色 `0x8000AAFF`），在 `paint()` 中通过 `drawFilledRect` 绘制

### 12. Widget 键码使用 Keys 常量

Widget组件（TextFieldWidget、SliderWidget、ScrollableWidget等）的 `onKey()` 方法中的键码必须使用 `mc::Keys` 命名空间常量（定义在 `common/input/KeyBinding.hpp`），而非硬编码数值或GLFW宏。键动作判断使用 `KeyAction::Press/Repeat/Release`（定义在 `Types.hpp`），修饰键判断使用 `hasMod(static_cast<KeyMods>(mods), KeyMods::Shift)` 而非位掩码 `mods & 0x0001`。模板系统的 `parseKeyCode()` 和 `parseKeyMods()` 同样使用这些常量和枚举。
