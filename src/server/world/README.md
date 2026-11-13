# world 模块

## 目录结构树

```text
world/
├── blockentity/  # 该领域的子模块
├── blockevent/  # 该领域的子模块
├── chunk/  # 区块运行时、生命周期能力与唯一门面
├── drop/  # 该领域的子模块
├── entity/  # 该领域的子模块
├── gameevent/  # 该领域的子模块
├── gen/  # 该领域的子模块
├── lighting/  # 光照引擎、查询与运行时任务
├── player/  # 玩家实体、运行数据与生命周期
├── scheduling/  # 世界持有的区域任务执行设施
├── spawn/  # 该领域的子模块
├── storage/  # 该领域的子模块
├── structure/  # 该领域的子模块
├── time/  # 共享时间规则与时间线；服务端目录负责运行时推进
├── weather/  # 该领域的子模块
├── ServerWorld.cpp  # 模块接口与实现
└── ServerWorld.hpp  # 服务端世界配置结构
```

## 内部模块关系

ServerWorld 编排世界模拟并持有共享区域执行器；chunk 承接区块需求和生命周期，lighting 承接光照传播，gen 提供生成算法，storage 提供持久化。生成、光照和序列化复用同一世界的区域执行器。

## 上下游外部依赖关系

公共层提供领域类型与接口；服务端通过应用装配连接玩家、网络和世界实现。源文件的构建归属与领域目录一致。

## 容易踩的坑

- 目录按领域职责组织；跨领域协作通过明确入口和基础类型。
- 新增或迁移源文件时同步更新所有消费目标的 CMake 清单。

- 每个生成步骤的 WorldGenRegion 必须绑定生成器种子，不能依赖 ServerWorld 是否存在；独立区块管理器同样需要以正确种子进行群系边界抖动和放置过滤。

### 生成需求撤销与任务完成

票据降级要重新汇总目标，不能用“安全可卸载”作为取消需求的前提。显式请求按各自目标交付，
共享邻居仅在最后一条有效依赖移除后停止推进。排队任务可发取消信号，但任务身份和邻居引用必须
保留到执行器确认完成；已开始的生成阶段完整执行，再根据最新需求决定是否继续。延迟重调度只
保存坐标，不能保存旧目标，也不能在旧任务尚未结束时复位取消令牌。
区域调度器保留任务体至回调结束，完成、取消和失败回调在任务仍存活时核对身份；
取消请求只能置位令牌，禁止提前清除任务指针或释放邻居引用。

### 依赖扫描与取消后的交付

邻居从首次依赖扫描起保活，包括已经就绪的邻居；等待其余依赖期间不可卸载并重建它们。
按实际登记集合释放保活，不能在完成时按坐标重新推算邻居。完成、取消、失败和需求撤销均须结清。
区块已达到某请求的阶段时应立即交付，即使更高阶段仍有任务；取消确认后的重调度也必须交付
已经就绪的请求，否则 worker 与区域任务均空闲时仍会遗留 future。生成失败须向所有上游请求传播。

### 卸载冷却与距离图回调

卸载收尾发现持有者仍被引用时，以最弱生成级别注册冷却票据。冷却存在期间保留持有者，到期后恢复回收。生成邻域和退场回调只更新已有持有者，邻居依赖由调度器按需创建，避免冷却传播重新引发生成与卸载循环。

### 区块发布与世界接入

区块门面只承接业务操作，内部由生命周期控制器编排需求、来源、生成和卸载。注册表统一发布与移除；来源与 primer 一并安装，存档命中进入 Loaded 状态。加载错误不能重新生成覆盖存档。

阶段请求按各自目标交付；FULL 数据存在不代表实体接入、后处理或光照发送已经完成。世界接入在主线程按预算消费激活消息，消息必须校验原 holder 身份。生成后处理等待模拟资格；同一生命周期至多注册一次。

保存失败恢复脏状态并冷却，保存后变脏或恢复需求则中止退场。退场与请求登记共用调度锁。存储在途表由存储自身结清，完成接收端在区块运行时关闭后仍能安全接收并丢弃迟到回调。

测试夹具若需异步存储，必须显式注入并启动 IO 与 Compute 池；无池时读写内联执行。

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

- 邻居依赖采用连续边表并按指针去重，通知时在锁内复制紧凑快照；任务结束取出保活边时转移整个数组，不保留历史哈希桶。双向解除关系和邻居引用计数的顺序不能改变。
