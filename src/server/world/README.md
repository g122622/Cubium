# Server World 模块

服务端世界模块，负责服务端的核心世界管理功能，包括区块管理、实体追踪、自然生成、天气系统等。

## 目录结构

```
src/server/world/
├── ServerWorld.hpp/cpp              # 服务端世界核心类（生成器海平面/区块/实体/光照/tick/方块实体tick/末影龙战斗管理/isBlockInLine射线遍历/chunkManager区块访问）
├── ServerLightQueue.hpp/cpp         # 运行时方块变更光照延迟队列（按区块分组去重，tick drain→submit worker 或 fallback 同步）
├── RuntimeLightingProvider.hpp/cpp  # 运行时/区块加载光照 worker provider（继承 StarLightLightingProvider；构造时 5×5 shared_ptr 保活；markLightChanged 收集 dirty section 而非回调）
├── RuntimeLightTask.hpp/cpp         # 运行时光照传播 worker 任务（继承 ITask；execute 经 TLS 引擎调 blocksChangedInChunk 传播→取 dirty→_enqueueLightFlush 入主线程 flush 队列）
├── ChunkLoadLightTask.hpp/cpp       # 区块加载光照 worker 任务（继承 ITask；execute 经 TLS 引擎 light()/forceHandleEmptySectionChanges+checkChunkEdges→取 dirty→_enqueueLightFlush + _enqueueChunkSend 续延）
├── ServerChunkManager.hpp/cpp       # 区块管理器（加载/生成/卸载协调，委托 ChunkTaskScheduler 调度生成）
├── SingleChunkLifecycleManager.hpp/cpp  # 单区块生命周期状态机（票据/显式请求/共享邻居需求汇总、状态推进与等待者）
├── scheduling/                    # 世界区域依赖与计算任务分发
│   ├── AreaTaskScheduler.hpp/cpp   # 区域就绪后提交计算池，生成/光照/序列化共用
│   └── README.md                 # 调度边界与生命周期约束
├── ChunkTaskScheduler.hpp/cpp       # 调度核心：schedule/checkNeighbour/onChunkGenComplete，持有 ReentrantAreaLock
├── ChunkProgressionTask.hpp/cpp     # 单状态推进任务（构建 WorldGenRegion，调用 _executeStepTask，完成后回调 onChunkGenComplete）
├── StaticChunkCache2D.hpp           # 预分配二维区块缓存模板（构造时一次性填充，无空洞，越界断言）
├── blockevent/
│   ├── BlockEventData.hpp           # 方块事件数据结构（位置、方块类型、事件参数）
│   └── README.md
├── drop/
│   ├── BlockDropHandler.hpp/cpp     # 方块掉落处理器（LootTable系统）
│   └── README.md
├── entity/
│   ├── EntityTracker.hpp/cpp        # 实体追踪器（客户端可见性管理）
│   ├── ItemPickupManager.hpp/cpp    # 物品拾取管理器
│   └── README.md
├── player/
│   ├── ServerPlayerEntityManager.hpp/cpp  # 服务端玩家实体管理
│   └── README.md
├── spawn/
│   ├── NaturalSpawner.hpp/cpp       # 自然生成器（怪物/动物/环境生物）
│   ├── DespawnManager.hpp/cpp       # 消失管理器（实体消失距离检查）
│   ├── VillageSiege.hpp/cpp         # 村庄围攻事件
│   └── README.md
├── time/
│   ├── GameTime.hpp/cpp            # 游戏时间（昼夜循环/日光周期）
│   └── README.md
└── weather/
    ├── WeatherManager.hpp/cpp       # 天气管理器（天气周期/闪电）
    └── README.md
```

## 内部模块关系

