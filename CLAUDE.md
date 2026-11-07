# 📋 Cubium 项目规范

## 项目概述

Cubium 是一个现代化的 Minecraft 克隆项目，采用客户端-服务器架构，使用 C++20 编写，使用 Vulkan 进行渲染。该项目旨在尽可能复制 Java 版 1.21.11 的体验，同时保持与现有 Minecraft 生态系统的兼容性（资源包、世界存档、数据包），崇尚优雅、整洁、清晰的架构和极致的高性能与低内存占用。

## 依赖规范

- src/common不能依赖src/client和src/server
- 反之，src/client和src/server可以依赖src/common
- 为了保证common目录的简洁，所有client和server的专属逻辑都必须放在src/client和src/server目录下，不能放在common目录下

## git 规范

大部分任务直接在 `main` 提交并推送到 `origin/main`。除非用户指定，否则不要开新的分支，也不要开pr。

提交信息格式：
```
<type>(<scope>): <subject>
<body>
```

合并方式：不允许使用线性历史（rebase）。


### pr规范

当用户命令你采用 PR 流程时，提交代码并完成所有工作之后，使用 gh 工具发起 PR。本项目使用了若干机器人进行自动代码审查、以及clang-tidy进行静态分析，因此需要在 PR 发起之后等待至少 10 分钟（这是为了等待自动化机器人完成代码审查和静态分析完毕），机器人的审查结论会以评论的形式反馈在 PR 上。等待机器人完成审查之后，读取这些评论并判断是否需要采纳，并等待静态分析出结果；如需修改代码，则提交修改后的代码（此时不用等机器人再次审查，直接操作 gh 工具把这个分支合入 main）。最后删除当前分支并切换回main。

## 关键类型

所有类型都在命名空间 `mc` 中（客户端类型在 `mc::client`，服务端类型在 `mc::server`）：

### 基本类型
- `i8`, `i16`, `i32`, `i64` - 有符号整数
- `u8`, `u16`, `u32`, `u64` - 无符号整数
- `f32`, `f64` - 浮点数

统一使用上述类型别名，不允许使用int、float等关键字。

### 游戏类型
- `ChunkCoord`, `BlockCoord`, `WorldHeight` - 坐标类型
- `BlockId`, `ItemId`, `EntityId`, `BiomeId`, `DimensionId` - ID 类型
- `PlayerId` - 玩家标识符

### 世界类型
- `ChunkPos`, `BlockPos`, `SectionPos`, `GlobalPos`(相当于BlockPos+维度id) - 位置类型
- `ChunkId` - 64 位区块标识符
- `BlockState` - 带属性的方块状态
- `ChunkSection` - 16x16x16 方块段
- `ChunkData` - 完整区块数据（CHUNK_SECTIONS 个段，主世界为 24）

### 区块生成类型
- `ChunkStatus`：生成阶段（EMPTY → STRUCTURE_STARTS → STRUCTURE_REFERENCES → BIOMES → NOISE → SURFACE → CARVERS → FEATURES → INITIALIZE_LIGHT → LIGHT → SPAWN → FULL）
- `ChunkPrimer`：生成过程中的中间区块（累积式，同一对象贯穿所有状态）
- `ChunkProgressionTask`：单状态推进任务，提交到 UniversalWorkerPool 执行（`src/server/world/`）
- `ChunkTaskScheduler`：生成调度核心（双向邻居依赖图 + 区域锁，`src/server/world/`）
- `SingleChunkLifecycleManager`：单区块生命周期状态机（NewChunkHolder 等价物，`src/server/world/`）
- `IChunk`：区块接口

### 生物群系类型
- `BiomeId` - 生物群系标识符（含主世界/下界/末界，对齐 MC 1.21.11）
- `Biome` - 生物群系定义，包含气候、特性、雕刻器
- `BiomeContainer` - 4x4x4 采样生物群系存储
- `IBiomeSource` - 生物群系分布接口（`common/world/biome/BiomeSource.hpp`）
- `MultiNoiseBiomeSource` / `EndBiomeSource` / `FixedBiomeSource` - 具体分布实现（`server/world/gen/biome/source/`）

### 网络类型
- `protocol::ConnectionProtocol` - 连接协议阶段枚举（Handshake/Login/Configuration/Play），位于 `network/protocol/ConnectionProtocol.hpp`
- `protocol::PacketFlow` - 数据包流向（Clientbound/Serverbound），位于 `network/protocol/PacketFlow.hpp`
- `protocol::PacketType` - 数据包类型标识（`{PacketFlow flow; std::string id;}` + `PacketTypeHash`），位于 `network/protocol/PacketType.hpp`
- `ir::IrPacket` - IR 数据包根类型（variant-of-variants，阶段变体 → 叶子包），位于 `network/ir/IrPacket.hpp`
- `ir::play::*` - Play 阶段叶子包结构体（如 `BlockUpdate`、`KeepAlive`、`LevelChunkWithLight`、`AddEntity`），位于 `network/ir/packets/play/`
- `buffer::RegistryByteBuf` - IR 编解码所用的字节缓冲区，位于 `network/buffer/RegistryByteBuf.hpp`
- `codec::StreamCodec` / `codec::IdDispatchCodec` - IR 编解码器框架，位于 `network/codec/`
- `PacketSerializer` / `PacketDeserializer` - 二进制序列化原语（位于 `network/codec/`，ChunkSync 等仍在用，非 Packet 基类）
- Java wire 编解码器 - 位于 `network/backend/java/codecs/`（如 `JavaPlayCodecs.hpp`）
- `pipeline::Connection<RegistryByteBuf>` - 网络管道连接，对外 API 为 `send(ir::IrPacket)` / `onPacket(listener)`，位于 `network/pipeline/Connection.hpp`
- `transport::LocalTransport` - 集成服务器同进程零拷贝直传 IR 包（取代旧 IServerConnection/LocalEndpoint/LocalConnectionPair）

**线格式**：Java 后端为 VarInt(length) + VarInt(packetID) + payload，由 `backend/java/codecs/` 中的 codec 进行 IR ↔ wire 双向转换。旧的 12 字节定长头与 `ConnectionManager::encapsulatePacket`/`PacketHandler::handlePacket`/`dispatchPacket` 漏斗已删除，入站分发改为 `ServerPlayRouter`（服务端）和客户端 `ClientPlayVisitor`，均通过 `std::visit` 遍历 `ir::PlayPacket`。

**业务枚举**（位于 `network/protocol/`）：
- `EntityAnimation` / `EntityStatus` → `protocol/EntityEvents.hpp`
- `BlockInteractionAction` / `BossInfoAction` / `PlayerAbilityFlags` → `protocol/GameActions.hpp`
- `TitleAction` → `protocol/TitleActions.hpp`

