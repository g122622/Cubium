# Math 模块

本目录包含 Minecraft Reborn 项目的数学工具模块，提供向量运算、随机数生成、射线检测、视锥剔除等核心数学功能。

## 目录结构

```
src/common/util/math/
├── MathUtils.hpp          # 数学工具函数（内联实现）
├── MathUtils.cpp          # 占位文件（函数已在头文件内联）
├── Vector2.hpp            # 2D 向量类
├── Vector3.hpp            # 3D 向量类
├── Vector4.hpp            # 4D 向量模板类
├── frustum/               # 视锥剔除子系统
│   ├── Frustum.hpp        # 视锥类定义
│   ├── Frustum.cpp        # 视锥类实现
│   └── README.md          # 视锥模块文档
├── random/                # 随机数生成子系统
│   ├── IRandom.hpp        # 随机数生成器接口
│   ├── IRandom.cpp        # 接口默认实现
│   ├── Random.hpp         # 统一封装（通过宏选择算法）
│   ├── LcgRandom.hpp      # 线性同余生成器
│   ├── LcgRandom.cpp      # 线性同余实现
│   ├── Mt19937Random.hpp  # Mersenne Twister 生成器
│   ├── Mt19937Random.cpp  # Mersenne Twister 实现
│   ├── Xoroshiro128ppRandom.hpp   # xoroshiro128++ 生成器
│   ├── Xoroshiro128ppRandom.cpp   # xoroshiro128++ 实现
│   ├── Xoshiro256ppRandom.hpp     # xoshiro256++ 生成器
│   ├── Xoshiro256ppRandom.cpp     # xoshiro256++ 实现
│   ├── UniformIntDistribution.hpp    # 均匀整数分布
│   ├── UniformIntDistribution.cpp    # 均匀整数分布实现
│   ├── UniformRealDistribution.hpp   # 均匀实数分布
│   └── UniformRealDistribution.cpp   # 均匀实数分布实现
└── ray/                   # 射线检测子系统
    ├── Ray.hpp            # 射线数据结构
    ├── Raycast.hpp        # 射线检测上下文和接口
    └── Raycast.cpp        # DDA 算法实现
```

## 文件详细说明

### 根目录文件

#### MathUtils.hpp

**职责**：提供通用数学工具函数集合。

**主要内容**：
- **基本数学函数**
  - `toRadians()` / `toDegrees()` - 角度转换
  - `clamp()` - 数值范围限制
  - `lerp()` - 线性插值
  - `smoothstep()` - 平滑插值
  - `square()` / `cube()` - 平方/立方
  - `isZero()` / `approxEqual()` - 浮点数近似比较
  - `fastInverseSqrt()` - 快速逆平方根近似
  - `ceilTo()` / `floorTo()` / `roundTo()` - 取整函数

- **区块相关计算**
  - `toChunkCoord()` - 世界坐标转区块坐标
  - `toWorldCoord()` - 区块坐标转世界坐标
  - `toLocalCoord()` - 世界坐标转区块内坐标
  - `chunkPosToId()` / `idToChunkPos()` - 区块坐标与64位ID互转

- **角度处理函数**
  - `wrapDegrees()` - 角度规范化到 [-180, 180)
  - `wrapDegreesPositive()` - 角度规范化到 [0, 360)
  - `clampAngle()` - 限制角度变化量

- **距离计算**
  - `distanceHorizontalSq()` - 水平距离平方
  - `distanceSq()` - 3D距离平方

#### Vector2.hpp

**职责**：2D向量类，用于平面位置、方向、UV坐标等。

**主要内容**：
- 构造函数：默认构造、分量构造、标量构造
- 静态常量：`ZERO`, `ONE`, `UP`, `DOWN`, `LEFT`, `RIGHT`
- 算术运算：加减乘除、取负、复合赋值
- 向量运算：
  - `length()` / `lengthSquared()` - 长度
  - `normalized()` / `normalize()` - 归一化
  - `dot()` - 点积
  - `cross()` - 2D叉积（返回标量）
  - `distance()` / `distanceSquared()` - 距离
  - `lerp()` - 线性插值
  - `rotated()` - 旋转向量
  - `perpendicular()` - 垂直向量
  - `angle()` - 获取向量角度
  - `fromAngle()` - 从角度创建单位向量
