# 世界生成系统 (World Generation System)

本目录实现了 Minecraft 1.16.5 的完整世界生成系统，包括地形生成、生物群系分布、结构生成、特征放置等核心功能。

## 目录结构

```
gen/
├── carver/              # 雕刻器系统（洞穴、峡谷）
├── chunk/               # 区块生成器
├── feature/             # 特征系统（树木、矿石、植被等）
│   ├── lake/            # 湖泊特征
│   ├── ocean/           # 海洋特征（海带、海草、海泡菜、珊瑚）
│   ├── ore/             # 矿石特征
│   ├── nether/          # 下界特征（萤石/玄武岩/岩浆/火焰）
│   ├── fungus/          # 下界巨型菌类特征
│   ├── spike/           # 末地黑曜石柱特征
│   ├── gateway/         # 末地折跃门特征
│   ├── template/        # 结构模板系统
│   ├── tree/            # 树木生成
│   │   ├── foliage/     # 树叶放置器
│   │   └── trunk/       # 树干放置器
│   └── vegetation/      # 植被特征
├── jigsaw/              # Jigsaw 结构组装系统
├── noise/               # 噪声生成器
├── placement/           # 放置器系统
├── region/              # 世界生成区域（预留）
├── settings/            # 生成设置配置
├── spawn/               # 区块生成时的生物放置
├── structure/           # 结构生成系统
│   ├── pieces/          # 结构片段（预留）
│   └── structures/      # 具体结构实现
└── surface/             # 地表构建器
```

---

## 模块详解

### 1. 区块生成器 (`chunk/`)

#### IChunkGenerator.hpp - 区块生成器接口

定义了区块生成的核心接口 `IChunkGenerator`，包含以下生成阶段：

| 方法 | 说明 |
|------|------|
| `generateStructureStarts()` | 生成结构起点 |
| `generateStructureReferences()` | 生成结构引用 |
| `generateBiomes()` | 生成生物群系数据 |
| `generateNoise()` | 生成噪声地形 |
| `buildSurface()` | 构建地表层 |
| `applyCarvers()` | 应用雕刻器 |
| `placeFeatures()` | 放置特征 |
| `spawnInitialMobs()` | 生成初始生物 |

`WorldGenRegion` 提供有限的世界视图，访问范围由当前生成阶段的 `ChunkStatus::taskRange()` 决定；`FEATURES`、`NOISE` 等阶段会使用更大的动态方阵窗口，越界或缺失 chunk 会在热路径上断言失败。

#### NoiseChunkGenerator.hpp - 噪声区块生成器

实现主世界和下界的标准地形生成器，使用多层噪声生成地形。

**核心噪声生成器：**
- `m_mainDensityNoise` - 主密度噪声（16倍频）
- `m_secondaryDensityNoise` - 次密度噪声（16倍频）
- `m_weightNoise` - 权重噪声（8倍频）
- `m_surfaceDepthNoise` - 地表深度噪声

**使用方法：**
```cpp
DimensionSettings settings = DimensionSettings::overworld();
NoiseChunkGenerator generator(seed, std::move(settings));

ChunkPrimer primer(chunkX, chunkZ);
generator.generateBiomes(region, primer);
generator.generateNoise(region, primer);
generator.buildSurface(region, primer);
```

---

### 2. 噪声生成器 (`noise/`)

#### INoiseGenerator.hpp - 噪声生成器接口

定义噪声生成器的基础接口：
```cpp
class INoiseGenerator {
public:
    virtual f32 noise(f32 x, f32 y, f32 z) const = 0;
    virtual f32 noise2D(f32 x, f32 z) const;
};
```

#### ImprovedNoiseGenerator.hpp - 改进的 Perlin 噪声

实现标准的 3D Perlin 噪声，参考 MC 1.16.5 的 `ImprovedNoiseGenerator`。

**特性：**
- 256 字节排列表
- 8 个梯度向量方向
- 输出范围约 [-1, 1]

