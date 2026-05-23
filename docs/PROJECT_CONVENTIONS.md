## 项目概述

Minecraft Reborn 是一个现代化的 Minecraft 克隆项目，采用客户端-服务器架构，使用 C++20 编写，使用 Vulkan 进行渲染。该项目旨在尽可能复制 Java 版 1.16.5 的体验，同时保持与现有 Minecraft 生态系统的兼容性（资源包、世界存档、数据包）。

## 关键类型

所有类型都在命名空间 `mc` 中（客户端类型在 `mc::client`，服务端类型在 `mc::server`）：

### 基本类型
- `i8`, `i16`, `i32`, `i64` - 有符号整数
- `u8`, `u16`, `u32`, `u64` - 无符号整数
- `f32`, `f64` - 浮点数（性能优先使用 f32）

### 游戏类型
- `ChunkCoord`, `BlockCoord`, `WorldHeight` - 坐标类型
- `BlockId`, `ItemId`, `EntityId`, `BiomeId`, `DimensionId` - ID 类型
- `PlayerId` - 玩家标识符

### 世界类型
- `ChunkPos`, `BlockPos`, `SectionPos` - 位置类型
- `ChunkId` - 64 位区块标识符
- `BlockState` - 带属性的方块状态
- `ChunkSection` - 16x16x16 方块段
- `ChunkData` - 完整区块数据（16 个段）

### 区块生成类型
- `ChunkStatus`：生成阶段（EMPTY → BIOMES → NOISE → SURFACE → CARVERS → FEATURES → LIGHT → HEIGHTMAPS → FULL）
- `ChunkPrimer`：生成过程中的中间区块状态
- `SingleChunkLifecycleManager`：管理区块加载状态和 future
- `ChunkGenerateTask`：区块生成任务，提交到 ServerWorkerPool 执行
- `IChunk`：生成的区块接口

### 生物群系类型
- `BiomeId` - 生物群系标识符（170 个生物群系，兼容 MC 1.16.5）
- `Biome` - 生物群系定义，包含气候、特性、雕刻器
- `BiomeContainer` - 4x4x4 采样生物群系存储
- `BiomeProvider` - 生物群系分布基类
- `LayerBiomeProvider` - 基于层的生物群系生成（MC 1.16.5）

### 噪声类型
- `INoiseGenerator` - 噪声接口
- `ImprovedNoiseGenerator` - MC 风格柏林噪声
- `OctavesNoiseGenerator` - 多倍频噪声（最多 16 个倍频）
- `PerlinNoiseGenerator`, `SimplexNoiseGenerator` - 其他噪声类型

### 渲染器类型
- `Vertex`, `ModelVertex`, `GuiVertex` - 顶点类型
- `Face` - 三角形面
- `MeshData` - 网格顶点/索引缓冲区
- `TextureRegion` - 图集中的 UV 坐标
- `BakedBlockModel`, `UnbakedBlockModel` - 模型类型

### 渲染器 API 类型（平台无关）
- `IRenderEngine` - 主渲染引擎接口
- `IVertexBuffer`, `IIndexBuffer`, `IUniformBuffer`, `IStagingBuffer` - 缓冲区接口
- `ITexture`, `ITextureAtlas` - 纹理接口
- `ICamera` - 相机接口
- `RenderState` - 混合、深度、光栅化状态
- `RenderType` - 命名渲染类型（MC 1.16.5 风格）

### 雾类型
- `FogMode`：雾模式枚举（None, Linear, Exp2）
- `FogUBO`：雾 uniform 缓冲区数据（fogStart, fogEnd, fogDensity, fogColor）
- `FogManager`：雾效果管理器

### 网络类型
- `PacketType` - 数据包类型枚举
- `PacketHeader` - 12 字节数据包头
- `Packet` - 数据包基类
- `PacketSerializer/Deserializer` - 二进制序列化
- `IServerConnection` - 服务器连接接口
- `LocalEndpoint`, `LocalConnectionPair` - 集成服务器的本地 IPC

### 错误处理
- `Result<T>` - 可能失败操作的结果类型
- `Error` - 包含错误码和消息的错误容器
- `ErrorCode` - 错误码枚举

### 设置类型
- `BooleanOption`, `RangeOption`, `FloatOption` - 设置选项类型
- `EnumOption<T>` - 枚举设置类型
- `StringOption`, `ResourcePackListOption` - 其他设置
- `SettingsBase` - 设置管理基类

