# 结构生成系统 (Structure Generation System)

## 目录结构树

```text
structure/
├── Structure.hpp                      # 结构基类定义
├── Structure.cpp                      # 结构基类实现
├── StructureBoundingBox.hpp           # 结构边界框（用于判断片段与区块交集）
├── StructureBoundingBox.cpp
├── JigsawStructure.hpp                # Jigsaw 拼图结构基类
├── JigsawStructure.cpp
├── StructureManager.hpp               # 结构管理器（注册、查询、生成协调）
├── StructureManager.cpp
├── pools/                             # Jigsaw 模板池
│   ├── Pools.hpp                      # 模板池注册入口
│   ├── Pools.cpp
│   ├── ProcessorLists.hpp             # 结构处理器列表（方块替换、完整性等）
│   ├── ProcessorLists.cpp
│   ├── bastion/                       # 堡垒遗迹模板池
│   ├── pillager_outpost/              # 掠夺者前哨站模板池
│   ├── trial_chambers/                # 试炼密室模板池
│   └── village/                       # 村庄模板池
└── structures/                        # 具体结构实现
    ├── BastionRemnantStructure.*      # 堡垒遗迹（Jigsaw）
    ├── BuriedTreasureStructure.*      # 埋藏宝藏（程序化）
    ├── DesertPyramidStructure.*       # 沙漠神殿（程序化）
    ├── EndCityStructure.*             # 末地城（递归模板）
    ├── FortressStructure.*            # 下界要塞（程序化+Jigsaw）
    ├── IglooStructure.*               # 雪屋（模板堆叠）
    ├── JungleTempleStructure.*        # 丛林神庙（程序化）
    ├── MineshaftStructure.*           # 废弃矿井（程序化片段）
    ├── NetherFossilStructure.*        # 下界化石（模板）
    ├── OceanMonumentPieces.*          # 海洋纪念碑片段（房间图系统）
    ├── OceanMonumentStructure.*       # 海洋纪念碑（程序化）
    ├── OceanRuinStructure.*           # 海底废墟（模板+IntegrityProcessor）
    ├── PillagerOutpostStructure.*     # 掠夺者前哨站（Jigsaw）
    ├── RuinedPortalStructure.*        # 废弃传送门（模板）
    ├── ShipwreckStructure.*           # 沉船（模板）
    ├── StrongholdPieces.*             # 要塞片段（走廊、房间、传送门室等）
    ├── StrongholdStructure.*          # 要塞（递归片段系统）
    ├── SwampHutStructure.*            # 沼泽小屋（程序化）
    ├── TrialChambersStructure.*       # 试炼密室（Jigsaw）
    ├── VillageStructure.*             # 村庄（Jigsaw）
    └── WoodlandMansionStructure.*     # 林地府邸（程序化房间布局）
```

## 内部模块关系

```
┌─────────────────────────────────────────────────────────────────┐
│                      StructureManager                            │
│              (注册、查询、协调所有结构的生成)                       │
└───────────────────────────┬─────────────────────────────────────┘
                            │
    ┌───────────────────────┼───────────────────────┐
    │                       │                       │
    ▼                       ▼                       ▼
┌─────────────┐      ┌─────────────┐      ┌─────────────────┐
│  Structure  │      │JigsawStruct │      │StructureBounding│
│   (基类)    │      │   (基类)    │      │     Box         │
└──────┬──────┘      └──────┬──────┘      └─────────────────┘
       │                    │
       │                    │
       ▼                    ▼
┌──────────────────────────────────────────────────────────────┐
│                      structures/                              │
│  ┌──────────────────┐  ┌──────────────────┐                  │
│  │ 程序化结构        │  │ 模板化结构        │                  │
│  │ - Mineshaft      │  │ - Shipwreck      │                  │
│  │ - OceanMonument  │  │ - Igloo          │                  │
│  │ - Stronghold     │  │ - RuinedPortal   │                  │
│  │ - DesertPyramid  │  │ - OceanRuin      │                  │
│  │ - JungleTemple   │  │ - NetherFossil   │                  │
│  └──────────────────┘  └──────────────────┘                  │
│                                                               │
│  ┌──────────────────┐  ┌──────────────────┐                  │
│  │ Jigsaw 结构      │  │ 递归结构          │                  │
│  │ - Village        │  │ - EndCity        │                  │
│  │ - BastionRemnant │  │ - WoodlandMansion│                  │
│  │ - PillagerOutpost│  │                  │                  │
│  │ - TrialChambers  │  │                  │                  │
│  └──────────────────┘  └──────────────────┘                  │
└──────────────────────────────────────────────────────────────┘
                            │
                            ▼
                  ┌─────────────────┐
                  │     pools/      │
                  │  Jigsaw 模板池   │
                  └─────────────────┘
```

