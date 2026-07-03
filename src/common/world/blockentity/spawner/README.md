# 刷怪笼方块实体（Spawner Block Entity）

刷怪笼（Mob Spawner）方块实体，自动在附近区域周期性生成实体。

## 目录结构

```
spawner/
├── MobSpawnerBlockEntity.hpp  # 刷怪笼方块实体定义
├── MobSpawnerBlockEntity.cpp  # 刷怪笼方块实体实现
├── SpawnerLogic.hpp           # 刷怪笼逻辑公共类（对应 MC Java BaseSpawner）
├── SpawnerLogic.cpp           # 刷怪笼逻辑实现
└── README.md
```

## 内部模块关系

```
SpawnerLogic（公共类，对应 MC Java BaseSpawner）
    ├── MobSpawnerBlockEntity（方块刷怪笼，持有 SpawnerLogic 实例）
    └── SpawnerMinecartEntity（刷怪笼矿车，持有 SpawnerLogic 实例）

SpawnerLogic 包含：
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

### 1. setEntityId 行为（对齐 MC Java BaseSpawner.setEntityId）

调用 `setEntityId()` 时：
- 直接设置 `m_nextEntityId` 为指定实体类型
- 如果 `m_spawnPotentials` 为空，自动添加一个权重为 1 的条目
- 重置生成延迟（在 `[minSpawnDelay, maxSpawnDelay]` 范围内随机），但**不会**从 `m_spawnPotentials` 中重新随机选择实体类型
- 注意：旧的实现调用了 `_delay()`，而 `_delay()` 内部会调用 `_selectNextEntity()` 从候选列表中随机选择，导致 `setEntityId` 刚设置的 `m_nextEntityId` 被覆盖。现已修复为直接重置延迟而不调用 `_delay()`，与 MC Java `BaseSpawner.setEntityId()` 行为一致（MC Java 只修改 nextSpawnData 的 entity id，不重选候选）

此方法由以下路径调用：
- `SpawnerBlock::onBlockActivated()` — 玩家手持刷怪蛋右键刷怪笼
- `SpawnEggItem::onItemUse()` — 刷怪蛋使用时检测到刷怪笼方块实体
- 要塞结构生成时配置蠹虫刷怪笼

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

### 7. 成功生成实体时播放粒子事件

刷怪笼成功生成实体后，会通过 `IWorld::playEvent(WorldEvents::MOB_SPAWNER_PARTICLES, pos, 0)` 通知客户端播放爆发粒子效果（2004 号世界事件）。客户端收到后在方块中心 2 格范围内随机生成 20 个烟雾粒子和 20 个火焰粒子。参考 MC Java `BaseSpawner.serverTick()` 中成功生成后调用 `levelEvent(2004, pos, 0)` 的逻辑。

### 8. 客户端持续粒子效果（animateTick）

刷怪笼方块 `SpawnerBlock::animateTick()` 每客户端 tick 在方块内随机位置生成 1 个烟雾粒子和 1 个火焰粒子，两者共享同一随机坐标。参考 MC Java `BaseSpawner.clientTick()` 中的持续粒子逻辑。注意这是与生成事件粒子独立的、持续播放的视觉效果。