## 可复用基础设施

以下列出了项目中可复用的基础类型、工具类和系统组件，应在开发时优先使用。

### 数学工具

**向量类型** (`src/common/util/math/`)
- `Vector2<T>` - 2D 向量模板，支持长度、归一化、点积、叉积、旋转、插值
- `Vector3` - 3D 向量，用于位置/速度，支持 `Position`/`Velocity` 别名
- `Vector4<T>` - 4D 向量模板，用于颜色(RGBA)、齐次坐标、四元数

**数学常量** (`src/common/util/math/MathConstants.hpp`)
- `mc::math::PI`, `PI_DOUBLE`, `TWO_PI`, `TAU`, `HALF_PI`, `QUARTER_PI`, `E`
- `mc::math::EPSILON`, `LARGE_EPSILON`, `FLOAT_MAX`, `FLOAT_MIN`

**度数/弧度转换** (`src/common/util/math/MathUtils.hpp`)
- `mc::math::DEG_TO_RAD`, `RAD_TO_DEG` - 度/弧度转换常量
- `toRadians()`, `toDegrees()` - 度/弧度转换函数

**游戏常量** (`src/common/core/Constants.hpp`)

`mc::game` 命名空间：
- `PLAYER_HEIGHT`, `PLAYER_EYE_HEIGHT`, `PLAYER_WIDTH`, `PLAYER_SNEAK_HEIGHT` - 玩家尺寸
- `PLAYER_MAX_HEALTH`, `PLAYER_MAX_AIR` - 玩家生命值/氧气
- `MAX_LIGHT_LEVEL`, `MIN_LIGHT_LEVEL` - 光照等级范围
- `DAY_LENGTH_TICKS`, `DAY_LENGTH_SECONDS` - 游戏日长度

`mc::network` 命名空间：
- `PROTOCOL_VERSION` - 协议版本
- `DEFAULT_PORT`, `DEFAULT_RCON_PORT` - 默认端口
- `MAX_PACKET_SIZE`, `MAX_UNCOMPRESSED_SIZE`, `MIN_COMPRESSION_THRESHOLD` - 数据包限制
- `CONNECT_TIMEOUT_MS`, `READ_TIMEOUT_MS`, `WRITE_TIMEOUT_MS` - 超时设置
- `KEEP_ALIVE_INTERVAL_MS`, `KEEP_ALIVE_TIMEOUT_MS` - 心跳设置
- `MAX_PACKETS_PER_SECOND`, `MAX_LOGIN_ATTEMPTS` - 速率限制

`mc::world` 命名空间：
- `MIN_BUILD_HEIGHT`, `MAX_BUILD_HEIGHT`, `SEA_LEVEL` - 高度限制
- `CHUNK_WIDTH`, `CHUNK_HEIGHT`, `CHUNK_SECTION_HEIGHT`, `CHUNK_SECTIONS`, `CHUNK_VOLUME` - 区块尺寸
【重要】CHUNK_HEIGHT和MAX_BUILD_HEIGHT目前虽然值一样，但是语义存在巨大区别。
constexpr i32 CHUNK_HEIGHT = MAX_BUILD_HEIGHT - MIN_BUILD_HEIGHT;
未来可能MIN_BUILD_HEIGHT会向下拓展成-64，这时候CHUNK_HEIGHT就不等于MAX_BUILD_HEIGHT了！使用的时候务必小心这个坑！
- `CHUNK_SHIFT`, `SECTION_SHIFT`, `CHUNK_MASK` - 区块位运算常量
- `CHUNK_LOAD_RADIUS`, `CHUNK_UNLOAD_RADIUS`, `MAX_CHUNKS_LOADED` - 区块加载
- `WORLD_SEED_DEFAULT`, `SPAWN_CHUNK_RADIUS` - 世界生成
- `BLOCK_UPDATE_RADIUS` - 方块更新范围

`mc::entity` 命名空间：
- `MAX_ENTITIES_PER_CHUNK`, `MAX_PLAYERS`, `MAX_ENTITIES` - 实体数量限制
- `LegacyEntityTypeId` - 实体类型ID枚举（旧版，用于网络同步）
- `EntityStatus` - 实体状态枚举
- `ENTITY_TRACKING_RANGE`, `PLAYER_TRACKING_RANGE` - 追踪距离

