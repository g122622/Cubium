# Util Module

通用工具库，提供跨项目的基础设施组件。

## 目录结构

```
util/
├── assert/                        # 断言库（运行时检查、堆栈跟踪）
│   ├── Assert.hpp                 # 核心断言类和管理器
│   ├── Assert.cpp                 # 实现
│   ├── AssertMacros.hpp           # 断言宏定义
│   ├── AssertAll.hpp              # 统一包含头文件
│   ├── CrashHandler.hpp           # 崩溃处理器（SEH/信号捕获+调用栈+局部变量输出）
│   ├── CrashHandler.cpp           # 崩溃处理器实现
│   └── README.md                  # 详细文档
├── cache/                         # LRU 缓存实现
│   ├── Long2IntLRUCache.hpp/cpp   # 链表+哈希表实现
│   ├── OpenAddressingLRUCache.hpp/cpp  # 开放寻址实现（推荐）
│   └── README.md                  # 详细文档
├── concurrent/                    # 并发原语（详见 README.md）
│   └── ReentrantAreaLock.hpp/cpp  # 按区块坐标分区的可重入区域锁（对齐 Moonrise）
├── color/                         # 颜色工具
│   └── DyeColor.hpp               # 染料颜色枚举（MC 1.16.5 16色）
├── core/                          # 核心工具
│   └── CoordConverter.hpp         # 区块坐标转换器
├── crypto/                        # 加密哈希工具
│   ├── Sha256.hpp/cpp             # SHA-256（世界种子哈希）
│   ├── Md5.hpp/cpp                # MD5（离线 UUID 生成）
│   └── README.md                  # 详细文档
├── math/                          # 数学工具
│   ├── MathUtils.hpp/cpp          # 数学函数（角度转换、插值等）
│   ├── MathConstants.hpp          # 数学常量（PI、EPSILON 等）
│   ├── Vector2.hpp                # 2D 向量
│   ├── Vector3.hpp                # 3D 向量
│   ├── Vector4.hpp                # 4D 向量模板
│   ├── random/                    # 随机数生成器（详见 README.md）
│   ├── ray/                       # 射线检测（详见 README.md）
│   ├── frustum/                   # 视锥剔除（详见 README.md）
│   └── README.md                  # 详细文档
├── nbt/                           # NBT 序列化（详见 README.md）
│   ├── Nbt.hpp                    # 主头文件
│   ├── Nbt.cpp                    # 实现
│   ├── NbtInternal.hpp            # 内部实现
│   ├── LICENSE                    # MIT 许可证
│   └── README.md
├── property/                      # 方块状态属性系统（详见 README.md）
│   ├── IProperty.hpp              # 属性接口
│   ├── Property.hpp               # 属性模板基类
│   ├── BooleanProperty.hpp        # 布尔属性
│   ├── IntegerProperty.hpp        # 整数属性
│   ├── EnumProperty.hpp/cpp       # 枚举属性
│   ├── DirectionProperty.hpp      # 方向属性
│   ├── StateContainer.hpp         # 状态容器
│   ├── StateHolder.hpp            # 状态持有者
│   ├── Properties.hpp             # 预定义属性集合
│   ├── FluidProperties.hpp        # 流体属性
│   └── README.md
├── text/                          # 富文本系统（详见 README.md）
│   ├── ITextComponent.hpp/cpp     # 文本组件接口
│   ├── ITextComponentFwd.hpp      # 前向声明
│   ├── StringTextComponent.hpp    # 纯文本组件
│   ├── TranslationTextComponent.hpp/cpp  # 翻译键组件
│   ├── TextStyle.hpp/cpp          # 样式定义
│   ├── TextEvents.hpp             # 点击/悬停事件
│   ├── TextParser.hpp/cpp         # § 代码解析器
│   └── README.md
├── thread/                        # 线程工具（详见 README.md）
│   ├── ITask.hpp                  # 任务接口
│   ├── UniversalWorkerPool.hpp/cpp   # 服务端任务池
│   └── README.md
├── AxisAlignedBB.hpp/cpp          # 轴对齐包围盒
├── CompressionUtils.hpp/cpp       # gzip 压缩/解压
├── DateTimeUtils.hpp              # 日期时间格式化/解析（MC Java 版兼容格式）
├── Direction.hpp                  # 方向枚举及工具
├── LinkedHashSet.hpp              # 保持插入顺序的哈希集合
├── NibbleArray.hpp/cpp            # 4 位数组（光照数据）
├── PlatformInfo.hpp/cpp           # 平台信息
├── RateLimiter.hpp                # 限流器
├── StringUtils.hpp                # 字符串工具
├── TimeUtils.hpp                  # 时间工具（时间戳、file_time_type 跨平台转换）
├── SpecialDates.hpp               # 特殊日期工具
└── UuidUtils.hpp/cpp              # UUID 工具（离线 UUID 生成）
```

