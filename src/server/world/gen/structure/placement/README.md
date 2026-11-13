# 结构放置策略 (Structure Placement)

决定结构在世界中的空间分布方式。

## 目录结构树

```text
placement/
├── StructurePlacement.hpp                 # 放置基类、枚举、ExclusionZone
├── StructurePlacement.cpp
├── RandomSpreadStructurePlacement.hpp     # 网格随机分布（大多数结构）
├── RandomSpreadStructurePlacement.cpp
├── ConcentricRingsStructurePlacement.hpp  # 同心环分布（要塞）
└── ConcentricRingsStructurePlacement.cpp
```

## 内部模块关系

```
StructurePlacement (抽象基类)
├── RandomSpreadStructurePlacement     — 网格内随机偏移，适用于村庄、神殿等
└── ConcentricRingsStructurePlacement  — 同心环分布，仅用于要塞
```

- `StructurePlacement` 定义三步检查流程：候选位置计算 → 频率缩减 → 排斥区检查
- `RandomSpreadStructurePlacement` 将世界划分为 spacing×spacing 网格，每格选一个候选区块
- `ConcentricRingsStructurePlacement` 预计算所有要塞位置并缓存，按环查询

## 上下游外部依赖关系

### 上游依赖（本模块依赖）

| 模块 | 用途 |
|------|------|
| `util/math/random/Random.hpp` | 随机数生成（setLargeFeatureWithSalt 等） |
| `util/math/MathUtils.hpp` | floorDiv 地板除运算 |
| `util/math/MathConstants.hpp` | PI_DOUBLE 常量 |
| `world/chunk/base/ChunkPos.hpp` | 区块坐标类型 |
| `resource/ResourceLocation.hpp` | 排斥区引用的 StructureSet ID |

### 下游依赖（依赖本模块）

| 模块 | 用途 |
|------|------|
| `world/gen/structure/StructureManager` | 创建和管理放置策略实例 |
| `world/gen/chunk/NoiseChunkGenerator` | 区块生成时调用 isStructureChunk 判断结构位置 |

## 容易踩的坑

### 1. spacing 必须大于 separation

RandomSpreadStructurePlacement 构造时 spacing <= separation 会导致 offsetRange <= 0，nextInt 崩溃。已有 MC_ASSERT_RELEASE 断言保护。

### 2. ConcentricRings 有两张表，别混用

`ringLayoutChunks()` 只含环布局起点（每个要塞搜索方形的中心），供 /locate 用；`resolveRingPositions()` 才是经半径 112 群系搜索改写后的最终坐标，供区块生成判定用。两者最多相差 7 个区块，混用会让要塞位置判错。两张表各自按世界种子缓存，共用同一把 `m_cacheMutex`。

### 3. 同心环解析必须单飞，且不能退化成逐区块重算

128 次群系搜索是秒级开销，而 `StructurePlacementState` 每个区块都会新建一个实例，所以缓存只能挂在放置规则上：`resolveRingPositions` 在持锁状态下计算，并发到达的判定线程排队等结果。若改成"每个实例各算一次"，729 个区块就是 729 次全量搜索；若把计算挪到锁外做双检，则并发线程会各跑一遍。搜索内部按要塞并行（`util::parallelFor`），因为每个要塞只用自己那条 fork 出来的随机流，对群系源只读。

### 4. FrequencyReductionMethod 对应特定结构

- Default: 大多数结构
- LegacyType1: 掠夺者前哨站
- LegacyType2: 埋藏宝藏（固定盐值 10387320）
- LegacyType3: 废弃矿井

选错方法会导致结构位置与原版不一致。

### 5. 排斥区不在本模块判定

`ExclusionZone` 要求「另一个结构集在附近没有落点」，本模块看不到整张结构集表，只保管配置（`exclusionZone()`）；真正的检查在 `StructurePlacementState::passesExclusionZone`。所以 `RandomSpreadStructurePlacement::isStructureChunk` 只做候选位置 + 频率缩减，不要在这里补排斥区逻辑。
