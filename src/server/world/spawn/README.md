# Spawn 模块 - 实体自然生成系统

## 目录结构

```
spawn/
├── NaturalSpawner.hpp       # 自然生成器 - 管理实体生成循环和密度控制
├── NaturalSpawner.cpp       # 自然生成器实现
├── DespawnManager.hpp       # 生物消失管理器 - 防止实体无限累积
├── DespawnManager.cpp       # 生物消失管理器实现
├── VillageSiege.hpp         # 村庄围攻系统（僵尸围村）
├── VillageSiege.cpp         # 村庄围攻系统实现
├── SpawnConditions.hpp      # 生成条件检查工具函数
└── SpawnConditions.cpp      # 生成条件检查实现
```

## 内部模块关系

```
NaturalSpawner（主生成器）
    ├── EntityDensityManager（密度管理器）── 全局 cap：count < maxInstancesPerChunk * spawnableChunkCount / 289
    │       └── MobDensityTracker（密度追踪器）── SpawnCosts 系统核心，每 tick 清空重建
    ├── LocalMobCapCalculator（本地 cap）── 每玩家每分类跨区块共享，防单区域堆积
    └── SpawnConditions（条件检查）── 光照、碰撞、位置检查

VillageSiege（村庄围攻）
    └── SpawnConditions（条件检查）── 复用生成位置验证

DespawnManager（消失管理）
    └── 独立运行，无内部依赖
```

**生成流程**：每 tick，NaturalSpawner 检查玩家周围区块，根据生物群系配置、光照条件、实体密度限制决定是否生成实体。VillageSiege 在午夜时刻有概率触发僵尸围村。DespawnManager 在每 tick 后检查实体消失条件。

**finalizeSpawn 调用**：所有 MobEntity 生成路径在 `spawnEntity()` 前必须调用 `finalizeSpawn(world, difficulty, spawnReason)`，以完成基于难度的初始化（拾取物品能力、默认装备、附魔等）。NaturalSpawner 使用 `SpawnReason::Natural`，VillageSiege 使用 `SpawnReason::Event`。详见 `entity/core/README.md`。

**canSpawnAt 实例级检查**：NaturalSpawner 在创建实体并设置位置后、`finalizeSpawn` 之前，对 CreatureEntity 调用 `canSpawnAt(x, y, z)` 进行实例级生成规则检查（对应 MC `PathfinderMob.checkSpawnRules`，即 `getWalkTargetValue >= 0`）。这确保了动物不会在黑暗处生成、怪物不会在明亮处生成等寻路权重约束。详见 `CreatureEntity::canSpawnAt` 和 `CreatureEntity::getPathWeight`。

## 上下游外部依赖关系

### 上游依赖（本模块依赖的外部模块）

| 依赖项 | 说明 |
|-------|------|
| `ServerWorld` | 服务端世界实例，获取区块、方块状态、光照 |
| `MobSpawnInfo` | 生物群系配置的生成信息，包含可生成实体类型 |
| `EntitySpawnPlacementRegistry` | 实体放置规则（地面、水中、岩浆等） |
| `EntityRegistry` | 实体类型注册表 |
| `BiomeRegistry` | 生物群系注册表，获取生成配置 |
| `VillageManager` | 村庄管理器，判断玩家是否在村庄内 |
| `common/world/spawn/MobSpawnInfo.hpp` | 生成配置数据结构 |
| `common/util/math/random/Random.hpp` | 随机数生成器 |

### 下游依赖（依赖本模块的外部模块）

| 调用方 | 说明 |
|-------|------|
| `ServerWorld` | 在 tick 循环中调用 NaturalSpawner、DespawnManager、VillageSiege |
| 区块生成系统 | 区块首次生成时调用 `spawnInChunk()` 放置被动动物 |

## 实体数量限制常量

每分类的全局 cap 公式：`count < maxInstancesPerChunk * spawnableChunkCount / MAGIC_NUMBER(289)`，无 `max(...,1)` 下限保护。`spawnableChunkCount` 为玩家固定刷怪距离（SPAWN_DISTANCE_CHUNK=8）内已加载区块数（满载≈289），由 `_countSpawnableChunks` 去重统计（对应原版 `DistanceManager.getNaturalSpawnChunkCount`）。持久化生物（`isNoDespawnRequired`/`preventDespawn`）不计入 cap 计数（对应原版 `createState` 跳过 `isPersistenceRequired`）。

| 分类 | 最大实例数/区块 | isPersistent | despawnDistance |
|------|-----------|--------------|-----------------|
| Monster（怪物） | 70 | false | 128 |
| Creature（动物） | 10 | true（每 400tick 节流） | 128 |
| Ambient（环境生物） | 15 | false | 128 |
| Axolotls | 5 | false | 128 |
| UndergroundWaterCreature | 5 | false | 128 |
| WaterCreature（鱿鱼/海豚/鹦鹉螺） | 5 | false | 128 |
| WaterAmbient（鳕鱼/鲑鱼/河豚/热带鱼） | 20 | false | 64 |
| Misc | -1 | true | 128 |

> **分类对齐要点**：cod/salmon/pufferfish/tropical_fish 属 WaterAmbient（非 WaterCreature），squid/dolphin/nautilus 属 WaterCreature，glow_squid 属 UndergroundWaterCreature。分类错配会导致该分类真实计数永远为 0、cap 永久失效、无限累积。

## 生成循环结构

