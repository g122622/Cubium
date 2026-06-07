# Mesh 模块

本模块负责客户端区块网格构建的并行执行与调度决策，采用"独立调度器 + 纯执行器"双组件架构。

## 目录结构树

```text
src/client/renderer/mesh/
├── MeshWorkerPool.hpp          # 纯执行线程池（FIFO）
├── MeshWorkerPool.cpp
├── MeshBuildScheduler.hpp      # 独立调度器（优先级/视锥/取消）
├── MeshBuildScheduler.cpp
└── README.md
```

## 内部模块关系

```mermaid
graph LR
    A[ClientWorld] -->|submit MeshBuildRequest| B[MeshBuildScheduler]
    B -->|dispatch MeshWorkerTask| C[MeshWorkerPool]
    C -->|call| D[ChunkMesher]
    C -->|MeshWorkerResult| B
    B -->|latest result callback| A

    style A fill:#e8f5e9,stroke:#2e7d32,stroke-width:2px,color:#111
    style B fill:#fff3e0,stroke:#ef6c00,stroke-width:2px,color:#111
    style C fill:#e3f2fd,stroke:#1565c0,stroke-width:2px,color:#111
    style D fill:#f3e5f5,stroke:#6a1b9a,stroke-width:2px,color:#111
```

模块分工：
- **MeshBuildScheduler**（调度器）：策略层，负责优先级排序、视锥剔除、任务取消、代际管理。
- **MeshWorkerPool**（执行器）：算力层，负责线程池执行、结果回收、取消检查。

## 上下游外部依赖关系

**上游依赖（依赖本模块）：**
- `ClientWorld`：提交网格构建请求、接收构建结果回调。

**下游依赖（本模块依赖）：**
- `src/client/renderer/trident/chunk/ChunkMesher.*`：执行实际的网格构建。
- `src/common/world/chunk/ChunkData.hpp`：区块数据结构。
- `src/client/renderer/MeshTypes.*`：网格类型定义。
- `spdlog`：日志。
- `glm`：视锥判定中的矩阵/向量。
- `perfetto`：性能追踪。

## 容易踩的坑

- 不要把策略逻辑塞回 `MeshWorkerPool`。执行器应保持"提交即执行"的简单职责。
- 调度器提交同区块新任务时，必须取消旧任务并更新"最新 taskId"，否则会出现旧结果回写。
- `MeshSchedulerViewState` 必须每帧更新后再 `tick()`，否则视锥优先和背后取消会滞后。
- `ChunkMesher` 取消信号要传透到深循环，仅在外层判断无法及时止损。
