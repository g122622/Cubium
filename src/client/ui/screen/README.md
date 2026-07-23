# Screen 模块（旧版兼容层）

本目录承载基于 `common/screen/IScreen` 的旧版屏幕系统。容器类屏幕（背包/创造/工作台/箱子/熔炉/制图台）已全部迁移到 kagero 体系（`ui/minecraft/screens/`），本目录仅余 `ScreenManager` 与 `MapScreen` 待 MapScreen 迁移 kagero 后清空。新代码应使用 `ui/minecraft/screens/` 目录。

## 目录结构

```text
src/client/ui/screen/
├── ScreenManager.hpp/cpp           # 旧版屏幕栈管理器（委托给 ScreenStackWidget）
└── MapScreen.hpp/cpp               # 地图查看屏幕（全屏显示已填充地图，继承 IScreen）
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
                                 ▼
┌───────────────────┐
│    MapScreen      │
│  (继承 IScreen)   │
└───────────────────┘
```

- `ScreenManager`：单例，委托给 `ScreenStackWidget` 管理屏幕栈，不直接持有屏幕
- `MapScreen`：全屏地图查看，依赖 `MapRenderer` 和 `ClientMapDataCache`，是本目录最后一个 IScreen 屏

## 上下游外部依赖关系

### 本模块依赖

| 模块 | 用途 |
|------|------|
| `common/screen/IScreen.hpp` | 屏幕接口（onClick/onRelease/onDrag/onKey 签名含修饰键参数） |
| `client/renderer/trident/gui/GuiRenderer.hpp` | GUI 渲染器 |
| `client/renderer/map/MapRenderer.hpp` | 地图渲染器 |
| `client/ui/minecraft/widgets/ScreenStackWidget.hpp` | 屏幕栈组件（ScreenManager 委托对象） |
| `GLFW` | 键盘和鼠标常量（GLFW_MOD_SHIFT, GLFW_KEY_Q 等） |

### 依赖本模块

| 模块 | 用途 |
|------|------|
| `client/application/ClientApplication.hpp` | 决定打开哪个屏幕，转发输入和滚轮事件 |
| `client/ui/minecraft/screens/` | kagero 屏幕系统，容器屏已全部迁移至此 |

## 容易踩的坑

- **渲染器/尺寸必须先设置**：`MapScreen` 必须先调用 `setRenderers()` 和 `setScreenSize()` 再进入渲染，否则不会绘制任何内容。
- **MapScreen 需要地图数据缓存**：`MapScreen` 必须设置 `ClientMapDataCache` 才能显示地图内容。
- **screen 目录是旧版兼容**：新屏幕应放在 `ui/minecraft/screens/`。容器屏（InventoryScreen/CreativeScreen/CraftingScreen/ChestScreen/FurnaceScreen/CartographyScreen）已迁移到 kagero 体系，本目录仅余 MapScreen 待迁移。
- **IScreen 接口签名**：`onClick` 和 `onRelease` 含 `mods` 参数（GLFW 修饰键），`onDrag` 含 `button` 参数（鼠标按钮），所有 IScreen 子类必须匹配新签名。
- **容器交互在 kagero 体系**：容器交互系统（拾取/快速移动/拖拽分发/丢弃/中键克隆等）位于 `ui/minecraft/screens/ContainerScreenBase.hpp` + `ui/kagero/widget/ContainerInteraction.hpp`，本目录不再承载容器交互逻辑。