### 错误处理
- `Result<T>` - 可能失败操作的结果类型
- `Error` - 包含错误码和消息的错误容器
- `ErrorCode` - 错误码枚举
- 新增代码不允许使用c++异常，统一走Result/Error体系。

### 设置类型
- `BooleanOption`, `RangeOption`, `FloatOption` - 设置选项类型
- `EnumOption<T>` - 枚举设置类型
- `StringOption`, `ResourcePackListOption` - 其他设置
- `SettingsBase` - 设置管理基类

## 可复用基础设施

这些文件/目录提供了大量丰富的基础设施以供复用，包括但不限于：

```
src/common/core/
├── Types.hpp                    # 基础类型定义（i8/u8/f32等、游戏类型、枚举）
├── Result.hpp                   # 错误处理系统（Result<T>、Error、ErrorCode）
├── Result.cpp                   # Result 实现
├── Constants.hpp                # 游戏常量（命名空间组织：game/network/world/entity/item/capacity）
├── EnumSet.hpp                  # 枚举集合工具（基于 std::bitset）
├── BlockRaycastResult.hpp       # 方块射线投射结果类型
├── GameDirectory.hpp/cpp        # 游戏目录管理器（统一管理所有游戏路径）
├── DefaultValues.hpp            # 集中默认值定义
└── settings/                    # 设置系统
    ├── SettingsBase.hpp/cpp     # 设置基类（JSON 持久化、变更通知）
    ├── SettingsTypes.hpp        # 设置选项类型（Boolean/Range/Float/Enum/String）
    └── ResourcePackListOption.hpp  # 资源包列表选项

src/common/world/
├── IWorld.hpp/cpp              # 世界访问接口
├── IWorldWriter.hpp            # 世界写入接口（生成用）
├── WorldConstants.hpp          # 世界常量和坐标转换工具，如世界高度限制、区块尺寸等
├── WorldConfig.hpp             # 世界配置
├── WorldEvents.hpp             # 世界事件ID常量
├── GlobalPos.hpp               # 全局位置类型

src/common/world/chunk/
├── base/                       # 位置与标识纯值类型（ChunkPos、SectionPos、ChunkId）
├── data/                       # 区块数据模型（ChunkData、ChunkPrimer、ChunkSection、Heightmap、BiomeContainer、IChunk）
├── gen/                        # 生成管线（ChunkStatus、ChunkPyramid、ChunkStep、ChunkDependencies）
└── load/                       # 加载票据系统（ChunkLoadTicket、ChunkLoadTicketManager、ChunkDistanceGraph、ChunkLoadLevel）

block/
├── Block.hpp/cpp                   # 方块基类，定义核心属性和行为
├── BlockState.hpp/cpp              # 方块状态类，不可变状态对象
├── BlockPos.hpp                    # 方块位置坐标类
├── BlockRegistry.hpp/cpp           # 方块注册表（单例）
├── BlockSoundType.hpp/cpp          # 方块声音类型定义
├── BlockTags.hpp/cpp               # 方块标签系统（分组判断），可用于判断方块类型，推荐首选！
├── FireInfoRegistry.hpp/cpp        # 火焰信息注册表（燃烧/蔓延属性）
├── HarvestTool.hpp                 # 挖掘工具类型定义
├── IBeaconBeamColorProvider.hpp    # 信标光束颜色提供者接口
├── IBucketPickupHandler.hpp        # 桶提取接口
├── IGrowable.hpp                   # 可生长方块接口
├── ILiquidContainer.hpp            # 液体容器接口
├── IWaterLoggable.hpp/cpp          # 含水方块接口
├── Material.hpp/cpp                # 材质系统（物理属性）
├── PlantType.hpp                   # 植物类型定义
├── WaterLoggableHelpers.hpp        # 含水方块工具函数

```

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
- `MAX_LIGHT_LEVEL`（一般为15）, `MIN_LIGHT_LEVEL`（一般为0） - 光照等级范围
- `DAY_LENGTH_TICKS`, `DAY_LENGTH_SECONDS` - 游戏日长度

`mc::network` 命名空间：
- `PROTOCOL_VERSION` - 协议版本
- `DEFAULT_PORT`, `DEFAULT_RCON_PORT` - 默认端口
- `MAX_PACKET_SIZE`, `MAX_UNCOMPRESSED_SIZE`, `MIN_COMPRESSION_THRESHOLD` - 数据包限制
- `CONNECT_TIMEOUT_MS`, `READ_TIMEOUT_MS`, `WRITE_TIMEOUT_MS` - 超时设置
- `KEEP_ALIVE_INTERVAL_MS`, `KEEP_ALIVE_TIMEOUT_MS` - 心跳设置
- `MAX_PACKETS_PER_SECOND`, `MAX_LOGIN_ATTEMPTS` - 速率限制

`mc::world` 命名空间：
- `MIN_BUILD_HEIGHT`, `MAX_BUILD_HEIGHT`, `SEA_LEVEL` - 高度限制。【重要】只能使用这些mc::world下的常量作为高度限制，不能硬编码0、256等数字，因为未来可能会频繁修改这些高度限制。
- `CHUNK_WIDTH`, `CHUNK_HEIGHT`, `CHUNK_SECTION_HEIGHT`, `CHUNK_SECTIONS` - 区块尺寸。【重要】只能使用这些mc::world下的常量作为区块尺寸，不能硬编码16等数字，因为未来可能会频繁修改区块尺寸。另外，有些地方可能使用位运算来计算区块坐标（例如 `x >> CHUNK_SHIFT`），务必使用这些常量来确保位运算的正确性，而不是简单 >> 4。
【重要】CHUNK_HEIGHT和MAX_BUILD_HEIGHT值不同，语义也完全不同。CHUNK_HEIGHT = MAX_BUILD_HEIGHT - MIN_BUILD_HEIGHT = 320 - (-64) = 384，而MAX_BUILD_HEIGHT = 320。使用的时候务必小心这个区别！
- `CHUNK_SHIFT`, `SECTION_SHIFT`, `CHUNK_MASK` - 区块位运算常量
- `CHUNK_LOAD_RADIUS` - 默认区块加载半径
- `SPAWN_CHUNK_RADIUS` - 出生点常驻区块半径

世界常量、尺寸与坐标换算工具统一定义在 `src/common/world/WorldConstants.hpp`（`Constants.hpp` 通过 include 重新导出）。使用时直接 include 该头文件并引用其中常量/函数，不要硬编码数字；具体清单以该文件为准，本文档不再重复罗列——重复罗列会随代码演进迅速失真。

`mc::entity` 命名空间：
- `MAX_ENTITIES_PER_CHUNK`, `MAX_ENTITIES` - 实体数量限制

`mc::item` 命名空间：
- `DEFAULT_MAX_STACK_SIZE` - 物品默认最大堆叠数（64）

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
- 参考 MC Java `InventoryHelper.spawnItemStack()`、`Entity.entityDropItem()`（对齐 1.21.11）

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

