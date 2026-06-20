# Kagero UI 引擎

**Kagero**（陽炎，かげろう）是 Cubium 项目的现代化 UI 引擎，采用声明式模板、响应式状态管理和组件化架构。

## 目录结构

```
kagero/
├── Types.hpp                    # 基础类型定义（Rect, Margin, Padding, Anchor等）
├── KageroEngine.hpp/cpp         # UI引擎核心类，统一管理所有UI组件
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
│   ├── Widget.hpp               # Widget基类，所有UI组件的基类
│   ├── IWidgetContainer.hpp     # 容器接口，CRTP模板
│   ├── ContainerWidget.hpp/cpp  # 通用容器组件
│   ├── ButtonWidget.hpp         # 按钮组件（ButtonWidget, ImageButtonWidget）
│   ├── TextWidget.hpp           # 文本显示组件
│   ├── RichTextWidget.hpp       # 富文本组件，支持ITextComponent渲染
│   ├── TextFieldWidget.hpp      # 文本输入框组件
│   ├── CheckboxWidget.hpp       # 复选框组件
│   ├── SliderWidget.hpp         # 滑动条组件
│   ├── ListWidget.hpp           # 列表组件
│   ├── ScrollableWidget.hpp     # 可滚动容器组件
│   ├── SlotWidget.hpp           # 物品槽组件（用于背包等）
│   ├── Viewport3DWidget.hpp     # 3D视口组件（用于物品预览等）
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

## 上下游外部依赖关系

**被依赖方（谁使用了Kagero）：**
- `src/client/` - 客户端主程序通过 KageroEngine 创建和管理所有UI界面
- `src/client/screen/` - 各种Screen（主菜单、暂停菜单、背包界面等）使用Widget构建

**依赖方（Kagero使用了谁）：**
- `common/core/Types.hpp` - 基础类型定义（i32, u32, String等）
- `common/core/Result.hpp` - 错误处理
- `common/util/text/Utf8.hpp` - UTF-8 编码/解码/迭代工具（所有文本Widget依赖此模块处理多字节字符）
- `client/ui/Font.hpp` - 字体渲染和字形查找
- `client/ui/Glyph.hpp` - 字形渲染
- `client/renderer/api/` - 渲染抽象接口（ICanvas实现）
- GLFW - 窗口/输入事件
- spdlog - 日志

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

### 10. ScrollableWidget 内容尺寸

ScrollableWidget需要正确设置内容尺寸（setContentSize），否则滚动条不会出现或滚动范围不正确。

### 11. TextFieldWidget UTF-8 注意事项

TextFieldWidget 内部所有位置/索引操作基于 **码点**（而非字节偏移），使用 `util::text::Utf8.hpp` 工具函数进行码点索引与字节偏移之间的转换。修改此组件时务必注意：
- `m_cursorPosition` 和 `m_selectionEnd` 是码点索引，不是字节偏移
- 字符串截断/删除操作必须通过 `utf8CodepointToByteOffset` 转换后再操作 `std::string`
- `measureTextWidth`、`_measurePrefixWidth`、`positionFromTextOffset` 均按码点迭代
- 光标闪烁使用 `m_cursorBlinkTimer`（秒）+ `m_cursorVisible` 布尔值，周期 500ms
- 选区高亮使用 `m_selectionColor`（默认半透明蓝色 `0x8000AAFF`），在 `paint()` 中通过 `drawFilledRect` 绘制
