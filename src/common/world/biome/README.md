# Biome 模块

本目录包含 Minecraft Reborn 项目的生物群系（Biome）系统，负责生物群系定义、生成和分布。该系统完整复刻了 Minecraft Java 1.16.5 的层叠生物群系生成算法。

## 目录结构

```
biome/
├── Biome.hpp                    # 生物群系定义类
├── Biome.cpp
├── BiomeGenerationSettings.hpp  # 生物群系生成设置（特征配置）
├── BiomeGenerationSettings.cpp
├── BiomeProvider.hpp            # 生物群系提供者基类
├── BiomeProvider.cpp
├── BiomeRegistry.hpp            # 生物群系注册表
├── BiomeRegistry.cpp
├── Biomes.hpp                   # 生物群系 ID 常量定义
├── layer/                       # 层叠生成系统
│   ├── BiomeValues.hpp          # 层系统内部生物群系值
│   ├── BiomeValues.cpp
│   ├── Layer.hpp                # 层接口定义（IArea, ITransformer 等）
│   ├── LayerCacheConfig.hpp     # 缓存配置
│   ├── LayerContext.hpp         # 层上下文（随机数、缓存）
│   ├── LayerContext.cpp
│   ├── LayerUtil.hpp            # 层链构建工具
│   ├── LayerUtil.cpp
│   └── transformers/            # 层变换器
│       ├── TransformerTraits.hpp    # 变换器特征类
│       ├── TransformerTraits.cpp
│       ├── SourceLayers.hpp         # 源层（岛屿、海洋温度）
│       ├── SourceLayers.cpp
│       ├── ClimateLayers.hpp        # 气候层（温度、深海）
│       ├── ClimateLayers.cpp
│       ├── EdgeLayers.hpp           # 边缘层（气候过渡、生物群系边缘）
│       ├── EdgeLayers.cpp
│       ├── ZoomLayers.hpp           # 缩放层
│       ├── ZoomLayers.cpp
│       ├── BiomeLayers.hpp          # 生物群系分配层
│       ├── BiomeLayers.cpp
│       └── MergeLayers.hpp          # 合并层（山丘、河流、海洋混合）
│       └── MergeLayers.cpp
```

## 文件详细说明

### 核心文件

#### Biome.hpp / Biome.cpp

**职责**：定义单个生物群系的属性和行为。

**主要内容**：
- `Biome` 类：存储生物群系的所有属性
  - 基本信息：ID、名称、类别（Category）
  - 地形参数：深度（depth）、比例（scale）
  - 气候参数：温度、湿度、大陆度、侵蚀度
  - 方块设置：表面方块、次表面方块、水下方块、基岩方块
  - 生成设置：特征列表（树木、矿石、花卉等）
  - 生物生成设置：可生成的生物类型和概率

- `BiomeClimate` 结构体：气候设置（降水类型、温度、降水量）
- `Biomes` 命名空间：170 个生物群系 ID 常量（与 MC 1.16.5 完全一致）

**关键枚举**：
```cpp
enum class Category {
    None, Taiga, ExtremeHills, Jungle, Mesa, Plains,
    Savanna, Icy, TheEnd, Beach, Forest, Ocean,
    Desert, River, Swamp, Mushroom, Nether
};
```

#### BiomeGenerationSettings.hpp / BiomeGenerationSettings.cpp

**职责**：管理生物群系的特征生成配置。

**主要内容**：
- 按装饰阶段（DecorationStage）组织特征列表
- 提供预设配置（createPlains、createForest、createDesert、createRiver、createFrozenRiver、createSwampHills、createWarmOcean、createColdOcean、createFrozenOcean、createDeepWarmOcean、createDeepLukewarmOcean、createDeepColdOcean、createDeepFrozenOcean 等）
- `BiomeFeaturePlacer` 类：在区块中放置特征

**装饰阶段**：
- `UndergroundOres`：矿石生成
- `Lakes`：湖泊装饰（水湖、熔岩湖）
- `SurfaceStructures`：表面结构（冰刺等）
- `VegetalDecoration`：植被装饰（树木、花卉、草丛）

#### BiomeProvider.hpp / BiomeProvider.cpp

**职责**：生物群系提供者的抽象基类。

**主要接口**：
```cpp
virtual BiomeId getBiome(i32 x, i32 y, i32 z) const = 0;
virtual BiomeId getNoiseBiome(i32 noiseX, i32 noiseY, i32 noiseZ) const = 0;
virtual f32 getDepth(i32 x, i32 z) const = 0;
virtual f32 getScale(i32 x, i32 z) const = 0;
virtual void fillBiomeContainer(BiomeContainer& container, ChunkCoord x, ChunkCoord z) = 0;
```

