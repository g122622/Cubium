# Renderer 模块

客户端渲染系统，负责 Cubium 的所有渲染功能。

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
│   ├── pipeline/
│   │   ├── IPipeline.hpp         # 管线接口
│   │   ├── RenderState.hpp       # 渲染状态（混合/深度/剔除）
│   │   └── RenderType.hpp        # 渲染类型（MC 1.16.5 风格）
│   └── texture/
│       ├── ITexture.hpp          # 纹理接口
│       ├── ITextureAtlas.hpp     # 纹理图集构建器接口
│       └── TextureRegion.hpp     # 纹理区域（UV坐标）
├── Camera.hpp/cpp                # 第一人称相机实现
├── MeshTypes.hpp/cpp             # 网格类型定义（顶点、面、图集）
├── mesh/
│   ├── MeshBuildScheduler.hpp/cpp # 独立调度器（视锥/距离优先 + 取消）
│   ├── MeshBuildTask.hpp/cpp      # ITask 子类：单区块网格构建
│   ├── MeshDataPool.hpp/cpp       # MeshData 回收池（单桶 free-list）
│   ├── MeshResultQueue.hpp/cpp    # 线程安全结果队列
│   └── MeshWorkerTypes.hpp        # MeshWorkerResult 结构体
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
│   │       ├── animal/             # 动物模型目录（每模型一文件）
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

## 内部模块关系

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

核心调用链：
- `TridentEngine` 是渲染引擎主入口，协调所有子渲染器
- `api/` 定义抽象接口，`trident/` 提供 Vulkan 实现
- `ChunkMesher` 将 `ChunkData` 转换为 `MeshData`，`ChunkRenderer` 负责GPU上传和渲染
- `UniversalWorkerPool`(ClientCompute) + `MeshBuildScheduler` 实现异步网格构建，调度与执行解耦（计算池由 ClientApplication 持有）

## 上下游外部依赖关系

### 上游依赖（本模块依赖的）

**外部依赖**：
- **Vulkan SDK** - 图形 API
- **VulkanMemoryAllocator** - GPU 内存管理
- **GLFW** - 窗口系统
- **GLM** - 数学库
- **spdlog** - 日志

**内部依赖**：
- `common/core/Types.hpp` - 基础类型
- `common/core/Result.hpp` - 错误处理
- `common/world/chunk/ChunkData.hpp` - 区块数据
- `common/world/block/Block.hpp` - 方块定义
- `common/resource/ResourceLocation.hpp` - 资源定位
- `client/ui/Font.hpp` - 字体渲染
- `client/resource/ResourceManager.hpp` - 资源管理

### 下游依赖（依赖本模块的）

- `client/ClientApplication.hpp` - 客户端应用主类，初始化和使用渲染引擎
- `client/world/ClientWorld.hpp` - 客户端世界管理器，触发区块网格构建

## 容易踩的坑

### 1. Vulkan 验证层性能

Debug 模式下验证层会显著降低性能。Release 构建应关闭验证层。

### 2. 纹理图集 UV 坐标

纹理显示错位或闪烁时，使用 `TextureAtlas::getRegion()` 获取正确的 UV 坐标，不要手动计算。

### 3. 区块网格更新卡顿

接收大量区块时主线程会卡顿。使用 `UniversalWorkerPool`(ClientCompute) 异步构建网格，并限制每帧处理数量。

### 4. 多帧资源轮换

Uniform 缓冲区数据竞争时，使用 `IUniformBuffer` 的多帧轮换功能，每帧使用不同的缓冲区。

### 5. 着色器路径

找不到着色器文件时，使用 `resolveShaderPath()` 解析路径，它会自动搜索多个目录。

### 6. 相机坐标系

Minecraft 使用右手坐标系，yaw=0 看向 +Z 方向。参考 `Camera::updateVectors()` 的实现。

### 7. 雾效果参数

