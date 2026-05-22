# Client Module

Minecraft Reborn 客户端模块，负责游戏客户端的所有功能，包括渲染、UI、网络、输入处理、命令补全和世界管理。

## 目录结构

```
src/client/
├── application/            # 应用程序入口
│   ├── ClientApplication.hpp/cpp  # 客户端主应用类
│   └── ClientLaunchParams         # 启动参数配置
├── chat/                   # 聊天系统
│   └── ChatHistory.hpp/cpp # 聊天历史记录管理
├── command/                # 客户端命令树与补全
│   └── ClientCommandManager.hpp/cpp # 本地命令树快照和建议生成
├── input/                  # 输入管理
│   └── InputManager.hpp/cpp # 键盘/鼠标输入处理
├── main.cpp               # 程序入口点
├── network/               # 网络通信
│   └── NetworkClient.hpp/cpp # TCP/本地连接客户端
├── sound/                 # 音频系统
│   ├── AudioService.hpp/cpp  # 独立音频线程入口
│   ├── SoundEngine.hpp/cpp   # 声音播放核心
│   ├── SoundHandler.hpp/cpp  # sounds.json 解析与注册
│   └── ...                   # 缓冲区、音乐、环境音、后端
├── renderer/              # 渲染系统
│   ├── api/               # 平台无关渲染接口
│   │   ├── IRenderEngine.hpp    # 渲染引擎接口
│   │   ├── Types.hpp/cpp        # 顶点/网格类型定义
│   │   ├── BlendMode.hpp        # 混合模式
│   │   ├── CompareOp.hpp        # 深度比较操作
│   │   ├── CullMode.hpp         # 面剔除模式
│   │   ├── buffer/              # 缓冲区接口
│   │   ├── camera/              # 相机接口
│   │   ├── mesh/                # 网格数据
│   │   ├── pipeline/            # 管线/渲染状态
│   │   └── texture/             # 纹理接口
│   ├── trident/           # Trident Vulkan 渲染引擎
│   │   ├── core/          # 核心组件
│   │   │   ├── TridentContext.hpp/cpp   # Vulkan 上下文
│   │   │   ├── TridentEngine.hpp/cpp    # 渲染引擎主类
│   │   │   ├── TridentSwapchain.hpp/cpp # 交换链
│   │   │   ├── buffer/          # 缓冲区实现
│   │   │   ├── pipeline/        # 管线实现
│   │   │   ├── render/          # 渲染通道/帧管理
│   │   │   └── texture/         # 纹理实现
│   │   ├── chunk/         # 区块渲染器
│   │   │   ├── ChunkRenderer.hpp/cpp    # 区块 GPU 缓冲管理
│   │   │   ├── ChunkMesher.hpp/cpp      # 区块网格生成
│   │   │   └── AmbientOcclusionCalculator.hpp/cpp # AO 计算
│   │   ├── cloud/         # 云渲染器
│   │   │   └── CloudRenderer.hpp/cpp
│   │   ├── sky/           # 天空渲染器
│   │   │   ├── SkyRenderer.hpp/cpp
│   │   │   └── CelestialCalculations.hpp/cpp # 天体计算
│   │   ├── fog/           # 雾效果
│   │   │   └── FogManager.hpp/cpp
│   │   ├── gui/           # GUI 渲染器
│   │   │   ├── GuiRenderer.hpp/cpp
│   │   │   ├── GuiTextureAtlas.hpp/cpp
│   │   │   ├── GuiSpriteAtlas.hpp/cpp
│   │   │   ├── GuiSpriteManager.hpp/cpp
│   │   │   └── ...
│   │   ├── item/          # 物品渲染器
│   │   │   └── ItemRenderer.hpp/cpp
│   │   ├── entity/        # 实体渲染器
│   │   │   ├── EntityRenderer.hpp/cpp
│   │   │   ├── EntityRendererManager.hpp/cpp
│   │   │   ├── EntityPipeline.hpp/cpp
│   │   │   ├── EntityTextureAtlas.hpp/cpp
│   │   │   ├── ItemEntityRenderer.hpp/cpp
│   │   │   └── model/     # 实体模型
│   │   ├── particle/      # 粒子系统
│   │   │   ├── Particle.hpp/cpp
│   │   │   ├── ParticleManager.hpp/cpp
│   │   │   └── particles/ # 粒子类型
│   │   ├── weather/       # 天气渲染器
│   │   │   └── WeatherRenderer.hpp/cpp
│   │   ├── block/         # 方块破坏效果
│   │   │   ├── BreakProgressManager.hpp/cpp
│   │   │   └── BreakProgressRenderer.hpp/cpp
│   │   └── util/          # Vulkan 工具
│   │       └── VulkanUtils.hpp
│   ├── mesh/              # 网格调度与执行
│   │   ├── MeshWorkerPool.hpp/cpp
│   │   └── MeshBuildScheduler.hpp/cpp
│   ├── Camera.hpp/cpp     # 第一人称相机控制器
│   └── MeshTypes.hpp/cpp  # 网格数据类型
├── resource/              # 资源加载
│   ├── ResourceManager.hpp/cpp       # 资源管理器
│   ├── BlockModelLoader.hpp/cpp      # 方块模型加载器
│   ├── BlockStateLoader.hpp/cpp      # 方块状态加载器
│   ├── BlockModelCache.hpp/cpp       # 模型缓存
│   ├── TextureAtlasBuilder.hpp/cpp   # 纹理图集构建器
│   ├── ItemTextureAtlas.hpp/cpp      # 物品纹理图集
│   ├── EntityTextureLoader.hpp/cpp   # 实体纹理加载器
│   └── DestroyStageTextures.hpp/cpp  # 破坏阶段纹理
├── settings/              # 客户端设置
│   └── ClientSettings.hpp/cpp
├── ui/                    # 用户界面
│   ├── Font.hpp/cpp       # 字体系统
│   ├── FontRenderer.hpp/cpp
│   ├── FontTextureAtlas.hpp/cpp
│   ├── DefaultAsciiFont.hpp/cpp
│   ├── Glyph.hpp          # 字形定义
│   ├── TridentCanvas.hpp/cpp # Kagero UI 的 Vulkan 画布
│   ├── screen/            # 屏幕管理
│   │   ├── README.md
│   │   ├── ScreenManager.hpp/cpp
│   │   ├── AbstractContainerScreen.hpp
│   │   ├── CraftingScreen.hpp/cpp
│   │   └── CreativeScreen.hpp/cpp
│   ├── kagero/            # Kagero UI 框架
│   │   ├── KageroEngine.hpp/cpp  # UI 引擎主类
│   │   ├── Types.hpp              # 类型定义
│   │   ├── event/                 # 事件系统
│   │   │   ├── Event.hpp
│   │   │   ├── EventBus.hpp
│   │   │   ├── InputEvents.hpp
│   │   │   ├── UIEvents.hpp
│   │   │   └── WidgetEvents.hpp
│   │   ├── layout/        # 布局系统
│   │   │   ├── LayoutSystem.hpp
│   │   │   ├── algorithms/    # 布局算法
│   │   │   │   ├── FlexLayout.hpp/cpp
│   │   │   │   ├── GridLayout.hpp/cpp
│   │   │   │   └── AnchorLayout.hpp/cpp
│   │   │   ├── constraints/   # 布局约束
│   │   │   ├── core/          # 布局引擎
│   │   │   └── integration/   # Widget 适配器
│   │   ├── paint/         # 绘图抽象
│   │   │   ├── Color.hpp
│   │   │   ├── Geometry.hpp/cpp
│   │   │   ├── PaintContext.hpp/cpp
│   │   │   ├── TextureImage.hpp/cpp
│   │   │   └── contracts/     # 接口定义
│   │   │       ├── ICanvas.hpp
│   │   │       ├── IImage.hpp
│   │   │       ├── IPaint.hpp
│   │   │       ├── IPath.hpp
│   │   │       └── ISurface.hpp
│   │   ├── state/         # 状态管理
│   │   │   ├── ReactiveState.hpp
│   │   │   ├── StateBinding.hpp
│   │   │   ├── StateObserver.hpp
│   │   │   └── StateStore.hpp
│   │   ├── template/      # 模板系统
│   │   │   ├── Template.hpp
│   │   │   ├── TemplateSystem.hpp/cpp
│   │   │   ├── binder/        # 绑定上下文
│   │   │   ├── bindings/      # 内置绑定
│   │   │   ├── compiler/      # 模板编译器
│   │   │   ├── core/          # 模板核心
│   │   │   ├── parser/        # AST/词法/语法分析
│   │   │   └── runtime/       # 运行时
│   │   ├── widget/        # Widget 组件
│   │   │   ├── Widget.hpp          # 基类
│   │   │   ├── ButtonWidget.hpp
│   │   │   ├── CheckboxWidget.hpp
│   │   │   ├── ContainerWidget.hpp/cpp
│   │   │   ├── ListWidget.hpp
│   │   │   ├── ScrollableWidget.hpp
│   │   │   ├── SliderWidget.hpp
│   │   │   ├── SlotWidget.hpp
│   │   │   ├── TextFieldWidget.hpp
│   │   │   ├── TextWidget.hpp
│   │   │   ├── Viewport3DWidget.hpp
│   │   │   └── IWidgetContainer.hpp
│   │   └── docs/          # 文档
│   └── minecraft/         # Minecraft UI 业务层
│       ├── MinecraftUIContext.hpp/cpp
│       ├── resources/     # UI 资源
│       │   ├── MinecraftTypeface.hpp/cpp
│       │   └── ResourceProvider.hpp/cpp
│       ├── screens/       # 游戏屏幕
│       │   ├── Screen.hpp/cpp
│       │   ├── ScreenManager.hpp/cpp
│       │   ├── MainMenuScreen.hpp/cpp
│       │   ├── InventoryScreen.hpp/cpp
│       │   ├── ContainerScreen.hpp/cpp
│       │   ├── PauseScreen.hpp/cpp
│       │   ├── OptionsScreen.hpp/cpp
│       │   └── DebugScreenWidget.hpp/cpp
│       ├── templates/     # UI 模板文件
│       │   ├── main_menu.tpl
│       │   ├── pause_menu.tpl
│       │   ├── options.tpl
│       │   └── inventory.tpl
│       └── widgets/       # Minecraft 特定 Widget
│           ├── CrosshairWidget.hpp/cpp
│           ├── HotbarWidget.hpp/cpp
│           ├── HealthBarWidget.hpp/cpp
│           ├── HungerBarWidget.hpp/cpp
│           ├── ExperienceBar.hpp/cpp
│           ├── ChatWidget.hpp/cpp
│           ├── HudWidget.hpp/cpp
│           ├── InventorySlot.hpp/cpp
│           ├── SlotWidget.hpp/cpp
│           ├── ScreenStackWidget.hpp/cpp
│           └── Viewport3DWidget.hpp/cpp
├── window/                # 窗口管理
│   └── Window.hpp/cpp     # GLFW 窗口封装
└── world/                 # 客户端世界
    ├── ClientWorld.hpp/cpp      # 客户端世界管理
    ├── ClientWeather.hpp        # 客户端天气状态
    ├── color/                   # 颜色解析系统
    │   ├── ColorResolver.hpp    # 颜色解析器接口
    │   ├── BiomeColors.hpp/cpp  # 生物群系颜色解析器
    │   └── README.md            # 模块文档
    └── entity/
        ├── ClientEntity.hpp/cpp       # 客户端实体
        └── ClientEntityManager.hpp/cpp # 客户端实体管理
```