#### BiomeRegistry.hpp / BiomeRegistry.cpp

**职责**：管理所有注册的生物群系定义。

**主要内容**：
- `BiomeRegistry` 单例类：存储和查询生物群系定义
- `BiomeFactory` 命名空间：工厂函数创建预设生物群系

**使用方法**：
```cpp
BiomeRegistry::instance().initialize();
const Biome& biome = BiomeRegistry::instance().get(Biomes::Plains);
```

### 层叠生成系统（layer/）

#### Layer.hpp

**职责**：定义层系统的核心接口。

**主要类型**：
- `IArea`：区域采样接口
- `IAreaContext`：区域上下文（随机数生成）
- `IExtendedAreaContext`：扩展上下文（创建延迟区域）
- `ITransformer0/1/2`：零/单/双输入变换器接口
- `IAreaFactory`：区域工厂接口
- `SharedFactory`：共享工厂包装器

#### LayerContext.hpp / LayerContext.cpp

**职责**：提供层系统的上下文和缓存。

**主要类型**：
- `LayerContext`：实现 `IExtendedAreaContext`
  - 位置感知随机数生成（FastRandom.mix 算法）
  - 共享缓存管理
- `LazyArea`：延迟计算区域，带 LRU 缓存

#### LayerUtil.hpp / LayerUtil.cpp

**职责**：构建完整的生物群系层链。

**主要函数**：
- `buildOverworldLayers()`：构建主世界层链
- `createOverworldLayers()`：创建主世界 LayerStack
- `LayerStack` 类：管理层链，提供采样接口
- `LayerBiomeProvider` 类：基于层系统的生物群系提供者

#### BiomeValues.hpp / BiomeValues.cpp

**职责**：定义层系统使用的内部生物群系值和辅助函数。

**主要内容**：
- 170 个生物群系值常量（与 Biomes::Id 对应）
- `Climate` 命名空间：温度区域值（Ocean、Warm、Medium、Cool、Icy）
- `SpecialBits` 命名空间：特殊变体位操作
- 辅助函数：`isOcean`、`isShallowOcean`、`isBadlands`、`isJungle`、`isSnowy`、`areBiomesSimilar` 等

#### transformers/ 目录

**变换器特征类（TransformerTraits.hpp）**：
- `IC0Transformer`：无偏移变换器（采样单点）
- `IC1Transformer`：+1 偏移变换器
- `ICastleTransformer`：四方向变换器（N/E/S/W + 中心）
- `IBishopTransformer`：四对角变换器（SW/SE/NE/NW + 中心）

**源层（SourceLayers.hpp）**：
- `IslandLayer`：初始岛屿生成（10% 陆地概率，原点固定陆地）
- `OceanLayer`：海洋温度生成（使用 Perlin 噪声）

**气候层（ClimateLayers.hpp）**：
- `AddIslandLayer`：扩展岛屿
- `AddSnowLayer`：分配温度区域
- `RemoveTooMuchOceanLayer`：减少过多海洋
- `DeepOceanLayer`：生成深海

**边缘层（EdgeLayers.hpp）**：
- `CoolWarmEdgeLayer`：温暖与凉爽区域过渡
- `HeatIceEdgeLayer`：炎热与冰冻区域过渡
- `SpecialEdgeLayer`：添加特殊变体位
- `BiomeEdgeLayer`：生物群系边缘处理

**缩放层（ZoomLayers.hpp）**：
- `ZoomLayer`：2x 放大（Normal 和 Fuzzy 两种模式）

**生物群系层（BiomeLayers.hpp）**：
- `BiomeLayer`：温度值转生物群系 ID
- `RareBiomeLayer`：稀有变体生成
- `ShoreLayer`：海岸生成
- `SmoothLayer`：边界平滑

**合并层（MergeLayers.hpp）**：
- `AddMushroomIslandLayer`：蘑菇岛生成
- `AddBambooForestLayer`：竹林生成
- `StartRiverLayer`：河流噪声起始
- `RiverLayer`：河流通道生成
- `HillsLayer`：山丘变体生成
- `MixRiverLayer`：河流合并
- `MixOceansLayer`：海洋温度合并

## 模块关系图