```cpp
ImprovedNoiseGenerator noise(seed);
// 创建生成区域（按阶段半径准备动态窗口）
std::vector<IChunk*> chunks = {...};
WorldGenRegion region(chunkX, chunkZ, radius, std::move(chunks));

#### OctavesNoiseGenerator.hpp - 多倍频噪声

组合多个噪声层，每层频率和振幅递增，产生更自然的地形。

---

### 3. 雕刻器系统 (`carver/`)

#### WorldCarver.hpp - 雕刻器基类

定义雕刻器的通用接口和工具方法：

```cpp
template<typename Config>
class WorldCarver {
public:
    virtual bool carve(ChunkPrimer& chunk, ...) = 0;
    virtual bool shouldCarve(...) const = 0;
    
    // 工具方法
    static bool isCarvable(const BlockState& state);
    bool carveEllipsoid(...);
};
```

**雕刻掩码 `CarvingMask`：** 追踪哪些位置已被雕刻，防止重复雕刻。

#### CaveCarver.hpp - 洞穴雕刻器

生成洞穴系统，包含：
- 房间（大型球形空间）
- 隧道（弯曲的通道）

#### CanyonCarver.hpp - 峡谷雕刻器

生成峡谷地形，产生深长的裂谷。

#### UnderwaterCarver.hpp - 水下雕刻器

生成水下洞穴和峡谷，用于水下地形。

---

### 4. 特征系统 (`feature/`)

#### Feature.hpp - 特征基类

定义特征生成的基础接口和配置：

```cpp
// 方块匹配规则
class RuleTest {
public:
    virtual bool test(const BlockState& state, Random& random) const = 0;
};

