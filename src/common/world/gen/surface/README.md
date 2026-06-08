# 地表构建系统 (Surface System)

## 目录结构

```
src/common/world/gen/surface/
├── Surface.hpp              # 聚合头文件，包含 SurfaceRules
├── SurfaceRules.hpp         # MC 1.21 SurfaceRules 条件/规则系统
├── SurfaceRules.cpp         # SurfaceRules 实现
└── README.md                # 本文档
```

## 模块整体职责

地表构建系统负责在区块生成过程中，根据生物群系类型和噪声值，将默认方块（石头）替换为适合该生物群系的地表方块（草方块、沙子、雪等）和次地表方块（泥土、砂岩等）。

本项目使用 MC 1.21 的 SurfaceRules 规则树系统，通过条件+规则组合实现灵活的地表生成。

## 内部模块关系

```
Surface[Surface.hpp<br/>聚合头文件]
    └── SurfaceRules[SurfaceRules.hpp<br/>规则系统]
         ├── SurfaceCondition（条件）
         │   ├── StoneDepthCondition
         │   ├── YCondition
         │   ├── WaterCondition
         │   ├── BiomeCondition
         │   ├── NoiseThresholdCondition
         │   ├── VerticalGradientCondition
         │   ├── SteepCondition
         │   ├── TemperatureCondition
         │   └── HoleCondition
         ├── SurfaceRule（规则）
         │   ├── IfTrueRule
         │   ├── SequenceRule
         │   ├── BlockRule
         │   └── BandlandsRule
         ├── SurfaceRuleContext（上下文）
         └── SurfaceSystem（规则执行器）

外部依赖:
    ChunkPrimer ← SurfaceSystem 读取/写入方块
    Biome ← BiomeCondition 判断
    BlockState/VanillaBlocks ← BlockRule 输出
    NormalNoise ← NoiseThresholdCondition 判断
    NoiseChunkGenerator ← 调用 SurfaceSystem.buildSurface()
```

## 上下游外部依赖关系

### 上游依赖（本模块依赖）

| 模块 | 用途 |
|------|------|
| `common/core/Types.hpp` | 基础类型定义 (i32, f32, u64 等) |
| `common/world/block/BlockState.hpp` | 方块状态 |
| `common/world/block/VanillaBlocks.hpp` | 原版方块定义 |
| `common/world/chunk/ChunkPrimer.hpp` | 区块数据 |
| `common/world/biome/Biome.hpp` | 生物群系定义 |
| `common/world/gen/noise/NormalNoise.hpp` | 正态噪声 |

### 下游依赖（依赖本模块）

| 模块 | 用途 |
|------|------|
| `NoiseChunkGenerator` | 区块生成器在 SURFACE 阶段调用地表构建 |

## 核心类型

### SurfaceRules（MC 1.21 规则树系统）

基于规则树的声明式地表生成系统：

- **条件（SurfaceCondition）**：`StoneDepthCondition`, `YCondition`, `WaterCondition`, `BiomeCondition`, `NoiseThresholdCondition`, `VerticalGradientCondition`, `SteepCondition`, `TemperatureCondition`, `HoleCondition`
- **规则（SurfaceRule）**：`IfTrueRule`, `SequenceRule`, `BlockRule`, `BandlandsRule`
- **上下文（SurfaceRuleContext）**：维护当前位置的状态（stoneDepth、waterHeight、biome等）
- **SurfaceSystem**：规则执行器，遍历区块每个方块应用规则

工厂命名空间 `SurfaceRules` 提供便捷创建方法：`onFloor()`, `underFloor()`, `isBiome()`, `ifTrue()`, `sequence()`, `overworld()`, `nether()`, `end()`

每个维度有独立的 SurfaceRules 配置：
- **主世界**：`SurfaceRules::overworld(seed)` — 包含草地、沙子、雪、石山、恶地等规则
- **下界**：`SurfaceRules::nether(seed)` — 包含基岩底/顶、5种下界生物群系
- **末地**：`SurfaceRules::end()` — 全末地石

## 容易踩的坑

### 1. 方块状态指针检查

规则执行器内部使用 `VanillaBlocks::getState()` 获取方块状态，某些方块可能未定义。规则执行时会检查方块状态是否为空，静默跳过。

### 2. 恶地色带连续性

`BandlandsRule` 的陶瓦色带依赖**世界坐标**，如果误用区块内坐标会在区块边界出现明显断层。正确做法：使用 `chunk.x()*16 + x`、`chunk.z()*16 + z` 计算世界坐标。

### 3. 区块坐标范围

`buildSurface()` 的 `x` 和 `z` 参数是区块内坐标 (0-15)，不是世界坐标。调用时需确保正确的坐标转换。

### 4. SurfaceRules 条件组合顺序

`SequenceRule` 按顺序尝试规则，返回第一个非空结果。规则顺序直接影响生成结果，确保规则优先级正确。

### 5. 基岩生成由 SurfaceRules 驱动

MC 1.21 中基岩不再由区块生成器直接放置，而是通过 SurfaceRules 的 `VerticalGradientCondition` 规则实现。主世界和下界的基岩层均通过此机制生成。