`mc::capacity` 命名空间：
- `DEFAULT_BUFFER_SIZE`, `PACKET_BUFFER_SIZE` - 缓冲区大小
- `ENTITY_LIST_INITIAL`, `CHUNK_MAP_INITIAL`, `PLAYER_LIST_INITIAL` - 容器初始容量
- `MAX_PLAYER_NAME_LENGTH`, `MAX_CHAT_MESSAGE_LENGTH`, `MAX_COMMAND_LENGTH`, `MAX_PATH_LENGTH` - 字符串限制

**数学函数** (`src/common/util/math/MathUtils.hpp`)
- `toRadians()`, `toDegrees()`, `clamp()`, `lerp()`, `smoothstep()`
- `square()`, `cube()`, `isZero()`, `approxEqual()`
- `fastInverseSqrt()`, `floorTo<T>()`, `ceilTo<T>()`, `roundTo<T>()`
- `toChunkCoord()`, `wrapDegrees()`, `distanceSq()`, `distanceHorizontalSq()`

**射线** (`src/common/util/math/ray/Ray.hpp`)
- `Ray` - 射线类，支持 `at(t)` 获取点、`fromAngles()` 从角度创建

### 几何工具

**方向枚举** (`src/common/util/Direction.hpp`)
- `Direction` - 方向枚举（Down, Up, North, South, West, East, None）
- `Axis` - 轴向枚举（X, Y, Z）
- `Rotation` - 旋转枚举（None, Clockwise90, Clockwise180, CounterClockwise90）
- `Mirror` - 镜像枚举
- `Directions` 命名空间：`opposite()`, `xOffset()`, `yOffset()`, `zOffset()`, `getAxis()`, `isHorizontal()`, `isVertical()`, `rotateY()`, `fromName()`, `toBlockFace()`

**轴对齐包围盒** (`src/common/util/AxisAlignedBB.hpp`)
- `AxisAlignedBB` - 碰撞检测核心类
- `fromPosition(pos, width, height)`, `fromBlock(x, y, z)` - 工厂方法
- `intersects()`, `contains()` - 相交/包含检测
- `offset()`, `expand()`, `grow()`, `shrink()` - 变换
- `calculateXOffset()`, `calculateYOffset()`, `calculateZOffset()` - MC 碰撞核心算法

**4位数组** (`src/common/util/NibbleArray.hpp`)
- `NibbleArray` - 4 位紧凑存储（用于光照数据，0-15 范围）
- `get(x, y, z)`, `set(x, y, z, value)` - 坐标访问
- `get(index)`, `set(index, value)` - 索引访问
- `fill()`, `isEmpty()`, `isValid()`, `data()` - 工具方法

### 缓存和工具

**LRU 缓存** (`src/common/util/cache/Long2IntLRUCache.hpp`)
- `Long2IntLRUCache` - 64 位键到 32 位值的 LRU 缓存
- `packCoords(x, z)` - 坐标打包为 64 位键

**限流器** (`src/common/util/RateLimiter.hpp`)
- `RateLimiter` - 每秒调用次数限制器
- `tryAcquire()` - 尝试获取许可

**时间工具** (`src/common/util/TimeUtils.hpp`)
- `TimeUtils::getCurrentTimeMs()` - 毫秒时间戳
- `TimeUtils::getCurrentTimeUs()` - 微秒时间戳

**枚举集合** (`src/common/core/EnumSet.hpp`)
- `EnumSet<T>` - 基于 `std::bitset` 的枚举集合，要求枚举有 `Count` 值

### 实体工具

**物品掉落工具** (`src/common/entity/utils/ItemDropHelper.hpp`)
- `ItemDropHelper` - 物品掉落工具类，统一的随机速度计算和物品实体生成
- `getBlockDropVelocity()` - 方块掉落式随机速度
- `getSimpleDropVelocity()` - 简单随机速度（实体丢弃）
- `getPlayerDropVelocity()` - 玩家丢弃物品速度
- `getGaussianVelocity()` - 高斯分布速度（发射器）
- `spawnItemEntity()` - 在指定位置生成物品实体
- `spawnItemAtEntity()` - 在实体位置生成物品实体
- `spawnItemEntities()` - 批量生成物品实体
- `DEFAULT_PICKUP_DELAY = 10` - 默认拾取延迟（ticks）
- `DEFAULT_LIFETIME = 6000` - 默认存活时间（ticks）
- 参考 MC 1.16.5 `InventoryHelper.spawnItemStack()`, `Entity.entityDropItem()`

