# Renderer 模块

本文档详细介绍 Minecraft Reborn 项目的客户端渲染系统，包括目录结构、各组件职责、模块关系、依赖项、使用方法和注意事项。

## 目录结构树

```
src/client/renderer/
├── api/                          # 平台无关的渲染抽象接口
│   ├── BlendMode.hpp             # 混合状态定义
│   ├── CompareOp.hpp             # 深度比较操作定义
│   ├── CullMode.hpp              # 面剔除模式定义
│   ├── IRenderEngine.hpp         # 渲染引擎主接口
│   ├── TridentApi.hpp            # Trident API 统一头文件
│   ├── Types.hpp/cpp             # 基础类型定义（顶点、面、枚举）
│   ├── buffer/
│   │   └── IBuffer.hpp           # 缓冲区接口（顶点/索引/Uniform）
│   ├── camera/
│   │   ├── CameraConfig.hpp      # 相机配置结构
│   │   └── ICamera.hpp           # 相机接口
│   ├── mesh/
│   │   └── MeshData.hpp          # 网格数据结构
│   ├── pipeline/
│   │   ├── IPipeline.hpp         # 管线接口
│   │   ├── RenderState.hpp       # 渲染状态（混合/深度/剔除）
│   │   └── RenderType.hpp        # 渲染类型（MC 1.16.5 风格）
│   └── texture/
│       ├── ITexture.hpp          # 纹理接口
│       ├── ITextureAtlas.hpp     # 纹理图集构建器接口
│       └── TextureRegion.hpp     # 纹理区域（UV坐标）
├── Camera.hpp/cpp                # 相机实现（第一人称）
├── MeshTypes.hpp/cpp             # 网格类型定义（顶点、面、图集）
├── mesh/
│   ├── MeshWorkerPool.hpp/cpp    # 纯执行线程池
│   └── MeshBuildScheduler.hpp/cpp # 独立调度器（视锥/距离优先 + 取消）
├── trident/                      # Trident Vulkan 渲染引擎实现
│   ├── core/                     # 核心组件
│   │   ├── Trident.hpp           # 引擎统一头文件
│   │   ├── TridentContext.hpp/cpp    # Vulkan 上下文（实例/设备/队列）
│   │   ├── TridentEngine.hpp/cpp     # 渲染引擎主类（IRenderEngine 实现）
│   │   ├── TridentSwapchain.hpp/cpp  # 交换链管理
│   │   ├── buffer/
│   │   │   └── TridentBuffer.hpp/cpp # Vulkan 缓冲区实现
│   │   ├── pipeline/
│   │   │   └── TridentPipeline.hpp/cpp   # Vulkan 管线实现
│   │   ├── render/
│   │   │   ├── DescriptorManager.hpp/cpp # 描述符管理器
│   │   │   ├── FrameManager.hpp/cpp      # 帧管理器（同步/命令缓冲）
│   │   │   ├── RenderPassManager.hpp/cpp # 渲染通道管理器
│   │   │   └── UniformManager.hpp/cpp    # Uniform 缓冲区管理器
│   │   └── texture/
│   │       └── TridentTexture.hpp/cpp    # Vulkan 纹理实现
│   ├── chunk/                    # 区块渲染
│   │   ├── AmbientOcclusionCalculator.hpp/cpp   # AO 计算
│   │   ├── ChunkMesher.hpp/cpp   # 区块网格生成器
│   │   └── ChunkRenderer.hpp/cpp # 区块渲染器（GPU 缓冲区管理）
│   ├── cloud/                    # 云渲染
│   │   └── CloudRenderer.hpp/cpp # Fast/Fancy 云渲染
│   ├── entity/                   # 实体渲染
│   │   ├── AnimalRenderers.hpp   # 动物渲染器集合
│   │   ├── EntityPipeline.hpp/cpp    # 实体渲染管线
│   │   ├── EntityRenderer.hpp/cpp    # 实体渲染器基类
│   │   ├── EntityRendererManager.hpp/cpp # 实体渲染器管理器
│   │   ├── EntityTextureAtlas.hpp/cpp    # 实体纹理图集
│   │   ├── ItemEntityRenderer.hpp/cpp    # 物品实体渲染器
│   │   ├── LivingRenderer.hpp    # 生物渲染器基类
│   │   └── model/                # 实体模型
│   │       ├── AnimalModels.hpp/cpp  # 动物模型
│   │       ├── EntityModel.hpp/cpp   # 实体模型基类
│   │       └── ModelRenderer.hpp/cpp # 模型渲染器
│   ├── fog/                      # 雾效果
│   │   └── FogManager.hpp/cpp    # 雾效果管理（线性/指数雾）
│   ├── gui/                      # GUI 渲染
│   │   ├── GuiAtlasRegistry.hpp/cpp   # GUI 图集注册表
│   │   ├── GuiRenderer.hpp/cpp   # GUI 渲染器（文本/矩形/纹理）
│   │   ├── GuiSprite.hpp         # GUI 精灵定义
│   │   ├── GuiSpriteAtlas.hpp/cpp    # GUI 精灵图集
│   │   ├── GuiSpriteManager.hpp/cpp  # GUI 精灵管理器
│   │   ├── GuiSpriteParser.hpp/cpp   # GUI 精灵解析器
│   │   ├── GuiSpriteRegistry.hpp/cpp # GUI 精灵注册表
│   │   ├── GuiTextureAtlas.hpp/cpp   # GUI 纹理图集
│   │   ├── GuiTextureLoader.hpp/cpp  # GUI 纹理加载器
│   │   └── GuiTextureManager.hpp/cpp # GUI 纹理管理器
│   ├── item/                     # 物品渲染
│   │   └── ItemRenderer.hpp/cpp  # 物品图标渲染器
│   ├── particle/                 # 粒子系统
│   │   ├── Particle.hpp/cpp      # 粒子基类
│   │   ├── ParticleManager.hpp/cpp   # 粒子管理器
│   │   └── particles/            # 具体粒子类型
│   │       ├── RainParticle.hpp/cpp  # 雨滴粒子
│   │       └── SnowParticle.hpp/cpp  # 雪花粒子
│   ├── sky/                      # 天空渲染
│   │   ├── CelestialCalculations.hpp/cpp # 天体计算（太阳/月亮/星星）
│   │   └── SkyRenderer.hpp/cpp   # 天空渲染器
│   ├── util/                     # 工具函数
│   │   └── VulkanUtils.hpp       # Vulkan 辅助函数
│   ├── weather/                  # 天气渲染
│   │   └── WeatherRenderer.hpp/cpp   # 雨/雪渲染器
│   ├── block/                    # 方块渲染
│   │   ├── BreakProgressManager.hpp/cpp # 破坏进度管理
│   │   └── BreakProgressRenderer.hpp/cpp # 破坏进度渲染器
│   └── firstperson/              # 第一人称手部渲染
│       ├── README.md             # 模块文档
│       ├── ArmPose.hpp           # 手臂姿态枚举
│       ├── FirstPersonRenderer.hpp/cpp # 第一人称渲染器主类
│       ├── ItemCameraTransforms.hpp/cpp # 物品相机变换
│       ├── ItemInHandRenderer.hpp/cpp   # 手持物品渲染器
│       ├── MatrixStack.hpp/cpp   # 矩阵栈（变换层级管理）
│       └── PlayerModel.hpp/cpp   # 玩家模型（双足模型扩展）
└── util/                         # 渲染工具
    ├── GpuInfo.hpp               # GPU 信息提取
    └── ShaderPath.hpp            # 着色器路径解析
```

