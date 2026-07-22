# Client Module

Cubium 客户端模块，负责游戏客户端的所有功能，包括渲染、UI、网络、输入处理、命令补全和世界管理。

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
├── renderer/              # 渲染系统（详见 renderer/README.md）
│   ├── api/               # 平台无关渲染接口
│   ├── trident/           # Trident Vulkan 渲染引擎
│   ├── mesh/              # 网格调度与执行
│   ├── Camera.hpp/cpp     # 第一人称相机控制器
│   └── MeshTypes.hpp/cpp  # 网格数据类型
├── resource/              # 资源加载
│   ├── ResourceManager.hpp/cpp       # 资源管理器
│   ├── BlockModelLoader.hpp/cpp      # 方块模型加载器
│   ├── BlockStateLoader.hpp/cpp      # 方块状态加载器
│   ├── BlockModelCache.hpp/cpp       # 模型缓存
│   ├── TextureAtlasBuilder.hpp/cpp   # 纹理图集构建器
│   ├── ItemTextureAtlas.hpp/cpp      # 物品纹理图集
│   ├── atlas/                        # 统一图集数据驱动子系统（AtlasManager，对齐 MC 1.21.11 TextureManager）
│   └── EntityTextureLoader.hpp/cpp   # 实体纹理加载器
├── settings/              # 客户端设置
│   └── ClientSettings.hpp/cpp
├── ui/                    # 用户界面（详见 ui/README.md）
│   ├── Font*.hpp/cpp      # 字体系统
│   ├── kagero/            # Kagero UI 框架
│   └── minecraft/         # Minecraft UI 业务层
├── window/                # 窗口管理
│   └── Window.hpp/cpp     # GLFW 窗口封装
└── world/                 # 客户端世界（详见 world/README.md）
    ├── ClientWorld.hpp/cpp      # 客户端世界管理
    ├── ClientWeather.hpp        # 客户端天气状态
    ├── color/                   # 颜色解析系统
    └── entity/                  # 客户端实体
```

## 内部模块关系

```
┌─────────────────────────────────────────────────────────────┐
│                    ClientApplication                         │
│  (协调所有子系统，管理主循环)                                  │
└─────────────────────────────────────────────────────────────┘
          │
          ├──► Window (GLFW 窗口)
          ├──► InputManager (键盘/鼠标输入)
          ├──► ClientSettings (设置)
          ├──► NetworkClient ◄─────┐
          │       │                │
          │       ▼                │
          │   [服务端通信]          │
          │                        │
          ├──► ClientWorld         │
          │       │                │
          │       ├──► ClientEntityManager
          │       ├──► UniversalWorkerPool/ClientCompute (异步网格构建+皮肤加载)
          │       └──► ClientWeather
          │                        │
          ├──► TridentEngine (渲染引擎)
          │       │
          │       ├──► ChunkRenderer ◄── ClientCompute(UniversalWorkerPool)
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
          │       ├──► BlockModelLoader
          │       ├──► BlockStateLoader
          │       └──► TextureAtlasBuilder
          │
          ├──► KageroEngine (UI 框架)
          │       ├──► Widget 系统
          │       ├──► Layout 系统
          │       ├──► State 系统
          │       └──► Template 系统
          │
          └──► IntegratedServer (内置服务器)
                  └──► LocalConnection ◄─────┘
