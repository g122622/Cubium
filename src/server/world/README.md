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
在未调用 `initialize()` 的 `ServerWorld` 上调用 `setBlockState()` 会在光照更新阶段触发断言。**所有测试必须先初始化世界**。

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
