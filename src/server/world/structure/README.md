# Structure 模块 - 结构定位算法

承接原 `ServerWorld::findNearestStructure` / `findNearestMapStructure` 的网格搜索算法。MC Java 中结构定位属 `ChunkGenerator` 职责，与世界状态容器无关，故从 `ServerWorld` 下沉至此独立门面。

## 目录结构

```
structure/
├── StructureLocator.hpp   # 结构定位门面（无状态静态类）声明
└── StructureLocator.cpp   # 网格搜索算法实现（RandomSpread / ConcentricRings 两路分支）
```

## 内部模块关系

```
StructureLocator（无状态静态类，接收 const ServerWorld&）
    ├── findNearestStructure(structureId)   # 按结构 ID 查找最近
    │       └── StructureSetRegistry → StructureSet → placement()
    │              ├── RandomSpreadStructurePlacement   # 网格搜索：getPotentialStructureChunk 枚举候选区块
    │              └── ConcentricRingsStructurePlacement # 环形预计算：getRingPositions 直接取全部位置
    │       └── StructureCheck（缓存）── checkStart 过滤不含结构的区块
    │
    └── findNearestMapStructure(tagId)      # 按结构标签查找最近
            └── StructureTags::getTag → 遍历标签内所有 structureId → 逐个调 findNearestStructure 取最近
```

**搜索流程**：以中心方块坐标为原点，将 `maxDistance` 向上取整为区块半径 `chunkRadius`。依据放置策略类型分支：

- **RandomSpread**（如村庄、神庙）：按 `spacing` 划分网格，对每个网格单元用 `getPotentialStructureChunk(seed, baseX, baseZ)` 算出该网格的候选区块，经距离裁剪 + `isStructureChunk`（频率缩减 + 排斥区）校验 + `StructureCheck` 缓存过滤后，取距中心最近者。
- **ConcentricRings**（要塞 Stronghold）：直接取 `getRingPositions(seed)` 预计算的全部环形区块位置，同样经距离裁剪 + 缓存过滤后取最近。

**距离比较**：全程使用距离平方 `distSq` 避免开方，初始阈值 `maxDistance² + 1.0`（`findNearestStructure`）/ `numeric_limits<f64>::max()`（`findNearestMapStructure` 跨结构聚合）。

## 上下游外部依赖关系

### 上游依赖（本模块依赖的外部模块）

| 依赖项 | 说明 |
|-------|------|
| `ServerWorld` | 经 public accessor 取 `seed()`、`chunkManager()`；本身不持引用 |
| `ServerChunkManager` | `generator()` 取 `IChunkGenerator` |
| `IChunkGenerator` | `structureCheck()` 取结构校验缓存（可能为 nullptr） |
| `StructureSetRegistry` | 单例，`findByStructure(structureId)` 查结构所属 StructureSet |
| `StructureSet` | `placement()` 取放置规则 |
| `RandomSpreadStructurePlacement` | 网格放置：`spacing()` / `getPotentialStructureChunk()` |
| `ConcentricRingsStructurePlacement` | 环形放置：`getRingPositions()` |
| `StructurePlacement`（基类） | `isStructureChunk()` / `getLocatePos()` |
| `StructureCheck` | 区块结构存在性缓存：`checkStart()` / `setFeatureCheckResult()` |
| `StructureTags` | `getTag(tagId)` 取标签内结构 ID 列表 |

### 下游依赖（依赖本模块的外部模块）

| 调用方 | 说明 |
|-------|------|
| `ServerWorld` | `findNearestStructure` / `findNearestMapStructure` 虚分发委托本门面（IWorld 接口 override） |
| `LocateCommand` | `/locate` 命令经 `IWorld::findNearestStructure` 虚分发 |
| `ExplorationMapFunction` | 藏宝图生成经 `IWorld::findNearestMapStructure` 虚分发 |
| `DolphinGoals` | 海豚寻宝经 `IWorld::findNearestMapStructure` 虚分发 |

> 所有调用方均经 `IWorld` 虚接口分发，不直接依赖 `StructureLocator`，故门面可独立演进。

## StructureCheck 缓存三层结果