### NBT 系统 (`src/common/util/nbt/`)

**核心类型**
- `TagId` - NBT 标签类型枚举（End, Byte, Short, Int, Long, Float, Double, ByteArray, std::string, List, Compound, IntArray, LongArray）
- `tags::tag` - 抽象基类
- `tags::compound_tag` - 复合标签（键值对映射）
- `tags::list_tag` - 列表基类（同类型元素）
- `tags::string_tag`, `tags::int_tag`, `tags::long_tag` 等 - 基本类型标签

**序列化上下文** (`mc::nbt::Context`)
- `Contexts::java` - Java 版格式（大端序二进制）
- `Contexts::bedrock_net` - 基岩版网络格式（小端序 + VarInt/Zigzag）
- `Contexts::bedrock_disk` - 基岩版磁盘格式
- `Contexts::mojangson` - Mojangson 文本格式

**使用示例**
```cpp
mc::nbt::tags::compound_tag tag;
tag.put("name", std::string("Test"));
tag.put("value", 42);
auto& str = tag.get<mc::nbt::tags::string_tag>("name");
```

### 服务器/客户端架构 (`src/server/`, `src/client/`)

**服务器端**
- `IServer` - 服务器接口
- `MinecraftServer` - 服务器抽象基类
- `IntegratedServer` - 内置服务器（单机模式）
- `StandaloneServer` - 独立服务器（多人模式）
- `ServerWorld` - 服务端世界管理器
- `ServerPlayer` - 服务端玩家实体

**客户端端**
- `ClientApplication` - 客户端应用主类
- `ClientWorld` - 客户端世界管理器
- `NetworkClient` - 网络客户端
- `ClientPlayerPredictor` - 客户端玩家预测器

**管理器** (`src/server/core/`)
- `PlayerManager` - 玩家管理
- `ConnectionManager` - 连接管理
- `TimeManager` - 时间管理
- `TeleportManager` - 传送管理
- `KeepAliveManager` - 心跳管理
- `PositionTracker` - 位置追踪
- `PacketHandler` - 数据包处理

### 性能追踪（Perfetto）

### 追踪类别

- `rendering.*` - 帧、Vulkan、区块网格、实体、GUI、天空等
- `game.*` - Tick、实体、物理、AI
- `world.*` - 区块、生物群系、生成阶段
- `network.*` - 数据包、同步、连接
- `server.*` - 服务器 tick、玩家、世界、实体
- 更多类别，参见 `src\common\perfetto\TraceCategories.hpp`

### 用法

```cpp
#include "perfetto/TraceEvents.hpp"

// 初始化
// 作用域事件
MC_TRACE_EVENT("rendering.frame", "RenderFrame");

MC_TRACE_EVENT("server.world",
        "MinecraftServer::handleBlockInteractionPacket",
        "pos", fmt::format("({}, {}, {})", pos.x, pos.y, pos.z),
        "playerId", playerId,
        [flow = ::perfetto::Flow::ProcessScoped(pos.toId())](::perfetto::EventContext ctx) {
            flow(ctx);
});

// 计数器
MC_TRACE_COUNTER("rendering.frame", "FPS", fps);
```

你必须保证MC_TRACE_EVENT/MC_TRACE_COUNTER等的第一个参数在`src\common\perfetto\TraceCategories.hpp`中已经被注册，否则会导致编译错误。

## 错误处理模式

### 禁用异常

**本项目全面禁用 C++ 异常机制。** 所有错误处理使用 `Result<T>` 类型或断言。

**替代方案：**
- **可恢复错误**：使用 `Result<T>` 类型
- **不可恢复错误**：使用 `MC_ASSERT_RELEASE` / `MC_ASSERT_RELEASE_MSG`

**边界处理：** 与第三方库交互时，在边界捕获异常并转换为 `Result<T>`：

```cpp
// JSON 解析边界处理
auto result = mc::json::parse(jsonString);
if (result.failed()) {
    return result.error();
}

// ASIO 网络边界处理
try {
    asio::connect(*m_socket, endpoints);
} catch (const asio::system_error& e) {
    return Error(ErrorCode::ConnectionFailed, e.what());
}
```

### Result<T> 使用模式

