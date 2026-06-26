# 刷怪笼方块实体（Spawner Block Entity）

刷怪笼（Mob Spawner）方块实体，自动在附近区域周期性生成实体。

## 目录结构

```
spawner/
├── MobSpawnerBlockEntity.hpp  # 刷怪笼方块实体定义
├── MobSpawnerBlockEntity.cpp  # 刷怪笼方块实体实现
└── README.md
```

## 内部模块关系

```
BlockEntity（基类）
    ↑
MobSpawnerBlockEntity
    - SpawnEntry（生成条目：实体ID + 权重）
    - CustomSpawnRules（自定义光照限制）
```

## 上下游外部依赖关系

### 上游依赖（谁使用了这个模块）

- `world/block/blocks/mob/SpawnerBlock` - 刷怪笼方块，创建 MobSpawnerBlockEntity
- `world/gen/structure/structures/StrongholdPieces` - 要塞传送门房间放置蠹虫刷怪笼后配置实体类型
- `world/blockentity/core/BlockEntityRegistry` - 注册 MobSpawnerBlockEntity 工厂
- `world/chunk/` - 区块加载时反序列化刷怪笼方块实体

### 下游依赖（这个模块依赖了谁）

- `world/blockentity/BlockEntity.hpp` - 方块实体基类
- `entity/core/EntityRegistry.hpp` - 实体类型注册表（通过实体ID查找EntityType）
- `entity/core/EntityType.hpp` - 实体类型（创建实体实例、查询分类）
- `entity/core/EntityClassification.hpp` - 实体分类（判断是否为和平生物）
- `entity/core/EntitySpawnPlacementRegistry.hpp` - 生成放置规则注册表（默认生成条件检查）
- `entity/combat/DifficultyHelper.hpp` - 难度辅助（和平模式下禁止非和平生物生成）
- `entity/combat/DifficultyInstance.hpp` - 难度实例（finalizeSpawn 需要）
- `world/IWorld.hpp` - 世界接口（生成实体、查询附近实体、光照查询）
- `world/chunk/data/ChunkData.hpp` - 区块数据（生成规则适配器中查询高度图和生物群系）
- `world/biome/Biomes.hpp` - 生物群系常量（默认生物群系回退值）
- `world/lighting/InternalLightUtils.hpp` - 光照工具（天空变暗计算）

## 容易踩的坑

### 1. setEntityId 会自动添加默认生成候选

调用 `setEntityId()` 时，如果 `m_spawnPotentials` 为空，会自动添加一个权重为 1 的条目。这是为了让只设置单一实体类型的刷怪笼（如要塞蠹虫刷怪笼）无需手动添加候选列表。

### 2. NBT 格式兼容 MC Java 1.21+

`loadFromNBT` 支持 MC Java 1.21 的 SpawnData 格式（`{entity: {id: "minecraft:silverfish"}}`）以及旧版格式（`{id: "minecraft:silverfish"}`）。`saveToNBT` 始终使用新版格式。

### 3. 生成延迟的单位是 tick

`m_spawnDelay`、`m_minSpawnDelay`、`m_maxSpawnDelay` 均以游戏 tick 为单位（20 tick = 1 秒）。默认初始延迟 20 tick（1 秒），之后每次生成间隔 200-800 tick（10-40 秒）。

### 4. onlyOpsCanSetNbt 返回 true

与 MC Java 一致，刷怪笼的 NBT 数据仅 OP 玩家可修改。

### 5. CustomSpawnRules 光照检查

当 `m_customSpawnRules` 存在时，刷怪笼在生成前会检查每个生成位置的方块光照和天空光照是否在指定范围内，不满足时跳过该位置。当 `m_customSpawnRules` 不存在时，刷怪笼会通过 `EntitySpawnPlacementRegistry::canSpawnEntity()` 检查默认生成放置规则（地面/水中/岩浆放置类型和自定义谓词）。CustomSpawnRules 使用原始光照值，不应用天空变暗。

NBT 格式：`SpawnData.CustomSpawnRules.block_light_limit` 和 `sky_light_limit` 为 `IntArray[min, max]`。

### 6. 和平难度下非和平生物不生成

当 CustomSpawnRules 存在且实体分类为 Monster 时，和平难度下刷怪笼不会生成该实体（与 MC Java BaseSpawner 行为一致）。
