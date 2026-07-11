# Spawn 模块

本模块定义生物群系的实体生成配置，参考 MC 1.21.11 的 `MobSpawnInfo` 系统（`EntityClassification` 已对齐 1.21.11 的 8 类 `MobCategory`，spawn list 工厂方法主要按 1.16.5 `BiomeMaker` 对齐，逐步迁移至 1.21.11）。

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

不同海洋类型有不同的生物配置（均按 MC 1.16.5 `BiomeMaker` 对齐）：
- **普通海洋 / 深海**：squid(1,1,4)+dolphin(1,1,2) WC，cod(10,3,6) WA，8 标准陆地怪物 + drowned + bat。深海 spawn list 与浅水一致（仅 generation settings 不同）。
- **温水海洋（浅）**：squid(10,1,2)+dolphin(2,1,2) WC，cod(15,3,6)+pufferfish(5,1,3)+tropical_fish(25,8,8) WA，8 怪物 + drowned + bat。
- **温水海洋（深）**：与浅水差异为 squid(8,1,4) 与 cod(8,3,6)（权重不同），其余一致。
- **冷水海洋**：squid(3,1,4)+dolphin(2,1,2) WC，cod(15,3,6)+salmon(15,1,5) WA，8 怪物 + drowned + bat。深海 spawn list 与浅水一致。
- **冰冻海洋**：squid(1,1,4) WC（无 dolphin），salmon(15,1,5) WA（无 cod），polar_bear(1,1,2) Creature，8 怪物 + drowned + bat，无 stray。深海 spawn list 与浅水一致。
- **暖水海洋（浅）**：pufferfish(15,1,3)+tropical_fish(25,8,8) WA，squid(10,4,4)+dolphin(2,1,2) WC，8 怪物 + bat，无 drowned/cod/salmon。
- **深海暖水海洋**：与暖水海洋相似，但有 drowned、无 pufferfish，squid 权重 5、minCount 1。

### 8. 史莱姆区块判断算法与 Java 不兼容

`SlimeChunkChecker` 使用 Java `LegacyRandomSource`（48位 LCG）算法，与项目默认的 `Xoroshiro128ppRandom` 不兼容。史莱姆区块判断必须使用 `SlimeChunkChecker` 的静态方法，不能直接用 `math::Random` 替代。

### 9. spawn list 与原版 1.16.5 的对齐状态

`MobSpawnInfo.cpp` 中各工厂方法已按 MC 1.16.5 `BiomeMaker` 与 `DefaultBiomeFeatures` 收敛，主要对齐点：
- **cod/salmon/tropical_fish/pufferfish 原版归 `WaterAmbient`**，本项目已对齐（此前误归 `WaterCreature`）。
- **hoglin 原版归 `Monster`**（虽是 AnimalEntity 子类），本项目已对齐。
- **Savanna 系列拆分三个工厂方法**：`createSavanna()`（无 llama/wolf）、`createSavannaPlateau()`（仅 llama，无 wolf）、`createShatteredSavanna()`（与 Savanna 相同）。1.16.5 中 ShatteredSavannaPlateau 也无 llama/wolf，复用 `createShatteredSavanna()`。
- **DeepLukewarmOcean 独立工厂方法**：与浅水版本在 squid/cod 权重上有差异（squid 8 vs 10、cod 8 vs 15），DeepColdOcean/DeepFrozenOcean 与浅水版本 spawn list 一致，仍复用浅水工厂方法。
- **Jungle 系列三个工厂方法**：`createJungle()`（parrot 40,1,2 + ocelot 2,1,3 + panda 1,1,2）、`createSparseJungle()`（仅 baseJungleSpawns，1.16.5 无 wolf）、`createBambooJungle()`（parrot 40,1,2 + panda 80,1,2 + ocelot 2,1,1）。
- **SnowyBeach 独立工厂方法**：与 SnowyTundra 差异较大（无 creature、skeleton 权重 100、无 stray、概率 0.1F），使用 `createSnowyBeach()`；IceSpikes spawn list 与 SnowyTundra 完全一致，复用 `createSnowy()`。

仍存在的偏差（以 `TODO(spawn-list-*)` 标注）：
- `EntityClassification` 已扩展至 8 类，与 MC 1.21.11 `MobCategory` 完全对齐（`Axolotls` 与 `UndergroundWaterCreature` 已加入）。美西螈已归入独立的 `Axolotls` 分类，不再塞进 `WaterCreature`。
- `UndergroundWaterCreature` 分类与发光鱿鱼（glow_squid）实体均已就绪，LushCaves 已添加 `glow_squid (10,4,6)` spawn entry。
- 多个 1.16.5 实体未注册（parched、camel、bogged、armadillo），对应 spawn list 待补。nautilus 已注册并加入海洋生物群系。

### 10. Jungle 系列变体使用不同工厂方法

Jungle 系列生物群系按 MC 1.16.5 `BiomeMaker` 拆分为三个工厂方法，必须按变体正确调用：
- `createJungle()`：Jungle / JungleHills / ModifiedJungle / ModifiedJungleEdge（含 ocelot 2,1,3 + parrot 40,1,2 + panda 1,1,2）
- `createSparseJungle()`：JungleEdge / ModifiedJungleEdge（旧名 sparseJungle，仅 baseJungleSpawns，无 ocelot/parrot/panda/wolf；1.21.11 才加 wolf，本项目对齐 1.16.5）
- `createBambooJungle()`：BambooJungle / BambooJungleHills（panda weight=80、ocelot pack=1，与普通 jungle 不同）

三者共享 `applyBaseJungleSpawns()`（对应 `DefaultBiomeFeatures.func_243747_h()`），含 farmAnimals + **额外 chicken** + commonSpawns。新增 Jungle 变体时应复用该辅助函数。

### 11. Savanna 系列变体使用不同工厂方法

Savanna 系列生物群系按 MC 1.16.5 `BiomeMaker` 拆分为三个工厂方法：
- `createSavanna()`：Savanna（farmAnimals + horse + donkey + commonSpawns，无 llama/wolf）
- `createSavannaPlateau()`：SavannaPlateau（在 Savanna 基础上加 llama 8,4,4，无 wolf）
- `createShatteredSavanna()`：ShatteredSavanna / ShatteredSavannaPlateau（spawn list 与普通 Savanna 相同，无 llama/wolf）

三者共享 `applyBaseSavannaSpawns()` 辅助函数。1.16.5 中 ShatteredSavanna/ShatteredSavannaPlateau 的 shattered 标志仅影响地形（generation settings），不影响 spawn list。