## 上下游外部依赖关系

### 上游依赖（本模块依赖）

| 模块 | 用途 |
|------|------|
| `world/gen/jigsaw/` | Jigsaw 管理器、模板池、拼图块 |
| `world/gen/feature/template/` | NBT 模板加载和处理器 |
| `world/gen/chunk/IChunkGenerator.hpp` | 区块生成器接口（高度、生物群系查询） |
| `world/block/VanillaBlocks.hpp` | 方块状态注册表 |
| `world/IWorldWriter.hpp` | 世界写入接口 |
| `world/biome/Biome.hpp` | 生物群系定义 |
| `util/math/random/Random.hpp` | 随机数生成 |
| `resource/ResourceLocation.hpp` | 资源定位符 |

### 下游依赖（依赖本模块）

| 模块 | 用途 |
|------|------|
| `world/gen/chunk/NoiseChunkGenerator` | 区块生成器在 FEATURES 阶段调用结构生成 |
| `server/world/ServerWorld` | 世界初始化时调用 `StructureRegistry::initialize()` |

## 容易踩的坑

### 1. 结构种子计算必须与 MC 1.16.5 一致

结构位置由 `spacing`、`separation`、`salt` 三个参数决定。种子计算公式错误会导致结构位置不一致。

```cpp
// 正确的种子计算方式
i64 combinedSeed = worldSeed ^ (chunkX * 341873128712LL) ^ (chunkZ * 132897987541LL) + salt;
```

### 2. 跨区块结构片段必须使用 `placeInChunk`

大型结构（要塞、海洋纪念碑等）跨多个区块，只能在起点区块调用 `generate()`，其他相关区块调用 `placeInChunk()`。若直接在所有区块调用 `generate()` 会导致重复生成。

### 3. 模板化结构必须先设置 TemplateManager

`IglooStructure`、`ShipwreckStructure`、`RuinedPortalStructure` 等依赖 NBT 模板的结构，必须在生成前通过 `setTemplateManager()` 注入模板管理器，否则模板加载会失败。

### 4. 结构生成时机与阶段顺序

结构生成在 `ChunkStatus::STRUCTURE_STARTS` 和 `STRUCTURE_REFERENCES` 阶段计算起点，在 `FEATURES` 阶段实际放置方块。若过早直接写入世界，会被后续地形阶段覆盖。

### 5. 生物群系检查遗漏

每个结构必须正确实现 `validBiomes()` 和 `canGenerate()`，否则结构会生成在错误的生物群系中。

### 6. Jigsaw 模板池注册顺序

Jigsaw 结构（村庄、堡垒遗迹等）的起始模板池必须在 `StructureRegistry::initialize()` 中先于结构注册，否则生成时会崩溃或生成空结构。

### 7. 结构间距参数配置

`spacing` 必须大于 `separation`，否则结构位置计算会出错。参考 MC 1.16.5 原版参数。

### 8. `StructureBoundingBox` 边界包含性

边界框的 `max` 坐标是包含边界，计算交集时需注意：
```cpp
// 正确的区块相交检测
return m_maxX >= chunkMinX && m_minX <= chunkMaxX &&
       m_maxZ >= chunkMinZ && m_minZ <= chunkMaxZ;
```

### 9. 海洋纪念碑的房间图系统

`OceanMonumentStructure` 使用 `OceanMonumentPieces` 的房间图系统，`RoomDefinition`、房间匹配 helper 和 Elder Guardian 生成入口已落地，但 `generate()` 的生命周期、朝向随机化、room claim 顺序仍需与 Java 版对齐。

### 10. 要塞的递归片段系统

`StrongholdStructure` 使用 `StrongholdPieces` 片段系统，`PieceWeight`、门类型、石砖变体已落地，但 `canGenerate()` 当前恒为 true，未按 MC 1.16.5 强要塞位置分布判定，且递归驱动逻辑与原版有偏差。

### 11. 区块访问窗口越界

`WorldGenRegion` 使用阶段特定的 `ChunkStatus::taskRange()` 窗口，请求的区块缺失会触发断言。结构生成代码必须确保在正确的阶段访问正确范围内的区块。

### 12. 仅在 StructureType 增加枚举但未注册

若只在 `StructureType` 枚举增加新类型但未在 `StructureRegistry::initialize()` 注册，运行时该结构不会生成。