---

## 组件详细介绍

### 1. api/ - 平台无关渲染抽象层

这是渲染系统的核心抽象层，定义了与具体图形 API 无关的接口，使得渲染系统可以支持多种后端（目前仅 Vulkan）。

#### 1.1 IRenderEngine.hpp - 渲染引擎主接口

**职责**：定义渲染引擎的完整生命周期和核心功能。

**主要内容**：
- `RenderEngineConfig`：引擎配置（窗口尺寸、VSync、验证层等）
- `FrameContext`：帧上下文（帧索引、相机矩阵、时间等）
- `IRenderEngine`：主接口
  - 生命周期管理：`initialize()`, `destroy()`, `beginFrame()`, `endFrame()`, `present()`
  - 资源创建：`createVertexBuffer()`, `createIndexBuffer()`, `createUniformBuffer()`, `createTexture()`
  - 渲染状态：`setRenderType()`, `bindTexture()`, `bindUniformBuffer()`
  - 绘制：`drawIndexed()`, `draw()`, `drawIndexedInstanced()`

**使用方法**：
```cpp
#include "renderer/api/IRenderEngine.hpp"

auto engine = mc::client::renderer::api::createRenderEngine(
    mc::client::renderer::api::RenderBackend::Vulkan);

mc::client::renderer::api::RenderEngineConfig config;
config.appName = "MyGame";
config.enableValidation = true;
config.enableVSync = true;

engine->initialize(window, config);

while (running) {
    engine->beginFrame();
    engine->setCamera(&camera);
    // 绘制...
    engine->endFrame();
    engine->present();
}
engine->destroy();
```

#### 1.2 Types.hpp - 基础类型

**职责**：定义渲染相关的枚举和基础数据结构。

**主要内容**：
- `Vertex`：顶点格式（位置、法线、UV、颜色、光照）
- `Face`：方块面朝向枚举（Bottom, Top, North, South, West, East）
- `BufferUsage`：缓冲区用途（Vertex, Index, Uniform, Staging, Storage）
- `MemoryType`：内存类型（DeviceLocal, HostVisible, HostCoherent）
- `IndexType`：索引类型（U16, U32）
- `BlockGeometry`：方块几何工具函数

#### 1.3 BlendMode.hpp - 混合状态

**职责**：定义颜色混合配置。

**主要内容**：
- `BlendFactor`：混合因子（Zero, One, SrcAlpha, OneMinusSrcAlpha 等）
- `BlendOp`：混合操作（Add, Subtract, ReverseSubtract, Min, Max）
- `BlendState`：完整混合配置
  - 预设：`disabled()`, `alpha()`, `additive()`, `premultiplied()`, `multiply()`

#### 1.4 CompareOp.hpp - 深度比较

**职责**：定义深度测试配置。

**主要内容**：
- `CompareOp`：比较操作（Never, Less, Equal, LessEqual, Greater 等）
- `DepthState`：深度状态
  - 预设：`disabled()`, `readOnly()`, `readWrite()`, `equal()`

#### 1.5 CullMode.hpp - 面剔除

**职责**：定义面剔除配置。

**主要内容**：
- `CullMode`：剔除模式（None, Front, Back, FrontAndBack）
- `FrontFace`：正面朝向（CounterClockwise, Clockwise）
- `PolygonMode`：填充模式（Fill, Line, Point）
- `RasterizerState`：光栅化状态
  - 预设：`defaults()`, `doubleSided()`, `wireframe()`

#### 1.6 RenderState.hpp - 渲染状态

**职责**：组合混合、深度、剔除状态。

**主要内容**：
- `RenderState`：完整渲染状态
  - 预设：`solid()`, `cutout()`, `cutoutMipped()`, `translucent()`, `lines()`, `additive()`

