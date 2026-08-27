# density/ — MC 1.18+ 密度函数系统

## 概述

MC 1.18+ 引入的密度函数系统，用于地形生成和气候参数计算。
密度函数是可组合的表达式树，每个节点接收方块坐标 (x, y, z) 返回一个密度值。

NoiseRouter 持有 15 个密度函数引用，其中 6 个用于 Climate.Sampler。

## 目录结构

```
density/
├── DensityFunction.hpp              — 密度函数核心接口
├── DensityFunctions.hpp             — 所有密度函数实现类 + 工厂函数
├── DensityFunctions.cpp             — 实现
├── BlendedNoise.hpp                 — MC 1.18+ 混合噪声密度函数（旧式三层 Perlin）
├── BlendedNoise.cpp                 — 实现
├── Beardifier.hpp                   — 结构物地形修饰器（Beard/Bury 贡献计算）
├── Beardifier.cpp                   — 实现
├── TerrainProvider.hpp              — 主世界地形样条数据（offset/factor/jaggedness）
├── TerrainProvider.cpp              — 实现
├── OreVeinifier.hpp                 — 矿脉生成器（铜/铁矿脉）
├── OreVeinifier.cpp                 — 实现
├── NoiseRouter.hpp                  — 噪声路由器（持有 15 个密度函数）
├── NoiseRouter.cpp                  — 实现
├── NoiseBindingVisitor.hpp          — 数据驱动噪声叶子绑定访问器（UnboundNoiseLeaf→真实叶子）
├── NoiseBindingVisitor.cpp          — 实现
├── DensityFunctionLoader.hpp        — density_function JSON 加载 + Holder 引用解析
├── DensityFunctionLoader.cpp        — 实现
├── DensityFunctionRegistry.hpp      — name→shared_ptr<DensityFunction> 注册表
├── DensityFunctionRegistry.cpp      — 实现
├── DensityFunctionTypeRegistry.hpp  — type→工厂映射（数据驱动多态分发）
├── DensityFunctionTypeRegistry.cpp  — 实现
├── NoiseChunk.hpp                   — 区块噪声采样单元（三线性插值 + 缓存管理）
├── NoiseChunk.cpp                   — 实现
├── ast/                             — 密度函数 AST 编译器子系统（DFC 风格扁平指令序列求值器 + asmjit JIT，详见 ast/README.md）
└── README.md                        — 本文件
```

## 内部模块关系

```
NoiseRouter ──持有──→ 15 个 DensityFunction
     │
     └──createClimateSampler()──→ Climate.Sampler
                                     │
Climate.Sampler ──持有──→ 6 个 DensityFunction 引用
                           (temperature, vegetation, continents, erosion, depth, ridges)

NoiseChunk ──包装──→ DensityFunction (Interpolated/CacheAllInCell/CacheOnce)
     │
     └──持有──→ NoiseInterpolator[] (三线性插值器)
     └──持有──→ CellCache[] (cell 内缓存)
     └──持有──→ Aquifer (含水层采样器)

DensityFunction 实现链:
NoiseDensity ──持有──→ NormalNoise ──持有──→ 2 × PerlinNoise ──持有──→ PerlinLayer[]
ShiftedNoise ──持有──→ NormalNoise + 3 × DensityFunction (shiftX/Y/Z)
BlendedNoise ──持有──→ 3 × PerlinNoise (minLimit/maxLimit/main) + PerlinLayer[]
EndIslands ──持有──→ SimplexNoise
```

## 外部依赖关系

### 依赖

- `common/world/gen/noise/NormalNoise.hpp` — 噪声采样
- `common/world/gen/noise/PerlinNoise.hpp` — Perlin 噪声
- `common/world/gen/noise/SimplexNoise.hpp` — Simplex 噪声（EndIslands）
- `common/world/biome/climate/Climate.hpp` — Climate.Sampler 创建
- `common/world/WorldConstants.hpp` — MIN_BUILD_HEIGHT, MAX_BUILD_HEIGHT
- `common/util/math/MathUtils.hpp` — 数学工具

