# Server World 模块

服务端世界模块，负责服务端的核心世界管理功能，包括区块管理、实体追踪、自然生成、天气系统等。

## 目录结构

```
src/server/world/
├── ServerWorld.hpp/cpp              # 服务端世界核心类（区块/实体/光照/tick管理）
├── ServerChunkManager.hpp/cpp       # 区块管理器（加载/生成/卸载协调）
├── ChunkGenerateTask.hpp/cpp        # 区块生成任务（提交到 ServerWorkerPool）
├── drop/
│   ├── BlockDropHandler.hpp/cpp     # 方块掉落处理器（LootTable系统）
│   └── README.md
├── entity/
│   ├── EntityTracker.hpp/cpp        # 实体追踪器（客户端可见性管理）
│   ├── EntityChunkTracker.hpp/cpp   # 实体区块追踪（按区块追踪实体）
│   ├── ItemPickupManager.hpp/cpp    # 物品拾取管理器
│   └── README.md
├── player/
│   ├── ServerPlayerEntityManager.hpp/cpp  # 服务端玩家实体管理
│   └── README.md
├── spawn/
│   ├── NaturalSpawner.hpp/cpp       # 自然生成器（怪物/动物/环境生物）
│   ├── SpawnConditions.hpp/cpp      # 生成条件检查工具
│   ├── DespawnManager.hpp/cpp       # 消失管理器（实体消失距离检查）
│   ├── VillageSiege.hpp/cpp         # 村庄围攻事件
│   └── README.md
└── weather/
    ├── WeatherManager.hpp/cpp       # 天气管理器（天气周期/闪电）
    └── README.md
```

## 内部模块关系

```
                    ┌─────────────────┐
                    │   ServerWorld   │
                    │  (核心容器)      │
                    └────────┬────────┘
                             │
         ┌───────────────────┼───────────────────┬───────────────────┐
         │                   │                   │                   │
         ▼                   ▼                   ▼                   ▼
┌─────────────────┐ ┌─────────────────┐ ┌─────────────────┐ ┌─────────────────┐
│ServerChunkManager│ │  EntityTracker  │ │ WeatherManager  │ │ NaturalSpawner  │
│  (区块管理)      │ │  (实体追踪)      │ │  (天气管理)     │ │  (自然生成)     │
└────────┬────────┘ └────────┬────────┘ └─────────────────┘ └────────┬────────┘
         │                   │                                            │
         ▼                   ▼                                            ▼
┌─────────────────┐ ┌─────────────────┐                          ┌─────────────────┐
│ChunkGenerateTask│ │ItemPickupManager│                          │ SpawnConditions │
│  (生成任务)      │ │  (物品拾取)      │                          │  (生成条件)      │
└────────┬────────┘ └─────────────────┘                          └─────────────────┘
         │                   │
         ▼                   ▼
┌─────────────────┐ ┌─────────────────┐
│ ServerWorkerPool│ │BlockDropHandler │
│  (通用线程池)   │ │  (方块掉落)      │
└─────────────────┘ └─────────────────┘
```

**核心依赖链**：
- `ServerWorld` 持有所有子模块的实例或指针
- `ServerChunkManager` 依赖 `ServerWorkerPool`（由 MinecraftServer 注入）
- `EntityTracker` 与 `ItemPickupManager` 协同处理实体可见性和拾取
- `NaturalSpawner` 依赖 `SpawnConditions` 进行生成位置检查

## 上下游外部依赖关系

**上游依赖**：
- `server/core/MinecraftServer` - 创建 ServerWorld，注入 Worker 池和存储管理器
- `server/core/ServerDimensionManager` - 多维度管理，协调各维度 ServerWorld
- `common/world/IWorld` - 世界接口定义
- `common/world/ICollisionWorld` - 碰撞世界接口
- `common/world/chunk/` - 区块数据和生命周期管理

**下游消费者**：
- `server/core/PlayerManager` - 通过 ServerWorld 管理玩家实体
- `server/sync/BlockUpdateSyncManager` - 接收方块变化回调
- `server/sync/ChunkSendManager` - 接收区块加载事件
- `server/sync/EntitySyncManager` - 接收实体状态变化
- `server/command/` - 命令执行（通过 ServerWorld 接口）