#### 1.7 RenderType.hpp - 渲染类型

**职责**：MC 1.16.5 风格的命名渲染类型。

**主要内容**：
- `RenderType`：命名渲染类型（名称 + 状态 + 排序索引）
- 预定义渲染类型：
  - 方块：`solid()`, `cutout()`, `cutoutMipped()`, `translucent()`
  - 实体：`entitySolid()`, `entityCutout()`, `entityTranslucent()`
  - 特殊：`sky()`, `clouds()`, `particle()`, `gui()`, `lightning()`, `lines()`

#### 1.8 pipeline/IPipeline.hpp - 管线接口

**职责**：定义渲染管线抽象。

**主要内容**：
- `ShaderStage`：着色器阶段枚举
- `ShaderModuleDesc`：着色器模块描述
- `PipelineDesc`：管线描述
- `IPipeline`：管线接口
- `IPipelineLayout`：管线布局接口
- `IDescriptorSet`：描述符集接口

#### 1.9 texture/ - 纹理接口

**职责**：定义纹理和纹理图集抽象。

**主要内容**：
- `TextureFormat`：纹理格式（R8_UNORM, R8G8B8A8_SRGB, BC 压缩格式等）
- `TextureFilter`：过滤模式（Nearest, Linear, Mipmap 变体）
- `TextureAddressMode`：寻址模式（Repeat, MirroredRepeat, ClampToEdge 等）
- `TextureDesc`：纹理描述
- `ITexture`：纹理接口
- `ITextureAtlas`：纹理图集接口
- `TextureRegion`：纹理区域（UV 坐标）
- `ITextureAtlasBuilder`：纹理图集构建器接口

#### 1.10 buffer/IBuffer.hpp - 缓冲区接口

**职责**：定义各类缓冲区抽象。

**主要内容**：
- `IBuffer`：缓冲区基础接口
- `IVertexBuffer`：顶点缓冲区接口
- `IIndexBuffer`：索引缓冲区接口
- `IUniformBuffer`：Uniform 缓冲区接口（支持多帧轮换）
- `IStagingBuffer`：暂存缓冲区接口

#### 1.11 camera/ - 相机接口

**职责**：定义相机抽象。

**主要内容**：
- `ProjectionMode`：投影模式（Perspective, Orthographic）
- `CameraConfig`：相机配置（FOV、宽高比、近远平面、移动速度等）
- `ICamera`：相机接口
  - 位置/旋转：`setPosition()`, `setRotation()`, `forward()`, `right()`, `up()`
  - 移动：`moveForward()`, `moveRight()`, `moveUp()`, `look()`
  - 投影：`setProjectionMode()`, `setFOV()`, `setAspectRatio()`
  - 矩阵：`viewMatrix()`, `projectionMatrix()`, `viewProjectionMatrix()`

#### 1.12 mesh/MeshData.hpp - 网格数据

**职责**：定义网格数据结构。

**主要内容**：
- `MeshData`：网格数据（顶点数组 + 索引数组）
- `ChunkMeshData`：区块网格数据（实心网格 + 透明网格）

---

### 2. Camera.hpp/cpp - 相机实现

**职责**：第一人称相机实现。

**主要内容**：
- `Camera`：相机类（实现 `ICamera` 接口）
  - 使用 Minecraft 坐标系约定：yaw=0 看向 +Z
  - 支持透视/正交投影
  - 脏标记优化
- `CameraController`：相机控制器
  - WASD 移动 + 鼠标视角
  - 冲刺/潜行倍率

**使用方法**：
```cpp
#include "renderer/Camera.hpp"

mc::client::Camera camera;
camera.setPosition(0.0f, 64.0f, 0.0f);
camera.setFOV(70.0f);

// 在游戏循环中
camera.handleMouseMove(mouseDeltaX, mouseDeltaY);
camera.update(deltaTime);
engine.setCamera(&camera);
```

---

### 3. MeshTypes.hpp/cpp - 网格类型

**职责**：定义顶点格式和方块几何工具。

**主要内容**：
- `Vertex`：顶点格式（位置、法线、UV、颜色、光照）
- `Face`：方块面朝向
- `BlockGeometry`：方块几何工具
  - `getFaceNormal()`：获取面法线
  - `getFaceVertices()`：获取面顶点位置
  - `getFaceIndices()`：获取面索引
  - `getFaceDirection()`：获取面方向向量
  - `shouldRenderFace()`：面剔除判断
- `MeshData`：网格数据结构
- `TextureRegion`：纹理区域
- `TextureAtlas`：简单纹理图集（瓦片坐标到 UV）

---

### 4. mesh/ - 网格调度与执行

**职责**：将“调度策略”与“线程执行”解耦，减少过期任务浪费并提升可见区块响应。

**主要内容**：
- `MeshBuildScheduler`：
  - 接收 `MeshSchedulerViewState`，按距离/视锥重排 pending。
  - 负责同区块任务代际管理与取消。
- `MeshWorkerPool`：
  - 仅消费 `MeshWorkerTask` 并产出 `MeshWorkerResult`。
  - 支持协作取消信号。

**使用方法**：
```cpp
mc::client::MeshWorkerPool workerPool(-1);
workerPool.start();

mc::client::MeshSchedulerConfig config;
config.maxDispatchedTaskCount = 64;
config.reprioritizeIntervalFrames = 6;
config.cameraMoveThreshold = 2.0f;
config.cameraDirectionDotThreshold = 0.96f;
config.behindCancelDotThreshold = -0.35f;
config.behindCancelDistanceChunks = 8.0f;

mc::client::MeshBuildScheduler scheduler(workerPool, config);
scheduler.setViewState(viewState);
scheduler.tick();
scheduler.drainCompleted([](mc::client::MeshWorkerResult&& result) {
    // 更新 GPU 缓冲区
}, 4);
```