每 tick：全局预过滤出仍可生成的分类列表（friendly/persistent 过滤 + 全局 cap + 持久化分类 400tick 节流）→ `_getSpawnableChunks` 收集玩家 8 区块内已加载区块并随机打乱（无 1/17 概率丢弃）→ 逐块逐分类：全局 cap 复检 + 本地 cap（`LocalMobCapCalculator`）复检 + `_spawnForClassificationInChunk` 外层 3 轮尝试（群体规模 `ceil(random*4)`），生成成功后 `addMob` 占用本地配额。

## 生成规则速查

| 分类 | 光照条件 | 距离要求 | 特殊条件 |
|------|---------|---------|---------|
| 怪物 | 光照 ≤ 7 | 24-128 格 | 黑暗环境 |
| 动物 | 光照 > 7 | 24-128 格 | 每 400 tick 尝试一次 |
| 环境生物 | 光照 ≤ 7 | 24-128 格 | 随机概率 |
| 水生生物 | 在水中 | 24-128 格 | 需要水域 |

## DespawnManager 消失规则

每 tick 遍历**所有**生物实体（无每 tick 数量上限），对应原版 `Mob.checkDespawn` 每实体每 tick 检查。纯函数 `shouldDespawn(mob, closestPlayerDistSq, difficulty, currentTick, random)` 便于覆盖边界，无玩家时用 `kNoPlayer(-1.0)` 哨兵表示（对应原版 `getNearestPlayer` 返回 null 时保留实体）。

| 条件 | 行为 |
|------|------|
| 距离玩家 > despawnDistance（128，WaterAmbient=64） | 立即消失（需 `canDespawn`） |
| 距离玩家 > 32 格且空闲 > 600 tick | 1/800 概率消失（需 `canDespawn`） |
| 距离玩家 < 32 格 | 重置空闲时间 noActionTime=0 |
| 和平难度下的怪物（isDespawnPeaceful） | 立即消失 |
| 持久化（isNoDespawnRequired / preventDespawn） | 重置空闲时间，永不消失 |
| 无玩家（kNoPlayer） | 保留实体（不做任何事） |

## VillageSiege 触发条件

| 条件 | 说明 |
|------|------|
| 夜晚 | 游戏时间 12000-24000 tick |
| 午夜时刻 | dayTime == 18000 tick |
| 玩家在村庄内 | 非旁观者玩家位于有效村庄 |
| 生物群系非蘑菇岛 | 蘑菇岛是安全区域 |
| 10% 概率 | 每晚午夜触发 |

## 与 MC 1.16.5 的对应关系

| 本项目类 | MC 1.16.5 对应类 |
|---------|-----------------|
| `NaturalSpawner` | `WorldEntitySpawner.NaturalSpawner` |
| `EntityDensityManager` | `WorldEntitySpawner.EntityDensityManager` |
| `MobDensityTracker` | `WorldEntitySpawner.MobDensityTracker` |
| `VillageSiege` | `VillageSiege` |
| `SpawnConditions` | `EntitySpawnPlacementRegistry` (部分) |

## 容易踩的坑

### 1. 实体数量限制不生效

**问题**：`EntityDensityManager::canSpawn()` 返回 true，但实际不应生成。

**原因**：实体计数未正确传递给 `EntityDensityManager`。

**解决**：确保从 `EntityManager` 正确获取实体分类计数，而非传递空的 map。

### 2. 密度追踪器内存持续增长

**问题**：`MobDensityTracker` 不断增长，内存占用持续增加。

**原因**：未定期清理密度数据。

**解决**：在合适时机（如每 tick 结束或玩家离开时）调用 `clear()`。

### 3. 区块生成时 SpawnInfo 获取错误

**问题**：`spawnInChunk()` 生成的实体类型不正确。

**原因**：硬编码了生物群系的 `MobSpawnInfo`。

**解决**：从区块的生物群系容器获取正确的 `MobSpawnInfo`。

### 4. 光照检查时机

**问题**：怪物在光照充足的区域生成。

**原因**：光照数据尚未更新。

**解决**：确保在光照计算完成后进行生成检查。

### 5. 实体放置类型不匹配

**问题**：水生生物在陆地上生成。

**原因**：未正确设置实体的放置类型。

**解决**：在 `EntitySpawnPlacementRegistry` 中正确注册放置类型。

### 6. 生成距离计算错误

**问题**：实体在玩家视野外生成。

**原因**：距离检查使用曼哈顿距离而非欧几里得距离。

**解决**：使用距离平方计算（`distanceSq = dx*dx + dy*dy + dz*dz`），常量 `MIN_SPAWN_DISTANCE_SQ = 24*24`，`MAX_SPAWN_DISTANCE_SQ = 128*128`。

### 7. SpawnCosts 判断失效

**问题**：`isValid()` 返回 false 导致密度检查被跳过。

**原因**：`SpawnCosts` 默认构造的 `energyBudget` 和 `charge` 都是 0。

**解决**：只在需要时创建有效的 `SpawnCosts`，如 `SpawnCosts costs(1.0, 0.7)`。

### 8. CHUNK_HEIGHT 与 MAX_BUILD_HEIGHT 混淆

**问题**：使用 `CHUNK_HEIGHT` 作为世界高度限制。

**原因**：两者当前值相同但语义不同。`CHUNK_HEIGHT` 是区块高度范围，`MAX_BUILD_HEIGHT` 是玩家可到达的最大高度。

**解决**：未来 `MIN_BUILD_HEIGHT` 可能向下拓展为 -64，届时 `CHUNK_HEIGHT` 将不等于 `MAX_BUILD_HEIGHT`。务必使用正确的常量。

### 9. 蘑菇岛检测

蘑菇岛是安全区域，不会发生僵尸围攻。`isMushroomBiome()` 通过生物群系 ID 检查：
1. 直接比较 `Biomes::MushroomFields` 和 `Biomes::MushroomFieldShore`
