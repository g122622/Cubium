# core/ - Trident 核心组件

本目录包含 Trident 渲染引擎的核心 Vulkan 组件，是整个渲染系统的基础设施。

## 目录结构

```
core/
├── Trident.hpp              # 统一头文件，包含所有核心组件
├── TridentContext.hpp/cpp   # Vulkan 上下文（实例/设备/队列）
├── TridentEngine.hpp/cpp    # 主引擎类（实现 IRenderEngine 接口）
├── TridentSwapchain.hpp/cpp # 交换链管理
├── buffer/                  # 缓冲区实现
│   └── TridentBuffer.hpp/cpp
├── pipeline/                # 管线实现
│   └── TridentPipeline.hpp/cpp
├── render/                  # 渲染管理器
│   ├── DescriptorManager.hpp/cpp
│   ├── FrameManager.hpp/cpp
│   ├── RenderPassManager.hpp/cpp
│   └── UniformManager.hpp/cpp
└── texture/                 # 纹理实现
    ├── TridentTexture.hpp/cpp
    ├── TridentTextureAtlas.hpp/cpp
    ├── TridentTextureAtlasAdapter.hpp/cpp
    ├── AnimatedSprite.hpp/cpp
    └── TextureAtlasTicker.hpp/cpp
```

## 组件介绍

### TridentContext

Vulkan 上下文管理，负责：

- **实例创建**：创建 Vulkan 实例，配置验证层
- **物理设备选择**：选择合适的 GPU，查询设备特性
- **逻辑设备创建**：创建逻辑设备和队列
- **命令池管理**：创建和管理命令池
- **单次命令辅助**：提供 `beginSingleTimeCommands()` / `endSingleTimeCommands()` 方法

```cpp
// 使用示例
TridentContext context;
TridentConfig config;
config.enableValidation = true;
auto result = context.initialize(window, config);

// 单次命令执行（用于资源上传）
VkCommandBuffer cmd = context.beginSingleTimeCommands();
// ... 记录命令 ...
context.endSingleTimeCommands(cmd);
```

### TridentEngine

主引擎类，实现 `IRenderEngine` 接口：

- **生命周期管理**：`initialize()`, `destroy()`, `beginFrame()`, `endFrame()`, `present()`
- **资源创建**：创建顶点缓冲区、索引缓冲区、Uniform 缓冲区、纹理、纹理图集
- **渲染状态**：`setRenderType()`, `bindTexture()`, `bindUniformBuffer()`
- **子渲染器管理**：区块、天空、云、实体、GUI、粒子、天气、物品等渲染器
- **帧上下文**：管理相机矩阵、时间状态、天气状态
- **MSAA 支持**：根据设置和设备能力选择采样数

### TridentSwapchain

交换链管理：

- 创建/重建交换链
- 管理交换链图像和图像视图
- 处理窗口大小变化
- 获取下一帧图像和呈现

### buffer/ - 缓冲区模块

详见 [buffer/README.md](buffer/README.md)

### pipeline/ - 管线模块

图形管线封装：

- **TridentPipeline**：Vulkan 图形管线实现
- **TridentPipelineConfig**：管线配置结构
- **TridentPipelineCache**：管线缓存

### render/ - 渲染管理器模块

- **DescriptorManager**：描述符集布局、描述符池、描述符集分配
- **FrameManager**：命令缓冲区、信号量、栅栏、帧生命周期
- **RenderPassManager**：渲染通道、深度缓冲区、帧缓冲区（支持 MSAA + resolve）
- **UniformManager**：相机和光照 Uniform 缓冲区管理

### texture/ - 纹理模块

- **TridentTexture**：Vulkan 纹理（图像、视图、采样器）
- **TridentTextureAtlas**：纹理图集实现
- **TridentTextureAtlasAdapter**：图集适配器（实现 ITextureAtlas 接口）
- **AnimatedSprite**：动画精灵（帧动画）
- **TextureAtlasTicker**：图集动画 tick 管理

## 依赖关系

```
TridentEngine
├── TridentContext
├── TridentSwapchain
├── buffer/
│   └── TridentBuffer (所有渲染器使用)
├── pipeline/
│   └── TridentPipeline
├── render/
│   ├── DescriptorManager
│   ├── FrameManager
│   ├── RenderPassManager
│   └── UniformManager
└── texture/
    └── TridentTexture
```

## 使用示例

```cpp
#include "client/renderer/trident/core/Trident.hpp"

// 创建渲染引擎
auto engine = mc::client::renderer::api::createRenderEngine(
    mc::client::renderer::api::RenderBackend::Vulkan);

// 初始化
mc::client::renderer::api::RenderEngineConfig config;
config.appName = "MyGame";
config.enableValidation = true;
engine->initialize(window, config);

// 创建顶点缓冲区
auto vbo = engine->createVertexBuffer(1024, sizeof(Vertex));

// 上传数据
vbo->upload(vertexData, sizeof(vertexData), 0);

// 渲染循环
while (running) {
    engine->beginFrame();
    engine->setCamera(&camera);
    // 绘制...
    engine->endFrame();
    engine->present();
}

engine->destroy();
```

## 容易踩的坑

1. **缓冲区上传**：顶点和索引缓冲区是设备本地内存，必须通过 `upload()` 方法使用暂存缓冲区上传数据，不能直接映射。

2. **命令缓冲区**：`beginSingleTimeCommands()` 返回的命令缓冲区必须通过 `endSingleTimeCommands()` 提交，否则会泄漏命令缓冲区。

3. **帧同步**：不要超过 `maxFramesInFlight` 帧在飞，否则会导致 GPU/CPU 同步问题。

4. **MSAA**：创建管线时必须使用与渲染通道相同的 `rasterizationSamples`。

## 相关测试

- `tests/client/renderer/test_trident_engine.cpp` - 核心组件测试
- `tests/client/renderer/test_trident_api.cpp` - API 接口测试