区块存档缺失后的生成调度在区域执行器上获取调度锁；排队期间用共享引用保活 holder，并在执行前校验身份。
主线程按时间预算消费存档完成、世界后处理和卸载队列，未处理的旧任务先于新任务交付；同步请求持续泵送目标 Future。
LIGHT 票据释放由后续统一票据推进传播，避免批量发送区块时反复遍历距离图。
票据回调只尝试取得调度锁；繁忙时合并同坐标的最新级别，后续推进重试，期间暂缓自动卸载。
随机刻、方块实体和降水使用独立模拟范围：玩家实体模拟距离外再保留一圈方块 tick，显式强票据按级别保留范围。

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
│ChunkTaskScheduler│ │ItemPickupManager│                          │ 生成规则注册表 │
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
- `ServerChunkManager` 持有每世界独立的 `AreaTaskScheduler`；`ChunkTaskScheduler` 持有 `ReentrantAreaLock` 保护生成依赖图，并委托世界区域调度器分发任务，协调 `SingleChunkLifecycleManager` 的状态推进
- `ChunkProgressionTask` 由 `ChunkTaskScheduler` 提交到 worker 池执行，构建 `WorldGenRegion`（邻居从 `StaticChunkCache2D<ChunkPrimer*>` 转换）并调用 `ServerChunkManager::_executeStepTask`
- `EntityTracker` 与 `ItemPickupManager` 协同处理实体可见性和拾取
- `NaturalSpawner` 通过公共生成注册表检查放置条件与实体谓词

## 上下游外部依赖关系

**上游依赖**：
- `server/core/MinecraftServer` - 创建 ServerWorld，注入 Worker 池和存储管理器
- `server/core/ServerDimensionManager` - 多维度管理，协调各维度 ServerWorld
- `common/world/IWorld` - 世界接口定义
- `common/world/ICollisionWorld` - 碰撞世界接口
- `common/world/chunk/` - 区块数据和生命周期管理

**下游消费者**：
- `server/core/PlayerManager` - 通过 ServerWorld 管理玩家实体
- `server/network/sync/BlockUpdateSyncManager` - 接收方块变化回调
- `server/network/sync/ChunkSendManager` - 接收区块加载事件
- `server/command/` - 命令执行（通过 ServerWorld 接口）

**共享资源**：
- `SingleLevelStorageManager` - 由 MinecraftServer 创建，所有维度共享
- `UniversalWorkerPool` - 计算线程池，由 MinecraftServer 统一管理

## 出生点管理

`ServerWorld` 持有存档级世界出生数据（`RespawnData`：维度 + 出生方块 + yaw/pitch）。整个存档只有一份，
保存在主世界 level.dat 的 `spawn` 复合标签中；新玩家进入世界时从共享出生点附近搜索落点。

### 核心接口

| 接口 | 说明 |
|------|------|
| `respawnData()` | 获取原始出生数据（未做边界修正） |
| `setRespawnData(data)` | 设置出生数据（`/setworldspawn` 调用） |
| `spawnPosition()` | 出生位置脚部坐标（方块中心 X/Z +0.5，Y 为方块上表面） |
| `worldBorderAdjustedRespawnData()` | 出生方块在边界外时改取边界中心列的 MOTION_BLOCKING 高度图位置 |
| `initializeWorldSpawn()` | 新世界首次启动查找出生位置；主世界使用 `Climate::Sampler::findSpawnPosition()` 气候搜索。仅主世界调用 |
| `applyLevelRuntimeData(data)` | 从 level.dat 读取的运行时数据中恢复出生数据 |

### 数据流

```
level.dat (spawn 复合标签：dimension/pos/yaw/pitch，initialized 字段)
    → LevelDatCodec::readRuntimeData 读取
    → LevelRuntimeData.spawnData + initialized
    → MinecraftServer::initializeWorld 按 initialized 分支：
        - initialized=true  → applyLevelRuntimeData 直接使用存档出生数据
        - initialized=false → applyLevelRuntimeData 后再 initializeWorldSpawn 覆盖为真实出生点
    → ServerWorld::m_respawnData
    → MinecraftServer::saveAllWorldData(true) 写回 level.dat（spawn 复合标签，initialized 写 true）
    → 登录/重生通过 SetDefaultSpawnPosition 下发（经边界修正的有效出生数据）
    → /setworldspawn 命令修改后广播给所有玩家
```

**仅主世界持有世界出生数据**：下界/末地不调 `initializeWorldSpawn`，所有出生数据消费路径
（玩家初始位置、指南针指向、重生兜底）只读主世界 `overworld->world()`。

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