```cpp
// 返回值或错误
Result<int> divide(int a, int b) {
    if (b == 0) {
        return Error(ErrorCode::InvalidArgument, "Division by zero");
    }
    return a / b;  // 隐式转换
}

// 检查结果
auto result = divide(10, 2);
if (result.success()) {
    int value = result.value();
} else {
    // 处理错误
}

// 使用 MC_TRY_VAR 简化错误传播
Result<void> process() {
    MC_TRY_VAR(value, divide(10, 2));  // 失败时自动返回错误
    // 使用 value...
    return Result<void>::ok();
}

// 使用 expect() 提供更好的错误信息
int value = result.expect("divide() should never fail");

// 使用 MC_TRY_VOID 处理 Result<void>
Result<void> save() {
    MC_TRY_VOID(validate());
    MC_TRY_VOID(writeToFile());
    return Result<void>::ok();
}
```

### JSON 安全解析

使用 `mc::json::parse()` 进行安全的 JSON 解析（不抛出异常）：

```cpp
#include "util/JsonUtils.hpp"

auto result = mc::json::parse(jsonString);
if (result.failed()) {
    return result.error();
}
const auto& json = result.value();

// 类型安全的字段获取
auto nameResult = mc::json::getString(json, "name");
auto countResult = mc::json::getInt(json, "count");
```

### 错误码

| 类别 | 错误码 |
|------|--------|
| 通用 | Unknown, InvalidArgument, NullPointer, OutOfRange, Overflow, OutOfBounds, InvalidState, InvalidData, NotInitialized |
| 资源 | NotFound, AlreadyExists, ResourceExhausted, OutOfMemory |
| 文件 | FileNotFound, FileOpenFailed, FileReadFailed, FileWriteFailed, FileCorrupted, DecompressionFailed |
| 网络 | ConnectionFailed, ConnectionClosed, ConnectionTimeout, InvalidPacket, ProtocolError |
| 游戏 | InvalidBlock, InvalidItem, InvalidEntity, InvalidPlayer, InvalidWorld |
| 渲染 | InitializationFailed, OperationFailed, CapacityExceeded, Unsupported |
| 权限 | PermissionDenied, Unauthorized |
| 资源包 | ResourcePackNotFound, ResourcePackInvalid, ResourceNotFound, ResourceParseError, TextureLoadFailed, TextureAtlasFull, ModelNotFound, BlockStateNotFound |
| 命令 | CommandNotFound, CommandSyntaxError, CommandPermissionDenied, CommandExecutionFailed, CommandInvalidArgument, CommandExpectedArgument, CommandExpectedLiteral, CommandExpectedSeparator |
| 数据解析 | JsonParseError, NbtParseError, InvalidFormat, DataValidationFailed |
| 存档 | WorldNotFound, WorldCorrupted, WorldLocked, WorldIncompatible, ChunkNotFound, ChunkCorrupted, ChunkSaveFailed, ChunkLoadFailed, SnapshotNotFound, SnapshotCorrupted, SnapshotCreateFailed, SnapshotRestoreFailed, ImportFailed, ExportFailed, RocksDBError, VersionTooNew, ChecksumMismatch |

## 断言库

项目提供了全面的运行时检查断言库。

### 用法

```cpp
#include "common/util/assert/AssertAll.hpp"

// 虽然AssertAll提供了大量断言工具，但目前只允许使用下面的断言宏：

// Release 模式断言（始终启用）
MC_ASSERT_RELEASE(index < capacity);

MC_ASSERT_RELEASE_MSG(index < capacity, "XXXXX");

// 未使用变量标记
MC_UNUSED(unusedParam);
```

## 依赖项

通过 vcpkg 管理：
- **glm** - 数学库
- **spdlog** - 日志
- **nlohmann-json** - JSON 解析
- **glfw3** - 窗口/输入
- **Vulkan** - 图形 API
- **VulkanMemoryAllocator** - GPU 内存管理
- **asio** - 网络（异步 I/O）
- **GTest** - 测试框架
- **stb** - 图像加载
- **perfetto** - 性能追踪

## 随机模块

【重要】所有需要随机数的场景，必须使用下面这个已经封装好的随机数生成器，严禁自行使用mt19937！

```cpp
#include "util/math/random/Random.hpp"

mc::math::Random rng(seed);
i32 value = rng.nextInt(100);    // [0, 100)
i32 range = rng.nextInt(10, 20); // [10, 20]
f32 f = rng.nextFloat();          // [0.0, 1.0)
f32 g = rng.nextGaussian(0.0, 1.0); // 正态分布
```