### 被依赖

- `common/world/biome/source/MultiNoiseBiomeSource` — 持 `const Sampler&`（由 `RandomState.sampler()` 提供，Sampler 内部含 NoiseRouter 的 6 个气候函数）
- `common/world/biome/source/EndBiomeSource` — 使用 Climate.Sampler.erosion 区分末地生物群系
- `common/world/gen/chunk/NoiseChunkGenerator` — 使用 finalDensity 地形生成

## 维度密度函数配置

| 维度 | finalDensity 管线 |
|------|-------------------|
| 主世界 | `slideOverworld(postProcess(depth + continents + 0.5*erosion + 0.5*ridgesPV))` |
| 下界 | `noNewCaves(slideNetherLike(blendedNoise + yClampedGradient(0,128,1.5,-1.5)))` |
| 末地 | `postProcess(slideEndLike(cache2d(endIslands) + blendedNoise_end))` |

其中 BlendedNoise 参数：
- 主世界: xzScale=0.25, yScale=0.125, xzFactor=80, yFactor=160, smear=8
- 下界: xzScale=0.25, yScale=0.375, xzFactor=80, yFactor=60, smear=8
- 末地: xzScale=0.25, yScale=0.25, xzFactor=80, yFactor=160, smear=4

## 关键算法对齐要点（MC 1.21.11）

### 1. YClampedGradient toY 值

主世界 `depthPlusOffset` 中的 `YClampedGradient` 范围为 `[MIN_BUILD_HEIGHT, MAX_BUILD_HEIGHT]`（即 [-64, 320]），toY 使用 `MAX_BUILD_HEIGHT`（320），而非 `MAX_BUILD_HEIGHT - 1`（319）。

### 2. entrances 无 min(5.0) 包装

MC 原版 `entrances()` 直接返回 `add(slopedCheese, mul(5.0, entrancesFunc))` 的结果，不使用 `min(5.0, ...)` 裁剪。该密度子树现由数据驱动 `density_function` JSON（`minecraft:overworld/caves/entrances` 等）提供，经 `DensityFunctionLoader` 解析注册。

### 3. EndIslands float 精度

Java 的 `EndIslandDensityFunction.getHeightValue()` 使用 `float` 算术：
- `Mth.sqrt(float)` 返回 float，不是 double
- `Mth.abs((float)k1)` 先转 float 再取绝对值（大数精度截断）
- 阈值使用 `-0.9f`（约 -0.89999998）而非 `-0.9`（double）
- 整数除法和取模使用 `/` 和 `%`（负数向零取整），而非 `>>` 和 `&`（负数行为不同）

### 4. DensityFunctions::Invert 边界

`Invert`（1/x）的边界值在输入范围跨越零点时应使用 `±infinity`，而非 `±1e6`。当输入为 0 时返回 `+infinity`（IEEE 754）。

### 5. Mapped 边界计算（Abs, Square, Cube）

- `Abs`: `minValue = max(0.0, input.minValue)`（MC 对齐修复：不是 `max(0, min(|min|, |max|))`）
- `Square`: `minValue = max(0.0, input.minValue)`, `maxValue = max(min², max²)`
- `Cube`: `minValue = min³`, `maxValue = max³`（立方在跨零时 minValue < 0）

### 6. MappedNoise 边界计算

MC 1.21 使用 `midpoint ± |halfAmplitude| * noise.maxValue()` 公式：
- `midpoint = (fromValue + toValue) / 2`
- `halfAmplitude = (toValue - fromValue) / 2`
- 不论 fromValue 与 toValue 的大小关系，结果始终关于 midpoint 对称

### 7. WeirdScaledSampler 边界计算

- `minValue = 0.0`（因为 compute 使用了 abs）
- `maxValue = maxRarity * noise.maxValue()`，其中 Type1 的 `maxRarity = 2.0`，Type2 的 `maxRarity = 3.0`

