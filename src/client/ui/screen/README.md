# Screen 模块（旧版兼容层）

本目录承载基于 `common/screen/IScreen` 的旧版屏幕系统。当前工程已有 Kagero Widget 体系，但容器类屏幕仍依赖这里的实现。新代码应使用 `ui/minecraft/screens/` 目录。

## 目录结构

```text
src/client/ui/screen/
├── ScreenManager.hpp/cpp           # 旧版屏幕栈管理器（委托给 ScreenStackWidget）
├── AbstractContainerScreen.hpp     # 容器屏幕模板基类（槽位渲染、交互处理、拖拽、提示）
├── MapScreen.hpp/cpp               # 地图查看屏幕（全屏显示已填充地图）
└── tooltip/
    ├── BundleTooltipRenderer.hpp   # 收纳袋 tooltip 渲染器（独立工具类，被 AbstractContainerScreen 调用）
    ├── BundleTooltipRenderer.cpp   # 布局算法（无 ItemRenderer 依赖，可链接到 mc_tests）
    ├── BundleTooltipRendererRender.cpp  # 渲染主入口（依赖 ItemRenderer，仅 mc_client 构建）
    └── README.md                   # tooltip 子模块说明
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
        ▼                        ▼
┌───────────────────┐  ┌───────────────────┐
│AbstractContainerScreen│ │    MapScreen      │
│   (继承 IScreen)     │  │  (继承 IScreen)   │
└───────────────────┘  └───────────────────┘
        │
        ▼
┌───────────────────┐
│   容器菜单系统     │
│ AbstractContainerMenu│
└───────────────────┘
```

- `ScreenManager`：单例，委托给 `ScreenStackWidget` 管理屏幕栈，不直接持有屏幕
- `AbstractContainerScreen<Menu>`：容器屏幕模板基类，提供槽位渲染、交互处理、拖拽、悬停提示
- `MapScreen`：全屏地图查看，依赖 `MapRenderer` 和 `ClientMapDataCache`

## 容器交互系统

`AbstractContainerScreen<Menu>` 支持以下容器交互类型，覆盖 Minecraft Java 版的完整交互协议：

### 交互类型

| 交互 | 操作 | ClickAction | ClickType |
|------|------|-------------|-----------|
| 拾取/放置 | 左键点击槽位 | Pickup | Pick/PickSome |
| 快速移动 | Shift+左键 | QuickMove | QuickMove |
| 快捷栏交换 | 数字键1-9 / F键 | Swap | Swap |
| 丢弃 | Q键 / Ctrl+Q | Throw | Throw/ThrowAll |
| 创造模式复制 | 中键点击 | Clone | Clone |
| 拖拽分发 | 左键拖拽/右键拖拽/中键拖拽 | QuickCraft | QuickCraft |
| 双击拾取全部 | 双击槽位 | PickupAll | PickAll |
| 点击外部丢弃 | 点击容器外部 | Pickup | Pick/PickSome |

### 交互流程

1. **点击（onClick）**：检测 Shift/Ctrl 修饰键和鼠标按钮，决定交互类型
   - Shift+左键 → QuickMove（快速移动）
   - 中键 → Clone（创造模式复制）
   - 光标有物品 + 非拖拽中 + 有效槽位 → 进入拖拽模式
   - 点击外部（-999 槽位）→ 丢弃光标物品
   - 双击检测（500ms 阈值）→ PickupAll
   - 左键/右键 → Pickup（拾取/放置）

2. **释放（onRelease）**：完成拖拽分发
   - 拖拽中释放鼠标 → 调用 `_finishQuickCraft` 发送 START/ADD_SLOT/END 序列

3. **拖动（onDrag）**：累积拖拽目标槽位
   - 检查槽位是否可接受物品、是否已在列表中

4. **键盘（onKey）**：
   - ESC/E → 关闭屏幕
   - Q → 丢弃悬停槽位物品（Ctrl+Q 丢弃整组）
   - 1-9 → 与快捷栏交换
   - F → 与副手交换

### 悬停槽位追踪

`m_hoveredSlotIndex` 在每帧 `render()` 中通过 `_updateHoveredSlot()` 更新，用于键盘操作（Q键丢弃、数字键交换）。在 `onClick()` 和 `onDrag()` 中也会同步更新。

### 拖拽分发协议

拖拽操作通过三步协议发送到菜单层：
1. **START**：发送到 -999 槽位，携带拖拽模式（均匀/逐个/填满）
2. **ADD_SLOT**：发送到每个选中的槽位
3. **END**：发送到 -999 槽位，触发实际分发

按钮编码：低2位 = 事件状态（0=START, 1=ADD_SLOT, 2=END），高2位 = 拖拽模式（0=均匀, 1=逐个, 2=填满）

