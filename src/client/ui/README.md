# src/client/ui - 用户界面模块

本目录包含 Cubium 客户端的完整用户界面系统，包括字体渲染、Kagero UI 框架和 Minecraft 特定的 UI 组件。

## 目录结构

```
ui/
├── Font.hpp                          # 字体类定义（含 getRandomGlyph 混淆文字支持）
├── Font.cpp                          # 字体实现（含 buildWidthIndex 宽度索引）
├── FontRenderer.hpp                  # 文本渲染器（含混淆 §k 渲染支持）
├── FontRenderer.cpp                  # 文本渲染实现
├── FontTextureAtlas.hpp              # 字形纹理图集（二叉树打包算法）
├── FontTextureAtlas.cpp              # 纹理图集实现
├── Glyph.hpp                         # 字形数据结构、TextStyle（含 obfuscated 字段）、颜色常量
├── DefaultAsciiFont.hpp              # 默认ASCII位图字体（调试用）
├── DefaultAsciiFont.cpp              # 默认字体实现
├── GuiScale.hpp/cpp                  # GUI缩放计算
├── TridentCanvas.hpp                 # Trident渲染器画布实现
├── TridentCanvas.cpp                 # 画布实现（ICanvas的Vulkan实现）
├── kagero/                           # Kagero UI框架（命名空间：mc::client::ui::kagero）
│   ├── KageroEngine.hpp/cpp          # UI引擎核心（分层Widget管理、双击检测、右键分发）
│   ├── Types.hpp                     # 基础类型定义
│   ├── README.md                     # 框架文档
│   ├── docs/                         # 详细文档
│   │   ├── README.md                 # 文档索引
│   │   ├── 01-quick-start.md         # 快速开始
│   │   ├── 02-template-system.md     # 模板系统
│   │   ├── 03-state-system.md        # 状态系统
│   │   ├── 04-event-system.md        # 事件系统
│   │   └── 05-built-in-widgets.md    # 内置组件
│   ├── event/                        # 事件系统
│   │   ├── Event.hpp                 # 事件基类
│   │   ├── EventBus.hpp              # 事件总线（订阅/发布）
│   │   ├── InputEvents.hpp           # 输入事件
│   │   ├── UIEvents.hpp              # UI事件
│   │   └── WidgetEvents.hpp          # 组件事件
│   ├── state/                        # 状态管理
│   │   ├── ReactiveState.hpp         # 响应式状态
│   │   ├── StateBinding.hpp          # 状态绑定
│   │   ├── StateObserver.hpp         # 状态观察者
│   │   └── StateStore.hpp            # 全局状态存储
│   ├── layout/                       # 布局系统
│   │   ├── LayoutSystem.hpp          # 布局系统入口
│   │   ├── core/                     # 核心组件
│   │   ├── algorithms/               # 布局算法（Flex/Grid/Anchor）
│   │   ├── constraints/              # 布局约束
│   │   └── integration/              # Widget集成
│   ├── paint/                        # 绘制抽象层
│   │   ├── Color.hpp                 # 颜色定义
│   │   ├── Geometry.hpp              # 几何类型
│   │   ├── PaintContext.hpp/cpp      # 绘制上下文
│   │   ├── TextureImage.hpp/cpp      # 纹理图像
│   │   └── contracts/                # 接口定义（ICanvas/IPaint/IImage/IPath/ITextBlob/ITypeface）
│   ├── template/                     # 模板系统（声明式XML风格）
│   │   ├── Template.hpp              # 模板入口
│   │   ├── TemplateSystem.hpp/cpp    # 模板系统API
│   │   ├── parser/                   # 解析器（Lexer/Parser/Ast）
│   │   ├── compiler/                 # 编译器
│   │   ├── binder/                   # 绑定上下文
│   │   ├── runtime/                  # 运行时（TemplateInstance/UpdateScheduler）
│   │   ├── core/                     # 核心配置
│   │   └── bindings/                 # 内置绑定
│   └── widget/                       # Widget组件
│       ├── Widget.hpp                # Widget基类（含onDoubleClick/onRightClick虚方法）
│       ├── ButtonWidget.hpp          # 按钮组件
│       ├── CheckboxWidget.hpp        # 复选框组件
│       ├── ContainerWidget.hpp/cpp   # 容器组件
│       ├── IWidgetContainer.hpp      # 容器接口
│       ├── ListWidget.hpp/cpp        # 列表组件
│       ├── ScrollableWidget.hpp      # 滚动容器
│       ├── SliderWidget.hpp          # 滑块组件
│       ├── SlotWidget.hpp            # 物品槽组件
│       ├── TextFieldWidget.hpp       # 文本输入框
│       ├── TextWidget.hpp            # 文本组件
│       ├── RichTextWidget.hpp        # 富文本组件（含混淆 §k 动画支持）
│       └── Viewport3DWidget.hpp      # 3D视口组件
├── minecraft/                        # Minecraft特定UI
│   ├── MinecraftUIContext.hpp/cpp    # Minecraft UI上下文（状态/事件绑定）
│   ├── UiConstants.hpp               # UI常量定义
│   ├── resources/                    # UI资源
│   │   ├── MinecraftTypeface.hpp/cpp # Minecraft字体
│   │   └── ResourceProvider.hpp/cpp  # 资源提供者
│   ├── screens/                      # 游戏屏幕
│   │   ├── Screen.hpp/cpp            # 屏幕基类
│   │   ├── ScreenManager.hpp/cpp     # 屏幕栈管理
│   │   ├── AbstractContainerScreen.hpp # 容器屏幕基类
│   │   ├── ContainerScreen.hpp/cpp   # 容器屏幕
│   │   ├── InventoryScreen.hpp/cpp   # 背包屏幕
│   │   ├── MainMenuScreen.hpp/cpp    # 主菜单屏幕
│   │   ├── OptionsScreen.hpp/cpp     # 选项屏幕
│   │   ├── PauseScreen.hpp/cpp       # 暂停屏幕
│   │   ├── LoadingScreen.hpp/cpp     # 加载屏幕
│   │   ├── CreateWorldScreen.hpp/cpp # 创建世界屏幕
│   │   ├── WorldSelectionScreen.hpp/cpp # 世界选择屏幕
│   │   ├── DebugScreenWidget.hpp/cpp # 调试屏幕组件（F3屏幕）
│   │   ├── CraftingScreen.hpp/cpp    # 合成屏幕
│   │   ├── CreativeScreen.hpp/cpp    # 创造模式物品库屏幕
│   │   ├── LoomScreen.hpp/cpp        # 织布机屏幕
│   │   ├── CartographyScreen.hpp/cpp # 制图台屏幕
│   │   ├── FurnaceScreen.hpp/cpp     # 熔炉屏幕
│   │   ├── MapScreen.hpp/cpp         # 地图屏幕
│   │   └── TemplateScreen.hpp/cpp    # 模板屏幕基类
│   ├── targetinfo/                   # 准星目标信息覆盖层
│   │   ├── TargetInfo.hpp/cpp        # 目标快照和格式化辅助
│   │   ├── TargetInfoResolver.hpp/cpp # 方块/实体命中解析
│   │   └── TargetInfoWidget.hpp/cpp  # HUD目标提示Widget
│   ├── templates/                    # UI模板
│   │   ├── main_menu.tpl             # 主菜单模板
│   │   ├── pause_menu.tpl            # 暂停菜单模板
│   │   ├── options.tpl               # 选项模板
│   │   ├── inventory.tpl             # 背包模板
│   │   ├── loading.tpl               # 加载屏幕模板
│   │   ├── create_world.tpl          # 创建世界模板
│   │   ├── world_selection.tpl       # 世界选择模板
│   │   └── loom.tpl                  # 织布机模板
│   └── widgets/                      # Minecraft特定组件
│       ├── ChatWidget.hpp/cpp        # 聊天组件
│       ├── CrosshairWidget.hpp/cpp   # 准星组件
│       ├── ExperienceBar.hpp/cpp     # 经验条
│       ├── HealthBarWidget.hpp/cpp   # 生命值条
│       ├── HotbarWidget.hpp/cpp      # 快捷栏
│       ├── HudWidget.hpp/cpp         # HUD主组件
│       ├── HungerBarWidget.hpp/cpp   # 饥饿值条
│       ├── InventorySlot.hpp/cpp     # 背包槽位
│       ├── SlotWidget.hpp/cpp        # 槽位组件
│       ├── ScreenStackWidget.hpp/cpp # 屏幕栈组件
│       ├── TitleWidget.hpp/cpp       # 标题组件
│       └── Viewport3DWidget.hpp/cpp  # 3D视口
└── screen/                           # 屏幕系统（旧版兼容，逐步迁移到minecraft/screens）
    ├── AbstractContainerScreen.hpp
    ├── FurnaceScreen.hpp/cpp
    ├── CartographyScreen.hpp/cpp
    ├── MapScreen.hpp/cpp
    └── ScreenManager.hpp/cpp
```

