## 项目概述

Cubium 是一个现代化的 Minecraft 克隆项目，采用客户端-服务器架构，使用 C++20 编写，使用 Vulkan 进行渲染。该项目旨在尽可能复制 Java 版 1.21.11 的体验，同时保持与现有 Minecraft 生态系统的兼容性（资源包、世界存档、数据包）。

## 依赖规范

- src/common不能依赖src/client和src/server
- 反之，src/client和src/server可以依赖src/common
- 为了保证common目录的简洁，所有client和server的专属逻辑都必须放在src/client和src/server目录下，不能放在common目录下

## git 规范

### 提交信息格式
```
<type>(<scope>): <subject>
<body>
```

### 合并方式
不允许使用线性历史（rebase）

## 关键类型

所有类型都在命名空间 `mc` 中（客户端类型在 `mc::client`，服务端类型在 `mc::server`）：

### 基本类型
- `i8`, `i16`, `i32`, `i64` - 有符号整数
- `u8`, `u16`, `u32`, `u64` - 无符号整数
- `f32`, `f64` - 浮点数

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
- `ChunkGenerateTask`：区块生成任务，提交到 UniversalWorkerPool 执行
- `IChunk`：生成的区块接口

### 生物群系类型
- `BiomeId` - 生物群系标识符（含主世界/下界/末界，对齐 MC 1.21.11）
- `Biome` - 生物群系定义，包含气候、特性、雕刻器
- `BiomeContainer` - 4x4x4 采样生物群系存储
- `BiomeProvider` - 生物群系分布基类
- `MultiNoiseBiomeSource` - 多噪声生物群系生成（对齐 MC 1.18+，取代旧版 LayerBiomeProvider）

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

**业务枚举**（已从旧 `packet/` 目录迁出，位于 `network/protocol/`）：
- `EntityAnimation` / `EntityStatus` → `protocol/EntityEvents.hpp`
- `BlockInteractionAction` / `BossInfoAction` / `PlayerAbilityFlags` → `protocol/GameActions.hpp`
- `TitleAction` → `protocol/TitleActions.hpp`

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
├── ChunkData.hpp/cpp              # 区块数据存储（ChunkSection、ChunkData、ChunkDataRef）
├── ChunkId.hpp                    # 区块唯一标识符（包含维度）
├── ChunkPos.hpp                   # 区块位置类型
├── ChunkStatus.hpp/cpp            # 区块生成阶段定义
├── IChunk.hpp/cpp                 # 区块接口和基础类型
├── SectionPos.hpp                 # 区块段位置类型

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
- `CHUNK_WIDTH`, `CHUNK_HEIGHT`, `CHUNK_SECTION_HEIGHT`, `CHUNK_SECTIONS`, `CHUNK_VOLUME` - 区块尺寸。【重要】只能使用这些mc::world下的常量作为区块尺寸，不能硬编码16等数字，因为未来可能会频繁修改区块尺寸。另外，有些地方可能使用位运算来计算区块坐标（例如 `x >> CHUNK_SHIFT`），务必使用这些常量来确保位运算的正确性，而不是简单 >> 4。
【重要】CHUNK_HEIGHT和MAX_BUILD_HEIGHT值不同，语义也完全不同。CHUNK_HEIGHT = MAX_BUILD_HEIGHT - MIN_BUILD_HEIGHT = 320 - (-64) = 384，而MAX_BUILD_HEIGHT = 320。使用的时候务必小心这个区别！
这些常量定义在 `src/common/world/WorldConstants.hpp` 中（`Constants.hpp` 通过 include 重新导出以保持兼容）。
- `CHUNK_SHIFT`, `SECTION_SHIFT`, `CHUNK_MASK` - 区块位运算常量
- `CHUNK_LOAD_RADIUS`, `CHUNK_UNLOAD_RADIUS`, `MAX_CHUNKS_LOADED` - 区块加载
- `WORLD_SEED_DEFAULT`, `SPAWN_CHUNK_RADIUS` - 世界生成
- `BLOCK_UPDATE_RADIUS` - 方块更新范围

另外，`src\common\world\WorldConstants.hpp` 这个文件也提供了巨量的世界相关常量和工具方法，必须尽可能复用而不是自己硬编码：