### 性能追踪（Perfetto + Tracy 双轨）

项目内置两套 profiler 后端，可独立开关、同时启用（双轨录制），对外 `MC_TRACE_*` API 宏名统一：

- **Perfetto**（`MC_ENABLE_TRACING`，默认 ON）：进程内录制到 `.perfetto-trace` 文件，供 ui.perfetto.dev 分析。
- **Tracy**（`MC_ENABLE_TRACY`，**默认 OFF**）：in-memory 采集，client 自动监听 8086 端口，用 tracy GUI 连接拉取查看。

#### 追踪类别

追踪类别参见 `src\common\profiler\TraceCategories.hpp` 中的 `mc::trace::TraceEvents` 枚举树。

#### 用法

```cpp
#include "common/profiler/TraceEvents.hpp"

// 作用域事件（推荐）；category 取自 TraceEvents 枚举树
MC_TRACE_SCOPED_EVENT(TraceEvents.Rendering.Frame, "RenderFrame");

MC_TRACE_SCOPED_EVENT(TraceEvents.Server.World,
        "MinecraftServer::handleBlockInteractionPacket",
        "pos", fmt::format("({}, {}, {})", pos.x, pos.y, pos.z),
        "playerId", playerId,
        [flow = ::perfetto::Flow::ProcessScoped(pos.toId())](::perfetto::EventContext ctx) {
            flow(ctx);
});

// 计数器
MC_TRACE_COUNTER(TraceEvents.Rendering.Frame, "FPS", fps);

// 瞬时事件
MC_TRACE_INSTANT_EVENT(TraceEvents.Game.Tick, "SomethingHappened");
```

可用宏：`MC_TRACE_SCOPED_EVENT`（作用域事件，最常用）、`MC_TRACE_INSTANT_EVENT`（瞬时事件）、`MC_TRACE_COUNTER`（计数器）、`MC_TRACE_EVENT_BEGIN`/`MC_TRACE_EVENT_END`（手动跨函数配对）、`MC_TRACE_SET_THREAD_NAME`（线程命名）。

你必须保证传入的枚举叶节点在 `src\common\profiler\TraceCategories.hpp` 中已注册，否则会导致编译错误（仅 Perfetto 后端启用时生效；Tracy 不消费 category）。

## 错误处理模式

