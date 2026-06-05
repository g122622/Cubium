# Kagero UI 引擎文档

**Kagero**（陽炎，かげろう）是 Cubium 项目的现代化 UI 引擎，采用声明式模板、响应式状态管理和组件化架构。

开发准则：若要对kagero进行修改，务必保持 代码、文档、单测 三者同步。

## 核心特性

- **声明式模板** - XML 风格的模板语法，零内联脚本，编译时验证
- **响应式状态** - 自动追踪状态变化并更新 UI，支持观察者模式
- **事件总线** - 类型安全的事件分发系统，支持优先级和过滤
- **组件化架构** - 可复用的 Widget 组件库，支持自定义扩展
- **Flex 布局** - 类似 CSS Flexbox 的现代布局系统

## 文档目录

1. [快速开始](./01-quick-start.md)
   - 基本概念和命名空间
   - 创建简单界面
   - 状态管理入门
   - 事件处理基础
   - 布局系统简介

2. [模板系统](./02-template-system.md)
   - 模板语法
   - 编译和实例化
   - 绑定上下文
   - 属性绑定和事件绑定
   - 条件渲染和循环渲染
   - 更新调度器

3. [状态系统](./03-state-system.md)
   - StateStore 全局状态存储
   - Reactive 响应式包装器
   - Binding 双向绑定
   - Computed 计算属性
   - 观察者辅助类
   - 状态选择器

4. [事件系统](./04-event-system.md)
   - Event 基类
   - EventBus 事件总线
   - 输入事件（鼠标、键盘）
   - UI 事件（焦点、值变化）
   - Widget 事件（按钮、滑块）
   - 自定义事件

5. [内置组件](./05-built-in-widgets.md)
   - Widget 基类
   - TextWidget 文本组件
   - ButtonWidget 按钮组件
   - CheckboxWidget 复选框
   - SliderWidget 滑块
   - TextFieldWidget 文本输入框
   - ContainerWidget 容器
   - ScrollableWidget 滚动容器
   - ListWidget 列表
   - SlotWidget 物品槽
   - Viewport3DWidget 3D 视口
   - PaintContext 绘制抽象

## 架构概览

```
kagero/
├── Types.hpp              # 基础类型（Rect, Margin, Padding, Anchor）
├── widget/                # Widget 组件
│   ├── Widget.hpp         # Widget 基类
│   ├── ButtonWidget.hpp   # 按钮
│   ├── TextWidget.hpp     # 文本
│   ├── TextFieldWidget.hpp # 文本输入
│   ├── CheckboxWidget.hpp # 复选框
│   ├── SliderWidget.hpp   # 滑块
│   ├── ContainerWidget.hpp # 容器
│   ├── ListWidget.hpp     # 列表
│   ├── ScrollableWidget.hpp # 滚动容器
│   ├── SlotWidget.hpp     # 物品槽
│   ├── Viewport3DWidget.hpp # 3D 视口
│   ├── PaintContext.hpp   # 绘制上下文
│   └── IWidgetContainer.hpp # 容器接口
├── state/                 # 状态管理
│   ├── StateStore.hpp     # 全局状态存储
│   ├── ReactiveState.hpp  # 响应式状态
│   ├── StateBinding.hpp   # 状态绑定工具
│   └── StateObserver.hpp  # 观察者辅助
├── event/                 # 事件系统
│   ├── Event.hpp          # 事件基类
│   ├── EventBus.hpp       # 事件总线
│   ├── InputEvents.hpp    # 输入事件
│   ├── UIEvents.hpp       # UI 事件
│   └── WidgetEvents.hpp   # 组件事件
├── template/              # 模板系统
│   ├── Template.hpp       # 入口文件
│   ├── parser/            # 解析器
│   │   ├── Lexer.hpp      # 词法分析
│   │   ├── Parser.hpp     # 语法分析
│   │   └── Ast.hpp        # 抽象语法树
│   ├── compiler/          # 编译器
│   │   └── TemplateCompiler.hpp
│   ├── binder/            # 绑定
│   │   └── BindingContext.hpp
│   ├── runtime/           # 运行时
│   │   └── TemplateInstance.hpp
│   ├── core/              # 核心配置
│   │   ├── TemplateConfig.hpp
│   │   └── TemplateError.hpp
│   └── bindings/          # 内置绑定
│       ├── BuiltinWidgets.hpp
│       └── BuiltinEvents.hpp
├── layout/                # 布局系统
│   ├── LayoutSystem.hpp   # 入口文件
│   ├── core/              # 核心
│   │   ├── MeasureSpec.hpp
│   │   ├── LayoutResult.hpp
│   │   └── LayoutEngine.hpp
│   ├── algorithms/        # 布局算法
│   │   ├── FlexLayout.hpp
│   │   ├── GridLayout.hpp
│   │   └── AnchorLayout.hpp
│   ├── constraints/       # 约束
│   │   └── LayoutConstraints.hpp
│   └── integration/       # 集成
│       └── WidgetLayoutAdaptor.hpp
└── paint/                 # 绘制抽象层（内部使用，类似chromium的skia）
    ├── Color.hpp
    ├── ICanvas.hpp
    ├── IImage.hpp
    ├── IPaint.hpp
    ├── IPath.hpp
    ├── ISurface.hpp
    ├── ITypeface.hpp
    ├── ITextBlob.hpp
    └── Geometry.hpp
```

## 快速示例

```cpp
#include "kagero/template/Template.hpp"
#include "kagero/state/StateStore.hpp"
#include "kagero/event/EventBus.hpp"
#include "kagero/widget/ButtonWidget.hpp"

using namespace mc::client::ui::kagero;

// 1. 初始化模板系统
tpl::initializeTemplateSystem();

// 2. 编译模板
tpl::compiler::TemplateCompiler compiler;
auto compiled = compiler.compile(R"(
    <screen id="main" width="800" height="600">
        <text id="title" x="300" y="50" text="Hello Kagero!"/>
        <button id="btn_start" x="300" y="200" width="200" height="40"
                text="Start Game" on:click="onStart"/>
    </screen>
)");

// 3. 创建绑定上下文
state::StateStore& store = state::StateStore::instance();
event::EventBus& eventBus = event::EventBus::instance();
tpl::binder::BindingContext ctx(store, eventBus);

// 4. 注册回调
ctx.exposeCallback("onStart", [](widget::Widget* w, const event::Event& e) {
    std::cout << "Game started!" << std::endl;
});

// 5. 实例化模板
tpl::runtime::TemplateInstance instance(compiled.get(), ctx);
instance.registerDefaultFactories();
auto root = instance.instantiate();
```

## 版本信息

- **版本**: 1.0.0
- **命名空间**: `mc::client::ui::kagero`
- **C++ 标准**: C++20

## 参考资料

本项目参考了 Minecraft 1.16.5 的 UI 实现，并结合现代 C++ 设计模式进行了重构和扩展。

- Widget 系统参考 MC 1.16.5 `net.minecraft.client.gui.widget`
- 布局系统参考 CSS Flexbox 规范
- 模板系统借鉴了现代前端框架的声明式设计