- **initialized 字段**：level.dat 的 `initialized` 标记世界是否已完成首次出生点计算。`false` 时 `initializeWorld` 会调 `initializeWorldSpawn` 计算真实出生点并覆盖模板占位；`true` 时直接使用存档中的 `spawn` 复合标签。新世界模板写入 `initialized=0` 与默认出生数据。
- **位置是出生方块本身**：玩家脚部在 `pos.y + 1`。`spawnPosition()` 负责换算；网络包 `SetDefaultSpawnPosition` 下发的是方块坐标。
- **朝向范围**：`RespawnData::of` 归一化 yaw 到 [-180, 180]、夹紧 pitch 到 [-90, 90]。
- **保存一致性**：`saveAllWorldData()` 直接写 `world->respawnData()`，保存/加载循环不丢失维度与 pitch。
- **共享出生点落点搜索**：登录与新玩家进入时由 `PlayerSpawnFinder::findSpawn()` 在 `spawnRadius` 规则范围内寻找无碰撞无液体位置；下界/末地或冒险模式世界只做高度修正。
- **Dimension 不再持有出生点**：旧 `Dimension::m_spawnPoint` 已删除，跨维度传送必须显式给出目标坐标。

## 容易踩的坑

- 每个生成步骤的 WorldGenRegion 必须绑定生成器种子，不能依赖 ServerWorld 是否存在；独立区块管理器同样需要以正确种子进行群系边界抖动和放置过滤。

### 生成需求撤销与任务完成

票据降级要重新汇总目标，不能用“安全可卸载”作为取消需求的前提。显式请求按各自目标交付，
共享邻居仅在最后一条有效依赖移除后停止推进。排队任务可发取消信号，但任务身份和邻居引用必须
保留到执行器确认完成；已开始的生成阶段完整执行，再根据最新需求决定是否继续。延迟重调度只
保存坐标，不能保存旧目标，也不能在旧任务尚未结束时复位取消令牌。

### 卸载冷却与距离图回调

卸载收尾发现持有者仍被引用时，以最弱生成级别注册冷却票据。冷却存在期间保留持有者，到期后恢复回收。生成邻域和退场回调只更新已有持有者，邻居依赖由调度器按需创建，避免冷却传播重新引发生成与卸载循环。

### 区块异步生成架构（Moonrise 对齐）
区块生成系统已对齐 Moonrise mod 架构，彻底消除跑图时的 "missing chunk in access window" / "chunk status below request" 错误。核心设计：

- **`ChunkTaskScheduler`**（`ChunkTaskScheduler.hpp/cpp`）：调度核心。`schedule(x, z, targetStatus)` 一次只推进一步状态；`checkNeighbour` 验证邻居是否达到 `ChunkStep::getRequiredStatusAtRadius(distance)` 所需状态，未就绪则建立**双向依赖图**（`center.addBlockingNeighbour` / `neighbour.addWaitingNeighbour`）并挂起任务，**绝不返回低状态区块**；`onChunkGenComplete` 在任务完成时推进 `currentGenStatus`、释放邻居引用计数、通知等待的邻居重新调度。持有 `ReentrantAreaLock m_schedulingLockArea`（coordinateShift=6，对齐 Moonrise getChunkSystemLockShift()；每 section 覆盖 64×64 区块，使 2*maxAccessRadius 锁只触达 1~4 个 section），保证 检查→建依赖→创建任务→完成→通知 全流程原子。
- **`SingleChunkLifecycleManager`**（NewChunkHolder 等价物）：持有**可变 `std::unique_ptr<ChunkPrimer> m_currentChunk`** + `m_currentGenStatus`（非不可变快照）。`getChunkIfPresentUnchecked(status)` 检查 `m_currentGenStatus.isAtLeast(status)` 后返回 `m_currentChunk`（同一累积 Primer）。`onChunkGenComplete(status)` 推进 `m_currentGenStatus`（primer 同一对象，无需重存）。
- **`ChunkProgressionTask`**：单状态推进任务。`execute` 从 `StaticChunkCache2D<ChunkPrimer*>` 构建 `WorldGenRegion`（转换为 `vector<IChunk*>`，所有邻居已由 `checkNeighbour` 保证达到所需状态），调用 `_executeStepTask`，推进 primer 状态，FULL 完成时 `_finalizeGeneratedChunkSync` 转 `ChunkData` 存入内存缓存。
- **`StaticChunkCache2D<T>`**（`StaticChunkCache2D.hpp`）：预分配二维缓存模板，构造时一次性填充 `(2*radius+1)²` 个条目，**不允许 nullptr**（loader 必须返回有效值）。替代旧的 `GenerationChunkCache`（增量填充、允许空洞），从根本上消除窗口内 nullptr。
- **`ReentrantAreaLock`**（`common/util/concurrent/`）：按区块坐标的可重入区域锁，`lock(x,z,radius)` 覆盖 `[x±radius, z±radius]`，同线程重入仅限完全覆盖的子区域。`schedule` 持 `maxAccessRadius` 锁，`onChunkGenComplete` 持 `maxAccessRadius` 锁；解锁后重调度时持 `2*maxAccessRadius` 锁。
- **`ChunkStep::getRequiredStatusAtRadius(radius)`**（`common/world/chunk/gen/ChunkStep.hpp`）：byRadius[] 查找表，对齐 Moonrise `ChunkStepMixin`。`schedule` 遍历 `[center±neighbourReadRadius]` 范围邻居，按 Chebyshev 距离查询每个邻居所需状态。
- **世界区域依赖**（`scheduling/AreaTaskScheduler`）：各坐标维护任务顺序，全部坐标就绪后才提交通用计算池；生成、加载光照、运行时光照和区块序列化共用一个实例，不同世界互不阻塞。