对可能失败的操作使用 `Result<T>`：

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
```

若要使用错误码，必须参照相关定义文件。

## 断言库

项目提供了全面的运行时检查断言库。

【重要】代码中需要加上大量断言来保证代码质量，以便在运行时及时发现和定位问题。

【重要】**所有日志（spdlog）与断言消息（`MC_ASSERT_*_MSG`）必须用英文书写**。注释与文档用简体中文，但日志/断言面向的是运行输出与崩溃报告，统一英文便于检索与和原版日志对照。

用法：

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

- **spdlog** - 日志。
- **nlohmann-json** - JSON 解析
- **asio** - 网络（异步 I/O）
- **GTest** - 测试框架
- **stb** - 图像加载

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

### 代码质量
- 必须有断言和单元测试
- 每个方法都要有文档注释
- 测试覆盖率必须达到 95%+
- "测试即契约"原则

### 目录结构
保持整洁优雅的目录结构，充分使用子目录、子目录的子目录，永远不要在一个目录里堆放大量文件，这会很乱！

### 命名空间使用
使用嵌套命名空间来隔离子系统：
```cpp
namespace mc {
namespace entity {
namespace attribute {
enum class Operation : u8 { ... };
}}}
```

## 容易踩的坑

本节记录项目中反复出现、难以排查的陷阱。新增同类代码前务必先核对这里，避免重蹈覆辙。各子目录 README 的"容易踩的坑"章节记录更局部的坑，本节只收录跨模块、影响面大的通用坑。

### 0. 原版数据一律运行时从数据包读取，不要生成 C++ 代码

项目要像原版 MC 一样：**在初始化阶段读取数据包**，而不是把数据包内容展开成 `.gen.cpp` 编译进去。区块生成系统、`ItemTagLoader`、`BlockTagLoader` 都是这条路线。

- ✅ 正确做法：`data/<ns>/...` 下的 JSON 在启动期由加载器读取（`GenericTagLoader` / `ItemTagLoader` / `BlockTagLoader` 等），支持多数据包合并与 `replace` 语义，改数据包即改行为，无需重编译。
- ❌ 错误做法：写脚本把数据包展开成 `*.gen.cpp` 内嵌进二进制。这类文件与数据包会无声漂移，且玩家/服务端无法通过数据包定制。
- 例外仅限**数据包中根本不存在**的信息（如原版硬编码在注册代码里的属性）。这类数据以「实测快照」形式内嵌，并在注释中写明：它为什么无法从数据包得到、数值从何而来。

### 1. 高度图 +1 语义：`ctx.getHeight` vs `getTopBlockY`

MC 1.21.11 生成期间，`WorldGenContext.getHeight(...)` 走 `WorldGenRegion.getHeight`，其实现为 `chunk.getHeight(...) + 1`（`WorldGenRegion.java:404-407`），即返回"最高方块上方一格空气的 Y"（blockY+1），**不是方块本身 Y**。而 `ChunkAccess.getHeight` 返回 blockY（`getFirstAvailable-1`）。

项目的 `WorldGenRegion::getTopBlockY` 转发 `ChunkPrimer::getTopBlockY`，返回 blockY（等价 MC `ChunkAccess.getHeight`），**少了这个 +1**。因此：**凡是对应 MC `ctx.getHeight(...)` 调用的 placement/feature 代码，必须对 `getTopBlockY` 结果 +1**。

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

每个 README.md 都必须使用简体中文，且语言凝练、信息密度高，必须至少包含：
1. ✅ **目录结构树** - 清晰展示文件组织。每个文件后面必须加上一句话描述，且可通过括号补充说明一些要点，如：
```
src/client/renderer/
├── api/                          # 平台无关的渲染抽象接口
│   ├── BlendMode.hpp             # 混合状态定义
│   ├── CompareOp.hpp             # 深度比较操作定义
│   ├── CullMode.hpp              # 面剔除模式定义
│   ├── IRenderEngine.hpp         # 渲染引擎主接口
│   ├── TridentApi.hpp            # Trident API 统一头文件
│   ├── Types.hpp/cpp             # 基础类型定义（顶点、面、枚举）
后略
```
2. ✅ **内部模块关系** - 分析各组件之间的依赖关系
3. ✅ **上下游外部依赖关系** - 谁依赖了这个目录下的文件？这个目录下的文件依赖了谁？
4. ✅ **容易踩的坑** - 常见问题和解决方案。如果你需要缩减文档内容，必须优先保留这个部分，因为它能直接提升开发效率，减少不必要的错误和调试时间。

除此之外，不允许出现其他内容，如：
- ❌ 每个文件的具体职责和详细描述
- ❌ 具体的代码示例（除非非常必要）
- ❌ 涉及了哪些单元测试
- ❌ 过于细节的实现细节（如某个函数的具体实现）
- ❌ 已实现的进度
- ❌ 未来的计划、未实现的地方等

如果你在阅读的过程中发现了这些不该有的内容，直接删除它们！如果你觉得有必要保留一些细节内容，必须放在容易踩的坑部分，并且要非常简练！

原则上，单个 README.md 不应该超过300~500行（巨型目录下的除外，因为巨型目录光是目录结构树就很大了）。

## 其他

### 关于配置和传参

函数参数和配置结构体不允许使用和设置默认值，因为大量的默认值会导致数据流变得难以理解，难以追踪某个值是如何被设置的、是某层默认的还是外部传入的，增加理解和调试难度。若不得不增加默认值，必须征求我的同意。

### 我个人对架构的偏好

- 对于一个子系统，我偏好一个单独的门面类来对外承接所有的接口，其他类都不对外暴露。例如`src\common\world\storage`是存储子系统，其只对外暴露了GlobalStorageManager和SingleLevelStorageManager这两个门面类，前者负责世界列表、打开存档等，后者负责单存档运行时的区块/玩家/实体/方块实体读写、保存协调等。其他的类和文件，外界不允许直接使用（基础类型等除外）。
- 对于目录结构，我偏好分门别类的、有较大深度的树形结构，而不是平铺的结构。此外，如果一个目录下的文件被且仅被另一个目录所依赖，则前者要在后者的目录树下，而不是与之平级甚至高级。
- 对于一个子系统下的基础文件（比如类型声明、工具库等），且它们是依赖链的末梢，则将他们放在该子系统的base目录下。例如：`base/buffer/`、`base/codec/`、`base/types/`等。
- 不允许出现容易让人混淆的api/类名/方法名等。这里的混淆，既包括单个的混淆，也包括相互之间的混淆。

### 关于“对齐MC”这件事的边界

我希望能在逻辑、数值、时序、协议等方面完整对齐MC，但在对齐过程中，务必使用更清晰的架构、更高性能的实现等。此外，**不要**对诸如“浮点精度”、“数学精度”、“-ffast-math”、“溢出”、“精度累积”的这种细枝末节的地方进行对齐，避免因此增加开发复杂度和性能负担。

### 对于重构类任务的准则

1.不做任何api兼容，直接一步到位新代码（因此主调者需要修改，这符合预期）
2.我有强烈代码洁癖，不允许留任何旧代码旧文件。不允许通过注释等任何手段保留旧代码，不允许以兼容为理由保留旧代码，不允许未经允许的情况下乱加adapter或兼容层来保留旧代码。重构类任务的目标就是把旧代码完全替换掉，留下干净的新实现。
3. 我能容忍激进重构后导致的编译错误和运行时错误，请你放行大胆进行重构。

### 对于所有任务的准则

当你完成一个任务后，不要停下来，请继续做后面的任务，直到任务清空你才能停！你时间充足、上下文也充足。

注意本项目基建已经相当完善（代码量已经百万级别），各种常数、常用工具函数、音频系统、粒子系统、资源包系统、命令系统、实体系统、物品系统、物理系统、碰撞、tick调度、存档系统、成就系统等都已经有了相当完善的实现(另外world对象上面也挂了相当多的工具方法以便访问世界、操作世界等)，务必充分探索以实现复用，避免重复实现。

在进行需求设计与缺陷修复时，优先采取系统性、结构化的代码架构。每当面对一项需求或修复任务，应首先审视整个代码仓库及上下游依赖，考量是否存在同类模式，并进行全局性的统一优化，而非仅限于单一层面的修补。拒绝以临时性的兜底方案掩盖问题，而应由底向上深究，致力于探寻更为优化的解决方案。

---

当你发现了存量代码中有任何地方违反了上述任意规范，请你顺手直接修改它们，你有权限直接修改任何违反规范的代码。

# 📋 Cubium 代码规范

## 1. 总则

### 1.1 规范目的

本规范旨在确保项目代码的**一致性**、**可维护性**、**可读性**和**安全性**，降低团队协作成本，提高代码质量。

### 1.2 例外处理

如有特殊情况需要违反本规范，必须：
1. 立即停止，向用户询问，征得同意之后方可继续
2. 在代码中添加明确注释说明原因
3. 结束任务之后给用户明确反馈

### 1.3 有关防御性编程

不要过度防御性编程，这会导致代码臃肿、不易发现真正的bug和架构缺陷。只在必要的边界和不受信任的输入处进行防御性检查，其他地方可以假设前置条件已经满足。

❌ 不好的例子：

```cpp
    // 区块 tick - 包括区块内实体、方块随机刻、区块状态更新等
    if (m_chunkManager && pendingTasksCount >= 0) {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Tick, "ServerWorld::tick::ChunkTick");
        m_chunkManager->tick(pendingTasksCount);
    }

    // 光照更新 - 限制每 tick 最多处理 32768 个区块，避免过长卡顿
    if (m_lightManager && m_lightManager->hasLightWork()) {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Tick, "ServerWorld::tick::LightManager");
        m_lightManager->tick(32768, true, true);
    }

    if (m_skyLight != nullptr) {
        m_skyLight->forceHandleEmptySectionChanges(m_provider, chunk, emptySections);
    }
```

✅ 正确例子，最佳实践：

```cpp
    // 区块 tick - 包括区块内实体、方块随机刻、区块状态更新等
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Tick, "ServerWorld::tick::ChunkTick");
    MC_ASSERT_RELEASE(pendingTasksCount >= 0); // 转为断言，便于迅速暴露问题
    m_chunkManager->tick(pendingTasksCount);

    // 光照更新 - 限制每 tick 最多处理 32768 个区块，避免过长卡顿
    if (m_lightManager->hasLightWork()) {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Tick, "ServerWorld::tick::LightManager");
        m_lightManager->tick(32768, true, true);
    }

    m_skyLight->forceHandleEmptySectionChanges(m_provider, chunk, emptySections);