### 8. 洞穴噪声参数（数据驱动 noise JSON）

MC 1.21.11 关键修正（参数现由数据驱动 `noise` JSON 提供，经 `NoiseLoader` 加载到 `Noises` 注册表）：
- `SPAGHETTI_2D_MODULATOR`: firstOctave=-11（非 -2）, amplitudes={1.0}（非 {2.0, 1.0}）
- `SPAGHETTI_2D`: firstOctave=-7（非 -2）, amplitudes={1.0}（非 {2.0, 1.0}）
- `SPAGHETTI_ROUGHNESS`: firstOctave=-5, amplitudes={1.0}（非 {1.0, 1.0, 1.0, 1.0}）

### 9. 缓存层包装（数据驱动 noise_router 模板）

MC 1.21 中各密度的缓存层不同（由数据驱动 `noise_settings` JSON 的 `noise_router` 字段经 `SurfaceRuleDeserializer`/`DensityFunctionLoader` 解析）：
- `temperature`/`vegetation`: 不缓存（直接 shiftedNoise2d）
- `continents`/`erosion`/`ridges`: flatCache（非 cache2D）
- `endIslands`: 种子参数始终为 0（非世界种子）

**FlatCache 区块级预计算**：`FlatCache` 对齐原版 `NoiseChunk.FlatCache`（NoiseChunk.java:619-665），
在 `NoiseChunk::apply()` 替换 Marker 时注入区块几何（`firstNoiseX/Z`、`noiseSizeXZ`）并
`precompute=true`，构造期双 for 预计算整张 quart XZ 网格到 `values[(sizeXZ+1)²]` 数组，
之后 `compute()` 退化为 O(1) 数组查表（越界回退 `m_input->compute`）。主世界区块
`cellCountXZ=4,cellWidth=4` → `noiseSizeXZ=4`，数组 25 项。

此优化的收益点：`GenerateBiomes_MultiNoiseBiomeSource` 的 1536 点气候采样循环中，
continents/erosion/ridges 从逐点全精度 `NormalNoise::getValue`（单值缓存 0 命中）
变为 25 点预计算 + 1536 次 O(1) 查表。temperature/vegetation 虽然本身不缓存，
但其 shiftX/shiftZ 子函数命中同一 FlatCache 数组，省掉 shiftA/shiftB 重算。

非 NoiseChunk 上下文（如 `factory::flatCache` 非 Marker 直接构造、零散 getHeight 查询）
无区块几何，`precompute=false` 退化为单值 lastPos 缓存，保证正确性。

### 10. BeardifierMarker

- `BeardifierMarker`：Marker 类型，在 NoiseChunk::apply() 中替换为实际 Beardifier（当前暂返回 0.0）

### 11. preliminarySurfaceLevel

已完整实现，对齐 MC 1.21.11 `preliminarySurfaceLevel(offset, factor, amplified)`。使用 `FindTopSurface` 密度函数从 upperBound 向下搜索 density > 0 的位置，返回第一个满足条件的 Y 坐标（cellHeight=8）。实现包含 `remap`、`offsetToDepth` 辅助函数和 `FindTopSurface` 密度函数类。`amplified` 参数通过 `slideOverworld` 影响顶部和底部 slide 范围。该密度函数现由数据驱动 `noise_settings` JSON 的 `noise_router.preliminary_surface_level` 字段提供。

**影响范围**：preliminarySurfaceLevel 影响含水层水位采样（`NoiseChunk::samplePreliminarySurfaceLevel`），进而影响 `m_skipSamplingAboveY` 和含水层条带高度。海洋海平面以下的水仍由 `NoiseBasedAquifer::computeSubstance` 快速路径直接填充全局流体，与 preliminarySurfaceLevel 无关。

### 12. NoiseChunk::cachedClimateSampler 与 spawnTarget

