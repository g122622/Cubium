/*
* Copyright (c) 2026 Guo Yi
* 
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
* 
* The above copyright notice and this permission notice shall be included in all
* copies or substantial portions of the Software.
* 
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
* 
*/

# 区块生成与网格构建对象复用改造计划

## 1. 目标

本计划聚焦以下三类高频分配点：

1. `ChunkGenerateTask` 的频繁 `make_unique`
2. 区块生成过程中反复创建的邻居窗口临时容器
3. `MeshData` 内部顶点/索引缓冲的反复扩容

目标不是引入一个全局通用 slab 分配器，而是按对象生命周期和线程边界做局部复用：

- 对线程内短命临时对象，优先使用 `thread_local scratch context`
- 对需要跨接口转移所有权的对象，谨慎使用对象池
- 所有复用都必须显式重置状态，不能依赖“析构后自然干净”

## 2. 当前问题归类

### 2.1 `ChunkGenerateTask`

当前路径：

- [ServerChunkManager.cpp](/E:/dev/minecraft-reborn-branch-3/src/server/world/ServerChunkManager.cpp:305)
- [ChunkGenerateTask.hpp](/E:/dev/minecraft-reborn-branch-3/src/server/world/ChunkGenerateTask.hpp:33)
- [ServerWorkerPool.hpp](/E:/dev/minecraft-reborn-branch-3/src/common/util/thread/ServerWorkerPool.hpp:106)

现状：

- 每次排队生成都 `std::make_unique<ChunkGenerateTask>`
- `ServerWorkerPool::submit()` 接受 `std::unique_ptr<ITask>`
- `ServerWorkerPool` 内部又将其转成 `std::shared_ptr<InternalTask>`

结论：

- `ChunkGenerateTask` 理论上可以池化
- 但它受制于 `ServerWorkerPool` 的任务所有权模型，改造侵入性最大
- 不适合作为第一阶段落地点

### 2.2 邻居窗口临时容器

当前路径：

- [ServerChunkManager.cpp](/E:/dev/minecraft-reborn-branch-3/src/server/world/ServerChunkManager.cpp:326)
- [ServerChunkManager.cpp](/E:/dev/minecraft-reborn-branch-3/src/server/world/ServerChunkManager.cpp:535)
- [ServerChunkManager.cpp](/E:/dev/minecraft-reborn-branch-3/src/server/world/ServerChunkManager.cpp:659)

现状：

- 每个生成阶段都会新建：
  - `std::vector<IChunk*> neighbors`
  - `std::vector<std::shared_ptr<ChunkData>> loadedNeighbors`
  - `std::vector<std::unique_ptr<ChunkPrimer>> missingNeighbors`
- 同步生成路径和异步生成路径都重复这套模式
- `missingNeighbors` 中还会频繁 `make_unique<ChunkPrimer>`

结论：

- 这些对象强烈适合线程本地复用
- 它们是“单次任务内部 scratch”，不应建全局共享池
- 第一阶段优先改这里

### 2.3 `MeshData`

当前路径：

- [MeshTypes.hpp](/E:/dev/minecraft-reborn-branch-3/src/client/renderer/MeshTypes.hpp:112)
- [MeshWorkerPool.cpp](/E:/dev/minecraft-reborn-branch-3/src/client/renderer/mesh/MeshWorkerPool.cpp:181)
- [ChunkMesher.cpp](/E:/dev/minecraft-reborn-branch-3/src/client/renderer/trident/chunk/ChunkMesher.cpp:104)

现状：

- `MeshWorkerResult` 自带两个 `MeshData`
- `ChunkMesher::generateSplitMesh()` 会不断向 `vertices` / `indices` push
- 当前只有预估 `reserve`，但没有长期复用策略
- 如果队列抖动明显，会有频繁扩容和释放

结论：

- `MeshData` 更适合“保留容量复用 + worker 本地缓冲”
- 不建议一开始做复杂的全局跨线程 buffer 池
- 第一阶段先补齐可复用语义和 worker 内复用设计

## 3. 生命周期与线程边界

### 3.1 `ChunkGenerateTask`

- 创建线程：主线程 / 调度线程
- 执行线程：`ServerWorkerPool` worker
- 回收线程：完成回调所在 worker 线程
- 所有权：`unique_ptr<ITask>` 提交后由 worker 池持有

