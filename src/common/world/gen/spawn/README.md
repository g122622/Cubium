# Spawn Module

世界生成时的生物放置系统，负责在区块首次生成时放置被动动物。

## 目录结构

```
spawn/
├── WorldGenSpawner.hpp      # 区块生成生物放置器
├── WorldGenSpawner.cpp      # 实现
└── README.md                # 本文件
```

## 核心类型

### SpawnedEntityData

生成实体数据结构，用于记录区块生成时应该放置的实体信息。

```cpp
struct SpawnedEntityData {
    std::string entityTypeId;      // 实体类型ID（如 "minecraft:pig"）
    f32 x, y, z;                   // 生成位置
    world::spawn::SpawnReason spawnReason;  // 生成原因（默认 ChunkGeneration）
};
```

### SpawnReason 枚举

定义实体生成的各种原因。完整定义见 `entity/core/EntitySpawnPlacementRegistry.hpp`。

常用值：
- `ChunkGeneration` - 区块生成时放置（WorldGenSpawner 使用）
- `Natural` - 自然刷新生成
- `SpawnEgg` - 刷怪蛋
- `Spawner` - 刷怪笼
- `Breeding` - 繁殖

## WorldGenSpawner

区块生成时的生物放置器，参考 MC 1.16.5 `WorldEntitySpawner.performWorldGenSpawning`。

### 功能特点

1. **仅生成被动动物**：只处理 `Creature` 分类的实体
2. **生物群系感知**：根据生物群系的生成配置选择动物类型
3. **加权随机选择**：根据配置权重随机选择动物种类
4. **群体生成**：支持一次生成多个同类型动物
5. **位置验证**：使用 `EntitySpawnPlacementRegistry` 验证生成位置

### 使用方式

```cpp
WorldGenSpawner spawner;
std::vector<SpawnedEntityData> entities;

// 区块生成时调用
i32 count = spawner.spawnInitialMobs(
    region,      // WorldGenRegion
    biome,        // 区块中心的主要生物群系
    chunkX, chunkZ,
    generator,    // 区块生成器
    random,
    entities      // 输出：生成的实体数据
);

// 之后由 ServerWorld 创建实际实体
world.spawnEntitiesFromChunkGeneration(entities);
```

### 生成流程

1. 获取生物群系的 Creature 生成列表
2. 计算总权重并加权随机选择动物类型
3. 根据生物群系生成概率决定是否生成
4. 随机选择区块内位置并查找地面高度
5. 使用 `EntitySpawnPlacementRegistry` 验证位置
6. 在组内随机偏移生成多个个体
7. 将实体数据存入 `SpawnedEntityData` 列表

## 与 NaturalSpawner 的区别

| 特性 | WorldGenSpawner | NaturalSpawner |
|------|-----------------|----------------|
| 触发时机 | 区块首次生成 | 运行时定期 tick |
| 生成分类 | 仅 Creature | Monster、Creature、Ambient 等 |
| 生成条件 | 不需要光照条件 | 需要检查光照、玩家距离等 |
| 数据流 | 输出 SpawnedEntityData | 直接创建实体 |

## 依赖关系

- `entity/core/EntitySpawnPlacementRegistry.hpp` - 位置验证、SpawnReason 枚举
- `entity/core/EntityRegistry.hpp` - 实体类型查询
- `entity/core/EntityClassification.hpp` - 实体分类
- `world/biome/Biome.hpp` - 生物群系生成配置
- `world/spawn/MobSpawnInfo.hpp` - 生成条目定义

## 测试用例

- [tests/common/world/gen/ChunkSpawnIntegrationTest.cpp](../../../../tests/common/world/gen/ChunkSpawnIntegrationTest.cpp) - 集成测试
- [tests/common/entity/EntitySpawnPlacementRegistryTest.cpp](../../../../tests/common/entity/EntitySpawnPlacementRegistryTest.cpp) - SpawnReason 枚举测试

## 参考

- MC 1.16.5 `net.minecraft.world.spawner.WorldEntitySpawner`
- MC 1.16.5 `net.minecraft.entity.SpawnReason`