// 矿石配置
struct OreFeatureConfig : public IFeatureConfig {
    std::unique_ptr<RuleTest> target;  // 目标方块规则
    const BlockState* state;            // 矿石方块
    i32 size;                           // 矿脉大小
};
```

#### ConfiguredFeature.hpp - 配置化特征

组合特征与其放置配置，支持统一的特征注册：

```cpp
class ConfiguredFeatureBase {
public:
    virtual bool place(WorldGenRegion& region, ChunkPrimer& chunk, 
                      IChunkGenerator& generator, Random& random,
                      const BlockPos& pos) = 0;
    virtual DecorationStage stage() const = 0;
};
```

**FeatureRegistry：** 管理所有配置化特征，按装饰阶段组织。

#### 海洋特征 (`feature/ocean/`)

海洋特征在 `VegetalDecoration` 阶段注册，负责补充海底生态：

- `KelpFeature`：生成海带柱（`kelp_plant` 主体 + `kelp` 顶部）。
- `SeagrassFeature`：放置普通海草与高海草（上下半状态）。
- `SeaPickleFeature`：在活珊瑚基底上放置 1-4 个海泡菜。
- `CoralFeature`：随机生成树形/蘑菇形/爪形珊瑚结构并附带扇状装饰。
- `OceanDecorationFeature`：补充潮涌核心、干海带块、海龟蛋、气泡柱、海晶石楼梯/台阶等海洋装饰，并已挂接到非暖水海洋 biome 的生成设置中。

#### 树木生成 (`feature/tree/`)

##### TrunkPlacer.hpp - 树干放置器

负责生成树干，返回树叶位置信息：

```cpp
class TrunkPlacer {
public:
    virtual std::vector<FoliagePosition> placeTrunk(
        WorldGenRegion& world, Random& random, i32 height,
        const BlockPos& startPos, std::set<BlockPos>& trunkBlocks,
        const BlockState* trunkBlock) = 0;
};
```

**已实现的树干放置器（6种）：**
| 类名 | 用途 |
|------|------|
| `StraightTrunkPlacer` | 简单垂直树干（橡树、白桦、云杉、丛林木） |
| `DarkOakTrunkPlacer` | 2x2 树干（深色橡树） |
| `FancyTrunkPlacer` | 弯曲树干（大型橡树） |
| `ForkyTrunkPlacer` | 分叉树干（金合欢） |
| `GiantTrunkPlacer` | 巨型树干（巨型云杉） |
| `MegaJungleTrunkPlacer` | 巨型丛林木树干 |

##### FoliagePlacer.hpp - 树叶放置器

负责生成树叶：

```cpp
class FoliagePlacer {
public:
    void placeFoliage(WorldGenRegion& world, Random& random,
                      i32 trunkHeight, const std::vector<FoliagePosition>& positions,
                      const std::set<BlockPos>& trunkBlocks, i32 trunkOffset,
                      const BlockState* foliageBlock);
};
```

**已实现的树叶放置器（9种）：**
| 类名 | 用途 |
|------|------|
| `BlobFoliagePlacer` | 球形树叶（橡树、白桦） |
| `PineFoliagePlacer` | 锥形树叶（松树） |
| `SpruceFoliagePlacer` | 尖锥形树叶（云杉） |
| `AcaciaFoliagePlacer` | 伞形树叶（金合欢） |
| `DarkOakFoliagePlacer` | 密集球形树叶（深色橡树） |
| `JungleFoliagePlacer` | 稀疏单层树叶（丛林木） |
| `MegaPineFoliagePlacer` | 大型锥形树叶（巨型云杉） |
| `BushFoliagePlacer` | 单层球形树叶（灌木） |
| `FancyFoliagePlacer` | 密集球形树叶（大型橡树） |

#### 矿石特征 (`feature/ore/`)

##### OreFeature.hpp - 矿石生成器

使用球形采样算法在石头中放置矿石：

```cpp
class OreFeature {
public:
    bool place(WorldGenRegion& region, ChunkPrimer& chunk,
               Random& random, const BlockPos& origin,
               const OreFeatureConfig& config);
};
```

**预定义矿石：**
- 主世界：煤矿、铁矿、金矿、红石矿、钻石矿、青金石矿、绿宝石矿、铜矿
- 下界：石英矿、下界金矿、远古残骸

#### 植被特征 (`feature/vegetation/`)

| 特征 | 说明 |
|------|------|
| `FlowerFeature` | 花卉生成 |
| `GrassFeature` | 草丛生成 |
| `BigMushroomFeature` | 巨型蘑菇生成 |
| `CactusFeature` | 仙人掌生成 |
| `SugarCaneFeature` | 甘蔗生成 |
| `IceSpikeFeature` | 冰刺生成 |

#### 模板系统 (`feature/template/`)

##### Template.hpp - 结构模板

从 NBT 文件加载的结构模板，用于 Jigsaw 结构生成。支持 MC 1.16.5 的多调色板（Palette）机制：

```cpp
class Template {
public:
    // 多调色板支持（结构变体）
    void addPalette(Palette palette);
    const Palette* selectPalette(math::Random& rng) const;
    size_t getPaletteCount() const;
    
    bool place(IWorldWriter& world, const BlockPos& pos,
               const PlacementSettings& settings, Random& rng, u32 flags) const;
    
    static BlockPos transformBlockPos(const BlockPos& pos, i32 mirror, 
                                      i32 rotation, const BlockPos& center);
};

// 调色板：存储一组方块，支持按类型快速查找
class Palette {
public:
    const std::vector<BlockInfo>& blocks() const;
    const std::vector<const BlockInfo*>& getBlocksByType(const Block& block) const;
};
```

---

### 5. Jigsaw 系统 (`jigsaw/`)

实现 MC 1.16.5 的 Jigsaw 结构组装算法，用于村庄等复杂结构的生成。

#### JigsawPiece.hpp - 拼图块

定义单个结构模板：

```cpp
class JigsawPiece {
public:
    virtual const String& getTypeName() const = 0;
    virtual std::unique_ptr<JigsawPiece> clone() const = 0;
    
    // 连接点
    const std::vector<JigsawJoint>& getJoints() const;
    void addJoint(const JigsawJoint& joint);
};

