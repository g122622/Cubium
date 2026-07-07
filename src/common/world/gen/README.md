# 世界生成系统 (World Generation System)

本目录实现了 Minecraft 1.21 的完整世界生成系统，包括地形生成、生物群系分布、结构生成、特征放置等核心功能。所有维度（主世界、下界、末地）统一使用 NoiseChunkGenerator + 密度函数管线。

## 目录结构

```
gen/
├── aquifer/                     # 含水层系统
│   ├── Aquifer.hpp/cpp          # 含水层抽象基类（含工厂方法）
│   ├── Aquifers.hpp             # 便捷包含头文件
│   ├── DisabledAquifer.hpp/cpp  # 禁用含水层的空实现
│   ├── FluidPickerFactory.hpp/cpp # 流体选择器工厂函数
│   ├── FluidStatus.hpp/cpp      # 流体状态记录 + FluidPicker 类型别名
│   └── NoiseBasedAquifer.hpp/cpp # 基于噪声的含水层实现
├── carver/                      # 雕刻器系统
│   ├── WorldCarver.hpp/cpp      # 雕刻器基类
│   ├── CaveCarver.hpp/cpp       # 洞穴雕刻器
│   ├── CanyonCarver.hpp/cpp     # 峡谷雕刻器
│   ├── NetherWorldCarver.hpp/cpp # 下界雕刻器
│   ├── CarvingMask.hpp/cpp      # 雕刻掩码（防止重复雕刻）
│   ├── CarvingContext.hpp        # 雕刻上下文
│   ├── CarverConfiguration.hpp/cpp # 配置结构体 + 工厂函数
│   └── Carvers.hpp              # 便捷包含头文件
├── chunk/                       # 区块生成器
│   ├── IChunkGenerator.hpp/cpp  # 区块生成器接口
│   ├── NoiseChunkGenerator.hpp/cpp # 统一噪声生成器（主世界/下界/末地）
│   └── DebugChunkGenerator.hpp/cpp # 调试平坦生成器
├── density/                     # 密度函数系统
│   ├── DensityFunction.hpp      # 密度函数接口
│   ├── DensityFunctions.hpp/cpp # 密度函数实现（含 BlendedNoise、EndIslands）
│   ├── BlendedNoise.hpp/cpp     # MC 1.18+ 混合噪声密度函数
│   ├── NoiseChunk.hpp/cpp       # 噪声区块数据
│   ├── NoiseRouter.hpp/cpp      # 噪声路由器
│   ├── NoiseRouterData.hpp/cpp  # 噪声路由数据
│   └── README.md
├── feature/                     # 特征系统
│   ├── Feature.hpp/cpp          # 特征基类
│   ├── FeaturePlacer.hpp/cpp    # 按需放置特征（从 ServerWorld 已加载区块构建 WorldGenRegion）
│   ├── ConfiguredFeature.hpp/cpp # 配置化特征基类 + ConfiguredFeatureRegistry
│   ├── ConfiguredFeatureLoader.hpp/cpp # 数据驱动 configured_feature JSON 加载器
│   ├── FeatureTypeRegistry.hpp/cpp # feature type 字符串→C++ 工厂映射
│   ├── FeatureSorter.hpp/cpp    # MC 1.21 特征拓扑排序器
│   ├── MonsterRoomFeature.hpp/cpp # 地牢特征（复刻 MC 1.21.11）
│   ├── FeatureSpread.hpp/cpp    # 特征扩散配置
│   ├── DecorationStage.hpp      # 装饰阶段枚举
│   ├── cave/                    # 洞穴特征（滴水石、繁茂洞穴）
│   ├── fungus/                  # 下界巨型菌类特征
│   ├── gateway/                 # 末地折跃门特征
│   ├── lake/                    # 湖泊特征
│   ├── nether/                  # 下界特征（萤石、玄武岩、岩浆）
│   ├── ocean/                   # 海洋特征（海带、海草、珊瑚、蓝冰）
│   ├── ore/                     # 矿石特征
│   ├── predicate/               # 方块谓词（特征放置条件判断）
│   ├── spike/                   # 末地黑曜石柱特征
│   ├── state/                   # 方块状态提供器
│   ├── template/                # 结构模板系统（NBT模板加载）
│   ├── tree/                    # 树木生成
│   │   ├── foliage/             # 树叶放置器（9种）
│   │   └── trunk/               # 树干放置器（多种）
│   └── vegetation/              # 植被特征（花卉、草丛、蘑菇）
├── jigsaw/                      # Jigsaw 结构组装系统（详见 jigsaw/README.md）
│   ├── JigsawAssembler.hpp/cpp  # 组装算法（优先级队列/PoolAliasBinding/VoxelShape 空间追踪）
│   ├── JigsawPlacer.hpp/cpp     # 放置器（virtual place() 多态分发 + 回退方块）
│   ├── JigsawTransform.hpp/cpp  # 坐标/连接点变换、边界框计算
│   ├── JigsawPiece.hpp/cpp      # 拼图块基类（virtual place()）
│   ├── SingleJigsawPiece.hpp/cpp # 单模板拼图块 + LegacySingleJigsawPiece
│   ├── ListJigsawPiece.hpp/cpp  # 列表拼图块（递归放置子块）
│   ├── FeatureJigsawPiece.hpp/cpp # 地物拼图块（调用 ConfiguredFeature）
│   ├── EmptyJigsawPiece.hpp/cpp # 空拼图块
│   ├── TemplatePool.hpp/cpp     # 模板池（权重随机选择）
│   ├── TemplatePoolRegistry.hpp/cpp # 模板池注册表
│   ├── TemplatePoolLoader.hpp/cpp # 模板池 JSON 加载器
│   ├── AssemblyTypes.hpp        # PlacedPiece + PendingJoint
│   ├── JigsawTypes.hpp          # 枚举 + JigsawJoint + MaxDistance
│   ├── JigsawMatcher.hpp        # 连接点匹配工具
│   ├── JigsawOrientation.hpp    # Jigsaw 方向枚举
│   ├── JigsawJunction.hpp       # 连接点交叉数据结构
│   ├── SequencedPriorityIterator.hpp # 按 placementPriority 降序迭代器
│   ├── PoolAliasBinding.hpp/cpp # 池别名绑定
│   ├── PoolAliasLookup.hpp      # 池别名预解析查找表
│   ├── ProcessorListRegistry.hpp/cpp # 处理器列表注册表
│   ├── ProcessorListLoader.hpp/cpp # 处理器列表 JSON 加载器
│   └── JigsawLoaderUtils.hpp    # 加载器共用工具
├── noise/                       # MC 1.18+ 噪声生成器
│   ├── PerlinNoise.hpp/cpp      # 多倍频 Perlin 噪声
│   ├── NormalNoise.hpp/cpp      # 双 Perlin 噪声（地形生成核心）
│   ├── SimplexNoise.hpp/cpp     # Simplex 噪声（末地岛屿）
│   ├── Noise.hpp                # 统一头文件
│   └── README.md
├── placement/                   # 放置器系统
│   ├── Placement.hpp/cpp        # 放置器基类
│   ├── PlacementRegistry.hpp/cpp # 放置器注册（type 字符串→工厂）
│   ├── Placements.hpp/cpp       # 放置器实现（13种）
│   ├── PlacementUtils.hpp/cpp   # 放置器工具
│   ├── PlacedFeature.hpp/cpp    # placed_feature（配置化特征 + 放置链）
│   ├── PlacedFeatureRegistry.hpp/cpp # placed_feature 注册表（数据驱动）
│   └── PlacedFeatureLoader.hpp/cpp # 数据驱动 placed_feature JSON 加载器
├── settings/                    # 生成设置配置
│   ├── DimensionSettings.hpp/cpp # 维度设置（主世界/下界/末地）
│   ├── NoiseSettings.hpp        # 噪声参数
│   ├── ScalingSettings.hpp      # 缩放设置
│   ├── SlideSettings.hpp        # 滑动设置
│   └── Settings.hpp             # 综合设置
├── spawn/                       # 区块生成时的生物放置
│   └── WorldGenSpawner.hpp/cpp  # 初始动物生成
├── structure/                   # 结构生成系统
│   ├── Structure.hpp/cpp        # 结构基类
│   ├── StructureBoundingBox.hpp # 结构边界
│   ├── StructureManager.hpp/cpp # 结构管理器
│   ├── JigsawStructure.hpp/cpp  # Jigsaw 结构
│   ├── pools/                   # 结构模板池
│   │   ├── Pools.hpp/cpp        # 池注册
│   │   ├── ProcessorLists.hpp/cpp # 处理器列表
│   │   ├── village/             # 村庄模板池
│   │   ├── pillager_outpost/    # 掠夺者前哨站模板池
│   │   ├── bastion/             # 堡垒遗迹模板池
│   │   └── trial_chambers/      # 试炼密室模板池
│   └── structures/              # 具体结构实现（16种）
│       ├── VillageStructure.hpp/cpp
│       ├── MineshaftStructure.hpp/cpp
│       ├── StrongholdStructure.hpp/cpp
│       ├── OceanMonumentStructure.hpp/cpp
│       ├── BastionRemnantStructure.hpp/cpp
│       ├── EndCityStructure.hpp/cpp
│       ├── FortressStructure.hpp/cpp
│       └── ...（其他结构）
├── surface/                     # MC 1.21 地表规则系统
│   ├── Surface.hpp              # 聚合头文件
│   ├── SurfaceRules.hpp         # SurfaceRules 条件/规则系统
│   ├── SurfaceRules.cpp         # SurfaceRules 实现
│   └── README.md
└── valueprovider/               # 值提供器
```

