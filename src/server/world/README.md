# Server World 模块

服务端世界模块，负责服务端的核心世界管理功能，包括区块管理、实体追踪、自然生成、天气系统等。

## 目录结构

```
src/server/world/
├── ServerWorld.hpp/cpp              # 服务端世界核心类（区块/实体/光照/tick/方块实体tick/末影龙战斗管理/isBlockInLine射线遍历/getOrLoadChunk同步区块加载）
├── ServerLightQueue.hpp/cpp         # 运行时方块变更光照延迟队列（按区块分组去重，tick drain→submit worker 或 fallback 同步）
├── RuntimeLightingProvider.hpp/cpp  # 运行时/区块加载光照 worker provider（继承 StarLightLightingProvider；构造时 5×5 shared_ptr 保活；markLightChanged 收集 dirty section 而非回调）
├── RuntimeLightTask.hpp/cpp         # 运行时光照传播 worker 任务（继承 ITask；execute 经 TLS 引擎调 blocksChangedInChunk 传播→取 dirty→_enqueueLightFlush 入主线程 flush 队列）
├── ChunkLoadLightTask.hpp/cpp       # 区块加载光照 worker 任务（继承 ITask；execute 经 TLS 引擎 light()/forceHandleEmptySectionChanges+checkChunkEdges→取 dirty→_enqueueLightFlush + _enqueueChunkSend 续延）
├── ServerChunkManager.hpp/cpp       # 区块管理器（加载/生成/卸载协调，委托 ChunkTaskScheduler 调度生成）
├── SingleChunkLifecycleManager.hpp/cpp  # 单区块生命周期状态机（NewChunkHolder 等价物：请求聚合/状态推进/等待者/双向邻居依赖）
├── ChunkTaskScheduler.hpp/cpp       # 调度核心：schedule/checkNeighbour/onChunkGenComplete，持有 ReentrantAreaLock
├── ChunkProgressionTask.hpp/cpp     # 单状态推进任务（构建 WorldGenRegion，调用 _executeStepTask，完成后回调 onChunkGenComplete）
├── StaticChunkCache2D.hpp           # 预分配二维区块缓存模板（构造时一次性填充，无空洞，越界断言）
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
│ChunkTaskScheduler│ │ItemPickupManager│                          │ SpawnConditions │
│ (生成调度核心)   │ │  (物品拾取)      │                          │  (生成条件)      │
└────────┬────────┘ └─────────────────┘                          └─────────────────┘
         │
         ▼
┌─────────────────┐  ┌──────────────────────┐
│ChunkProgressionTask│ │ SingleChunkLifecycleManager │
│ (单状态推进任务) │  │  (NewChunkHolder 等价物)    │
└────────┬────────┘  └──────────────────────┘
         │                   │
         ▼                   ▼
┌─────────────────┐ ┌─────────────────┐
│ UniversalWorkerPool│ │BlockDropHandler │
│  (区域互斥线程池)│ │  (方块掉落)      │
└─────────────────┘ └─────────────────┘
```

**核心依赖链**：
- `ServerWorld` 持有所有子模块的实例或指针
- `ServerChunkManager` 依赖 `UniversalWorkerPool`（由 MinecraftServer 注入），委托 `ChunkTaskScheduler` 调度生成
- `ChunkTaskScheduler` 持有 `ReentrantAreaLock`（调度区域锁）和两个 `UniversalWorkerPool`（并行池 + 区域互斥池），协调 `SingleChunkLifecycleManager` 的状态推进
- `ChunkProgressionTask` 由 `ChunkTaskScheduler` 提交到 worker 池执行，构建 `WorldGenRegion`（邻居从 `StaticChunkCache2D<ChunkPrimer*>` 转换）并调用 `ServerChunkManager::_executeStepTask`
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
- `UniversalWorkerPool` - 计算线程池，由 MinecraftServer 统一管理

## 出生点管理

`ServerWorld` 负责管理世界出生点的位置和朝向，新玩家首次进入世界时在此处生成。

### 核心接口

