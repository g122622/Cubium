# Util Module

通用工具库，提供跨项目的基础设施组件。包含断言、缓存、数学、NBT序列化、属性系统、平台信息等核心工具。

## 目录结构

```
util/
├── assert/                    # 断言库
│   ├── Assert.hpp             # 核心断言类和管理器
│   ├── Assert.cpp             # 实现
│   ├── AssertMacros.hpp       # 断言宏定义
│   ├── AssertAll.hpp          # 统一包含头文件
│   └── README.md              # 详细文档
├── cache/                     # 缓存实现
│   ├── Long2IntLRUCache.hpp   # LRU缓存（链表+哈希表）
│   ├── Long2IntLRUCache.cpp
│   ├── OpenAddressingLRUCache.hpp  # 开放寻址LRU缓存
│   └── OpenAddressingLRUCache.cpp
├── math/                      # 数学工具
│   ├── MathUtils.hpp          # 数学函数
│   ├── MathUtils.cpp
│   ├── Vector2.hpp            # 2D向量
│   ├── Vector3.hpp            # 3D向量
│   ├── random/                # 随机数生成器
│   │   ├── IRandom.hpp        # 随机数接口
│   │   ├── IRandom.cpp
│   │   ├── Random.hpp         # 统一封装
│   │   ├── LcgRandom.hpp/cpp  # 线性同余
│   │   ├── Mt19937Random.hpp/cpp  # Mersenne Twister
│   │   ├── Xoroshiro128ppRandom.hpp/cpp  # xoroshiro128++
│   │   ├── Xoshiro256ppRandom.hpp/cpp     # xoshiro256++
│   │   ├── UniformIntDistribution.hpp/cpp
│   │   └── UniformRealDistribution.hpp/cpp
│   └── ray/                   # 射线检测
│       ├── Ray.hpp            # 射线数据结构
│       ├── Raycast.hpp        # 射线检测接口
│       └── Raycast.cpp
├── nbt/                       # NBT序列化
│   ├── Nbt.hpp                # NBT库主头文件
│   ├── Nbt.cpp                # 实现
│   ├── NbtInternal.hpp        # 内部实现
│   ├── README.md              # NBT文档
│   └── LICENSE                # 许可证
├── property/                  # 属性系统
│   ├── IProperty.hpp          # 属性接口
│   ├── Property.hpp           # 属性模板基类
│   ├── BooleanProperty.hpp    # 布尔属性
│   ├── IntegerProperty.hpp    # 整数属性
│   ├── EnumProperty.hpp       # 枚举属性
│   ├── DirectionProperty.hpp  # 方向属性
│   ├── FluidProperties.hpp    # 流体属性
│   ├── Properties.hpp         # 方块状态属性
│   ├── StateContainer.hpp     # 状态容器
│   └── StateHolder.hpp        # 状态持有者
├── AxisAlignedBB.hpp          # 轴对齐包围盒
├── AxisAlignedBB.cpp
├── CompressionUtils.hpp       # gzip 压缩/解压工具
├── CompressionUtils.cpp
├── Direction.hpp              # 方向枚举
├── NibbleArray.hpp            # 4位数组
├── NibbleArray.cpp
├── PlatformInfo.hpp           # 平台信息
├── PlatformInfo.cpp
├── StringUtils.hpp            # 字符串工具函数
└── TimeUtils.hpp              # 时间工具
```

## 模块详解

### 1. 断言库 (assert/)

功能强大的运行时断言库，支持多种断言级别、堆栈跟踪和自定义处理器。

**断言级别：**
| 级别 | 宏 | Debug | Release | 用途 |
|------|-----|-------|---------|------|
| Debug | `MC_ASSERT` | 启用 | 禁用 | 开发调试 |
| Release | `MC_ASSERT_RELEASE` | 启用 | 启用 | 关键检查 |
| Fatal | `MC_ASSERT_FATAL` | 启用 | 启用 | 不可恢复错误 |