// 连接点信息
struct JigsawJoint {
    BlockPos sourcePos;      // 源位置
    String sourceName;       // 源连接点名称
    String targetPool;       // 目标模板池
    String targetName;       // 目标连接点名称
    JigsawPlacementBehaviour projection;
};
```

**拼图块类型：**
- `EmptyJigsawPiece` - 空拼图块
- `SingleJigsawPiece` - 单模板拼图块
- `ListJigsawPiece` - 列表拼图块

#### JigsawPattern.hpp - 模板池

管理一组相关联的拼图块及其权重：

```cpp
class JigsawPattern {
public:
    const JigsawPiece* getRandomPiece(Random& rng) const;
    std::vector<const JigsawPiece*> getShuffledPieces(Random& rng) const;  // MC 1.16.5 核心
    void addPiece(std::unique_ptr<JigsawPiece> piece, i32 weight = 1);
};
```

**选择方法**:
- `getRandomPiece` - 随机选择一个拼图块
- `getShuffledPieces` - 返回打乱后的完整列表，用于遍历尝试放置

#### JigsawManager.hpp - 结构组装器

实现 BFS 结构组装算法：

```cpp
class JigsawManager {
public:
    static std::vector<PlacedPiece> assemble(
        JigsawPatternRegistry& patternRegistry,
        const JigsawPattern& startPool,
        i32 maxDepth, const BlockPos& startPos, Random& rng);
};
```

**组装流程：**
1. 从起始模板池选择起始块
2. 遍历所有连接点
3. 从目标模板池选择匹配块
4. 递归组装直到达到最大深度或无更多连接点

---

### 6. 放置器系统 (`placement/`)

控制特征在世界中的放置位置。

#### Placement.hpp - 放置器基类

```cpp
class Placement {
public:
    virtual std::vector<BlockPos> getPositions(
        WorldGenRegion& region, Random& random,
        const IPlacementConfig& config, const BlockPos& basePos) const = 0;
};
```

**已实现的放置器（13种）：**

| 放置器 | 配置 | 说明 |
|--------|------|------|
| `CountPlacement` | `CountPlacementConfig` | 每区块放置多次 |
| `ChancePlacement` | `ChancePlacementConfig` | 概率放置 |
| `HeightRangePlacement` | `HeightRangePlacementConfig` | Y 坐标范围限制 |
| `BiomePlacement` | `BiomePlacementConfig` | 生物群系过滤 |
| `NoisePlacement` | `NoisePlacementConfig` | 噪声阈值 |
| `CountNoisePlacement` | `CountNoiseConfig` | 噪声控制数量 |
| `DepthAveragePlacement` | `DepthAverageConfig` | 基准深度附近 |
| `TopSolidPlacement` | `EmptyPlacementConfig` | 顶层固体方块 |
| `CarvingMaskPlacement` | `EmptyPlacementConfig` | 雕刻掩码位置 |
| `RandomOffsetPlacement` | `RandomOffsetConfig` | 随机偏移 |
| `WaterDepthThresholdPlacement` | `WaterDepthThresholdConfig` | 水深阈值 |
| `SeaLevelPlacement` | `SeaLevelConfig` | 海平面位置 |
| `SpreadPlacement` | `EmptyPlacementConfig` | 扩散放置 |
| `SquarePlacement` | `EmptyPlacementConfig` | 方形分散 |

**链式放置：**
```cpp
auto placement = std::make_unique<ConfiguredPlacement>(
    std::make_unique<CountPlacement>(),
    std::make_unique<CountPlacementConfig>(10)
)->then(
    std::make_unique<HeightRangePlacement>(),
    std::make_unique<HeightRangePlacementConfig>(10, 0, 50)
);
```

---

### 7. 结构系统 (`structure/`)

#### Structure.hpp - 结构基类

定义世界结构的通用接口：

```cpp
class Structure {
public:
    virtual const String& name() const = 0;
    virtual StructureSeparationSettings separationSettings() const = 0;
    virtual const std::vector<BiomeId>& validBiomes() const = 0;
    
