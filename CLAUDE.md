# CLAUDE.md

本文件为 Claude Code (claude.ai/code) 在本仓库中工作时提供指导。

## 项目概述

Minecraft Reborn 是一个现代化的 Minecraft 克隆项目，采用客户端-服务器架构，使用 C++17 编写，使用 Vulkan 进行渲染。该项目旨在尽可能复制 Java 版 1.16.5 的体验，同时保持与现有 Minecraft 生态系统的兼容性（资源包、世界存档、数据包）。

## 关键类型

所有类型都在命名空间 `mc` 中（客户端类型在 `mc::client`，服务端类型在 `mc::server`）：

### 基本类型
- `i8`, `i16`, `i32`, `i64` - 有符号整数
- `u8`, `u16`, `u32`, `u64` - 无符号整数
- `f32`, `f64` - 浮点数（性能优先使用 f32）
- `String`, `StringView` - 字符串类型

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
- `ChunkTask`：工作池的生成任务
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
- `GRAVITY` - 重力加速度 (blocks/tick²)
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
- `CHUNK_WIDTH`, `CHUNK_HEIGHT`, `CHUNK_SECTION_HEIGHT`, `CHUNK_SECTIONS`, `CHUNK_VOLUME` - 区块尺寸
- `CHUNK_SHIFT`, `SECTION_SHIFT`, `CHUNK_MASK` - 区块位运算常量
- `MIN_BUILD_HEIGHT`, `MAX_BUILD_HEIGHT`, `SEA_LEVEL` - 高度限制
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

### NBT 系统 (`src/common/util/nbt/`)

**核心类型**
- `TagId` - NBT 标签类型枚举（End, Byte, Short, Int, Long, Float, Double, ByteArray, String, List, Compound, IntArray, LongArray）
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

## 易错点和陷阱

- `WorldGenRegion` 不是 `IBlockReader`；不要直接将其传递给 `Block::isValidPosition`。
    - 在世界生成特性中，在 `WorldGenRegion` 上下文中运行时应使用显式的本地放置检查（`isWater`、支撑方块检查）。
- `WorldGenRegion` 现在使用阶段特定的 `ChunkStatus::taskRange()` 窗口，如果请求的区块缺失，`getTopBlockY()` 会触发断言。
    - 不要将窗口外的高度查询视为"高度 0"；应修复区域半径或调用点。
- `IWorld` 现在暴露了 `BlockPos` 重载用于方块位置语义。
    - 只要调用者已有 `BlockPos` 就应优先使用，并通过 `using IWorld::...` 重导出重载集来保持 `ServerWorld` 风格的 xyz 实现。
- `ISpawnWorldReader`、`ClientWorld` 和光照/生成辅助类不属于该 `IWorld` 契约。
    - 保持这些接口使用原生 xyz 签名；不要仅仅为了镜像 `IWorld` 而在非 `IWorld` 读取器上强制添加 `BlockPos` 重载。
- 蓝冰放置如果没有冰块在采样起始位置周围，将始终失败。
    - 测试必须在精确的采样邻域设置冰块，而不是以会改变海底检测的方式替换整个水层。
- 避免将临时 `BlockState` 副本传递给世界写入 API。
    - 优先使用 `state.with(...)` / `defaultState()` 返回的规范引用；`ServerWorld::setBlock` 现在通过 `stateId` 规范化作为安全网。
- `ChunkData::setSkyEmptinessMap(const bool* map)` / `setBlockEmptinessMap(const bool* map)` 需要拿到完整的区块段空隙图。
    - 不要再把 `std::vector<bool>` 的结果直接丢成 `nullptr`；如果上游拿到的是按段更新结果，必须先拷贝成连续的 `bool[]` 再写回区块。
- `Heightmap` 内部存储的是 `y + 1`，不是实际方块 Y。
    - 只有 `getTopBlockY()` 这一层才应该把它转换回块坐标，不要直接把原始高度图值当作方块位置。
- `WorldLightManager::tick(...)` 现在依赖有序的预算消耗。
    - 除非重新设计预算模型并匹配测试，否则不要恢复之前的五五开分配。
