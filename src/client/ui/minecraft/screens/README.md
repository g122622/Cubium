# Minecraft Screens 模块

本目录存放 Minecraft 风格的屏幕与调试界面实现，包括主菜单、背包、暂停菜单和 F3 调试屏幕。

## 目录结构树

```text
src/client/ui/minecraft/screens/
├── DebugScreenWidget.hpp / .cpp
├── Screen.hpp / .cpp
├── ScreenManager.hpp / .cpp
├── MainMenuScreen.hpp / .cpp
├── PauseScreen.hpp / .cpp
├── InventoryScreen.hpp / .cpp
├── ContainerScreen.hpp / .cpp
└── ...
```

## 文件介绍

- `DebugScreenWidget.*`：F3 调试信息面板，显示坐标、光照、维度、系统信息等。
- `Screen.*`：Minecraft 业务层屏幕基类。
- `ScreenManager.*`：业务屏幕栈管理。
- 其余 `*Screen.*`：具体菜单、物品栏、容器和选项屏幕。

## 内部模块关系

- `DebugScreenWidget` 依赖 `ClientWorld`、`Camera`、`Player`、`ClientDimensionManager`。
- `ScreenManager` 管理各具体屏幕生命周期。
- 调试屏幕由 `ClientApplicationBootstrap` 创建并注入运行时依赖。

## 外部依赖关系

- 被 `src/client/application/features/ClientApplicationBootstrap.cpp` 初始化并接入 UI 层。
- 依赖 `src/client/world/`、`src/client/renderer/` 和 `src/client/dimension/`。

## 容易踩的坑

- 不要在 F3 界面里硬编码维度名；必须通过 `ClientDimensionManager` 取当前维度显示名。
- 调试信息里的玩家坐标应来自玩家真实位置，而不是相机摇晃后的矩阵结果。
