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
| `world/gen/NoiseChunkGenerator` | 噪声区块生成器，读取 `spawnTarget` 传给 `NoiseChunk::cachedClimateSampler`；通过 `randomState()` 暴露 `RandomState`（其 `sampler()` 已设置 `spawnTarget`） |
| `world/gen/RandomState` | 根据 `DimensionKind` 选择 NoiseRouter 和 SurfaceRules；`create()` 时将 `settings.spawnTarget` 设置到 `Climate::Sampler` 上 |
| `server/world/ServerWorld` | `initializeWorldSpawn()` 通过 `NoiseChunkGenerator::randomState()` 获取 `Sampler`，调用 `findSpawnPosition()` 进行气候搜索出生点 |
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
- `NoiseChunkGenerator` 构造函数 — 流体选择器选择

### 5. oreVeinsEnabled 字段

矿脉生成由 `DimensionSettings::oreVeinsEnabled` 字段控制，而非硬编码的 `dimensionKind` 判断。所有新维度预设必须正确设置此字段。

### 6. NoiseSettings::create() 验证

`NoiseSettings::create()` 工厂方法会在创建时验证参数合法性（height 是 16 的倍数、minY 是 16 的倍数、sizeHorizontal/sizeVertical 在 [1, 4] 范围内）。预设方法（overworld/nether/end 等）不经过此验证，因为它们已知合法。

### 7. FillLayerEntry 与 updateLayers() 的配合

`FlatLevelGeneratorSettings::updateLayers()` 会将非运动阻挡方块（如水）在展开层列表中替换为 `nullptr`，同时记录到 `fillLayerEntries()` 列表中。`FlatChunkGenerator::placeFeatures()` 在特性放置完成后，通过 `_placeFillLayers()` 将这些位置的空气方块替换为原始方块状态。如果只修改 `layers()` 而不调用 `updateLayers()`，`fillLayerEntries` 将不会更新。

### 8. structureOverrides 结构生成覆盖

`FlatLevelGeneratorSettings` 新增 `structureOverrides` 字段（`std::vector<ResourceLocation>`），用于控制平坦世界中允许生成的结构集：

- **空列表**：不生成任何结构
- **非空列表**：仅生成指定的结构集（白名单），受 `_hasBiomesForStructureSet()` 生物群系兼容性过滤

MC 1.21.11 原版中 `structureOverrides` 为 `Optional<HolderSet<StructureSet>>`，`Optional.empty` 表示使用所有结构集，`Optional.present` 表示仅使用指定集合。项目简化为空列表=不生成结构，以与 `hasDecoration`/`hasLakes` 的行为保持一致。

`createDefault()` 默认配置启用 `minecraft:villages` 和 `minecraft:strongholds`，对齐 MC 原版超平坦世界预设。

### 9. spawnTarget 字段（出生点气候目标）

`DimensionSettings` 新增 `spawnTarget` 字段（`std::vector<world::biome::climate::ParameterPoint>`），对齐 MC 1.21.11 `NoiseGeneratorSettings.spawnTarget`，用于 `Climate::Sampler::findSpawnPosition()` 在气候空间中径向搜索最佳出生点。

**预设填充规则**：

| 预设 | spawnTarget 内容 | 来源 |
|------|-----------------|------|
| `overworld()` | 2 个 ParameterPoint | `OverworldBiomeBuilder::spawnTarget()` |
| `largeBiomesPreset()` | 2 个 ParameterPoint | 同上 |
| `amplified()` | 2 个 ParameterPoint | 同上 |
| `nether()` / `end()` / `caves()` / `floatingIslands()` / `flat()` | 空列表 | 无（沿用 (0,0) 区块作为出生点） |

**ParameterPoint 结构**（主世界 2 项）：
- `temperature`/`humidity`/`erosion`：`m_fullRange`（全范围）
- `continentalness`：`Parameter::span(m_inlandContinentalness, m_fullRange)`（内陆到全范围）
- `depth`：`Parameter::point(0.0f)`（地表层）
- `weirdness`：以 `±0.16` 分割为两项（负奇异性 / 正奇异性）
- `offset`：0

**数据流**：
```
OverworldBiomeBuilder::spawnTarget()
    → DimensionSettings::overworld().spawnTarget
    → RandomState::create() 调用 sampler->setSpawnTarget(settings.spawnTarget)
    → Climate::Sampler 持有 spawnTarget
    → ServerWorld::initializeWorldSpawn() 调用 sampler.findSpawnPosition()
    → 在气候空间中径向搜索最佳出生点
```

**注意**：`spawnTarget` 仅影响出生点查找，不影响生物群系分布。空 `spawnTarget` 表示使用 (0,0) 区块作为出生点（对齐 MC 下界/末地行为）。