## 内部模块关系

```
┌─────────────────────────────────────────────────────────────────────┐
│                        IChunkGenerator                               │
│               (区块生成器接口，协调所有子系统)                         │
└───────────────────────────────┬─────────────────────────────────────┘
                                │
    ┌───────────────────────────┼───────────────────────────┐
    │                           │                           │
    ▼                           ▼                           ▼
┌─────────┐              ┌───────────┐               ┌───────────┐
│  noise/ │──────────────│  density/ │              │ settings/ │
│噪声生成 │              │ 密度函数  │               │ 生成设置  │
└────┬────┘              └─────┬─────┘               └───────────┘
     │                         │
     │    ┌────────────────────┴────────────────────┐
     │    │                                         │
     ▼    ▼                                         ▼
┌─────────────────┐                         ┌─────────────┐
│   carver/       │                         │   aquifer/  │
│ 雕刻器（洞穴/峡谷）│                        │  含水层系统  │
└─────────────────┘                         └─────────────┘
                                                     │
┌─────────────────────────────────────────────────────┴───────────────┐
│                           feature/                                   │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌──────────┐            │
│  │   tree/  │  │   ore/   │  │ ocean/   │  │ vegetation/│           │
│  │ 树木生成  │  │ 矿石特征 │  │ 海洋特征  │  │  植被特征  │            │
│  └────┬─────┘  └──────────┘  └──────────┘  └───────────┘            │
│       │                                                              │
│       │  ┌────────────────┐  ┌────────────────┐                     │
│       └──│ trunk/         │  │ foliage/       │                     │
│          │ 树干放置器      │  │ 树叶放置器      │                     │
│          └────────────────┘  └────────────────┘                     │
│                                                                      │
│  ┌──────────────┐  ┌───────────────┐  ┌──────────────────┐         │
│  │  template/   │  │  predicate/   │  │     state/       │         │
│  │ NBT模板加载   │  │ 方块谓词判断   │  │ 方块状态提供器   │          │
│  └──────────────┘  └───────────────┘  └──────────────────┘         │
└──────────────────────────────────────────────────────────────────────┘
                                │
                                ▼
                    ┌───────────────────────┐
                    │      placement/       │
                    │ 放置器（位置选择逻辑） │
                    └───────────────────────┘
                                │
        ┌───────────────────────┼───────────────────────┐
        │                       │                       │
        ▼                       ▼                       ▼
┌───────────────┐       ┌───────────────┐       ┌───────────────┐
│   structure/  │       │    jigsaw/    │       │    surface/   │
│ 结构生成系统   │───────│ Jigsaw组装    │       │ SurfaceRules  │
│ (村庄/神殿等)  │       │ (村庄组装算法) │       │ (地表规则树)  │
└───────────────┘       └───────────────┘       └───────────────┘
        │
        ▼
┌───────────────┐
│  spawn/       │
│ 初始生物放置   │
└───────────────┘
```

