# 世界生成系统 (World Generation System)

本目录实现了 Minecraft 1.16.5 的完整世界生成系统，包括地形生成、生物群系分布、结构生成、特征放置等核心功能。

## 目录结构

```
gen/
├── aquifer/                     # 含水层系统
│   └── Aquifer.hpp/cpp          # 地下水体生成
├── carver/                      # 雕刻器系统
│   ├── WorldCarver.hpp/cpp      # 雕刻器基类
│   ├── CaveCarver.hpp/cpp       # 洞穴雕刻器
│   ├── CanyonCarver.hpp/cpp     # 峡谷雕刻器
│   ├── UnderwaterCarver.hpp/cpp # 水下雕刻器
│   ├── NetherCaveCarver.hpp/cpp # 下界洞穴雕刻器
│   ├── CarvingMask.hpp/cpp      # 雕刻掩码（防止重复雕刻）
│   ├── CarvingContext.hpp/cpp   # 雕刻上下文
│   └── Carvers.hpp              # 雕刻器常量
├── chunk/                       # 区块生成器
│   ├── IChunkGenerator.hpp/cpp  # 区块生成器接口
│   ├── NoiseChunkGenerator.hpp/cpp # 主世界/下界噪声生成器
│   ├── NetherChunkGenerator.hpp/cpp # 下界专用生成器
│   ├── EndChunkGenerator.hpp/cpp    # 末地专用生成器
│   └── DebugChunkGenerator.hpp/cpp  # 调试平坦生成器
├── density/                     # 密度函数系统
│   ├── DensityFunction.hpp      # 密度函数接口
│   ├── DensityFunctions.hpp/cpp # 密度函数实现
│   ├── NoiseChunk.hpp/cpp       # 噪声区块数据
│   ├── NoiseRouter.hpp/cpp      # 噪声路由器
│   ├── NoiseRouterData.hpp/cpp  # 噪声路由数据
│   └── README.md
├── feature/                     # 特征系统
│   ├── Feature.hpp/cpp          # 特征基类
│   ├── ConfiguredFeature.hpp/cpp # 配置化特征
│   ├── FeatureSpread.hpp/cpp    # 特征扩散配置
│   ├── DecorationStage.hpp      # 装饰阶段枚举
│   ├── FeatureIds.hpp           # 特征ID常量
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
├── jigsaw/                      # Jigsaw 结构组装系统
│   ├── JigsawManager.hpp/cpp    # BFS 组装算法
│   ├── JigsawPattern.hpp/cpp    # 模板池
│   ├── JigsawPiece.hpp/cpp      # 拼图块
│   └── JigsawJunction.hpp       # 连接点信息
├── noise/                       # 噪声生成器
│   ├── INoiseGenerator.hpp      # 噪声接口
│   ├── Noise.hpp                # 噪声常量
│   ├── ImprovedNoiseGenerator.hpp/cpp # Perlin 噪声
│   └── OctavesNoiseGenerator.hpp/cpp  # 多倍频噪声
├── placement/                   # 放置器系统
│   ├── Placement.hpp/cpp        # 放置器基类
│   ├── PlacementRegistry.hpp/cpp # 放置器注册
│   ├── Placements.hpp/cpp       # 放置器实现（13种）
│   └── PlacementUtils.hpp/cpp   # 放置器工具
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
└── surface/                     # 地表构建器
    ├── Surface.hpp              # 地表类型枚举
    ├── SurfaceBuilder.hpp       # 地表构建器基类
    └── SurfaceBuilders.hpp/cpp  # 12种地表构建器实现
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
│ 结构生成系统   │───────│ Jigsaw组装    │       │ 地表构建器    │
│ (村庄/神殿等)  │       │ (村庄组装算法) │       │ (草地/沙子等) │
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

相同种子应产生相同的世界，但噪声参数不同会导致不一致。

```cpp
// 错误：每次调用创建新的随机数
f32 value = ImprovedNoiseGenerator(Random(seed)).noise(x, y, z);

// 正确：复用噪声生成器
ImprovedNoiseGenerator noise(seed);
f32 value = noise.noise(x, y, z);
```

### 2. 区块边界处理

特征生成跨越区块边界时可能导致截断。`WorldGenRegion` 提供邻居区块访问，但必须在正确的生成阶段访问正确范围内的区块。

### 3. 结构生成顺序

结构依赖于特征生成的结果，必须严格按照 `DecorationStage` 顺序生成。

### 4. 雕刻掩码状态

雕刻掩码在多次雕刻调用间应保持状态，防止重复雕刻同一位置。

### 5. 生物群系缓存

生物群系数据在 `generateBiomes` 阶段完成后应只读，后续阶段不应修改。

### 6. 模板加载时机

Jigsaw 模板在方块注册前加载会失败。初始化顺序：方块 → 生物群系 → Jigsaw模板池 → 特征。

### 7. 高度图更新

生成方块后必须更新高度图，否则后续生成会出错。`ChunkPrimer` 中设置方块会自动更新高度图。

### 8. WorldGenRegion 不是 IBlockReader

`WorldGenRegion` 不是 `IBlockReader`，直接传递给 `Block::isValidPosition` 会导致编译错误。在世界生成特征中，使用显式的本地放置检查（`isWater`、支撑方块检查）。

### 9. 区块访问窗口越界

`WorldGenRegion` 使用阶段特定的 `ChunkStatus::taskRange()` 窗口，如果请求的区块缺失，`getTopBlockY()` 会触发断言。不要将窗口外的高度查询视为"高度 0"；应修复区域半径或调用点。

### 10. 临时 BlockState 副本

将临时 `BlockState` 副本传递给世界写入 API 可能导致状态 ID 不一致。优先使用 `state.with(...)` / `defaultState()` 返回的规范引用。

### 11. 海洋特征注册顺序

`KelpFeatureIds` 和 `SeagrassFeatureIds` 按海洋温度分开，添加新海洋变体时必须保持 `FeatureRegistry::initialize()` 顺序、`BiomeGenerationSettings` 映射和海洋断言同步。

### 12. 区块生成阶段与访问范围

每个 `ChunkStatus` 阶段有对应的 `taskRange()` 定义需要访问的邻居区块范围。`FEATURES`、`NOISE` 等阶段使用更大的动态方阵窗口，越界或缺失 chunk 会在热路径上断言失败。生成代码必须确保在正确的阶段访问正确范围内的区块。