`NoiseChunk::cachedClimateSampler(spawnTarget)` 对齐 MC 1.21.11 `NoiseChunk.cachedClimateSampler(router, spawnTarget)`，将 `spawnTarget` 传递给 `Climate::Sampler` 用于出生点查找。

**行为**：
- 首次调用时构造 `Sampler`，并立即调用 `sampler->setSpawnTarget(spawnTarget)` 设置出生点气候目标
- 后续调用若 `spawnTarget` 与缓存中的不同，则刷新 `sampler->spawnTarget()`（对齐 MC 行为，避免使用过期的 spawnTarget）
- 返回 `std::shared_ptr<climate::Sampler>`，调用方持有共享所有权

**调用方**：
- `NoiseChunkGenerator::generateBiomes()` 通过 `noiseChunk.cachedClimateSampler(m_settings.spawnTarget)` 获取 Sampler，并使用其 `sample()` 方法进行生物群系采样
- `ServerWorld::initializeWorldSpawn()` 不直接使用 NoiseChunk 的 Sampler，而是通过 `NoiseChunkGenerator::randomState()->sampler()` 访问（该 Sampler 在 `RandomState::create()` 时已设置 `spawnTarget`）

**重要**：`spawnTarget` 参数不再被忽略。先前实现有 `(void)spawnTarget;` 占位 TODO，现已完整实现数据流：`DimensionSettings::spawnTarget` → `RandomState::create()` → `Sampler::setSpawnTarget()` → `findSpawnPosition()`。

## 容易踩的坑

1. **NormalNoise 种子**：两个 PerlinNoise 实例必须使用不同种子（第二个偏移 0xDEADBEEF）
2. **INPUT_FACTOR = 1.018...**：第二个 Perlin 噪声的坐标乘以此因子，不能遗漏
3. **YClampedGradient 范围**：主世界使用 [-64, 320]，下界/末地使用 [0, 128]
4. **ShiftA vs ShiftB**：ShiftA 使用 (x, 0, z)，ShiftB 使用 (z, x, 0)，坐标顺序不同
5. **peaksAndValleys 公式**：`mul(-3, add(abs(add(abs(ridges), -2/3)), -1/3))` 需要精确实现
6. **缓存线程安全**：Cache2D/CacheAllInCell 使用 mutable 缓存，线程不安全，
   每个区块生成任务应有独立实例。FlatCache 在 `precompute=true` 时构造期单线程
   预计算完成后 `compute()` 只读查表（`m_values` 不再变更），故 per-NoiseChunk
   实例只读安全；仅 `precompute=false` 的单值回退路径仍含 mutable 缓存
7. **NoiseChunk 插值顺序**：三线性插值必须按 Y → X → Z 顺序调用 updateForY/updateForX/updateForZ，
   否则结果错误
8. **NoiseChunk slice 交换**：advanceCellX 后必须调用 swapSlices() 切换缓冲区
9. **NoiseInterpolator 双缓冲**：slice0/slice1 分别存储当前列和下一列的角点数据，
   初始化时需要先填充 slice0
10. **BlendedNoise 涂抹效果与 SoA 向量化**：`compute` 对 Y 轴应用涂抹效果（产生条纹结构），
    且 octave 循环已走 SoA 向量化路径（复用各 `PerlinNoise` 自带的 `soa()` 连续置换表块，
    `perlinSampleSoA` 内核 + `#pragma clang loop vectorize(enable)`，标量顺序累加保 bit-exact）。
    SoA 布局、三个回退根因规避、精度双轨与 epsilon 陷阱详见 `noise/README.md` 的
    "SoA 向量化加速"章节。涂抹 epsilon 必须用 `static_cast<f64>(1.0e-7f)`（float→double），
    与 `PerlinLayer::noiseWithSmear` 逐位一致，否则 BlendedNoise 边界 floor 跨越致远超 1e-9 漂移
11. **EndIslands 种子**：使用 `LegacyRandomSource(seed).consumeCount(17292)` 初始化 SimplexNoise，
    确保与 Java 版生成相同的世界
