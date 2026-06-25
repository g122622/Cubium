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
- `entity/core/EntityType.hpp` - 实体类型（创建实体实例）
- `entity/combat/DifficultyInstance.hpp` - 难度实例（finalizeSpawn 需要）
- `world/IWorld.hpp` - 世界接口（生成实体、查询附近实体）

## 容易踩的坑

### 1. setEntityId 会自动添加默认生成候选

调用 `setEntityId()` 时，如果 `m_spawnPotentials` 为空，会自动添加一个权重为 1 的条目。这是为了让只设置单一实体类型的刷怪笼（如要塞蠹虫刷怪笼）无需手动添加候选列表。

### 2. NBT 格式兼容 MC Java 1.21+

`loadFromNBT` 支持 MC Java 1.21 的 SpawnData 格式（`{entity: {id: "minecraft:silverfish"}}`）以及旧版格式（`{id: "minecraft:silverfish"}`）。`saveToNBT` 始终使用新版格式。

### 3. 生成延迟的单位是 tick

`m_spawnDelay`、`m_minSpawnDelay`、`m_maxSpawnDelay` 均以游戏 tick 为单位（20 tick = 1 秒）。默认初始延迟 20 tick（1 秒），之后每次生成间隔 200-800 tick（10-40 秒）。

### 4. onlyOpsCanSetNbt 返回 true

与 MC Java 一致，刷怪笼的 NBT 数据仅 OP 玩家可修改。