- 陆地使用线性雾（`FogMode::Linear`）
- 水下/岩浆使用指数雾（`FogMode::Exp2`）
- 雾颜色应与天空颜色协调

### 8. 实体渲染顺序

透明实体渲染顺序错误时，应按距离排序，从远到近渲染。使用 `RenderType::translucent()` 的排序索引。

### 9. GUI 坐标系

GUI 使用屏幕坐标系，左上角为 (0, 0)，Y 轴向下。

### 10. 帧大小变化

窗口大小变化后渲染异常时，监听窗口大小变化，调用 `engine->onResize()` 重建交换链。

### 11. ChunkMesher 液面剔除

仅根据透明度确定液体可见性，会在水生植被周围产生散乱的水面片。液面剔除必须将空碰撞的水下植物（如海草和海带）视为隐藏面的邻居。

### 12. MatrixStack 调用顺序

在第一人称渲染中，`MatrixStack` 调用顺序直觉可能会产生误导。按原版顺序应用变换并依赖后乘语义；避免临时性的原地行/列编辑。

### 13. 第一人称物品网格缓存

在双手之间共享一个第一人称物品网格缓存，会因为主手和副手在同一帧持有不同物品而抖动。主手和副手需要独立的缓存。

### 14. 第一人称网格回收

退役的第一人称网格只在 `destroy()` 中回收，会导致重复的物品更改使旧的 Vulkan 缓冲区在整个会话期间保持活动。退役的第一人称网格必须在帧倒计时上回收。

### 15. EntityPipeline 网格更新

将动画网格更新切换回每帧销毁+创建，会导致 `vkAllocateMemory` 回到渲染热路径。`EntityPipeline::updateMesh(...)` 必须保留 GPU 缓冲区并仅在需要时增长容量。

### 16. MeshBuildScheduler 职责边界

把优先级逻辑放回执行路径会导致职责混乱。优先级和取消策略属于 `MeshBuildScheduler`；`MeshBuildTask` 应只负责"构建并推结果"。计算池（`UniversalWorkerPool`/ClientCompute）仅提供通用算力。

### 17. ChunkMesher 预留策略

双层网格（实心 + 透明）使用相同的初始容量，会把峰值内存翻倍。`ChunkMesher` 的 `generateSplitMesh()` 预留策略必须按 pass 区分，透明层的初始容量要明显小于实心层。

### 18. MeshSchedulerViewState 更新时机

如果视图状态过期，视锥体优先级和相机后取消将滞后于相机移动。每帧在调用 `ClientWorld::update(...)` 之前必须更新 `MeshSchedulerViewState`。

### 19. MeshBuildScheduler 并发预算

把 `maxDispatchedTaskCount` 按视距线性放大到很大，完成队列里每个 chunk mesh 都可能是数 MB 级别，会导致内存压力。给 `MeshBuildScheduler` 的并发预算要保持保守。

### 20. 网格任务取消同步

在分发前取消的待处理网格任务不会产生工作器结果，区块可能会因陈旧的任务 ID 而卡住。保持 `activeMeshTaskId` 与调度器跟踪同步。

### 21. ChunkMesher API 签名变更

更改 `ChunkMesher` 网格 API 时，需要同时更新运行时和测试。测试需要调用正确的签名。

### 22. BlockModelCache 快速路径

在渲染热路径中通过 `toModelKey()` 路由 `BlockModelCache::getBlockAppearance(const BlockState*)` 会重建模型键，引入可避免的字符串解析。`stateId` 缓存是预期的快速路径。

### 23. TridentEngine MSAA 采样数

`WindowConfig` 不再携带采样数，各处需要协调 MSAA 设置。`TridentEngine` 拥有实际的 MSAA 采样计数选择。`TridentContext::maxUsableSampleCount()` 将请求限制到硬件限制，`RenderPassManager` 在需要时创建多重采样颜色/深度附件加上解析附件，每个主通道管线必须接收相同的 `VkSampleCountFlagBits`。