```

这可能会导致一些空指针访问，但这通常是由于架构设计缺陷或其他bug引起的，过度防御会掩盖这些问题。正确的做法是通过单元测试、集成测试和代码审查来发现和修复这些问题，而不是在每个调用处添加冗余检查。

我能容忍这些错误发生，但我不能容忍你通过防御性编程来掩盖这些错误。

如果你在编写代码过程中发现现存了一些这样的冗余检查，务必顺手删除它们。

---

## 2. C++语言特性规范

### 2.1 C++标准

强制使用C++20标准。

### 2.2 允许使用的C++20特性

| 特性 | 使用建议 | 示例 |
|------|----------|------|
| `auto` | ✅ 推荐 | `auto iter = vec.begin();` |
| 结构化绑定 | ✅ 推荐 | `auto [x, y, z] = position;` |
| `std::optional` | ✅ 推荐 | `std::optional<Block> getBlock();` |
| `std::variant` | ✅ 推荐 | `std::variant<Block, Entity> hitResult;` |
| `std::any` | ⚠️ 谨慎 | 避免过度使用 |
| `if constexpr` | ✅ 推荐 | 模板元编程 |
| `std::string_view` | ✅ 推荐 | 只读字符串参数 |
| 折叠表达式 | ✅ 推荐 | 模板编程 |
| `std::filesystem` | ✅ 推荐 | 文件操作 |

### 2.3 禁止使用的特性

```cpp
// ❌ 禁止：C风格强制转换
int* ptr = (int*)malloc(sizeof(int));

// ✅ 推荐：C++风格强制转换
int* ptr = static_cast<int*>(malloc(sizeof(int)));

// ❌ 禁止：原始数组
int arr[10];

// ✅ 推荐：std::array或std::vector
std::array<int, 10> arr;
std::vector<int> vec;

// ❌ 禁止：宏定义常量
#define MAX_PLAYERS 100

// ✅ 推荐：constexpr常量
inline constexpr int MAX_PLAYERS = 100;

// ❌ 禁止：异常会导致性能问题，建议使用Result/Expected类型处理错误
// 对于现有存量代码中的异常处理，考虑到兼容性，暂时不强制重构，但新代码中强烈建议不要使用异常。
try {
    // 正常逻辑
} catch (...) {
    // 错误处理
}

// ✅ 推荐：使用Result/Expected类型
Result<void> result = doSomething();
if (!result.success()) {
    // 错误处理
}
```

### 2.4 类设计规范

```cpp
// ✅ 推荐：Rule of Five
class Chunk {
public:
    Chunk();                                    // 默认构造函数
    Chunk(const Chunk& other);                  // 拷贝构造
    Chunk(Chunk&& other) noexcept;              // 移动构造
    Chunk& operator=(const Chunk& other);       // 拷贝赋值
    Chunk& operator=(Chunk&& other) noexcept;   // 移动赋值
    ~Chunk();                                   // 析构函数
    
    // 工厂方法
    static std::unique_ptr<Chunk> create(int32_t x, int32_t z);
    
private:
    // 数据成员
    std::vector<Block> m_blocks;
    ChunkPos m_position;
    // 私有方法（私有方法必须加前缀_以示区分）
    void _loadFromDisk();
};

// ✅ 必须：明确指定访问控制
class Entity {
public:
    // 公共接口
    void update(float deltaTime);
    
protected:
    // 派生类可访问
    virtual void onTick();
    
private:
    // 私有实现
    void _internalUpdate();
    EntityID m_id;
};

// ❌ 禁止：友元滥用
class A {
    friend class B;  // 除非必要，否则避免
};
```

### 2.5 函数设计规范

```cpp
// ✅ 推荐：参数顺序（必要参数在前，可选参数在后）
void spawnEntity(EntityType type, 
                 Vector3 position,
                 std::optional<EntityData> data = std::nullopt);

// ✅ 推荐：使用string_view传递只读字符串
void loadTexture(std::string_view path);

// ✅ 推荐：const引用传递大对象
void processChunk(const Chunk& chunk);

// ✅ 推荐：移动语义
void setTexture(std::unique_ptr<Texture> texture);

// ✅ 推荐：[[nodiscard]]标记必须检查返回值的函数
[[nodiscard]] Result<void> initialize();

// ✅ 推荐：[[nodiscard]]标记可能产生新资源的函数
[[nodiscard]] std::unique_ptr<Entity> createEntity();

// ✅ 推荐：复用BlockPos、ChunkPos等已有工具类表示方块/区块坐标，降低参数复杂度
```

---

### 2.6 控制流与代码布局

1. `if`、`else`、`for`、`while`、`do` 等控制流语句的代码块必须使用大括号，即使只有一条语句也不得省略。代码块中的语句必须另起一行，禁止将条件、循环和执行语句压缩在同一行。
2. 不要将多个独立语句堆叠在同一行。函数、lambda 和控制流代码块应采用展开的多行格式，空代码块除外。
3. 适当增加简体中文注释，解释设计意图、对象所有权、执行时序、边界条件和复杂逻辑；不要用重复代码字面含义的注释增加噪声。
4. 使用空行分隔参数校验、状态准备、核心处理和资源清理等逻辑段落；函数定义之间保留空行。不要将整个函数挤成连续代码，也不要在每条简单语句之间机械插入空行。
5. `.clang-format` 必须保留自动补充控制流大括号、展开非空短代码块和分隔定义的配置，防止格式化时重新压缩代码。

```cpp
if (callback == nullptr) {
    return;
}

// 先取出待处理任务，再在锁外执行，允许回调继续注册任务。
auto pending = takePendingCallbacks();
for (auto& task : pending) {
    task();
}

clearCompletedTasks();
```

---

## 3. 命名规范

### 3.1 文件命名

| 类型 | 规范 | 示例 |
|------|------|------|
| 头文件 | `PascalCase.hpp` | `ChunkManager.hpp` |
| 源文件 | `PascalCase.cpp` | `ChunkManager.cpp` |
| CMake文件 | `PascalCase.cmake` | `CompilerWarnings.cmake` |
| 测试文件 | `test_*.cpp` | `test_chunk.cpp` |
| 配置脚本 | `kebab-case.sh` | `build-project.sh` |

### 3.2 类型命名

```cpp
// ✅ 类/结构体：PascalCase
class ChunkManager;
struct Vector3;
enum class BlockType;

// ✅ 模板参数：PascalCase
template<typename T, typename Allocator>
class Buffer;

// ✅ 类型别名：PascalCase
using ChunkPos = Vector3;
using EntityList = std::vector<Entity*>;

// ✅ 必须：接口类以I开头
class IEntityManager;
```

### 3.3 变量命名

```cpp
// ✅ 成员变量：m_前缀 + camelCase
class Player {
private:
    float m_health;
    std::string m_name;
    Vector3 m_position;
};

// ✅ 局部变量：camelCase
void update() {
    float health = 100.0f;
    std::string playerName = "Steve";
}

// ✅ 全局变量：g_前缀 + camelCase
inline constexpr int g_maxPlayers = 100;

// ✅ 静态成员变量：s_前缀 + camelCase
class Entity {
private:
    static uint64_t s_nextId;
};

// ✅ 常量：UPPER_SNAKE_CASE
inline constexpr int MAX_CHUNKS = 1024;
inline constexpr float GRAVITY = 9.81f;