## 内部模块关系

```
┌─────────────────────────────────────────────────────────────────┐
│                         util 模块                                │
├─────────────────────────────────────────────────────────────────┤
│  assert (独立)     crypto (独立)      text (独立)               │
│  cache (独立)      thread (独立)      property ← Direction      │
│  nbt (独立)        color (独立)       core (独立)               │
│                                                                   │
│  math/                                                             │
│  ├── MathUtils, MathConstants (基础)                             │
│  ├── Vector2/3/4 (依赖 MathUtils)                                │
│  ├── random/ (独立子系统)                                         │
│  ├── ray/ ← Vector3, MathUtils                                   │
│  └── frustum/ ← Vector3, AxisAlignedBB                          │
│                                                                   │
│  根级文件:                                                         │
│  AxisAlignedBB ← Direction, Vector3                              │
│  LinkedHashSet (独立, 仅依赖 STL)                                 │
│  NibbleArray (独立)                                               │
│  PlatformInfo (独立)                                              │
│  RateLimiter (独立)                                               │
│  StringUtils (独立)                                               │
│  TimeUtils (独立)                                                 │
│  DateTimeUtils ← TimeUtils（概念关联，无代码依赖）                 │
│  SpecialDates (独立)                                              │
│  UuidUtils ← Md5                                                  │
│  CompressionUtils (依赖 zlib)                                    │
└─────────────────────────────────────────────────────────────────┘
```

各子模块内部结构详见其 README.md。

## 上下游外部依赖关系

### 上游依赖（本模块依赖的）

| 依赖 | 用途 |
|------|------|
| `common/core/Types.hpp` | 基础类型定义（i8, i32, f32, u64 等） |
| `common/core/Constants.hpp` | 常量定义（PI, EPSILON, CHUNK_WIDTH 等） |
| `common/world/IWorld.hpp` | 世界接口（射线检测） |
| `common/world/block/Block.hpp` | 方块定义（射线检测返回值） |
| `common/command/ICommandSource.hpp` | Uuid 类型定义 |
| `spdlog` | 日志（断言处理器可选） |
| `<random>` | 随机数基础（Mt19937Random） |
| `nlohmann-json` | JSON 解析（ITextComponent） |
| `zlib` | gzip 压缩/解压 |
| `<chrono>` | 日期时间工具（DateTimeUtils） |

### 下游依赖（依赖本模块的）

本模块是整个项目的基础设施层，几乎所有其他模块都依赖：