```cpp
enum class ChunkLoadPriority : i32 {
    Critical = 0,  // 玩家所在区块
    High = 1,      // 玩家周围区块
    Normal = 2,    // 正常加载
    Low = 3,       // 远处区块
    Background = 4 // 后台生成
};
constexpr u32 CHUNK_UNLOAD_DELAY_MS = 30000; // 30秒

// 区块保存间隔 (毫秒)
constexpr u32 CHUNK_SAVE_INTERVAL_MS = 60000; // 1分钟

constexpr f32 TERRAIN_HEIGHT_VARIATION = 16.0f;
constexpr f32 TERRAIN_BASE_HEIGHT = 64.0f;
constexpr f32 CAVE_FREQUENCY = 0.02f;
constexpr f32 ORE_FREQUENCY = 0.01f;

constexpr i32 LIGHT_UPDATE_DISTANCE = 15;

constexpr i32 BLOCK_UPDATE_DISTANCE = 64;

// 红石更新延迟 (ticks)
constexpr i32 REDSTONE_DELAY = 2;

constexpr i32 ENTITY_ACTIVATION_RANGE_PLAYER = 128;
constexpr i32 ENTITY_ACTIVATION_RANGE_MONSTER = 32;
constexpr i32 ENTITY_ACTIVATION_RANGE_ANIMAL = 32;
constexpr i32 ENTITY_ACTIVATION_RANGE_MISC = 16;

// 实体追踪范围
constexpr i32 ENTITY_TRACKING_RANGE_PLAYER = 64;
constexpr i32 ENTITY_TRACKING_RANGE_MONSTER = 64;
constexpr i32 ENTITY_TRACKING_RANGE_ANIMAL = 48;
constexpr i32 ENTITY_TRACKING_RANGE_MISC = 32;

// 实体消失范围
constexpr i32 ENTITY_DESPAWN_RANGE = 128;

// 检查Y坐标是否在有效范围内
inline bool isValidY(i32 y)
{
    return y >= MIN_BUILD_HEIGHT && y < MAX_BUILD_HEIGHT;
}

// 将世界坐标转换为区块坐标
inline i32 toChunkCoord(i32 worldCoord)
{
    return worldCoord >= 0 ? worldCoord / CHUNK_WIDTH : (worldCoord + 1) / CHUNK_WIDTH - 1;
}

// 将世界坐标转换为区块内本地坐标
inline i32 toLocalCoord(i32 worldCoord)
{
    i32 local = worldCoord % CHUNK_WIDTH;
    return local >= 0 ? local : local + CHUNK_WIDTH;
}

// 将区块坐标转换为世界坐标
inline i32 toWorldCoord(i32 chunkCoord)
{
    return chunkCoord * CHUNK_WIDTH;
}

// 将Y坐标转换为区块段索引
inline i32 toSectionIndex(i32 y)
{
    return (y - MIN_BUILD_HEIGHT) / CHUNK_SECTION_HEIGHT;
}

// 将区块段索引转换为Y坐标
inline i32 sectionToY(i32 sectionIndex)
{
    return MIN_BUILD_HEIGHT + sectionIndex * CHUNK_SECTION_HEIGHT;
}

// 检查区块坐标是否有效
inline bool isValidChunkCoord(i32 chunkX, i32 chunkZ)
{
    constexpr i32 WORLD_BORDER = 30000000;
    constexpr i32 MIN_CHUNK = -WORLD_BORDER / CHUNK_WIDTH;
    constexpr i32 MAX_CHUNK = WORLD_BORDER / CHUNK_WIDTH;
    return chunkX >= MIN_CHUNK && chunkX <= MAX_CHUNK && chunkZ >= MIN_CHUNK && chunkZ <= MAX_CHUNK;
}

```

`mc::entity` 命名空间：
- `MAX_ENTITIES_PER_CHUNK`, `MAX_PLAYERS`, `MAX_ENTITIES` - 实体数量限制
- `LegacyEntityTypeId` - 实体类型ID枚举（旧版，用于网络同步）
- `EntityStatus` - 实体状态枚举
- `ENTITY_TRACKING_RANGE`, `PLAYER_TRACKING_RANGE` - 追踪距离

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
- **Tracy**（`MC_ENABLE_TRACY`，默认 ON）：in-memory 采集，client 自动监听 8086 端口，用 tracy GUI 连接拉取查看（进程内不写文件）。
- 两者同时启用时，`MC_TRACE_*` 宏会同时向两套后端发事件；两者皆关时所有宏空展开、零开销。

### 追踪类别

追踪类别参见 `src\common\profiler\TraceCategories.hpp` 中的 `mc::trace::TraceEvents` 枚举树。你只能使用这棵树上的叶子节点（如 `TraceEvents.Server.Tick`），其字符串值已在该文件的 `PERFETTO_DEFINE_CATEGORIES` 中注册（仅 Perfetto 后端需要注册，Tracy 侧不消费 category）。使用树外的类别会导致编译错误（仅 Perfetto 启用时）。

### 用法

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

在 `.cpp` 中通常在 include 区后加 `using namespace mc::trace;`，即可直接写 `TraceEvents.Server.Tick`；在 `.hpp` 中用全限定 `::mc::trace::TraceEvents.X.Y`，不要在头文件加 `using namespace`。

可用宏：`MC_TRACE_SCOPED_EVENT`（作用域事件，最常用）、`MC_TRACE_INSTANT_EVENT`（瞬时事件）、`MC_TRACE_COUNTER`（计数器）、`MC_TRACE_EVENT_BEGIN`/`MC_TRACE_EVENT_END`（手动跨函数配对）、`MC_TRACE_SET_THREAD_NAME`（线程命名）。