**使用示例：**
```cpp
#include "common/util/assert/AssertAll.hpp"

// 基本断言
MC_ASSERT(ptr != nullptr);
MC_ASSERT_MSG(size > 0, "Size must be positive");

// 比较断言（带值输出）
MC_ASSERT_EQ(expected, actual);
MC_ASSERT_LT(value, max);

// 范围断言
MC_ASSERT_INDEX(idx, array.size());

// 致命断言
MC_ASSERT_FATAL(initialized);
```

**依赖项：** 无外部依赖，跨平台支持（Windows/Linux/macOS）

---

### 2. 缓存 (cache/)

两种LRU缓存实现，用于坐标到值的映射。

#### Long2IntLRUCache

传统LRU实现，使用 `std::unordered_map` + `std::list`。

```cpp
#include "util/cache/Long2IntLRUCache.hpp"

Long2IntLRUCache cache(1024);

// 存储值
i64 key = Long2IntLRUCache::packCoords(chunkX, chunkZ);
cache.put(key, value);

// 获取值
i32 value;
if (cache.get(key, value)) {
    // 命中
}

// 批量操作
std::lock_guard<std::mutex> lock(cache.getMutex());
cache.putLocked(key1, value1);
cache.getLocked(key2, value2);
```

#### OpenAddressingLRUCache

开放寻址哈希表实现，更高效、更好的缓存局部性。

```cpp
#include "util/cache/OpenAddressingLRUCache.hpp"

OpenAddressingLRUCache cache(1024);

// 使用方式与Long2IntLRUCache相同
cache.put(cache.packCoords(x, z), value);

// 额外的统计功能
cache.hitCount();
cache.missCount();
cache.resetStats();
```

**性能对比：** `OpenAddressingLRUCache` 比 `Long2IntLRUCache` 快约2-3倍。

---

### 3. 数学工具 (math/)

#### MathUtils.hpp

常用数学函数：

```cpp
#include "util/math/MathUtils.hpp"

using namespace mc::math;

// 度/弧度转换
f32 radians = toRadians(90.0f);
f32 degrees = toDegrees(PI);

// 插值
f32 result = lerp(a, b, t);
f32 smooth = smoothstep(0.0f, 1.0f, x);

// 区块坐标转换
ChunkCoord cx = toChunkCoord(worldX);
BlockCoord localX = toLocalCoord(worldX);
u64 chunkId = chunkPosToId(cx, cz);

// 角度处理
f32 wrapped = wrapDegrees(720.0f);  // -> 0.0f
f32 clamped = clampAngle(source, target, maxChange);
```

#### Vector2 / Vector3

2D和3D向量类：

```cpp
#include "util/math/Vector3.hpp"

Vector3 pos(10.0f, 64.0f, 20.0f);
Vector3 dir = Vector3::fromAngles(pitch, yaw);

f32 len = pos.length();
f32 dist = pos.distanceTo(other);
Vector3 norm = pos.normalized();
f32 dot = a.dot(b);
Vector3 cross = a.cross(b);

// 坐标转换
BlockCoord bx = pos.blockX();
ChunkCoord cx = toChunkCoord(pos.x);
```

#### 随机数生成器 (math/random/)

多种随机数算法实现，通过宏选择：

```cpp
#include "util/math/random/Random.hpp"

mc::math::Random rng(seed);

// 基本方法
i32 value = rng.nextInt(100);      // [0, 100)
i32 range = rng.nextInt(10, 20);   // [10, 20]
f32 f = rng.nextFloat();            // [0.0, 1.0)
f64 d = rng.nextDouble();           // [0.0, 1.0)
bool b = rng.nextBoolean();

// 高斯分布
f32 gaussian = rng.nextGaussian(mean, stddev);

// MC风格种子设置
rng.setSeedWithHash(worldSeed);
```

**可用算法：**
- `Mt19937Random` - Mersenne Twister（最高兼容性）
- `Xoroshiro128ppRandom` - xoroshiro128++（默认，高性能）
- `Xoshiro256ppRandom` - xoshiro256++（高质量）
- `LcgRandom` - 线性同余（最小状态）

**随机值范围 (RandomRanges)：**

