# Server Application Module

## Directory Structure

```
src/server/application/
├── IServer.hpp              # Server interface definition
├── MinecraftServer.hpp      # Abstract base class declaration
├── MinecraftServer.cpp      # Abstract base class implementation
├── IntegratedServer.hpp     # Integrated server (single-player) declaration
├── IntegratedServer.cpp     # Integrated server (single-player) implementation
├── StandaloneServer.hpp     # Standalone server (multi-player) declaration
└── StandaloneServer.cpp     # Standalone server (multi-player) implementation
```

## File Descriptions

### IServer.hpp

**Responsibility:** Defines the unified interface for all server types.

**Key Components:**

- `IServer` - Pure abstract interface class that provides:
  - Lifecycle management: `initialize()`, `shutdown()`, `tick()`, `isRunning()`
  - Core managers access: `playerManager()`, `connectionManager()`, `timeManager()`, `teleportManager()`, `keepAliveManager()`, `positionTracker()`, `packetHandler()`, `gameModeManager()`
  - Permission managers access: `whitelistManager()`, `bannedPlayerList()`, `bannedIpList()`, `opListManager()`
  - Dimension-aware world access: `dimensionManager()`, `getPlayerWorld(PlayerId)`
  - Interaction managers access: `blockInteractionManager()`, `miningManager()`, `containerManager()`, `inventoryManager()`
    - Player inventory access: `playerInventory(playerId)`
  - Command system access: `commandRegistry()`
  - Scoreboard system access: `scoreboard()`
  - Configuration access: `viewDistance()`, `maxPlayers()`, `seed()`, `currentTick()`

  **注意**: `m_world`、单世界访问器（`world()`、`chunkManager()`、`lightManager()`、`entityManager()`、`entityTracker()`、`physicsEngine()`、`weatherManager()`、`itemPickupManager()`）以及同步管理器访问器（`entitySyncManager()`、`chunkSendManager()`、`lightSyncManager()`、`blockUpdateSyncManager()`）已从 IServer 移除。同步管理器和刷怪管理器现在由 `ServerDimension` 持有，通过 `dimensionManager()` 获取维度后访问。

**Design Pattern:** Interface Segregation - provides clean abstraction for server types without exposing implementation details.

---

### MinecraftServer.hpp / MinecraftServer.cpp

**Responsibility:** Abstract base class providing shared implementation for all server types.

**Key Components:**

- Inherits from `IServer`
- Holds all core managers as `unique_ptr` members
- Implements `tick()` main loop framework
- Provides packet dispatching and handling
- Attaches the world sound callback after world creation so entity sounds can be broadcast through the server helpers
- Owns the server-side computation worker pool `m_computationWorkerPool` for chunk generation and other compute-heavy tasks

**Protected Methods (for subclasses):**

- `initializeCoreManagers()` - Creates PlayerManager, ConnectionManager, TimeManager, etc.
- `initializeWorld()` - Initializes world and command registry
- `initializeInteractionManagers()` - Creates BlockInteractionManager, MiningManager, etc.
- `initializeSyncManagers()` - 已移除；同步管理器现在在 `ServerDimension::initialize()` 中创建
- `initializeChunkSyncManagers()` - 已移除；区块同步管理器现在在 `ServerDimension::initialize()` 中创建
- `initializeRegistries()` - Loads vanilla blocks, items, enchantments, recipes
- `setupWorldCallbacks()` - Sets up chunk loading, block change, entity spawning, and light change callbacks
- `shutdownManagers()` - Cleanup in correct order

**Pure Virtual Methods (must be implemented by subclasses):**

- `pollNetwork()` - Network event polling (LocalConnection vs TCP)
- `broadcastPacket()` - Broadcast to all players
- `getPlayerIdForSession()` - Map session ID to player ID
- `sendPacketToPlayer()` - Send to specific player
- `handleLoginRequestPacket()` - Login handling
- `handleHotbarSelectPacket()` - Hotbar selection handling
- `handleContainerClickPacket()` - Container interaction handling
- `handleCloseContainerPacket()` - Container close handling

**Shared Base-Class Handlers / Hooks:**