// ✅ 枚举值：PascalCase（scoped enum）
enum class BlockType {
    Air,
    Stone,
    Dirt,
    Grass
};
```

### 3.4 函数命名

私有方法用_前缀以示区分。

### 3.5 命名空间规范

需要使用命名空间隔离各个子系统的标识符。下面是最佳实践：

```cpp
namespace mc {
namespace entity {
namespace attribute {

/**
 * @brief 属性修改器操作类型
 *
 * 定义属性修改器如何影响基础值
 */
enum class Operation : u8 {
    // ...
}}}}

// ❌ 禁止：不允许使用 using namespace std！
using namespace std;  // ❌
```

【重要】对于只在单个 .cpp 文件中使用的类、函数、全局变量等，应放入匿名 namespace，避免污染，**并在其命名前加上下划线prefix**，而不是使用static或全局命名空间。

---

## 4. 日志使用规范

1. 不允许使用spdlog::debug和spdlog::trace的日志级别，它们会降低性能。只允许用info以及以上级别的日志。
2. 对于任何偏移正常路径、逻辑等异常情况，必须使用warn以及以上级别的日志记录，不允许使用info级别来掩盖他们。（包括但不限于：fallback、未实现、弃用等）
3. 所有日志内容和错误消息必须使用英文，不允许用中文

## 5. 注释与文档规范

### 5.1 Doxygen文档注释

只在头文件中使用Doxygen风格的文档注释即可。

```cpp
/**
 * @brief 加载区块
 * 
 * 从磁盘或缓存中加载指定位置的区块。如果区块不存在，
 * 则使用世界生成器生成新区块。
 * 
 * @param pos 区块位置
 * @param priority 加载优先级（0-10，10最高）
 * @return Result<std::shared_ptr<Chunk>> 加载结果
 * 
 * @note 此方法是异步的，返回后区块可能尚未完全加载
 * @warning 不要在主线程中调用高优先级加载
 * 
 */
Result<std::shared_ptr<Chunk>> loadChunk(ChunkPos pos, int priority);
```

### 5.2 行内注释

推荐使用较多的简体中文行内注释解释逻辑、算法步骤、设计决策等。

```cpp
// ✅ 推荐：解释"为什么"而不是"是什么"
// 使用二次插值平滑相机移动，避免突变
float alpha = smoothstep(0.0f, 1.0f, deltaTime * 5.0f);

// ✅ 【重要】必须：标记待办事项。所有没做完的工作、妥协、简化实现等都必须明确标记，避免遗忘：
// TODO: 优化区块加载算法，当前O(n²)复杂度
// FIXME: 内存泄漏问题，需要在析构函数中释放
// TODO: 临时解决方案，等待其他模块完成后重构
// NOTE: 此处性能关键，不要随意修改
```

【重要】暂时的简化实现、不完整实现、因为未实现等开发进度原因而导致暂时未使用的代码、函数和变量等，必须加上TODO注释（注释中要有明文`TODO`，便于全文搜索），如果你顺手发现了某段代码中的逻辑不完整、缺少TODO注释则需要补上TODO注释，以便未来的开发者知道哪里需要完善实现。

### 5.3 代码区域标记

```cpp
// ============================================================================
// 区块加载系统
// ============================================================================
```

### 5.4 注释必须全部使用简体中文

### 5.5 禁止出现“对齐MC”之类的注释

下面这些“对齐MC”之类的注释不能出现；如果你发现代码中有这些注释，顺手将它们删除：

```cpp
    // 参考 MC 1.16.5 Minecraft.runTick() 中闪电闪烁的处理 ❌这行注释要删了
    m_renderer->setLightningFlashBrightness(m_world.weather().lightningFlashBrightness());

    // 对齐 MC 1.21.11 FirstPersonRenderer::renderArmFirstPerson。 ❌这行注释要删了
    const f32 sqrtSwing = std::sqrt(swingProgress);

    // 参考: net.minecraft.block.LiquidBlock#reactWithNeighbors   ❌这行注释要删了

/**
 * @brief 基于方块坐标的位置目标
 *
 * 对齐 MC 1.16.5 BlockPosWrapper，始终使用方块中心点作为导航/注视位置。 ❌这行注释要删了
 */
class BlockPosTarget final : public IPositionTarget {}

    // 水生生物 - MC 1.16.5 权重 ❌这行注释要删了
    info.setMaxWaterCreatureInstances(DEFAULT_MAX_WATER_CREATURES);
    info.addWaterCreatureSpawn(SpawnEntry("minecraft:cod", 15, 3, 6)); // MC 1.16.5: 15 ❌这行注释要删了
    info.addWaterCreatureSpawn(SpawnEntry("minecraft:salmon", 15, 1, 5)); // MC 1.16.5: 15 ❌这行注释要删了
    info.addWaterCreatureSpawn(SpawnEntry("minecraft:squid", 3, 1, 4)); // MC 1.16.5: 3 ❌这行注释要删了

        // 触发通用的 EffectsChangedTrigger
        // 参考 MC 1.16.5: CriteriaTriggers.EFFECTS_CHANGED.trigger(ServerPlayer) ❌这行注释要删了
```

---

## 6. 内存管理规范

### 6.1 智能指针使用

```cpp
// ✅ 推荐：优先使用std::unique_ptr
std::unique_ptr<Chunk> chunk = std::make_unique<Chunk>();

// ✅ 推荐：共享所有权使用std::shared_ptr
std::shared_ptr<Texture> texture = std::make_shared<Texture>();

// ❌ 严禁：裸new/delete
Type* ptr = new Type();  // ❌ 尽可能不要使用裸指针
delete ptr;              // ❌
```

### 6.2 容器使用

```cpp
// ✅ 推荐：预分配容量
blocks.reserve(16 * 256 * 16);  // 区块大小
// ✅ 推荐：使用emplace_back代替push_back
// ✅ 推荐：使用string_view避免拷贝
void processString(std::string_view str);
```

---

## 7. 错误处理规范

### 7.1 Result类型使用

```cpp
// ✅ 推荐：使用Result类型处理错误
Result<void> initialize() {
    auto result = createWindow();
    if (!result.success()) {
        return result;  // 传播错误
    }
    
    result = createRenderer();
    if (!result.success()) {
        return result;
    }
    
    return Result<void>::ok();
}

Result<void> initialize() {
    TRY(createWindow());
    TRY(createRenderer());
    return Result<void>::ok();
}

// ✅ 推荐：携带错误信息
class Error {
public:
    enum class Code {
        Success = 0,
        NotFound,
        InvalidArgument,
        OutOfMemory,
        // ...
    };
    
    Code code() const;
    const std::string& message() const;
    const std::string& source() const;  // 错误位置
};

// 使用
Result<Chunk*> loadChunk(ChunkPos pos) {
    if (!chunkExists(pos)) {
        return Error{
            Error::Code::NotFound,
            fmt::format("Chunk at {} not found", pos),
            "ChunkManager::loadChunk"
        };
    }
    // ...
}
```

---

## 8. include路径规范

```cpp
// ✅ 推荐：使用./或不使用./
#include "Vector3.hpp"// ✅
#include "./Vector3.hpp"// ✅
#include "./math/Vector3.hpp"// ✅