## 模块职责

### 1. application/ - 应用程序入口

**职责**: 客户端应用的生命周期管理、初始化、主循环、关闭。

**核心类**:
- `ClientApplication`: 主应用类，协调所有子系统
- `ClientLaunchParams`: 启动参数配置（窗口大小、服务器地址等）

**主循环架构**:

客户端主循环采用固定时间步长 (fixed timestep) 物理更新 + 可变帧率渲染的架构：

```
主循环每帧:
1. 计算 deltaTime（帧间隔时间）
2. handleEvents() - 处理输入事件
3. update(deltaTime) - 更新游戏逻辑
   ├── 玩家物理更新（20 TPS 固定步长）
   │   └── m_playerPhysicsAccumulator 累积时间
   ├── 计算 partialTick
   │   └── partialTick = accumulator / TICK_DURATION (范围 [0.0, 1.0))
   ├── 相机位置插值（使用 partialTick）
   ├── 实体 tick 和插值更新
   │   ├── entityManager.tick() - 每 tick 调用
   │   ├── entityManager.updateInterpolation(deltaTime) - 每帧平滑插值
   │   └── entityManager.updateAnimations(partialTick) - 动画插值
   ├── 世界更新（区块网格构建）
   ├── 时间和天气同步
   └── UI 状态更新（传递 partialTick）
4. render() - 渲染帧
5. input.endFrame() - 清理瞬时输入状态
```

