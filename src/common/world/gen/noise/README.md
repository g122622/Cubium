# Noise 噪声生成器模块

本目录包含 Minecraft 1.21 世界生成所需的噪声生成器实现，已对齐 MC 1.21.11 Java 源码。

## 目录结构

```
src/common/world/gen/noise/
├── PerlinNoise.hpp              # MC 1.18+ 多倍频 Perlin 噪声（支持任意振幅列表）
├── PerlinNoise.cpp
├── PerlinNoiseSoA.hpp           # Perlin SoA 向量化内核（C2ME 风格 octave 并行,构造期拍平连续块）
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

## SoA 向量化加速（C2ME 风格 octave 并行）

### 动机

密度函数求值器 JIT 落地后（eval 提速 1.59×），瓶颈转移到外部噪声采样（JIT 路径 external 占 54.9%，见 `docs/iterations/密度函数求值器JIT可行性评估.md` 第 7 节）。噪声采样的大头是 NormalNoise（JAGGED 17 octave）与 BlendedNoise（main 8 / min&max 各 16 octave）。本次对 PerlinNoise 的 octave 循环引入 SIMD 加速，效仿 C2ME `c2me-opts-natives-math` 的 `ext_math.h`。

### 杠杆：单点内 octave 并行（非多点批处理）

C2ME 的 SIMD 杠杆是**单点内 octave 并行**——每个 SIMD 通道算一个 octave，各自独立 256 项置换表做独立 gather 链，hash 链 `p[p[p[h]+y]+z]` 内部串行不碰。多点批处理不是杠杆（C2ME 批处理 API 只是标量包装器）。AVX2 f64 4 通道，JAGGED（17 octave）/ BlendedNoise（16 octave）octave 数充足喂得饱。

### SoA 数据布局——连续背靠背 + 64 字节对齐

`PerlinNoiseSoA`（`PerlinNoiseSoA.hpp`）单次 64 字节对齐分配持有所有 octave 数据：

```
[perms: u8[256*N]]   所有 octave 置换表背靠背连续（gather base 跨 octave 连续）
[originX: f64[N]] [originY] [originZ]   构造期随机偏移
[amplitude] [inputFactor] [valueFactor]  标量参数 SoA 数组
```

- **置换表连续背靠背**：`perms[i*256 + ...]`，跨 octave gather base 连续 → 规避回退 bug1。
- **u8 存储**：回退前已验证 bit-exact；clang AVX2 gather u8 用 `vpmovzxb` 零扩展。
- **零拷贝**：构造期从各 `PerlinLayer` 的 `m_permutation` 拷一次到连续块（构造期一次性，非热点）；运行期 `perlinSampleSoA` 只读连续块。

### 可向量化的 octave 循环

`PerlinNoise::getValue` / `BlendedNoise::compute` 的 octave 循环标注 `#pragma clang loop vectorize(enable) interleave(enable) interleave_count(2)`，让 clang 跨 octave 内联向量化。结果先写扁平栈数组 `ds[k]`，再按顺序标量累加（保 bit-exact，SIMD 只并行采样不并行累加）。

### 三个回退根因（commit 495832dd9）已逐条规避

上次 SoA 拍平被回退，纯因**性能倒退**（精度 bit-exact 过了 1e-9）。三个实现 bug：

1. **置换表按值拷贝进 `PerlinSoALayer::std::array<u8,256>`**，运行期每个 octave 置换表分散在 `vector<PerlinSoALayer>` 各元素内，gather base 跨 octave 不连续 → 破坏缓存局部性、阻碍向量化。**规避**：`PerlinNoiseSoA::perms` 为单一连续 `u8[256*N]`。
2. **`perlinSample` 是标量单 octave 函数**，`for layer: perlinSample(layer.perm...)` 跨函数调用 + 每 layer 指针不同 → clang 无法跨 octave 向量化。**规避**：`perlinSampleSoA` 内联进 octave 循环 + `#pragma clang loop vectorize(enable)`。
3. **`PerlinSoALayer` 按值存 256B 置换表**，构造期 collect 拷贝 + 运行期分散访问。**规避**：`PerlinNoiseSoA` 持续指向同一次 64 字节对齐分配，零拷贝。

### SoA 默认强制开启

SoA 无条件强制开启，**无任何开关/宏/条件回退**。标量 `PerlinLayer::noise`/`noiseWithSmear` 保留仅作 ULP 测试的 reference ground truth（`DensityAstUlpTest.cpp` 内调对比），生产 `PerlinNoise::getValue`/`BlendedNoise::compute` 无条件走 SoA。若性能倒退，靠 `git revert` 整个提交回退，不靠运行期/编译期开关。

### 精度双轨

- **保留** `DensityAstBaselineTest`/`DensityAstCompileTest` 的 1e-9 门禁（复用 bit-exact 内核 + 标量顺序累加，SoA 路径与原标量路径理论 bit-exact）。
- **新增** `DensityAstUlpTest.cpp`：SoA 路径 vs 标量 reference 的 ULP 漂移监控（纯观测，16 ulp 阈值，不卡门禁）。若 clang 跨 octave 向量化后 FMA 融合致 ULP 差异突破 1e-9，ULP 报告定位漂移点与量级，据实测决定放宽阈值还是对该文件加 `-ffp-contract=off`。

### 精度关键：涂抹 epsilon 陷阱

`perlinSampleSoA` 的 Y 涂抹 epsilon 必须用 `static_cast<f64>(1.0e-7f)`（float 字面量转 double），与 `PerlinLayer::noiseWithSmear` 逐位一致。`1.0e-7`（double）与 `1.0e-7f→double` 值不同，边界附近 `floor(base/yScale + epsilon)` 会跨越整数，致 `smearOffset` 差一个 `yScale` 量级，远超 1e-9。NormalNoise 路径 yScale=0 不触发，BlendedNoise 路径（yScale≠0）必须严格一致。

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