## 内部模块关系

```
┌─────────────────────────────────────────────────────────────────┐
│                         ClientApplication                        │
└─────────────────────────────────────────────────────────────────┘
                                 │
                                 ▼
┌─────────────────────────────────────────────────────────────────┐
│                          ScreenManager                           │
│                    (屏幕栈管理、事件分发)                          │
└─────────────────────────────────────────────────────────────────┘
                                 │
              ┌──────────────────┼──────────────────┐
              ▼                  ▼                  ▼
┌──────────────────┐ ┌──────────────────┐ ┌──────────────────┐
│  MinecraftUIContext │ │    KageroEngine   │ │   FontRenderer   │
│  (MC业务绑定)       │ │   (UI引擎核心)     │ │   (文本渲染)      │
└──────────────────┘ └──────────────────┘ └──────────────────┘
              │                  │                  │
              ▼                  ▼                  ▼
┌──────────────────┐ ┌──────────────────┐ ┌──────────────────┐
│    Templates      │ │     Widgets       │ │      Font        │
│   (UI模板)         │ │    (UI组件)        │ │   (字体系统)      │
└──────────────────┘ └──────────────────┘ └──────────────────┘
              │                  │                  │
              ▼                  ▼                  ▼
┌──────────────────┐ ┌──────────────────┐ ┌──────────────────┐
│ BindingContext    │ │    State/Event    │ │ FontTextureAtlas │
│   (绑定上下文)      │ │  (状态/事件系统)    │ │   (字形图集)      │
└──────────────────┘ └──────────────────┘ └──────────────────┘
                                 │
                                 ▼
                     ┌──────────────────┐
                     │   TridentCanvas   │
                     │  (Vulkan绘制实现)   │
                     └──────────────────┘
                                 │
                                 ▼
                     ┌──────────────────┐
                     │   GuiRenderer     │
                     │  (GUI渲染器)       │
                     └──────────────────┘
```