- `handleBlockPlacementPacket()` - Shared placement / block-use flow in `MinecraftServer`, subclasses only provide inventory/container hooks
- `handleCreativeInventoryActionPacket()` - Shared creative inventory slot write-back flow in `MinecraftServer`
- `broadcastLightUpdate()` - Shared LightUpdate packet serialization and broadcast path in `MinecraftServer`
- `setupChunkSendCallback()` - Shared chunk send/unload callback installation in `MinecraftServer`
- `setupRaidManagerCallbacks()` - Shared raid event callback installation in `MinecraftServer`
- `getHeldItemForPlacement()` / `getSelectedHotbarSlot()` / `setInventoryItem()` / `syncPlayerInventory()` / `tryOpenCraftingContainer()` - subclass hooks for local-vs-remote inventory/container differences

**Tick Phases:**

1. `m_timeManager->tick()` - 更新时间，推进全局 tick 和 day time
2. 清理断开连接玩家 - 回收玩家追踪与区块票据
3. `m_dimensionManager->tick()` - 统一驱动所有维度；每个 `ServerDimension::tick()` 内部依次执行：
   - `ServerWorld::tick()` - 世界 tick（区块管理器、光照、天气、实体、物理等）
   - `entitySyncManager()->tick()` - 同步该维度实体位置
   - `chunkSendManager()->processPendingSends()` - 发送该维度待发区块
   - `blockUpdateSyncManager()->flushPendingUpdates()` - 发送该维度待处理方块更新
   - `naturalSpawner()->tick()` - 该维度自然刷怪（仅主世界和下界有 hostile 刷怪，仅主世界有 passive 刷怪）
   - `despawnManager()->tick()` - 该维度生物消失检查
4. `sharedStorage()->tickAutoSave(currentTick())` - 在服务器级单点驱动共享存储 autosave，避免 3 个维度重复驱动同一份 `SingleLevelStorageManager`
5. `tickEntities()` - 主世界实体 tick、物品拾取、实体追踪
6. `miningManager().tick()` - 更新挖掘进度（遍历所有维度）
7. `pollNetwork()` - 处理网络事件（子类实现）
8. `tickKeepAlive()` - 发送心跳并配合超时检查

**注意**: `m_world` 成员已移除。原由 `m_world` 提供的快捷访问（刷怪、同步等）已下沉到 `ServerDimension`。原 `MinecraftServer::tick()` 中的 `m_naturalSpawner->tick()`、`m_despawnManager->tick()`、`entitySyncManager().tick()`、`chunkSendManager().processPendingSends()`、`blockUpdateSyncManager().flushPendingUpdates()` 调用均已移入 `ServerDimension::tick()`，由维度各自执行。

---

### IntegratedServer.hpp / IntegratedServer.cpp

**Responsibility:** Single-player server implementation with dedicated thread and LocalConnection.

**Key Features:**

- Runs in a separate thread from the client
- Uses `LocalConnectionPair` for intra-process communication
- Single player only (`maxPlayers = 1`)
- Direct inventory management (`m_clientInventory`)
- `playerInventory()` overrides the shared interface so the local player resolves to `m_clientInventory`, while other ids still use the normal inventory manager
- Container menu handling (`m_openMenu`, with the integrated-server menu player bound to the active menu for shared packet handling)
- Creative mode login uses the shared creative inventory helper to populate the local player inventory before the screen opens
- Creative inventory slot edits are handled through `CreativeInventoryActionPacket` and then mirrored back to the client inventory sync path
- Right-click crafting-table / chest / furnace-family interaction is routed through the shared menu factory:
  - empty hand / non-block item opens `CraftingMenu`
  - chest and furnace menus are created from the same `ContainerManager` / `AbstractContainerMenu` path as the client sync packets
- Generic right-click block activation is routed to `BlockInteractionManager::handleBlockUse()` when placement path does not apply
- The world sound callback is attached during initialization so local entity sounds reach the client through the same server broadcast path
- World type routing:
  - 由 `ServerDimensionManager` 统一装配主世界 `ServerWorld`
  - `IntegratedServer` 只提供 `WorldType` 配置，不再直接创建主世界 `NoiseChunkGenerator`

**Configuration (`IntegratedServerParams`):**

