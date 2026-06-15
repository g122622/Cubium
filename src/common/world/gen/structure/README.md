#结构生成系统(Structure Generation System)

本模块实现 MC 1.21.11 的数据驱动结构生成管线，包括结构定义、结构集合、放置策略和 Jigsaw 组装。

        ##目录结构树

```text structure /
├── Structure.hpp #结构基类定义（含 SpawnOverrides、BiomeTag 引用、typeToId/nameToStructureType 静态方法）
├── Structure.cpp #结构基类实现（含 generateChest/generateDispenser 辅助方法）
├── StructureBoundingBox.hpp #结构边界框（用于判断片段与区块交集）
├── StructureBoundingBox.cpp
├── StructureSet.hpp #结构集合模型（加权条目 +
    放置规则）
├── StructureSet.cpp #20 个原版 StructureSet 注册
├── StructureSetLoader.hpp #结构集合 JSON 加载器（数据包）
├── StructureSetLoader.cpp
├── StructureDefinitionLoader.hpp #结构定义 JSON 加载器（数据包）
├── StructureDefinitionLoader.cpp
├── JigsawStructure.hpp #Jigsaw 拼图结构基类
├── JigsawStructure.cpp
├── StructureManager.hpp #结构管理器（注册、查询、生成协调）
├── StructureManager.cpp
├── placement / #结构放置策略（MC 1.21.11 对齐）
│   ├── StructurePlacement.hpp #放置基类、FrequencyReductionMethod、ExclusionZone
│   ├── StructurePlacement.cpp #频率缩减检查、排斥区检查
│   ├── RandomSpreadStructurePlacement.hpp #网格随机 / 三角分布（大多数结构）
│   ├── RandomSpreadStructurePlacement.cpp
│   ├── ConcentricRingsStructurePlacement.hpp #同心环分布（要塞）
│   ├── ConcentricRingsStructurePlacement.cpp
│   └── README.md
├── pools / #Jigsaw 模板池
│   ├── Pools.hpp / cpp #模板池注册入口
│   ├── ProcessorLists.hpp / cpp #结构处理器列表
│   ├── bastion / #堡垒遗迹模板池
│   ├── pillager_outpost / #掠夺者前哨站模板池
│   ├── trial_chambers / #试炼密室模板池
│   └── village / #村庄模板池
└── structures / #具体结构实现
    ├── BastionRemnantStructure.*#堡垒遗迹（Jigsaw）
    ├── BuriedTreasureStructure.*#埋藏宝藏（程序化）
    ├── DesertPyramidStructure.*#沙漠神殿（程序化）
    ├── EndCityStructure.*#末地城（递归模板）
    ├── FortressStructure.*#下界要塞（程序化）
    ├── IglooStructure.*#雪屋（模板堆叠）
    ├── JungleTempleStructure.*#丛林神庙（程序化）
    ├── MineshaftStructure.*#废弃矿井（程序化片段）
    ├── NetherFossilStructure.*#下界化石（模板）
    ├── OceanMonumentPieces.*#海洋纪念碑片段
    ├── OceanMonumentStructure.*#海洋纪念碑（程序化）
    ├── OceanRuinStructure.*#海底废墟（模板 +
    IntegrityProcessor）
    ├── PillagerOutpostStructure.*#掠夺者前哨站（Jigsaw）
    ├── RuinedPortalStructure.*#废弃传送门（模板）
    ├── ShipwreckStructure.*#沉船（模板）
    ├── StrongholdPieces.*#要塞片段（走廊、房间、传送门室等）
    ├── StrongholdStructure.*#要塞（递归片段系统）
    ├── SwampHutStructure.*#沼泽小屋（程序化）
    ├── TrialChambersStructure.*#试炼密室（Jigsaw）
    ├── VillageStructure.*#村庄（Jigsaw）
    └── WoodlandMansionStructure.*
            #林地府邸（程序化房间布局）
```

            ##MC 1.21.11 数据驱动管线

``` worldgen /
        structure/*.json     → Structure (定义结构类型、生物群系标签、装饰阶段)
worldgen/structure_set/*.json → StructureSet (加权条目 + StructurePlacement)
tags/worldgen/biome/*.json    → BiomeTag (has_structure/* 标签)
worldgen/processor_list/*.json→ ProcessorList (方块替换处理器)

生成流程:
1. STRUCTURE_STARTS 阶段:
   - 遍历 StructureSetRegistry 中的每个 StructureSet
   - StructurePlacement.isStructureChunk() 三步检查:
     a. 计算候选区块位置（网格偏移 + 频率缩减）
     b. 频率缩减检查（Default/LegacyType1/2/3）
     c. 排斥区检查（如掠夺者前哨站排斥村庄）
   - StructureSet.selectEntry() 按权重随机选择结构
   - Structure.generate() 创建 StructureStart（不写方块！）

2. STRUCTURE_REFERENCES 阶段:
   - 扫描 17×17 区块范围，收集与当前区块相交的结构引用
   - 调用 StructureStart.incrementRefCount()

3. FEATURES 阶段:
   - 遍历 structureReferences（跨区块引用）
   - 从源区块获取 StructureStart，调用 Structure.placeInChunk()
   - placeInChunk() → StructurePiece.generate() 写入方块
```

