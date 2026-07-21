# Trident 渲染引擎

Trident 是 Cubium 客户端的 Vulkan 渲染引擎，实现了完整的 3D 渲染管线，包括区块渲染、实体渲染、天空渲染、粒子系统、GUI 渲染等功能。

## 目录结构

```
trident/
├── core/                    # 核心组件
│   ├── Trident.hpp          # 统一头文件
│   ├── TridentContext.hpp/cpp   # Vulkan 上下文（实例、设备、队列管理）
│   ├── TridentEngine.hpp/cpp    # 主引擎类（实现 IRenderEngine）
│   ├── TridentSwapchain.hpp/cpp # 交换链管理
│   ├── buffer/              # 缓冲区
│   │   └── TridentBuffer.hpp/cpp    # 顶点/索引/Uniform/暂存缓冲区
│   ├── pipeline/            # 管线
│   │   └── TridentPipeline.hpp/cpp  # 图形管线
│   ├── render/              # 渲染管理
│   │   ├── DescriptorManager.hpp/cpp # 描述符管理
│   │   ├── FrameManager.hpp/cpp      # 帧同步管理
│   │   ├── RenderPassManager.hpp/cpp # 渲染通道管理（支持 MSAA + resolve）
│   │   └── UniformManager.hpp/cpp    # Uniform 缓冲区管理
│   └── texture/             # 纹理
│       ├── TridentTexture.hpp/cpp    # 纹理/纹理图集
│       ├── AnimatedSprite.hpp/cpp    # 动画精灵（帧动画）
│       └── TextureAtlasTicker.hpp/cpp # 图集动画tick管理
├── chunk/                   # 区块渲染
│   ├── AmbientOcclusionCalculator.hpp/cpp # 环境光遮蔽计算
│   ├── ChunkMesher.hpp/cpp  # 区块网格生成
│   ├── ChunkRenderer.hpp/cpp # 区块 GPU 渲染
│   └── README.md             # 区块模块文档
├── cloud/                   # 云渲染
│   └── CloudRenderer.hpp/cpp # 云层渲染器（Fast/Fancy 模式）
├── entity/                  # 实体渲染
│   ├── EntityRenderer.hpp/cpp       # 实体渲染器基类
│   ├── EntityRendererManager.hpp/cpp # 实体渲染器管理
│   ├── EntityPipeline.hpp/cpp       # 实体渲染管线
│   ├── EntityTextureAtlas.hpp/cpp   # 实体纹理图集
│   ├── ItemEntityRenderer.hpp/cpp   # 物品实体渲染器
│   ├── LivingRenderer.hpp           # 生物渲染器模板
│   ├── AnimalRenderers.hpp          # 动物渲染器
│   └── model/                # 实体模型
│       ├── EntityModel.hpp/cpp      # 模型基类
│       ├── ModelRenderer.hpp/cpp    # 模型部件渲染
│       └── animal/                  # 动物模型目录（每模型一文件）
├── fog/                     # 雾效果
│   └── FogManager.hpp/cpp   # 雾管理器（Linear/Exp2）
├── firstperson/             # 第一人称渲染
│   ├── FirstPersonRenderer.hpp/cpp  # 第一人称渲染器（手部、手持物品）
│   ├── PlayerModel.hpp/cpp          # 玩家模型（双足模型扩展）
│   ├── ItemInHandRenderer.hpp/cpp   # 手持物品渲染器
│   ├── MatrixStack.hpp/cpp          # 矩阵栈（变换层级管理）
│   ├── ItemCameraTransforms.hpp/cpp # 物品相机变换
│   ├── ArmPose.hpp                  # 手臂姿态枚举
│   └── README.md                    # 模块文档
├── gui/                     # GUI 渲染
│   ├── GuiRenderer.hpp/cpp  # GUI 渲染器
│   ├── GuiTextureAtlas.hpp/cpp  # GUI 纹理图集
│   ├── GuiTextureLoader.hpp/cpp  # GUI 纹理加载器
│   ├── GuiTextureManager.hpp/cpp # GUI 纹理管理
│   ├── GuiSprite.hpp        # GUI 精灵定义
│   ├── GuiSpriteAtlas.hpp/cpp    # GUI 精灵图集
│   ├── GuiSpriteManager.hpp/cpp  # GUI 精灵管理
│   ├── GuiSpriteParser.hpp/cpp   # GUI 精灵解析
│   ├── GuiSpriteRegistry.hpp/cpp # GUI 精灵注册表
│   └── GuiAtlasRegistry.hpp # 多图集注册表
├── item/                    # 物品渲染
│   └── ItemRenderer.hpp/cpp # 物品图标渲染
├── particle/                # 粒子系统
│   ├── Particle.hpp/cpp     # 粒子基类
│   ├── ParticleManager.hpp/cpp # 粒子管理器
│   └── particles/           # 粒子类型
│       ├── RainParticle.hpp/cpp # 雨滴粒子
│       └── SnowParticle.hpp/cpp # 雪花粒子
├── sky/                     # 天空渲染
│   ├── SkyRenderer.hpp/cpp  # 天空盒渲染器
│   └── CelestialCalculations.hpp/cpp # 天体计算
├── util/                    # 工具
│   └── VulkanUtils.hpp      # Vulkan 辅助函数
├── weather/                 # 天气渲染
│   └── WeatherRenderer.hpp/cpp # 雨雪渲染器
├── block/                   # 方块渲染
│   ├── BreakProgressManager.hpp/cpp # 破坏进度管理
│   └── BreakProgressRenderer.hpp/cpp # 破坏进度渲染
└── blockentity/             # 方块实体渲染器
    ├── IBlockEntityRenderer.hpp    # 渲染器接口模板
    ├── BlockEntityRenderer.hpp/cpp # 渲染器基类
    ├── BlockEntityRendererDispatcher.hpp/cpp # 渲染器调度器
    ├── README.md                   # 模块文档
    ├── model/                      # 方块实体模型
    └── renderers/                  # 具体渲染器
        └── PistonRenderer.hpp/cpp  # 活塞渲染器
```