```cpp
struct IntegratedServerParams {
    std::string worldName;
    std::string gameDirectoryRoot;
    i64 seed;
    GameMode defaultGameMode;
    i32 viewDistance;
    i32 tickRate;
    WorldType worldType;
};
```

**Key Methods:**

- `initialize(config)` - Start server thread with given config
- `stop()` - Stop server thread gracefully
- `getClientEndpoint()` - Get `LocalEndpoint` for client to connect
- `clientInventory()` - Access the single player's inventory
- `playerInventory(playerId)` - Return the local inventory for the single-player client and fall back to shared inventory lookups for everyone else

**Thread Model:**

- Dedicated `m_serverThread` running `mainLoop()`
- Mutex `m_clientDataMutex` for thread-safe inventory access
- Tick timing: `1000 / tickRate` milliseconds per tick

---

### StandaloneServer.hpp / StandaloneServer.cpp

**Responsibility:** Multi-player server with TCP networking.

**Key Features:**

- TCP-based networking via `TcpServer`
- Multiple players support
- Settings file persistence (`ServerSettings`)
- Command-line parameter overrides
- Perfetto tracing integration
- `ContainerManager` callbacks are forwarded to client protocol packets (`OpenContainer`, `CloseContainer`, `ContainerContent`)
- Shared container menu creation covers crafting tables, chests, furnaces, blast furnaces, and smokers through the same open-container path
- Creative inventory login and slot writes use the same shared creative inventory helper and `CreativeInventoryActionPacket` path as the integrated server
- Non-placement right-click interaction path now routes through `BlockInteractionManager::handleBlockUse()` for block activation
- The world sound callback is attached during initialization so mob/player sounds are broadcast the same way as other server events
- Full world type support:
  - 主世界生成模式通过 `ServerDimensionManager::initialize(..., overworldType)` 统一下发
  - `StandaloneServer` 不再直接装配主世界 chunk pipeline

**Configuration (`StandaloneServerParams`):**

```cpp
struct StandaloneServerParams {
    std::optional<std::string> configPath;
};
```

**Key Methods:**

- `initialize(params)` - Initialize with command-line parameters
- `run()` - Block on main loop
- `stop()` - Graceful shutdown
- `settings()` - Access `ServerSettings` for runtime configuration

**Network Events:**

- `onClientConnect()` - New TCP session
- `onClientDisconnect()` - Session closed
- Packet routing via session ID

---

## File Relationships

```
                    IServer.hpp
                         ^
                         |
                    MinecraftServer.hpp/cpp
                    /                  \
                   /                    \
    IntegratedServer.hpp/cpp    StandaloneServer.hpp/cpp
```

**Dependency Flow:**

1. `IServer` defines the contract
2. `MinecraftServer` implements shared logic, delegates network to subclasses
3. `IntegratedServer` uses `LocalConnectionPair` for single-player
4. `StandaloneServer` uses `TcpServer` for multi-player

---

## Module Overview

### Overall Responsibility

The `application` module serves as the **entry point and orchestrator** for the Minecraft server. It:

1. **Initializes** all server subsystems (managers, world, network)
2. **Coordinates** tick execution across all components
3. **Routes** network packets to appropriate handlers
4. **Manages** server lifecycle (startup, shutdown)
5. **Provides** unified access to all server components

### 线程池职责划分

- **计算线程池**：`MinecraftServer::m_computationWorkerPool`
    - 用于区块生成、区块状态推进等计算型异步任务
- **存储 IO 线程池**：`MinecraftServer::m_ioWorkerPool`
    - 由 `MinecraftServer` 持有，再注入到 `SingleLevelStorageManager`
    - 用于 Section 读写、刷新与其他持久化任务
- **模块专属线程池**：客户端网格构建、资源加载等仍按各自模块管理

### Input and Output

| Direction  | Component       | Description                                                              |
| ---------- | --------------- | ------------------------------------------------------------------------ |
| **Input**  | Network packets | Player movement, block interactions, chat, login                         |
| **Input**  | Configuration   | `ServerSettings` / `IntegratedServerParams` / `StandaloneServerParams` |
| **Input**  | World data      | Chunk requests, entity spawning                                          |
| **Output** | Network packets | Chunk data, entity updates, teleport, game state                         |
| **Output** | World changes   | Block modifications, entity spawning/removal                             |
| **Output** | Player state    | Position updates, inventory sync                                         |

