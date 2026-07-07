# Spawn 模块

本模块定义生物群系的实体生成配置，参考 MC 1.16.5 的 `MobSpawnInfo` 系统。

## 目录结构

```
src/common/world/spawn/
├── IWorldSpawnAdapter.hpp  # IWorld → ISpawnWorldReader 适配器（供 EntitySpawnPlacementRegistry 使用）
├── MobSpawnInfo.hpp        # 生成信息类型定义（SpawnCosts、SpawnEntry、SpawnCategory、MobSpawnInfo）
├── MobSpawnInfo.cpp        # 各生物群系的工厂方法实现（createPlains、createOcean 等）
├── SlimeChunkChecker.hpp   # 史莱姆区块判断工具（使用世界种子确定性计算）
└── SlimeChunkChecker.cpp   # 史莱姆区块判断算法实现（Java LCG 复刻）
```

## 内部模块关系

```
┌─────────────────────────────────────────────────────────────────┐
│                    spawn 模块                                    │
├─────────────────────────────────────────────────────────────────┤
│  MobSpawnInfo.hpp                                               │
│  ├── SpawnCosts        生成成本（控制高密度区域实体数量）         │
│  ├── SpawnEntry        单个实体类型的生成配置条目                │
│  ├── SpawnCategory     生成分类容器（怪物、动物、环境等）        │
│  └── MobSpawnInfo      完整的生物群系实体生成信息                │
│      └── Builder       流式构建器                               │
│                                                                 │
│  MobSpawnInfo.cpp                                               │
│  └── 工厂方法：主世界、下界、末地、洞穴等生物群系的生成配置      │
│                                                                 │
│  SlimeChunkChecker.hpp/.cpp                                     │
│  ├── isSlimeChunk()      判断区块是否为史莱姆区块（10%概率）     │
│  ├── computeSlimeChunkSeed()  计算区块种子（MC公式）             │
│  └── getSurfaceSlimeSpawnChance()  地表生成概率（月相相关）      │
└─────────────────────────────────────────────────────────────────┘
```

## 上下游外部依赖关系

**上游依赖：**
- `common/core/Types.hpp` - 基础类型（i32, f32, f64, std::string 等）
- `common/entity/core/EntityClassification.hpp` - 实体分类枚举（Monster、Creature、Ambient 等）

**下游使用：**
- `Biome.hpp` / `BiomeRegistry.cpp` - 生物群系定义，通过 `Biome::spawnInfo()` 获取生成信息
- `WorldGenSpawner.hpp` - 区块生成时放置动物（仅使用 Creature 分类）
- `NaturalSpawner.hpp` (server) - 运行时自然生成（处理所有分类）
- `EntitySpawnPlacementRegistry.cpp` - 史莱姆生成条件检查使用 `SlimeChunkChecker::isSlimeChunk()`

## 容易踩的坑

### 1. 权重理解错误

权重是相对值，实际概率 = `weight / totalWeight`。例如平原僵尸权重 95，骷髅权重 100，则僵尸概率 = 95/195 ≈ 48.7%，而非 95%。

### 2. 实例数量限制混淆

`SpawnEntry::minCount/maxCount` 是**单次生成**的数量范围，`SpawnCategory::maxInstances` 是该分类在区域内的**总实体数量上限**。例如僵尸单次生成 4 个，但区域内最多 70 个怪物。

### 3. 工厂方法返回临时对象

工厂方法返回值，不是引用。`const MobSpawnInfo& info = MobSpawnInfo::createPlains()` 是悬垂引用，危险！

### 4. 空指针检查

`getSpawnCost()` 可能返回 `nullptr`，使用前必须检查：
```cpp
const SpawnCosts* costs = spawnInfo.getSpawnCost(entityId);
if (costs && costs->isValid()) { /* 使用成本限制 */ }
```

### 5. 实体类型ID格式

必须使用完整的命名空间格式 `"minecraft:zombie"`，而非简写 `"zombie"`。

### 6. Builder 模式必须调用 build()

`MobSpawnInfo info = MobSpawnInfo::Builder().addSpawn(...);` 类型不匹配，必须 `.build()` 结尾。

### 7. 工厂方法的海洋变体差异

不同海洋类型有不同的生物配置：
- **暖水海洋**：热带鱼/河豚归 WaterAmbient，鱿鱼/海豚归 WaterCreature，含 8 条标准陆地怪物 + bat，无 cod/salmon/drowned
- **深海暖水海洋**：与暖水海洋相似，但有 drowned、无 pufferfish，squid 权重更低
- **冰冻海洋**：鲑鱼为主，北极熊，**没有鳕鱼**
- **深海**：更多鱿鱼

### 8. 史莱姆区块判断算法与 Java 不兼容

`SlimeChunkChecker` 使用 Java `LegacyRandomSource`（48位 LCG）算法，与项目默认的 `Xoroshiro128ppRandom` 不兼容。史莱姆区块判断必须使用 `SlimeChunkChecker` 的静态方法，不能直接用 `math::Random` 替代。

### 9. spawn list 与原版 1.16.5 仍有未收敛偏差

`MobSpawnInfo.cpp` 中各工厂方法（`createOcean`/`createSavanna`/`createSnowy`/`createCrimsonForest`/`createLushCaves` 等）的 spawn list 与原版 MC Java 1.16.5 仍存在若干已知偏差。这些偏差以 `TODO(spawn-list-<biome>)` 形式逐项标注在对应工厂方法内，文件顶部有汇总说明。收敛任一项时需同步删除对应 TODO。共性偏差包括：
- `EntityClassification` 仅 6 类，缺失原版的 `UndergroundWaterCreature` 与 `Axolotls`，导致美西螈等临时塞进 `WaterCreature`
- cod/salmon/tropical_fish/pufferfish 原版归 `WaterAmbient`，本项目误归 `WaterCreature`
- hoglin 原版归 `Creature`，本项目误归 `Monster`
- 多个 1.16.5 实体未注册（parched、camel、nautilus、glow_squid、bogged、armadillo、zombie_horse），对应 spawn list 待补

### 10. Jungle 系列变体使用不同工厂方法

Jungle 系列生物群系按 MC 1.16.5 `OverworldBiomes` 拆分为三个工厂方法，必须按变体正确调用：
- `createJungle()`：Jungle / JungleHills / ModifiedJungle / ModifiedJungleEdge（含 ocelot 2,1,3 + parrot 40,1,2 + panda 1,1,2）
- `createSparseJungle()`：JungleEdge（旧名 sparseJungle，含 wolf 8,2,4，无 ocelot/parrot/panda）
- `createBambooJungle()`：BambooJungle / BambooJungleHills（panda weight=80、ocelot pack=1，与普通 jungle 不同）

三者共享 `applyBaseJungleSpawns()`（对应 `BiomeDefaultFeatures.baseJungleSpawns()`），含 farmAnimals + **额外 chicken** + commonSpawns。新增 Jungle 变体时应复用该辅助函数。