- 哈希函数支持（用于 `std::unordered_set`/`std::unordered_map`）

#### Vector3.hpp

**职责**：3D向量类，用于位置、方向、速度等3D量。

**主要内容**：
- 构造函数：默认构造、分量构造、标量构造
- 静态常量：`ZERO`, `ONE`, `UP`, `DOWN`, `LEFT`, `RIGHT`, `FORWARD`, `BACK`
- 算术运算：加减乘除、取负、复合赋值
- 向量运算：
  - `length()` / `lengthSquared()` / `lengthHorizontal()` - 长度
  - `normalized()` / `normalize()` - 归一化
  - `dot()` - 点积
  - `cross()` - 3D叉积（返回向量）
  - `distance()` / `distanceSquared()` / `distanceHorizontal()` - 距离
  - `lerp()` - 线性插值
  - `pitch()` / `yaw()` - 获取欧拉角
  - `fromAngles()` - 从角度创建方向向量
- 坐标转换：
  - `blockX()` / `blockY()` / `blockZ()` - 取方块坐标
  - `floored()` / `ceiled()` - 取整向量
- 类型别名：`Position`, `Velocity`
- 哈希函数支持

### random/ 子目录

#### IRandom.hpp / IRandom.cpp

**职责**：定义随机数生成器接口，所有算法实现此接口。

**主要方法**：
- `setSeed(u64)` - 设置种子
- `nextU64()` - 核心方法，返回 [0, UINT64_MAX] 范围随机数
- `nextU32()` - 返回 [0, UINT32_MAX] 范围随机数
- `nextInt()` - 返回随机 i32
- `nextInt(i32 bound)` - 返回 [0, bound) 范围随机整数（无偏差算法）
- `nextInt(i32 min, i32 max)` - 返回 [min, max] 范围随机整数
- `nextBoolean()` - 返回随机布尔值
- `nextFloat()` / `nextFloat(f32 min, f32 max)` - 返回随机浮点数
- `nextDouble()` / `nextDouble(f64 min, f64 max)` - 返回随机双精度浮点数
- `nextGaussian(f32 mean, f32 stddev)` - 高斯分布随机数（Marsaglia polar method）
- `nextLong()` / `nextLong(i64 bound)` - 返回随机长整数
- `setSeedWithHash(i64)` - MC风格种子哈希
- `skip(u64 count)` - 跳过指定数量随机数

**设计特点**：
- 参考 Minecraft 1.16.5 的 `Random` 类设计
- `nextInt(bound)` 使用无偏差算法避免模偏差
- 高斯分布使用缓存优化（每次生成两个值，缓存第二个）

#### Random.hpp

**职责**：通过编译宏选择底层随机数算法的统一封装。

**使用方法**：
```cpp
#include "math/random/Random.hpp"

mc::math::Random rng(seed);
i32 value = rng.nextInt(100);  // [0, 100)
f32 f = rng.nextFloat();        // [0.0, 1.0)
```

**算法选择**（通过定义宏）：
| 宏 | 算法 | 周期 | 状态大小 | 特点 |
|---|------|------|---------|------|
| `MC_RANDOM_XOROSHIRO128PP` | xoroshiro128++ | 2^128-1 | 16字节 | **默认**，高性能，小状态 |
| `MC_RANDOM_XOSHIRO256PP` | xoshiro256++ | 2^256-1 | 32字节 | 高质量 |
| `MC_RANDOM_LCG` | 线性同余 | ~2^64 | 8字节 | 最小状态 |
| 默认 | Mersenne Twister | 2^19937-1 | 2.5KB | 最高兼容性 |

#### LcgRandom.hpp / LcgRandom.cpp

**职责**：线性同余随机数生成器实现。

**特点**：
- 周期：约 2^64
- 状态极小：仅 8 字节
- 速度极快：仅一次乘法和一次加法
- 使用 MMIX 参数 (Knuth): `X_{n+1} = (a * X_n + c) mod 2^64`

**适用场景**：对随机质量要求不高、需要极小状态的场合。

#### Mt19937Random.hpp / Mt19937Random.cpp

**职责**：Mersenne Twister 随机数生成器实现。

**特点**：
- 周期长：2^19937 - 1
- 质量高：通过所有统计测试
- 兼容性好：基于 `std::mt19937_64`
- 状态大：2.5KB