```
┌─────────────────────────────────────────────────────────────────┐
│                     ChunkGenerator                               │
│                          │                                       │
│                          ▼                                       │
│                   BiomeProvider                                  │
│                          │                                       │
│            ┌─────────────┴─────────────┐                         │
│            │                           │                         │
│     LayerBiomeProvider          (其他实现)                       │
│            │                                                     │
│            ▼                                                     │
│       LayerStack                                                 │
│            │                                                     │
│            ▼                                                     │
│    ┌───────────────────┐                                         │
│    │    IAreaFactory   │◄───── 工厂模式                          │
│    └───────────────────┘                                         │
│            │                                                     │
│    ┌───────┴───────┬─────────────┬─────────────┐                 │
│    │               │             │             │                 │
│    ▼               ▼             ▼             ▼                 │
│ SourceFactory  TransformFactory MergeFactory  ...               │
│    │               │             │                               │
│    ▼               ▼             ▼                               │
│ ITransformer0  ITransformer1  ITransformer2                     │
│    │               │             │                               │
│    ▼               ▼             ▼                               │
│ IslandLayer    ZoomLayer     HillsLayer                         │
│ OceanLayer     BiomeLayer    MixRiverLayer                       │
│ ...            ...           ...                                │
└─────────────────────────────────────────────────────────────────┘
```

## 整体职责

1. **生物群系定义**：定义 170 个生物群系的属性、气候、特征配置
2. **层叠生成**：实现 MC 1.16.5 的层叠生物群系生成算法
3. **生物群系查询**：提供世界坐标到生物群系的映射接口
4. **特征配置**：管理各生物群系的树木、矿石、花卉等特征

## 输入与输出

### 输入

| 输入项 | 类型 | 来源 | 说明 |
|--------|------|------|------|
| 世界种子 | `u64` | 世界创建 | 用于初始化随机数生成 |
| 世界坐标 | `(x, y, z)` | 区块生成 | 查询生物群系 |
| 区块坐标 | `(chunkX, chunkZ)` | 区块生成 | 批量填充生物群系容器 |

### 输出

| 输出项 | 类型 | 用途 |
|--------|------|------|
| 生物群系 ID | `BiomeId` | 区块生成、地形高度、特征放置 |
| 生物群系定义 | `const Biome&` | 获取深度、比例、特征配置 |
| 生物群系容器 | `BiomeContainer` | 区块数据的一部分 |

## 依赖项

### 上游依赖

```
common/core/Types.hpp          # 基础类型定义
common/util/math/random/       # 随机数生成器
common/util/cache/             # LRU 缓存
common/world/chunk/            # 区块数据结构
common/world/block/            # 方块定义
common/world/gen/noise/        # 噪声生成器
common/world/gen/feature/      # 特征系统
common/world/spawn/            # 生物生成设置
```

### 下游依赖

```
common/world/gen/chunk/        # 区块生成器
server/world/                  # 服务端世界管理
client/world/                  # 客户端世界渲染
```

## 使用方法

### 创建生物群系提供者

```cpp
#include "world/biome/layer/LayerUtil.hpp"

// 创建主世界生物群系提供者
u64 seed = 12345;
bool largeBiomes = false;

auto provider = std::make_unique<LayerBiomeProvider>(seed, largeBiomes);
```

### 查询生物群系

```cpp
// 获取单个坐标的生物群系
BiomeId biomeId = provider->getBiome(x, y, z);

// 获取生物群系定义
const Biome& biome = provider->getBiomeDefinition(biomeId);

// 获取地形参数
f32 depth = provider->getDepth(x, z);
f32 scale = provider->getScale(x, z);

// 批量获取（更高效）
BiomeId biomes[256];
provider->getBiomesBatch(startX, 0, startZ, 16, 16, biomes);
```

### 填充区块生物群系容器

```cpp
void generateChunk(ChunkData& chunk, ChunkCoord x, ChunkCoord z) {
    BiomeContainer container;
    provider->fillBiomeContainer(container, x, z);
    chunk.setBiomeContainer(std::move(container));
}
```

### 注册自定义生物群系

```cpp
void registerCustomBiomes() {
    BiomeRegistry& registry = BiomeRegistry::instance();
    registry.initialize();  // 先初始化默认生物群系
    
    Biome customBiome(200, "custom:my_biome");
    customBiome.setCategory(Biome::Category::Plains);
    customBiome.setTemperature(0.8f);
    customBiome.setSurfaceBlock(&VanillaBlocks::GRASS_BLOCK);
    registry.registerBiome(customBiome);
}
```

