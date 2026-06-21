# 地表构建系统 (Surface System)

## 目录结构

```
src/common/world/gen/surface/
├── CaveSurface.hpp              # 地表/洞穴方向枚举（Floor, Ceiling）
├── VerticalAnchor.hpp/cpp       # Y 坐标锚点（Absolute, AboveBottom, BelowTop）
├── SurfaceCondition.hpp/cpp     # SurfaceRules 条件基类及所有条件实现
├── SurfaceRule.hpp/cpp          # SurfaceRules 规则基类及所有规则实现
├── SurfaceRuleContext.hpp/cpp   # SurfaceRules 上下文（维护当前位置状态）
├── SurfaceSystem.hpp/cpp        # SurfaceRules 执行器（遍历区块应用规则）
├── SurfaceRulesFactory.hpp/cpp  # 条件/规则工厂函数 + 维度规则树（overworld/nether/end）
├── SurfaceRules.hpp             # 聚合头文件（包含以上所有头文件）
├── Surface.hpp                  # 外部聚合头文件（包含 SurfaceRules.hpp）
└── README.md                    # 本文档
```

## 内部模块关系

```
SurfaceRules.hpp（聚合头文件）
    ├── CaveSurface.hpp
    ├── VerticalAnchor.hpp
    ├── SurfaceCondition.hpp → SurfaceRuleContext.hpp（前向声明）
    ├── SurfaceRule.hpp → SurfaceCondition.hpp, SurfaceRuleContext.hpp
    ├── SurfaceRuleContext.hpp → VerticalAnchor.hpp, Biomes.hpp, BlockState.hpp
    ├── SurfaceSystem.hpp → SurfaceRule.hpp, PositionalRandomFactory.hpp
    └── SurfaceRulesFactory.hpp → SurfaceCondition.hpp, SurfaceRule.hpp

依赖方向：
    SurfaceRulesFactory → SurfaceCondition, SurfaceRule → SurfaceRuleContext → VerticalAnchor, CaveSurface
    SurfaceSystem → SurfaceRule, SurfaceRuleContext

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
| `HeightProvider` | 使用 VerticalAnchor 解析 Y 坐标 |
| `CarverConfiguration` | 使用 VerticalAnchor 作为 lavaLevel |
| `StructureDefinitionLoader` | 使用 VerticalAnchor 解析高度提供者 |
| `RandomState` | 持有 SurfaceSystem 实例 |

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