**字体系统**：Font → FontTextureAtlas（字形纹理打包）→ FontRenderer（文本渲染）

**Kagero框架**：KageroEngine → Widget系统 + State系统 + Event系统 + Layout系统 + Template系统 → Paint抽象层

**Minecraft UI**：MinecraftUIContext → Screens/Widgets → Templates → Kagero框架

## 上下游外部依赖关系

### 本模块依赖的外部模块

| 模块 | 用途 |
|------|------|
| `common/core` | 基础类型、错误处理 |
| `common/util` | 工具函数、数学库 |
| `common/resource` | 资源加载（资源包） |
| `client/renderer/api` | 渲染接口（IRenderEngine等） |
| `client/renderer/trident/gui` | GUI渲染器、GuiRenderer |
| `GLFW` | 窗口和输入事件 |
| `Vulkan` | 图形渲染 |
| `spdlog` | 日志 |

### 依赖本模块的外部模块

| 模块 | 用途 |
|------|------|
| `client/ClientApplication` | 客户端主应用，初始化UI系统 |
| `client/game/ClientPlayer` | 玩家状态绑定到HUD |
| `client/input/InputManager` | 输入事件分发到UI |
| `client/renderer/GuiRenderer` | 使用FontRenderer、TridentCanvas |

## 容易踩的坑