    virtual bool canGenerate(IWorld& world, IChunkGenerator& generator,
                            Random& rng, i32 chunkX, i32 chunkZ);
    virtual std::unique_ptr<StructureStart> generate(...) const;
};
```

**结构间距设置：**
```cpp
struct StructureSeparationSettings {
    i32 spacing;     // 平均间距（区块）
    i32 separation;  // 最小间距（区块）
    i32 salt;        // 随机种子盐
};
```

#### 已实现的结构（10种）

| 结构 | 文件 | 说明 |
|------|------|------|
| `VillageStructure` | `structures/VillageStructure.hpp` | 村庄（平原、沙漠、热带草原、针叶林、雪地） |
| `MineshaftStructure` | `structures/MineshaftStructure.hpp` | 废弃矿井 |
| `StrongholdStructure` | `structures/StrongholdStructure.hpp` | 要塞 |
| `DesertPyramidStructure` | `structures/DesertPyramidStructure.hpp` | 沙漠神殿 |
| `JungleTempleStructure` | `structures/JungleTempleStructure.hpp` | 丛林神庙 |
| `OceanMonumentStructure` | `structures/OceanMonumentStructure.hpp` | 海洋纪念碑 |
| `RuinedPortalStructure` | `structures/RuinedPortalStructure.hpp` | 废弃传送门 |
| `BuriedTreasureStructure` | `structures/BuriedTreasureStructure.hpp` | 埋藏宝藏 |
| `ShipwreckStructure` | `structures/ShipwreckStructure.hpp` | 沉船 |
| `OceanRuinStructure` | `structures/OceanRuinStructure.hpp` | 海底废墟 |

---

### 8. 地表构建器 (`surface/`)

#### SurfaceBuilder.hpp - 地表构建器基类

```cpp
class SurfaceBuilder {
public:
    virtual void buildSurface(
        Random& random, ChunkPrimer& chunk, const Biome& biome,
        i32 x, i32 z, i32 startHeight, f32 surfaceNoise,
        const BlockState* defaultBlock, const BlockState* defaultFluid,
        i32 seaLevel, const SurfaceBuilderConfig& config) = 0;
};

struct SurfaceBuilderConfig {
    const BlockState* topBlock;       // 表层方块
    const BlockState* underBlock;     // 次表层方块
    const BlockState* underWaterBlock; // 水下表面方块
};
```

**已实现的地表构建器（12种）：**

| 构建器 | 说明 |
|--------|------|
| `DefaultSurfaceBuilder` | 标准草地/泥土表面 |
| `MountainSurfaceBuilder` | 高海拔石头和雪 |
| `DesertSurfaceBuilder` | 沙子和砂岩 |
| `SwampSurfaceBuilder` | 沼泽粘土斑块 |
| `FrozenOceanSurfaceBuilder` | 冻结海洋的冰层 |
| `BadlandsSurfaceBuilder` | 恶地的陶瓦和红沙 |
| `BeachSurfaceBuilder` | 水位线附近的沙子 |
| `GiantTreeTaigaSurfaceBuilder` | 大型针叶林的灰化土 |
| `ShatteredSavannaSurfaceBuilder` | 破碎热带草原的石头斑块 |
| `BambooJungleSurfaceBuilder` | 竹林的灰化土 |
| `NetherForestsSurfaceBuilder` | 下界森林的玄武岩 |
| `SoulSandValleySurfaceBuilder` | 灵魂沙谷的灵魂沙 |

---

### 9. 生成设置 (`settings/`)

#### DimensionSettings.hpp - 维度设置

```cpp
struct DimensionSettings {
    NoiseSettings noise;           // 噪声设置
    const BlockState* defaultBlock; // 默认方块（石头）
    const BlockState* defaultFluid; // 默认流体（水）
    i32 seaLevel;                  // 海平面高度
    i32 bedrockRoof;               // 基岩顶部
    i32 bedrockFloor;              // 基岩底部
    
    static DimensionSettings overworld();
    static DimensionSettings nether();
    static DimensionSettings end();
    static DimensionSettings flat();
};
```

#### NoiseSettings.hpp - 噪声设置

```cpp
struct NoiseSettings {
    i32 height = 256;               // 噪声高度
    i32 sizeHorizontal = 1;         // 水平大小
    i32 sizeVertical = 2;           // 垂直大小
    ScalingSettings scaling;        // 缩放设置
    SlideSettings topSlide;         // 顶部滑动
    SlideSettings bottomSlide;      // 底部滑动
    f32 densityFactor = 1.0f;       // 密度因子
    f32 densityOffset = -0.46875f;  // 密度偏移
    bool simplexSurfaceNoise;       // Simplex 地表噪声
    bool randomDensityOffset;       // 随机密度偏移
    bool isAmplified;               // 放大化
    