// ❌ 禁止：使用`../`跳转到上级目录，这会导致路径混乱，增加维护难度
#include "../../common/util/math/Vector3.hpp"// ❌
// 这种情况下建议换成绝对路径：
#include "common/util/math/Vector3.hpp"// ✅
```

---

## 9. 性能相关规范

### 9.2 避免的性能陷阱

```cpp
// ❌ 禁止：不必要的拷贝
std::string getName() { return m_name; }  // ❌

// ✅ 推荐：返回const引用
const std::string& getName() const { return m_name; }  // ✅

// ❌ 禁止：在循环中重复计算
for (int i = 0; i < vec.size(); i++) {  // size()每次调用
    // ...
}

// ✅ 推荐：缓存循环条件
for (int i = 0, n = vec.size(); i < n; i++) {  // ✅
    // ...
}

```

---

## 10. 安全规范

### 10.1 输入验证

```cpp
// ✅ 推荐：验证所有外部输入
Result<void> loadFile(const std::string& path) {
    // 验证路径
    if (path.empty()) {
        return Error::invalidArgument("Path cannot be empty");
    }
    
    // 防止路径遍历
    if (path.find("..") != std::string::npos) {
        return Error::invalidArgument("Invalid path");
    }
    
    // 验证文件扩展名
    if (!path.ends_with(".json")) {
        return Error::invalidArgument("Invalid file type");
    }
    
    // ...
}

// ✅ 推荐：限制输入大小
void processMessage(const std::string& message) {
    constexpr size_t MAX_MESSAGE_LENGTH = 256;
    if (message.length() > MAX_MESSAGE_LENGTH) {
        return Error::invalidArgument("Message too long");
    }
    // ...
}
```

# Cubium Agent 规范

## 用语规范

在和用户对话、向用户汇报、向用户询问等场景下，必须使用严肃且清晰的语言风格，避免口语化。和用户对话的时候一律使用简体中文回应用户。

向用户询问的时候，必须向用户补充大量背景信息，以便对相关子系统没那么了解的用户也能快速了解到问题背景和本质。

## 构建

【重要】由于本项目代码量已达百万级，构建时间很长，任务的超时等待时间必须30分钟以上！你在等待构建完成的过程中，不允许做其他任何事情。必须保证`run_in_background=false`
等待子代理（agent）完成的过程中，不允许做其他任何事情。必须保证`run_in_background=false`
有时候，任务（构建、agent、子代理等）可能会被自动后台化，这时请必须使用 TaskOutput 工具阻塞等待，不要和子代理同时做事情。

在windows上，只允许执行下面唯一构建命令：
```bash
./scripts/configure.sh build
```

在macos上：
```bash
cmake --build --preset macos-relwithdebinfo -- -j{当前机器核心数减去2}
# 当前机器核心数要通过命令获取
```

如果仍然对于构建过程存在疑难问题，可参考`docs/BUILD.md`

### 判断构建是否真的结束（不要被重定向日志骗了）

**不要把构建日志重定向到文件后再 `tail` 它来判断进度。** 重定向的 stdout 在进程退出前不会落到文件（缓冲），而且 ninja 打印 `Linking CXX executable ...` 这一行是在**链接开始时**，链接结束后不再有任何输出。两个因素叠加，看起来就像「卡在 Linking 且 CPU 为 0」。

正确的判据（任选其一，必要时组合）：

1. **后台任务输出里的完成标记**：`cmake --build ... > /tmp/x.log 2>&1; echo exit=$?` 中 `echo exit=$?` 的输出会进到后台任务自己的输出文件，看到 `exit=0` 才是真的结束了；
2. **产物 mtime**：`stat -c '%y' build/bin/RelWithDebInfo/mc_tests.exe`，若 mtime 晚于本次构建启动时刻，说明链接已完成；
3. **幂等验证**：再跑一次同样的 `cmake --build`，若输出 `ninja: no work to do.` 就说明上一次已经完整结束、依赖图已闭合。

排查「是否真的卡死」的标准动作：`tasklist` 查 `ninja.exe` / `clang++.exe` / `lld-link.exe` / 目标 exe 是否还在；都不在 = 没有进程在等，就是已经结束。

## 运行

```bash
./build/bin/RelWithDebInfo/minecraft-client --quick-play-new # 客户端（会同时启动内置服务端）
./build/bin/RelWithDebInfo/minecraft-server --profiler_enabled=false # 服务端（命令行参数由 gflags 解析，可用 --gametest 无头跑测试、--config 指定配置） 建议绝大多数场景下明确传入--profiler_enabled=false，除非是在CPU性能分析场景下才需要启用这个flag
./build/bin/RelWithDebInfo/minecraft-server.exe --gametest # 服务端无头跑集成测试
./build/bin/RelWithDebInfo/mc_tests.exe --gtest_filter="*GameTestServer*" # GameTestServer自动化集成测试
./build/bin/RelWithDebInfo/mc_tests.exe --gtest_filter="你的测试过滤器" # 其他单元测试

```

运行过程中的日志也会一并输出到控制台，如果你要抓日志来调试，请编译后直接执行上述命令即可，尽量不要让用户手动去执行。

日志一般非常大，请尽量不要读取全文，只抓取你需要的部分。

## 测试

当遇到测试错误的时候，未经过我的允许，不得擅自将代码回退到基线版本并重编译！因为这会非常耗时间！

如果你需要运行测试，请尽量不要运行整个mc-tests和整个集成测试，因为它包含巨量测试用例，会严重超时。你只需要运行你的修改涉及到的少量测试就行了，因为每晚会有个夜间的ci，它会利用晚上跑全量测试，如果这个全量测试有问题，我会收集起来，定期找你反馈。

你可能会碰到很多测试错误，它们要么是以前留下的存量问题，要么是测试并行执行的时候相互之间出现了状态污染，总之它们大概率不是你本次任务的问题，不要以此为理由将代码回退到基线版本并重编译！这很耗时！

【重要】对于**明显与本次改动无关**的测试失败（例如与本任务子系统无关的自然生成、光照、刷怪蛋等用例），不要调查、不要隔离复跑、不要对比基线、不要因此阻塞任务，直接忽略并在汇报中一句话说明即可。

【重要】**严禁擅自执行 `git worktree add`（或任何等价手段，如复制仓库到临时目录）去建立额外工作树并编译基线做对比**。不得以"确认是否预存问题"为由新增任何构建。需要基线信息时，只能通过阅读 git 历史、阅读 main 上的提交 diff、逻辑推断获得。

你新增单元测试的时候，务必记得把新增的测试文件编入Cmake列表，否则这个文件无法被编译和运行！

## 代码格式化

提交代码之前，必须使用clang-format对你修改的文件进行格式化：

```
运行 scripts/format/clang_format_all.ts # 这个脚本会自动定位clang-format并自动格式化文件 必须node24版本（可以在同一行命令中先nvm切换到24）
```

格式化完成后，不要重新编译一遍，以免浪费时间；如果出现了超出你预期文件范围的改动，属于正常现象，一起提交即可。

## clang-tidy 抑制注释（NOLINT）写法

【重要】`// NOLINTNEXTLINE` 只对**紧邻的下一行**生效。若中间夹了任何其他注释行（哪怕是解释该抑制
原因的说明），抑制就会落到那条注释上，被抑制的诊断照常报出，且**没有任何提示**——表现为"写了 NOLINT
却依然报错"。