---

### 5. trident/ - Trident Vulkan 渲染引擎

这是渲染系统的 Vulkan 实现，完全实现 `api/` 中定义的接口。

#### 5.1 core/ - 核心组件

##### TridentContext.hpp/cpp - Vulkan 上下文

**职责**：管理 Vulkan 实例、物理设备、逻辑设备和队列。

**主要内容**：
- Vulkan 实例创建和验证层
- 物理设备选择
- 逻辑设备和队列创建
- 单次命令缓冲区辅助

##### TridentEngine.hpp/cpp - 渲染引擎主类

**职责**：实现 `IRenderEngine` 接口，协调所有渲染组件。

**主要内容**：
- 初始化所有子渲染器（区块、天空、GUI、实体、雾、云、粒子、天气、物品、破坏进度）
- 帧渲染循环管理
- 资源创建代理
- 渲染回调系统
- 根据客户端抗锯齿开关和设备能力选择实际的 MSAA sample count，并同步给渲染通道和主通道管线

##### TridentSwapchain.hpp/cpp - 交换链

**职责**：管理交换链、图像和图像视图。

**主要内容**：
- 交换链创建/重建
- 图像获取和呈现
- 窗口大小变化处理

##### render/ - 渲染管理器

- **DescriptorManager**：描述符集布局、描述符池、描述符集分配
- **FrameManager**：命令缓冲区、信号量、栅栏、帧生命周期
- **RenderPassManager**：渲染通道、深度缓冲区、帧缓冲区；多重采样开启时会额外创建 multisampled color/depth attachment 和 resolve attachment
- **UniformManager**：相机和光照 Uniform 缓冲区管理

##### buffer/TridentBuffer.hpp/cpp - 缓冲区实现

**职责**：实现 `IBuffer` 接口。

**主要内容**：
- `TridentBuffer`：Vulkan 缓冲区基类
- `TridentStagingBuffer`：暂存缓冲区（HOST_VISIBLE 内存，用于 CPU 到 GPU 数据传输）
- `TridentVertexBuffer`：顶点缓冲区（设备本地内存，支持 `upload()` 直接上传）
- `TridentIndexBuffer`：索引缓冲区（设备本地内存，支持 `upload()` 直接上传）
- `TridentUniformBuffer`：Uniform 缓冲区（支持多帧轮换）

**数据上传方式**：
- 顶点/索引缓冲区：使用 `upload(data, size, offset)` 方法，内部通过暂存缓冲区和 `vkCmdCopyBuffer` 实现
- Uniform 缓冲区：使用 `upload(data, size)` 方法，直接映射 HOST_VISIBLE 内存
- 暂存缓冲区：使用 `upload()` 上传数据，然后用 `copyTo()` 复制到目标缓冲区

##### pipeline/TridentPipeline.hpp/cpp - 管线实现

**职责**：实现 `IPipeline` 接口。

**主要内容**：
- `TridentPipeline`：Vulkan 图形管线
- `TridentPipelineConfig`：管线配置（主通道由 `TridentEngine` 填充实际 `rasterizationSamples`）
- `TridentPipelineCache`：管线缓存

##### texture/TridentTexture.hpp/cpp - 纹理实现

**职责**：实现 `ITexture` 和 `ITextureAtlas` 接口。

**主要内容**：
- `TridentTexture`：Vulkan 纹理（图像、视图、采样器）
- `TridentTextureAtlas`：纹理图集实现

#### 5.2 chunk/ - 区块渲染

##### ChunkMesher.hpp/cpp - 区块网格生成器

**职责**：将 `ChunkData` 转换为可渲染的 `MeshData`。

**主要内容**：
- `generateMesh()`：生成完整区块网格
- `generateSectionMesh()`：生成单个区块段网格
- 支持两种光照模式：
  - `Flat`：平面光照（每面统一光照）
  - `Smooth`：平滑光照（逐顶点 AO）
- 可选贪婪网格合并
- `ChunkMeshCache`：网格缓存

##### AmbientOcclusionCalculator.hpp/cpp - AO 计算器

**职责**：计算方块面的逐顶点 AO 值。

**主要内容**：
- 基于 MC 1.16.5 的 `BlockModelRenderer.AmbientOcclusionFace` 实现
- 采样角落光照和透明度
- 计算顶点颜色乘数和亮度

##### ChunkRenderer.hpp/cpp - 区块渲染器

**职责**：管理区块 GPU 缓冲区，执行渲染。

**主要内容**：
- 区块缓冲区管理（创建、更新、删除）
- 纹理图集管理
- 异步 GPU 上传（非阻塞）
- 延迟缓冲区销毁

#### 5.3 sky/ - 天空渲染

##### SkyRenderer.hpp/cpp - 天空渲染器

**职责**：渲染天空穹顶、太阳、月亮和星星。

**主要内容**：
- 天空穹顶渲染（渐变色）
- 太阳渲染（带光晕）
- 月亮渲染（月相）
- 星星渲染（1500 颗星星）
- 天空颜色计算（基于时间、天气）

##### CelestialCalculations.hpp/cpp - 天体计算

**职责**：计算太阳、月亮位置和天空颜色。

**主要内容**：
- `calculateCelestialAngle()`：天体角度计算
- `calculateMoonPhase()`：月相计算
- `calculateSunDirection()`：太阳方向计算
- `calculateSkyColor()`：天空颜色计算
- `calculateSunriseSunsetColor()`：日出日落颜色计算
- `calculateStarBrightness()`：星星亮度计算