- `TemptGoal` 现在通过主手/副手物品堆过滤真正的 `Player` 实体，`PanicGoal` / `WaterAvoidingRandomWalkingGoal` 现在直接查询 `IWorld::isWaterAt(...)` / `isLavaAt(...)`。
    - 保持测试与世界查询接口一致；不要仅通过 stub 移动来伪造这些目标。
- `KelpFeatureIds` 和 `SeagrassFeatureIds` 现在按海洋温度分开。
    - 添加新海洋变体时，保持 `FeatureRegistry::initialize()` 顺序、`BiomeGenerationSettings` 映射和海洋断言同步。
- `CraftingMenu::stillValid()` 现在使用玩家到工作台的距离。
    - 保持工作台可访问性绑定到方块实体位置，使容器有效性匹配预期的交互范围。
- `ChickenEntity::tick()` 在计时器到期时生成鸡蛋物品实体。
    - 生成后立即重置计时器，否则鸡会批量发射鸡蛋。
- `ChestContainer` 和 `FurnaceContainer` 现在需要真正的 `PlayerInventory` 并位于 `AbstractContainerMenu` 下。
    - 不要通过遗留的 `Container` 路由创建箱子/熔炉 GUI；使用共享菜单工厂/打开容器钩子。
- `ContainerPacketHandler::handleContainerClick()` 现在依赖存储在集成服务器菜单玩家上的活动菜单指针。
    - 打开时保持 `getMenuPlayer().setOpenContainerMenu(...)`，关闭时 `clearOpenContainerMenu()`，否则客户端点击会在到达菜单前被丢弃。
- `GlassBottleItem` 在决定瓶子是否可以装满之前，沿玩家视线进行采样。
    - 这里的液体方块不提供可用的碰撞形状，所以纯命中测试不足以检测水源。
- `PaneBlock` 连接形状按 4 位掩码缓存并使用规范化坐标。
    - 不要回退到单个中心形状占位符或重新引入像素空间盒子坐标；那会破坏碰撞和渲染测试。
- `ChunkMesher` 中的液面剔除必须将空碰撞的水下植物（如海草和海带）视为隐藏面的邻居。
    - 不要仅根据透明度来确定液体可见性，否则会在水生植被周围重新引入散乱的水面片。
- `MatrixStack` 调用顺序直觉在 PoseStack 对齐后可能会产生误导。
    - 在第一人称渲染中，按原版顺序应用变换并依赖后乘语义；避免临时性的原地行/列编辑。
- Kagero 中的 `StackLayoutAlgorithm` 应该在容器交叉轴上拉伸子元素，而不仅仅是行内容大小。
    - 依赖全宽子元素的堆栈/列测试需要适配器遵守容器宽度，否则内部边界将保持为固有大小。
- 不要在双手之间共享一个第一人称物品网格缓存。
    - 主手和副手可以在同一帧持有不同物品，所以单个缓存会抖动并在每帧渲染时分配 GPU 内存。
- 退役的第一人称网格必须在帧倒计时上回收，而不仅仅在 `destroy()` 中。
    - 否则重复的物品更改会使旧的 Vulkan 缓冲区在整个会话期间保持活动。
- `EntityPipeline::updateMesh(...)` 必须保留 GPU 缓冲区并仅在需要时增长容量。
    - 不要将动画网格更新切换回每帧销毁+创建；保持可重用的暂存缓冲区和原地上传，否则 `vkAllocateMemory` 会回到渲染热路径。
- 不要把优先级逻辑放回 `MeshWorkerPool`。
    - 优先级和取消策略属于 `MeshBuildScheduler`；`MeshWorkerPool` 应保持仅执行。
- `ChunkMesher` 的 `generateSplitMesh()` 预留策略必须按 pass 区分。
    - 透明层的初始容量要明显小于实心层，否则双层网格会把峰值内存翻倍。
- 每帧在调用 `ClientWorld::update(...)` 之前必须更新 `MeshSchedulerViewState`。
    - 如果视图状态过期，视锥体优先级和相机后取消将滞后于相机移动。
