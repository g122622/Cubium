# Noise 噪声生成器模块

本目录包含 Minecraft 1.21 世界生成所需的噪声生成器实现，已对齐 MC 1.21.11 Java 源码。

## 目录结构

```
src/common/world/gen/noise/
├── PerlinNoise.hpp              # MC 1.18+ 多倍频 Perlin 噪声（支持任意振幅列表）
├── PerlinNoise.cpp
├── NormalNoise.hpp              # MC 1.18+ 双 Perlin 噪声（地形生成核心）
├── NormalNoise.cpp
├── SimplexNoise.hpp             # Simplex 噪声（用于末地岛屿生成）
├── SimplexNoise.cpp
├── PerlinSimplexNoise.hpp       # 多倍频 Simplex 噪声（用于旧版生物群系气候噪声）
├── PerlinSimplexNoise.cpp
├── Noises.hpp                   # 噪声参数注册表（定义所有 MC 噪声的 firstOctave 和 amplitudes）
├── Noises.cpp
├── Noise.hpp                    # 统一头文件（包含 PerlinNoise、NormalNoise、SimplexNoise）
└── README.md                    # 本文档
```

## 内部模块关系

```
PerlinNoise（MC 1.18+ 多倍频 Perlin）
    └── 内含 PerlinLayer（单个倍频层）
    └── 支持 noiseWithSmear()（Y 轴涂抹，用于 BlendedNoise）
    └── 两种构造路径：
        ├── 现代路径：PositionalRandomFactory.fromHashOf("octave_N")（NormalNoise 使用）
        └── 旧版路径：共享 JavaLegacyRandom 顺序消费随机数（BlendedNoise 使用）
            └── 对应 MC PerlinNoise(RandomSource, Pair<Integer, DoubleList>, false)
            └── createLegacyForBlendedNoise / createLegacyForLegacyNetherBiome
    └── 被 NormalNoise 和 BlendedNoise 使用

NormalNoise（MC 1.18+ 双 Perlin 噪声）
    └── 内含两个 PerlinNoise 实例（坐标偏移避免相关性）
    └── 被密度函数系统广泛使用（NoiseDensity、ShiftedNoise 等）
    └── INPUT_FACTOR ≈ 1.018，确保两路噪声不相关

SimplexNoise（2D/3D Simplex 噪声）
    └── 被 EndIslands 密度函数使用
    └── 使用 JavaLegacyRandom(seed).consumeCount(17292) 种子初始化
    └── JavaLegacyRandom 精确复刻 Java LegacyRandomSource（48位 LCG）

PerlinSimplexNoise（多倍频 Simplex 噪声）
    └── 负倍频层使用主噪声派生种子
    └── 被 OverworldBiomeBuilder 用于气候噪声
    └── 构造时使用 JavaLegacyRandom（与 MC Biome.java 中的 WorldgenRandom(LegacyRandomSource(seed)) 一致）
    └── 注意：种子派生使用 float 精度乘法（9.223372E18f）

Noises（噪声参数注册表）
    └── 定义 TEMPERATURE、VEGETATION、CONTINENTALNESS 等 27+ 种噪声参数
    └── 所有参数已对齐 MC 1.21.11 Noises.java
```

**命名空间**：所有噪声类位于 `mc::world::gen::noise` 命名空间。

## 上下游外部依赖关系

### 上游依赖（本模块使用的）

```
core/Types.hpp                      # 基础类型（f32, f64, i32, u8, u64 等）
util/math/random/Random.hpp         # 随机数生成器（Random）
util/math/random/PositionalRandomFactory.hpp  # 位置随机工厂（MC 1.18+ 噪声需要）
```

### 下游依赖（使用本模块的）

```
gen/density/DensityFunctions.hpp    # 密度函数（使用 NormalNoise 和 SimplexNoise）
gen/density/BlendedNoise.hpp        # 混合噪声密度函数（使用 PerlinNoise）
gen/density/NoiseBindingVisitor.cpp # 数据驱动噪声叶子绑定（经 Noises::get 取参数）
gen/surface/SurfaceRules.hpp        # 地表规则
gen/placement/Placements.cpp        # 放置器（噪声阈值放置）
gen/aquifer/Aquifer.cpp             # 含水层生成
```

## 关键算法对齐要点

### 1. PerlinNoise 缩放因子计算

