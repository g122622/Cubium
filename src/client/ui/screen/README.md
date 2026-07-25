# Screen 模块（屏幕管理器）

本目录承载 `ScreenManager` —— 屏幕栈管理器的单例门面，委托给 kagero 体系的 `ScreenStackWidget` 管理屏幕栈。所有具体屏幕（容器屏、主菜单、暂停、地图等）均位于 `ui/minecraft/screens/`，本目录不再持有任何屏幕实现。

## 目录结构

```text
src/client/ui/screen/
├── ScreenManager.hpp/cpp           # 屏幕栈管理器单例（委托 ScreenStackWidget）
└── README.md                       # 本文档
```

## 内部模块关系

```
┌─────────────────────────────────────────────────────────────────┐
│                      ClientApplication                           │
└─────────────────────────────────────────────────────────────────┘
                                 │ 输入/滚轮事件转发
                                 ▼
┌─────────────────────────────────────────────────────────────────┐
│                        ScreenManager                             │
│        （单例，委托 ScreenStackWidget 管理屏幕栈）                  │
└─────────────────────────────────────────────────────────────────┘
                                 │
                                 ▼
┌─────────────────────────────────────────────────────────────────┐
│                    ScreenStackWidget                             │
│        （kagero Screen 屏幕栈：push/pop/clear + 事件分发）          │
└─────────────────────────────────────────────────────────────────┘
```

- `ScreenManager`：单例门面，委托给 `ScreenStackWidget` 管理屏幕栈，不直接持有屏幕。
- `ScreenStackWidget`：kagero `Screen` 屏幕栈的实际管理者（位于 `ui/minecraft/widgets/`）。

## 上下游外部依赖关系

### 本模块依赖

| 模块 | 用途 |
|------|------|
| `client/ui/minecraft/widgets/ScreenStackWidget.hpp` | 屏幕栈组件（ScreenManager 委托对象） |
| `client/ui/minecraft/screens/Screen.hpp` | kagero 屏幕基类（openScreen/getCurrentKageroScreen 的参数与返回类型） |
| `GLFW` | 键盘和鼠标常量（GLFW_MOD_SHIFT, GLFW_KEY_Q 等） |

### 依赖本模块

| 模块 | 用途 |
|------|------|
| `client/application/ClientApplication.hpp` | 决定打开哪个屏幕，转发输入和滚轮事件 |
| `client/ui/minecraft/screens/` | kagero 屏幕系统，全部屏幕位于此 |

## 容易踩的坑

- **屏幕统一走 kagero 体系**：新屏幕一律放在 `ui/minecraft/screens/` 并继承 `Screen`，经 `ScreenManager::instance().openScreen(std::make_unique<...>())` 推入栈。旧的 IScreen 接口已删除，不再支持。
- **容器交互在 kagero 体系**：容器交互系统（拾取/快速移动/拖拽分发/丢弃/中键克隆等）位于 `ui/minecraft/screens/ContainerScreenBase.hpp` + `ui/kagero/widget/ContainerInteraction.hpp`。