**共享资源**：
- `SingleLevelStorageManager` - 由 MinecraftServer 创建，所有维度共享
- `ServerWorkerPool` - 计算线程池，由 MinecraftServer 统一管理

## 出生点管理

`ServerWorld` 负责管理世界出生点的位置和朝向，新玩家首次进入世界时在此处生成。

### 核心接口

| 接口 | 说明 |
|------|------|
| `worldSpawnPoint()` | 获取出生点坐标（`Vector3d`） |
| `spawnAngle()` | 获取出生点朝向角度（`f32`，度，范围 [-180, 180]） |
| `setWorldSpawnPoint(pos, angle)` | 同时设置出生点位置和朝向，`angle` 默认 `0.0f` |
| `setSpawnAngle(angle)` | 仅设置出生点朝向，不影响位置 |
| `initializeWorldSpawn()` | 世界初始化时在 (0,0) 区块查找合适出生位置 |
| `applyLevelRuntimeData(data)` | 从 level.dat 读取的运行时数据中恢复出生点（含朝向） |

### 数据流

```
level.dat (SpawnAngle 字段)
    → JavaLevelDatReader / LevelDatCodec 读取
    → LevelRuntimeData.spawnAngle
    → ServerWorld::applyLevelRuntimeData() 写入 m_spawnAngle
    → MinecraftServer::saveAllWorldData() 通过 world->spawnAngle() 写回 level.dat
    → MinecraftServer::sendInitialGameState() 通过 SpawnPositionPacket 发送给客户端
    → /setworldspawn 命令修改后广播给所有玩家
```

### 注意事项

- **朝向范围**：MC 原版使用 `Mth.wrapDegrees()` 归一化到 [-180, 180]，`SetWorldSpawnCommand` 已做归一化处理。
- **保存一致性**：`saveAllWorldData()` 从 `world->spawnAngle()` 读取朝向值写入 level.dat，确保保存/加载循环不丢失。
- **客户端同步**：`SpawnPositionPacket` 包含 angle 字段，客户端 `ClientWorld::setSpawnPoint(x, y, z, angle)` 接收并存储。
- **Dimension 出生点**：`Dimension::spawnPoint()` 是每个维度的出生点（独立于世界出生点），当前不含朝向字段。

## 容易踩的坑

### 区块异步生成竞态条件
多个线程同时请求同一区块可能导致重复生成。`ServerChunkManager` 使用 `SingleChunkLifecycleManager` 管理状态，通过 `m_syncGenerationMutex` 保护同步生成。**应优先使用异步 API**，避免在主线程频繁使用 `getChunkSync()`。

### 实体追踪器内存泄漏
实体移除后未从追踪器取消追踪会导致泄漏。`ServerWorld::removeEntity()` 会自动处理追踪器状态更新。如果直接调用 `entityManager().removeEntity()`，需要手动调用 `entityTracker().untrackEntity()`。

### 物品拾取延迟
刚丢弃的物品会被立即拾取。`ItemEntity` 默认有 10 tick 拾取延迟，`ItemPickupManager` 自动处理此逻辑。

### 天气状态不同步
客户端天气不一致时检查 `hasWeatherChanged()` 并广播天气更新包。

### 光照初始化时机
区块加载后光照未初始化会导致客户端显示错误。设置 `ChunkLoadedCallback` 在区块加载完成后初始化光照。

### 未初始化世界调用 setBlockState
在未调用 `initialize()` 的 `ServerWorld` 上调用 `setBlockState()` 会在光照更新阶段触发断言。**所有需要光照的测试必须先初始化世界**。不需要光照的测试（如 `tickPrecipitation`）可以直接调用，因为 `tickPrecipitation()` 仅依赖 `m_chunkManager` 和 `m_weatherManager`，无需 `initialize()`。

### tickPrecipitation 降水 tick 系统