## 内部模块关系

```
┌─────────────────────────────────────────────────────────────────┐
│                    StructureSetRegistry                          │
│            (20 个原版 StructureSet，按放置策略分组)                │
└───────────────────────────┬─────────────────────────────────────┘
                            │
            ┌───────────────┼───────────────┐
            │               │               │
            ▼               ▼               ▼
    ┌──────────────┐ ┌──────────────┐ ┌──────────────┐
    │ StructureSet │ │ StructureSet │ │ StructureSet │
    │  villages    │ │  strongholds │ │  nether_     │
    │  (5 变体)    │ │  (同心环)    │ │  complexes   │
    │  RandomSpread│ │ConcentricRing│ │  (2:3 权重)  │
    └──────┬───────┘ └──────┬───────┘ └──────┬───────┘
           │                │                │
           ▼                ▼                ▼
    ┌──────────────────────────────────────────────────┐
    │              StructurePlacement                   │
    │  ├─ RandomSpreadStructurePlacement               │
    │  │   (LINEAR/TRIANGULAR, frequency, exclusion)   │
    │  └─ ConcentricRingsStructurePlacement            │
    │      (128 要塞, distance=32, spread=3)            │
    └──────────────────────────────────────────────────┘
                            │
                            ▼
    ┌──────────────────────────────────────────────────┐
    │              Structure (基类)                     │
    │  ├─ id() → ResourceLocation                      │
    │  ├─ biomeTag() → BiomeTag*                       │
    │  ├─ spawnOverrides() → SpawnOverrides*           │
    │  ├─ terrainAdaptation() → None/Bury/BeardThin/   │
    │  │                          BeardBox/Encapsulate │
    │  ├─ typeToId(StructureType) → ResourceLocation   │
    │  ├─ nameToStructureType(name) → optional<Type>   │
    │  ├─ generate() → StructureStart (仅创建片段)     │
    │  ├─ generateChest() → 放置宝箱+设置战利品表      │
    │  ├─ generateDispenser() → 放置发射器+设置战利品表 │
    │  └─ placeInChunk() → 写入方块 (FEATURES 阶段)   │
    └──────────────────────────────────────────────────┘
                            │
        ┌───────────────────┼───────────────────┐
        │                   │                   │
        ▼                   ▼                   ▼
  程序化结构          Jigsaw 结构         递归片段结构
  - Mineshaft         - Village           - EndCity
  - OceanMonument     - BastionRemnant    - WoodlandMansion
  - DesertPyramid     - PillagerOutpost   - Stronghold
  - SwampHut          - TrialChambers
  - BuriedTreasure
```

## 上下游外部依赖关系

### 上游依赖（本模块依赖）