**partialTick（部分 tick）**:

`partialTick` 是一个 `[0.0, 1.0)` 范围的浮点值，表示当前渲染帧在两个游戏 tick 之间的位置：
- `0.0` = 刚完成一个 tick
- `0.5` = 距离下一个 tick 还有一半时间
- `~1.0` = 即将执行下一个 tick

**用途**:
- 实体位置插值：`getInterpolatedPosition(partialTick)` 使实体移动平滑
- 相机位置插值：玩家视角平滑跟随
- 动画插值：行走周期、攻击挥动等动画
- GUI 动画：屏幕过渡效果
- 天气效果：雨雪粒子动画
- 方块实体动画：箱子开合、活塞移动

**使用方法**:
```cpp
mc::client::ClientApplication app;
mc::client::ClientLaunchParams params;
params.windowWidth = 1920;
params.windowHeight = 1080;
params.username = "Player";

auto result = app.initialize(params);
if (result.success()) {
    app.run();
}
```

### 2. sound/ - 音频系统

**职责**: 通过 `AudioService` 管理独立音频线程，异步处理播放、停止、暂停、恢复、listener 更新、声音定义重载、音乐和环境音。

**关键约束**:
- `ClientApplication` 只持有 `AudioService`，不直接持有 `SoundEngine`
- `SoundEngine` 的所有 OpenAL 调用必须在音频线程内执行
- 音频资源直接共享 `common/resource/ResourcePackList`

