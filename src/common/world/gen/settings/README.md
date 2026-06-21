# Settings 模块 - 世界生成设置

本模块包含世界生成的配置设置，参考 Minecraft 1.21.11 的 NoiseGeneratorSettings 和 FlatLevelGeneratorSettings 实现。

## 目录结构

```
settings/
├── DimensionSettings.cpp       # 维度设置预设实现（主世界/大型群系/放大化/下界/末地/洞穴/浮岛/平坦）
├── DimensionSettings.hpp       # 维度设置定义（默认方块、流体、海平面等）+ DimensionKind 枚举
├── FlatLayerInfo.hpp           # 平坦层信息（方块 + 高度）
├── FlatLevelGeneratorSettings.cpp # 平坦世界设置实现
├── FlatLevelGeneratorSettings.hpp # 平坦世界设置定义（含 FillLayerEntry）
├── NoiseSettings.hpp           # 噪声设置定义（地形噪声参数、尺寸、密度、滑动、预设 + guardY 验证）
├── ScalingSettings.hpp         # 缩放设置定义（噪声在 XZ/Y 轴的缩放比例）
├── SlideSettings.hpp           # 滑动设置定义（地形边界平滑过渡参数）
└── Settings.hpp                # 总头文件（聚合所有设置类型）
```

## 内部模块关系

```
Settings.hpp（总入口）
├── DimensionSettings（维度设置）
│   └── NoiseSettings（噪声设置）
│       ├── ScalingSettings（缩放设置）
│       └── SlideSettings（滑动设置）
├── FlatLevelGeneratorSettings（平坦世界设置）
│   ├── FlatLayerInfo（平坦层信息）
│   └── FillLayerEntry（填充层条目，非运动阻挡层的延迟放置信息）
└── ScalingSettings / SlideSettings
```

**依赖链**：`SlideSettings` + `ScalingSettings` → `NoiseSettings` → `DimensionSettings`

## 上下游外部依赖关系

### 本模块依赖

| 依赖项 | 用途 |
|--------|------|
| `common/core/Types.hpp` | 基础类型 i32、f32 等 |
| `common/world/WorldConstants.hpp` | world::MAX_BUILD_HEIGHT、world::SEA_LEVEL 等 |
| `common/world/block/Block.hpp` | BlockState 前向声明 |
| `common/world/block/BlockRegistry.hpp` | 方块注册表访问 |
| `common/world/block/registry/VanillaBlocks.hpp` | 原版方块定义 |
| `common/world/biome/BiomeIds.hpp` | Biomes::Plains 等生物群系 ID |

### 被依赖

| 使用者 | 用途 |
|--------|------|
| `world/gen/FlatChunkGenerator` | 平坦世界区块生成器，核心使用者 |
| `world/gen/NoiseChunkGenerator` | 噪声区块生成器 |
| `world/gen/RandomState` | 根据 DimensionKind 选择 NoiseRouter 和 SurfaceRules |
| `server/ServerDimensionManager` | 服务端维度初始化 |

## 容易踩的坑

### 1. BlockState 指针有效性

`DimensionSettings::overworld()` 等预设方法在调用时从 `VanillaBlocks::getState()` 获取 `BlockState*`。**必须在方块注册完成后调用**，否则返回 `nullptr`。

### 2. NoiseSettings 默认值不适合所有维度

`NoiseSettings` 的默认构造值是主世界参数，其他维度需使用对应预设：

```cpp
// ❌ 错误：默认值用于下界
NoiseSettings noise; // 主世界参数

// ✅ 正确：使用预设
NoiseSettings noise = NoiseSettings::nether();
```

### 3. 海平面高度与噪声高度的配合

`seaLevel` 必须在噪声高度范围内，否则水体生成异常。`floatingIslands()` 的 `seaLevel = -64` 是 MC 特意设计的。

### 4. DensityKind 枚举与 switch 语句

添加新 `DimensionKind` 枚举值后，必须同时更新以下 switch 语句：
- `RandomState::create()` — NoiseRouter 选择
- `RandomState::create()` — SurfaceRules 选择
- `RandomState::createRouterCopy()` — 路由器副本创建
- `NoiseChunkGenerator` 构造函数 — 流体选择器选择

### 5. oreVeinsEnabled 字段

矿脉生成由 `DimensionSettings::oreVeinsEnabled` 字段控制，而非硬编码的 `dimensionKind` 判断。所有新维度预设必须正确设置此字段。

### 6. NoiseSettings::create() 验证

`NoiseSettings::create()` 工厂方法会在创建时验证参数合法性（height 是 16 的倍数、minY 是 16 的倍数、sizeHorizontal/sizeVertical 在 [1, 4] 范围内）。预设方法（overworld/nether/end 等）不经过此验证，因为它们已知合法。

### 7. FillLayerEntry 与 updateLayers() 的配合

`FlatLevelGeneratorSettings::updateLayers()` 会将非运动阻挡方块（如水）在展开层列表中替换为 `nullptr`，同时记录到 `fillLayerEntries()` 列表中。`FlatChunkGenerator::placeFeatures()` 在特性放置完成后，通过 `_placeFillLayers()` 将这些位置的空气方块替换为原始方块状态。如果只修改 `layers()` 而不调用 `updateLayers()`，`fillLayerEntries` 将不会更新。