约束：

- 若做对象池，归还必须发生在任务完全结束之后
- 不能在回调还要读取 `takeResult()` 之前就回池
- 池对象必须支持重复 `reset(...)`

### 3.2 邻居窗口 scratch

- 创建线程：执行线程自身
- 使用线程：单个 worker 或同步生成调用线程
- 生命周期：一次生成阶段内有效
- 所有权：不对外暴露，只作为内部临时存储

约束：

- 必须线程隔离
- 同步生成路径不能和异步 worker 共用同一份可变 scratch
- `clear()` 后保留容量，避免反复释放

### 3.3 `MeshData`

- 创建线程：mesh worker
- 生产线程：mesh worker
- 消费线程：主线程/渲染提交线程
- 生命周期：完成队列出队前不能复用底层 buffer

约束：

- 不能把正在排队等待主线程消费的 buffer 直接复用于下一任务
- 因此 `MeshData` 不适合“单份 worker scratch 直接暴露给结果队列”
- 需要“结果对象”和“scratch 对象”分离

## 4. 第一阶段落地策略

### 4.1 邻居窗口：引入 `ChunkGenerationScratchContext`

建议新增内部结构：

```cpp
struct ChunkGenerationScratchContext {
    std::vector<IChunk*> neighbors;
    std::vector<std::shared_ptr<ChunkData>> loadedNeighbors;
    std::vector<std::unique_ptr<ChunkPrimer>> missingNeighbors;

    void prepare(i32 radius);
    void reset();
};
```

设计要求：

- `prepare(radius)` 负责：
  - 计算 `diameter * diameter`
  - `resize()` 三个容器到目标大小
  - 将 `neighbors` 填空为 `nullptr`
- `reset()` 负责：
  - `neighbors.clear()`
  - `loadedNeighbors.clear()`
  - `missingNeighbors.clear()`
  - 但不释放 capacity

落点建议：

- 在 `ServerChunkManager.cpp` 内匿名命名空间定义
- 使用 `thread_local ChunkGenerationScratchContext`
- 同步路径和异步路径统一通过一个辅助函数取 scratch

建议新增辅助接口：

```cpp
ChunkGenerationScratchContext& getChunkGenerationScratchContext();
```

以及把 `collectNeighborChunks(...)` 改成支持 scratch 上下文直接传入，避免每次单独新建三个 vector。

### 4.2 `MeshData`：增加“清空但保留容量”的显式语义

当前 `clear()` 已经保留 `std::vector` 容量，但语义不够明确。建议补齐：

```cpp
void clearButKeepCapacity();
void shrinkToFitIfLarge(size_t maxVertexCapacity, size_t maxIndexCapacity);
```

目的：

- 明确调用点是在“复用缓冲”而不是普通清空
- 对异常大的 chunk mesh，允许回收过大容量，避免长期占住峰值内存

建议新增容量治理策略：

- 普通路径：`clearButKeepCapacity()`
- 若容量超过阈值，例如：
  - `vertices.capacity() > NORMAL_VERTEX_CAPACITY * 4`
  - `indices.capacity() > NORMAL_INDEX_CAPACITY * 4`
- 则执行一次有界收缩

### 4.3 `MeshWorkerPool`：引入结果缓冲复用的设计前置

第一阶段先不直接做跨线程池化实现，只做结构准备：

建议新增内部概念：

```cpp
struct MeshBuildScratch {
    MeshData solidMesh;
    MeshData transparentMesh;

    void reset();
};
```

用途：

- worker 内部先把 mesher 输出写到 scratch
- 后续第二阶段再决定：
  - 是把 scratch 的内容 swap 到结果对象
  - 还是引入 `MeshWorkerResultPool`

注意：

- 由于 `m_completedQueue` 跨线程传递结果，scratch 本体不能直接进完成队列
- 否则 worker 线程无法继续复用

### 4.4 `ChunkGenerateTask`：先做可池化设计，不立即接入

建议把 `ChunkGenerateTask` 改成支持复用的形态：