## 上下游外部依赖关系

### 上游依赖（本模块依赖）

| 模块 | 用途 |
|------|------|
| `biome/` | 生物群系定义、生物群系提供者 |
| `block/` | 方块状态、方块注册表 |
| `chunk/` | 区块数据结构、ChunkPrimer |
| `entity/` | 实体类型（生物放置） |
| `resource/` | 资源加载（结构模板） |
| `util/math/` | 随机数、数学工具 |
| `util/nbt/` | NBT 解析（结构模板） |

### 下游依赖（依赖本模块）

| 模块 | 用途 |
|------|------|
| `server/world/` | ServerWorld 调用生成器生成区块 |
| `server/chunk/` | ServerChunkManager 管理区块加载和生成 |

## 容易踩的坑

### 1. 噪声种子一致性

相同种子应产生相同的世界。使用 `PositionalRandomFactory` 和固定种子偏移（如 `seed ^ 0x11111111ULL`）确保不同噪声通道使用不同随机序列。

### 2. 区块边界处理

特征生成跨越区块边界时可能导致截断。`WorldGenRegion` 提供邻居区块访问，但必须在正确的生成阶段访问正确范围内的区块。

### 3. 结构生成顺序

结构依赖于特征生成的结果，必须严格按照 `DecorationStage` 顺序生成。