`ServerWorld::tickPrecipitation()` 实现了 MC 的冰/雪运行时形成逻辑（对应 MC 的 `ServerLevel.tickIceAndSnow()`）：

- **冰形成**：不受天气状态影响，低温生物群系中水面自动结冰。使用 `Biome::shouldFreeze(world, x, y, z, seaLevel, true)` 检查温度、光照、流体类型和邻居水域暴露。
- **降雪**：仅在下雨时执行（`WeatherManager::isRaining()`），且 `MAX_SNOW_ACCUMULATION_HEIGHT` > 0 时才放置雪层。使用 `Biome::shouldSnow()` 检查温度、光照、空气/已有雪层和下方支撑。
- **概率**：每个 `randomTickSpeed` 迭代以 1/48 概率触发降水 tick，与 MC 的随机 tick 概率一致。
- **高度图**：使用 `MOTION_BLOCKING` 高度图确定表面 Y 坐标。冰检查位置是 `topY`（水面/地面），雪检查位置是 `topY + 1`（上方空气）。
- **实体推出**：雪层增加时调用 `Block::pushEntitiesUp()` 将嵌入方块的实体向上推出，防止实体卡入方块。
- **SNOWY 属性**：放置新雪层时，更新下方方块的 SNOWY 属性（草方块、菌丝等）。
- **handlePrecipitation**：TODO - 尚未实现 `Block::handlePrecipitation` 系统（炼药锅填充等）。

### ServerWorld 不再默认自建 ChunkManager
`ServerWorld` 必须由外部注入 `ServerChunkManager`，再调用 `initialize()`。主调者通过 `ServerDimensionManager` 或测试装配 helper 显式创建。

### 多维度重复打开世界存档
每个维度 `ServerWorld` 都自己 `open()` 世界目录会导致下界/末地初始化时重复获取同一个 `WorldSessionLock`。**`SingleLevelStorageManager` 提升到 MinecraftServer 层，只初始化一次**。

### 共享存储重复全量保存
三个 `ServerWorld` 共享存储，关服时如果每个都执行 `saveAll()` 会重复落盘。**共享存储的全量保存由 MinecraftServer 统一执行，`ServerWorld::shutdown()` 只释放自身资源**。

### 析构函数里做业务关闭
析构函数执行保存、发包、广播等业务逻辑会导致重复副作用。**析构函数只允许兜底式本地释放，`shutdown()/close()` 必须幂等**。

### 替换 ChunkManager 时视距回退
替换 `ServerChunkManager` 后未同步 `viewDistance` 会导致首帧加载区块数量异常。**`ServerWorld::setChunkManager()` 现在自动同步视距**。

### 调试世界判断方式
旧代码通过 `ServerWorldConfig.isDebugWorld` 字段判断（默认为 true，导致所有世界被错误识别）。**现在通过 `IChunkGenerator::isDebugGenerator()` 虚方法检测**。

### 区块生成任务不要让子代理执行
区块生成线程池是全局共享资源，子代理执行可能导致构建系统锁死。**必须由主代理管理**。

### 维度感知建筑高度
`ServerWorld` 覆写了 `IWorld::getMinBuildHeight()` 和 `IWorld::getMaxBuildHeight()`，基于 `DimensionType::minHeight()` / `DimensionType::maxHeight()` 返回维度特定的建筑高度范围。这意味着不同维度的 `ServerWorld` 实例会返回不同的高度界限（如下界 -64~128、末地 0~256 等）。代码中需要维度感知高度时应调用这两个虚方法而非硬编码 `world::MIN_BUILD_HEIGHT` / `world::MAX_BUILD_HEIGHT` 全局常量。

### notifyBlockUpdate 与 setBlockState 的区别
方块实体内部数据变化后需要通知客户端时，应使用 `notifyBlockUpdate(pos)` 而非 `setBlockState(pos, state, 3)`。`setBlockState` 在 `oldState == newState` 时直接返回 false，不会触发 `m_onBlockChanged` 回调，客户端收不到更新。`notifyBlockUpdate` 即使方块状态未改变也会触发回调，对应 MC Java 的 `Level.sendBlockUpdated()`。
