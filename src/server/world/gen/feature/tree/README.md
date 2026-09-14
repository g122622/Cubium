# 树木生成系统 (Tree Generation System)

本目录实现了 Minecraft 1.16.5 风格的树木生成系统，采用 **TrunkPlacer + FoliagePlacer** 架构，支持灵活组合不同类型的树干和树冠。

## 目录结构

```
tree/
├── TreeFeature.hpp             # 树木特征主类和配置
├── TreeFeature.cpp             # 树木特征实现
├── featuresize/                # 最小尺寸约束（FeatureSize）
│   ├── FeatureSize.hpp         # 基类 + TwoLayers/ThreeLayers 子类（纯头文件）
│   └── README.md
├── trunk/                      # 树干放置器
│   ├── TrunkPlacer.hpp/cpp     # 树干放置器基类
│   ├── BendingTrunkPlacer.hpp/cpp # 弯曲树干（杜鹃树/红树林）
│   ├── StraightTrunkPlacer.hpp/cpp  # 直树干（橡树/白桦/云杉/丛林木）
│   ├── CherryTrunkPlacer.hpp/cpp    # 樱花树干放置器
│   └── TrunkPlacers.hpp/cpp    # 其他树干放置器（深色橡树/精美/金合欢/巨型云杉/巨型丛林木）
├── foliage/                    # 树叶放置器
│   ├── FoliagePlacer.hpp/cpp   # 树叶放置器基类（支持加权树叶提供者）
│   ├── BlobFoliagePlacer.hpp/cpp    # 球形树叶（橡树/白桦）
│   ├── CherryFoliagePlacer.hpp/cpp  # 樱花树叶放置器
│   ├── RandomSpreadFoliagePlacer.hpp/cpp # 随机散布树叶（杜鹃树）
│   └── FoliagePlacers.hpp/cpp  # 其他树叶放置器（松树/云杉/金合欢/深色橡树/丛林木/巨型松树/灌木/精美）
└── README.md
```

## 内部模块关系

```
TreeFeature（主入口）
    └── TreeFeatureConfig（配置）
            ├── trunkBlock / foliageBlock（方块状态）
            ├── foliageProvider（可选加权树叶提供者，优先于 foliageBlock）
            ├── TrunkPlacer（树干放置器）
            │       ├── StraightTrunkPlacer
            │       ├── DarkOakTrunkPlacer
            │       ├── FancyTrunkPlacer
            │       ├── ForkyTrunkPlacer
            │       ├── GiantTrunkPlacer
            │       ├── MegaJungleTrunkPlacer
            │       ├── CherryTrunkPlacer
            │       └── BendingTrunkPlacer
            ├── FoliagePlacer（树叶放置器）
            │       ├── BlobFoliagePlacer
            │       ├── PineFoliagePlacer
            │       ├── SpruceFoliagePlacer
            │       ├── AcaciaFoliagePlacer
            │       ├── DarkOakFoliagePlacer
            │       ├── JungleFoliagePlacer
            │       ├── MegaPineFoliagePlacer
            │       ├── BushFoliagePlacer
            │       ├── FancyFoliagePlacer
            │       ├── CherryFoliagePlacer
            │       └── RandomSpreadFoliagePlacer
            └── FeatureSize（最小尺寸约束）
                    ├── TwoLayersFeatureSize（limit/lowerSize/upperSize）
                    └── ThreeLayersFeatureSize（limit/upperLimit/lowerSize/middleSize/upperSize）
```

**生成流程**：`TreeFeature::place()` → `TrunkPlacer::placeTrunk()` 返回 `FoliagePosition` 列表 → `FoliagePlacer::placeFoliage()` 生成树叶

## 上下游外部依赖关系

### 上游依赖

| 模块 | 路径 | 用途 |
|------|------|------|
| FeatureSpread | `../FeatureSpread.hpp` | 随机数值范围配置 |
| WorldGenRegion | `../../chunk/IChunkGenerator.hpp` | 区块区域访问 |
| BlockRegistry | `../../../block/BlockRegistry.hpp` | 方块注册表 |
| VanillaBlocks | `../../../block/VanillaBlocks.hpp` | 原版方块常量 |
| BlockPos | `../../../chunk/ChunkPos.hpp` | 方块位置类型 |
| Random | `../../../../util/math/random/Random.hpp` | 随机数生成器 |

### 下游依赖

- `mc::world::gen::feature` - 特征系统通过 `TreeFeatures` 工厂类创建预定义树木
- `mc::world::biome` - 生物群系通过 `BiomeGenerationSettings` 配置树木特征