#### 5.4 cloud/ - 云渲染

##### CloudRenderer.hpp/cpp - 云渲染器

**职责**：渲染天空中的云层。

**主要内容**：
- 两种渲染模式：
  - `Fast`：只渲染底面（单层平面）
  - `Fancy`：渲染完整 3D 立方体
- 云纹理加载（资源包或程序化生成）
- 云位置跟随相机
- 云颜色随时间/天气变化

#### 5.5 fog/ - 雾效果

##### FogManager.hpp/cpp - 雾效果管理器

**职责**：管理雾效果参数。

**主要内容**：
- `FogMode`：雾模式枚举（None, Linear, Exp2）
- `FogUBO`：雾效果 Uniform 缓冲区数据
- 线性雾（陆地）：fogStart/fogEnd
- 指数雾（水中/岩浆）：fogDensity
- 根据渲染距离和天气自动计算

#### 5.6 gui/ - GUI 渲染

##### GuiRenderer.hpp/cpp - GUI 渲染器

**职责**：渲染 2D GUI 元素。

**主要内容**：
- 文本渲染（支持 UTF-8）
- 矩形渲染（填充、边框、渐变）
- 纹理渲染（支持多图集）
- 字体渲染集成
- 多图集槽位支持：
  - 槽位 0：字体纹理
  - 槽位 1：物品纹理图集
  - 槽位 2+：GUI 纹理图集

##### 其他 GUI 组件

- **GuiAtlasRegistry**：图集注册表
- **GuiSpriteAtlas**：精灵图集
- **GuiSpriteManager**：精灵管理器
- **GuiSpriteParser**：精灵解析器
- **GuiTextureAtlas**：纹理图集
- **GuiTextureLoader**：纹理加载器

#### 5.7 entity/ - 实体渲染

##### EntityRenderer.hpp/cpp - 实体渲染器基类

**职责**：定义实体渲染器接口。

**主要内容**：
- `render()`：渲染实体
- `renderShadow()`：渲染阴影
- `renderNameTag()`：渲染名称标签

##### EntityPipeline.hpp/cpp - 实体渲染管线

**职责**：管理实体渲染的 Vulkan 管线。

**主要内容**：
- 实体网格创建/更新/销毁
- 模型矩阵推送常量
- 纹理图集绑定

##### EntityRendererManager.hpp/cpp - 实体渲染器管理器

**职责**：根据实体类型分派渲染。

**主要内容**：
- 渲染器注册和查找
- 实体网格缓存
- 模型网格生成

##### model/ - 实体模型

- **EntityModel**：模型基类
- **ModelRenderer**：模型渲染器
- **AnimalModels**：动物模型定义

#### 5.8 particle/ - 粒子系统

##### ParticleManager.hpp/cpp - 粒子管理器

**职责**：管理粒子的生命周期和渲染。

**主要内容**：
- 粒子添加和移除
- 粒子更新（tick）
- 粒子渲染（批量）
- 最大粒子数限制（16384）

##### particles/ - 具体粒子类型

- **RainParticle**：雨滴粒子
- **SnowParticle**：雪花粒子

#### 5.9 weather/ - 天气渲染

##### WeatherRenderer.hpp/cpp - 天气渲染器

**职责**：渲染雨滴和雪花效果。

**主要内容**：
- 雨/雪层渲染（非粒子方式，更高效）
- 根据天气强度调整密度
- 程序化纹理生成
- 与粒子系统配合使用

#### 5.10 item/ - 物品渲染

##### ItemRenderer.hpp/cpp - 物品渲染器

**职责**：在 GUI 中渲染物品图标。

**主要内容**：
- 物品图标渲染
- 方块物品纹理映射
- 物品堆叠数量显示

#### 5.11 block/ - 方块渲染

##### BreakProgressRenderer.hpp/cpp - 破坏进度渲染器

**职责**：渲染方块破坏进度覆盖层。

**主要内容**：
- 10 阶段破坏纹理
- 叠加混合模式渲染
- 从 `BreakProgressManager` 获取进度数据

##### BreakProgressManager.hpp/cpp - 破坏进度管理器

**职责**：管理玩家挖掘进度。

#### 5.12 firstperson/ - 第一人称手部渲染

**职责**：渲染玩家第一人称视角下的手部和手持物品。

**主要文件**：

##### FirstPersonRenderer.hpp/cpp - 第一人称渲染器主类

**核心功能**：
- 渲染玩家手臂（第一人称视角）
- 渲染手持物品
- 处理挥动手臂动画
- 处理使用物品动画（吃食物、拉弓等）
- 处理地图等特殊物品渲染

**动画系统**：
- 挥动动画（swing）：攻击或使用物品时触发
- 装备动画（equip）：切换手持物品时触发
- 使用物品动画（use）：根据物品类型不同（食物/弓/盾牌等）

##### PlayerModel.hpp/cpp - 玩家模型

**核心功能**：
- 扩展 BipedModel，添加玩家特有部件
- 外层皮肤装饰（帽子、外套、袖子、裤腿）
- 细手臂支持（Alex 模型）
- 手臂姿态支持（空手、持物品、拉弓、格挡等）

##### MatrixStack.hpp/cpp - 矩阵栈

**核心功能**：
- 管理变换层级（push/pop）
- translate/rotate/scale 变换
- 矩阵乘法优化
- 参考自 MC 1.16.5 MatrixStack

##### ItemCameraTransforms.hpp/cpp - 物品相机变换

**核心功能**：
- 定义物品在各种渲染场景下的变换参数
- TransformType：NONE, THIRD_PERSON_LEFT/RIGHT, FIRST_PERSON_LEFT/RIGHT, HEAD, GUI, GROUND, FIXED
- ItemTransform：旋转、平移、缩放参数
- 变换来源：物品模型 JSON 定义

