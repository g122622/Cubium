# server 模块

## 目录结构树

```text
server/
├── access/  # 准入名单与权限门面
├── advancement/  # 该领域的子模块
├── application/  # 服务端生命周期与领域装配
├── bossbar/  # 该领域的子模块
├── command/  # 服务端命令与命令源
├── core/  # 方块身份、状态容器与行为扩展入口
├── dimension/  # 多维度运行时管理
├── event/  # 服务端领域事件
├── function/  # 该领域的子模块
├── interaction/  # 玩家物品、挖掘与容器交互
├── item/  # 该领域的子模块
├── menu/  # 该领域的子模块
├── mod/  # 该领域的子模块
├── network/  # 连接会话与网络同步
├── physics/  # 该领域的子模块
├── player/  # 玩家实体、记录、移动和模式
├── registry/  # 注册表启动与数据加载
├── scoreboard/  # 该领域的子模块
├── settings/  # 服务端设置
├── stats/  # 该领域的子模块
├── test/  # 该领域的子模块
├── world/  # 权威世界模拟、区块、生成与存储
├── CMakeLists.txt  # 模块说明与构建配置
├── Makefile  # 模块说明与构建配置
├── cmake_install.cmake  # 模块说明与构建配置
└── main.cpp  # 模块接口与实现
```

## 内部模块关系

各文件承接目录树中标明的单一职责，通过同领域接口组合。独占的辅助实现位于使用者目录下。

## 上下游外部依赖关系

公共层提供领域类型与接口；服务端通过应用装配连接玩家、网络和世界实现。源文件的构建归属与领域目录一致。

## 容易踩的坑

- 目录按领域职责组织；跨领域协作通过明确入口和基础类型。
- 新增或迁移源文件时同步更新所有消费目标的 CMake 清单。
- 服务端实现被拆成三个共享编译单元，加文件前先判断该放哪：`mc_server_storage`（`world/storage/`）、`mc_gen`（`world/gen/` 等 worldgen 实现）、`mc_server_shared`（其余被 `minecraft-server` 与 `mc_tests` 共用的实现）。把文件同时写进 `minecraft-server` 与 `mc_tests` 会重复编译并让 `build/` 体积翻倍，正确做法是加进 `mc_server_shared` 的清单。
- `application/console/ui/**`、`main.cpp`、`ServerApplicationEntry.cpp`、`StandaloneServer.cpp` 留在可执行文件内部，不能进 `mc_server_shared`：该库被 `mc_tests` 链接，而 `mc_tests` 不链接 FTXUI。
- 共享库一律用 `OBJECT` 而不是 `STATIC`：服务端有依赖静态初始化顺序的注册表装配，静态库下未被引用的 `.o` 会被链接器丢弃，行为会静默改变。

### 1. 线程安全

- `PlayerManager` 的公共方法都是线程安全的
- `ChunkWorkerPool` 在 Worker 线程执行区块生成
- 回调到主线程时需使用 `processPendingSends()` 或类似的同步机制
- `ChunkSendManager::submitChunkData()` 是线程安全的

### 2. 区块同步顺序

区块发送依赖 `ChunkLoadTicketManager` 的追踪回调：
1. 玩家移动触发 `PlayerChunkTracker` 更新
2. `TrackingChangeCallback` 通知 `ChunkSendManager`
3. 区块加载完成后自动发送给追踪玩家

**注意**：不要手动调用 `ChunkSendManager::sendChunkToPlayers()`，应使用票据系统。

### 3. 光照初始化

区块加载后光照由 `ChunkLoadLightTask` 在 worker 线程异步完成（效仿 Moonrise `ThreadedLevelLightEngine`）：
```cpp
// 正确方式：ChunkLoadedCallback 入队区块加载光照任务
chunkManager.setChunkLoadedCallback([world](ChunkCoord x, ChunkCoord z) {
    world->enqueueChunkLoadLight(x, z);
});
```
`enqueueChunkLoadLight` 对中心区块 add LIGHT 票据保活后，构造 `ChunkLoadLightTask` 提交到
`UniversalWorkerPool` 区域互斥池（writeRadius=2）。worker 完成后经 `ServerWorld` 续延队列回主线程
flush（`markLightChanged` 同步 visible nibble 到 ChunkSection）+ send（`ChunkSendManager` 发包）。
tick 顺序严格 flush → send → drain，保证客户端收到正确光照而非全黑区块。

### 4. Manager 初始化顺序

`MinecraftServer` 的初始化顺序：
1. `initializeRegistries()` - 游戏注册表（方块、物品、附魔、战利品表、配方），从 `DataPackList` 加载数据
2. `initializeCoreManagers()` - 核心 Manager
3. `initializeWorld()` - 世界和区块管理器
4. `initializeInteractionManagers()` - 交互 Manager
5. `ServerDimension::initialize()` - 每个维度独立创建同步管理器和刷怪管理器（替代原 `initializeSyncManagers()` + `initializeChunkSyncManagers()`）
6. `setupWorldCallbacks()` - 设置回调（遍历所有维度）

### 5. 维度感知的世界访问

- `MinecraftServer::m_world` 已移除。世界访问必须通过 `ServerDimensionManager` / `ServerDimension` / `getPlayerWorld(PlayerId)` 进行。
- 同步管理器（ChunkSendManager、BlockUpdateSyncManager；实体同步由 `EntityTracker` + `ServerWorld` 广播回调承担，光照同步由 `ServerWorld` 承担，均无独立管理器）和刷怪管理器（NaturalSpawner、DespawnManager、VillageSiege）现在由各 `ServerDimension` 持有，不再从 `MinecraftServer` 访问。
- 所有维度的 `ServerWorld::tick()` 都通过 `ServerDimensionManager::tick()` 统一调度，每个维度在自己的 `ServerDimension::tick()` 中独立执行同步和刷怪逻辑。

### 6. 命令注册

命令在 `CommandRegistry::registerDefaults()` 中自动注册。自定义命令需手动注册：
```cpp
server.commandRegistry().dispatcher().registerCommand(
    mc::command::literal("mycommand")
        .executes([](auto& context) {
            // 命令逻辑
            return 1;
        })
);
```