## 内部模块关系

```
TridentEngine（主引擎）
    ├── TridentContext（Vulkan上下文）
    ├── TridentSwapchain（交换链）
    ├── FrameManager（帧同步）
    └── DescriptorManager（描述符）
            └── UniformManager（UBO管理）

渲染器层（依赖 core 组件）：
    ├── ChunkRenderer → ChunkMesher → AmbientOcclusionCalculator
    ├── EntityRenderer → EntityModel/ModelRenderer
    ├── SkyRenderer/CloudRenderer
    ├── WeatherRenderer → ParticleManager
    ├── GuiRenderer → GuiSprite系统
    └── ItemRenderer/BlockEntityRenderer
```

## 上下游外部依赖关系

### 上游依赖（本模块依赖的模块）

| 模块 | 用途 |
|------|------|
| `common/core` | 基础类型定义 |
| `common/world` | 区块数据、方块状态 |
| `common/entity` | 实体数据 |
| `common/resource` | 纹理、模型资源加载 |
| `common/util` | 数学工具、断言 |
| `client/renderer/api` | 平台无关的渲染接口（IRenderEngine 等） |
| `client/ui` | 字体数据 |

### 下游依赖（依赖本模块的模块）

| 模块 | 用途 |
|------|------|
| `client/ClientApplication` | 主客户端应用，创建并驱动渲染引擎 |
| `client/world/ClientWorld` | 提供世界数据给渲染器 |

## 容易踩的坑

### Vulkan 资源生命周期

**问题**：在设备销毁后访问 Vulkan 对象会导致崩溃。

**解决**：确保资源销毁顺序正确 —— 先销毁资源（texture.destroy()），再销毁设备。

### 描述符集更新时机

**问题**：在渲染过程中更新描述符集可能导致闪烁。

**解决**：使用帧同步，只在帧开始时更新描述符集，等待上一帧完成后再更新。

### 纹理图集溢出

**问题**：图集空间不足导致纹理加载失败。

**解决**：使用足够大的图集尺寸（如 4096x4096），实现动态图集扩展，监控图集使用率。

### 区块网格内存泄漏

**问题**：频繁加载/卸载区块可能导致内存增长；若每个区块独占 `VkBuffer`+`VkDeviceMemory`、旧 buffer 延迟销毁，峰期新旧共存会令显存翻倍。

**解决**：`ChunkRenderer` 已改造为 mega-buffer 子分配（见 chunk/README.md）——vertex（128MB/段）/index（32MB/段）大 buffer 段内用 OffsetAllocator 子分配，区块只持段内 offset，旧区间延迟归还后由 OffsetAllocator 复用，段 `VkBuffer`/`VkDeviceMemory` 全程不销毁（仅 `destroy()` 释放）。配合统一暂存池上传，消除每区块 `vkAllocateMemory` 抖动与峰期翻倍。顶点格式统一为 f32（28B/顶点）、索引用 u16，进一步压低稳态显存。

### 验证层性能影响

**问题**：Debug 模式下验证层会显著降低性能。

**解决**：开发时启用验证层，Release 构建禁用验证层，使用 `MC_ENABLE_VULKAN_VALIDATION` CMake 选项控制。

### 窗口大小变化处理

**问题**：窗口大小变化时交换链需要重建，否则渲染崩溃。

**解决**：`onWindowResize` 时需调用 `m_swapchain.recreate()` 并更新所有依赖交换链的资源（RenderPassManager、FrameManager 等）。

### 雾效果参数对齐

**问题**：UBO 数据对齐不正确导致渲染异常。

**解决**：FogUBO 结构体需使用 `alignas` 确保正确对齐（fogStart/fogEnd/fogDensity/fogMode 用 `alignas(4)`，fogColor 用 `alignas(16)`）。

### 多线程区块网格生成

**问题**：多线程访问共享资源导致竞争。

**解决**：使用互斥锁保护共享数据，ChunkMesher 支持协作取消信号（generateMesh/generateSplitMesh/generateSectionMesh 均接收 `abortSignal`）。

### 云高度与维度渲染

**注意**：云高度根据维度动态设置（主世界 192 格，下界/末地无云）。通过 `TridentEngine::setCloudHeight(cloudHeight, hasClouds)` 设置。使用 `hasClouds` 布尔字段判断是否渲染云，而非依赖 NaN 检测（由于 `-ffast-math`，NaN 检测不可靠）。

### MSAA 采样配置

**注意**：管线封装保留 1x 作为通用默认值，但主通道管线会被 `TridentEngine` 覆盖成当前实际 sample count。新增主渲染器时必须沿用同一份采样配置。

## 外部资源

- [Vulkan 教程](https://vulkan-tutorial.com/)
- [Minecraft 1.16.5 渲染系统](https://minecraft.wiki/w/Rendering)