##### ItemInHandRenderer.hpp/cpp - 手持物品渲染器

**核心功能**：
- 渲染玩家手中的物品
- 根据物品类型应用不同的变换
- 方块物品 vs 普通物品的渲染逻辑

##### ArmPose.hpp - 手臂姿态枚举

**定义的手臂姿态**：
- `Empty`：空手
- `Item`：持有普通物品
- `Block`：格挡（盾牌）
- `BowAndArrow`：拉弓
- `ThrowSpear`：投掷三叉戟
- `CrossbowCharge`：装填弩
- `CrossbowHold`：持有已装填的弩
- `EatOrDrink`：吃食物/喝药水
- `Map`：使用地图

---

### 6. util/ - 工具函数

#### GpuInfo.hpp - GPU 信息

**职责**：从 Vulkan 设备属性提取 GPU 信息。

**主要内容**：
- `DebugGpuInfo`：GPU 信息结构（厂商、型号、显存等）
- `getGpuInfo()`：从 Vulkan 属性提取信息

#### ShaderPath.hpp - 着色器路径

**职责**：解析着色器文件路径。

**主要内容**：
- `resolveShaderPath()`：从文件名解析完整路径
- 支持多种路径模式（build/shaders, shaders, bin/shaders）

---

## 模块关系

```
┌─────────────────────────────────────────────────────────────────┐
│                        Client Application                        │
└─────────────────────────────────────────────────────────────────┘
                                │
                                ▼
┌─────────────────────────────────────────────────────────────────┐
│                     Camera.hpp/cpp                               │
│  (第一人称相机实现，实现 ICamera 接口)                            │
└─────────────────────────────────────────────────────────────────┘
                                │
                                ▼
┌─────────────────────────────────────────────────────────────────┐
│                    trident/TridentEngine                          │
│  (渲染引擎主类，实现 IRenderEngine 接口)                          │
│  ├── core/TridentContext (Vulkan 实例/设备/队列)                 │
│  ├── core/TridentSwapchain (交换链)                              │
│  ├── core/render/FrameManager (帧同步)                           │
│  ├── core/render/RenderPassManager (渲染通道)                    │
│  ├── core/render/DescriptorManager (描述符)                      │
│  ├── core/render/UniformManager (Uniform 缓冲区)                 │
│  ├── chunk/ChunkRenderer (区块渲染)                              │
│  ├── sky/SkyRenderer (天空渲染)                                  │
│  ├── cloud/CloudRenderer (云渲染)                                │
│  ├── fog/FogManager (雾效果)                                     │
│  ├── gui/GuiRenderer (GUI 渲染)                                  │
│  ├── entity/EntityRendererManager (实体渲染)                     │
│  ├── particle/ParticleManager (粒子系统)                         │
│  ├── weather/WeatherRenderer (天气渲染)                          │
│  ├── item/ItemRenderer (物品渲染)                                │
│  └── block/BreakProgressRenderer (破坏进度渲染)                  │
└─────────────────────────────────────────────────────────────────┘
                                │
                                ▼
┌─────────────────────────────────────────────────────────────────┐
│                           api/                                    │
│  (平台无关抽象接口)                                               │
│  ├── IRenderEngine (渲染引擎接口)                                │
│  ├── ICamera (相机接口)                                          │
│  ├── IBuffer (缓冲区接口)                                        │
│  ├── IPipeline (管线接口)                                        │
│  ├── ITexture (纹理接口)                                         │
│  ├── RenderType (渲染类型)                                       │
│  └── RenderState (渲染状态)                                      │
└─────────────────────────────────────────────────────────────────┘
```

---

## 整体职责

Renderer 模块负责 Minecraft Reborn 客户端的所有渲染功能：

1. **渲染抽象**：通过 `api/` 提供平台无关的渲染接口
2. **Vulkan 实现**：通过 `trident/` 实现 Vulkan 后端
3. **区块渲染**：高效渲染游戏世界区块
4. **实体渲染**：渲染玩家、动物、物品等实体
5. **天空/天气**：渲染天空穹顶、太阳、月亮、星星、云、雨、雪
6. **GUI 渲染**：渲染 2D 界面元素
7. **视觉效果**：雾效果、粒子系统、破坏进度

---

## 输入和输出

### 输入

| 数据类型 | 来源 | 用途 |
|---------|------|------|
| 区块数据 | `ChunkData` | 生成区块网格 |
| 实体状态 | `Entity`, `ClientEntity` | 实体渲染 |
| 相机状态 | `Camera` | 视图/投影矩阵 |
| 时间状态 | `dayTime`, `gameTime` | 天空/天气效果 |
| 天气状态 | `rainStrength`, `thunderStrength` | 雨/雪/天空颜色 |
| 纹理资源 | `ResourceManager` | 方块/物品纹理 |
| GUI 数据 | UI 系统 | 文本/矩形/纹理绘制 |

### 输出

| 数据类型 | 目标 | 用途 |
|---------|------|------|
| 渲染帧 | 屏幕 | 显示游戏画面 |
| GPU 缓冲区 | GPU | 顶点/索引/Uniform 数据 |
| 纹理数据 | GPU | 纹理图集 |

---

## 依赖项

### 外部依赖

- **Vulkan SDK**：图形 API
- **VulkanMemoryAllocator**：GPU 内存管理
- **GLFW**：窗口系统
- **GLM**：数学库
- **spdlog**：日志

### 内部依赖