## 重要说明

### MC Java 源码参考
你可以在 `D:\Minecraft\MC研究\Minecraft1.16.5源码\net\minecraft` 访问 MC Java 1.16.5 源码作为参考。本项目旨在尽可能复制 Java 版游戏玩法。

### 代码质量
- 必须有断言和单元测试
- 每个方法都要有文档注释
- 测试覆盖率必须达到 95%+
- "测试即契约"原则

### 目录结构
保持整洁优雅的目录结构，适当使用子目录。永远不要在一个目录里堆放大量文件。

### 编译警告
所有编译警告必须解决。

### 命名空间使用
使用嵌套命名空间来隔离子系统：
```cpp
namespace mc {
namespace entity {
namespace attribute {
enum class Operation : u8 { ... };
}}}
```

## 自维护规则

**每次重大更改后**（新模型、新页面、新控制器、路由更改、迁移更改、新测试文件、架构变更），更新此 CLAUDE.md 文件以反映当前状态。具体来说：

- 将新模型/控制器/页面/路由添加到下面的相关表格中
- 如果添加了新测试，更新测试计数
- 将任何新的易错点或模式添加到"易错点和陷阱"部分
- 保持此文件作为在此项目上工作的 AI 会话的唯一事实来源

## 日志级别必须使用至少info，因为目前未开放debug级别的日志，debug级别日志看不到。

## 函数参数和配置结构体不允许使用、设置默认值，因为大量的默认值会导致数据流变得难以理解，难以追踪某个值是如何被设置的、是某层默认的还是外部传入的，增加理解和调试难度。若不得不增加默认值，必须征求我的同意。

## 任务进行过程中，遇到不懂的可以随时派出子代理进行核查，而不是亲自查找文件以节省上下文。我必须看到你派出了子代理，而不是一个人独自做！

## 【重要】README.md 使用指南

几乎每个目录下都有一个简体中文编写的 README.md，它很重要，在你探索项目实现的时候，提前看看它，能节省不少多余的文件查找与查看，节省不少上下文开销。

【重要】为了节省上下文，请你尽量阅读各级目录的 README.md 来获取你需要的信息，而不是直接查看代码文件。

例如：当你修改想修改或查看`src\common\world\chunk\ChunkLoadTicketManager.cpp`，你必须提前依次查看目录链上的各级目录上的readme文件：

- 项目根目录的 README.md
- src\common\README.md
- src\common\world\README.md
- src\common\world\chunk\README.md

同理，当你对代码职责、接口、架构等做了修改，必须视情况决定是否递归修改/新增目录链上的各级README.md！

如果你发现当前的README.md内容过时了，或者不够清晰了，或者缺乏重要信息了，你必须更正/更新它！如果你发现自己途径的目录没有README.md了，你必须新增一个！

注意，tests目录不考虑，不要写 README.md。

### 文档内容

每个 README.md 都必须使用简体中文，必须至少包含：
1. **目录结构树** - 清晰展示文件组织
2. **文件介绍** - 每个文件的职责和主要内容
3. **内部模块关系** - 分析各组件之间的依赖关系
4. **外部依赖关系** - 谁依赖了这个目录下的文件？这个目录下的文件依赖了谁？
5. **容易踩的坑** - 常见问题和解决方案

## 其他

### 对于重构类任务的准则

1.不做任何api兼容，直接一步到位新代码（因此主调者需要修改，这符合预期）
2.我有强烈代码洁癖，不允许留任何旧代码旧文件。不允许通过注释等任何手段保留旧代码，不允许以兼容为理由保留旧代码，不允许未经允许的情况下乱加adapter或兼容层来保留旧代码。重构类任务的目标就是把旧代码完全替换掉，留下干净的新实现。

### 对于所有任务的准则

当你完成一个任务后，不要停下来，请继续做后面的任务，直到任务清空你才能停！你时间充足、上下文也充足。

注意本项目基建已经相当完善（代码量已经百万级别），各种常数、常用工具函数、音频系统、粒子系统、资源包系统、命令系统、实体系统、物品系统、物理系统、碰撞、tick调度、存档系统、成就系统等都已经有了相当完善的实现(另外world对象上面也挂了相当多的工具方法以便访问世界、操作世界等)，务必充分探索以实现复用，避免重复实现，或者以未实现为理由留下TODO。