```

## 上下游外部依赖关系

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

### 被依赖

客户端模块是顶层模块，不被其他模块依赖。

## 容易踩的坑

### 1. Vulkan 资源销毁顺序

Vulkan 对象必须按正确顺序销毁，否则会导致崩溃。使用 `TridentEngine::destroy()` 按依赖顺序销毁，不要手动销毁 Vulkan 对象。

### 2. 区块网格构建阻塞主线程

区块网格构建是 CPU 密集操作，在主线程执行会导致卡顿。使用 `UniversalWorkerPool`(ClientCompute) 异步构建，每帧限制处理数量 (`maxPerFrame = 4`)。

### 3. 纹理图集更新

纹理图集更新后，已缓存的纹理区域失效。重建图集后调用 `ChunkRenderer::loadTextureAtlas()` 或使用 `ResourceManager::reload()` 完整重新加载。

### 4. GUI 渲染顺序

GUI 元素渲染顺序错误导致遮挡。使用 `zIndex` 控制层级，按正确顺序调用渲染回调。

### 5. 网络包处理

网络包处理顺序错误导致状态不一致。在 `poll()` 中处理所有待处理包，不要在回调中阻塞。

### 6. 输入状态清理

`isKeyJustPressed()` 状态未正确清理。每帧结束时调用 `input.endFrame()`。

### 7. 线程安全

多线程访问共享资源导致崩溃。`UniversalWorkerPool`(ClientCompute) 使用 `shared_ptr<ChunkData>` 共享数据，主线程处理结果，Worker 只读取。

### 8. 主循环 partialTick

客户端主循环采用固定时间步长物理更新 + 可变帧率渲染架构。`partialTick` 是 `[0.0, 1.0)` 范围的浮点值，表示当前渲染帧在两个游戏 tick 之间的位置，用于实体位置插值、相机位置插值、动画插值等。

### 9. 音频线程约束

`ClientApplication` 只持有 `AudioService`，不直接持有 `SoundEngine`。`SoundEngine` 的所有 OpenAL 调用必须在音频线程内执行。

### 10. MeshSchedulerViewState 更新时机

每帧在调用 `ClientWorld::update(...)` 之前必须更新 `MeshSchedulerViewState`，否则视锥体优先级和相机后取消将滞后于相机移动。

### 11. MeshBuildScheduler 并发预算

`ClientApplication` 里给 `MeshBuildScheduler` 的并发预算要保持保守（如 `maxDispatchedTaskCount = 64`），避免内存压力。

### 12. ChunkMesher 液面剔除

液面剔除必须将空碰撞的水下植物（如海草和海带）视为隐藏面的邻居，否则会在水生植被周围重新引入散乱的水面片。

### 13. 第一人称物品网格缓存

不要在双手之间共享一个第一人称物品网格缓存，主手和副手需要独立的缓存，否则会因主手和副手在同一帧持有不同物品而抖动。

### 14. EntityPipeline 网格更新

`EntityPipeline::updateMesh(...)` 必须保留 GPU 缓冲区并仅在需要时增长容量；保持可重用的暂存缓冲区和原地上传，避免每帧销毁+创建导致 `vkAllocateMemory` 回到渲染热路径。

### 15. 字形图集已满

`FontTextureAtlas::addGlyph` 返回错误时，增大图集大小（如 `font.initialize(512)`）或预加载常用字符。

### 16. 事件订阅未取消

订阅事件后忘记取消订阅会导致内存泄漏或野指针回调。使用 RAII 管理 `EventSubscription`。

### 17. Widget 生命周期

Widget 被移除后仍持有指针会导致野指针访问。使用 `std::unique_ptr` 管理 Widget 生命周期，不要持有裸指针，使用 `Widget::WeakPtr`。

### 18. 状态更新循环

状态更新触发观察者，观察者又更新状态会形成死循环。应检查是否需要更新后再设置，或通过事件总线解耦。

### 19. 布局未更新

窗口大小改变时未重新布局会导致 Widget 位置或大小不正确。调用 `engine.resize(width, height)` 或 `widget->setBounds(...)`。

### 20. 线程安全（UI）

UI 组件不是线程安全的。所有 UI 操作必须在主线程执行，跨线程通信使用事件总线。

### 21. ClientWorld 不是 IWorld 实现

它只提供自己的 xyz 查询接口，调试屏幕和客户端工具代码不要假设这里存在 `BlockPos` overload。

### 22. 维度切换顺序

正确顺序是先 `setDimensionId()`，再 `clearChunks()`，这样迟到的旧维度区块包才会被丢弃。

### 23. 光照包处理

`onLightUpdate()` 会先标记 `meshRebuildPending`，如果同一 chunk 的网格任务还在路上，就等当前任务结束后再补提，避免单个 chunk 被光照更新线性打爆。
