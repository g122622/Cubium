# Noise 噪声生成器模块

本目录包含 Minecraft 世界生成所需的噪声生成器实现，包括 MC 1.16.5 经典噪声和 MC 1.18+ 新噪声系统。

## 目录结构

```
src/common/world/gen/noise/
├── INoiseGenerator.hpp          # 噪声生成器接口（抽象基类）
├── ImprovedNoiseGenerator.hpp   # 改进的 Perlin 噪声生成器（MC 1.16.5 风格）
├── ImprovedNoiseGenerator.cpp
├── OctavesNoiseGenerator.hpp    # 多倍频噪声 + Simplex + PerlinNoiseGenerator（MC 1.16.5 风格）
├── OctavesNoiseGenerator.cpp
├── PerlinNoise.hpp              # MC 1.18+ 多倍频 Perlin 噪声（支持任意振幅列表）
├── PerlinNoise.cpp
├── NormalNoise.hpp              # MC 1.18+ 双 Perlin 噪声（地形生成核心）
├── NormalNoise.cpp
└── Noise.hpp                    # 统一头文件（仅包含 ImprovedNoiseGenerator 和 OctavesNoiseGenerator）
```

## 内部模块关系

```
INoiseGenerator（接口）
    └── ImprovedNoiseGenerator（基础 Perlin 噪声，f64 精度）
            └── OctavesNoiseGenerator（多倍频叠加，MC 1.16.5 风格）

PerlinNoise（MC 1.18+ 多倍频 Perlin）
    └── 内含 PerlinLayer（单个倍频层，类似 ImprovedNoiseGenerator）
    └── 被 NormalNoise 使用

NormalNoise（MC 1.18+ 双 Perlin 噪声）
    └── 内含两个 PerlinNoise 实例（坐标偏移避免相关性）

SimplexNoiseGenerator（独立实现）
    └── 被 PerlinNoiseGenerator（旧版地表深度噪声）使用
```

**命名空间说明**：
- `mc::ImprovedNoiseGenerator`、`mc::OctavesNoiseGenerator`、`mc::SimplexNoiseGenerator`、`mc::PerlinNoiseGenerator`：MC 1.16.5 风格（命名空间 `mc`）
- `mc::world::gen::noise::PerlinNoise`、`mc::world::gen::noise::NormalNoise`：MC 1.18+ 风格（命名空间 `mc::world::gen::noise`）

## 上下游外部依赖关系

### 上游依赖（本模块使用的）

```
core/Types.hpp                      # 基础类型（f32, f64, i32, u8, u64 等）
util/math/random/Random.hpp         # 随机数生成器（IRandom, Random）
util/math/random/PositionalRandomFactory.hpp  # 位置随机工厂（MC 1.18+ 噪声需要）
util/math/MathUtils.hpp             # 数学工具函数（lerp3 三线性插值）
```

### 下游依赖（使用本模块的）

```
gen/chunk/NoiseChunkGenerator.hpp   # 主世界区块生成器（使用多种噪声生成地形）
gen/chunk/NetherChunkGenerator.hpp  # 下界区块生成器
gen/chunk/EndChunkGenerator.hpp     # 末地区块生成器
gen/density/DensityFunctions.hpp    # 密度函数（使用 NormalNoise）
gen/surface/SurfaceBuilders.hpp     # 地表构建器（使用噪声生成地表变化）
gen/surface/SurfaceRules.hpp        # 地表规则
gen/placement/Placements.cpp        # 放置器（噪声阈值放置）
gen/aquifer/Aquifer.cpp             # 含水层生成
```

## 容易踩的坑

### 1. 倍频索引理解错误

MC 的倍频系统中，负数索引表示低频（大尺度），0 是最高频（小尺度）。

```cpp
// 错误理解：认为倍频数是 0 到 15
OctavesNoiseGenerator noise(seed, 0, 15);  // 错误！

// 正确理解：minOctave = -15 表示最低频层，maxOctave = 0 表示最高频层
OctavesNoiseGenerator noise(seed, -15, 0);  // 正确
```

### 2. 新旧噪声系统混用

MC 1.18+ 使用 `NormalNoise` 和 `PerlinNoise`（命名空间 `mc::world::gen::noise`），与 MC 1.16.5 的 `OctavesNoiseGenerator`（命名空间 `mc`）API 不同。新维度生成器应使用新噪声系统。

### 3. 随机数生成器状态共享

```cpp
// 错误：多个噪声生成器共享同一个随机数生成器
math::Random rng(seed);
ImprovedNoiseGenerator noise1(rng);  // rng 状态改变
ImprovedNoiseGenerator noise2(rng);  // noise2 使用不同的随机序列！

// 正确：每个生成器使用独立的种子
ImprovedNoiseGenerator noise1(seed);
ImprovedNoiseGenerator noise2(seed + 1);
```

### 4. 坐标偏移忽略

`ImprovedNoiseGenerator` 有内部坐标偏移（xOffset/yOffset/zOffset），构造时随机生成，用于避免不同种子产生相似模式。实际采样坐标 = 输入坐标 + 随机偏移。若需精确控制坐标，注意偏移会被自动添加。

### 5. 大坐标精度问题

当坐标超过约 33554432（2^25）时，浮点精度会出问题：

```cpp
f32 largeX = 40000000.0f;
f32 value = noise.noise(largeX, y, z);  // 可能产生伪影

// 解决方案：使用 maintainPrecision
f32 safeX = OctavesNoiseGenerator::maintainPrecision(largeX);
f32 value = noise.noise(safeX, y, z);
```

### 6. 噪声值范围

噪声值范围约为 [-1, 1]，但不是严格保证，多倍频叠加后可能略微超出。如需归一化到 [0, 1]：
```cpp
f32 normalized = (noise.noise(x, y, z) + 1.0f) * 0.5f;
```

### 7. OctavesNoiseGenerator 的 skip(262) 调用

创建多个倍频层时会跳过 262 个随机数，这是参考 MC 实现，确保不同倍频层有不同的噪声模式。复现 MC 世界时必须保持此行为。

### 8. Noise.hpp 不包含新噪声

`Noise.hpp` 统一头文件只包含 `ImprovedNoiseGenerator.hpp` 和 `OctavesNoiseGenerator.hpp`，不包含 MC 1.18+ 的 `NormalNoise` 和 `PerlinNoise`。使用新噪声需要直接包含对应头文件。

### 9. NormalNoise 的 INPUT_FACTOR

`NormalNoise` 使用两个 `PerlinNoise` 实例，第二个的坐标乘以 `INPUT_FACTOR ≈ 1.018`，避免两个噪声的相关性。这是 MC 1.18+ 地形生成平滑性的关键。