**适用场景**：需要最高随机质量的场合，或与 C++ 标准库兼容。

#### Xoroshiro128ppRandom.hpp / Xoroshiro128ppRandom.cpp

**职责**：xoroshiro128++ 随机数生成器实现（**默认算法**）。

**特点**：
- 周期：2^128 - 1
- 状态小：仅 16 字节
- 速度快：比 Mersenne Twister 快约 2-3 倍
- 质量高：通过 BigCrush 测试
- 支持快速跳转（`skip()` 方法，O(log count)）

**实现细节**：
- 使用 SplitMix64 扩展种子
- 支持预计算跳转多项式快速前进

#### Xoshiro256ppRandom.hpp / Xoshiro256ppRandom.cpp

**职责**：xoshiro256++ 随机数生成器实现。

**特点**：
- 周期：2^256 - 1
- 状态：32 字节
- 速度快：比 Mersenne Twister 快约 2 倍
- 质量极高：通过所有统计测试
- 支持快速跳转

#### UniformIntDistribution.hpp / UniformIntDistribution.cpp

**职责**：均匀整数分布生成器，避免重复创建 `std::uniform_int_distribution`。

**使用方法**：
```cpp
UniformIntDistribution dist(1, 100);

// 使用已有随机数生成器
Random rng(seed);
i32 value = dist(rng);

// 或便捷方法（不推荐高频调用）
i32 value = dist.generate();
```

#### UniformRealDistribution.hpp / UniformRealDistribution.cpp

**职责**：均匀实数分布生成器，避免重复创建 `std::uniform_real_distribution`。

**使用方法**：
```cpp
UniformRealDistribution dist(0.0f, 1.0f);
Random rng(seed);
f32 value = dist(rng);
```

### ray/ 子目录

#### Ray.hpp

**职责**：射线数据结构定义。

**主要内容**：
- `origin` - 射线起点
- `direction` - 射线方向（应归一化）
- `at(f32 t)` - 获取射线上距离 t 处的点
- `fromAngles()` - 从 pitch 和 yaw 角度创建射线

**设计特点**：
- 参考 MC 的 `RayTraceContext`
- 支持从 MC 风格角度（pitch 正为下，yaw 正为左）创建

#### Raycast.hpp / Raycast.cpp

**职责**：方块射线检测实现。

**主要类型**：
- `RaycastContext` - 射线检测上下文
  - `ray` - 射线
  - `maxDistance` - 最大检测距离（默认 5 格）
  - `endPosition()` - 获取射线终点

**主要函数**：
```cpp
BlockRaycastResult raycastBlocks(const RaycastContext& context, const IBlockReader& world);
```

**算法说明**（DDA算法，参考 MC 的 `IBlockReader.doRayTrace`）：
1. 计算射线在各轴上穿过一个方块所需的时间比率
2. 每次选择最近的方块边界进入
3. 检查每个方块是否可碰撞
4. 超出距离限制或 Y 范围时停止
5. 区块未加载（nullptr）视为空气，继续检测

**实现细节**：
- 使用 MC 风格的端点偏移避免边界精度问题
- 正确处理起点在方块内部的情况
- 正确识别击中面（Direction）

## 文件关系图

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

## 模块概述

### 整体职责

math 模块提供游戏核心数学功能：

1. **基础数学工具**：角度转换、插值、坐标变换、浮点数比较
2. **向量运算**：2D/3D 向量的算术运算、几何计算
3. **随机数生成**：多种算法可选的随机数系统，MC 风格 API
4. **射线检测**：基于 DDA 算法的方块碰撞检测

### 输入和输出

**输入**：
- 原始数值（角度、坐标、标量）
- 向量数据
- 随机种子
- 射线参数（起点、方向、最大距离）
- 方块世界读取器（`IBlockReader`）

**输出**：
- 转换后的数值
- 向量计算结果
- 随机数序列
- 射线检测结果（`BlockRaycastResult`）

### 依赖项

