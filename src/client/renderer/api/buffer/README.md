# buffer/ - 缓冲区抽象接口

本目录是渲染缓冲区的**平台无关抽象层**，定义缓冲区与统一暂存缓冲池的接口，不依赖任何具体后端（Vulkan/OpenGL/...）。后端实现位于 `trident/core/buffer/`。

## 目录结构

```
api/buffer/
├── IBuffer.hpp              # 缓冲区接口（顶点、索引、Uniform）
├── IStagingBufferPool.hpp   # 统一暂存缓冲池接口 + StagingHandle 句柄
└── OffsetAllocatorHeader.hpp # OffsetAllocator（三方）头文件包装（补 #pragma once）
```

## IStagingBufferPool — 统一暂存缓冲池

用 OffsetAllocator 在一个持久映射的大 `HOST_VISIBLE|HOST_COHERENT` buffer 内子分配暂存区间，消除散落在各处的"每次上传都 `vkCreateBuffer`+`vkAllocateMemory`+`vkDestroyBuffer`+`vkFreeMemory`"反模式。

### 两种回收语义

- **同步模式**（`stage` / `copyToBuffer` / `release`）：`copyToBuffer` 内部 submit 单次命令缓冲并等待 fence，`release` 立即归还 offset。用于资源加载/初始化/图集子区域上传。
- **异步模式**（`stageAsync` / `backingBuffer`）：调用方自行将复制命令录进当前帧命令缓冲，句柄登记到 `frameIndex` 回收桶，池在下一帧 `recycleFrame` 归还。用于每帧动画上传，避免每帧阻塞等 fence。

### StagingHandle

```cpp
struct StagingHandle {
    void* mappedPtr;   // 已加 offset，直接 memcpy
    u64   offset;      // backing buffer 内偏移
    u32   metadata;    // OffsetAllocator 内部记账，release 时原样回传
    u32   segmentIndex;// 多段 backing buffer 索引（当前实现恒 0，预留扩展）
    u64   size;        // 实际数据大小（未含对齐 padding）
    bool  valid;       // 分配失败时 false
};
```

### 两种上传形态

- **buffer-to-buffer**（顶点/索引数据到 DEVICE_LOCAL buffer）：`stage(size)` → `memcpy(handle.mappedPtr, ...)` → `copyToBuffer(handle, dstBuffer, dstOffset)` → `release(handle)`。池内部做 submit+wait。
- **buffer-to-image**（纹理整图/子区域上传）：`stage(size)` → `memcpy` → 自行 `beginSingleTimeCommands` + layout transition + `vkCmdCopyBufferToImage(cmd, backingBuffer(0), dstImage, ..., bufferOffset = handle.offset, ...)` + layout transition + `endSingleTimeCommands` → `release(handle)`。`release` 在 cmd submit+wait 后安全。

### 为什么不封装 copyToImage

图集上传需要 `VkBufferImageCopy`（srcOffset/dstImageOffset/extent）+ image layout transition，后者必须由持 `VkImage` 的图集对象自己录制。池强行封装会破坏分层。故池只暴露 `backingBuffer()` + `handle.offset`，由调用方自行录制。

## 上下游外部依赖关系

**上游依赖（本模块依赖）：**
- `common/core/Result.hpp`、`common/core/Types.hpp`
- `OffsetAllocator`（三方，经 `OffsetAllocatorHeader.hpp` 包装）

**下游依赖（被谁使用）：**
- `trident/core/buffer/TridentStagingBufferPool` — Vulkan 后端实现
- `TridentContext::stagingPool()` — 池的归属，所有上传点经此访问
- `EntityPipeline` / `ChunkRenderer` / `FirstPersonRenderer` — mega-buffer 段子分配后的同步上传
- `AtlasHandle` / `ItemTextureAtlas` / `EntityTextureAtlas` / `TridentTexture` / `TridentVertexBuffer` / `TridentIndexBuffer` — 图集/buffer 上传点

## 容易踩的坑

1. **`release` 必须在 `copyToBuffer` 返回后调用**：同步模式下 `copyToBuffer` 内部 submit+wait fence，返回时 GPU 已完成复制，此时 `release` 归还 offset 才安全。提前 `release` 会让 offset 被复用、数据被覆盖。

2. **异步模式不调 `release`**：异步句柄登记到帧回收桶，由 `recycleFrame` 统一回收。若误调 `release` 会 double-free OffsetAllocator 的 node。

3. **buffer-to-image 的 `bufferOffset` 用 `handle.offset`**：`vkCmdCopyBufferToImage` 的 `VkBufferImageCopy.bufferOffset` 必须是池内偏移，不是 0。对齐由池保证（`kStagingAlign=16`，满足 `bufferOffset` 4 字节要求）。

4. **池未初始化时返回 nullptr**：`TridentContext::stagingPool()` 在 `TridentEngine::initialize` 创建池之前返回 nullptr。上传点应判空并报错（见 EntityPipeline/ChunkRenderer 的 `staging pool not available` 错误），不要静默回退到一次性 buffer。