双轨说明：`MC_TRACE_SCOPED_EVENT` 双轨启用时同时发 Perfetto `TRACE_EVENT` 与 Tracy `ZoneScopedN`（变量名按 `__LINE__` 唯一化，允许同一作用域多个）；`MC_TRACE_COUNTER` 同时发 `TRACE_COUNTER` 与 `TracyPlot`（转 double，>2^53 大值会丢精度）；`MC_TRACE_INSTANT_EVENT`/`BEGIN`/`END` 在 Tracy 侧降级为 message 边界标记。`MC_TRACE_SET_THREAD_NAME` 转调 `ProfilerManager::setThreadName`，由门面同时写两套后端。

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

### MC Java 源码参考
你可以在 `D:\Minecraft\MC研究\Minecraft1.21.11源码\net\minecraft` 访问 MC Java 1.21.11 源码作为参考。本项目旨在尽可能复制 Java 版游戏玩法。

### 代码质量
- 必须有断言和单元测试
- 每个方法都要有文档注释
- 测试覆盖率必须达到 95%+
- "测试即契约"原则

### 目录结构
保持整洁优雅的目录结构，适当使用子目录。永远不要在一个目录里堆放大量文件。

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

### 1. 高度图 +1 语义：`ctx.getHeight` vs `getTopBlockY`

MC 1.21.11 生成期间，`WorldGenContext.getHeight(...)` 走 `WorldGenRegion.getHeight`，其实现为 `chunk.getHeight(...) + 1`（`WorldGenRegion.java:404-407`），即返回"最高方块上方一格空气的 Y"（blockY+1），**不是方块本身 Y**。而 `ChunkAccess.getHeight` 返回 blockY（`getFirstAvailable-1`）。

项目的 `WorldGenRegion::getTopBlockY` 转发 `ChunkPrimer::getTopBlockY`，返回 blockY（等价 MC `ChunkAccess.getHeight`），**少了这个 +1**。因此：**凡是对应 MC `ctx.getHeight(...)` 调用的 placement/feature 代码，必须对 `getTopBlockY` 结果 +1**。

漏掉 +1 的典型后果：`HeightmapPlacement` 返回草方块本身 Y → `TreeFeature` 的 `startPos` 落在草方块上 → `_calculateAvailableHeight` 在 `y=0` 检查草方块（不可替换）立即返回 -2 → 所有依赖 heightmap 的 feature（含全部树木）判定"空间不足"而生成失败，且**无任何报错**（feature 失败本就是常态，静默返回 false）。目前已 +1 的有 `HeightmapPlacement`、`SurfaceRelativeThresholdFilterPlacement`、`CountOnEveryLayerPlacement`；新增同类代码务必核对 MC 源码确认调用方是否走 `WorldGenRegion.getHeight`。

### 2. random_selector / simple_random_selector 子特征引用是 PlacedFeature id

MC 1.21.11 中 `RandomFeatureConfiguration.features[]` / `default` 是 `WeightedPlacedFeature` / `Holder<PlacedFeature>`，即 **PlacedFeature 引用**，命中后调用其 `place(origin)`（先走该 PlacedFeature 自带的 placement 链，再放置其配置化特征）。**不是 ConfiguredFeature 引用**。

项目的数据包存在两种写法（均合法）：(1) 字符串 id 指向已注册 placed_feature（如 `"minecraft:spruce_checked"`、`"minecraft:oak_checked"`）；(2) 内联对象 `{"feature":"<configured_id>","placement":[]}`，加载器提取出 configured id（如 `"minecraft:oak_bees_005"`，无对应 placed_feature 文件）。

若错误地仅按 `ConfiguredFeatureRegistry` 解析这些 id，则所有以 placed_feature id 作 default 的 trees_*（snowy/taiga/jungle/savanna/water/windswept_hills 等）都会因注册表查不到而 `default-miss` 静默失败，**整个生物群系的树木都不生成，且无报错**。正确做法：解析子特征 id 时**先查 `PlacedFeatureRegistry`，未命中再查 `ConfiguredFeatureRegistry`** 兜底（见 `RandomSelectorFeature.cpp` 的 `dispatchChildFeature`）。新增同类 selector feature 必须沿用此双注册表解析。

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

### 对于重构类任务的准则

1.不做任何api兼容，直接一步到位新代码（因此主调者需要修改，这符合预期）
2.我有强烈代码洁癖，不允许留任何旧代码旧文件。不允许通过注释等任何手段保留旧代码，不允许以兼容为理由保留旧代码，不允许未经允许的情况下乱加adapter或兼容层来保留旧代码。重构类任务的目标就是把旧代码完全替换掉，留下干净的新实现。
3. 我能容忍激进重构后导致的编译错误和运行时错误，请你放行大胆进行重构。

### 对于所有任务的准则

当你完成一个任务后，不要停下来，请继续做后面的任务，直到任务清空你才能停！你时间充足、上下文也充足。

注意本项目基建已经相当完善（代码量已经百万级别），各种常数、常用工具函数、音频系统、粒子系统、资源包系统、命令系统、实体系统、物品系统、物理系统、碰撞、tick调度、存档系统、成就系统等都已经有了相当完善的实现(另外world对象上面也挂了相当多的工具方法以便访问世界、操作世界等)，务必充分探索以实现复用，避免重复实现。

---

当你发现了存量代码中有任何地方违反了上述任意规范，请你顺手直接修改它们，你有权限直接修改任何违反规范的代码。