**使用方法**:
```cpp
auto audioService = std::make_unique<sound::AudioService>(resourcePackList, settings);
auto initResult = audioService->initialize();
if (initResult.success()) {
    audioService->play(std::make_unique<sound::SoundInstance>(...));
}
```

### 3. renderer/ - 渲染系统

**职责**: 所有图形渲染功能，包括区块、实体、天空、GUI、粒子等。

#### 2.1 api/ - 平台无关渲染接口

**职责**: 定义平台无关的渲染 API 抽象，支持未来扩展到其他图形后端。

**核心接口**:
- `IRenderEngine`: 渲染引擎主接口
- `IVertexBuffer`/`IIndexBuffer`/`IUniformBuffer`: 缓冲区接口
- `ITexture`/`ITextureAtlas`: 纹理接口
- `ICamera`: 相机接口
- `RenderType`: 渲染类型（solid、cutout、translucent 等）

#### 2.2 trident/ - Vulkan 渲染引擎

**职责**: Vulkan 图形 API 的完整实现。

**核心组件**:
- `TridentEngine`: 渲染引擎主类，实现 `IRenderEngine`
- `TridentContext`: Vulkan 实例、设备、队列管理
- `TridentSwapchain`: 交换链管理
- `TridentPipeline`: 图形管线封装

**子渲染器**:
| 渲染器 | 文件 | 职责 |
|--------|------|------|
| ChunkRenderer | `chunk/` | 区块网格 GPU 缓冲管理、渲染 |
| ChunkMesher | `chunk/` | 区块网格生成（顶点、索引、AO） |
| SkyRenderer | `sky/` | 天空盒、太阳、月亮、星星 |
| CloudRenderer | `cloud/` | 云层渲染（Fast/Fancy 模式） |
| FogManager | `fog/` | 雾效果（线性/指数） |
| GuiRenderer | `gui/` | GUI 元素渲染 |
| ItemRenderer | `item/` | 物品图标渲染 |
| EntityRenderer | `entity/` | 实体渲染（动物、怪物等） |
| ParticleManager | `particle/` | 粒子系统管理 |
| WeatherRenderer | `weather/` | 雨/雪天气效果 |
| BreakProgressRenderer | `block/` | 方块破坏动画 |

**渲染流程**:
```
TridentEngine::render()
  beginFrame()
    skyRenderer.render()       // 天空（背景）
    cloudRenderer.render()     // 云
    chunkRenderer.render()     // 区块（实心）
    entityRenderCallback()     // 实体
    chunkRenderer.render()     // 区块（透明）
    weatherRenderer.render()   // 天气粒子
    guiRenderer.render()       // GUI
    breakProgressRenderer.render() // 破坏动画
  endFrame()
  present()
```

#### 2.3 mesh/ - 网格调度与执行

**职责**: 为区块网格构建提供“策略层 + 执行层”拆分架构，降低主线程卡顿并提升有效构建命中率。