**并发模型**：邻居在任务执行期间可变（ChunkPrimer 累积式），但调度保证无并发写冲突——`checkNeighbour` 确保任务开始前所有邻居达所需状态，世界 `AreaTaskScheduler` 串行化重叠写区域（等价 Moonrise `AreaDependentQueue`）。这使 FEATURES/LIGHT 跨区块写邻居成为设计预期，而非 bug。

详见 `docs/BUG-WorldGenRegion-Access-Window.md`（问题根因）。

### 区块可用性的宣告必须与区块安装同时生效

`SingleChunkLifecycleManager` 对外宣告"该区块在某个状态可用"（`sourceState ∈ {LoadedFromStorage, Ready}` 或 `currentGenStatus` 达标）时，`m_currentChunk` 必须已经非空。存档命中路径统一走 `publishStorageLoaded(primer)`：在**同一临界区**内安装 primer、把 `currentGenStatus` 推进到 FULL（存档区块即完整区块）、把 `sourceState` 推进到 `LoadedFromStorage`；`noteStorageMissing()` 只负责"确认存档缺失"。任何"先推进状态、后安装区块"的写法都会让并发调度者（`schedule` 的空区块分支、`checkNeighbour` 的就绪判定、`buildNeighbourCache` 的 primer 取用）观察到"状态已就绪但区块缺失"，据此为该 holder 重复调度一次 EMPTY 加载，最终让依赖邻居取到空 primer 并在生成区域里崩溃。`markLoadedFromStorageReady` 与 `setCurrentChunk` 内均有不变量断言把关。

### 调度判据必须是一次原子读取

判断 holder 能否推进状态，需要同时知道"区块对象是否存在"与"存档来源是否已解析 / 生成状态是否达标"。这些字段虽在同一临界区内一并写入，但若**分多次加锁读取**，两次读取之间仍可能跨越一次状态发布，读出"区块为空 + 状态就绪"这一在真实状态中并不存在的组合。故 `ChunkTaskScheduler::schedule` 统一用 `SingleChunkLifecycleManager::generationSnapshot()` 一次取齐；`buildNeighbourCache` 对邻居用 `getChunkIfPresentUnchecked`（状态 ∧ 对象的合取）而非只比较状态。

### 区块卸载是事件驱动 + 保底批量（Moonrise 对齐）

`ServerChunkManager` 的卸载路径不再有周期性全量扫描，也没有秒级延迟观察期：