```cpp
#include "util/math/random/RandomRanges.hpp"

// 均匀分布范围
mc::math::RandomValueRange range(1.0f, 3.0f);
i32 count = range.generateInt(rng);  // [1, 3]

// 二项分布范围
mc::math::BinomialRange binomial(10, 0.3f);  // 10次试验，30%成功率
i32 successes = binomial.generateInt(rng);

// 固定值
mc::math::ConstantRange fixed(5);
i32 value = fixed.generateInt(rng);  // 总是返回5
```

#### 射线检测 (math/ray/)

```cpp
#include "util/math/ray/Raycast.hpp"

Ray ray(origin, direction.normalized());
RaycastContext context(ray, maxDistance);
BlockRaycastResult result = raycastBlocks(context, world);

if (result.hit) {
    BlockPos hitPos = result.pos;
    Direction hitFace = result.face;
    f32 distance = result.distance;
}
```

---

### 4. NBT序列化 (nbt/)

Minecraft NBT（Named Binary Tag）格式序列化库，支持多种格式：

```cpp
#include "util/nbt/Nbt.hpp"
#include <fstream>

using namespace mc::nbt;

// 读取Java版NBT
std::ifstream input("level.dat", std::ios::binary);
input >> contexts::java;
auto root = tags::compound_tag::read(input);

// 访问数据
auto& levelName = root->get<tags::string_tag>("LevelName");
i32 gameType = root->get<tags::int_tag>("GameType");

// 创建NBT数据
tags::compound_tag player;
player.put("name", std::string("Steve"));
player.put("level", 100);
player.put("health", 20.0f);

// 写入文件
std::ofstream output("player.dat", std::ios::binary);
output << contexts::java << player;

// Mojangson文本格式
std::cout << contexts::mojangson << player;
```

**支持格式：**
- Java Edition（大端序）
- Bedrock Network（小端序 + Zigzag）
- Bedrock Disk（小端序）
- Mojangson（文本格式）

---

### 5. 属性系统 (property/)

Minecraft风格的方块状态属性系统，用于表示方块的可变状态。

```cpp
#include "util/property/Properties.hpp"
#include "util/property/StateContainer.hpp"

// 获取预定义属性
const BooleanProperty& lit = BlockStateProperties::LIT;
const DirectionProperty& facing = BlockStateProperties::FACING;
const IntegerProperty& power = BlockStateProperties::POWER_0_15;

// 使用属性
bool isLit = state.get(lit);
Direction dir = state.get(facing);
const BlockState& newState = state.with(facing, Direction::North);

// 创建自定义属性
auto customProp = IntegerProperty::create("custom", 0, 10);
```

**预定义属性：**
- 布尔属性：LIT, POWERED, OPEN, WATERLOGGED, UP, DOWN, NORTH, SOUTH, EAST, WEST 等
- 方向属性：FACING, HORIZONTAL_FACING
- 整数属性：AGE_0_15, LEVEL_0_15, POWER_0_15, ROTATION_0_15 等
- 轴属性：AXIS, HORIZONTAL_AXIS

---

### 6. 其他工具

#### AxisAlignedBB

轴对齐包围盒，用于碰撞检测：

```cpp
#include "util/AxisAlignedBB.hpp"

// 从实体创建
AxisAlignedBB box = AxisAlignedBB::fromPosition(pos, width, height);

// 从方块创建
AxisAlignedBB box = AxisAlignedBB::fromBlock(x, y, z);

// 碰撞检测
if (box.intersects(other)) { ... }

// 计算移动偏移
f32 offsetX = box.calculateXOffset(other, velocityX);
f32 offsetY = box.calculateYOffset(other, velocityY);
f32 offsetZ = box.calculateZOffset(other, velocityZ);
```

#### Direction

六方向枚举及工具函数：