- `common/core/Types.hpp`：基础类型
- `common/core/Result.hpp`：错误处理
- `common/world/chunk/ChunkData.hpp`：区块数据
- `common/world/block/Block.hpp`：方块定义
- `common/resource/ResourceLocation.hpp`：资源定位
- `client/ui/Font.hpp`：字体渲染
- `client/resource/ResourceManager.hpp`：资源管理

---

## 使用方法

### 初始化渲染引擎

```cpp
#include "renderer/api/IRenderEngine.hpp"
#include "renderer/Camera.hpp"
#include "renderer/trident/core/TridentEngine.hpp"

// 创建渲染引擎
auto engine = mc::client::renderer::api::createRenderEngine(
    mc::client::renderer::api::RenderBackend::Vulkan);

// 配置
mc::client::renderer::api::RenderEngineConfig config;
config.appName = "Minecraft Reborn";
config.enableValidation = true;
config.enableVSync = true;
config.maxFramesInFlight = 2;

// 初始化
auto result = engine->initialize(glfwWindow, config);
if (!result.success()) {
    // 处理错误
}

// 初始化子渲染器
auto tridentEngine = static_cast<mc::client::renderer::trident::TridentEngine*>(engine.get());
tridentEngine->initializeChunkRenderer();
tridentEngine->initializeSkyRenderer();
tridentEngine->initializeGuiRenderer();
// ... 其他子渲染器
```

### 渲染循环

```cpp
// 创建相机
mc::client::Camera camera;
camera.setPosition(0.0f, 64.0f, 0.0f);
camera.setFOV(70.0f);
camera.setAspectRatio(windowWidth / windowHeight);

while (running) {
    // 更新相机
    camera.handleMouseMove(mouseDeltaX, mouseDeltaY);
    camera.update(deltaTime);
    
    // 开始帧
    engine->beginFrame();
    engine->setCamera(&camera);
    
    // 更新时间状态
    tridentEngine->updateTime(dayTime, gameTime, partialTick);
    tridentEngine->updateWeather(rainStrength, thunderStrength);
    
    // 渲染区块
    tridentEngine->chunkRenderer().render(commandBuffer, pipelineLayout);
    
    // 渲染天空
    tridentEngine->skyRenderer().render(commandBuffer, projection, view, cameraPos);
    
    // 渲染实体
    tridentEngine->entityRendererManager().renderWithPipeline(commandBuffer, entity, partialTick);
    
    // 渲染 GUI
    tridentEngine->guiRenderer().beginFrame(screenW, screenH);
    tridentEngine->guiRenderer().drawText("Hello World", 10, 10);
    tridentEngine->guiRenderer().render(commandBuffer);
    
    // 结束帧
    engine->endFrame();
    engine->present();
}
```

### 异步区块网格构建

```cpp
mc::client::MeshWorkerPool workerPool(-1);
workerPool.start();

mc::client::MeshBuildScheduler scheduler(workerPool, schedulerConfig);
scheduler.setViewState(viewState);
scheduler.tick();

scheduler.drainCompleted([&chunkRenderer](mc::client::MeshWorkerResult&& result) {
  chunkRenderer.updateChunk(result.chunkId, result.solidMesh);
}, 4);

workerPool.shutdown();
```

---

## 容易踩的坑

### 1. Vulkan 验证层

**问题**：Debug 模式下运行很慢。

**解决方案**：Release 构建默认关闭验证层。开发时开启，发布时关闭。

```cpp
mc::client::renderer::api::RenderEngineConfig config;
#ifdef _DEBUG
config.enableValidation = true;
#else
config.enableValidation = false;
#endif
```

### 2. 纹理图集 UV 坐标

**问题**：纹理显示错位或闪烁。

**解决方案**：使用 `TextureAtlas::getRegion()` 获取正确的 UV 坐标，而不是手动计算。

### 3. 区块网格更新卡顿

**问题**：接收大量区块时主线程卡顿。

**解决方案**：使用 `MeshWorkerPool` 异步构建网格，并限制每帧处理数量。

### 4. 多帧资源轮换

**问题**：Uniform 缓冲区数据竞争。

**解决方案**：使用 `IUniformBuffer` 的多帧轮换功能，每帧使用不同的缓冲区。

```cpp
auto uniformBuffer = engine->createUniformBuffer(sizeof(CameraUBO), 2);  // 双缓冲
// 每帧
uniformBuffer->advanceFrame();
uniformBuffer->upload(&cameraData, sizeof(CameraUBO));
```

### 5. 着色器路径

**问题**：找不到着色器文件。

**解决方案**：使用 `resolveShaderPath()` 解析路径，它会自动搜索多个目录。

```cpp
auto vertexPath = mc::client::resolveShaderPath("chunk.vert.spv");
auto fragmentPath = mc::client::resolveShaderPath("chunk.frag.spv");
```

### 6. 相机坐标系

**问题**：相机方向与预期不符。

**解决方案**：Minecraft 使用右手坐标系，yaw=0 看向 +Z 方向。参考 `Camera::updateVectors()` 的实现。

### 7. 雾效果参数

**问题**：雾效果不正确。

**解决方案**：
- 陆地使用线性雾（`FogMode::Linear`）
- 水下/岩浆使用指数雾（`FogMode::Exp2`）
- 雾颜色应与天空颜色协调

### 8. 实体渲染顺序

**问题**：透明实体渲染顺序错误。

**解决方案**：透明实体应按距离排序，从远到近渲染。使用 `RenderType::translucent()` 的排序索引。

### 9. GUI 坐标系

**问题**：GUI 元素位置不对。

**解决方案**：GUI 使用屏幕坐标系，左上角为 (0, 0)，Y 轴向下。

### 10. 帧大小变化

**问题**：窗口大小变化后渲染异常。