- `ClientApplication` 里给 `MeshBuildScheduler` 的并发预算要保持保守。
    - 不要再把 `maxDispatchedTaskCount` 按视距线性放大到很大，完成队列里每个 chunk mesh 都可能是数 MB 级别。
- 在分发前取消的待处理网格任务不会产生工作器结果。
    - 保持 `activeMeshTaskId` 与调度器跟踪同步（否则区块可能会因陈旧的任务 ID 而卡住，永远不会重新提交）。
- 更改 `ChunkMesher` 网格 API 时，同时更新运行时和测试。
    - `tests/client/renderer/test_renderer.cpp` 现在调用 5 参数的 `generateSplitMesh(..., neighbors, cancelSignal)` 签名。
- 不要假设旧的 BaseLightEngine 队列位打包语义仍然有效。
    - 光照队列条目现在携带完整的世界坐标；如果重新引入 6 位 X/Z 截断或陈旧的解码路径，天空/方块光在远离原点时会严重不同步。
- 在天空光代码中，要明确内部级别的含义。
    - 内部级别是反转的（`0` 最亮，`15` 最暗）。从原版/Starlight 直接光代码移植逻辑时如果不转换，可能会静默反转清除/重传播行为。
- 对于天空光屋顶闭合修复，只从不透明源方块阻止增加传播。
    - 不要将相同的不透明源门控应用于减少路径，否则屋顶下的陈旧光永远不会被移除。
- 在减少传播期间处理 `currentLevel < targetLevel` 时，阻塞边缘（`target=darkest`）情况不能总是被视为"安全存活的源"。
    - 首先强制清除 + 减少级联，并将相邻的增加重检排队；否则在 FIFO 波前执行下侧面天空光可能会丢失。
- 不要将 `BaseLightEngine` 队列处理切换回 LIFO（`--length` 弹出末尾）。
    - Starlight 风格的传播依赖于 FIFO 波前排序；LIFO 会在复杂遮挡下引入不必要的振荡和延迟收敛。
- `ClientWorld::entityManager()` 返回 `ClientEntityManager`，而不是共享的 `common::EntityManager`。
    - 客户端本地 `Player` 不会在这条链路里跑 `Player::tick()`；客户端只会 tick 代理实体和本地物理。
- 不要假设 `m_player->isInWater()` 在客户端是权威的。
    - 这个值只会在 `Entity::baseTick()` / `updateEnvironmentState()` 或本地物理刷新路径里更新；它对本地玩家可用，但仍不是服务端权威结果。
- `Player::updateMoveDistance()` 现在使用专用的采样位置，`Player::setPosition()` 重置移动/晃动状态。
    - 不要再把 `prevPosition` 当成脚步声或视野晃动的采样基准；它是插值历史状态，多次物理更新会把同一段位移重复计数。
- `Player::updatePhysics()` 可以在没有物理引擎的轻量级测试世界中运行。
    - 在这种情况下，代码会回退到直接移动，所以测试应该验证状态刷新，而不是假设存在完整的碰撞求解器。
- `Entity::refreshDimensions()` 现在是重建实体缓存的 `EntitySize` 和 `AxisAlignedBB` 的规范方法。
    - 任何运行时大小更改都必须立即刷新缓存，否则移动和地面检查将继续使用陈旧的盒子。
- `DyeableArmorItem` 将颜色存储在 `ItemStack` 的结构化标签树中。
    - 清除颜色时也必须清除空的 `display` 标签，否则元数据相等性会发散，盔甲堆将停止按预期合并。
- 玩家站起过渡现在在从蹲伏/游泳/睡眠切换之前检查目标姿势盒子是否适合。
    - 当你需要原版风格的低天花板行为时，不要用原始站立姿势更改绕过 `setSneaking()` / `setSwimming()` / `setSleeping()`。
- `EntityMetadataPacket` / `EntityMetadataSerializer` 现在同时供给服务器跟踪和客户端实体应用。
    - `EntityTracker` 负责 spawn 内联 metadata 和 dirty metadata packet，`ClientEntity::setMetadata()` 负责把原始数据写进本地数据管理器；新增字段时三处必须一起改。