### 网络同步

本地模式直接调用 `m_menu->clicked()`；网络模式通过 `m_clickSender` 发送 `ContainerClickPacket`。`_sendSlotClick` 和 `_sendOutsideClick` 自动选择模式。

## 上下游外部依赖关系

### 本模块依赖

| 模块 | 用途 |
|------|------|
| `common/screen/IScreen.hpp` | 屏幕接口（onClick/onRelease/onDrag/onKey 签名含修饰键参数） |
| `common/entity/inventory/PlayerInventory.hpp` | 玩家背包 |
| `common/entity/inventory/AbstractContainerMenu.hpp` | 容器菜单基类（槽位管理、点击逻辑、拖拽协议） |
| `common/network/packet/InventoryPackets.hpp` | 容器点击包、关闭包、创造库存动作包 |
| `client/renderer/trident/gui/GuiRenderer.hpp` | GUI 渲染器 |
| `client/renderer/trident/item/ItemRenderer.hpp` | 物品渲染器 |
| `client/renderer/map/MapRenderer.hpp` | 地图渲染器（CartographyScreen、MapScreen） |
| `client/ui/minecraft/widgets/ScreenStackWidget.hpp` | 屏幕栈组件（ScreenManager 委托对象） |
| `GLFW` | 键盘和鼠标常量（GLFW_MOD_SHIFT, GLFW_KEY_Q 等） |

### 依赖本模块

| 模块 | 用途 |
|------|------|
| `client/application/ClientApplication.hpp` | 决定打开哪个屏幕，转发输入和滚轮事件 |
| `client/ui/minecraft/screens/` | 新版屏幕系统，部分屏幕复用此处的实现 |

## 容易踩的坑

- **渲染器/尺寸必须先设置**：`MapScreen` 等必须先调用 `setRenderers()` 和 `setScreenSize()` 再进入渲染，否则不会绘制任何内容。
- **MapScreen 需要地图数据缓存**：`MapScreen` 必须设置 `ClientMapDataCache` 才能显示地图内容。
- **AbstractContainerScreen 模板参数**：继承时必须指定正确的 `Menu` 类型，槽位索引和点击逻辑由菜单定义。
- **screen 目录是旧版兼容**：新屏幕应放在 `ui/minecraft/screens/`，本目录逐步迁移中。背包屏（InventoryScreen）、创造屏（CreativeScreen）、工作台屏（CraftingScreen）、箱子屏（ChestScreen）、熔炉屏（FurnaceScreen）、制图台屏（CartographyScreen）已迁移到 kagero 体系，本目录仅余地图屏（MapScreen）待迁移。
- **悬停提示渲染必须在最后**：所有屏幕的 `renderTooltip` / `_renderTooltip` 必须在 `render()` 末尾、`renderCarriedItem` / `_renderCarriedItem` 之后调用，因为 GuiRenderer 使用画家算法（后绘制覆盖先绘制），提示框必须渲染在所有其他元素之上。
- **m_hoveredSlotIndex 自动更新**：在 `render()` 每帧中通过 `_updateHoveredSlot()` 更新，键盘操作（Q键丢弃、数字键交换）依赖此索引。
- **拖拽分发需要 Player**：`_isValidDragMode` 检查 `m_playerInventory->getPlayer()` 是否非空，测试拖拽时必须用 `PlayerInventory(&player)` 构造。
- **IScreen 接口签名**：`onClick` 和 `onRelease` 含 `mods` 参数（GLFW 修饰键），`onDrag` 含 `button` 参数（鼠标按钮），所有 IScreen 子类必须匹配新签名。
- **收纳袋 tooltip 委托**：`AbstractContainerScreen::renderItemTooltip` 在检测到 `BundleItem::isBundleItem(stack)` 时，委托给 `tooltip::BundleTooltipRenderer::render`（见 `tooltip/BundleTooltipRenderer.hpp`）。该渲染器复刻 MC 1.21.11 `ClientBundleTooltip` 的 4 列网格、进度条、"+N" 溢出指示等布局，使用 GuiRenderer 纯色矩形渲染（未来升级到纹理化渲染见 tooltip/README.md 中的 TODO）。
- **Item::addInformation 接入**：两条 tooltip 路径在渲染普通物品时调用 `Item::addInformation(stack, world, lines, false)` 附加物品自定义 tooltip。`world` 为 `IWorld*`（可空），对应 MC 1.21.11 `Item.TooltipContext.of(level)` 在 `level` 为 null 时返回 EMPTY 上下文——客户端 Player 的 `world()` 为 null（`ClientWorld` 不继承 `IWorld`），此时 `addInformation` 仍被调用，但子类需跳过依赖世界的逻辑（如 `FilledMapItem` 的缩放级别提示）。