**解决方案**：监听窗口大小变化，调用 `engine->onResize()` 重建交换链。

### 11. ChunkMesher 液面剔除

**问题**：仅根据透明度来确定液体可见性，会在水生植被周围重新引入散乱的水面片。

**解决方案**：`ChunkMesher` 中的液面剔除必须将空碰撞的水下植物（如海草和海带）视为隐藏面的邻居。

### 12. MatrixStack 调用顺序

**问题**：在第一人称渲染中，`MatrixStack` 调用顺序直觉可能会产生误导。

**解决方案**：在第一人称渲染中，按原版顺序应用变换并依赖后乘语义；避免临时性的原地行/列编辑。

### 13. 第一人称物品网格缓存

**问题**：在双手之间共享一个第一人称物品网格缓存，会因为主手和副手在同一帧持有不同物品而抖动，并在每帧渲染时分配 GPU 内存。

**解决方案**：不要在双手之间共享一个第一人称物品网格缓存，主手和副手需要独立的缓存。

### 14. 第一人称网格回收

**问题**：退役的第一人称网格只在 `destroy()` 中回收，会导致重复的物品更改使旧的 Vulkan 缓冲区在整个会话期间保持活动。

**解决方案**：退役的第一人称网格必须在帧倒计时上回收，而不仅仅在 `destroy()` 中。

### 15. EntityPipeline 网格更新

**问题**：将动画网格更新切换回每帧销毁+创建，会导致 `vkAllocateMemory` 回到渲染热路径。

**解决方案**：`EntityPipeline::updateMesh(...)` 必须保留 GPU 缓冲区并仅在需要时增长容量；保持可重用的暂存缓冲区和原地上传。

### 16. MeshWorkerPool 职责边界

**问题**：把优先级逻辑放回 `MeshWorkerPool` 会导致职责混乱。

**解决方案**：优先级和取消策略属于 `MeshBuildScheduler`；`MeshWorkerPool` 应保持仅执行。

### 17. ChunkMesher 预留策略

**问题**：双层网格（实心 + 透明）使用相同的初始容量，会把峰值内存翻倍。

**解决方案**：`ChunkMesher` 的 `generateSplitMesh()` 预留策略必须按 pass 区分，透明层的初始容量要明显小于实心层。

### 18. MeshSchedulerViewState 更新时机

**问题**：如果视图状态过期，视锥体优先级和相机后取消将滞后于相机移动。

**解决方案**：每帧在调用 `ClientWorld::update(...)` 之前必须更新 `MeshSchedulerViewState`。

### 19. MeshBuildScheduler 并发预算

**问题**：把 `maxDispatchedTaskCount` 按视距线性放大到很大，完成队列里每个 chunk mesh 都可能是数 MB 级别，会导致内存压力。

**解决方案**：`ClientApplication` 里给 `MeshBuildScheduler` 的并发预算要保持保守。

### 20. 网格任务取消同步

**问题**：在分发前取消的待处理网格任务不会产生工作器结果，区块可能会因陈旧的任务 ID 而卡住，永远不会重新提交。

**解决方案**：保持 `activeMeshTaskId` 与调度器跟踪同步。

### 21. ChunkMesher API 签名变更

**问题**：更改 `ChunkMesher` 网格 API 时，需要同时更新运行时和测试。

**解决方案**：`tests/client/renderer/test_renderer.cpp` 现在调用 5 参数的 `generateSplitMesh(..., neighbors, cancelSignal)` 签名，确保测试与实现同步。

### 22. BlockModelCache 快速路径

**问题**：在渲染热路径中通过 `toModelKey()` 路由 `BlockModelCache::getBlockAppearance(const BlockState*)` 会重建模型键，引入可避免的字符串解析。

**解决方案**：`stateId` 缓存是预期的快速路径；不要在那里重建模型键。

### 23. TridentEngine MSAA 采样数

**问题**：`WindowConfig` 不再携带采样数，各处需要协调 MSAA 设置。

**解决方案**：`TridentEngine` 现在拥有实际的 MSAA 采样计数选择。`TridentContext::maxUsableSampleCount()` 将请求限制到硬件限制，`RenderPassManager` 在需要时创建多重采样颜色/深度附件加上解析附件，每个主通道管线必须接收相同的 `VkSampleCountFlagBits`。

---

## 涉及的测试用例

渲染模块的测试主要在 `tests/client/` 目录下：

| 测试文件 | 测试内容 |
|---------|---------|
| `CameraTest.cpp` | 相机位置、旋转、投影矩阵计算 |
| `MeshTypesTest.cpp` | 顶点格式、方块几何、网格数据操作 |
| `MeshWorkerPoolTest.cpp` | 异步网格构建、优先级队列、结果处理 |
| `AmbientOcclusionCalculatorTest.cpp` | AO 计算、光照采样 |
| `ChunkMesherTest.cpp` | 区块网格生成、面剔除 |
| `CelestialCalculationsTest.cpp` | 天体角度、天空颜色、月相计算 |
| `FogManagerTest.cpp` | 雾参数计算、模式切换 |
| `GuiRendererTest.cpp` | 文本渲染、矩形绘制、纹理绘制 |
| `TextureAtlasTest.cpp` | 纹理图集 UV 计算 |

运行测试：
```powershell
./build/bin/Release/mc_tests.exe --filter="renderer*"
```

---

## 参考文档

- [Vulkan Tutorial](https://vulkan-tutorial.com/)
- [Minecraft 1.16.5 Source Code](https://github.com/OfficialMinecraftMCP/Minecraft-1.16.5-MCP)
- [Minecraft Wiki - Rendering](https://minecraft.wiki/w/Rendering)