| 接口 | 说明 |
|------|------|
| `worldSpawnPoint()` | 获取出生点坐标（`Vector3d`） |
| `spawnAngle()` | 获取出生点朝向角度（`f32`，度，范围 [-180, 180]） |
| `setWorldSpawnPoint(pos, angle)` | 同时设置出生点位置和朝向，`angle` 默认 `0.0f` |
| `setSpawnAngle(angle)` | 仅设置出生点朝向，不影响位置 |
| `initializeWorldSpawn()` | 世界初始化时查找合适出生位置；主世界使用 `Climate::Sampler::findSpawnPosition()` 气候搜索。仅主世界调用，下界/末地无独立出生点 |
| `applyLevelRuntimeData(data)` | 从 level.dat 读取的运行时数据中恢复出生点（含朝向）；SpawnY 为脚下方块 Y，读取时 +1 转为玩家脚位置 |

### 数据流

```
level.dat (SpawnX/Y/Z, SpawnAngle, initialized 字段)
    → LevelDatCodec::readRuntimeData 读取
    → LevelRuntimeData.{spawnX/Y/Z, spawnAngle, initialized}
    → MinecraftServer::initializeWorld 按 initialized 分支：
        - initialized=true  → applyLevelRuntimeData 直接使用存档出生点
        - initialized=false → applyLevelRuntimeData 后再 initializeWorldSpawn 覆盖为真实出生点
    → ServerWorld::m_worldSpawnPoint（玩家脚位置，方块上方）
    → MinecraftServer::saveAllWorldData 写回 level.dat（SpawnY 存脚下方块 Y，initialized 写 true）
    → MinecraftServer::sendInitialGameState 通过 SpawnPositionPacket 发送给客户端
    → /setworldspawn 命令修改后广播给所有玩家
```

**仅主世界持有世界出生点**：下界/末地不调 `initializeWorldSpawn`，其 `ServerWorld::m_worldSpawnPoint` 保持构造默认值且无消费方。所有出生点消费路径（玩家初始位置、指南针指向、重生兜底）只读主世界 `overworld->world()->worldSpawnPoint()`。

### initializeWorldSpawn 气候搜索流程

`initializeWorldSpawn()` 对主世界使用 `Climate::Sampler::findSpawnPosition()` 在气候空间中径向搜索最佳出生点：

```
NoiseChunkGenerator::randomState()  →  RandomState
                                      ↓
                                   sampler()  →  Climate::Sampler
                                                  ↓
                                              spawnTarget()  →  非空？
                                                  ↓ 是
                                              findSpawnPosition()  →  BlockPos
                                                  ↓
                                              ChunkPos(climateSpawn)  →  spawnChunk
                                                  ↓
                                              在 spawnChunk 内查找有效出生 Y 坐标
```

**降级行为**：当 `DimensionSettings::spawnTarget` 为空、`NoiseChunkGenerator` 不可用、`RandomState` 未初始化、或 `sampler.spawnTarget()` 为空时，自动降级为 (0,0) 区块作为出生点。

### 注意事项

- **initialized 字段**：level.dat 的 `initialized` 标记世界是否已完成首次出生点计算。`false` 时 `initializeWorld` 会调 `initializeWorldSpawn` 计算真实出生点并覆盖模板占位；`true` 时直接使用存档中的 SpawnX/Y/Z。新世界模板写入 `initialized=0` + `SpawnX/Y/Z=0/0/0`，首次启动计算后由 shutdown 落盘 `initialized=1`。
- **SpawnY 语义**：level.dat 中 `SpawnY` 为"脚下方块 Y"。`applyLevelRuntimeData` 读取时 +1 转为玩家脚位置（方块上方）；`saveAllWorldData` 写盘时 -1 转回脚下方块 Y。`m_worldSpawnPoint` 一律为玩家脚位置，与 `initializeWorldSpawn`、`setWorldSpawnPoint`（来自实体 position，即脚位置）、默认值 `(0, SEA_LEVEL+1, 0)` 语义统一。
- **朝向范围**：`SetWorldSpawnCommand` 已用 `wrapDegrees()` 归一化到 [-180, 180]。
- **保存一致性**：`saveAllWorldData()` 从 `world->spawnAngle()` 读取朝向值写入 level.dat，确保保存/加载循环不丢失。
- **客户端同步**：`SpawnPositionPacket` 包含 angle 字段，客户端 `ClientWorld::setSpawnPoint(x, y, z, angle)` 接收并存储。
- **Dimension 出生点**：`Dimension::spawnPoint()` 是每个维度的出生点（独立于世界出生点），当前不含朝向字段；仅跨维度传送兜底（`transferPlayerToDimension` 的 `position.value_or`）使用，下界/末地保持构造默认值。