| 模块 | 用途 |
|------|------|
| `world/gen/jigsaw/` | Jigsaw 管理器、模板池、拼图块 |
| `world/gen/feature/template/` | NBT 模板加载和处理器 |
| `world/gen/chunk/IChunkGenerator.hpp` | 区块生成器接口 |
| `world/biome/BiomeTag.hpp` | 生物群系标签（has_structure/*） |
| `world/biome/BiomeTags.hpp` | 34 个 has_structure 标签注册 |
| `world/block/VanillaBlocks.hpp` | 方块状态注册表 |
| `world/IWorldWriter.hpp` | 世界写入接口 |
| `resource/ResourceLocation.hpp` | 资源定位符 |

### 下游依赖（依赖本模块）

| 模块 | 用途 |
|------|------|
| `world/gen/chunk/NoiseChunkGenerator` | 区块生成器在三个阶段调用结构生成 |
| `world/gen/density/Beardifier` | 结构地形平滑密度函数 |
| `server/world/ServerWorld` | 世界初始化时调用注册；`findNearestStructure()` 使用 `typeToId()` 查询注册表 |
| `server/command/commands/LocateCommand` | `/locate` 命令使用 `nameToStructureType()` 解析玩家输入 |

## 容易踩的坑

### 1. 结构生成阶段顺序（编译期强制保证）

结构必须严格按 MC 1.21.11 管线执行：
- `STRUCTURE_STARTS`: 只创建 StructureStart，**禁止写方块**（`Structure::generate()` 不接受 `IWorldWriter&` 参数，编译期保证）
- `STRUCTURE_REFERENCES`: 收集跨区块引用，incrementRefCount()
- `FEATURES`: 通过 `placeInChunk()` → `StructurePiece::generate()` 写入方块

`Structure::generate()` 签名为 `generate(IChunkGenerator&, Random&, chunkX, chunkZ)`，不接受 `IWorldWriter&`，确保结构起点阶段不可能写入方块。所有方块写入必须通过 `StructurePiece::generate()` 在 FEATURES 阶段执行。

### 2. 跨区块结构引用

FEATURES 和 Beardifier 阶段必须使用 `chunk.structureReferences()` 遍历源区块的 StructureStart，而非仅使用 `chunk.structureStarts()`。否则跨区块的大型结构（村庄、堡垒遗迹等）会丢失片段。

### 3. StructureSet 放置策略

每个 StructureSet 有独立的 StructurePlacement，包含三步检查：
1. 候选区块计算（spacing/separation/spreadType/salt）
2. 频率缩减（Default/LegacyType1/LegacyType2/LegacyType3）
3. 排斥区（ExclusionZone，如掠夺者前哨站排斥村庄）

旧代码路径（StructureSeparationSettings + findStructureStart）仍作为兼容接口存在，新代码应使用 StructureSet + StructurePlacement。

### 4. FrequencyReductionMethod 差异

- `Default`: `nextFloat() < frequency`，大多数结构使用
- `LegacyType1`: 掠夺者前哨站（基于方块坐标种子）
- `LegacyType2`: 埋藏宝藏（固定盐值 10387320）
- `LegacyType3`: 废弃矿井（setLargeFeatureSeed + nextDouble）

### 5. RandomSpreadType

- `Linear`: 均匀随机分布（spacing - separation 范围内均匀取值）
- `Triangular`: 两次随机取平均，产生更集中的分布（末地城、林地府邸、海洋纪念碑使用）

### 6. 生物群系标签

结构生物群系过滤已从 `validBiomes()` 线性搜索迁移到 `biomeTag()` O(1) 查找。BiomeTags 注册了 34 个 `has_structure/*` 标签。旧代码路径仍保留以兼容。

### 7. SpawnOverrides

部分结构需要覆盖其边界框内的默认生物生成规则：
- 海洋纪念碑: 4 只守卫者（Full 边界框）
- 掠夺者前哨站: 1 只掠夺者（Full 边界框）
- 堡垒遗迹: 2-4 只怪物 + 2-4 只生物（Piece 边界框）
- 下界要塞: 2-4 只怪物（Piece 边界框）
- 沼泽小屋: 1 只女巫（Full 边界框）

### 8. TerrainAdaptation

Beardifier 根据 TerrainAdaptation 类型计算地形密度偏移：
- `None`: 无偏移
- `Bury`: 线性距离衰减，将结构埋入地下
- `BeardThin`: 高斯核 + 胡须曲线 × 0.8
- `BeardBox`: BeardThin + 垂直方向使用完整高度范围 × 0.8
- `Encapsulate`: 半分辨率 Bury × 0.8，完全包裹

### 9. StructureBoundingBox 边界包含性

边界框的 `max` 坐标是包含边界，计算交集时需注意：
```cpp
return m_maxX >= chunkMinX && m_minX <= chunkMaxX &&
       m_maxZ >= chunkMinZ && m_minZ <= chunkMaxZ;
```

### 10. 资源位置标识

所有结构现在使用 `ResourceLocation` 作为标识（如 `minecraft:village_plains`），替代旧的 `StructureType` 枚举。`StructureType` 作为兼容接口保留，后续将迁移完成。使用 `Structure::typeToId()` 进行 `StructureType → ResourceLocation` 转换，使用 `Structure::nameToStructureType()` 进行反向转换（支持别名和 `minecraft:` 前缀）。

### 11. StructureType 枚举与名称映射

`StructureType::Temple` 是多个结构的兼容类型（沙漠神殿、丛林神庙、雪屋、沼泽小屋、下界化石共享此枚举值）。`nameToStructureType()` 的别名映射将这些子名称都映射到 `StructureType::Temple`，而 `typeToId(StructureType::Temple)` 统一返回 `minecraft:temple`。注意 `bastion_remnant` / `bastion` 均映射到 `StructureType::Bastion`，而 `typeToId` 返回 `minecraft:bastion_remnant`（而非 `minecraft:bastion`）。

### 12. generateChest / generateDispenser 辅助方法

`StructurePiece` 基类提供了 `generateChest()` 和 `generateDispenser()` 两个辅助方法，用于在结构生成中放置带有战利品表的容器方块。这两个方法封装了以下逻辑：

1. 通过坐标偏移计算世界坐标
2. 检查位置是否在生成边界内
3. 放置带朝向的方块（宝箱使用 `HORIZONTAL_FACING`，发射器使用 `FACING`）
4. 通过 `dynamic_cast<IWorld*>(&world)->getBlockEntity()` 获取自动创建的方块实体
5. 设置战利品表（`ChestEntity::setLootTable()` / `DispenserBlockEntity::setLootTable()`）

这些方法要求 `WorldGenRegion` 正确覆写 `getBlockEntity()`/`setBlockEntity()`/`removeBlockEntity()` 方法，以便在结构生成阶段访问和操作方块实体。`WorldGenRegion::setBlockState()` 会在放置有方块实体的方块时自动创建对应的 `BlockEntity`，然后通过 `getBlockEntity()` 即可获取并设置战利品表。