### Dependencies

**Internal Dependencies:**

```
server/application/
├── server/core/           # PlayerManager, ConnectionManager, TimeManager, etc.
├── server/interaction/    # BlockInteractionManager, MiningManager, etc.
├── server/dimension/      # ServerDimension, ServerDimensionManager
├── server/sync/           # EntitySyncManager, ChunkSendManager, LightSyncManager (由 ServerDimension 持有)
├── server/world/          # ServerWorld, ServerChunkManager, WeatherManager
├── server/network/        # TcpServer, TcpSession
├── server/command/        # CommandRegistry
├── server/menu/           # CraftingMenu
├── common/entity/inventory/container/  # Chest/Furnace/Hopper 菜单实现
├── common/network/        # Packets, LocalConnection
├── common/world/          # World, Chunk, Lighting, Generation
├── common/entity/         # Player, Inventory, Loot
├── common/item/           # Items, BlockItems, Recipes
├── common/physics/        # PhysicsEngine
└── common/perfetto/       # Tracing
```

**External Dependencies:**

- `spdlog` - Logging
- `std::thread` - Threading
- `std::atomic` - Thread-safe flags

## 文件介绍补充

### `MinecraftServer.hpp/cpp`

- 新增 `m_computationWorkerPool`
- 在子类创建 `ServerChunkManager` 后调用 `setWorkerPool(&m_computationWorkerPool)` 注入计算池
- 在 `stopCore()` 中统一停止计算池
- `bindWorldIoWorkerPool()` 负责将 IO Worker Pool 注入到 `SingleLevelStorageManager`
- `MinecraftServer` 不再直接使用 `WorldStoragePaths` 解析存档目录
- 世界目录选择与打开改由 `GlobalStorageManager::openLevel()` 承担
- 共享 `SingleLevelStorageManager` 的保存与关闭职责固定在 `MinecraftServer::shutdownManagers()`：先显式停止 autosave，再按世界模式决定是否执行一次全量保存，最后调用 `close()` 释放资源，避免 3 个维度 world 各自重复落盘或由析构隐式保存
- `MinecraftServer` 不再持有 `m_world` 成员。原 `m_world` 提供的主世界快捷引用和同步管理器访问已移至 `ServerDimension`。世界 tick 调度由 `m_dimensionManager->tick()` 统一驱动，同步管理器和刷怪管理器在各维度的 `ServerDimension::tick()` 中独立执行。
- `MinecraftServer` 不再持有同步管理器（`m_entitySyncManager`、`m_chunkSendManager`、`m_blockUpdateSyncManager`、`m_lightSyncManager`），这些管理器现在由各 `ServerDimension` 实例各自持有。
- `MinecraftServer` 不再持有刷怪管理器（`m_naturalSpawner`、`m_despawnManager`），这些管理器现在由各 `ServerDimension` 实例各自持有。
- `IServer` 接口新增 `getPlayerWorld(PlayerId)` 方法用于维度感知的世界访问，替代原有的 `world()` 单世界访问器。
- `IServer` 提供 `sharedStorage()` 作为跨维度共享存储入口，`save-*` 等命令不再通过主世界 `ServerWorld` 绕行。

## 模块关系

```mermaid
flowchart TD
    A[MinecraftServer] --> B[m_computationWorkerPool]
    A --> DM[ServerDimensionManager]
    DM --> OW[ServerDimension 主世界]
    DM --> NT[ServerDimension 下界]
    DM --> ED[ServerDimension 末地]
    OW --> C[ServerWorld]
    NT --> C2[ServerWorld]
    ED --> C3[ServerWorld]
    C --> D[ServerChunkManager]
    A --> E[GlobalStorageManager]
    A --> H[SingleLevelStorageManager]
    C --> H
    H --> F[StorageTaskManager]
    F --> G[Storage IO WorkerPool]
```

## 容易踩的坑