### 4. 雕刻掩码状态

雕刻掩码在多次雕刻调用间应保持状态，防止重复雕刻同一位置。

### 5. 生物群系缓存

生物群系数据在 `generateBiomes` 阶段完成后应只读，后续阶段不应修改。

### 6. 模板加载时机

Jigsaw 模板在方块注册前加载会失败。初始化顺序（`MinecraftServer::initializeRegistries`）：方块 → `PlacementRegistry::initialize` → `FeatureTypeRegistry::initializeBuiltinFeatureTypes` → `ConfiguredFeatureLoader` → `PlacedFeatureLoader` → `ConfiguredCarverLoader` → `BiomeRegistry::initialize` → `BiomeLoader`。各 Loader 从数据包 JSON 加载对应注册表，遇未实现 type 或未注册引用时严格报错中断。

### 7. 高度图更新

生成方块后必须更新高度图，否则后续生成会出错。`ChunkPrimer` 中设置方块会自动更新高度图。

### 8. WorldGenRegion 不是 IBlockReader

`WorldGenRegion` 不是 `IBlockReader`，直接传递给 `Block::isValidPosition` 会导致编译错误。在世界生成特征中，使用显式的本地放置检查（`isWater`、支撑方块检查）。

### 9. WorldGenRegion 维度感知高度

`WorldGenRegion` 通过构造函数接收 `DimensionId` 参数，覆写 `IWorld::getMinBuildHeight()` 和 `IWorld::getMaxBuildHeight()` 返回维度特定的建筑高度范围（主世界 -64~320，下界 0~128，末地 0~256）。`dimension()` 方法也基于此参数返回正确的维度 ID。`ServerChunkManager` 在创建 `WorldGenRegion` 时从 `ServerWorld::dimension()` 传入维度 ID。`isWithinWorldBounds()` 使用这些虚方法进行 Y 坐标范围检查。

### 10. 区块访问窗口越界

`WorldGenRegion` 使用当前 `ChunkStep` 的累积依赖窗口，并按 `directDependencies()` 校验请求区块的阶段。越界、缺失或请求阶段超过该距离允许阶段时会触发断言；不要将窗口外查询视为"高度 0"，应修复区域半径或调用点。

### 11. 临时 BlockState 副本

将临时 `BlockState` 副本传递给世界写入 API 可能导致状态 ID 不一致。优先使用 `state.with(...)` / `defaultState()` 返回的规范引用。

### 12. 海洋特征数据驱动

海洋特征（海带/海草等）现由数据包 JSON 驱动（`configured_feature`/`placed_feature`），通过 `BiomeLoader` 写入各海洋生物群系的 `BiomeGenerationSettings`。特征 id 统一为 `ResourceLocation`（如 `minecraft:kelp`、`minecraft:seagrass`），不再有 `KelpFeatureIds`/`SeagrassFeatureIds` 整型常量。添加新海洋变体时，在数据包 JSON 中配置并通过 biome 的 `features` 数组引用即可。

### 13. FeaturePlacer 按需放置

`FeaturePlacer` 用于在游戏逻辑（如树苗生长、骨粉）中按需放置特征。它从 `ServerWorld` 的已加载区块（`ChunkData*` → `IChunk*`）构建临时的 `WorldGenRegion`，使需要 `WorldGenRegion&` 的特征放置函数（如 `TreeFeature::place()`）可以在区块生成之外被调用。

**注意**：按需构建的 `WorldGenRegion` 使用无步骤验证构造函数（`m_generatingStep=nullptr`），`setBlockState` 的访问窗口检查被跳过，`ensureCanWrite()` 返回 false。对于树苗生长等场景，这是可接受的——周围的区块已确认加载。此外，`WorldGenRegion` 中的 `dynamic_cast<ChunkPrimer*>` 对 `ChunkData*` 会返回 nullptr，方块实体管理和液体后处理会被跳过。

### 13. 区块生成阶段与访问范围

每个 `ChunkStatus` 阶段通过 `ChunkStep` 定义需要访问的邻居区块范围和各距离允许的依赖状态。`FEATURES`、`NOISE` 等阶段使用更大的动态方阵窗口，越界、缺失或请求状态不匹配会在热路径上断言失败。生成代码必须确保在正确的阶段访问正确范围内、正确状态的区块。