```cpp
// 最低频率输入因子 = 2^firstOctave
// 最低频率值因子 = 2^(amplitudeCount - 1) / (2^amplitudeCount - 1)
// 注意：使用 amplitudes.size() 而非非零振幅数
m_lowestFreqInputFactor = std::pow(2.0, static_cast<f64>(m_firstOctave));
m_lowestFreqValueFactor = std::pow(2.0, static_cast<f64>(amplitudeCount - 1)) /
    (std::pow(2.0, static_cast<f64>(amplitudeCount)) - 1.0);
```

### 2. PerlinNoise::edgeValue 不使用 abs()

Java 原版的 `edgeValue` 直接使用振幅的原始值（含符号），不取绝对值。负振幅时结果与使用 abs() 不同。

### 3. PerlinNoise::maxBrokenValue 加 2.0

```cpp
// Java: maxBrokenValue(x) = edgeValue(x + 2.0)
// +2.0 补偿 noiseWithSmear 带来的额外范围
f64 maxBrokenValue(f64 maxInputValue) const { return edgeValue(maxInputValue + 2.0); }
```

### 4. noiseWithSmear 的 Y 轴吸附

```cpp
// Java: Math.min(Math.floor(fracY / yOffset + 1.0E-7F) * yOffset, fracY)
// 将 Y 小数部分吸附到 yOffset 间隔的网格线上，产生条纹结构
// 注意 epsilon 使用 float 字面量 1.0E-7F
```

### 5. Noises 注册表命名

- `SHIFT` → `"minecraft:offset"`（非 "minecraft:shift"）
- `SWAMP` → `"minecraft:surface_swamp"`（非 "minecraft:swamp"）

### 6. PerlinSimplexNoise 种子派生精度

```cpp
// Java: (long)(simplexnoise.getValue(...) * 9.223372E18F)
// Java 二元数值提升：getValue() 返回 double，9.223372E18F 是 float 字面量
// 乘法在 double 精度下进行（float 自动拓宽为 double）
// C++ 必须先拓宽 float 常量为 double，再在 double 精度下做乘法
const i64 seed = static_cast<i64>(derivedSeed * static_cast<f64>(9.223372E18f));
```

## 容易踩的坑

### 1. 倍频索引理解错误

MC 的倍频系统中，负数索引表示低频（大尺度），0 是最高频（小尺度）。

```cpp
// firstOctave = -4 表示最低频层为第 -4 倍频
// amplitudes 列表从低频到高频
PerlinNoise noise(seed, -4, {1.0, 1.0, 1.0, 1.0, 1.0});
```

### 2. 随机数生成器状态共享

```cpp
// 错误：多个噪声生成器共享同一个随机数生成器
math::Random rng(seed);
PerlinNoise noise1(rng, -4, amps);  // rng 状态改变
PerlinNoise noise2(rng, -4, amps);  // noise2 使用不同的随机序列！

// 正确：使用 PositionalRandomFactory
PerlinNoise noise1(seed, -4, amps);
PerlinNoise noise2(seed ^ 0xDEADBEEFULL, -4, amps);
```

### 3. 大坐标精度问题

当坐标超过约 33554432（2^25）时，浮点精度会出问题：

```cpp
// PerlinNoise::wrap() 处理大坐标精度
f64 safeX = PerlinNoise::wrap(largeX);
f64 value = noise.getValue(safeX, y, z);
```

### 4. 噪声值范围

- `PerlinNoise` 单倍频值范围约 [-1, 1]，多倍频叠加后范围取决于振幅
- `NormalNoise` 值范围约 [-maxValue, maxValue]，需要除以 maxValue 归一化
- `SimplexNoise` 2D 值范围约 [-70, 70]（乘以 70），3D 约 [-32, 32]（乘以 32）

### 5. NormalNoise 的 INPUT_FACTOR

`NormalNoise` 使用两个 `PerlinNoise` 实例，第二个的坐标乘以 `INPUT_FACTOR ≈ 1.018`，避免两个噪声的相关性。这是 MC 1.18+ 地形生成平滑性的关键。

### 6. NormalNoise::clone() 限制

只有通过种子构造的 NormalNoise 才能正确克隆。通过 Random& 构造的实例无法提取种子，调用 clone() 会触发断言。

### 7. BlendedNoise 涂抹效果

BlendedNoise 使用 `PerlinNoise::getValueWithSmear()` 对 Y 轴方向应用涂抹效果，产生条纹状结构。涂抹参数 `smearScaleMultiplier * yMultiplier / yFactor` 控制 Y 方向拉伸程度。

### 8. SimplexNoise 种子初始化

EndIslands 使用的 SimplexNoise 需要 `LegacyRandomSource(seed).consumeCount(17292)` 后创建，这是 MC 原版的种子推进逻辑，用于确保与 Java 版生成相同的世界。