**核心类**:
- `MeshBuildScheduler`: 独立调度器（视锥优先、距离优先、过期取消、代际管理）
- `MeshWorkerPool`: 纯执行线程池（FIFO 执行 + 协作取消）
- `MeshWorkerTask` / `MeshWorkerResult`: 执行输入输出结构

**使用方法**:
```cpp
MeshWorkerPool workerPool(-1);
workerPool.start();

MeshSchedulerConfig schedulerConfig;
schedulerConfig.maxDispatchedTaskCount = 64;
schedulerConfig.reprioritizeIntervalFrames = 6;
schedulerConfig.cameraMoveThreshold = 2.0f;
schedulerConfig.cameraDirectionDotThreshold = 0.96f;
schedulerConfig.behindCancelDotThreshold = -0.35f;
schedulerConfig.behindCancelDistanceChunks = 8.0f;

MeshBuildScheduler scheduler(workerPool, schedulerConfig);

// 每帧更新视图状态并驱动调度
scheduler.setViewState(viewState);
scheduler.tick();

// 每帧回收已完成结果
scheduler.drainCompleted([](MeshWorkerResult&& result) {
    // 更新 GPU 缓冲区
}, 4);
```

### 3. resource/ - 资源加载

**职责**: 加载和管理资源包、方块模型、纹理等。

**核心类**:
- `ResourceManager`: 资源管理主类
- `BlockModelLoader`: JSON 模型加载器
- `BlockStateLoader`: 方块状态定义加载器
- `BlockModelCache`: 已烘焙模型缓存
- `TextureAtlasBuilder`: 纹理图集构建器
- `ItemTextureAtlas`: 物品纹理图集
- `EntityTextureLoader`: 实体纹理加载器

**使用方法**:
```cpp
ResourceManager manager;
manager.addResourcePack(pack);
manager.loadAllResources();

// 获取方块外观
auto appearance = manager.getBlockAppearance(
    ResourceLocation("minecraft:stone"), {});

// 获取纹理区域
auto region = manager.getTextureRegion(
    ResourceLocation("minecraft:block/stone"));

// 构建纹理图集
auto atlas = manager.buildTextureAtlas();
```

### 4. ui/ - 用户界面

**职责**: 游戏 UI 系统，包括 HUD、菜单、物品栏等。

#### 4.1 kagero/ - Kagero UI 框架

**职责**: 自研 UI 框架，提供声明式 UI、响应式状态、模板系统。

**命名由来**: Kagero（陽炎），取自《火焰纹章：命运》，意为"阳炎"或"蜉蝣"，象征 UI 系统的轻盈灵动。

**核心模块**:
- **event/**: 事件系统（EventBus、输入事件、Widget 事件）
- **layout/**: 布局系统（Flex、Grid、Anchor）
- **paint/**: 绘图抽象（Canvas、Paint、Path）
- **state/**: 响应式状态（ReactiveState、StateStore）
- **template/**: 模板系统（Lexer、Parser、Compiler、Runtime）
- **widget/**: Widget 组件（Button、Checkbox、List、TextField 等）

**使用方法**:
```cpp
// 创建 UI 引擎
KageroEngine engine;
engine.initialize(canvas);

// 创建 Widget
auto button = std::make_shared<ButtonWidget>();
button->setText("Click Me");
button->setOnClick([]() { /* handle click */ });

// 添加到场景
engine.addWidget(button);

// 更新和渲染
engine.update(deltaTime);
engine.paint();
```

#### 4.2 minecraft/ - Minecraft UI 业务层

**职责**: Minecraft 特定的 UI 实现。

**屏幕**:
- `MainMenuScreen`: 主菜单
- `InventoryScreen`: 物品栏
- `ContainerScreen`: 容器界面（箱子、熔炉等）
- `CreativeScreen`: 创造模式物品库
- `PauseScreen`: 暂停菜单
- `OptionsScreen`: 设置界面
- `DebugScreenWidget`: 调试屏幕

**HUD Widget**:
- `CrosshairWidget`: 准星
- `HotbarWidget`: 快捷栏
- `HealthBarWidget`: 生命值条
- `HungerBarWidget`: 饥饿值条
- `ExperienceBar`: 经验条
- `ChatWidget`: 聊天框

### 5. network/ - 网络通信

**职责**: 与服务端的网络通信，支持 TCP 和本地连接。

**核心类**:
- `NetworkClient`: 网络客户端
- `ClientState`: 连接状态（Disconnected、Connecting、Playing 等）
- `NetworkClientCallbacks`: 事件回调

**使用方法**:
```cpp
NetworkClient client;

