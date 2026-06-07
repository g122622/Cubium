# Chunk 模块

本目录包含 Minecraft 区块系统的核心实现，参考 Minecraft Java Edition 1.16.5 的架构设计。

## 目录结构

```
src/common/world/chunk/
├── ChunkData.hpp/cpp              # 区块数据存储（ChunkSection、ChunkData、ChunkDataRef）
├── ChunkDistanceGraph.hpp/cpp     # 区块距离图（BFS 级别传播算法）
├── ChunkId.hpp                    # 区块唯一标识符（包含维度）
├── ChunkLoadTicket.hpp            # 显式 ticket 类型与集合定义
├── ChunkLoadTicketManager.hpp/cpp # 票据管理器与玩家来源聚合器
├── ChunkPos.hpp                   # 区块位置类型
├── ChunkPrimer.hpp/cpp            # 区块生成中间状态
├── ChunkStatus.hpp/cpp            # 区块生成阶段定义
├── IChunk.hpp/cpp                 # 区块接口和基础类型
├── SectionPos.hpp                 # 区块段位置类型
└── SingleChunkLifecycleManager.hpp/cpp  # 单区块生命周期管理
```

## 模块关系

```mermaid
flowchart LR
    A[显式ticket变化/玩家source变化] --> B[ChunkLoadTicketManager]
    B --> C[ServerChunkManager]
    C --> D[SingleChunkLifecycleManager]
    D --> E[ServerWorkerPool]
    E --> F[ChunkPrimer]
    F --> G[ChunkData]

    style A fill:#e3f2fd,stroke:#1e88e5,color:#0d47a1
    style B fill:#f3e5f5,stroke:#8e24aa,color:#4a148c
    style C fill:#e8f5e9,stroke:#43a047,color:#1b5e20
    style D fill:#fff3e0,stroke:#fb8c00,color:#e65100
    style E fill:#ede7f6,stroke:#5e35b1,color:#311b92
    style F fill:#e0f7fa,stroke:#00acc1,color:#006064
    style G fill:#fce4ec,stroke:#d81b60,color:#880e4f
```

## 内部模块关系

- **ChunkPos / SectionPos / ChunkId**：位置和标识类型，无依赖
- **ChunkStatus**：生成阶段定义，无依赖
- **IChunk**：区块接口，依赖 BlockState、BiomeContainer、Heightmap
- **ChunkData**：完整区块数据（实现 IChunk），依赖 ChunkSection、NibbleArray、BlockEntity
- **ChunkPrimer**：生成中间状态，依赖 ChunkData、ChunkStatus、Heightmap、StructureStart
- **ChunkLoadTicket**：显式 ticket 类型定义，无依赖
- **ChunkDistanceGraph**：BFS 级别传播算法，无依赖
- **ChunkLoadTicketManager**：聚合显式 ticket 和玩家 source，依赖 ChunkLoadTicket、ChunkDistanceGraph
- **SingleChunkLifecycleManager**：单区块生命周期管理，依赖 ChunkPrimer、ChunkData、ChunkStatus

## 上下游依赖关系

**被依赖方（上游）**：
- `common/core/Types.hpp` - 基础类型
- `common/core/Result.hpp` - 错误处理
- `common/world/block/Block.hpp` - 方块状态
- `common/world/biome/Biome.hpp` - 生物群系
- `common/world/WorldConstants.hpp` - 世界常量
- `common/util/NibbleArray.hpp` - 4 位数组
- `common/util/math/MathUtils.hpp` - 数学工具

**依赖方（下游）**：
- `server/world/ServerChunkManager.hpp` - 服务端区块管理器
- `server/world/chunk/ChunkGenerateTask.hpp` - 区块生成任务
- `client/world/ClientChunkManager.hpp` - 客户端区块管理器
- `world/World.hpp` - 世界类

## 容易踩的坑

1. **忘记调用 processUpdates()**
   - 显式 ticket、玩家 source 和追踪系统的更新是批处理的
   - 必须调用 `processUpdates()` 才能处理更新队列

2. **线程安全问题**
   - `SingleChunkLifecycleManager` 使用互斥锁保护
   - `ChunkLoadTicketManager` 的追踪玩家映射有单独的锁
   - `ChunkData` 和 `ChunkPrimer` **不是线程安全的**

3. **区块段懒创建**
   - 设置空气方块不会创建区块段
   - 读取未创建段返回空气/安全默认值
   - 写入非空气方块才会创建段

4. **光照初始化**
   - 天空光照默认为 15（全亮）
   - 方块光照默认为 0（无光）
   - 需要显式调用 `initializeSkyLight()` 和 `initializeBlockLight()`

5. **level 语义理解**
   - 级别越小优先级越高
   - 级别 <= 33 的区块应该被加载
   - 显式 ticket 和玩家 source 最终都会收敛到同一个 level 体系
   - 玩家来源中心级别 = `33 - 视距`

6. **ChunkStatus 比较**
   - 使用 `isAtLeast()` 和 `isBefore()` 进行状态比较
   - 不要直接比较 ordinal 值

7. **Future 链**
   - 每个 ChunkStatus 对应一个 Future
   - 需要正确处理 Future 完成和错误

8. **请求已取消但仍有回调**
   - 当前实现会保留回调入口，但会将结果标记为失败
   - 不要假设回调一定表示区块可用，必须检查 `success`

9. **高度图内部存储**
   - `Heightmap` 内部存储的是 `y + 1`，不是实际方块 Y
   - 只有 `getTopBlockY()` 这一层才应该把它转换回块坐标
   - 不要直接把原始高度图值当作方块位置

10. **ChunkData 空隙图设置**
    - `setSkyEmptinessMap()` / `setBlockEmptinessMap()` 需要拿到完整的区块段空隙图
    - 如果上游拿到的是按段更新结果，必须先拷贝成连续的 `bool[]` 再写回区块

11. **CHUNK_HEIGHT 与 MAX_BUILD_HEIGHT 的区别**
    - `CHUNK_HEIGHT = MAX_BUILD_HEIGHT - MIN_BUILD_HEIGHT`
    - 未来可能 MIN_BUILD_HEIGHT 会向下拓展成 -64
    - 届时 CHUNK_HEIGHT 就不等于 MAX_BUILD_HEIGHT 了
