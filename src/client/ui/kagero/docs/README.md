# Kagero UI 引擎文档

**Kagero**（陽炎，かげろう）是 Cubium 项目的现代化 UI 引擎，采用声明式模板、响应式状态管理和组件化架构。

开发准则：若要对 kagero 进行修改，务必保持 代码、文档、单测 三者同步。

> 详细架构、模块关系、依赖关系、容易踩的坑等内容请参阅 [../README.md](../README.md)。

## 核心特性

- **声明式模板** - XML 风格的模板语法，零内联脚本，编译时验证
- **响应式状态** - 自动追踪状态变化并更新 UI，支持观察者模式
- **事件总线** - 类型安全的事件分发系统，支持优先级和过滤
- **组件化架构** - 可复用的 Widget 组件库，支持自定义扩展
- **Flex 布局** - 类似 CSS Flexbox 的现代布局系统

## 文档目录

| 文档 | 说明 |
|------|------|
| [01-quick-start.md](./01-quick-start.md) | 快速开始：基本概念、创建界面、状态管理、事件处理 |
| [02-template-system.md](./02-template-system.md) | 模板系统：语法、编译、实例化、绑定 |
| [03-state-system.md](./03-state-system.md) | 状态系统：StateStore、Reactive、Binding、Computed |
| [04-event-system.md](./04-event-system.md) | 事件系统：EventBus、输入事件、UI事件、Widget事件 |
| [05-built-in-widgets.md](./05-built-in-widgets.md) | 内置组件：Widget基类、Button、Text、TextField等 |

## 内部模块关系

```
┌─────────────────────────────────────────────────────────────┐
│                      KageroEngine                           │
│                    (UI引擎入口)                              │
└─────────────────────────────────────────────────────────────┘
         │                 │                 │
         ▼                 ▼                 ▼
┌─────────────┐    ┌─────────────┐    ┌─────────────┐
│   Widget    │◄───│   Layout    │    │   Event     │
│  (组件层)   │    │  (布局层)   │    │  (事件层)   │
└─────────────┘    └─────────────┘    └─────────────┘
         │                                   │
         ▼                                   ▼
┌─────────────┐                      ┌─────────────┐
│   Paint     │                      │   State     │
│  (绘制层)   │                      │  (状态层)   │
└─────────────┘                      └─────────────┘
         │                                   │
         └───────────────┬───────────────────┘
                         ▼
                  ┌─────────────┐
                  │  Template   │
                  │  (模板层)   │
                  └─────────────┘
```

**依赖方向：**
- Widget → Paint（组件使用绘制接口）
- Widget → Event（组件发送/接收事件）
- Widget → State（组件绑定状态）
- Widget → Layout（组件参与布局）
- Template → Widget（模板实例化为组件树）
- Template → State（模板绑定状态）

## 上下游外部依赖关系

### 上游依赖（被谁使用）

| 模块 | 说明 |
|------|------|
| `client/ui/minecraft/` | Minecraft 特定 UI 实现（HUD、屏幕、菜单） |
| `client/application/ClientApplication` | 客户端主应用，初始化和管理 UI 引擎 |

### 下游依赖（依赖谁）

| 模块 | 说明 |
|------|------|
| `common/core/Types.hpp` | 基础类型定义（i32, u32, String 等） |
| `common/core/Result.hpp` | 错误处理 |
| `client/ui/Glyph.hpp` | 字形渲染 |
| `client/ui/FontRenderer.hpp` | 文本渲染 |
| `client/renderer/api/` | 渲染抽象接口（ICanvas 等） |
| `GLFW` | 窗口和输入事件 |
| `spdlog` | 日志 |

## 容易踩的坑

### 1. EventBus 线程安全

EventBus 是线程安全的，但回调中的共享数据需要保护。详见 [../README.md](../README.md)。

### 2. Reactive 循环依赖

双向绑定可能导致无限循环。使用值比较避免：
```cpp
a.observe([&b](i32, i32 newVal) {
    if (b.get() != newVal) b.set(newVal);  // 避免循环
});
```

### 3. Widget 生命周期

不要在回调中捕获 Widget 裸指针，可能悬空。使用 `std::weak_ptr` 或确保生命周期。

### 4. 布局时机

Widget 添加后需手动触发布局，否则 `width()`/`height()` 可能返回 0。

### 5. 模板绑定路径错误

绑定不存在的路径会静默失败。确保 `StateStore` 中有对应的键。

### 6. 状态批量更新

多次单独 `store.set()` 会触发多次回调。使用 `store.batchUpdate()` 批量更新。

> 以上问题的详细说明和更多陷阱请参阅 [../README.md](../README.md) 的"容易踩的坑"部分。