**内部依赖**：
- `common/core/Types.hpp` - 基础类型定义（`i8`, `i32`, `f32`, `u64` 等）
- `common/core/Constants.hpp` - 常量定义（`PI`, `EPSILON`, `CHUNK_WIDTH` 等）
- `common/core/BlockRaycastResult.hpp` - 射线检测结果
- `common/world/IWorld.hpp` - 世界接口（方块读取）
- `common/world/block/Block.hpp` - 方块定义
- `common/util/Direction.hpp` - 方向枚举

**外部依赖**：
- `<cmath>` - 标准数学库
- `<algorithm>` - `std::clamp`, `std::min`, `std::max`
- `<limits>` - 数值极限
- `<functional>` - 哈希函数
- `<random>` - `std::mt19937_64`（仅 Mt19937Random）
- `<memory>` - 智能指针

### 使用方法

#### 基本数学函数

```cpp
#include "common/util/math/MathUtils.hpp"

using namespace mc::math;

// 角度转换
f32 radians = toRadians(90.0f);
f32 degrees = toDegrees(HALF_PI);

// 插值
f32 value = lerp(0.0f, 100.0f, 0.5f);  // 50.0f
f32 smooth = smoothstep(0.0f, 1.0f, 0.5f);

// 坐标转换
ChunkCoord chunkX = toChunkCoord(worldX);
BlockCoord localX = toLocalCoord(worldX);
u64 chunkId = chunkPosToId(chunkX, chunkZ);

// 角度处理
f32 wrapped = wrapDegrees(540.0f);  // 180.0f
```

#### 向量运算

```cpp
#include "common/util/math/Vector3.hpp"

// 创建向量
Vector3 pos(10.0f, 64.0f, 20.0f);
Vector3 dir = Vector3::fromAngles(pitch, yaw);

// 基本运算
Vector3 sum = pos + dir;
Vector3 scaled = dir * 2.0f;
f32 len = dir.length();
Vector3 normalized = dir.normalized();

// 点积和叉积
f32 dot = a.dot(b);
Vector3 cross = a.cross(b);

// 距离计算
f32 dist = pos.distance(target);
f32 distSq = pos.distanceSquared(target);

// 坐标转换
BlockPos blockPos(pos.blockX(), pos.blockY(), pos.blockZ());
```

#### 随机数生成

```cpp
#include "common/util/math/random/Random.hpp"

mc::math::Random rng(seed);

// 基本随机数
i32 value = rng.nextInt(100);      // [0, 100)
i32 range = rng.nextInt(10, 20);   // [10, 20]
f32 f = rng.nextFloat();           // [0.0, 1.0)
f32 ranged = rng.nextFloat(5.0f, 10.0f);  // [5.0, 10.0)
bool b = rng.nextBoolean();

// 高斯分布
f32 normal = rng.nextGaussian(0.0f, 1.0f);

// 分布生成器
mc::math::UniformIntDistribution intDist(1, 100);
mc::math::UniformRealDistribution floatDist(0.0f, 1.0f);
i32 iv = intDist(rng);
f32 fv = floatDist(rng);
```

#### 射线检测

```cpp
#include "common/util/math/ray/Raycast.hpp"

// 从玩家视线创建射线
Ray ray = Ray::fromAngles(eyePos, pitch, yaw);
RaycastContext context(ray, 5.0f);  // 5格距离

// 执行检测
BlockRaycastResult result = raycastBlocks(context, world);

if (result.isHit()) {
    BlockPos hitBlock = result.blockPos();
    Direction hitFace = result.face();
    f32 distance = result.distance();
    Vector3 hitPos = result.position();
}
```

### 容易踩的坑

#### 1. 随机数种子状态

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
    i32 getRandomValue() {
        return m_rng.nextInt(100);
    }
};
```

#### 2. 随机数边界

```cpp
Random rng(seed);

// 注意：nextInt(bound) 返回 [0, bound)，不包含 bound
i32 value = rng.nextInt(100);  // 0-99

// nextInt(min, max) 返回 [min, max]，两端都包含
i32 range = rng.nextInt(1, 100);  // 1-100

// nextFloat() 返回 [0.0, 1.0)，不包含 1.0
f32 f = rng.nextFloat();  // 可能返回 0.9999... 但不会返回 1.0
```

#### 3. 向量比较使用近似相等

```cpp
Vector3 a(1.0f, 0.0f, 0.0f);
Vector3 b(1.0000001f, 0.0f, 0.0f);

