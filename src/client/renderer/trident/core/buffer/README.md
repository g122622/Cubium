# buffer/ - 缓冲区实现

本目录包含 Trident 渲染引擎的所有缓冲区类型实现。

## 目录结构

```
buffer/
├── TridentBuffer.hpp   # 缓冲区类声明（基类和所有派生类）
└── TridentBuffer.cpp   # 缓冲区类实现
```

## 文件介绍

### TridentBuffer.hpp/cpp

包含以下缓冲区类型：

| 类名 | 内存类型 | 用途 | 数据上传方式 |
|------|----------|------|-------------|
| `TridentBuffer` | 基类 | 通用 Vulkan 缓冲区基类 | - |
| `TridentStagingBuffer` | HOST_VISIBLE | CPU 到 GPU 数据传输 | `upload()` 直接映射 |
| `TridentVertexBuffer` | DEVICE_LOCAL | 顶点数据存储 | `upload()` 通过暂存缓冲区 |
| `TridentIndexBuffer` | DEVICE_LOCAL | 索引数据存储 | `upload()` 通过暂存缓冲区 |
| `TridentUniformBuffer` | HOST_VISIBLE | Uniform 数据 | `upload()` 直接映射 |

## 使用方法

### 顶点缓冲区

```cpp
#include "client/renderer/trident/core/buffer/TridentBuffer.hpp"

using namespace mc::client::renderer::trident;
using namespace mc::client::renderer::api;

// 创建顶点缓冲区
TridentVertexBuffer vbo;
auto result = vbo.create(context, sizeof(Vertex) * 1000, sizeof(Vertex));

// 上传顶点数据
Vertex vertices[1000] = { /* ... */ };
result = vbo.upload(vertices, sizeof(vertices), 0);

// 在渲染时绑定
VkCommandBuffer cmd = ...;
vbo.bind(cmd);

// 获取顶点数量
u32 count = vbo.vertexCount();

// 销毁
vbo.destroy();
```

### 索引缓冲区

```cpp
// 创建索引缓冲区
TridentIndexBuffer ibo;
auto result = ibo.create(context, sizeof(u32) * 6000, IndexType::U32);

// 上传索引数据
u32 indices[6000] = { /* ... */ };
result = ibo.upload(indices, sizeof(indices), 0);

// 在渲染时绑定
ibo.bind(cmd);

// 获取索引数量
u32 count = ibo.indexCount();
```

### 暂存缓冲区（手动方式）

```cpp
// 创建暂存缓冲区
TridentStagingBuffer staging;
auto result = staging.create(context, sizeof(Vertex) * 100);

// 上传数据到暂存缓冲区
result = staging.upload(vertices, sizeof(vertices), 0);

// 获取命令缓冲区并复制到目标缓冲区
VkCommandBuffer cmd = context->beginSingleTimeCommands();
result = staging.copyTo(cmd, &vbo, sizeof(vertices));
context->endSingleTimeCommands(cmd);

staging.destroy();
```

### Uniform 缓冲区

```cpp
// 创建 Uniform 缓冲区（双缓冲）
TridentUniformBuffer ubo;
auto result = ubo.create(context, sizeof(CameraUBO), 2);

// 每帧上传数据
ubo.advanceFrame();  // 切换到当前帧的缓冲区
ubo.upload(&cameraData, sizeof(CameraUBO), 0);

// 获取当前帧的缓冲区句柄
VkBuffer buffer = ubo.buffer(ubo.currentFrameIndex());
```

## 数据上传流程

### 设备本地缓冲区（Vertex/Index）

```
┌─────────────────┐
│   CPU 内存      │  原始数据
└────────┬────────┘
         │ stagingBuffer.upload()
         ▼
┌─────────────────┐
│ TridentStaging  │  HOST_VISIBLE 内存
│    Buffer       │  (CPU 可访问)
└────────┬────────┘
         │ vkCmdCopyBuffer
         ▼
┌─────────────────┐
│ TridentVertex/  │  DEVICE_LOCAL 内存
│  IndexBuffer    │  (GPU 高速访问)
└─────────────────┘
```

### 关键实现细节

1. **同步上传**：`upload()` 方法使用 `beginSingleTimeCommands()` / `endSingleTimeCommands()` 确保数据传输完成后再返回

2. **参数验证**：
   - 检查上下文是否初始化
   - 检查数据指针和大小有效性
   - 检查偏移范围是否超出缓冲区大小

3. **自动清理**：暂存缓冲区在数据传输完成后自动销毁

## 模块关系

```
TridentBuffer
├── 被 ChunkRenderer 使用（顶点/索引数据）
├── 被 EntityPipeline 使用（实体网格）
├── 被 GuiRenderer 使用（GUI 顶点）
├── 被 ParticleManager 使用（粒子顶点）
├── 被 SkyRenderer 使用（天空顶点）
└── 被 WeatherRenderer 使用（天气顶点）
```

## 性能考虑

1. **设备本地内存**：顶点和索引缓冲区使用 DEVICE_LOCAL 内存，确保 GPU 最佳访问性能

2. **暂存缓冲区生命周期**：每次上传创建临时暂存缓冲区，适合一次性上传场景；频繁更新场景应考虑复用暂存缓冲区

3. **Uniform 缓冲区多帧轮换**：避免 GPU/CPU 竞争，确保帧在飞时数据不会被覆盖

## 错误处理

所有方法返回 `Result<T>` 或 `Result<void>`：

```cpp
auto result = vbo.create(context, size, stride);
if (result.failed()) {
    spdlog::error("Failed to create vertex buffer: {}", result.error().message());
    return;
}
```

错误码：
- `ErrorCode::NullPointer`：上下文为空
- `ErrorCode::OutOfMemory`：内存分配失败
- `ErrorCode::InvalidArgument`：参数无效（空指针、零大小）
- `ErrorCode::OutOfRange`：偏移超出缓冲区范围

## 相关测试

- `tests/client/renderer/test_trident_engine.cpp` - `TridentBufferTest` 测试套件
  - `CreateVertexBuffer` / `CreateIndexBuffer`
  - `VertexBufferUploadViaStaging` / `IndexBufferUploadViaStaging`
  - `VertexBufferDirectUpload` / `IndexBufferDirectUpload`
  - `VertexBufferDirectUploadWithOffset` / `IndexBufferDirectUploadWithOffset`
  - `VertexBufferUploadInvalidParameters` / `IndexBufferUploadInvalidParameters`
  - `CreateUniformBuffer` / `UniformBufferUpload`
  - `CreateStagingBuffer` / `StagingBufferUploadAndCopy`