## 容易踩的坑

### 区块异步生成架构（Moonrise 对齐）
区块生成系统已对齐 Moonrise mod 架构，彻底消除跑图时的 "missing chunk in access window" / "chunk status below request" 错误。核心设计：

- **`ChunkTaskScheduler`**（`ChunkTaskScheduler.hpp/cpp`）：调度核心。`schedule(x, z, targetStatus)` 一次只推进一步状态；`checkNeighbour` 验证邻居是否达到 `ChunkStep::getRequiredStatusAtRadius(distance)` 所需状态，未就绪则建立**双向依赖图**（`center.addBlockingNeighbour` / `neighbour.addWaitingNeighbour`）并挂起任务，**绝不返回低状态区块**；`onChunkGenComplete` 在任务完成时推进 `currentGenStatus`、释放邻居引用计数、通知等待的邻居重新调度。持有 `ReentrantAreaLock m_schedulingLockArea`（coordinateShift=6，对齐 Moonrise getChunkSystemLockShift()；每 section 覆盖 64×64 区块，使 2*maxAccessRadius 锁只触达 1~4 个 section），保证 检查→建依赖→创建任务→完成→通知 全流程原子。
- **`SingleChunkLifecycleManager`**（NewChunkHolder 等价物）：持有**可变 `std::unique_ptr<ChunkPrimer> m_currentChunk`** + `m_currentGenStatus`（非不可变快照）。`getChunkIfPresentUnchecked(status)` 检查 `m_currentGenStatus.isAtLeast(status)` 后返回 `m_currentChunk`（同一累积 Primer）。`onChunkGenComplete(status)` 推进 `m_currentGenStatus`（primer 同一对象，无需重存）。
- **`ChunkProgressionTask`**：单状态推进任务。`execute` 从 `StaticChunkCache2D<ChunkPrimer*>` 构建 `WorldGenRegion`（转换为 `vector<IChunk*>`，所有邻居已由 `checkNeighbour` 保证达到所需状态），调用 `_executeStepTask`，推进 primer 状态，FULL 完成时 `_finalizeGeneratedChunkSync` 转 `ChunkData` 存入内存缓存。
- **`StaticChunkCache2D<T>`**（`StaticChunkCache2D.hpp`）：预分配二维缓存模板，构造时一次性填充 `(2*radius+1)²` 个条目，**不允许 nullptr**（loader 必须返回有效值）。替代旧的 `GenerationChunkCache`（增量填充、允许空洞），从根本上消除窗口内 nullptr。
- **`ReentrantAreaLock`**（`common/util/concurrent/`）：按区块坐标的可重入区域锁，`lock(x,z,radius)` 覆盖 `[x±radius, z±radius]`，同线程重入仅限完全覆盖的子区域。`schedule` 持 `maxAccessRadius` 锁，`onChunkGenComplete` 持 `2*maxAccessRadius` 锁（覆盖邻居的邻居）。
- **`ChunkStep::getRequiredStatusAtRadius(radius)`**（`common/world/chunk/gen/ChunkStep.hpp`）：byRadius[] 查找表，对齐 Moonrise `ChunkStepMixin`。`schedule` 遍历 `[center±neighbourReadRadius]` 范围邻居，按 Chebyshev 距离查询每个邻居所需状态。
- **`UniversalWorkerPool` 区域互斥**（`common/util/thread/`）：`submit(task, callback, centerX, centerZ, writeRadius, priority, abortSignal)` 重载，保证同一时刻不存在两个 `writeRadius` 区域重叠的任务同时执行。`writeRadius ≤ 0` 的状态（EMPTY~INITIALIZE_LIGHT）走并行池；`writeRadius > 0`（FEATURES=1, LIGHT=2）走区域互斥池。