- `Entity::getTypeId()` 现在优先使用在 `EntityType::create(...)` 期间注入的显式运行时 `typeId`。
    - 不要再依赖 `LegacyEntityType` 单独决定网络实体类型；很多工厂构造仍传 `LegacyEntityType::Unknown`，正确做法是保证实体通过注册表创建时注入注册名，繁殖等旁路也要显式继承父类型。
- 在运行未初始化时，不要从区块拥有的 nibble 数组引导 `StarLightEngine::light(...)`。
    - 镜像 Moonrise：从临时 NULL 状态 nibble 开始，运行 `handleEmptySectionChanges(..., isUnlit=true)`，然后 `lightChunk(...)`，最后将生成的 nibble 写回区块。
- 在未初始化的 `light(...)` 引导中，不要从陈旧的默认映射种子空隙缓存。
    - 先将缓存种子为 null；否则 `SkyStarLightEngine::initNibble(...)` 可能计算错误的 `lowestY` 并跳过预期的天空光源设置。
- 不要为光照子系统重新引入兼容性别名。
    - `StarLightEngine`、`BlockStarLightEngine`、`SkyStarLightEngine` 和 `StarLightLightingProvider` 现在是规范名称。
- 通过 `LightEngineUtils::worldToSectionPos(...)` 保持世界->段转换集中化。
    - 在存储映射中重新引入临时位解码逻辑可能会静默地将写入/读取路由到错误的段。
- 不要盲目地将源面阻止应用于所有方块光传播。
    - 全立方体发光源仍需要向外传播；源面遮挡检查应针对条件形状。
- 不要在渲染热路径中通过 `toModelKey()` 路由 `BlockModelCache::getBlockAppearance(const BlockState*)`。
    - `stateId` 缓存是预期的快速路径；在那里重建模型键会重新引入可避免的字符串解析。
- `TridentEngine` 现在拥有实际的 MSAA 采样计数选择。
    - `WindowConfig` 不再携带采样数；`TridentContext::maxUsableSampleCount()` 将请求限制到硬件限制，`RenderPassManager` 在需要时创建多重采样颜色/深度附件加上解析附件，每个主通道管线必须接收相同的 `VkSampleCountFlagBits`。
- 不要将默认参数重新引入光照 API。
    - 如果调用模式需要简化形式，添加重载而不是默认参数。
- `WorldLightManager::tick(...)` 现在依赖有序的预算消耗。
    - 除非重新设计预算模型并匹配测试，否则不要恢复之前的五五开分配。
- 不要在 `WorldLightManager` 方块更新或光照查询入口点上重新引入 `BlockPos` 包装器重载。
    - 原始坐标现在是光照调度的规范接口。
- 在镜像 Starlight 分支流时，将原始光照级别处理保持在入口点本地，如果传播核心仍期望内部级别，则在排队前转换。
    - 将原始源级别直接输入传播队列会使当前的反转级别存储模型不同步。
- 不要将客户端光包视为立即网格重建触发器。
    - `ClientWorld` 现在使用 `meshRebuildPending` 来合并同一区块的重复 `onLightUpdate()` 调用，而任务仍处于活动状态。
- 不要直接从服务器应用程序代码发送 `BlockUpdatePacket`。
    - `ServerWorld::setOnBlockChanged()` 现在供给 `BlockUpdateSyncManager`；同坐标去重和 tick 结束刷新必须保持集中化。
- `KeepAlivePacket::deserialize()` 期望完整包（12 字节头 + 8 字节时间戳）。
    - 服务端处理心跳响应时不要先剥掉头部，否则单人模式下会把正常的 KeepAlive 回复误报成 `Packet too small for keep alive`。
- 测试 `ServerWorld::setBlock()` 时，先初始化世界。
    - 未初始化的世界会触发光照更新断言路径（`MC_ASSERT_RELEASE(false)`），因为 `m_lightManager` 为 null。
- 命令别名应使用 `CommandNode::setRedirect(...)` 而不是复制子子树。
    - `/teleport` 和 `/xp` 现在依赖重定向，因此命令树、帮助输出和建议都保持同步。