NetworkClientCallbacks callbacks;
callbacks.onConnected = []() { /* connected */ };
callbacks.onChunkData = [](ChunkCoord x, ChunkCoord z, const std::vector<u8>& data) {
    // 处理区块数据
};
callbacks.onPlayerSpawn = [](PlayerId id, const std::string& name, f64 x, f64 y, f64 z) {
    // 玩家生成
};

client.setCallbacks(callbacks);

// 连接远程服务器
NetworkClientConfig config;
config.serverAddress = "127.0.0.1";
config.serverPort = 25565;
config.username = "Player";
client.connect(config);

// 或连接内置服务器
client.connectLocal(localEndpoint);

// 主循环
while (running) {
    client.poll();
}

client.disconnect("Quit");
```

### 6. input/ - 输入管理

**职责**: 键盘和鼠标输入处理。

**核心类**:
- `InputManager`: 输入管理器

**功能**:
- 按键状态查询（isKeyPressed、isKeyJustPressed）
- 鼠标按键状态查询
- 鼠标位置和增量
- 滚轮输入
- 鼠标锁定模式
- 按键绑定和动作回调
- 字符输入回调

**使用方法**:
```cpp
InputManager input;
input.initialize(window);

// 每帧更新
input.update();

// 查询按键状态
if (input.isKeyPressed(GLFW_KEY_W)) {
    player.moveForward();
}

if (input.isKeyJustPressed(GLFW_KEY_SPACE)) {
    player.jump();
}

// 鼠标增量
f64 deltaX = input.mouseDeltaX();
f64 deltaY = input.mouseDeltaY();

// 结束帧
input.endFrame();
```

### 7. world/ - 客户端世界

**职责**: 管理客户端区块、实体、天气、颜色解析等。

**核心类**:
- `ClientWorld`: 客户端世界管理器
- `ClientChunk`: 客户端区块数据
- `ClientEntityManager`: 客户端实体管理
- `ClientWeather`: 客户端天气状态

**子模块**:
- `color/`: 颜色解析系统，从生物群系获取草、树叶、水体颜色
  - `ColorResolver`: 颜色解析器抽象接口
  - `GrassColorResolver`/`FoliageColorResolver`/`WaterColorResolver`: 具体解析器
  - `BiomeColors`: 颜色常量和工具函数

**功能**:
- 区块加载/卸载管理
- 异步网格构建
- 区块数据接收（从服务端）
- 光照查询
- 实体管理
- 时间和天气同步
- 生物群系颜色解析（草、树叶、水体等）

详细文档请参阅 [`world/README.md`](world/README.md) 和 [`world/color/README.md`](world/color/README.md)。

### 8. chat/ - 聊天系统

**职责**: 聊天消息历史管理。

**核心类**:
- `ChatHistory`: 聊天历史记录

### 9. window/ - 窗口管理

**职责**: GLFW 窗口封装。

**核心类**:
- `Window`: 窗口管理器

### 10. settings/ - 客户端设置

**职责**: 客户端配置管理。

**核心类**:
- `ClientSettings`: 客户端设置（视频、音频、控制等）

## 模块关系

```
┌─────────────────────────────────────────────────────────────┐
│                    ClientApplication                         │
│  (协调所有子系统，管理主循环)                                  │
└─────────────────────────────────────────────────────────────┘
          │
          ├──► Window (GLFW 窗口)
          │
          ├──► InputManager (键盘/鼠标输入)
          │
          ├──► ClientSettings (设置)
          │
          ├──► NetworkClient ◄─────┐
          │       │                │
          │       ▼                │
          │   [服务端通信]          │
          │                        │
          ├──► ClientWorld         │
          │       │                │
          │       ├──► ClientEntityManager
          │       ├──► MeshWorkerPool (异步网格构建)
          │       └──► ClientWeather
          │
          ├──► TridentEngine (渲染引擎)
          │       │
          │       ├──► ChunkRenderer ◄── MeshWorkerPool
          │       ├──► SkyRenderer
          │       ├──► CloudRenderer
          │       ├──► EntityRenderer
          │       ├──► GuiRenderer ◄── KageroEngine
          │       ├──► ParticleManager
          │       ├──► WeatherRenderer
          │       ├──► FogManager
          │       └──► ItemRenderer
          │
          ├──► ResourceManager (资源加载)
          │       │
          │       ├──► BlockModelLoader
          │       ├──► BlockStateLoader
          │       └──► TextureAtlasBuilder
          │
          ├──► KageroEngine (UI 框架)
          │       │
          │       ├──► Widget 系统
          │       ├──► Layout 系统
          │       ├──► State 系统
          │       └──► Template 系统
          │
          └──► IntegratedServer (内置服务器)
                  │
                  └──► LocalConnection ◄─────┘
                       (本地 IPC 连接)
