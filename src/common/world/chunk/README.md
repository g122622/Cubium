# Chunk 模块

本目录包含 Minecraft 区块系统的核心实现。

## 目录结构

```
src/common/world/chunk/
├── base/
│   ├── ChunkId.hpp                    # 区块唯一标识符（包含维度）
│   ├── ChunkPos.hpp                   # 区块位置类型
│   └── SectionPos.hpp                 # 区块段位置类型
├── data/
│   ├── BiomeContainer.hpp/cpp        # 生物群系容器
│   ├── ChunkData.hpp/cpp             # 完整区块数据（实现 IChunk）
│   ├── ChunkPrimer.hpp/cpp           # 区块生成中间状态
│   ├── ChunkSection.hpp/cpp          # 区块段（16x16x16 方块）
│   ├── Heightmap.hpp/cpp             # 高度图
│   └── IChunk.hpp/cpp                # 区块接口和基础类型
├── gen/
│   ├── ChunkDependencies.hpp/cpp     # 区块依赖关系
│   ├── ChunkPyramid.hpp/cpp          # 区块生成金字塔（含 ChunkLevel 合并方法）
│   ├── ChunkStatus.hpp/cpp           # 区块生成阶段定义
│   └── ChunkStep.hpp                 # 区块生成步骤
├── load/
│   ├── ChunkDistanceGraph.hpp/cpp     # 区块距离图（BFS 级别传播算法）
│   ├── ChunkLoadLevel.hpp            # 区块加载级别枚举与工具函数
│   ├── ChunkLoadTicket.hpp           # 显式 ticket 类型与集合定义
│   └── ChunkLoadTicketManager.hpp/cpp # 票据管理器与玩家来源聚合器
└── README.md
```

