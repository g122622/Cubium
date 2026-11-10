# math 模块

## 目录结构树

```text
math/
├── frustum/  # 该领域的子模块
├── noise/  # 共享的确定性数学噪声
├── random/  # 该领域的子模块
├── ray/  # 该领域的子模块
├── MathConstants.hpp  # 模块接口与实现
├── MathUtils.cpp  # 模块接口与实现
├── MathUtils.hpp  # 度转弧度
├── Vector2.hpp  # 2D向量模板类
├── Vector3.hpp  # 3D向量模板类
└── Vector4.hpp  # 4D向量模板类
```

## 内部模块关系

各文件承接目录树中标明的单一职责，通过同领域接口组合。独占的辅助实现位于使用者目录下。

## 上下游外部依赖关系

公共层提供领域类型与接口；服务端通过应用装配连接玩家、网络和世界实现。源文件的构建归属与领域目录一致。

## 容易踩的坑

- 目录按领域职责组织；跨领域协作通过明确入口和基础类型。
- 新增或迁移源文件时同步更新所有消费目标的 CMake 清单。

### 0. 世界生成的装饰种子必须走 `WorldgenRandom`，不能用裸 `Xoroshiro128ppRandom`

原版 `ChunkGenerator.applyBiomeDecoration` 用 `new WorldgenRandom(new XoroshiroRandomSource(uniqueSeed))`
派生 decorSeed 与逐特征种子。`WorldgenRandom` 把所有位宽抽取都折算成"反复调用内层 `nextLong()`
取高位"，于是它自己的 `nextLong()` 会消耗**两个**内层 `nextLong()` 并各取高 32 位；直接在内层上调
`nextLong()` 则是消耗一个、取全 64 位。两者得到的 decorSeed 完全不同（实测某区块为
`0xF53DCD247D852AF7` 对 `0x91D8CBEBCC2C2BF7`），会让**所有** placed_feature 的
`setFeatureSeed` 种子错位，表现为石头变体/矿石/树木的大规模落位偏差，且无任何报错。

同理，`LegacyRandomSource`-flavor 的路径（结构放置的 frequency reducer、要塞环、结构生成）也必须
`WorldgenRandom(std::make_unique<JavaLegacyRandom>(0))`——其 `nextFloat()/nextDouble()` 与
Xoroshiro 的同名方法结果不同。

`IRandom` 为此提供了 `nextBits(bits)`：Legacy 覆写为自身的 `next(bits)`（只推进一次 LCG），
其余实现默认取 `nextU64()` 高位（推进两次）。**不要**在需要复刻按位宽抽取的地方自己写
`nextU64() >> (64 - bits)`——Legacy 内层会因此整体错位。

### 0.1 世界生成签名收 `math::IRandom&`，不要收 `math::Random&`

`math::Random` 是 `Xoroshiro128ppRandom` 的别名（具体类型）。世界生成链路上凡是"由调用方提供
随机源"的位置（`ConfiguredFeature::place`、`PlacementModifier::getPositions`、`StructurePiece::generate`、
`TreeDecorator`、`WorldCarver::carve`、`StructurePlacement::isStructureChunk` 等）一律收
`math::IRandom&`，才能接受 `WorldgenRandom` / `JavaLegacyRandom`。
只有"自己新建一个具体 RNG"（`math::Random rng(seed);`）时才用 `math::Random`。

### 0.2 `PositionalRandomFactory` 有两种 flavor，派生算法完全不同

- **Xoroshiro**（`XoroshiroRandomSource.forkPositional()`，128 位种子）：`fromHashOf` 用 **MD5**，
  `fromSeed` 与工厂种子异或。
- **Legacy**（`LegacyRandomSource.forkPositional()`，64 位种子）：`fromHashOf` 用
  **Java `String.hashCode()`**，`fromSeed` **不与工厂种子异或**。

构造方式即区分：`PositionalRandomFactory(seedLo, seedHi)` 得 Xoroshiro，
`PositionalRandomFactory(seed)` 得 Legacy。晶洞（GeodeFeature）与
`DualNoiseProvider`/`NoiseBasedStateProvider` 走 Legacy flavor，误用 Xoroshiro 会让噪声图案整体偏离。

### 1. 随机数种子状态

每次调用都创建新生成器会导致随机序列不正确，应复用同一个生成器：

```cpp
// 错误：每次调用都创建新生成器
i32 getRandomValue() {
    Random rng(time(nullptr));  // 每次都创建新实例
    return rng.nextInt(100);
}

// 正确：复用同一个生成器
class Game {
    math::Random m_rng;
public:
    Game(u64 seed) : m_rng(seed) {}
    i32 getRandomValue() { return m_rng.nextInt(100); }
};
```

### 2. 随机数边界

```cpp
Random rng(seed);
// nextInt(bound) 返回 [0, bound)，不包含 bound
i32 value = rng.nextInt(100);  // 0-99

// nextInt(min, max) 返回 [min, max]，两端都包含
i32 range = rng.nextInt(1, 100);  // 1-100

// nextFloat() 返回 [0.0, 1.0)，不包含 1.0
f32 f = rng.nextFloat();  // 可能返回 0.9999... 但不会返回 1.0
```

### 3. 向量比较使用近似相等

```cpp
Vector3 a(1.0f, 0.0f, 0.0f);
Vector3 b(1.0000001f, 0.0f, 0.0f);

// 正确：使用 == 运算符（内部使用 approxEqual）
if (a == b) { /* ... */ }

// 错误：直接比较浮点分量
if (a.x == b.x) { /* 精度问题 */ }
```

### 4. 向量归一化零向量处理

```cpp
Vector3 zero;
Vector3 normalized = zero.normalized();  // 返回 ZERO，不会崩溃

// 但最好先检查
if (dir.lengthSquared() > EPSILON) {
    dir.normalize();
}
```

### 5. 射线方向必须归一化

```cpp
// 错误：方向未归一化，距离计算会出错
Ray ray(origin, target - origin);

// 正确：归一化方向
Ray ray(origin, (target - origin).normalized());
```

### 6. 区块坐标负数处理

```cpp
// 世界坐标转区块坐标时，负数需要特殊处理
// toChunkCoord 已正确处理：
ChunkCoord chunk = toChunkCoord(-1);  // 返回 -1，不是 0
ChunkCoord chunk = toChunkCoord(-16); // 返回 -1
ChunkCoord chunk = toChunkCoord(-17); // 返回 -2
```

### 7. 高斯分布的缓存行为

```cpp
Random rng(seed);
// nextGaussian 使用 Marsaglia polar method
// 每次调用生成两个值，缓存第二个
// 这意味着连续两次调用不是独立的
f32 g1 = rng.nextGaussian();  // 生成两个，返回第一个
f32 g2 = rng.nextGaussian();  // 返回缓存的第二个

// 如果需要独立的值，这没问题
// 但如果设置种子后立即需要确定性的序列，注意这个行为
```

### 8. 帧率无关纠正因子

`exponentialDecayFactor(ratePerSecond, deltaTime)` 用于平滑插值，使纠正速度与帧率无关。公式：`correctionFactor = 1 - (1 - ratePerSecond)^deltaTime`

典型场景：客户端时间同步、网络位置插值、相机跟随。

```cpp
constexpr f32 CORRECTION_PER_SECOND = 0.5f;
const f32 factor = exponentialDecayFactor(CORRECTION_PER_SECOND, deltaTime);
currentValue += (targetValue - currentValue) * factor;
// 无论帧率如何，1秒内总纠正量都约为 50%
```
