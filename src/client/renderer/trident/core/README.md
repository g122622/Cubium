# core/ - Trident 核心组件

本目录包含 Trident 渲染引擎的核心 Vulkan 组件，是整个渲染系统的基础设施。

## 目录结构

```
core/
├── Trident.hpp              # 统一头文件，包含所有核心组件
├── TridentContext.hpp/cpp   # Vulkan 上下文（实例/设备/队列/命令池）
├── TridentEngine.hpp/cpp    # 主引擎类（实现 IRenderEngine 接口）
├── TridentSwapchain.hpp/cpp # 交换链管理
├── buffer/                  # 缓冲区实现
│   ├── TridentBuffer.hpp    # 缓冲区类声明（基类和所有派生类）
│   └── TridentBuffer.cpp    # 缓冲区类实现
├── pipeline/                # 管线实现
│   ├── TridentPipeline.hpp  # 图形管线封装
│   └── TridentPipeline.cpp
├── render/                  # 渲染管理器
│   ├── DescriptorManager.hpp/cpp  # 描述符集布局/池/集分配
│   ├── FrameManager.hpp/cpp       # 命令缓冲区/信号量/栅栏/帧生命周期
│   ├── RenderPassManager.hpp/cpp  # 渲染通道/深度缓冲区/帧缓冲区
│   └── UniformManager.hpp/cpp     # 相机和光照 Uniform 缓冲区管理
└── texture/                 # 纹理实现
    ├── TridentTexture.hpp/cpp         # Vulkan 纹理和纹理图集
    ├── AnimatedSprite.hpp/cpp         # 动画精灵（帧动画）
    └── TextureAtlasTicker.hpp/cpp     # 图集动画 tick 管理
```

## 内部模块关系

```
TridentEngine（主入口）
├── TridentContext（Vulkan 上下文，被所有组件依赖）
├── TridentSwapchain（交换链，依赖 Context）
├── buffer/TridentBuffer（缓冲区，依赖 Context）
│   ├── TridentVertexBuffer
│   ├── TridentIndexBuffer
│   └── TridentUniformBuffer
├── buffer/TridentStagingBufferPool（统一暂存缓冲池，依赖 Context + OffsetAllocator，注入 Context 供所有上传点复用）
├── pipeline/TridentPipeline（管线，依赖 Context 和 DescriptorManager）
├── render/（渲染管理器）
│   ├── DescriptorManager（依赖 Context）
│   ├── FrameManager（依赖 Context 和 Swapchain）
│   ├── RenderPassManager（依赖 Context 和 Swapchain）
│   └── UniformManager（依赖 Context 和 DescriptorManager）
└── texture/（纹理模块）
    ├── TridentTexture（依赖 Context）
    ├── TridentTextureAtlas（依赖 Texture）
    └── AnimatedSprite + TextureAtlasTicker（动画系统）
```

## 上下游外部依赖关系

**上游依赖（本模块依赖）：**
- `api/IRenderEngine`、`api/IVertexBuffer` 等接口 - 平台无关的渲染抽象接口
- `GLFWwindow` - 窗口系统
- Vulkan SDK - 图形 API

**下游依赖（被谁使用）：**
- `ChunkRenderer` - 区块顶点/索引数据
- `EntityPipeline` - 实体网格数据
- `GuiRenderer` - GUI 顶点数据
- `ParticleManager` - 粒子顶点数据
- `SkyRenderer` / `CloudRenderer` - 天空/云顶点数据
- `WeatherRenderer` - 天气顶点数据
- `ItemRenderer` - 物品渲染

## 容易踩的坑

1. **缓冲区上传**：顶点和索引缓冲区使用 DEVICE_LOCAL 内存，不能直接映射。`upload()` 经 `TridentContext::stagingPool()` 统一暂存缓冲池同步上传，不再每次新建临时 staging buffer。详见 [buffer/README.md](buffer/README.md)。

2. **命令缓冲区泄漏**：`beginSingleTimeCommands()` 返回的命令缓冲区必须通过 `endSingleTimeCommands()` 提交，否则会泄漏命令缓冲区。

3. **帧同步**：不要超过 `maxFramesInFlight` 帧在飞，否则会导致 GPU/CPU 同步问题。

4. **MSAA 采样数**：创建管线时必须使用与渲染通道相同的 `rasterizationSamples`。

5. **动画纹理生命周期**：动画纹理需要在主线程 tick 调用 `tickTextureAnimations()` 更新帧状态，在渲染帧调用 `uploadAnimationFrames()` 上传到 GPU。详见 [texture/README.md](texture/README.md)。

6. **Uniform 缓冲区帧轮换**：每帧前必须调用 `advanceFrame()` 切换到当前帧缓冲区，避免 GPU/CPU 竞争。

7. **缓冲区销毁顺序**：所有缓冲区必须在 `TridentContext` 销毁前先销毁，否则 Vulkan 句柄会泄漏。