```

## 整体职责

客户端模块负责：

1. **渲染**: 使用 Vulkan API 渲染游戏世界（区块、实体、天空、粒子等）
2. **UI**: 渲染 HUD、菜单、物品栏等用户界面
3. **网络**: 与服务端通信（TCP 或本地连接）
4. **输入**: 处理键盘和鼠标输入
5. **资源**: 加载和管理资源包（模型、纹理、音效等）
6. **世界**: 管理客户端区块缓存、实体、天气状态

## 输入和输出

### 输入

| 来源 | 数据类型 | 处理者 |
|------|----------|--------|
| 键盘/鼠标 | GLFW 输入事件 | InputManager |
| 网络 | 数据包 | NetworkClient |
| 资源包 | 文件/ZIP | ResourceManager |
| 设置 | JSON 文件 | ClientSettings |

### 输出

| 目标 | 数据类型 | 生成者 |
|------|----------|--------|
| 显示器 | Vulkan 渲染帧 | TridentEngine |
| 网络 | 数据包 | NetworkClient |
| 音频 | 音效播放 | (待实现) |

## 依赖项

### 外部依赖

| 库 | 用途 |
|----|------|
| GLFW | 窗口和输入管理 |
| Vulkan | 图形 API |
| VulkanMemoryAllocator | GPU 内存管理 |
| spdlog | 日志 |
| nlohmann_json | JSON 解析 |
| stb_image | 图像加载 |
| asio | 网络 (TCP) |

### 内部依赖

| 模块 | 依赖 |
|------|------|
| client | common (core, world, entity, network, resource, physics) |
| client | server (IntegratedServer) |

## 使用方法

### 启动客户端

```powershell
# Release 构建
cmake --build --preset windows-clang-relwithdebinfo
./build/bin/Release/minecraft-client.exe

# 带参数启动
./build/bin/Release/minecraft-client.exe --width 1920 --height 1080 --fullscreen
```

### 命令行参数

| 参数 | 说明 |
|------|------|
| `-h, --help` | 显示帮助信息 |
| `-w, --width <px>` | 设置窗口宽度（默认 1280） |
| `-H, --height <px>` | 设置窗口高度（默认 720） |
| `-f, --fullscreen` | 全屏模式启动 |
| `-s, --server <addr>` | 服务器地址 |
| `-p, --port <port>` | 服务器端口（默认 19132） |
| `-u, --username <name>` | 玩家用户名 |
| `--settings <path>` | 自定义设置文件路径 |
| `-v, --verbose` | 启用详细日志 |
| `--skip-integrated` | 跳过内置服务器（连接外部服务器时使用） |
| `--quick-play <id>` | 跳过主菜单，直接加载指定世界 |
| `--quick-play-new` | 跳过主菜单，直接创建新世界 |
| `--benchmark-exit-after-initialize` | 只执行 `ClientApplication::initialize` 的 shell 初始化路径，然后立即退出 |

```powershell
# Quick-play 示例
./minecraft-client --quick-play "My World"     # 直接加载名为 "My World" 的存档
./minecraft-client --quick-play-new            # 创建新世界并进入
```

### 连接服务器

```cpp
// 连接远程服务器
NetworkClientConfig config;
config.serverAddress = "192.168.1.100";
config.serverPort = 25565;
config.username = "Player";
client.connect(config);