- **common/world/** - 世界、区块、方块、流体、生物群系等
- **common/entity/** - 实体物理、移动、属性
- **common/network/** - 网络协议序列化
- **server/** - 服务端逻辑、区块生成、随机数
- **client/renderer/** - 渲染、视锥剔除、向量计算
- **item/** - 物品系统、NBT 序列化
- **command/** - 命令解析、NBT 路径

## 容易踩的坑

### 1. 断言宏限制

虽然 AssertAll.hpp 提供了大量断言工具，但**项目只允许使用**：
- `MC_ASSERT_RELEASE(cond)` - Release 模式断言（始终启用）
- `MC_ASSERT_RELEASE_MSG(cond, msg)` - 带消息的 Release 断言
- `MC_UNUSED(var)` - 未使用变量标记

不允许使用 `MC_ASSERT`、`MC_ASSERT_FATAL`、`MC_ASSERT_EQ` 等其他断言宏。

### 2. 断言副作用

```cpp
// ❌ 错误：断言在 Release 模式下不会执行
MC_ASSERT(initialize());

// ✅ 正确：先执行，再断言
bool ok = initialize();
MC_ASSERT_RELEASE(ok);
```

### 3. 随机数生成器必须复用

```cpp
// ❌ 错误：每次调用都创建新生成器
i32 getRandom() {
    Random rng(time(nullptr));  // 序列会不正确
    return rng.nextInt(100);
}

// ✅ 正确：复用同一个生成器
class Game {
    math::Random m_rng;
public:
    Game(u64 seed) : m_rng(seed) {}
    i32 getRandom() { return m_rng.nextInt(100); }
};
```

### 4. 随机数边界

- `nextInt(bound)` 返回 `[0, bound)`，**不包含 bound**
- `nextInt(min, max)` 返回 `[min, max]`，**两端都包含**
- `nextFloat()` 返回 `[0.0, 1.0)`，**不包含 1.0**

### 5. 区块坐标负数处理

`toChunkCoord` 已正确处理负数：`toChunkCoord(-1)` 返回 -1，不是 0。不要自己实现位运算来替代。

### 6. 缓存键打包方法不同

两个缓存类的 `packCoords` 实现不同：
- `Long2IntLRUCache`: `(x << 32) | (z & 0xFFFFFFFF)` - OR 打包
- `OpenAddressingLRUCache`: `(x << 32) ^ z` - XOR 打包

**不要混用两个缓存的键**。

### 7. NBT 忘记设置上下文

读取或写入时未设置正确的上下文会导致数据解析错误：
- `contexts::java` - Java Edition（大端序）
- `contexts::bedrock_net` - Bedrock 网络（小端序 + Zigzag）
- `contexts::bedrock_disk` - Bedrock 磁盘（小端序）
- `contexts::mojangson` - 文本格式

### 8. 属性实例必须唯一

不同实例的同名属性不相等。必须使用静态属性或预定义属性（如 `BlockStateProperties::LIT()`），不要重复创建同名属性。

### 9. 属性状态空间爆炸

属性组合数量 = 各属性值数量的乘积。例如 `6方向 * 16等级 * 2布尔 = 192 种状态`。注意控制属性数量避免组合爆炸。

### 10. NibbleArray 延迟分配

```cpp
NibbleArray arr;  // 未分配内存
u8 val = arr.get(0, 0, 0);  // 返回 0，不分配
arr.set(0, 0, 0, 5);  // 这时才分配内存
```

### 11. 向量浮点比较

`Vector3::operator==` 内部使用 EPSILON 容差，不要直接比较浮点分量。

### 12. 射线方向必须归一化

```cpp
// ❌ 错误：方向未归一化，距离计算会出错
Ray ray(origin, target - origin);

// ✅ 正确：归一化方向
Ray ray(origin, (target - origin).normalized());
```

### 13. 视锥剔除坐标系

- `isAABBVisible()` 期望**相机相对坐标**
- `isAABBVisibleWorld()` 自动转换为相机相对坐标，使用前必须调用 `setCameraPosition()`

### 14. 任务池必须启动

调用 `submit()` 前必须调用 `start()`，否则任务不会执行。

### 15. MD5 仅用于兼容性

MD5 已被证明不安全，**仅用于兼容性需求**（如 Minecraft 离线 UUID 生成），切勿用于密码存储或安全验证。

### 16. DateTimeUtils 时区依赖

`DateTimeUtils::formatDateTime()` 依赖本地时区（与 MC Java 版的 `ZoneId.systemDefault()` 行为一致），使用 `localtime_s/localtime_r`。`parseDateTimeToMillis()` **不**依赖本地时区：使用自实现的 `portableTimegm`（基于 Howard Hinnant 的 `days_from_civil` 算法）将 `struct tm` 视为 UTC 解释，再手动减去字符串携带的时区偏移，得到真实 UTC 时间戳。该实现完全可移植、线程安全，不依赖 `timegm`/`_mkgmtime` 平台扩展，也不修改 `TZ` 环境变量等全局状态。在不同时区的机器上，同一毫秒时间戳格式化出的字符串不同，但往返一致性（format → parse）始终成立。**不要在跨时区通信中使用格式化后的字符串传输时间**，应使用毫秒时间戳（`i64`）。

### 17. DateTimeUtils 与 MC Java 版兼容性

`MC_DATE_FORMAT` 常量（`"%Y-%m-%d %H:%M:%S %z"`）与 MC Java 版的 `DateTimeFormatter.ofPattern("yyyy-MM-dd HH:mm:ss Z")` 完全对应。成就进度（`CriterionProgress`）的 JSON 序列化使用此格式，可直接读写 Java 版存档文件。BannedPlayerList/BannedIpList 也使用此格式存储封禁时间。