- **候选入队**：票级越过加载阈值（`shouldLoad()==false`）、生成任务设置/清除、邻居引用计数 0↔1、依赖图增删等**状态变更点**都会经 `SingleChunkLifecycleManager` 的卸载状态观察者回调触发 `_updateUnloadCandidate`，满足「票级越阈 + `isSafeToUnload()` + 无玩家追踪 + 无在途保存」即进入候选队列。原「每 20 tick 全量扫描 + 30 秒延迟」已删除。
- **每 tick 出队**：`_processChunkUnloads` 按保底批量 `max(50, 候选数×5%)` 取用（对齐 Moonrise `minChunkUnloadCount`/`minChunkUnloadFraction`）；该值是下限而非上限——小队列一次清空，大队列按保底量平滑推进。队列惰性移除：撤出候选只改成员集合，deque 中的残留键弹出时丢弃。
- **三阶段卸载**：stage1（主线程捕获脏区块快照并置位「卸载在途」标记）→ stage2（ServerIO 异步落盘）→ stage3（`_finalizeUnloadAfterSave` 复检 `shouldLoad()`，持调度锁 `cancelGeneration` + `isSafeToUnload` 复检后移除 holder）。无脏数据的区块同样入队延后一个 tick 收尾，给出「玩家在相邻 tick 返回即中止卸载」的反悔窗口。
- **失败退避**：stage3 发现仍不安全（邻居在用、依赖图未清）时注册 `UNLOAD_COOLDOWN` 票据把该区块按 Border 级钉住 100 tick，到期后票级升过阈值自然重评候选资格；不再每 tick 立即重试。
- **已删除**：`CHUNK_UNLOAD_DELAY_MS`（30 秒延迟）、`UNLOAD_CHECK_INTERVAL_TICKS`、`MAX_UNLOADS_PER_TICK`、以及此前不可启用的 `m_maxLoadedChunks` 软上限强制卸载（Moonrise 与原版均无全局已加载上限；限制内存应靠视距与加载/发送速率限制，而非强制卸载应当加载的区块）。

### 测试夹具必须注入存储线程池

`SingleLevelStorageManager` 在未注入 ServerIO/ServerCompute 池时会降级为**在调用线程内联**读写存档。卸载按保底批量推进（每 tick 至少 `max(50, 候选数×5%)` 个区块），同步写盘（紧凑数组快照捕获与 RocksDB 写入，压缩由数据库自身处理）会把 tick 线程垄断到秒级，进而饿死每 tick 只执行一次的 `_drainPendingLoadCompletes`，表现为大批 holder 长期停在 `ResolvingStorage`、生成请求迟迟不完成。使用 `ServerChunkManager` 的测试夹具须与生产（`MinecraftServer`）一样注入两个池，并注意池创建后必须 `start()`。

### 区块需求、结果交接与世界后处理分开推进

`processTicketUpdates()` 只推进距离图并调度加载，登录、移动与光照票据不能在该阶段接收大量存档结果。
`processPendingChunkTasks()` 在推进票据后接收入口处的一批完成项；处理期间新到达的结果留给下一次推进。
需要同步数据的调用方使用 `requestChunkSync()`，由目标 Future 决定何时返回，不能把票据生效等同于数据就绪。

存储层回传结果前已经补齐缺失高度图。`_onChunkLoadComplete` 保持区块对象与来源状态的原子发布，
`onChunkLoaded` / `m_chunkLoadedCallback` 则经后处理队列延后到 `tick()`，避免在发布调用链中修改世界。
完成交接仍保留重入守卫，禁止嵌套消费同一队列。

### 实体追踪器内存泄漏
实体销毁后未从追踪器取消追踪会导致泄漏。`ServerWorld::destroyEntity()` 会自动处理追踪器状态更新（内部调 `entityTracker().untrackEntity()`）。如果直接调用 `entityManager().destroyEntity()`，需要手动调用 `entityTracker().untrackEntity()`。

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
3. **`m_lightQueue.drainAndProcess(*this)`**（主线程入队 worker）：swap 出任务表，逐区块构造 `RuntimeLightTask` 提交到 世界 `AreaTaskScheduler`（writeRadius=2）。executor 为 nullptr 时 fallback 同步经 TLS 引擎调 `blocksChangedInChunk`。

引擎已无 `m_mutex`（③-2b）：`WorldLightManager` 改 `thread_local` 引擎池（`acquireSkyLightEngine`/`acquireBlockLightEngine`），每个 worker 线程独占一套引擎，无引擎级锁。所有光照写操作（区块加载光照、运行时方块变更、LIGHT 生成阶段）统一经同一 世界 `AreaTaskScheduler`（writeRadius=2），重叠 5×5 区域的 nibble 写必被区域锁串行 → 满足 `SWMRNibbleArray` 更新侧非原子单写者语义。

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

