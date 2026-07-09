# Spawn Module

世界生成时的生物放置系统，负责在区块首次生成时放置被动动物。

## 目录结构

```
spawn/
├── WorldGenSpawner.hpp      # 区块生成生物放置器
├── WorldGenSpawner.cpp      # 实现
└── README.md                # 本文件
```

## 内部模块关系

本目录只有一个核心类 `WorldGenSpawner`，无内部子模块。

## 上下游外部依赖关系

### 上游依赖

| 模块 | 用途 |
|------|------|
| `entity/core/EntitySpawnPlacementRegistry.hpp` | 位置验证、SpawnReason 枚举、PlacementType 枚举 |
| `entity/core/EntityRegistry.hpp` | 实体类型查询 |
| `entity/core/EntityClassification.hpp` | 实体分类（Creature、Monster 等） |
| `world/biome/Biome.hpp` | 生物群系生成配置（MobSpawnInfo） |
| `world/spawn/MobSpawnInfo.hpp` | 生成条目定义（SpawnEntry、SpawnCosts） |
| `world/gen/chunk/IChunkGenerator.hpp` | 区块生成器接口 |
| `world/block/BlockRegistry.hpp` | 方块状态检查 |

### 下游依赖

| 模块 | 用途 |
|------|------|
| `world/gen/chunk/NoiseChunkGenerator` | 区块生成时调用 `spawnInitialMobs` |
| `server/world/ServerWorld` | 接收 `SpawnedEntityData` 列表并创建实体 |

## 容易踩的坑

### 1. WorldGenSpawner 只生成被动动物

仅处理 `EntityClassification::Creature` 分类（猪、牛、羊等）。怪物通过 `NaturalSpawner` 在夜间/黑暗环境生成，水生生物有单独的生成逻辑。

### 2. WorldGenRegion 不是 ISpawnWorldReader

`WorldGenRegion` 不直接实现 `ISpawnWorldReader` 接口。需要使用 `WorldGenRegionAdapter` 适配器来调用 `EntitySpawnPlacementRegistry::canSpawnEntity()` 等方法。

### 3. 生成位置必须在区块边界内

`_spawnGroup` 中使用 `std::clamp` 确保实体生成位置不会超出区块边界，考虑实体宽度：

```cpp
spawnX = std::clamp(spawnX, chunkStartX + width, chunkStartX + CHUNK_WIDTH - width);
```

### 4. 生物群系生成概率控制生成次数

`biome.spawnInfo().getCreatureSpawnProbability()` 返回值决定生成循环次数。每次循环有该概率尝试生成一组动物，平原等生物群系概率较高。该值是动物生成概率的**唯一数据来源**（对应原版 `MobSpawnSettings.getCreatureProbability()`），由 `BiomeLoader` 从数据包顶层 `creature_spawn_probability` 字段解析，缺省时回退到 BiomeFactory 工厂方法的设定（如 plains=0.1、snowy_plains=0.07）。`Biome::creatureSpawnProbability()` 仅为便捷代理，直接读取 `spawnInfo` 同一字段，WorldGenSpawner 与 NaturalSpawner 共用同一来源。

### 5. 马和驴对地形平坦度有特殊要求

`_checkSpawnRules` 中对马和驴额外检查周围 3x3 区域的高度差不超过 1 格，否则拒绝生成。

### 6. 必须检查 canSummon()

在 `_spawnGroup` 中首先检查 `entityType.canSummon()`，某些实体类型（如末影龙）不能通过生成器生成。

### 7. 高度图类型因实体而异

不同实体使用不同的高度图类型（通过 `EntitySpawnPlacementRegistry::getHeightmapType` 获取）。例如，飞行生物可能使用不同的高度图。

### 8. 与 NaturalSpawner 的区别

| 特性 | WorldGenSpawner | NaturalSpawner |
|------|-----------------|----------------|
| 触发时机 | 区块首次生成 | 运行时定期 tick |
| 生成分类 | 仅 Creature | Monster、Creature、Ambient 等 |
| 生成条件 | 不检查光照、玩家距离 | 需检查光照、玩家距离等 |
| 数据流 | 输出 SpawnedEntityData | 直接创建实体 |
| 实例级检查 | 不适用（无实体实例），延迟到 ServerWorld | 在 _trySpawnAt 中调用 canSpawnAt |

注意：WorldGenSpawner 输出的 `SpawnedEntityData` 不包含实体实例，无法执行实例级生成检查（如 `CreatureEntity::canSpawnAt`）。
该检查延迟到 `ServerWorld::spawnEntitiesFromChunkGeneration` 创建实体实例后执行，对应 MC `spawnMobsForChunkGeneration` 中的 `mob.checkSpawnRules` 调用。