```cpp
class ChunkGenerateTask : public util::ITask {
public:
    void reset(ChunkCoord x, ChunkCoord z, const ChunkStatus& targetStatus, GeneratorFunc generator);
    void clearState();
};
```

`reset(...)` 必须重置：

- `m_x`
- `m_z`
- `m_targetStatus`
- `m_generator`
- `m_result`
- `m_success`

`clearState()` 必须在取消、失败、成功取走结果后都可安全调用。

这一步先只改类设计，不改 `ServerWorkerPool` 所有权模型。

## 5. 第二阶段方案

### 5.1 `ChunkGenerateTaskPool`

建议接口：

```cpp
class ChunkGenerateTaskPool {
public:
    std::unique_ptr<ChunkGenerateTask, TaskPoolDeleter> acquire(
        ChunkCoord x,
        ChunkCoord z,
        const ChunkStatus& targetStatus,
        ChunkGenerateTask::GeneratorFunc generator);

    void release(ChunkGenerateTask* task);
};
```

这里的关键不是池本身，而是 deleter。

问题点：

- `ServerWorkerPool::submit()` 当前签名固定为 `std::unique_ptr<ITask>`
- 无法直接携带自定义 deleter

因此第二阶段需要二选一：

1. 改 `ServerWorkerPool::submit()` 支持模板化 unique_ptr / 自定义 deleter
2. 改 `InternalTask` 持有“任务对象 + 归还器”，不再只依赖默认 delete

推荐方向：

- 不改成模板化整个 worker pool 接口
- 新增轻量归还器字段，减少模板扩散和编译污染

### 5.2 `ChunkPrimer` 占位对象复用

`missingNeighbors[index] = std::make_unique<ChunkPrimer>(...)` 也是热点。

第二阶段可考虑：

- 为 scratch context 追加 `std::vector<ChunkPrimer>` 或对象池
- 但前提是先确认 `WorldGenRegion` / generator 不持久化这些占位指针

在未确认之前，不建议把 `ChunkPrimer` 直接从 `unique_ptr` 改成裸复用数组。

## 6. 验证策略

### 6.1 功能验证

必须覆盖：

- 同步区块生成
- 异步区块生成
- 异步取消
- 邻居依赖阻塞与唤醒
- mesh 生成正常路径
- mesh 取消路径
- mesh 异常路径

重点检查：

- 复用后是否有脏状态串任务
- `missingNeighbors` 是否残留上一次 `ChunkPrimer`
- `MeshWorkerResult` 是否出现顶点/索引串帧

### 6.2 性能指标

建议记录：

- 每次区块生成阶段的临时容器容量变化
- `neighbors/loadedNeighbors/missingNeighbors` 复用命中率
- `MeshData` 的 capacity 峰值与稳定值
- 区块生成 P50/P95/P99 耗时
- mesh 构建 P50/P95/P99 耗时

### 6.3 内存边界

必须限制：

- scratch context 容量上限
- `MeshData` 容量回收阈值

否则长时间跑图后可能把偶发峰值容量永久留下。

## 7. 实施顺序

### 阶段 1：低风险复用

1. 新增 `ChunkGenerationScratchContext`
2. 改造 `collectNeighborChunks()` 与生成路径使用 scratch
3. 为 `MeshData` 增加显式复用/收缩语义
4. 在 `MeshWorkerPool` 中为后续 scratch 复用预留结构

### 阶段 2：任务对象池化

1. 让 `ChunkGenerateTask` 支持 `reset(...)`
2. 设计 `ChunkGenerateTaskPool`
3. 改造 `ServerWorkerPool` 支持池对象回收
4. 补全取消/异常归还路径

### 阶段 3：进阶复用

1. 视 profile 决定是否复用 `ChunkPrimer` 占位对象
2. 视 mesh 队列压力决定是否引入 `MeshWorkerResultPool`

## 8. 当前建议结论

现阶段最应该先做的是：

1. 区块生成 scratch 容器复用
2. `MeshData` 缓冲复用语义

不应该第一刀就做的是：

1. 直接把 `ChunkGenerateTask` 接成全对象池
2. 把 `MeshWorkerResult` 做成跨线程共享可变缓冲

原因很简单：

- 前两者收益明确、改动小、线程边界清楚
- 后两者一旦做错，就是状态污染和并发 bug