因此必须遵守：

1. **抑制行与它要抑制的代码行之间，不得有任何其他行**。需要解释原因时，把说明写在抑制行**之前**，
   并用一条空注释行 `//` 与抑制行隔开。
2. **抑制要精确对准报出诊断的那一行**。`performance-unnecessary-value-param` 的告警位置是**参数本身**，
   所以抑制必须写在**那个参数所在的参数行**的正上方——写在函数名行上方无效（clang-format 会把参数拆成
   多行，函数名行与参数行不再是同一行）。
3. **行尾 NOLINT 不可靠**：写在行尾的 `// NOLINT(...)` 经 clang-format 重排参数后会留在函数名行，
   而诊断在参数行，抑制失效；同理行尾 `NOLINTNEXTLINE` 抑制的是下一行而非本行。**统一使用参数行上方的
   独立 `// NOLINTNEXTLINE(检查名)` 行**。

正确写法（抑制写在目标参数行正上方，且中间无其他行）：

```cpp
// heldItem 必须按值传入：ItemUseContext 构造函数对该 stack 做 const_cast 后持有可写指针，
// onItemUse 内 getItemStackMut() 会直接改它；若指向权威槽，物品侧的自改会绕过下方
// itemId/damage 对比，导致外层再 shrink 一次（双重消耗）。
Result<BlockUseResult> BlockInteractionManager::handleItemUseOn(
    PlayerId playerId,
    const BlockPos& pos,
    const Vector3& hitPos,
    Direction face,
    Hand hand,
    // NOLINTNEXTLINE(performance-unnecessary-value-param)
    ItemStack heldItem)
```

错误写法（抑制与签名之间夹了注释行 → 抑制落到注释行上，静默失效）：

```cpp
// NOLINTNEXTLINE(performance-unnecessary-value-param) — heldItem 必须按值传入：
// ItemUseContext 构造函数对该 stack 做 const_cast 后持有可写指针……
Result<BlockUseResult> BlockInteractionManager::handleItemUseOn(
    PlayerId playerId, const BlockPos& pos, ...)
```

改完必须用 `./scripts/tidy/tidy.sh --checks='-*,<检查名>' <文件>` 实测确认抑制生效（输出中不应再出现该诊断）。

## 子代理使用规范

【重要】最新规定，目前一律不准启动任何子代理。

【重要】不允许让子代理执行编译命令，因为多个子代理执行编译命令会导致构建系统出现大量严重问题甚至锁死，必须由你来执行编译命令！你必须在子代理的提示词当中显式说明这个问题。

## 不允许启动workflow

## 重要的外部路径（用于参考和辅助错误排查）

### Windows

当前资源包路径：C:\Users\Administrator\minecraft_reborn\resourcepacks\Vanilla
当前数据包路径：C:\Users\Administrator\minecraft_reborn\datapacks\Vanilla
可供参考的MC Java版源码路径：D:\Minecraft\MC研究\Minecraft1.21.11源码
可供参考的基岩版源码路径：E:\dev\MC\LeviLamina （注意，全是逆向出来的hpp文件，没有cpp文件）
（如果项目中代码是参考1.16.5的，必须迁移到1.21.11）
Moonrise优化模组路径：E:\dev\MC\Mods\Moonrise
ConcurrentUtil源码路径：D:\MiscProjects\ConcurrentUtil
horizon harness：e:/dev/mc/thirdpartymc/horizon 在被要求编写harness对齐原版mc的时候必须参考这个项目的做法

### WSL

和windows一致，直接换成 /mnt/c /mnt/d /mnt/e 等路径即可。

### MacOS
　
当前资源包路径：~\minecraft_reborn\resourcepacks\Vanilla
当前数据包路径：~\minecraft_reborn\datapacks\Vanilla
可供参考的MC源码路径：~/dev/MC/java/Minecraft1.21.11/
（如果项目中代码是参考1.16.5的，必须迁移到1.21.11）
Moonrise优化模组路径：~\dev\MC\Mods\Moonrise

## 其他

【重要】你时间充足，上下文也充足，必须始终选择最彻底、最正确、最准确的做法。

【重要】暂时的简化实现、不完整实现、因为未实现等开发进度原因而导致暂时未使用的代码、函数和变量等，必须加上TODO注释（注释中要有明文`TODO`，便于全文搜索），如果你顺手发现了某段代码中的逻辑不完整、缺少TODO注释则需要补上TODO注释，以便未来的开发者知道哪里需要完善实现。
【重要】暂时的简化实现、不完整实现、因为未实现等开发进度原因而导致暂时未使用的代码、函数和变量等，必须加上TODO注释（注释中要有明文`TODO`，便于全文搜索），如果你顺手发现了某段代码中的逻辑不完整、缺少TODO注释则需要补上TODO注释，以便未来的开发者知道哪里需要完善实现。
【重要】暂时的简化实现、不完整实现、因为未实现等开发进度原因而导致暂时未使用的代码、函数和变量等，必须加上TODO注释（注释中要有明文`TODO`，便于全文搜索），如果你顺手发现了某段代码中的逻辑不完整、缺少TODO注释则需要补上TODO注释，以便未来的开发者知道哪里需要完善实现。

可以先不考虑本项目的客户端，因为本项目的客户端已经停止维护。目前仅支持用java版客户端连接本项目服务端。

不要考虑兼容性，不要兼容旧存档和旧协议！

【gametest参考】

**基岩版**

- 逆向头文件 E:\dev\MC\LeviLamina\src\mc\scripting\modules\gametest

- 逆向头文件 E:\dev\MC\LeviLamina\src\mc\gametest

- 官方文档 E:\dev\MC\Mods\minecraft-creator\creator\ScriptAPI\minecraft\server-gametest

- 示例 E:\dev\MC\minecraft-gametests（介绍：This repo contains sample GameTest behavior files for Minecraft Bedrock Edition. Minecraft supports GameTests - a combination of JavaScript + MCStructures - for validating facets of Minecraft behavior. You can use GameTests to validate facets of your creations, as well!）

**java版**

D:\Minecraft\MC研究\Minecraft1.21.11源码\net\minecraft\gametest