- 计算池和 IO 池职责不同，不要复用同一个 `ServerWorkerPool`。
- `ServerChunkManager` 现在只接受外部注入的池指针，不能再假设自己拥有生命周期。
- `SingleLevelStorageManager` 的异步任务需要在 `open()` 之后才可使用。
- 生命周期规则现在要求：析构函数只做”兜底释放”，不负责隐式全量保存或带网络副作用的关闭逻辑；共享资源的保存/关闭必须由 `MinecraftServer` 顶层统一编排。
- `m_world` 已从 `MinecraftServer` 移除。所有世界访问必须通过 `ServerDimensionManager` / `ServerDimension` / `getPlayerWorld(PlayerId)` 进行。
- 同步管理器（EntitySyncManager、ChunkSendManager、BlockUpdateSyncManager、LightSyncManager）和刷怪管理器（NaturalSpawner、DespawnManager）现在由各 `ServerDimension` 持有，不再从 `MinecraftServer` 访问。

## 测试用例

- `tests/server/test_chunk_worker_pool.cpp`
- `tests/server/test_server_chunk_manager.cpp`
- `tests/common/world/storage/StorageTaskTest.cpp`

---

## Usage

### Integrated Server (Single-Player)

```cpp
#include "server/application/IntegratedServer.hpp"

// Create and configure
mc::server::IntegratedServerConfig config;
config.worldName = "my_world";
config.seed = 12345;
config.viewDistance = 10;
config.defaultGameMode = mc::GameMode::Survival;
config.tickRate = 20;

// Initialize
mc::server::IntegratedServer server;
auto result = server.initialize(config);
if (result.failed()) {
    // Handle error
}

// Get client endpoint for client connection
mc::network::LocalEndpoint* endpoint = server.getClientEndpoint();

// Run game loop (client side)
while (running) {
    // Send packets via endpoint->send()
    // Receive packets via endpoint->receive()
    // ...
}

// Shutdown
server.stop();
```

### 声音广播链路

实体调用 `Entity::playSound(...)` 后，会先进入 `ServerWorld::playSound(...)`，再由 `MinecraftServer::broadcastSound(...)` 或其子类实现把数据包发给附近玩家。这个路径现在同时服务于 `LivingEntity` 的受伤/死亡声、`MobEntity` 的环境声，以及 `Player` 的受伤/死亡声。

### 实体状态广播链路

实体状态事件（如守卫者攻击动画）通过 `IWorld::broadcastEntityStatus()` 接口广播：

```
GuardianAttackGoal::startExecuting()
  → world()->broadcastEntityStatus(entityId, status)
  → ServerWorld::broadcastEntityStatus()
  → m_onBroadcastEntityStatus callback
  → MinecraftServer::broadcastEntityStatusInRange()
  → 发送 EntityStatusPacket 给范围内玩家
```

客户端收到 `EntityStatusPacket` 后，根据状态码触发相应的动画或音效（如 `GuardianSoundStateful`）。

### 命令执行回调链路

命令方块矿车等实体执行命令通过 `IWorld::executeCommand()` 接口委托给服务器：

```
CommandBlockMinecartEntity::executeCommand()
  → world()->executeCommand(command, position, 2)
  → ServerWorld::executeCommand()
  → m_onExecuteCommand callback
  → IntegratedServer::命令执行回调
  → CommandRegistry::execute()
  → 返回命令结果
```

**回调初始化**（在 `IntegratedServer::initialize()` 中）：

```cpp
// 通过维度管理器获取主世界设置回调
auto* overworld = m_dimensionManager->getOverworld();
auto* world = overworld->world();
world->setOnExecuteCommand([this](const std::string& command,
                                     const Vector3d& position,
                                     i32 permissionLevel) -> i32 {
    // 自动添加 '/' 前缀（如果缺失）
    std::string cmd = command;
    if (!cmd.empty() && cmd[0] != '/') {
        cmd = "/" + cmd;
    }

    // 创建命令源（使用命令方块矿车的权限级别 2）
    command::ServerCommandSource source(this,
        nullptr, world, position, Vector2f(0.0f, 0.0f),
        permissionLevel, 0, "@");

    // 执行命令
    auto result = m_commandRegistry->execute(cmd, source);
    if (result.failed()) {
        spdlog::debug("Command execution failed for '{}': {}", cmd, result.error().message());
        return 0;
    }
    return result.value();
});
```