### 保存必须发生在 ServerChunkManager::shutdown() 之前
`ServerChunkManager::shutdown()` 会直接 `m_chunks.clear()`，之后世界里的任何修改都不再可能落盘。所以**关服落盘一律发生在它之前**：`MinecraftServer::shutdownManagers()` 先调 `saveAllWorldData(true)`，`ServerWorld::shutdown()` 再保存本维度实体，最后才轮到区块管理器关闭。任何"先关区块管理器再保存"的改动都会静默丢掉玩家最后一段时间的建造。

### 区块保存由区块层驱动，存储层没有数据
存储层不再缓存任何段数据，`SingleLevelStorageManager::saveChunks()` 从调用方提供的区块直接捕获紧凑数组。要保存什么必须由区块层决定：`ServerWorld::saveDirtyChunks(sync)` 遍历内存区块表中 `isDirty()` 且已完全生成的区块，将区块列表交给存储门面，**把所有脏区块的段攒进同一个批次**一次性提交。不要在存储层重新引入段缓存来"让保存有数据源"——读写路径访问同一批段的概率极低，缓存只会常驻内存而不命中。

### 共享存储下的保存由 MinecraftServer 统一执行
三个 `ServerWorld` 共享存储，各自保存自己的区块不会重复（区块按维度归属），但玩家数据与 level.dat 只应落盘一次，因此统一收口在 `MinecraftServer::saveAllWorldData()`；`ServerWorld::shutdown()` 只额外落盘**本维度自己**的实体。

### 关服实体落盘必须合成一个批次
`ServerWorld::shutdown()` 遍历全部已加载区块，把每个区块的"整段删除旧行 + 写存活实体"收集成 `ChunkEntityWrite`，**一次性**交给 `EntityStorageManager::replaceEntitiesInChunks` 提交。绝不能逐区块各提交一次：那是区块数量次 WAL fsync，视野距离 16 下 1089 个区块实测约 3.4 秒，而其中绝大多数区块根本没有实体、整段删除是空操作。`forEachLoadedChunk` 的结果条数必须与落盘条目数一一对应（有断言把关）——漏掉任一区块，它就不再是"清理 + 重写"的收敛状态，残留在盘上的旧行会在下次加载时复活成同 UUID 的重复实体。

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
- **保存**：`MinecraftServer::saveAllWorldData()`（服务器级全量保存入口）中，对末地维度的 `EndDragonFight` 调用 `SingleLevelStorageManager::saveDragonFightData()` 持久化战斗状态到 `data/end_dragon_fight.json`。末影龙战斗状态是世界级数据，落盘归属服务器级入口而非 `ServerWorld` 自身（原 `ServerWorld::saveAll` 越权方法已删除）
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

### chunkManager() 区块访问

`IWorld::chunkManager()` 返回 common 层窄接口 `world::chunk::IChunkManager`（`tryToGetChunkInMem` / `hasChunkInMem` / `requestFullChunkSync`）。`ServerWorld` 借 C++ **协变返回类型**把它覆写为返回具体的 `ServerChunkManager*`，因此服务端代码拿到的是完整管理器 API，common 层代码只看到那 3 个查询/加载入口——`ServerWorld` 自身不再需要 `getChunk`/`hasChunk`/`getOrLoadChunk` 这类逐操作转发方法。

**线程安全**：`requestFullChunkSync` 与 `ServerChunkManager::requestFullChunkSync` 相同，仅在服务端主线程调用安全（内部通过 `_drainPendingLoadCompletes` 泵送避免死锁）。

**使用场景**：`EndGatewayEntity::_generateExitPortal` 在末地外岛扫描区块判空时调用 `world.chunkManager()->requestFullChunkSync(...)`，完整复刻 MC Java 的 `TheEndGatewayBlockEntity.findExitPortalXZPosTentative` 行为。其他 common 层代码需要按需加载区块时也走同一入口，而非直接依赖 `ServerChunkManager` 类型（common 层无法依赖 server 层）。

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
- 对应的客户端接收路径：`ClientPlayVisitor` 处理 `ir::play::BlockEntityData` → `ClientWorld::onBlockEntityData()` → `BlockEntity::loadFromNBT()`。

### 隔导体的比较器通知

模拟输出方块发生状态变化、放置或移除时，除普通邻居通知外，还须通知隔一块红石导体的比较器。只通知紧邻方块会使容器输出变化后比较器保留旧值。

存档读取损坏或 I/O 错误必须完成请求为失败，不能进入缺失区块的生成与覆盖保存路径。
