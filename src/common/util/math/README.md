# Math 模块

本目录包含 Cubium 项目的数学工具模块，提供向量运算、随机数生成、射线检测、视锥剔除等核心数学功能。

## 目录结构

```
src/common/util/math/
├── MathUtils.hpp                  # 数学工具函数（角度转换、插值、坐标变换等）
├── MathUtils.cpp                  # 占位文件（函数已在头文件内联）
├── Vector2.hpp                    # 2D 向量类
├── Vector3.hpp                    # 3D 向量类
├── Vector4.hpp                    # 4D 向量模板类
├── frustum/                       # 视锥剔除子系统
│   ├── Frustum.hpp                # 视锥类定义
│   ├── Frustum.cpp                # 视锥类实现
│   └── README.md                  # 视锥模块文档
├── random/                        # 随机数生成子系统
│   ├── IRandom.hpp                # 随机数生成器接口
│   ├── IRandom.cpp                # 接口默认实现
│   ├── Random.hpp                 # 统一封装（通过宏选择算法，默认 xoroshiro128++）
│   ├── LcgRandom.hpp              # 线性同余生成器
│   ├── LcgRandom.cpp              # 线性同余实现
│   ├── Mt19937Random.hpp          # Mersenne Twister 生成器
│   ├── Mt19937Random.cpp          # Mersenne Twister 实现
│   ├── Xoroshiro128ppRandom.hpp   # xoroshiro128++ 生成器（默认算法）
│   ├── Xoroshiro128ppRandom.cpp   # xoroshiro128++ 实现
│   ├── Xoshiro256ppRandom.hpp     # xoshiro256++ 生成器
│   ├── Xoshiro256ppRandom.cpp     # xoshiro256++ 实现
│   ├── UniformIntDistribution.hpp    # 均匀整数分布
│   ├── UniformIntDistribution.cpp    # 均匀整数分布实现
│   ├── UniformRealDistribution.hpp   # 均匀实数分布
│   ├── UniformRealDistribution.cpp   # 均匀实数分布实现
│   ├── RandomRanges.hpp           # 随机值范围工具类（用于掉落表）
│   └── RandomRanges.cpp           # 随机值范围实现
└── ray/                           # 射线检测子系统
    ├── Ray.hpp                    # 射线数据结构
    ├── Raycast.hpp                # 射线检测上下文和接口
    └── Raycast.cpp                # DDA 算法实现
```

## 内部模块关系

```
                    MathUtils.hpp
                         │
          ┌──────────────┼──────────────┐
          │              │              │
     Vector2.hpp    Vector3.hpp     IRandom.hpp
          │              │              │
          │              │              ├──── LcgRandom
          │              │              ├──── Mt19937Random
          │              │              ├──── Xoroshiro128ppRandom
          │              │              ├──── Xoshiro256ppRandom
          │              │              │
          │              │         Random.hpp (统一封装)
          │              │              │
          │              │         UniformIntDistribution
          │              │         UniformRealDistribution
          │              │
          └──────────────┴──── Ray.hpp
                               │
                          Raycast.hpp/cpp
                               │
                         BlockRaycastResult (core/)
```

- **MathUtils**：提供基础数学函数，被所有其他模块依赖
- **Vector2/3/4**：向量类型，独立于随机模块
- **random/**：随机数生成子系统，IRandom 定义接口，Random.hpp 统一封装多种算法（通过宏选择：xoroshiro128++、xoshiro256++、LCG、MT19937）
- **ray/**：射线检测模块，依赖 Vector3 和 MathUtils，使用 DDA 算法
- **frustum/**：视锥剔除模块，独立于其他子模块

## 上下游外部依赖关系

**被依赖（上游调用方）**：
- `common/world/` - 世界生成、区块处理使用坐标转换函数
- `common/entity/` - 实体物理、移动使用向量和射线检测
- `common/world/gen/` - 噪声生成使用随机数
- `client/renderer/` - 渲染使用向量和视锥剔除
- `server/` - 服务端逻辑使用随机数和坐标转换

**依赖（下游依赖）**：
- `common/core/Types.hpp` - 基础类型定义（i8, i32, f32, u64 等）
- `common/core/Constants.hpp` - 常量定义（PI, EPSILON, CHUNK_WIDTH 等）
- `common/core/BlockRaycastResult.hpp` - 射线检测结果
- `common/world/IWorld.hpp` - 世界接口（方块读取）
- `common/world/block/Block.hpp` - 方块定义
- `common/util/Direction.hpp` - 方向枚举
- `<cmath>`, `<algorithm>`, `<limits>`, `<functional>` - 标准库

## 容易踩的坑

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