## 容易踩的坑

### 1. 生物群系 ID 与层系统值的关系

**问题**：`Biomes::Plains` 和 `layer::BiomeValues::Plains` 是相同的值，但在不同上下文中使用。

**解决方案**：
```cpp
// 在 Biome 层面使用 Biomes::Plains
BiomeId id = Biomes::Plains;

// 在 Layer 层面使用 BiomeValues::Plains
i32 layerValue = BiomeValues::Plains;

// 它们值相同，可以转换
BiomeId fromLayer = static_cast<BiomeId>(layerValue);
```

### 2. 层链构建顺序

**问题**：层链的构建顺序必须严格按照 MC 1.16.5 的顺序，否则生物群系分布会不正确。

**解决方案**：使用 `LayerUtil::buildOverworldLayers()` 或 `LayerUtil::createOverworldLayers()`，不要手动构建。

### 3. 缓存线程安全

**问题**：`LazyArea` 使用共享缓存，多线程访问需要加锁。

**解决方案**：
```cpp
// 使用批量接口减少锁竞争
layerStack->sampleBatch(startX, startZ, width, height, output);
// 而不是
for (int z = 0; z < height; z++) {
    for (int x = 0; x < width; x++) {
        output[z * width + x] = layerStack->sample(startX + x, startZ + z);
    }
}
```

### 4. 坐标系统

**问题**：层系统使用的是"区域坐标"，不是方块坐标。

**解决方案**：
```cpp
// 噪声坐标是 4x4 方块一个大块
BiomeId getNoiseBiome(i32 noiseX, i32 noiseY, i32 noiseZ) {
    return layerStack->sample(noiseX << 2, noiseZ << 2);
}
```

### 5. 随机数确定性

**问题**：相同的种子必须产生相同的生物群系分布。

**解决方案**：使用 `LayerContext::setPosition()` 和 `LayerContext::nextInt()`，它们实现了 MC 的 FastRandom.mix 算法。

### 6. 生物群系相似性判断

**问题**：`BiomeValues::areBiomesSimilar()` 使用类别判断，不是简单的 ID 比较。

**示例**：
```cpp
// Plains(1) 和 SunflowerPlains(129) 是相似的（同属 Plains 类别）
areBiomesSimilar(1, 129);  // true

// Desert(2) 和 DesertHills(17) 是相似的（同属 Desert 类别）
areBiomesSimilar(2, 17);   // true

// Plains(1) 和 Desert(2) 不相似
areBiomesSimilar(1, 2);    // false
```

### 7. 山丘变体的邻居检查

**问题**：`HillsLayer` 生成山丘变体时需要检查周围邻居是否相似。

**解决方案**：代码中已经实现了正确的邻居检查逻辑，确保山丘变体只在合适的区域生成。

### 8. 海洋温度层独立分支

**问题**：海洋温度层是独立分支，不参与气候层计算。

**解决方案**：在 `buildOverworldLayers()` 中，海洋温度通过 `MixOceansLayer` 最后合并到生物群系层。

## 层链流程图

```
IslandLayer
    │
    ▼ (FuzzyZoom)
AddIslandLayer × 4
    │
    ▼
RemoveTooMuchOceanLayer
    │
    ▼
AddSnowLayer
    │
    ▼
AddIslandLayer
    │
    ▼
CoolWarmEdgeLayer ── HeatIceEdgeLayer ── SpecialEdgeLayer
    │
    ▼ (Zoom × 2)
AddIslandLayer
    │
    ▼
AddMushroomIslandLayer
    │
    ▼
DeepOceanLayer
    │
    ├──────────────────────────────────────────┐
    │                                          │
    ▼ (气候层)                                 ▼ (海洋温度分支)
BiomeLayer                                OceanLayer
    │                                          │
    ▼                                          ▼ (Zoom × 6)
AddBambooForestLayer                        WarmOcean/LukewarmOcean/ColdOcean/FrozenOcean
    │
    ▼ (Zoom × 2)
BiomeEdgeLayer
    │
    ├──────────────────────────┐
    │                          │
    ▼                          ▼ (河流分支)
HillsLayer ◄──────────── StartRiverLayer
    │                          │
    ▼                          ▼ (Zoom × riverSize)
RareBiomeLayer              RiverLayer
    │                          │
    ▼ (Zoom × biomeSize)       ▼
ShoreLayer                  SmoothLayer
    │                          │
    ▼                          │
SmoothLayer                   │
    │                          │
    └──────────┬───────────────┘
               │
               ▼
         MixRiverLayer
               │
               ▼
         MixOceansLayer
               │
               ▼
         最终生物群系层
```

