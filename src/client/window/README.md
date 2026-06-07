# Window 模块

窗口管理模块，封装 GLFW 提供跨平台的窗口创建和管理功能。

## 目录结构

```
src/client/window/
├── Window.hpp    # 窗口类头文件，定义 WindowConfig 和 Window 类
└── Window.cpp    # 窗口类实现，包含 GLFW 初始化管理和回调转发
```

## 内部模块关系

本模块非常简单，仅由 `Window` 类和 `WindowConfig` 配置结构体组成：

- `WindowConfig` - 窗口配置结构体，包含尺寸、标题、全屏、VSync 等设置
- `Window` - 主窗口类，封装 GLFW 窗口操作和事件回调

```
WindowConfig ──► Window
```

## 上下游外部依赖关系

### 上游依赖（本模块依赖）

| 依赖 | 用途 |
|------|------|
| `GLFW` | 跨平台窗口和输入管理 |
| `common/core/Types.hpp` | 基础类型定义 (i32, f32 等) |
| `common/core/Result.hpp` | 错误处理 |
| `spdlog` | 日志输出 |
| `perfetto` | 性能追踪 |

### 下游依赖（谁依赖本模块）

| 模块 | 用途 |
|------|------|
| `ClientApplication` | 主使用者，创建和管理窗口实例 |
| `TridentContext` | 使用 GLFW window 句柄创建 Vulkan Surface |
| `InputManager` | 接收 Window 的输入事件 |

```
ClientApplication ──► Window ◄── TridentContext
       │                           │
       └──► InputManager ◄─────────┘
```

## 容易踩的坑

### 1. 多窗口实例的 GLFW 初始化

多个 Window 实例可能导致 GLFW 重复初始化或过早终止。代码使用静态计数器 `s_glfwInitCount` 管理引用计数，确保只在第一个窗口创建时初始化，最后一个窗口销毁时终止。无需手动处理。

### 2. 窗口尺寸 vs 帧缓冲尺寸

在高 DPI 显示器上，窗口尺寸和帧缓冲尺寸可能不同：
- `width()` / `height()` - 窗口逻辑尺寸（用于 UI 布局）
- `framebufferWidth()` / `framebufferHeight()` - 帧缓冲尺寸（用于渲染）

### 3. 回调中的 this 指针

静态回调函数无法直接访问类成员，需通过 `userData` 传递：

```cpp
window.setResizeCallback([](i32 width, i32 height, void* userData) {
    auto* app = static_cast<ClientApplication*>(userData);
    app->handleResize(width, height);
}, this);
```

### 4. 移动语义

Window 支持移动，但移动后原对象处于无效状态（`isValid() == false`, `handle() == nullptr`）。

### 5. VSync 设置时机

`setVSync()` 需要在窗口创建后调用，且需要在主线程。