**已迁移到 server/**：
- `SingleChunkLifecycleManager.hpp/cpp` → `src/server/world/`

**已合并**：
- `ChunkLevel.hpp/cpp` → 方法合并到 `ChunkPyramid`，常量移到 `ChunkLoadLevel`

## 模块关系

```mermaid
flowchart LR
    A[显式ticket变化/玩家source变化] --> B[ChunkLoadTicketManager]
    B --> C[ServerChunkManager]
    C --> D[SingleChunkLifecycleManager]
    D --> E[UniversalWorkerPool]
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

## 子目录职责

### base/ — 位置与标识类型

无外部依赖的纯值类型：
- **ChunkPos** — 区块坐标（x, z）
- **SectionPos** — 区块段坐标（x, y, z）
- **ChunkId** — 区块唯一标识（x, z, dimension）

### data/ — 数据模型

区块数据存储与访问：
- **IChunk** — 区块接口（定义 HeightmapType、ChunkLoadStatus 等）
- **ChunkSection** — 16x16x16 方块段（状态ID存储、光照、随机刻计数）
- **ChunkData** — 完整区块数据（实现 IChunk，包含段数组、高度图、方块实体等）
- **ChunkPrimer** — 生成中间状态（ProtoChunk）
- **BiomeContainer** — 生物群系采样容器
- **Heightmap** — 高度图（6 种类型）

### gen/ — 生成管线

区块生成阶段与依赖：
- **ChunkStatus** — 12 阶段生成链（EMPTY → FULL）
- **ChunkDependencies** — 按半径索引的依赖关系
- **ChunkStep** — 目标状态 + 直接依赖 + 累积依赖 + 可写半径 + byRadius[] 查找表（`getRequiredStatusAtRadius`/`neighbourReadRadius`，对齐 Moonrise ChunkStepMixin）
- **ChunkPyramid** — 生成金字塔（含 `radiusAroundFullChunk()`、`maxLevel()`、`generationStatus()` 等方法）

### load/ — 加载票据系统

区块加载优先级管理：
- **ChunkLoadLevel** — 加载级别枚举 + `shouldChunkLoad()`/`viewDistanceToLevel()` + `FULL_CHUNK_LEVEL`/`BLOCK_TICKING_LEVEL`/`ENTITY_TICKING_LEVEL` 常量
- **ChunkLoadTicket** — 票据类型与集合
- **ChunkDistanceGraph** — BFS 级别传播算法
- **ChunkLoadTicketManager** — 聚合显式 ticket 和玩家 source

## 生成依赖模型

使用 `ChunkPyramid::generationPyramid()` 定义区块生成阶段的依赖关系：

| 阶段 | 直接依赖 | 可写半径 |
|------|---------|---------|
| EMPTY | — | -1 |
| STRUCTURE_STARTS | [EMPTY] | -1 |
| STRUCTURE_REFERENCES | [STRUCTURE_STARTS(8)] | -1 |
| BIOMES | [STRUCTURE_STARTS(8)] | -1 |
| NOISE | [STRUCTURE_STARTS(8), BIOMES(1)] | 0 |
| SURFACE | [STRUCTURE_STARTS(8), BIOMES(1)] | 0 |
| CARVERS | [STRUCTURE_STARTS(8)] | 0 |
| FEATURES | [STRUCTURE_STARTS(8), CARVERS(1)] | 1 |
| INITIALIZE_LIGHT | [FEATURES] | -1 |
| LIGHT | [INITIALIZE_LIGHT(1)] | -1 |
| SPAWN | [BIOMES(1)] | -1 |
| FULL | [SPAWN] | -1 |

## 上下游依赖关系

**被依赖方（上游）**：
- `common/core/Types.hpp` — 基础类型
- `common/core/Result.hpp` — 错误处理
- `common/world/block/Block.hpp` — 方块状态
- `common/world/biome/Biome.hpp` — 生物群系
- `common/world/WorldConstants.hpp` — 世界常量
- `common/util/NibbleArray.hpp` — 4 位数组

**依赖方（下游）**：
- `server/world/ServerChunkManager.hpp` — 服务端区块管理器
- `server/world/SingleChunkLifecycleManager.hpp` — 单区块生命周期（NewChunkHolder 等价物）
- `server/world/ChunkTaskScheduler.hpp` — 生成调度核心
- `server/world/ChunkProgressionTask.hpp` — 单状态推进任务
- `client/world/ClientWorld.hpp` — 客户端世界
- `common/world/gen/` — 生成器
- `common/world/lighting/` — 光照引擎
- `common/world/storage/` — 存储系统

## 命名空间

所有类型统一在 `mc::world::chunk` 命名空间下。

## 容易踩的坑

1. **忘记调用 processUpdates()** — 显式 ticket、玩家 source 和追踪系统的更新是批处理的
2. **线程安全** — `SingleChunkLifecycleManager` 使用互斥锁；`ChunkData` 和 `ChunkPrimer` 不是线程安全的
3. **区块段懒创建** — 设置空气方块不会创建区块段；写入非空气方块才会创建
4. **光照初始化** — 天空光照默认 15（全亮），方块光照默认 0（无光）
5. **level 语义** — 级别越小优先级越高；级别 ≤ 33 的区块应该被加载
6. **ChunkStatus 比较** — 使用 `isAtLeast()` 和 `isBefore()`，不要直接比较 ordinal
7. **高度图内部存储** — `Heightmap` 内部存储 `y + 1`，不是实际方块 Y。`getTopBlockY` 返回方块本身 Y（内部值 -1），但**空列回退为 `MIN_BUILD_HEIGHT`**（与"minY 处有方块"无法区分）；需精确识别空列的调用方（如 `HeightmapPlacement`）改用 `getHeightmapFirstAvailable` 拿原始值（`y+1` 或 `NO_BLOCK_SENTINEL`），对齐 MC `Heightmap.getFirstAvailable`
8. **WorldGenRegion 阶段校验** — 生成期区块访问按当前 `ChunkStep::directDependencies()` 校验距离对应的允许状态；调用点需要传递实际请求的 `ChunkStatus`
9. **ChunkStatus 根状态自引用 parent** — `ChunkStatus` 构造函数将 `nullptr` parent 转为 `this`，因此 `EMPTY.parent() == &EMPTY`（非 nullptr）。遍历 parent 链时必须用 `*status != EMPTY` 而非 `status != nullptr` 作为终止条件（见 `ChunkStep::buildRequiredStatusByRadius`）
10. **getRadiusOf 返回最后覆盖半径** — `ChunkDependencies::getRadiusOf(status)` 返回覆盖该 status ordinal 的**最大**半径 j（前向循环覆盖），不是最小半径。例如 FULL 的累积依赖中 `getRadiusOf(STRUCTURE_STARTS) = 11`（外圈），而非 4（首次出现）。`byRadius[]` 构建依赖此语义：高状态先填小半径，低状态填剩余空隙，最终 `byRadius[r] == accumulatedDependencies.get(r)`
11. **byRadius[] 查找表** — `ChunkStep::getRequiredStatusAtRadius(radius)` 返回生成 `targetStatus` 时距离中心 `radius` 的邻居必须达到的状态。`byRadius[0] = accumulatedDependencies.get(0)`（中心区块前一步状态），表在 `ChunkPyramid` 构建时由 `buildRequiredStatusByRadius` 填充。与 Moonrise `ChunkStepMixin.moonrise$getRequiredStatusAtRadius` 对齐