**并发模型**：邻居在任务执行期间可变（ChunkPrimer 累积式），但调度保证无并发写冲突——`checkNeighbour` 确保任务开始前所有邻居达所需状态，`UniversalWorkerPool` 区域互斥串行化重叠写区域（等价 Moonrise `AreaDependentQueue`）。这使 FEATURES/LIGHT 跨区块写邻居成为设计预期，而非 bug。

详见 `docs/BUG-WorldGenRegion-Access-Window.md`（问题根因）。

### 实体追踪器内存泄漏
实体移除后未从追踪器取消追踪会导致泄漏。`ServerWorld::removeEntity()` 会自动处理追踪器状态更新。如果直接调用 `entityManager().removeEntity()`，需要手动调用 `entityTracker().untrackEntity()`。

### 物品拾取延迟
刚丢弃的物品会被立即拾取。`ItemEntity` 默认有 10 tick 拾取延迟，`ItemPickupManager` 自动处理此逻辑。

### 天气状态不同步
客户端天气不一致时检查 `hasWeatherChanged()` 并广播天气更新包。

### 光照初始化时机
区块加载后光照由 `ChunkLoadLightTask` 在 worker 线程异步完成（效仿 Moonrise `ThreadedLevelLightEngine`）。`ChunkLoadedCallback` 调 `ServerWorld::enqueueChunkLoadLight` 入队，完成后经 `ServerWorld` 续延队列回主线程 flush + send。光照未完成时发送区块会导致客户端全黑，故 send 必须在 flush 之后（见下 tick 顺序）。

### 运行时方块变更光照异步传播
`ServerWorld::setBlockState` 的光照更新**不在调用当场传播**，而是入队 `ServerLightQueue`（按区块分组、同坐标去重）。`ServerWorld::tick` 光照段顺序严格 **flush → send → drain**：

1. **`_drainPendingLightFlushes()`**（主线程）：取出上一 tick worker 完成后入队的 dirty section，逐项调真正的 `markLightChanged`（`_syncLightDataToChunk` 把 visible nibble 同步到 ChunkSection + `m_onLightChanged` 网络包）。
2. **`_drainPendingChunkSends()`**（主线程）：取出上一 tick worker 完成光照的区块坐标，调 `ChunkSendManager::sendChunkToTrackingPlayers`（serialize 读已 flush 的 ChunkSection nibble）+ `removeLightTicket` 释放 LIGHT 票据。
3. **`m_lightQueue.drainAndProcess(*this)`**（主线程入队 worker）：swap 出任务表，逐区块构造 `RuntimeLightTask` 提交到 `UniversalWorkerPool` 区域互斥池（writeRadius=2）。executor 为 nullptr 时 fallback 同步经 TLS 引擎调 `blocksChangedInChunk`。

引擎已无 `m_mutex`（③-2b）：`WorldLightManager` 改 `thread_local` 引擎池（`acquireSkyLightEngine`/`acquireBlockLightEngine`），每个 worker 线程独占一套引擎，无引擎级锁。所有光照写操作（区块加载光照、运行时方块变更、LIGHT 生成阶段）统一经同一 `UniversalWorkerPool` 区域互斥池（writeRadius=2），重叠 5×5 区域的 nibble 写必被区域锁串行 → 满足 `SWMRNibbleArray` 更新侧非原子单写者语义。

`RuntimeLightTask` / `ChunkLoadLightTask`（worker 线程）：持 `RuntimeLightingProvider`（5×5 `shared_ptr<ChunkData>` 保活防在途 UAF），经 TLS 引擎调 `blocksChangedInChunk` / `light` / `forceHandleEmptySectionChanges`+`checkChunkEdges`；`updateVisible` 内部的 `markLightChanged` 经 provider 收集到 `m_dirtySections` 而非触碰主线程独占回调；任务末尾 `_enqueueLightFlush` 把 dirty section 入主线程 flush 队列，`ChunkLoadLightTask` 还额外 `_enqueueChunkSend` 入区块发送续延队列。

