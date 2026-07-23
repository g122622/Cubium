# Minecraft UI 模块

Minecraft 游戏特定的 UI 组件和屏幕实现，基于 Kagero UI 框架构建。

## 目录结构

```
minecraft/
├── MinecraftUIContext.hpp/cpp     # UI 上下文，状态绑定和资源管理
├── UiConstants.hpp                # UI 常量定义（按钮尺寸、间距等）
├── resources/                     # UI 资源
│   ├── MinecraftTypeface.hpp/cpp  # Minecraft 字体封装
│   └── ResourceProvider.hpp/cpp   # GUI 资源提供者（纹理图集等）
├── screens/                       # 屏幕/界面
│   ├── Screen.hpp/cpp             # 屏幕基类
│   ├── TemplateScreen.hpp/cpp     # 模板驱动屏幕基类
│   ├── ScreenManager.hpp/cpp      # 屏幕栈管理
│   ├── MainMenuScreen.hpp/cpp     # 主菜单
│   ├── WorldSelectionScreen.hpp/cpp # 存档选择（含删除世界功能）
│   ├── CreateWorldScreen.hpp/cpp  # 创建世界
│   ├── PauseScreen.hpp/cpp        # 暂停菜单
│   ├── ConfirmScreen.hpp/cpp      # 通用确认对话框（双按钮）
│   ├── MessageScreen.hpp/cpp      # 通用通知对话框（单按钮）
│   ├── LoadingScreen.hpp/cpp      # 加载界面
│   ├── InventoryScreen.hpp/cpp    # 物品栏界面
│   ├── ContainerScreen.hpp/cpp    # 容器界面
│   ├── LoomScreen.hpp/cpp         # 织布机界面
│   ├── OptionsScreen.hpp/cpp      # 设置界面
│   └── DebugScreenWidget.hpp/cpp  # F3 调试屏幕
├── targetinfo/                    # 准星目标信息覆盖层
│   ├── TargetInfo.hpp/cpp         # 目标快照与格式化辅助
│   ├── TargetInfoResolver.hpp/cpp # 方块/实体目标解析
│   ├── TargetInfoWidget.hpp/cpp   # HUD 覆盖层渲染
│   └── README.md                  # 目标信息模块文档
├── widgets/                       # UI 控件
│   ├── HudWidget.hpp/cpp          # HUD 主控件（生命值、饥饿值等）
│   ├── HotbarWidget.hpp/cpp       # 快捷栏
│   ├── HealthBarWidget.hpp/cpp    # 生命值条
│   ├── HungerBarWidget.hpp/cpp    # 饥饿值条
│   ├── ExperienceBar.hpp/cpp      # 经验条
│   ├── ChatWidget.hpp/cpp         # 聊天框与命令补全
│   ├── CrosshairWidget.hpp/cpp    # 准星
│   ├── SlotWidget.hpp/cpp         # 物品槽基类
│   ├── InventorySlot.hpp/cpp      # 物品栏槽位
│   ├── ScreenStackWidget.hpp/cpp  # 屏幕栈控件
│   ├── TitleWidget.hpp/cpp        # 标题显示（/title 命令）
│   └── Viewport3DWidget.hpp/cpp   # 3D 视口控件
└── templates/                     # UI 模板文件
    ├── main_menu.tpl              # 主菜单模板
    ├── options.tpl                # 设置界面模板
    ├── pause_menu.tpl             # 暂停菜单模板
    ├── inventory.tpl              # 物品栏模板
    ├── create_world.tpl           # 创建世界模板
    ├── confirm_dialog.tpl         # 确认对话框模板
    ├── message_dialog.tpl         # 通知对话框模板
    ├── loading.tpl                # 加载界面模板
    ├── loom.tpl                   # 织布机界面模板
    └── world_selection.tpl        # 世界选择模板
```

## 内部模块关系

