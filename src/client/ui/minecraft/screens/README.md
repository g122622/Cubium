# Minecraft Screens 模块

本目录存放 Minecraft 风格的屏幕与调试界面实现，包括主菜单、背包、暂停菜单和 F3 调试屏幕。

## 目录结构

```text
src/client/ui/minecraft/screens/
├── Screen.hpp/cpp             # Minecraft 业务层屏幕基类
├── TemplateScreen.hpp/cpp     # 模板驱动屏幕基类
├── MainMenuScreen.hpp/cpp     # 主菜单
├── WorldSelectionScreen.hpp/cpp # 存档选择（含删除世界功能）
├── CreateWorldScreen.hpp/cpp  # 创建世界
├── PauseScreen.hpp/cpp        # 暂停菜单
├── ConfirmScreen.hpp/cpp      # 通用确认对话框（双按钮：确认/取消）
├── MessageScreen.hpp/cpp      # 通用通知对话框（单按钮：OK）
├── LoadingScreen.hpp/cpp      # 加载界面
├── InventoryScreen.hpp/cpp    # 物品栏界面（生存，继承 ContainerScreenBase）
├── CreativeScreen.hpp/cpp     # 创造物品库界面（继承 ContainerScreenBase + ItemPickerMenu）
├── CraftingScreen.hpp/cpp     # 工作台界面（3x3 合成，继承 ContainerScreenBase）
├── ChestScreen.hpp/cpp        # 箱子界面（单/双箱，动态高度，继承 ContainerScreenBase）
├── FurnaceScreen.hpp/cpp      # 熔炉界面（火焰/箭头动画，继承 ContainerScreenBase）
├── CartographyScreen.hpp/cpp  # 制图台界面（地图复制/扩展/锁定，继承 ContainerScreenBase）
├── ContainerScreenBase.hpp    # 容器屏共享基类（槽位布局/渲染/交互转发/居中定位）
├── ContainerScreen.hpp/cpp    # 容器界面
├── LoomScreen.hpp/cpp         # 织布机界面
├── OptionsScreen.hpp/cpp      # 设置界面
├── SignEditScreen.hpp/cpp     # 告示牌编辑界面（4行文本输入）
├── DebugScreenWidget.hpp/cpp  # F3 调试屏幕
└── README.md                  # 本文档
```

## 内部模块关系

- `Screen` 基类继承自 `ContainerWidget`，提供模态控制、生命周期回调（`onOpen`/`onClose`）、悬停状态管理
- `TemplateScreen` 继承 `Screen`，支持从 `.tpl` 模板文件加载 UI 布局
- `ScreenManager` 管理屏幕栈生命周期（push/pop/clear）、绘制顺序、事件传播
- `DebugScreenWidget` 依赖 `ClientWorld`、`Camera`、`Player`、`ClientDimensionManager`、`DifficultyInstance`、`ParticleManager` 显示调试信息
- `SignEditScreen` 使用 `TextFieldWidget` 实现4行文本编辑，支持 Tab/Shift+Tab 切换行、Enter 提交、ESC 取消；通过回调与网络层交互（发送 `UpdateSignPacket`）
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
- **熔炉进度同步**：客户端无熔炉方块实体（`FurnaceContainer::getFurnaceEntity()` 为 nullptr），火焰/箭头进度不能读实体。`FurnaceContainer` 把燃烧/熔炼进度绑到 tracked int 的独立存储成员，服务端每 tick 经 `syncProgressFromEntity()` + `detectAndSendChanges()` 经 `WindowPropertyPacket` 下推，客户端 `onWindowProperty` 回调 `setTrackedInt` 写入，`FurnaceScreen` 读 `getLitProgress()`/`getBurnProgress()` 驱动动画。火焰可见高度 = `ceil(litProgress × 13.0) + 1`，箭头可见宽度 = `ceil(burnProgress × 24.0)`。
- **制图台结果槽由服务端计算**：客户端 `CartographyContainer` 构造时 `world=nullptr`，`updateResult()` 空操作，不计算地图缩放/锁定/复制结果。结果槽内容由服务端 `CartographyContainer::updateResult()` 计算后经 `ContainerSlotPacket` 下推到客户端，客户端 `onContainerSlot` 写入并 `syncSlots()`。制图台网络回调（`onOpenContainer`/`onContainerContent`/`onContainerSlot`/`onCloseContainer`）均走 kagero `CartographyScreen` 分支。
- **制图台地图预览未注入**：`CartographyScreen` 结果槽为已填充地图时绘制 64×64 地图预览（`MapRenderer::renderMap`），但 `MapRenderer` 当前无所有者、`setMapRenderer` 未被调用，预览暂不渲染（待地图渲染器统一接入后修复）。`m_mapRenderer` 为 nullptr 时安全跳过。
- **地图查看屏待 Phase6 重建**：原 `MapScreen`（玩家使用已填充地图时的全屏地图查看屏）为死代码（零构造、无开屏入口）已删；`MapRenderer`/地图数据缓存在客户端无 owner 注入点，地图内容暂不渲染。地图数据网络同步走 IR `ir::play::MapItemData`（当前 Phase6 TODO，opaque no-op），待该链路重建后恢复地图查看屏。