因此存在**最多 2 tick 的最终一致性窗口**：setBlockState 返回 →（tick N）submit worker → worker 完成 + 入队 dirty →（tick N+1）flush visible。`getLightSubtracted` 等查询在 flush 前可能读旧值。若有逻辑依赖即时光照（如方块变更回调里立即查亮度），需自行评估或改走 tick 后查询。生成阶段（LIGHT step）的光照不经过此队列，由 `ServerChunkManager` LIGHT 分支经 TLS 引擎 `light()` 同步完成。

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
- **handlePrecipitation**：已实现 `Block::handlePrecipitation` 系统。在世界下雨时，对每个降水位置的表面方块调用 `handlePrecipitation()`，传入降水类型（Rain/Snow）。`CauldronBlock` 重写此方法：雨天 5% 概率增加水位，雪天 10% 概率增加水位。`LightningRodBlock` 重写此方法：雷暴时朝上的避雷针被激活。

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

### 末影龙战斗管理
`ServerWorld` 在末地维度（`DimensionManager::THE_END`）下持有 `EndDragonFight` 实例（`m_dragonFight`），通过 `IWorld::dragonFight()` 虚方法暴露给实体层。其他维度默认返回 `nullptr`。

**生命周期**：
- **创建**：`ServerWorld::initialize()` 中，当维度为末地时，从 `SingleLevelStorageManager::loadDragonFightData()` 加载已有数据或创建新实例
- **保存**：`ServerWorld::saveAll()` 中，调用 `SingleLevelStorageManager::saveDragonFightData()` 持久化战斗状态到 `data/end_dragon_fight.json`
- **使用**：`EnderDragonEntity::_onDeathUpdate()` 通过 `world.dragonFight()` 查询击杀状态并分发奖励

**数据流**：
```
data/end_dragon_fight.json
    → SingleLevelStorageManager::loadDragonFightData()
    → EndDragonFight::Data::fromJson()
    → EndDragonFight(worldSeed, data)
    → EnderDragonEntity._onDeathUpdate()
        → dragonFight->hasPreviouslyKilled()  // 经验区分
        → dragonFight->setDragonKilled()       // 龙蛋/折跃门/出口传送门
    → EndDragonFight::saveData().toJson()
    → SingleLevelStorageManager::saveDragonFightData()
    → data/end_dragon_fight.json
```

### 方块实体 tick 系统

`ServerWorld::tickBlockEntities()` 对应 MC Java 的 `Level.tickBlockEntities()`，在每 game tick 中遍历所有已加载区块的方块实体，对 `needsTick() == true` 且未被移除的方块实体调用其 `tick()` 方法。

**调用时机**：在 `ServerWorld::tick()` 中，TickManager 之后、tickEnvironment 之前执行。调试世界（`isDebugWorld()`）不执行方块实体 tick。

**遍历方式**：通过 `ServerChunkManager::forEachLoadedChunk()` 遍历所有已加载区块，对每个区块调用 `ChunkData::getAllBlockEntities()` 获取快照（返回 `std::vector<BlockEntity*>`，按值返回避免迭代失效），然后逐一检查并 tick。

**需要 tick 的方块实体**（`needsTick() == true`）：
- `MobSpawnerBlockEntity` — 刷怪笼（周期性生成实体）
- `AbstractFurnaceEntity` — 熔炉/高炉/烟熏炉（燃烧进度）
- `BrewingStandEntity` — 酿造台（酿造进度）
- `HopperEntity` — 漏斗（物品传输）
- `BeaconEntity` — 信标（效果应用）
- `ConduitEntity` — 潮涌核心（效果应用）
- `CampfireBlockEntity` — 营火（烹饪进度）
- `CommandBlockEntity` — 命令方块（命令执行）
- `PistonBlockEntity` — 活塞（移动动画）
- `DaylightDetectorEntity` — 日光探测器（信号更新）
- `TrialSpawnerBlockEntity` — 试炼刷怪笼（状态机推进）
- `CrafterBlockEntity` — 自动合成器（红石脉冲）