```
┌─────────────────────────────────────────────────────────────────┐
│                      MinecraftUIContext                          │
│              (状态绑定、资源管理、模板加载)                        │
└───────────────────────────┬─────────────────────────────────────┘
                            │
        ┌───────────────────┼───────────────────┐
        ▼                   ▼                   ▼
┌───────────────┐   ┌───────────────┐   ┌───────────────┐
│  resources/   │   │   screens/    │   │   widgets/    │
│ 字体、纹理资源 │   │ 屏幕基类、     │   │ HUD、聊天框、 │
│               │   │ 各种菜单界面   │   │ 准星、快捷栏  │
└───────────────┘   └───────┬───────┘   └───────┬───────┘
                            │                   │
                    ┌───────┴───────┐   ┌───────┴───────┐
                    │ ScreenManager │   │   HudWidget   │
                    │   屏幕栈管理   │   │ 整合各 HUD 元素│
                    └───────────────┘   └───────────────┘
                            │
                            ▼
                    ┌───────────────┐
                    │ templates/    │
                    │ XML 风格模板   │
                    └───────────────┘
```

各子模块内部关系：
- **screens/**：`Screen` 基类继承自 `ContainerWidget`，`ScreenManager` 管理屏幕栈生命周期，各具体屏幕（`MainMenuScreen`、`PauseScreen` 等）继承 `Screen`
- **widgets/**：`HudWidget` 整合 `HotbarWidget`、`HealthBarWidget`、`HungerBarWidget`、`ExperienceBar`；`ScreenStackWidget` 管理 kagero `Screen` 屏幕栈（push/pop/clear + 事件分发 + 屏幕变化回调）
- **targetinfo/**：`TargetInfoResolver` 解析方块/实体目标 → `TargetInfoSnapshot` → `TargetInfoWidget` 渲染 HUD 覆盖层

## 上下游外部依赖关系

**上游依赖（本模块依赖）：**
- `src/client/ui/kagero/` - Kagero UI 框架（Widget 基类、EventBus、StateStore、TemplateCompiler）
- `src/client/world/` - ClientWorld、ClientEntityManager
- `src/client/renderer/` - Trident 渲染器、GuiRenderer
- `src/client/chat/` - ChatHistory
- `src/client/command/` - ClientCommandManager（命令补全）
- `src/common/item/` - ItemStack

**下游依赖（被谁依赖）：**
- `src/client/application/` - ClientApplicationBootstrap 初始化并接入 UI 层

## 容易踩的坑

### 1. 模态屏幕事件传播

模态屏幕会阻止事件向下传播，导致底层控件无法接收事件。如需事件穿透，设置 `screen->setModal(false)`。

### 2. Widget 生命周期

Widget 使用原始指针引用外部资源（玩家、渲染器等），资源销毁后会导致悬空指针。必须在销毁资源前重置 Widget 的引用，例如 `hud.setPlayer(nullptr)`。

### 3. 光标闪烁同步

`ChatWidget` 的光标闪烁依赖 `tick()` 调用，游戏暂停时如果 `tick` 不更新会导致光标状态不一致。即使游戏暂停也要更新 ChatWidget 的 tick。

### 4. 屏幕栈内存管理

`ScreenStackWidget::push()` 后屏幕所有权转移给栈，外部不能再访问。如需后续访问，应在 push 前保存原始指针（注意生命周期）。

### 5. 模板路径

模板文件路径相对于工作目录，可能导致找不到文件。应使用绝对路径或确保工作目录正确。

### 6. 悬停状态更新

`Screen` 需要手动调用 `updateHover()` 更新子组件悬停状态，在输入处理中必须调用。

### 7. 聊天补全状态

`ChatWidget` 必须在命令管理器绑定后再进入交互状态，否则补全不会刷新。断开连接或切换世界后，聊天补全需要同步清空，不能继续使用旧命令树。

### 8. 调试屏幕性能

`DebugScreenWidget` 每帧收集大量数据可能影响性能。内部已做优化：FPS 每 0.5 秒更新一次，系统信息每 1 秒更新一次。

### 9. F3 调试信息

不要在 F3 界面里硬编码维度名，必须通过 `ClientDimensionManager` 取当前维度显示名。调试信息里的玩家坐标应来自玩家真实位置，而不是相机摇晃后的矩阵结果。

### 10. 目标信息模块

解析实体目标时必须使用客户端实体管理器，不能复用服务端世界接口。只在鼠标捕获时更新目标快照，否则切出 GUI 后会继续显示旧目标。玩家实体的显示名来自客户端缓存，不要假设实体本身已经携带用户名。

### 11. HUD 组件初始化

HUD 组件依赖渲染资源和玩家状态都已初始化，否则会出现空白或闪烁。

### 12. 3D 视口更新顺序

3D 视口和实体渲染器共享相机状态，更新顺序不能颠倒。