**权限级别说明**：
- 命令方块矿车使用权限级别 2（对应 OP 级别 2）
- 命令源名称使用 "@" 表示命令方块
- 命令执行位置使用实体的世界坐标

## 测试用例

- [tests/server/ServerWorldTest.cpp](../../../tests/server/ServerWorldTest.cpp) 覆盖世界声音回调转发。
- [tests/server/world/ServerWorldCommandExecuteTest.cpp](../../../tests/server/world/ServerWorldCommandExecuteTest.cpp) 覆盖命令执行回调机制。

### Standalone Server (Multi-Player)

```cpp
#include "server/application/StandaloneServer.hpp"

// Create with parameters
mc::server::StandaloneServerParams params;
params.port = 25565;
params.maxPlayers = 20;
params.worldName = "world";
params.seed = 12345;

// Initialize
mc::server::StandaloneServer server;
auto result = server.initialize(params);
if (result.failed()) {
    // Handle error
}

// Run blocking main loop
result = server.run();

// Or run in separate thread
std::thread serverThread([&server]() {
    server.run();
});

// Later...
server.stop();
if (serverThread.joinable()) {
    serverThread.join();
}
```

### Accessing Server Components

```cpp
// Through IServer interface
mc::server::IServer& server = getServer();

// Core managers
auto& playerManager = server.playerManager();
auto& timeManager = server.timeManager();
auto& teleportManager = server.teleportManager();

// Dimension-aware world access (replaces server.world())
auto* playerWorld = server.getPlayerWorld(playerId);

// Access dimension-specific managers through ServerDimension
auto& dimensionManager = server.dimensionManager();
auto* overworld = dimensionManager.getOverworld();
auto* chunkSendMgr = overworld->chunkSendManager();
auto* entitySyncMgr = overworld->entitySyncManager();
auto* naturalSpawner = overworld->naturalSpawner();

// Configuration
i32 viewDistance = server.viewDistance();
u64 currentTick = server.currentTick();
```

---

## Common Pitfalls

### 1. Thread Safety

**Problem:** IntegratedServer runs in a separate thread. Accessing `clientInventory()` requires synchronization.

**Solution:** Use `m_clientDataMutex` or ensure access only from server thread.

```cpp
// Wrong - race condition
auto& inventory = server.clientInventory();
inventory.setItem(0, item);  // May crash if server is ticking

// Correct - mutex protection
std::lock_guard<std::mutex> lock(server.m_clientDataMutex);
// ... access inventory
```

### 2. Initialization Order

**Problem:** Managers must be initialized in correct order due to dependencies.

**Correct Order:**

1. `initializeRegistries()` - Blocks, items, recipes
2. `initializeCoreManagers()` - PlayerManager, ConnectionManager, etc.
3. Create World, ChunkManager, LightManager
4. `initializeWorld()` - Command registry
5. `initializeInteractionManagers()` - Block, mining, container managers
6. `ServerDimension::initialize()` - Creates sync managers (EntitySyncManager, ChunkSendManager, BlockUpdateSyncManager, LightSyncManager) and spawn managers (NaturalSpawner, DespawnManager) per dimension
7. `setupWorldCallbacks()` - World event callbacks（包括方块变化回调）

**注意**: `initializeSyncManagers()` 和 `initializeChunkSyncManagers()` 已从 MinecraftServer 移除，同步管理器现在在各维度的 `ServerDimension::initialize()` 中创建。

### 3. Packet Handling Must Check Session Validity

**Problem:** Session may disconnect between packet dispatch and handling.

**Solution:** Always validate player data:

```cpp
void handlePacket(PlayerId playerId, ...) {
    auto* player = m_playerManager->getPlayer(playerId);
    if (!player || !player->loggedIn) {
        return;  // Player disconnected
    }
    // ... process packet
}
```

### 4. Dimension-Aware World Access

**Problem:** `m_world` has been removed from `MinecraftServer`. World access must go through `ServerDimension`.

**Solution:** Use `dimensionManager()` or `getPlayerWorld()` to access worlds:

```cpp
// 获取玩家所在维度的世界
auto* world = server.getPlayerWorld(playerId);
if (!world) {
    return;  // Player not in any dimension
}

// 获取特定维度的世界
auto& dimMgr = server.dimensionManager();
auto* overworld = dimMgr.getOverworld();
if (overworld && overworld->world()) {
    auto& chunkMgr = *overworld->world()->chunkManager();
}
```