- `CommandDispatcher::getSuggestions()` 是规范的 Tab 补全入口点。
    - 通过 `CommandNode::setCustomSuggestions(...)` 附加动态补全数据，而不是在命令中硬编码 Tab 列表。
- `CommandRegistry::getCommandNames()` 派生自调度器树。
    - 帮助输出自动跟踪别名和未来的命令注册，所以不要重新引入单独的手动名称列表。
- `CommandTreePacket` 是客户端的权威命令快照。
    - 客户端补全必须在登录后从 `onCommandTree()` 重建，并在断开连接时再次清除，否则聊天建议会变得陈旧。
- `CommandTreePacket` 只序列化包体。
    - 服务器代码必须用 `ConnectionManager::encapsulatePacket()` 恰好包装一次，客户端代码必须在调用 `handleCommandTree()` 之前剥离外部网络头；双重封装会使内部头看起来像空 JSON 字符串。
- `SaplingBlock` 和 `TreeFeature` 必须就根支撑方块达成一致。
    - 如果你扩展一侧，必须在同一更改中扩展另一侧，否则会出现经典的"可以放置但不能生长"不匹配。
- `MushroomBlock` 自然 tick 用于低光传播，而不是巨型蘑菇构建。
    - 将巨型蘑菇生成保留在特性层，这样方块保持可测试和本地化。
- `CactusBlock` 碰撞伤害应仅针对活体实体。
    - 使用 `LivingEntity::hurt()` 和 `DamageSources::cactus()`；非活体碰撞应保持无操作。
- 不要将 `spdlog::spdlog` 或 `GTest::gtest` 直接链接到已经使用 `mc_common` 或 `GTest::gtest_main` 的可执行文件。
    - Apple ld 会在同一个静态库两次出现在最终链接行时发出重复库警告。
- 在 AppleClang 上，无参数的 `MC_TRACE_EVENT(...)` 或 `MC_TRACE_*` 调用仍可能触发 `-Wvariadic-macro-arguments-omitted`。
    - 当跟踪站点没有真正的参数时，传递一个显式的虚拟键/值负载。
- 流体流动判定必须区分"目标流体状态"和"用于阻挡判断的流体类型"。
    - 修改 `FlowingFluid::canFlow()` / `canFlowInto()` 时，不能把所有路径都硬塞成 `*this`，否则容器方块和特殊替换规则会偏离原版语义。
- 液体方块必须继续把随机 tick 透传给流体。
    - `LiquidBlock::ticksRandomly()` 和 `LiquidBlock::randomTick()` 是岩浆火焰扩散的入口，漏掉后会出现"方块看起来对了，但行为不触发"的假正确。
- 岩浆时序是世界相关的。
    - `ServerWorld::setBlock()` 和流体 tick 调度要继续使用 `fluid.getTickDelay(*this)`，不要把主世界/下界差异重新硬编码回固定常量。
- 天气降水判定不能只看温度。
    - `WeatherUtils::canRainAt()` / `canSnowAt()` 需要结合生物群系的 `BiomeClimate::Precipitation::None` 以及温度阈值一起判断；沙漠、蘑菇岛、恶地等无降水生物群系必须在注册数据里显式标记为 `None`。
- 玩家命令反馈不要默认依赖 `ServerPlayer*`。
    - `ServerCommandSource::sendMessage()` 现在必须在只有 `playerId` 的情况下也能把消息发回在线连接，不能只写日志。
- 玩家背包同步必须使用 `PlayerInventoryPacket`。
    - `ContainerContentPacket` 只保留给真正打开的容器菜单；玩家物品栏刷新、拾取同步和 `/clear` 这类操作都应走玩家背包包。
- 创造模式物品库写回必须使用 `CreativeInventoryActionPacket`。
    - `CreativeScreen` 负责本地搜索、滚动和槽位编辑，真正落盘到服务端时必须走这个专用包，不要复用普通容器点击包。
