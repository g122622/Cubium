# buffer/ - 缓冲区实现

本目录包含 Trident 渲染引擎的所有缓冲区类型实现。

## 目录结构

```
buffer/
├── TridentBuffer.hpp   # 缓冲区类声明（基类和所有派生类）
└── TridentBuffer.cpp   # 缓冲区类实现
```

## 内部模块关系

```
TridentBuffer（基类，实现 IBuffer 接口）
├── TridentStagingBuffer（暂存缓冲区，用于 CPU→GPU 传输）
├── TridentVertexBuffer（顶点缓冲区，DEVICE_LOCAL 内存）
├── TridentIndexBuffer（索引缓冲区，DEVICE_LOCAL 内存）
└── TridentUniformBuffer（Uniform 缓冲区，HOST_VISIBLE 内存，支持多帧轮换）
```

## 上下游外部依赖关系

**上游依赖（本模块依赖）：**
- `TridentContext`：Vulkan 上下文，提供设备、命令池、单次命令提交
- `api::IBuffer` / `api::IVertexBuffer` / `api::IIndexBuffer` / `api::IUniformBuffer` / `api::IStagingBuffer`：平台无关的缓冲区接口

**下游依赖（被谁使用）：**
- `TridentEngine`：创建和管理所有缓冲区类型
- `ChunkRenderer`：区块顶点/索引数据
- `EntityPipeline`：实体网格数据
- `GuiRenderer`：GUI 顶点数据
- `ParticleManager`：粒子顶点数据
- `SkyRenderer`：天空顶点数据
- `WeatherRenderer`：天气顶点数据
- `UniformManager`：相机和光照 Uniform 数据

## 容易踩的坑

1. **设备本地缓冲区不能直接映射**：`TridentVertexBuffer` 和 `TridentIndexBuffer` 使用 DEVICE_LOCAL 内存，不能直接调用 `map()`，必须通过 `upload()` 方法使用内部暂存缓冲区上传数据。`map()` 会返回 nullptr 并输出警告日志。

2. **每次 upload 创建临时暂存缓冲区**：Vertex/IndexBuffer 的 `upload()` 方法每次都会创建临时暂存缓冲区并在传输完成后销毁。对于频繁更新场景，考虑在外部管理暂存缓冲区复用以提升性能。

3. **Uniform 缓冲区多帧轮换**：`TridentUniformBuffer` 支持双缓冲/三缓冲，每帧前必须调用 `advanceFrame()` 切换到当前帧缓冲区，避免 GPU/CPU 竞争导致画面撕裂或数据损坏。

4. **同步上传阻塞**：`upload()` 方法内部使用 `beginSingleTimeCommands()` / `endSingleTimeCommands()` 同步等待 GPU 完成传输，大数据上传会阻塞 CPU。批量上传时应考虑合并数据减少调用次数。

5. **缓冲区销毁时机**：所有缓冲区在析构时会自动调用 `destroy()`，但如果上下文已销毁，`destroy()` 会因为 `device == VK_NULL_HANDLE` 而提前返回，导致 Vulkan 句柄泄漏。确保在销毁 `TridentContext` 前先销毁所有缓冲区。