// 正确：使用 == 运算符（内部使用 approxEqual）
if (a == b) { /* ... */ }

// 错误：直接比较浮点分量
if (a.x == b.x) { /* 精度问题 */ }
```

#### 4. 向量归一化零向量处理

```cpp
Vector3 zero;
Vector3 normalized = zero.normalized();  // 返回 ZERO，不会崩溃

// 但最好先检查
if (dir.lengthSquared() > EPSILON) {
    dir.normalize();
}
```

#### 5. 射线方向必须归一化

```cpp
// 错误：方向未归一化，距离计算会出错
Ray ray(origin, target - origin);

// 正确：归一化方向
Ray ray(origin, (target - origin).normalized());
```

#### 6. 区块坐标负数处理

```cpp
// 世界坐标转区块坐标时，负数需要特殊处理
// toChunkCoord 已正确处理：
ChunkCoord chunk = toChunkCoord(-1);  // 返回 -1，不是 0
ChunkCoord chunk = toChunkCoord(-16); // 返回 -1
ChunkCoord chunk = toChunkCoord(-17); // 返回 -2
```

#### 7. 高斯分布的缓存行为

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

### 涉及的测试用例

测试文件位置：
- `tests/common/test_math.cpp` - MathUtils 和 Vector3 测试
- `tests/common/math/RaycastTest.cpp` - 射线检测测试

#### MathUtils 测试

| 测试名称 | 测试内容 |
|---------|---------|
| `ToRadiansToDegrees` | 角度与弧度转换 |
| `Clamp` | 数值范围限制 |
| `Lerp` | 线性插值 |
| `IsZero` | 零值判断 |
| `ApproxEqual` | 浮点数近似相等 |
| `ChunkCoordConversion` | 区块坐标转换 |
| `LocalCoordConversion` | 区块内坐标转换 |

#### Vector3 测试

| 测试名称 | 测试内容 |
|---------|---------|
| `Construction` | 向量构造 |
| `Arithmetic` | 算术运算（加减乘） |
| `DotProduct` | 点积计算 |
| `Length` | 长度计算 |
| `Normalize` | 归一化 |

#### 射线检测测试

| 测试名称 | 测试内容 |
|---------|---------|
| `NoHit` | 射线不命中任何方块 |
| `HitBlockDirectly` | 直接命中前方方块 |
| `HitBlockThroughAir` | 穿过空气命中方块 |
| `HitFirstBlockNotSecond` | 命中第一个方块而非后面的 |
| `HitFromDifferentDirections` | 从不同方向（东、西、上、下）命中 |
| `HitDiagonal` | 对角线方向命中 |
| `StartInsideBlock` | 起点在方块内部 |
| `ExceedMaxDistance` | 超出最大距离 |
| `DiagonalHitFirstBlock` | 斜向射线命中第一个方块 |
| `NegativeDirection` | 负方向射线 |
| `EdgeCase_ExactBoundary` | 边界情况：穿过方块边缘 |
| `CloseRange` | 近距离命中 |

## 性能特性

### 随机数算法性能比较

| 算法 | 生成速度 | 状态大小 | 质量 | 适用场景 |
|-----|---------|---------|------|---------|
| LCG | 最快 | 8B | 一般 | 简单场景、极小状态需求 |
| xoroshiro128++ | 快 | 16B | 高 | **默认选择**、游戏逻辑 |
| xoshiro256++ | 快 | 32B | 极高 | 需要高质量随机数 |
| MT19937 | 中等 | 2.5KB | 最高 | 兼容性、科学计算 |

### 射线检测性能

- 时间复杂度：O(n)，n 为射线穿过的方块数
- 典型情况：5 格距离约检测 5-20 个方块
- 优化：DDA 算法避免了逐方块遍历，直接跳跃到边界

## 扩展建议

1. **添加新的随机数算法**：继承 `IRandom` 接口，实现 `setSeed()` 和 `nextU64()` 方法。

2. **添加新的向量类型**：参考 `Vector2.hpp` 和 `Vector3.hpp` 的模式，添加 `Vector4` 或 `Matrix` 类型。

3. **扩展射线检测**：当前仅支持方块检测，可扩展支持实体检测、流体检测。

4. **添加噪声函数**：当前噪声函数位于 `common/world/gen/noise/`，可考虑统一到 math 模块。