```cpp
#include "util/Direction.hpp"

Direction dir = Direction::North;
Direction opposite = Directions::opposite(dir);  // South
i32 dx = Directions::xOffset(dir);  // 0
i32 dz = Directions::zOffset(dir);  // -1
Axis axis = Directions::getAxis(dir);  // Axis::Z

// 从名称解析
std::optional<Direction> d = Directions::fromName("north");

// 向量转换
Direction dir = Directions::fromVector(dx, dy, dz);

// 旋转和镜像（用于结构生成）
Rotation rot = Rotation::Clockwise90;
Rotation inv = Rotations::getInverse(rot);  // CounterClockwise90
Rotation sum = Rotations::add(Rotation::Clockwise90, Rotation::Clockwise90);  // Clockwise180
i32 degrees = Rotations::toDegrees(rot);  // 90

Mirror mir = Mirror::LeftRight;
Mirror mirInv = Mirrors::getInverse(mir);  // LeftRight（镜像自逆）

// 方向旋转
Direction rotated = Directions::rotateDirection(Direction::North, Rotation::Clockwise90);  // East
```

#### NibbleArray

4位紧凑存储数组，用于光照数据：

```cpp
#include "util/NibbleArray.hpp"

NibbleArray light;
light.set(5, 10, 3, 12);  // 设置位置(5,10,3)的光照为12
u8 value = light.get(5, 10, 3);  // 12

// 填充整个数组
NibbleArray filled = NibbleArray::filled(15);

// 访问底层数据
const std::vector<u8>& data = light.data();
```

#### PlatformInfo

跨平台系统信息获取：

```cpp
#include "util/PlatformInfo.hpp"

using namespace mc::util;

MemoryInfo mem = PlatformInfo::getMemoryInfo();
CpuInfo cpu = PlatformInfo::getCpuInfo();
u64 processMem = PlatformInfo::getProcessMemoryMB();
String platform = PlatformInfo::getPlatformName();
```

#### TimeUtils

高精度时间工具：

```cpp
#include "util/TimeUtils.hpp"

using namespace mc::util;

u64 ms = TimeUtils::getCurrentTimeMs();
u64 us = TimeUtils::getCurrentTimeUs();
```

#### StringUtils

字符串工具函数：

```cpp
#include "util/StringUtils.hpp"

using namespace mc::util;

// ASCII 小写转换
String lower = toLowerAscii("HELLO");  // "hello"

// 数字检测
if (isNumeric("12345")) { ... }        // true
if (isNumeric("-123", true)) { ... }   // true（允许符号）
if (isNumeric("12a45")) { ... }        // false
```

#### CompressionUtils

gzip 压缩/解压工具：

```cpp
#include "util/CompressionUtils.hpp"

using namespace mc::util;

// 解压 gzip 数据
std::vector<u8> compressed = /* ... 从文件读取 ... */;
std::vector<u8> decompressed = decompressGzip(compressed);
if (decompressed.empty()) {
    // 解压失败
}

// 压缩数据为 gzip 格式
std::vector<u8> data = /* ... 原始数据 ... */;
std::vector<u8> compressed = compressGzip(data);
if (compressed.empty()) {
    // 压缩失败
}
```

---

## 整体职责

`util` 模块提供游戏引擎的基础设施组件，是整个项目的底层支持库。职责包括：

1. **运行时诊断**：断言库提供调试和错误检测能力
2. **数据结构**：高性能缓存实现
3. **数学计算**：向量、随机数、射线检测
4. **序列化**：NBT格式支持Minecraft数据兼容
5. **状态管理**：属性系统支持方块状态的类型安全表示
6. **平台抽象**：跨平台系统信息获取

## 输入和输出

### 输入
- 应用程序运行时状态（断言条件）
- 缓存查询请求
- 数学运算参数
- NBT序列化数据流
- 系统信息查询

### 输出
- 断言结果（通过/失败/异常）
- 缓存命中/未命中
- 数学计算结果
- 序列化的NBT数据
- 系统信息结构体

## 依赖项

| 依赖 | 用途 |
|------|------|
| `common/core/Types.hpp` | 基础类型定义 |
| `common/core/Constants.hpp` | 常量定义 |
| `common/world/IWorld.hpp` | 世界接口（射线检测） |
| `spdlog` | 日志（可选，断言处理器） |
| `std::mutex` | 线程同步（缓存） |
| `<random>` | 随机数基础（Mt19937Random） |

## 使用方法