    static NoiseSettings overworld();
    static NoiseSettings amplified();
    static NoiseSettings nether();
    static NoiseSettings end();
};
```

---

### 10. 生物放置 (`spawn/`)

#### WorldGenSpawner.hpp - 区块生成时的生物放置

在区块首次生成时放置被动动物：

```cpp
class WorldGenSpawner {
public:
    i32 spawnInitialMobs(
        WorldGenRegion& region, const Biome& biome,
        i32 chunkX, i32 chunkZ, IChunkGenerator& generator,
        Random& random, std::vector<SpawnedEntityData>& outEntities);
};
```

**与 NaturalSpawner 的区别：**
- `WorldGenSpawner`: 区块生成时放置动物（仅 Creature 分类）
- `NaturalSpawner`: 运行时自然生成（怪物、动物、环境生物）

---

### 11. 装饰阶段 (`DecorationStage.hpp`)

定义特征生成的顺序阶段：

```cpp
enum class DecorationStage : u8 {
    RawGeneration = 0,      // 原始生成（基岩、岛屿）
    Lakes,                  // 湖泊
    LocalModifications,     // 局部修改
    UndergroundStructures,  // 地下结构
    SurfaceStructures,      // 地表结构
    Strongholds,            // 要塞
    UndergroundOres,        // 地下矿石
    UndergroundDecoration,  // 地下装饰
    VegetalDecoration,      // 植被装饰
    TopLayerModification,   // 顶层修改
};
```

---

## 整体职责

本模块负责 Minecraft 世界生成系统的核心实现：

1. **地形生成**：使用多层噪声算法生成三维地形
2. **生物群系分布**：基于层叠算法的生物群系分配
3. **结构生成**：村庄、要塞、神殿等大型结构
4. **特征放置**：树木、矿石、花草等小型特征
5. **雕刻处理**：洞穴、峡谷等地形雕刻
6. **地表构建**：根据生物群系生成地表层

---

## 输入和输出

### 输入

| 输入项 | 类型 | 来源 | 说明 |
|--------|------|------|------|
| 世界种子 | `u64` | 外部传入 | 确定世界生成的随机性 |
| 维度设置 | `DimensionSettings` | 预设或配置 | 噪声参数、方块类型等 |
| 生物群系数据 | `BiomeContainer` | BiomeProvider | 4x4x4 采样的生物群系 |
| 区块坐标 | `ChunkCoord` | 外部请求 | 要生成的区块位置 |
| 邻居区块 | `IChunk[]` | 区块管理器 | 生成所需的邻居数据 |

### 输出

| 输出项 | 类型 | 目标 | 说明 |
|--------|------|------|------|
| 区块数据 | `ChunkPrimer/ChunkData` | 区块管理器 | 完整的区块方块数据 |
| 高度图 | `Heightmap[]` | 区块数据 | 各类型高度图 |
| 实体数据 | `SpawnedEntityData[]` | 世界管理器 | 初始动物生成数据 |
| 结构引用 | `StructureStart[]` | 结构管理器 | 已生成的结构信息 |

---

## 依赖项

### 内部依赖

```text
gen/
├── depends on → biome/   # 生物群系定义
├── depends on → block/   # 方块状态
├── depends on → chunk/   # 区块数据结构
├── depends on → entity/  # 实体类型（生物放置）
├── depends on → resource/# 资源加载（结构模板）
└── depends on → util/    # 随机数、数学、断言等基础工具
```

### 外部依赖

- `glm` - 数学运算
- `spdlog` - 日志记录
- `nlohmann-json` - 配置和数据解析

---

## 使用方法

### 创建区块生成器

```cpp
#include "gen/chunk/NoiseChunkGenerator.hpp"
#include "gen/settings/DimensionSettings.hpp"

