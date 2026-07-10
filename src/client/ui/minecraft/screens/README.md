# Minecraft Screens 模块

本目录存放 Minecraft 风格的屏幕与调试界面实现，包括主菜单、背包、暂停菜单和 F3 调试屏幕。

## 目录结构

```text
src/client/ui/minecraft/screens/
├── Screen.hpp/cpp             # Minecraft 业务层屏幕基类
├── TemplateScreen.hpp/cpp     # 模板驱动屏幕基类
├── ScreenManager.hpp/cpp      # 业务屏幕栈管理
├── MainMenuScreen.hpp/cpp     # 主菜单
├── WorldSelectionScreen.hpp/cpp # 存档选择（含删除世界功能）
├── CreateWorldScreen.hpp/cpp  # 创建世界
├── PauseScreen.hpp/cpp        # 暂停菜单
├── ConfirmScreen.hpp/cpp      # 通用确认对话框（双按钮：确认/取消）
├── MessageScreen.hpp/cpp      # 通用通知对话框（单按钮：OK）
├── LoadingScreen.hpp/cpp      # 加载界面
├── InventoryScreen.hpp/cpp    # 物品栏界面
├── ContainerScreen.hpp/cpp    # 容器界面
├── LoomScreen.hpp/cpp         # 织布机界面
├── OptionsScreen.hpp/cpp      # 设置界面
├── DebugScreenWidget.hpp/cpp  # F3 调试屏幕
└── README.md                  # 本文档
```

## 内部模块关系

- `Screen` 基类继承自 `ContainerWidget`，提供模态控制、生命周期回调（`onOpen`/`onClose`）、悬停状态管理
- `TemplateScreen` 继承 `Screen`，支持从 `.tpl` 模板文件加载 UI 布局
- `ScreenManager` 管理屏幕栈生命周期（push/pop/clear）、绘制顺序、事件传播
- `DebugScreenWidget` 依赖 `ClientWorld`、`Camera`、`Player`、`ClientDimensionManager`、`DifficultyInstance`、`ParticleManager` 显示调试信息
- 各具体屏幕（`MainMenuScreen`、`PauseScreen` 等）继承 `Screen` 或 `TemplateScreen`

## 上下游外部依赖关系

**上游依赖（本模块依赖）：**
- `src/client/ui/kagero/` - Kagero UI 框架（ContainerWidget、PaintContext、TemplateCompiler 等）
- `src/client/world/` - ClientWorld、ClientEntityManager
- `src/client/dimension/` - ClientDimensionManager（维度信息显示）
- `src/client/network/` - NetworkClient（调试屏幕网络状态）
- `src/client/renderer/` - Camera、GpuInfo、CelestialCalculations、ParticleManager
- `src/common/entity/` - Player、Container、LoomContainer、DifficultyInstance
- `src/common/world/` - WorldConfig、WorldConstants、BiomeRegistry、Block

**下游依赖（被谁依赖）：**
- `src/client/application/` - ClientApplication、ClientApplicationSession、ClientApplicationBootstrap、ClientApplicationSettings、ClientApplicationTargetInfoUi

## 容易踩的坑

- 不要在 F3 界面里硬编码维度名；必须通过 `ClientDimensionManager` 取当前维度显示名。
- 调试信息里的玩家坐标应来自玩家真实位置，而不是相机摇晃后的矩阵结果。
- `DebugScreenWidget` 本地难度计算使用 `DifficultyInstance`，其中 `chunkInhabitedTime` 暂传 0（客户端区块未实现此字段），后续需补全。
- `Screen` 需要手动调用 `updateHover()` 更新子组件悬停状态。
- 模态屏幕会阻止事件向下传播，如需事件穿透设置 `screen->setModal(false)`。