```cpp
// 断言
#include "common/util/assert/AssertAll.hpp"

// 缓存
#include "common/util/cache/OpenAddressingLRUCache.hpp"

// 数学
#include "common/util/math/MathUtils.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/util/math/random/Random.hpp"

// NBT
#include "common/util/nbt/Nbt.hpp"

// 属性
#include "common/util/property/Properties.hpp"

// 其他
#include "common/util/AxisAlignedBB.hpp"
#include "common/util/Direction.hpp"
#include "common/util/NibbleArray.hpp"
#include "common/util/PlatformInfo.hpp"
#include "common/util/TimeUtils.hpp"
```

## 容易踩的坑

### 1. 断言副作用
```cpp
// 错误：断言在Release模式下不会执行
MC_ASSERT(initialize());

// 正确：先执行，再断言
bool ok = initialize();
MC_ASSERT_RELEASE(ok);
```

### 2. 缓存键冲突
```cpp
// 两个缓存的packCoords实现相同，但不要混用key
i64 key1 = Long2IntLRUCache::packCoords(x, z);
i64 key2 = OpenAddressingLRUCache::packCoords(x, z);
// key1 == key2，但应保持一致性
```

### 3. 随机数算法选择
```cpp
// 通过宏选择算法，默认是xoroshiro128++
// 在Random.hpp中修改：
#define MC_RANDOM_XOROSHIRO128PP  // 默认，高性能
// #define MC_RANDOM_MT19937       // 最高兼容性
// #define MC_RANDOM_XOSHIRO256PP  // 高质量
// #define MC_RANDOM_LCG           // 最小状态
```

### 4. NibbleArray延迟分配
```cpp
NibbleArray arr;  // 未分配内存
u8 val = arr.get(0, 0, 0);  // 返回0，不分配
arr.set(0, 0, 0, 5);  // 这时才分配
```

### 5. 属性生命周期
```cpp
// 属性应该是静态或长期存在的
// 错误：创建临时属性
auto prop = BooleanProperty::create("temp");
state.with(*prop, true);  // prop销毁后悬垂引用

// 正确：使用预定义属性
const BooleanProperty& prop = BlockStateProperties::LIT;
```

### 6. 属性状态数量爆炸
```cpp
// 属性组合数量 = 各属性值数量的乘积
// 6方向 * 16等级 * 2布尔 = 192种状态
// 注意控制属性数量避免组合爆炸
```

### 7. 向量浮点比较
```cpp
Vector3 a(1.0f, 0.0f, 0.0f);
Vector3 b(1.0f + EPSILON * 0.5f, 0.0f, 0.0f);
// 使用 approxEqual 进行浮点比较
// Vector3::operator== 内部使用 EPSILON 容差
```

## 测试用例

| 测试文件 | 覆盖模块 |
|---------|---------|
| `tests/common/util/assert/AssertTest.cpp` | 断言库（567行，全面覆盖） |
| `tests/common/util/cache/CacheBenchmark.cpp` | 缓存（性能对比、一致性测试） |

**测试覆盖：**
- 断言所有级别和宏
- 断言自定义处理器
- 断言堆栈跟踪
- 缓存基本操作（put/get/eviction）
- 缓存并发操作
- 缓存性能对比

## 文件统计

| 目录 | 头文件 | 源文件 | 行数 |
|------|--------|--------|------|
| assert/ | 3 | 1 | ~600 |
| cache/ | 2 | 2 | ~400 |
| math/ | 11 | 7 | ~900 |
| nbt/ | 3 | 1 | ~1300 |
| property/ | 9 | 0 | ~900 |
| root | 5 | 3 | ~600 |
| **总计** | **33** | **14** | **~4700** |

## 维护说明

1. **断言库**：添加新的断言级别需要同步更新 `Assert.hpp` 和 `AssertMacros.hpp`
2. **缓存**：修改缓存算法需要运行性能测试确保没有退化
3. **随机数**：新增算法需实现 `IRandom` 接口
4. **NBT**：遵循Minecraft协议规范，不要随意修改序列化格式
5. **属性**：新增属性类型需继承 `Property<T>` 或 `IProperty`