// 主世界生成器
DimensionSettings settings = DimensionSettings::overworld();
NoiseChunkGenerator generator(worldSeed, std::move(settings));

// 设置生物群系提供者
auto biomeProvider = std::make_unique<LayerBiomeProvider>(worldSeed);
generator.setBiomeProvider(std::move(biomeProvider));
```

### 生成区块

```cpp
// 创建区块 primer
ChunkPrimer primer(chunkX, chunkZ);
const i32 radius = 8;

// 创建生成区域（需要邻居区块）
std::vector<IChunk*> chunks = {...};
WorldGenRegion region(chunkX, chunkZ, radius, std::move(chunks));

// 按阶段生成
generator.generateBiomes(region, primer);
generator.generateNoise(region, primer);
generator.buildSurface(region, primer);
generator.applyCarvers(region, primer, false); // 空气雕刻
generator.applyCarvers(region, primer, true);  // 液体雕刻
generator.placeFeatures(region, primer);

// 生成初始动物
std::vector<SpawnedEntityData> entities;
generator.spawnInitialMobs(region, primer, entities);

// 转换为最终区块数据
std::unique_ptr<ChunkData> data = primer.toChunkData();
```

### 注册自定义特征

```cpp
// 创建特征配置
auto config = std::make_unique<TreeFeatureConfig>(
    Blocks::OAK_LOG->defaultState(),
    Blocks::OAK_LEAVES->defaultState(),
    std::make_unique<StraightTrunkPlacer>(4, 2, 0),
    std::make_unique<BlobFoliagePlacer>(2, 0)
);

// 创建放置配置
auto placement = std::make_unique<ConfiguredPlacement>(
    std::make_unique<CountPlacement>(),
    std::make_unique<CountPlacementConfig>(10)
);

// 注册特征
auto feature = std::make_unique<ConfiguredTreeFeature>(
    std::move(config), std::move(placement), "custom_tree"
);
FeatureRegistry::instance().registerFeature(
    std::move(feature), DecorationStage::VegetalDecoration
);
```

---

## 容易踩的坑

### 1. 噪声种子一致性

**问题**：相同种子应产生相同的世界，但噪声参数不同会导致不一致。

**解决**：确保所有噪声生成器使用相同的种子初始化方式。

```cpp
// 错误：每次调用创建新的随机数
f32 value = ImprovedNoiseGenerator(Random(seed)).noise(x, y, z);