### 1. 字形图集已满

**问题**：`FontTextureAtlas::addGlyph` 返回错误。

**原因**：字形纹理图集空间不足，无法容纳新字形。

**解决方案**：增大图集大小（`font.initialize(512)`）、预加载常用字符、或分多个图集。

### 2. 事件订阅未取消

**问题**：内存泄漏或野指针回调。

**原因**：订阅事件后忘记取消订阅。

**解决方案**：使用RAII管理订阅（`EventSubscription`），离开作用域自动取消订阅。

### 3. Widget生命周期

**问题**：访问已销毁的Widget。

**原因**：Widget被移除后仍持有指针。

**解决方案**：使用 `std::unique_ptr` 管理Widget生命周期，不要持有Widget的裸指针，使用 `Widget::WeakPtr`。

### 4. 状态更新循环

**问题**：状态更新触发观察者，观察者又更新状态，形成死循环。

**解决方案**：在观察者中检查是否需要更新，避免直接修改状态，而是发送事件。

### 5. 布局未更新

**问题**：Widget位置或大小不正确。

**原因**：未在窗口大小改变时重新布局。

**解决方案**：在 `onResize()` 中调用 `engine.resize(width, height)` 或 `widget->setBounds()`。

### 6. 模板编译错误

**问题**：模板编译失败，抛出 `TemplateError`。

**常见原因**：未闭合的标签、无效的属性名、绑定表达式语法错误。

**解决方案**：用 try-catch 捕获 `TemplateError` 并打印错误信息。

### 7. TridentCanvas限制

**问题**：圆角矩形、圆形等显示为普通矩形。

**原因**：`TridentCanvas` 是简化实现，不支持所有绘图操作。

**已知限制**：`drawRRect`/`drawCircle`/`drawOval` 退化为边界矩形，`drawPath` 忽略，`clipOutRect` 忽略并记录警告，`saveLayer`/`saveLayerAlpha` 只保存裁剪和变换状态。

### 8. 线程安全

**问题**：在非主线程访问UI组件。

**原因**：UI组件不是线程安全的。

**解决方案**：所有UI操作必须在主线程执行，使用事件总线跨线程通信。

### 9. Kagero详细文档位置

Kagero框架的详细文档不在本文件中，请参考 `kagero/docs/` 目录下的专题文档。

### 10. Kagero右键点击双重触发

右键点击（button == 1）时，`onClick` 和 `onRightClick` **都会触发**，这是有意为之的设计。组件应在 `onClick` 中检查 `button` 参数来区分左右键，或仅处理左键点击（button == 0）。

### 11. screen目录是旧版兼容

`ui/screen/` 目录是旧版屏幕系统，正在逐步迁移到 `minecraft/screens/`，新代码应使用后者。

### 12. 混淆文字（§k）宽度索引

**问题**：`Font::getRandomGlyph()` 返回 nullptr。

**原因**：`buildWidthIndex()` 尚未被调用（首次调用 `getRandomGlyph` 时会自动触发），或字体提供者为空。

**解决方案**：`getRandomGlyph` 会懒加载宽度索引，无需手动调用 `buildWidthIndex()`。若添加了新的字形提供者（`addProvider`），宽度索引会自动失效并在下次访问时重建。

**注意**：混淆文字的宽度匹配使用 `ceil(advance)` 作为整数键，空格不参与混淆替换，宽度 ≤ 0 或 > 32 的字形会被过滤。
