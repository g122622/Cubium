# Feature 开发指南

本文档介绍如何在 Minecraft Reborn 项目中添加新的世界生成特征（Feature）。

## 目录

1. [概述](#概述)
2. [Feature 系统架构](#feature-系统架构)
3. [添加新 Feature 的步骤](#添加新-feature-的步骤)
4. [放置器（Placement）系统](#放置器placement系统)
5. [常见陷阱与解决方案](#常见陷阱与解决方案)
6. [完整示例：添加新矿石](#完整示例添加新矿石)
7. [完整示例：添加新树木](#完整示例添加新树木)

---

## 概述

Feature（特征）是世界生成系统中的核心组件，用于在区块中放置矿石、树木、花草、结构等。系统基于 MC 1.16.5 架构设计。

### 相关文件位置

```
src/common/world/gen/feature/
├── Feature.hpp                    # Feature 基类
├── ConfiguredFeature.hpp          # 配置化特征基类和注册表
├── ConfiguredFeature.cpp          # 注册表实现
├── DecorationStage.hpp            # 装饰阶段枚举
├── ore/
│   ├── OreFeature.hpp             # 矿石特征
│   └── OreFeature.cpp             # 矿石特征实现
└── tree/
    ├── TreeFeature.hpp            # 树木特征
    ├── TreeFeature.cpp            # 树木特征实现
    ├── trunk/
    │   ├── TrunkPlacer.hpp        # 树干放置器基类
    │   └── StraightTrunkPlacer.hpp # 直树干放置器
    └── foliage/
        ├── FoliagePlacer.hpp      # 树叶放置器基类
        └── BlobFoliagePlacer.hpp  # 球形树叶放置器

src/common/world/gen/placement/
├── Placement.hpp                  # 放置器基类和配置
└── Placement.cpp                  # 放置器实现

src/common/world/biome/
├── BiomeGenerationSettings.hpp    # 生物群系生成设置
└── BiomeGenerationSettings.cpp    # 设置实现
```

---

## Feature 系统架构

### 装饰阶段（DecorationStage）

特征按阶段执行，顺序如下：

```
RawGeneration → Lakes → LocalModifications → UndergroundStructures
    → SurfaceStructures → Strongholds → UndergroundOres
    → UndergroundDecoration → VegetalDecoration → TopLayerModification
```

**重要**：每个阶段的特征ID从 0 开始编号，不同阶段的 ID 相互独立！

```cpp
enum class DecorationStage : u8 {
    RawGeneration = 0,
    Lakes,
    LocalModifications,
    UndergroundStructures,
    SurfaceStructures,
    Strongholds,
    UndergroundOres,        // 矿石放这里
    UndergroundDecoration,
    VegetalDecoration,      // 树木花草放这里
    TopLayerModification,
    Count
};
```

### 核心类关系

```
ConfiguredFeatureBase (抽象基类)
    ├── ConfiguredOreFeature (矿石特征)
    └── ConfiguredTreeFeature (树木特征)

FeatureRegistry (单例注册表)
    └── 按阶段存储特征指针

ConfiguredPlacement (放置器链)
    ├── CountPlacement (数量)
    ├── SquarePlacement (XZ分散)
    ├── HeightRangePlacement (高度范围)
    ├── SurfacePlacement (地表)
    └── ChancePlacement (概率)
```

---

## 添加新 Feature 的步骤

### 第一步：创建 Feature 配置类

```cpp
// MyFeature.hpp
#pragma once

#include "../Feature.hpp"
#include "../../placement/Placement.hpp"
#include <memory>

namespace mc {

class WorldGenRegion;

/**
 * @brief 我的特征配置
 */
struct MyFeatureConfig : public IFeatureConfig {
    BlockId blockToPlace;    // 要放置的方块
    i32 size;                // 大小

    MyFeatureConfig(BlockId block, i32 s)
        : blockToPlace(block), size(s) {}
};

} // namespace mc
```

### 第二步：创建 Feature 类

```cpp
// MyFeature.hpp 继续

/**
 * @brief 我的特征实现
 */
class MyFeature {
public:
    /**
     * @brief 放置特征
     * @param region 世界区域
     * @param random 随机数
     * @param pos 起始位置
     * @param config 配置
     * @return 是否成功放置
     */
    bool place(
        WorldGenRegion& region,
        math::Random& random,
        const BlockPos& pos,
        const MyFeatureConfig& config);
};

/**
 * @brief 配置化的特征
 *
 * 必须继承 ConfiguredFeatureBase 并实现三个方法：
 * - place(): 执行放置逻辑
 * - name(): 返回特征名称
 * - stage(): 返回装饰阶段
 */
class ConfiguredMyFeature : public ConfiguredFeatureBase {
public:
    ConfiguredMyFeature(
        std::unique_ptr<MyFeatureConfig> config,
        std::unique_ptr<ConfiguredPlacement> placement,
        const char* name);

    bool place(
        WorldGenRegion& region,
        ChunkPrimer& chunk,
        IChunkGenerator& generator,
        math::Random& random,
        const BlockPos& pos) override;

    [[nodiscard]] const char* name() const override { return m_name.c_str(); }
    [[nodiscard]] DecorationStage stage() const override { return DecorationStage::VegetalDecoration; }

private:
    std::unique_ptr<MyFeatureConfig> m_config;
    std::unique_ptr<ConfiguredPlacement> m_placement;
    std::string m_name;
    MyFeature m_feature;
};

/**
 * @brief 特征工厂类
 *
 * 负责：
 * 1. 创建所有预配置的特征实例
 * 2. 提供所有权转移方法给注册表
 */
struct MyFeatures {
    /// 初始化所有特征（在 FeatureRegistry::initialize() 中调用）
    static void initialize();

    /// 获取所有特征并清空内部存储（所有权转移）
    [[nodiscard]] static std::vector<std::unique_ptr<ConfiguredMyFeature>> getAllFeaturesAndClear();

    /// 获取只读访问
    [[nodiscard]] static const std::vector<std::unique_ptr<ConfiguredMyFeature>>& getAllFeatures();

    // 工厂方法
    static std::unique_ptr<ConfiguredMyFeature> createDefault();

private:
    static std::vector<std::unique_ptr<ConfiguredMyFeature>> s_features;
};

} // namespace mc
```

### 第三步：实现 Feature 类

```cpp
// MyFeature.cpp
#include "MyFeature.hpp"
#include "../../chunk/IChunkGenerator.hpp"
#include "../../chunk/ChunkPrimer.hpp"
#include <spdlog/spdlog.h>

namespace mc {

// 静态成员定义
std::vector<std::unique_ptr<ConfiguredMyFeature>> MyFeatures::s_features;

// ============================================================================
// MyFeature 实现
// ============================================================================

bool MyFeature::place(
    WorldGenRegion& region,
    math::Random& random,
    const BlockPos& pos,
    const MyFeatureConfig& config)
{
    // 实现放置逻辑
    // 1. 检查位置是否有效
    // 2. 在世界中放置方块
    // 3. 返回是否成功

    // 示例：放置单个方块
    if (pos.y < 0 || pos.y >= 256) {
        return false;
    }

    // 检查是否可以替换
    const BlockState* state = region.getBlockState(pos.x, pos.y, pos.z);
    if (state && !state->isAir()) {
        return false;
    }

    // 放置方块
    region.setBlockState(pos.x, pos.y, pos.z, BlockState(config.blockToPlace));
    return true;
}

// ============================================================================
// ConfiguredMyFeature 实现
// ============================================================================

ConfiguredMyFeature::ConfiguredMyFeature(
    std::unique_ptr<MyFeatureConfig> config,
    std::unique_ptr<ConfiguredPlacement> placement,
    const char* name)
    : m_config(std::move(config))
    , m_placement(std::move(placement))
    , m_name(name)
{
}

bool ConfiguredMyFeature::place(
    WorldGenRegion& region,
    ChunkPrimer& chunk,
    IChunkGenerator& generator,
    math::Random& random,
    const BlockPos& pos)
{
    (void)generator;
    (void)chunk;

    if (!m_config || !m_placement) {
        return false;
    }

    // 获取放置位置
    std::vector<BlockPos> positions = m_placement->getPositions(region, random, pos);

    if (positions.empty()) {
        return false;
    }

    bool placedAny = false;
    for (const BlockPos& placePos : positions) {
        // 有效性检查
        if (placePos.y < 0 || placePos.y >= 256) {
            continue;
        }

        if (m_feature.place(region, random, placePos, *m_config)) {
            placedAny = true;
        }
    }

    return placedAny;
}

// ============================================================================
// MyFeatures 实现
// ============================================================================

void MyFeatures::initialize() {
    s_features.clear();

    // 创建预配置的特征
    s_features.push_back(createDefault());

    spdlog::info("[MyFeatures] Initialized {} features", s_features.size());
}

std::vector<std::unique_ptr<ConfiguredMyFeature>> MyFeatures::getAllFeaturesAndClear() {
    std::vector<std::unique_ptr<ConfiguredMyFeature>> result = std::move(s_features);
    s_features.clear();
    return result;
}

const std::vector<std::unique_ptr<ConfiguredMyFeature>>& MyFeatures::getAllFeatures() {
    return s_features;
}

std::unique_ptr<ConfiguredMyFeature> MyFeatures::createDefault() {
    auto config = std::make_unique<MyFeatureConfig>(BlockId::Stone, 5);

    // 创建放置链：数量 -> 地表
    auto surfacePlacement = std::make_unique<SurfacePlacement>();
    auto surfaceConfig = std::make_unique<SurfacePlacementConfig>(0, false);

    auto countPlacement = std::make_unique<CountPlacement>();
    auto countConfig = std::make_unique<CountPlacementConfig>(3);

    auto surfaceConfigured = std::make_unique<ConfiguredPlacement>(
        std::move(surfacePlacement), std::move(surfaceConfig));
    auto countConfigured = std::make_unique<ConfiguredPlacement>(
        std::move(countPlacement), std::move(countConfig));

    // ⚠️ 重要：链的顺序！count -> surface
    countConfigured->setNext(std::move(surfaceConfigured));

    return std::make_unique<ConfiguredMyFeature>(
        std::move(config), std::move(countConfigured), "my_feature");
}

} // namespace mc
```

### 第四步：注册到 FeatureRegistry

```cpp
// ConfiguredFeature.cpp
#include "MyFeature.hpp"

void FeatureRegistry::initialize() {
    clear();

    // 注册矿石
    OreFeatures::initialize();
    auto oreFeatures = OreFeatures::getAllFeaturesAndClear();
    for (auto& feature : oreFeatures) {
        if (feature) {
            registerFeature(std::move(feature), DecorationStage::UndergroundOres);
        }
    }

    // 注册树木
    TreeFeatures::initialize();
    auto treeFeatures = TreeFeatures::getAllFeaturesAndClear();
    for (auto& feature : treeFeatures) {
        if (feature) {
            registerFeature(std::move(feature), DecorationStage::VegetalDecoration);
        }
    }

    // 【新增】注册你的特征
    MyFeatures::initialize();
    auto myFeatures = MyFeatures::getAllFeaturesAndClear();
    for (auto& feature : myFeatures) {
        if (feature) {
            registerFeature(std::move(feature), DecorationStage::VegetalDecoration);  // 选择正确的阶段
        }
    }

    spdlog::info("[FeatureRegistry] Initialized features:");
    for (size_t i = 0; i < m_featuresByStage.size(); ++i) {
        if (!m_featuresByStage[i].empty()) {
            spdlog::info("  Stage {}: {} features", i, m_featuresByStage[i].size());
        }
    }
}
```

### 第五步：添加到生物群系

```cpp
// BiomeGenerationSettings.cpp

BiomeGenerationSettings BiomeGenerationSettings::createForest() {
    BiomeGenerationSettings settings = createDefault();

    // 添加矿石（UndergroundOres 阶段，ID 从 0 开始）
    // 0: coal, 1: iron, 2: gold, 3: redstone, 4: diamond, 5: lapis, 6: emerald, 7: copper
    settings.addFeature(DecorationStage::UndergroundOres, 0);
    settings.addFeature(DecorationStage::UndergroundOres, 1);

    // 添加树木（VegetalDecoration 阶段，ID 也从 0 开始！）
    // 0: oak_tree, 1: birch_tree, 2: spruce_tree, ...
    settings.addFeature(DecorationStage::VegetalDecoration, 0);
    settings.addFeature(DecorationStage::VegetalDecoration, 1);

    // 【新增】添加你的特征（假设这是 VegetalDecoration 阶段的第 5 个特征）
    settings.addFeature(DecorationStage::VegetalDecoration, 5);

    return settings;
}
```

---

## 放置器（Placement）系统

放置器控制特征在世界中的放置位置。多个放置器可以链接在一起形成放置链。

### 可用的放置器

| 放置器 | 配置类 | 用途 |
|--------|--------|------|
| `CountPlacement` | `CountPlacementConfig` | 每区块尝试次数 |
| `SquarePlacement` | `EmptyPlacementConfig` | 在XZ平面内分散 |
| `HeightRangePlacement` | `HeightRangePlacementConfig` | 限制Y坐标范围 |
| `SurfacePlacement` | `SurfacePlacementConfig` | 找地表高度 |
| `ChancePlacement` | `ChancePlacementConfig` | 概率放置 |
| `BiomePlacement` | `BiomePlacementConfig` | 生物群系过滤 |

### 放置链的正确顺序

**关键**：放置器的执行顺序非常重要！

```cpp
// ✅ 正确：先决定次数，再分散位置，最后找高度/地表
CountPlacement -> SquarePlacement -> HeightRangePlacement
CountPlacement -> SquarePlacement -> SurfacePlacement

// ❌ 错误：Surface 在最前面会导致只找一次地表
SurfacePlacement -> CountPlacement  // 错误！
```

### 放置链代码示例

```cpp
// 矿石：每区块尝试N次，分散位置，限制高度
auto countPlacement = std::make_unique<CountPlacement>();
auto countConfig = std::make_unique<CountPlacementConfig>(20);

auto squarePlacement = std::make_unique<SquarePlacement>();
auto squareConfig = std::make_unique<EmptyPlacementConfig>();

auto heightPlacement = std::make_unique<HeightRangePlacement>();
auto heightConfig = std::make_unique<HeightRangePlacementConfig>(0, 0, 128);  // Y 0-127

// 构建链：count -> square -> height
auto heightConfigured = std::make_unique<ConfiguredPlacement>(
    std::move(heightPlacement), std::move(heightConfig));
auto squareConfigured = std::make_unique<ConfiguredPlacement>(
    std::move(squarePlacement), std::move(squareConfig));
auto countConfigured = std::make_unique<ConfiguredPlacement>(
    std::move(countPlacement), std::move(countConfig));

squareConfigured->setNext(std::move(heightConfigured));
countConfigured->setNext(std::move(squareConfigured));

// countConfigured 是链的入口
```

---

## 常见陷阱与解决方案

### 🚨 陷阱 1：特征 ID 映射错误

**问题**：BiomeGenerationSettings 中的特征 ID 是**阶段内索引**，不是全局索引！

```cpp
// ❌ 错误理解
// 以为 ID 8 是"第 9 个注册的特征"
settings.addFeature(DecorationStage::VegetalDecoration, 8);  // 错了！

// ✅ 正确理解
// VegetalDecoration 阶段的 ID 从 0 开始
// 假设注册顺序：oak=0, birch=1, spruce=2, jungle=3, sparse_oak=4
settings.addFeature(DecorationStage::VegetalDecoration, 0);  // 橡树
settings.addFeature(DecorationStage::VegetalDecoration, 1);  // 白桦
```

**解决方案**：查看 `FeatureRegistry::initialize()` 中的注册顺序，ID 就是注册顺序。

### 🚨 陷阱 2：放置链顺序错误

**问题**：放置器链的顺序决定了最终位置计算逻辑。

```cpp
// ❌ 错误：Surface 在前，只能找到一个位置
auto surfaceConfigured = std::make_unique<ConfiguredPlacement>(...);
auto countConfigured = std::make_unique<ConfiguredPlacement>(...);
surfaceConfigured->setNext(std::move(countConfigured));
// 结果：SurfacePlacement 找一个位置，CountPlacement 复制 N 次（同一个位置！）

// ✅ 正确：Count 在前，生成 N 个位置，每个都找地表
auto countConfigured = std::make_unique<ConfiguredPlacement>(...);
auto surfaceConfigured = std::make_unique<ConfiguredPlacement>(...);
countConfigured->setNext(std::move(surfaceConfigured));
// 结果：CountPlacement 生成 N 个基准位置，每个都调用 SurfacePlacement 找地表
```

### 🚨 陷阱 3：FEATURES 阶段被跳过

**问题**：区块生成时 FEATURES 阶段不执行，导致没有特征生成。

**原因**：`ServerChunkManager::checkNeighborsReady()` 中邻居检查使用了错误的阶段。

```cpp
// ❌ 错误：检查当前阶段的邻居
const ChunkStatus* requiredStatus = &status;  // FEATURES 需要 FEATURES 邻居

// ✅ 正确：检查父阶段的邻居
const ChunkStatus* requiredStatus = status.parent();  // FEATURES 需要 CARVERS 邻居
```

**验证方法**：在 `placeFeaturesForStage()` 中添加日志，确认被调用。

### 🚨 陷阱 4：生物群系没有设置 generationSettings

**问题**：创建了生物群系但没有设置 `BiomeGenerationSettings`，导致该生物群系没有特征。

```cpp
// ❌ 错误：没有设置生成设置
Biome createForest() {
    Biome biome(Biomes::Forest, "forest");
    biome.setDepth(0.1f);
    biome.setTemperature(0.7f);
    // 忘记调用 setGenerationSettings！
    return biome;
}

// ✅ 正确：设置生成设置
Biome createForest() {
    Biome biome(Biomes::Forest, "forest");
    biome.setDepth(0.1f);
    biome.setTemperature(0.7f);
    biome.setGenerationSettings(BiomeGenerationSettings::createForest());  // 必须！
    return biome;
}
```

### 🚨 陷阱 5：HeightRangePlacementConfig 参数错误

**问题**：构造函数参数顺序混淆。

```cpp
// HeightRangePlacementConfig(bottomOffset, topOffset, maximum)
//
// bottomOffset: 从 Y=0 开始的偏移
// topOffset: 从最大高度向下的偏移
// maximum: 最大高度

// ❌ 错误：想要 Y 0-127，但写成了 (0, 128, 0)
auto config = HeightRangePlacementConfig(0, 128, 0);  // 范围是 [0, 0-128] = 空！

// ✅ 正确：Y 0-127
auto config = HeightRangePlacementConfig(0, 0, 128);  // 范围是 [0, 128)
```

### 🚨 陷阱 6：WorldGenRegion 访问越界

**问题**：在 `place()` 中访问超出区块范围的位置。

```cpp
bool MyFeature::place(...) {
    // ❌ 可能越界：WorldGenRegion 只包含当前区块和 8 个邻居
    for (int y = 0; y < 300; ++y) {  // Y 可能超出高度限制
        region.getBlockState(pos.x, y, pos.z);
    }

    // ✅ 正确：检查边界
    constexpr i32 MIN_Y = 0;
    constexpr i32 MAX_Y = 255;

    for (i32 y = MIN_Y; y <= MAX_Y; ++y) {
        const BlockState* state = region.getBlockState(pos.x, y, pos.z);
        // ...
    }
}
```

### 🚨 陷阱 7：忘记注册到 FeatureRegistry

**问题**：创建了 Feature 类和工厂，但忘记在 `FeatureRegistry::initialize()` 中注册。

```cpp
// ❌ 忘记调用 MyFeatures::initialize()
void FeatureRegistry::initialize() {
    clear();
    OreFeatures::initialize();
    // MyFeatures::initialize();  // 忘记这行！

    // 即使写了下面的代码，MyFeatures::getAllFeaturesAndClear() 也会返回空
}
```

### 🚨 陷阱 8：clone() 方法未实现

**问题**：配置类包含 `unique_ptr` 成员，需要实现深拷贝。

```cpp
// ❌ 使用默认拷贝构造函数
struct TreeFeatureConfig : public IFeatureConfig {
    std::unique_ptr<TrunkPlacer> trunkPlacer;  // unique_ptr 不能拷贝！
    // 默认拷贝构造会失败
};

// ✅ 实现深拷贝
struct TreeFeatureConfig : public IFeatureConfig {
    std::unique_ptr<TrunkPlacer> trunkPlacer;

    TreeFeatureConfig(const TreeFeatureConfig& other) {
        if (other.trunkPlacer) {
            trunkPlacer = other.trunkPlacer->clone();  // 需要 clone() 方法
        }
    }
};
```

---

## 完整示例：添加新矿石

```cpp
// 1. 在 OreFeature.cpp 中添加工厂方法
std::unique_ptr<ConfiguredOreFeature> OreFeatures::createAmethystOre() {
    auto config = std::make_unique<OreFeatureConfig>(
        createOreTarget(OreTargetType::NaturalStone),
        BlockId::AmethystOre,  // 假设已定义
        8);  // 矿脉大小

    auto heightPlacement = std::make_unique<HeightRangePlacement>();
    auto heightConfig = std::make_unique<HeightRangePlacementConfig>(0, 64, 64);  // Y 0-63

    auto squarePlacement = std::make_unique<SquarePlacement>();
    auto squareConfig = std::make_unique<EmptyPlacementConfig>();

    auto countPlacement = std::make_unique<CountPlacement>();
    auto countConfig = std::make_unique<CountPlacementConfig>(2);  // 每区块 2 次

    // 链：count -> square -> height
    auto heightConfigured = std::make_unique<ConfiguredPlacement>(
        std::move(heightPlacement), std::move(heightConfig));
    auto squareConfigured = std::make_unique<ConfiguredPlacement>(
        std::move(squarePlacement), std::move(squareConfig));
    auto countConfigured = std::make_unique<ConfiguredPlacement>(
        std::move(countPlacement), std::move(countConfig));

    squareConfigured->setNext(std::move(heightConfigured));
    countConfigured->setNext(std::move(squareConfigured));

    return std::make_unique<ConfiguredOreFeature>(
        std::move(config), std::move(countConfigured), "amethyst_ore");
}

// 2. 在 initialize() 中注册
void OreFeatures::initialize() {
    s_features.clear();
    s_features.push_back(createCoalOre());
    // ... 其他矿石
    s_features.push_back(createAmethystOre());  // 新增
}

// 3. 添加到生物群系
BiomeGenerationSettings BiomeGenerationSettings::createMountains() {
    BiomeGenerationSettings settings = createDefault();
    settings.addFeature(DecorationStage::UndergroundOres, 6);  // 绿宝石
    settings.addFeature(DecorationStage::UndergroundOres, 8);  // 紫水晶（新ID）
    settings.addFeature(DecorationStage::VegetalDecoration, 2);  // 云杉
    return settings;
}
```

---

## 完整示例：添加新树木

```cpp
// 1. 创建树木配置
TreeFeatureConfig TreeFeatures::acaciaConfig() {
    TreeFeatureConfig config;
    config.trunkBlock = BlockId::AcaciaLog;
    config.foliageBlock = BlockId::AcaciaLeaves;
    config.trunkPlacer = std::make_unique<StraightTrunkPlacer>(4, 2, 0);
    config.foliagePlacer = std::make_unique<BlobFoliagePlacer>(
        FeatureSpread::spread(2, 1),
        FeatureSpread::fixed(0),
        2
    );
    config.minHeight = 4;
    return config;
}

// 2. 创建配置化特征
std::unique_ptr<ConfiguredTreeFeature> TreeFeatures::createAcaciaTree() {
    auto config = std::make_unique<TreeFeatureConfig>(acaciaConfig());

    auto surfacePlacement = std::make_unique<SurfacePlacement>();
    auto surfaceConfig = std::make_unique<SurfacePlacementConfig>(0, false);

    auto countPlacement = std::make_unique<CountPlacement>();
    auto countConfig = std::make_unique<CountPlacementConfig>(2);

    auto surfaceConfigured = std::make_unique<ConfiguredPlacement>(
        std::move(surfacePlacement), std::move(surfaceConfig));
    auto countConfigured = std::make_unique<ConfiguredPlacement>(
        std::move(countPlacement), std::move(countConfig));

    countConfigured->setNext(std::move(surfaceConfigured));

    return std::make_unique<ConfiguredTreeFeature>(
        std::move(config), std::move(countConfigured), "acacia_tree");
}

// 3. 注册
void TreeFeatures::initialize() {
    s_features.clear();
    s_features.push_back(createOakTree());      // ID 0
    s_features.push_back(createBirchTree());    // ID 1
    s_features.push_back(createSpruceTree());   // ID 2
    s_features.push_back(createJungleTree());   // ID 3
    s_features.push_back(createSparseOakTree());// ID 4
    s_features.push_back(createAcaciaTree());   // ID 5 (新增)
}

// 4. 添加到热带草原生物群系
BiomeGenerationSettings BiomeGenerationSettings::createSavanna() {
    BiomeGenerationSettings settings = createDefault();
    settings.addFeature(DecorationStage::VegetalDecoration, 5);  // 金合欢树
    return settings;
}
```

---

## 调试技巧

### 1. 检查特征注册

```cpp
// 在 FeatureRegistry::initialize() 中添加日志
spdlog::info("[FeatureRegistry] Initialized features:");
for (size_t i = 0; i < m_featuresByStage.size(); ++i) {
    if (!m_featuresByStage[i].empty()) {
        spdlog::info("  Stage {}: {} features", i, m_featuresByStage[i].size());
    }
}
```

### 2. 检查放置链调用

```cpp
bool ConfiguredMyFeature::place(...) {
    std::vector<BlockPos> positions = m_placement->getPositions(region, random, pos);
    spdlog::debug("[{}] Got {} positions from placement chain", m_name, positions.size());
    // ...
}
```

### 3. 检查生物群系特征

```cpp
// 在 BiomeFeaturePlacer::placeFeaturesForStage() 中添加日志
const auto& featureIds = settings.getFeatures(stage);
if (!featureIds.empty()) {
    spdlog::debug("Stage {}: {} features requested", static_cast<int>(stage), featureIds.size());
}
```

---

## 参考资料

- MC 1.16.5 源码：`net/minecraft/world/gen/feature/` 目录
- 项目现有实现：`OreFeature.cpp` 和 `TreeFeature.cpp`
- 放置器系统：`Placement.hpp/cpp`