`checkStart(chunkPosId, structureId, skipExisting)` 返回 `StructureCheckResult`：

| 结果 | 含义 | 处理 |
|------|------|------|
| `StartPresent` | 精确缓存命中：结构存在于该区块 | 计入候选，跳过放置规则二次校验 |
| `StartNotPresent` | 精确/近似缓存确认该区块不含目标结构 | 跳过该区块 |
| `ChunkLoadNeeded` | 缓存未命中 | 继续基于放置规则判断，并 `setFeatureCheckResult(chunkPosId, true)` 写入近似缓存供后续查询 |

`ConcentricRings` 分支的 `ChunkLoadNeeded` 结果**不写近似缓存**（与 `RandomSpread` 分支不同），保持与原实现一致。

## 容易踩的坑

### 1. 调用时机须在 StructureCheck 精确缓存填充之后

`StructureCheck` 的精确缓存（`m_loadedChunks`）在区块结构数据加载完成时由 `onStructureLoad` 填充。若在区块尚未加载时调用，`checkStart` 多数返回 `ChunkLoadNeeded`，退化为纯放置规则判断（近似），可能漏报或误报。`/locate` 通常由玩家触发，此时目标区域区块多已加载，影响较小；藏宝图/海豚寻宝在未加载区域可能精度下降。

### 2. worldSeed 类型转换

`world.seed()` 返回 `u64`，算法内 `static_cast<i64>(world.seed())` 转有符号用于放置规则的哈希计算。`getPotentialStructureChunk` / `isStructureChunk` / `getRingPositions` 均接收 `i64` 种子。切勿误传 `u64` 导致重载解析或符号扩展差异。

### 3. chunkRadius 向上取整

`chunkRadius = (maxDistance + 15) >> 4`，`+15` 保证非整区块距离向上取整（如 `maxDistance=100` → `chunkRadius=7` 而非 6）。改此常量会导致搜索范围偏小漏掉近邻结构。

### 4. RandomSpread 网格边界 ±1 余量

网格搜索范围 `(centerChunk ± chunkRadius) / spacing ± 1`，`±1` 是为保证候选区块落在搜索圆边缘的网格单元不被裁掉。删除会导致边界结构漏检。

### 5. findNearestMapStructure 跨结构聚合阈值

`findNearestMapStructure` 用 `numeric_limits<f64>::max()` 作初始阈值（而非 `maxDistance²`），因为它要在标签内**所有**结构的候选中取全局最近，单结构的 `maxDistance²` 阈值已在内层 `findNearestStructure` 应用过。若误改外层阈值会过早裁掉候选。

### 6. nullptr 守卫不可删

`chunkManager()` / `generator()` / `structureCheck()` 均可能返回 nullptr（世界初始化早期或生成器无结构支持）。算法对三者均做了 nullptr 判空后退化为无缓存路径，**非过度防御**，删会导致空指针解引用。

### 7. 命名空间须为 mc::server::structure（非 mc::server::world::structure）

`StructureLocator` 命名空间刻意定为 `mc::server::structure` 而非与目录一致的 `mc::server::world::structure`。原因：`ServerWorld` 位于 `mc::server` 命名空间，其实现文件 `ServerWorld.cpp` 内大量以 `world::xxx` 引用 `mc::world` 命名空间的符号（`world::CHUNK_WIDTH`/`world::tick::TickPriority`/`world::gamerule::GameRuleKeys`/`world::redstone::RedstoneSystem` 等），依赖「`mc::server::world` 不存在故 `world::` 回退解析到 `mc::world`」这一名字查找回退。若把 `StructureLocator` 放进 `mc::server::world::structure`，会令 `mc::server::world` 命名空间首次出现在 `ServerWorld.cpp`，导致所有 `world::xxx` 引用改为优先匹配 `mc::server::world::xxx`（不存在）而编译失败。目录 `server/world/structure/` 与命名空间解耦，与 `WeatherManager`（目录 `server/world/weather/` 但命名空间 `mc::server`）同例。`ServerWorld.cpp` 内委托写 `structure::StructureLocator::...`（`mc::server` 内相对解析到 `mc::server::structure`）。