**不需要 tick 的方块实体**（`needsTick() == false`，默认值）：
- `ChestEntity`、`BarrelEntity`、`ShulkerBoxEntity` 等纯存储方块实体
- `SignEntity`、`BannerEntity`、`LecternEntity` 等交互方块实体

**注意事项**：
- 方块实体在 tick 期间可能修改所在区块的方块实体映射（如活塞移动方块实体），因此必须使用 `getAllBlockEntities()` 的快照而非直接引用 `m_blockEntities`。
- 如果 tick 中的方块实体被移除（`isRemoved() == true`），应跳过其 tick。

### getOrLoadChunk 同步区块加载

`ServerWorld` 覆写 `IWorld::getOrLoadChunk(ChunkCoord x, ChunkCoord z)`，委托给 `m_chunkManager->requestFullChunkSync(x, z)`，对应 MC Java 的 `Level.getChunk(x, z, require=true)`：区块已加载则直接返回，否则在主线程上同步触发加载/生成。

**线程安全**：与 `requestFullChunkSync` 相同，仅在服务端主线程调用安全（内部通过 `_drainPendingLoadCompletes` 泵送避免死锁）。

**使用场景**：`EndGatewayEntity::_generateExitPortal` 在末地外岛扫描区块判空时调用 `world.getOrLoadChunk()`，完整复刻 MC Java 的 `TheEndGatewayBlockEntity.findExitPortalXZPosTentative` 行为。其他 common 层代码需要按需加载区块时也应使用此接口，而非直接调用 `ServerChunkManager`（common 层无法依赖 server 层）。

### broadcastBlockEntity 方块实体数据广播

`ServerWorld::broadcastBlockEntity(const BlockPos& pos)` 是方块实体数据变化后通知客户端的入口，对应 MC Java 的 `Level.markAndNotifyBlock` → `ServerPlayerGameMode.handleBlockChanged` 链路中的 `ServerLevel.sendBlockUpdated`。

**调用链**：
1. 方块实体内部数据变化（如告示牌编辑、箱子内容更新等）后调用 `world.broadcastBlockEntity(pos)`
2. `ServerWorld` 通过 `m_onBroadcastBlockEntity` 回调转发给上层（避免直接依赖 `MinecraftServer`）
3. `MinecraftServer::attachWorldBindings()` 中注册的回调：
   - 通过 `ServerWorld::getBlockEntity(pos)` 获取方块实体
   - 调用 `entity->getUpdateTag()` 生成 NBT 复合标签，包装为 `shared_ptr<nbt::CompoundTag>`
   - 调用 `MinecraftServer::broadcastBlockEntityInRange(pos, type, shared_ptr<CompoundTag>, 64.0f)` 广播
4. `broadcastBlockEntityInRange` 构造 `ir::play::BlockEntityData`（blockPosPacked + blockEntityType + CompoundTag，无长度前缀）发送给 64 格范围内的所有已登录玩家

**使用场景**：
- `ServerPlayer::handleUpdateSignPacket()` — 告示牌编辑完成后广播新文本
- `ServerPlayer::openSignEditor()` — 打开告示牌编辑器前先发送当前内容
- 其他需要同步方块实体数据变化的场景（如命令方块内容更新、箱子物品变化等）

**注意事项**：
- 未注册 `m_onBroadcastBlockEntity` 回调时调用 `broadcastBlockEntity()` 不会崩溃（空回调检查）。
- 回调注册通过 `setOnBroadcastBlockEntity()` 完成，由 `MinecraftServer::attachWorldBindings()` 在世界加载时统一注册。
- 对应的客户端接收路径：`NetworkClient::_handleBlockEntityData()` → `ClientWorld::onBlockEntityData()` → `BlockEntity::loadFromNBT()`。
