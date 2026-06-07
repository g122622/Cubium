# Screen 模块（旧版兼容层）

本目录承载基于 `common/screen/IScreen` 的旧版屏幕系统。当前工程已有 Kagero Widget 体系，但容器类屏幕仍依赖这里的实现。新代码应使用 `ui/minecraft/screens/` 目录。

## 目录结构

```text
src/client/ui/screen/
├── ScreenManager.hpp/cpp           # 旧版屏幕栈管理器（委托给 ScreenStackWidget）
├── AbstractContainerScreen.hpp     # 容器屏幕模板基类（槽位渲染、点击处理、拖拽、提示）
├── CraftingScreen.hpp/cpp          # 工作台屏幕（CraftingScreen 3x3）和玩家背包屏幕（InventoryCraftingScreen 2x2）
├── ChestScreen.hpp/cpp             # 箱子屏幕（支持多行箱子）
├── FurnaceScreen.hpp/cpp           # 熔炉屏幕（燃料、原料、结果槽位）
├── CartographyScreen.hpp/cpp       # 制图台屏幕（地图复制、扩展、锁定）
├── MapScreen.hpp/cpp               # 地图查看屏幕（全屏显示已填充地图）
└── CreativeScreen.hpp/cpp          # 创造模式物品库屏幕（搜索、滚动、垃圾槽、背包编辑）
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
│              （单例，委托 ScreenStackWidget 管理屏幕栈）            │
└─────────────────────────────────────────────────────────────────┘
                                 │
        ┌────────────────────────┼────────────────────────┐
        ▼                        ▼                        ▼
┌───────────────────┐  ┌───────────────────┐  ┌───────────────────┐
│   CreativeScreen   │  │AbstractContainerScreen│ │    MapScreen      │
│  (继承 IScreen)    │  │   (继承 IScreen)     │  │  (继承 IScreen)   │
└───────────────────┘  └───────────────────┘  └───────────────────┘
        │                        │
        ▼                        ▼
┌───────────────────┐  ┌───────────────────┐
│  PlayerInventory   │  │   容器菜单系统     │
│ CreativeInventory  │  │ AbstractContainerMenu│
└───────────────────┘  └───────────────────┘
```

- `ScreenManager`：单例，委托给 `ScreenStackWidget` 管理屏幕栈，不直接持有屏幕
- `AbstractContainerScreen<Menu>`：容器屏幕模板基类，提供槽位渲染、点击处理、拖拽、悬停提示
- `CreativeScreen`：直接继承 `IScreen`，不走容器点击流，直接编辑 `PlayerInventory` 并发送 `CreativeInventoryActionPacket`
- `MapScreen`：全屏地图查看，依赖 `MapRenderer` 和 `ClientMapDataCache`

## 上下游外部依赖关系

### 本模块依赖

| 模块 | 用途 |
|------|------|
| `common/screen/IScreen.hpp` | 屏幕接口 |
| `common/entity/inventory/PlayerInventory.hpp` | 玩家背包 |
| `common/entity/inventory/CreativeInventory.hpp` | 创造物品列表 |
| `common/entity/inventory/AbstractContainerMenu.hpp` | 容器菜单基类 |
| `common/network/packet/InventoryPackets.hpp` | 容器点击包、关闭包、创造库存动作包 |
| `client/renderer/trident/gui/GuiRenderer.hpp` | GUI 渲染器 |
| `client/renderer/trident/item/ItemRenderer.hpp` | 物品渲染器 |
| `client/renderer/map/MapRenderer.hpp` | 地图渲染器（CartographyScreen、MapScreen） |
| `client/ui/minecraft/widgets/ScreenStackWidget.hpp` | 屏幕栈组件（ScreenManager 委托对象） |
| `GLFW` | 键盘和鼠标常量 |

### 依赖本模块

| 模块 | 用途 |
|------|------|
| `client/application/ClientApplication.hpp` | 决定打开哪个屏幕，转发输入和滚轮事件 |
| `client/ui/minecraft/screens/` | 新版屏幕系统，部分屏幕复用此处的实现 |

## 容易踩的坑

- **渲染器/尺寸必须先设置**：`CreativeScreen`、`MapScreen` 等必须先调用 `setRenderers()` 和 `setScreenSize()` 再进入渲染，否则不会绘制任何内容。
- **创造屏幕不暂停游戏**：`CreativeScreen.isPauseScreen()` 返回 `false`。
- **E 键行为差异**：E 键在创造模式下不是”关闭背包”，而是进入创造库存界面；模式切换时要由上层决定应该打开哪个屏幕。
- **滚轮事件必须显式转发**：滚轮事件需通过 `ScreenManager.onScroll()` 转发到当前屏幕，否则创造物品库的滚动区不会响应。
- **创造库存依赖注册顺序**：创造库存条目依赖完整的方块和物品注册顺序，测试或启动时必须先初始化 `VanillaBlocks`，再初始化 `Items` 和 `BlockItemRegistry`。
- **MapScreen 需要地图数据缓存**：`MapScreen` 必须设置 `ClientMapDataCache` 才能显示地图内容。
- **AbstractContainerScreen 模板参数**：继承时必须指定正确的 `Menu` 类型，槽位索引和点击逻辑由菜单定义。
- **screen 目录是旧版兼容**：新屏幕应放在 `ui/minecraft/screens/`，本目录逐步迁移中。