## 容易踩的坑

### 1. 配置深拷贝问题

`TreeFeatureConfig` 包含 `unique_ptr` 成员，已实现深拷贝构造函数和赋值运算符，复制配置时正确调用即可。

### 2. 树叶距离属性未实现

`setFoliageDistance()` 方法尚未实现，树叶不会因距离树干太远而自动腐烂。

### 3. 高度检查边界

树木生成位置必须满足 `y >= 1 && y + trunkHeight < 256`，`TreeFeature::place()` 会检查边界，但不检查生成后的树叶是否会超出世界高度。

### 4. 空间检查半径与 forcePlacement

`_calculateAvailableHeight()` 使用 `config.minimumSize->getSizeAtHeight(trunkHeight, y)` 决定每层 y 的水平检查半径（对应 MC 1.21.11 `TreeFeature.getMaxFreeTreeHeight`）。若 `minimumSize` 为空（未在 JSON 中配置 `minimum_size`），退化为默认规则（底部 0、中段 1、顶部 2）。`forcePlacement=true` 会跳过该体积检查。

`minimumSize->minClippedHeight()` 有值时，允许实际可用高度不足 `trunkHeight` 但 >= 该值时仍生成（裁剪高度），用于 `fancy_oak` 等容忍较矮空间的配置。

### 5. TrunkPlacer 和 FoliagePlacer 必须配对

某些组合可能产生不自然的形状，推荐组合：
- StraightTrunkPlacer + BlobFoliagePlacer（橡树、白桦）
- StraightTrunkPlacer + SpruceFoliagePlacer（云杉）
- ForkyTrunkPlacer + AcaciaFoliagePlacer（金合欢）
- DarkOakTrunkPlacer + DarkOakFoliagePlacer（深色橡树）
- GiantTrunkPlacer + MegaPineFoliagePlacer（巨型云杉）
- BendingTrunkPlacer + RandomSpreadFoliagePlacer（杜鹃树）

### 6. 随机数种子一致性

相同种子必须生成相同的树木，使用 `math::Random` 类确保随机序列可重现。

### 7. FeatureSpread 理解

`FeatureSpread::spread(base, spread)` 返回 `[base, base + spread]`，不是 `[base - spread, base + spread]`：
```cpp
FeatureSpread::spread(4, 2)  // 返回 4, 5, 或 6
FeatureSpread::fixed(5)      // 总是返回 5
```

### 8. 初始化顺序

树木特征依赖方块系统。`MinecraftServer::initializeRegistries` 在 `VanillaBlocks::initialize()` 之后通过 `ConfiguredFeatureLoader` 从数据包加载 configured_feature（含 tree 类型），再经 `PlacedFeatureLoader` 包装为 placed_feature，最后由 `BiomeLoader` 写入各生物群系的 `features` 数组。不再有 `FeatureRegistry::initialize()` 硬编码注册。

### 9. 高度常量使用

必须使用 `mc::world::MIN_BUILD_HEIGHT`、`MAX_BUILD_HEIGHT` 等常量，禁止硬编码 0、256 等数字。

### 10. 区块尺寸常量使用

必须使用 `mc::world::CHUNK_WIDTH`、`CHUNK_HEIGHT`、`CHUNK_SECTION_HEIGHT` 等常量，禁止硬编码 16 等数字。

### 11. 加权树叶提供者（foliageProvider）

`TreeFeatureConfig::foliageProvider`（`WeightedBlockStateProvider`）优先级高于 `foliageBlock`。
设置后，每个叶片独立采样（用于杜鹃树混合杜鹃叶/开花杜鹃叶等场景）。
`FoliagePlacer::placeFoliage` 提供两个重载：单一 `foliageBlock` 版本和带 `foliageProvider` 版本。
`TreeFeature::place` 始终传递 `config.foliageProvider.get()`（可能为 nullptr，此时回退到 `foliageBlock`）。

### 12. RandomSpreadFoliagePlacer 边界保护

`RandomSpreadFoliagePlacer::placeFoliageInternal` 在 `radius == 0` 或 `foliageHeight == 0` 时
会跳过 `nextInt(0)` 调用（避免非法参数），将对应轴偏移置为 0。
`foliageHeight` IntProvider 为 nullptr 时，`getFoliageHeight` 安全返回 0。

必须使用 `mc::world::CHUNK_WIDTH`、`CHUNK_HEIGHT`、`CHUNK_SECTION_HEIGHT` 等常量，禁止硬编码 16 等数字。
