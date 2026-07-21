# buffer/ - 缓冲区实现

本目录包含 Trident 渲染引擎的缓冲区类型实现，以及统一暂存缓冲池。

## 目录结构

```
buffer/
├── TridentBuffer.hpp/cpp               # 缓冲区类（基类 + Vertex/Index/Uniform 派生）
└── TridentStagingBufferPool.hpp/cpp    # 统一暂存缓冲池（IStagingBufferPool 的 Vulkan 实现）
```

## 内部模块关系

```
TridentBuffer（基类，实现 IBuffer 接口）
├── TridentVertexBuffer（顶点缓冲区，DEVICE_LOCAL 内存）
├── TridentIndexBuffer（索引缓冲区，DEVICE_LOCAL 内存）
└── TridentUniformBuffer（Uniform 缓冲区，HOST_VISIBLE 内存，支持多帧轮换）

TridentStagingBufferPool —— 实现 api::IStagingBufferPool
    └── OffsetAllocator 在持久映射的大 HOST_VISIBLE|HOST_COHERENT buffer 内子分配暂存区间
```

> 旧的 `TridentStagingBuffer`（每次上传新建/销毁的暂存缓冲区）与 `api::IStagingBuffer` 接口已删除，统一由 `TridentStagingBufferPool` 取代。

## 上下游外部依赖关系

**上游依赖（本模块依赖）：**
- `TridentContext`：Vulkan 上下文，提供设备、命令池、单次命令提交（`beginSingleTimeCommands`/`endSingleTimeCommands`）
- `api::IBuffer` / `api::IVertexBuffer` / `api::IIndexBuffer` / `api::IUniformBuffer` / `api::IStagingBufferPool`：平台无关的缓冲区接口
- `OffsetAllocator`（三方）：暂存池内子分配

**下游依赖（被谁使用）：**
- `TridentContext::stagingPool()` — 池由 `TridentEngine` 创建并注入 Context，所有上传点经此访问
- `TridentVertexBuffer::upload` / `TridentIndexBuffer::upload` — 经 `stagingPool()` 上传到 DEVICE_LOCAL buffer
- `EntityPipeline` / `ChunkRenderer` / `FirstPersonRenderer` — mega-buffer 段子分配后的同步上传
- `AtlasHandle` / `ItemTextureAtlas` / `EntityTextureAtlas` / `TridentTexture` — 纹理整图/子区域上传

## 容易踩的坑

1. **设备本地缓冲区不能直接映射**：`TridentVertexBuffer` 和 `TridentIndexBuffer` 使用 DEVICE_LOCAL 内存，不能直接调用 `map()`，必须通过 `upload()` 方法上传数据。`upload()` 内部经 `m_context->stagingPool()` 同步上传（`stage`→`memcpy`→`copyToBuffer`→`release`），不再每次新建临时暂存 buffer。

2. **暂存缓冲池是唯一上传通道**：所有 CPU→GPU 数据上传应经 `TridentContext::stagingPool()`，不要再散落 `vkCreateBuffer`+`vkAllocateMemory` 的一次性 staging buffer。同步上传用 `stage`/`copyToBuffer`/`release`；每帧动画上传用 `stageAsync`/`backingBuffer` + 帧回收桶（见 `api/buffer/README.md`）。

3. **Uniform 缓冲区多帧轮换**：`TridentUniformBuffer` 支持双缓冲/三缓冲，每帧前必须调用 `advanceFrame()` 切换到当前帧缓冲区，避免 GPU/CPU 竞争导致画面撕裂或数据损坏。Uniform 数据小且每帧更新，走 HOST_VISIBLE 直接映射，不经暂存池。

4. **同步上传阻塞**：`copyToBuffer` 内部 submit 单次命令并等待 fence，大数据上传会阻塞 CPU。批量上传时应合并数据减少调用次数。

5. **缓冲区销毁时机**：所有缓冲区在析构时会自动调用 `destroy()`，但如果上下文已销毁，`destroy()` 会因为 `device == VK_NULL_HANDLE` 而提前返回，导致 Vulkan 句柄泄漏。确保在销毁 `TridentContext` 前先销毁所有缓冲区。暂存池本身由 `TridentEngine::destroy` 在设备销毁前释放。