## 涉及的测试用例

测试文件位于 `tests/common/world/biome/layer/BiomeLayerTest.cpp`，包含以下测试套件：

### BiomeLayerTest

| 测试名称 | 描述 |
|----------|------|
| OceanValuePreserved | 验证海洋值在 BiomeLayer 中保持不变 |
| MushroomFieldsPreserved | 验证蘑菇岛保持不变 |
| WarmClimateProducesCorrectBiomes | 验证温暖气候生成正确的生物群系（沙漠、热带草原、平原） |
| WarmClimateWithSpecialBitsProducesBadlands | 验证温暖气候带特殊位生成恶地变体 |
| MediumClimateProducesCorrectBiomes | 验证中等气候生成正确的生物群系 |
| MediumClimateWithSpecialBitsProducesJungle | 验证中等气候带特殊位生成丛林 |
| CoolClimateProducesCorrectBiomes | 验证凉爽气候生成正确的生物群系 |
| CoolClimateWithSpecialBitsProducesGiantTreeTaiga | 验证凉爽气候带特殊位生成大型针叶林 |
| IcyClimateProducesCorrectBiomes | 验证冰冻气候生成雪地生物群系 |
| UnknownValueReturnsMushroomFields | 验证未知值返回蘑菇岛 |

### RareBiomeLayerTest

| 测试名称 | 描述 |
|----------|------|
| PlainsToSunflowerPlains | 验证平原有概率变成向日葵平原 |
| PlainsStaysPlainsWhenNotLucky | 验证平原不满足概率时保持不变 |
| OtherBiomesUnchanged | 验证其他生物群系不受影响 |

### SmoothLayerTest

| 测试名称 | 描述 |
|----------|------|
| AllEqualReturnsEither | 验证所有邻居相等时随机选择 |
| EastWestEqualReturnsEast | 验证东西相等返回东 |
| NorthSouthEqualReturnsNorth | 验证南北相等返回北 |
| NoneEqualReturnsCenter | 验证都不相等返回中心 |

### ShoreLayerTest

| 测试名称 | 描述 |
|----------|------|
| MushroomFieldsAdjacentToShallowOceanBecomesShore | 验证蘑菇岛相邻浅海变成蘑菇岛海岸 |
| MushroomFieldsNotAdjacentToOceanStaysSame | 验证蘑菇岛不相邻海洋保持不变 |
| SnowyBiomeAdjacentToOceanBecomesSnowyBeach | 验证雪地相邻海洋变成雪地海滩 |
| JungleAdjacentToIncompatibleBecomesJungleEdge | 验证丛林相邻不兼容生物群系变成丛林边缘 |
| MountainsAdjacentToOceanBecomesStoneShore | 验证山地相邻海洋变成石岸 |
| NormalBiomeAdjacentToOceanBecomesBeach | 验证普通生物群系相邻海洋变成海滩 |
| RiverNotAffected | 验证河流不受海岸层影响 |
| SwampNotAffected | 验证沼泽不受海岸层影响 |

### BiomeValuesTest

| 测试名称 | 描述 |
|----------|------|
| IsOceanCorrect | 验证 isOcean 函数正确性 |
| IsShallowOceanCorrect | 验证 isShallowOcean 函数正确性 |
| IsBadlandsCorrect | 验证 isBadlands 函数正确性 |
| IsJungleCorrect | 验证 isJungle 函数正确性 |
| IsJungleCompatibleCorrect | 验证 isJungleCompatible 函数正确性 |
| IsSnowyCorrect | 验证 isSnowy 函数正确性 |
| IsMountainCorrect | 验证 isMountain 函数正确性 |
| AreBiomesSimilarCorrect | 验证 areBiomesSimilar 函数正确性 |
| SpecialBitsExtraction | 验证特殊位提取正确性 |
| SpecialBitsSet | 验证特殊位设置正确性 |

## 参考资料

- Minecraft Wiki - Biome: https://minecraft.wiki/w/Biome
- Minecraft Wiki - Biome/IDs: https://minecraft.wiki/w/Java_Edition_data_values#Biomes
- Minecraft 1.16.5 源码 - `net.minecraft.world.biome` 包
- Minecraft 1.16.5 源码 - `net.minecraft.world.gen.layer` 包