// 正确：复用噪声生成器
ImprovedNoiseGenerator noise(seed);
f32 value = noise.noise(x, y, z);
```

### 2. 区块边界处理

**问题**：特征生成跨越区块边界时可能导致截断或不一致。

**解决**：`WorldGenRegion` 提供邻居区块访问，确保边界正确处理。

```cpp
// 在 WorldGenRegion 中检查邻居
const BlockState* getBlock(i32 x, i32 y, i32 z) const {
    i32 relX = x - (m_mainX * 16);
    i32 relZ = z - (m_mainZ * 16);
    // 处理边界情况
    if (relX < 0 || relX >= 16 || relZ < 0 || relZ >= 16) {
        return getChunk(relX / 16, relZ / 16)->getBlock(relX & 15, y, relZ & 15);
    }
    return getMainChunk()->getBlock(relX, y, relZ);
}
```

### 3. 结构生成顺序

**问题**：结构依赖于特征生成的结果，顺序错误会导致问题。

**解决**：严格按照 `DecorationStage` 顺序生成。

```cpp
// 正确的生成顺序
for (auto stage : DecorationStages::getAll()) {
    generator.generateStage(region, chunk, stage);
}
```

### 4. 雕刻掩码重复使用

**问题**：雕刻掩码在多次雕刻调用间应保持状态。

**解决**：使用 `CarvingMask` 追踪已雕刻位置。

```cpp
CarvingMask mask(chunkX, chunkZ);
generator.applyCarvers(region, primer, false, mask); // 传递掩码
```

### 5. 生物群系缓存失效

**问题**：生物群系数据在生成过程中被修改。

**解决**：在 `generateBiomes` 阶段完成后，生物群系数据应只读。

### 6. 模板加载时机

**问题**：Jigsaw 模板在方块注册前加载会失败。

**解决**：确保模板加载在方块系统初始化之后。

```cpp
// 正确的初始化顺序
BlockRegistry::initialize();           // 1. 初始化方块
BiomeRegistry::initialize();           // 2. 初始化生物群系
JigsawPatternRegistry::initialize();   // 3. 初始化 Jigsaw 模板池
FeatureRegistry::initialize();         // 4. 初始化特征
```

### 7. 高度图更新

**问题**：生成方块后未更新高度图导致后续生成错误。

**解决**：在 `ChunkPrimer` 中设置方块时自动更新高度图。

### 8. WorldGenRegion 不是 IBlockReader

**问题**：`WorldGenRegion` 不是 `IBlockReader`，直接传递给 `Block::isValidPosition` 会导致编译错误或运行时断言。

**解决**：在世界生成特性中，使用显式的本地放置检查（`isWater`、支撑方块检查），而不是依赖 `Block::isValidPosition` 接口。

### 9. 区块访问窗口越界

**问题**：`WorldGenRegion` 使用阶段特定的 `ChunkStatus::taskRange()` 窗口，如果请求的区块缺失，`getTopBlockY()` 会触发断言。

**解决**：不要将窗口外的高度查询视为"高度 0"；应修复区域半径或调用点，确保在正确的生成阶段访问正确范围内的区块。

### 10. 蓝冰放置测试

**问题**：蓝冰放置测试如果没有冰块在采样起始位置周围，将始终失败。

**解决**：测试必须在精确的采样邻域设置冰块，而不是以会改变海底检测的方式替换整个水层。

### 11. 临时 BlockState 副本

**问题**：将临时 `BlockState` 副本传递给世界写入 API 可能导致状态 ID 不一致。

**解决**：优先使用 `state.with(...)` / `defaultState()` 返回的规范引用；`ServerWorld::setBlockState` 现在通过 `stateId` 规范化作为安全网。

### 12. 海洋特征注册顺序

**问题**：`KelpFeatureIds` 和 `SeagrassFeatureIds` 按海洋温度分开，添加新海洋变体时可能导致不一致。

**解决**：添加新海洋变体时，保持 `FeatureRegistry::initialize()` 顺序、`BiomeGenerationSettings` 映射和海洋断言同步。

---

## 涉及的测试用例

| 测试文件 | 路径 | 测试内容 |
|----------|------|----------|
| `test_chunk_generation.cpp` | `tests/common/world/gen/` | 区块生成基本流程 |
| `test_carver.cpp` | `tests/common/` | 雕刻器功能测试 |
| `test_tree_feature.cpp` | `tests/common/` | 树木生成测试 |
| `test_ore_feature.cpp` | `tests/common/` | 矿石生成测试 |
| `test_surface.cpp` | `tests/common/` | 地表构建测试 |
| `test_decoration_stage.cpp` | `tests/common/` | 装饰阶段顺序测试 |
| `test_vegetation_features.cpp` | `tests/common/world/gen/` | 植被特征测试 |
| `WorldGenDeterminismTest.cpp` | `tests/common/world/gen/` | 世界生成确定性测试 |
| `WorldGenSpawnerTest.cpp` | `tests/common/world/gen/` | 生物放置测试 |
| `ChunkSpawnIntegrationTest.cpp` | `tests/common/world/gen/` | 区块生成集成测试 |

---

## 参考资料

- Minecraft 1.16.5 源码：`D:\Minecraft\MC研究\Minecraft1.16.5源码\net\minecraft`
- MC Wiki 世界生成：https://minecraft.fandom.com/wiki/World_generation
- MC Wiki 区块格式：https://minecraft.fandom.com/wiki/Chunk_format

---

## 版本历史

| 版本 | 日期 | 变更 |
|------|------|------|
| 1.0.0 | 2025-03 | 初始版本，完成核心生成系统 |