- `CreativeInventory` 相关测试和启动代码必须按 `VanillaBlocks::initialize()` -> `Items::initialize()` -> `BlockItemRegistry::instance().initializeVanillaBlockItems()` 的顺序初始化。
    - 少了 `VanillaBlocks` 这一步时，创造物品库会出现空列表或缺失方块物品。
- `InventoryManager::setOnInventoryUpdate()` 在 `MinecraftServer::initializeInteractionManagers()` 里已经接好。
    - 服务器侧背包变更如果走 `inventoryManager()`，就要依赖这条回调刷新客户端，不要再手写一套新的同步分支。
- 冰块融化与破坏路径必须分开处理。
    - `IceBlock::randomTick()` 只负责融化，`onBlockRemoved()` 只负责破坏后的替换；不要再让随机刻回调 `onBlockRemoved()`，也不要把同一坐标的写回放在旧方块回调之前。
    - `CropBlock` 的骨粉增长必须从世界种子和方块位置派生随机数，不能再回到全局 `rand()`。
        - `FarmlandBlock` 的降雨补湿要同时看 `isRaining()` 和 `canRainAt(pos.up())`，否则测试世界里会出现伪阳性。
        - `ClientApplication` 已按功能域拆到 `src/client/application/features/`，主文件只保留编排与生命周期。
            - `setupInputBindings()` / `setupCamera()` 等逻辑已迁移，不要再往主文件补回重复实现。
        - `ClientApplicationBootstrap.cpp` 负责客户端初始化骨架，`initialize()` 只做调度。
            - 核心注册表、窗口/输入、渲染、游戏系统和 UI 初始化都应继续下沉到 bootstrap/helper 方法里。
        - `ClientApplicationHelpers` 的公共辅助函数位于 `mc::client::application::features` 命名空间。
            - 调用处必须显式限定或导入作用域，否则会出现"找不到标识符"的连锁编译错误。
        - `TargetInfoWidget` 位于 `mc::client::ui::minecraft::targetinfo`，`DebugScreenWidget` 仍位于 `mc::client::ui::minecraft`。
            - 不要把这两个命名空间混用到同一条类型解析路径里，`ClientApplication` 的目标信息刷新逻辑已经拆到 `features/`。
        - `ClientApplication::handleEvents()` 现在只做输入轮询和分流。
            - 覆盖层输入放在 `handleUiOverlayInput()`，游戏快捷键放在 `handleGameplayShortcutInput()`，玩家视角/移动放在 `handleMouseAndMovementInput()`，不要把新逻辑再塞回 `handleEvents()`。
        - `ClientApplication::handleBlockMiningInput()` 和 `handleBlockPlacementInput()` 已分开。
            - 挖掘的取消、开始、完成逻辑继续留在独立 helper 里，不要重新合并成一个大输入状态机。
        - `ClientApplicationNetwork.cpp` 里的网络回调必须同时维护世界、实体、容器和经验状态。
            - 本地玩家、远程玩家、普通实体、经验球和当前打开的容器屏幕都要分别同步，不能把回调留成只接收不落地的空壳。

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
3. **模块关系** - 分析各组件之间的依赖关系
4. **整体职责** - 模块作为整体的职责说明
5. **输入/输出** - 数据流向分析
6. **依赖项** - 外部和内部依赖
7. **使用方法** - 代码示例和集成指南
8. **容易踩的坑** - 常见问题和解决方案
9. **测试用例** - 相关测试文件说明
10. **Mermaid图表** - 架构图、流程图、类图等（使用简体中文和彩色样式）

## 其他

### 对于重构类任务的准则

1.不做任何api兼容，直接一步到位新代码（因此主调者需要修改，这符合预期）
2.我有强烈代码洁癖，不允许留任何旧代码旧文件。不允许通过注释等任何手段保留旧代码，不允许以兼容为理由保留旧代码，不允许未经允许的情况下乱加adapter或兼容层来保留旧代码。重构类任务的目标就是把旧代码完全替换掉，留下干净的新实现。

### 对于所有任务的准则

当你完成一个任务后，不要停下来，请继续做后面的任务，直到任务清空你才能停！你时间充足、上下文也充足
