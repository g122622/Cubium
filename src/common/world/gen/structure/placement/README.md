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

### 2. ConcentricRings 缓存线程安全

getRingPositions 使用双重检查锁和 mutex 保护缓存生成。首次调用成本较高（128 个要塞的位置计算），后续调用 O(1) 返回缓存。

### 3. 排斥区回调需外部注入

ExclusionZone 的检查回调通过 setExclusionZoneChecker 注入，未设置时排斥区检查默认通过。使用前必须确保回调已设置。

### 4. FrequencyReductionMethod 对应特定结构

- Default: 大多数结构
- LegacyType1: 掠夺者前哨站
- LegacyType2: 埋藏宝藏（固定盐值 10387320）
- LegacyType3: 废弃矿井

选错方法会导致结构位置与原版不一致。