### 5. Double Initialization

**Problem:** Calling `initialize()` twice causes error.

**Solution:** Check `m_initialized` flag or use `Result<void>` return value.

```cpp
auto result = server.initialize(config);
if (result.failed() && result.error().code() == ErrorCode::AlreadyExists) {
    // Already initialized
}
```

### 6. Proper Shutdown Sequence

**Problem:** Calling `stop()` without `shutdown()` or vice versa.

**Solution:** `shutdown()` calls `stopCore()` internally. For IntegratedServer, call `stop()`. For StandaloneServer, call `stop()` then the main loop exits.

---

## Test Coverage

| Test File                                             | Description                                     |
| ----------------------------------------------------- | ----------------------------------------------- |
| `tests/server/test_integrated_server.cpp`             | IntegratedServer unit tests                     |
| `tests/server/BlockUpdateSyncManagerTest.cpp`         | 方块更新 pending 去重、追踪玩家过滤、tick flush |
| `tests/server/ServerWorldBlockUpdateCallbackTest.cpp` | ServerWorld 方块变化回调触发                    |

**Test Categories:**

1. **Basic Lifecycle Tests:**
   - `CreateServer` - Verify initial state
   - `InitializeServer` - Basic initialization
   - `DoubleInitializeFails` - Error on double init
   - `StopWithoutInitialize` - Safe to stop without init

2. **Communication Tests:**
   - `GetClientEndpoint` - Endpoint availability
   - `ReceivePacketAfterStart` - Basic communication
   - `BidirectionalCommunication` - Send/receive
   - `MultipleSends` - Packet queue handling
   - `LargePacket` - Large data handling

3. **Timing Tests:**
   - `TickCountIncreases` - Verify tick progression
   - `ServerTicksWhileWaiting` - Background ticking

4. **Disconnect Tests:**
   - `ClientDisconnect` - Client-initiated disconnect
   - `ServerStopClosesEndpoint` - Server shutdown closes connection

---

## Architecture Decisions

### Why Two Server Types?

- **IntegratedServer:** Optimized for single-player, shares process with client, no network overhead
- **StandaloneServer:** Full TCP networking, multi-player support, persistent settings

### Why Abstract Base Class?

- `MinecraftServer` encapsulates 90%+ of shared logic
- Subclasses only implement network layer differences
- Easy to add new server types (e.g., LAN server)

### Why Direct Manager Access?

- Avoids deep call chains (`server.world().chunkManager()`)
- Allows fine-grained testing of components
- Follows composition over inheritance

### Why Separate Thread for IntegratedServer?

- Maintains consistent tick rate regardless of client frame rate
- Isolates server state from client rendering
- Enables true async chunk generation

## Mermaid Diagram

```mermaid
flowchart LR
    init["MinecraftServer 初始化"] --> dim["ServerDimension::initialize()"]
    dim --> block["BlockUpdateSyncManager"]
    dim --> chunk["ChunkSendManager"]
    dim --> light["LightSyncManager"]
    dim --> entity["EntitySyncManager"]
    world["ServerWorld::setBlockState"] --> callback["setOnBlockChanged"]
    callback --> block
    block --> flush["ServerDimension::tick() 末 flushPendingUpdates()"]
    flush --> packet["BlockUpdatePacket"]
    packet --> client["客户端"]

    style init fill:#ffd166,stroke:#b7791f,color:#111
    style dim fill:#8ecae6,stroke:#1d4ed8,color:#111
    style block fill:#90be6d,stroke:#2f6f3e,color:#111
    style chunk fill:#f4a261,stroke:#b45309,color:#111
    style light fill:#cdb4db,stroke:#6d28d9,color:#111
    style entity fill:#a8dadc,stroke:#457b9d,color:#111
    style world fill:#ffe8a3,stroke:#c99700,color:#111
    style callback fill:#bde0fe,stroke:#2563eb,color:#111
    style flush fill:#e9c46a,stroke:#a16207,color:#111
    style packet fill:#f1f5f9,stroke:#475569,color:#111
    style client fill:#e2e8f0,stroke:#334155,color:#111
```