// 或使用内置服务器（单机模式）
ClientApplication app;
ClientLaunchParams params;
params.skipIntegratedServer = false;  // 默认启用内置服务器
app.initialize(params);
app.run();
```

## 容易踩的坑

### 1. Vulkan 资源销毁顺序

**问题**: Vulkan 对象必须按正确顺序销毁，否则会导致崩溃。

**解决方案**: 
- 使用 `TridentEngine::destroy()` 按依赖顺序销毁
- 不要手动销毁 Vulkan 对象

### 2. 区块网格构建阻塞主线程

**问题**: 区块网格构建是 CPU 密集操作，在主线程执行会导致卡顿。

**解决方案**:
- 使用 `MeshWorkerPool` 异步构建
- 每帧限制处理数量 (`maxPerFrame = 4`)

### 3. 纹理图集更新

**问题**: 纹理图集更新后，已缓存的纹理区域失效。

**解决方案**:
- 重建图集后调用 `ChunkRenderer::loadTextureAtlas()`
- 使用 `ResourceManager::reload()` 完整重新加载

### 4. GUI 渲染顺序

**问题**: GUI 元素渲染顺序错误导致遮挡。

**解决方案**:
- 使用 `zIndex` 控制层级
- 按正确顺序调用渲染回调

### 5. 网络包处理

**问题**: 网络包处理顺序错误导致状态不一致。

**解决方案**:
- 在 `poll()` 中处理所有待处理包
- 不要在回调中阻塞

### 6. 输入状态清理

**问题**: `isKeyJustPressed()` 状态未正确清理。

**解决方案**:
- 每帧结束时调用 `input.endFrame()`

### 8. 线程安全

**问题**: 多线程访问共享资源导致崩溃。

**解决方案**:
- `MeshWorkerPool` 使用 `shared_ptr<ChunkData>` 共享数据
- 主线程处理结果，Worker 只读取

## 测试用例

测试文件位于 `tests/client/` 目录：

```
tests/client/
├── renderer/
│   ├── AmbientOcclusionCalculatorTest.cpp  # AO 计算测试
│   ├── test_renderer.cpp                   # 渲染类型测试
│   ├── test_trident_api.cpp                # API 接口测试
│   ├── test_trident_engine.cpp             # 引擎测试
│   ├── test_cloud_renderer.cpp             # 云渲染测试
│   ├── test_gui_item_color.cpp             # GUI 物品颜色测试
│   └── entity/
│       ├── AnimalModelTests.cpp            # 动物模型测试
│       └── test_entity_renderer_manager.cpp
├── resource/
│   ├── test_resource_manager_cloud_texture.cpp  # 纹理加载测试
│   ├── test_model_loader.cpp                    # 模型加载测试
│   ├── test_entity_texture_loader.cpp           # 实体纹理测试
│   ├── test_resource_location.cpp               # 资源位置测试
│   └── ItemTextureAtlasTest.cpp                 # 物品图集测试
├── world/
│   └── color/
│       └── BiomeColorsTest.cpp            # 生物群系颜色测试
├── test_mesh_worker_pool.cpp               # 网格线程池测试
└── ui/
    └── kagero/
        ├── layout/                          # 布局系统测试
        │   ├── FlexLayoutTest.cpp
        │   ├── GridLayoutTest.cpp
        │   ├── AnchorLayoutTest.cpp
        │   └── ...
        ├── state/                           # 状态系统测试
        │   ├── ReactiveStateTest.cpp
        │   ├── StateStoreTest.cpp
        │   └── ...
        └── widget/                          # Widget 测试
            ├── WidgetTest.cpp
            ├── ButtonWidgetTest.cpp
            ├── TextFieldWidgetTest.cpp
            └── ...
```

### 运行测试

```powershell
# 运行所有客户端测试
./build/bin/Release/mc_tests.exe --gtest_filter="Client*"

# 运行特定测试
./build/bin/Release/mc_tests.exe --gtest_filter="Client.Renderer*"
./build/bin/Release/mc_tests.exe --gtest_filter="Client.Kagero*"
```

## 构建配置

```powershell
# 配置项目
cmake -B build -G "Visual Studio 18" -A x64 `
    -DCMAKE_TOOLCHAIN_FILE=D:/tools/vcpkg/scripts/buildsystems/vcpkg.cmake

# 构建
cmake --build --preset windows-clang-relwithdebinfo

# 启用 Vulkan 验证层 (调试用)
cmake -B build -DMC_ENABLE_VULKAN_VALIDATION=ON
```

## 相关文档

- [Kagero UI 框架文档](ui/kagero/docs/)
- [CLAUDE.md 项目总览](../../CLAUDE.md)
