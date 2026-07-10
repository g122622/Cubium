# Server Application Module

## 目录结构树

```
src/server/application/
├── IServer.hpp              # 服务器统一接口定义
├── MinecraftServer.hpp      # 服务器抽象基类声明
├── MinecraftServer.cpp      # 服务器抽象基类实现（共享逻辑）
├── IntegratedServer.hpp     # 内置服务器声明（单机模式）
├── IntegratedServer.cpp     # 内置服务器实现（LocalConnection通信）
├── StandaloneServer.hpp     # 独立服务器声明（多人模式）
└── StandaloneServer.cpp     # 独立服务器实现（TCP网络通信）
```

## 内部模块关系

```
                    IServer.hpp
                         ^
                         |
                    MinecraftServer.hpp/cpp
                    /                  \
                   /                    \
    IntegratedServer.hpp/cpp    StandaloneServer.hpp/cpp
```

**继承关系：**
- `IServer` 定义服务器接口契约（含 `publishToLan` 局域网发布接口）
- `MinecraftServer` 实现共享逻辑，委托网络层给子类
- `IntegratedServer` 使用 `LocalConnectionPair` 实现单机通信；`publishToLan()` 后额外启动 `TcpServer` 接受远程玩家（双路径架构，详见下文第 11 节）
- `StandaloneServer` 使用 `TcpServer` 实现多人网络；`publishToLan()` 返回 `Unsupported`

**Tick 执行流程：**
1. `m_timeManager->tick()` - 更新时间
2. 清理断开连接玩家
3. `m_dimensionManager->tick()` - 驱动所有维度（各维度内部执行世界tick、实体同步、区块发送、刷怪等）
4. `sharedStorage()->tickAutoSave()` - 共享存储自动保存
5. `tickEntities()` - 实体tick、物品拾取、实体追踪
6. `miningManager().tick()` - 更新挖掘进度
7. `pollNetwork()` - 处理网络事件
8. `tickKeepAlive()` - 心跳检查

**线程池划分：**
- **计算线程池**：`m_computationWorkerPool` - 区块生成等计算型任务
- **存储IO线程池**：`m_ioWorkerPool` - 注入到 `SingleLevelStorageManager`

## 上下游外部依赖关系

**本模块依赖：**
- `server/core/` - PlayerManager, ConnectionManager, TimeManager 等
- `server/interaction/` - BlockInteractionManager, MiningManager 等
- `server/dimension/` - ServerDimension, ServerDimensionManager
- `server/sync/` - EntitySyncManager, ChunkSendManager 等（由 ServerDimension 持有）
- `server/world/` - ServerWorld, ServerChunkManager, WeatherManager
- `server/network/` - TcpServer, TcpSession
- `server/command/` - CommandRegistry, CommandStorage
- `server/menu/` - CraftingMenu
- `common/entity/inventory/container/` - 容器菜单实现
- `common/network/` - Packets, LocalConnection
- `common/world/` - World, Chunk, Lighting, Generation
- `common/entity/` - Player, Inventory, Loot
- `common/item/` - Items, BlockItems, Recipes
- `common/physics/` - PhysicsEngine
- `common/profiler/` - Tracing

**被依赖方：**
- 客户端启动单机模式时使用 `IntegratedServer`
- 独立服务器程序使用 `StandaloneServer`
- 测试代码通过 `IServer` 接口模拟服务器

## 容易踩的坑

### 1. 维度感知的世界访问
`m_world` 已从 `MinecraftServer` 移除，所有世界访问必须通过 `ServerDimensionManager` / `ServerDimension` / `getPlayerWorld(PlayerId)` 进行。同步管理器（EntitySyncManager、ChunkSendManager等）和刷怪管理器现在由各 `ServerDimension` 持有。

### 2. 线程安全
IntegratedServer 运行在独立线程，访问 `clientInventory()` 需要使用 `m_clientDataMutex` 同步。

### 3. 初始化顺序
必须按正确顺序初始化：
1. `initializeRegistries()` - 方块、物品、配方
2. `initializeCoreManagers()` - PlayerManager, ConnectionManager 等
3. 创建 World, ChunkManager, LightManager
4. `initializeWorld()` - 命令注册、CommandStorage 初始化
5. `initializeInteractionManagers()` - 交互管理器
6. `ServerDimension::initialize()` - 创建同步管理器和刷怪管理器
7. `setupWorldCallbacks()` - 世界事件回调

### 3.1 单机主机权限与作弊开关
单机主机的命令权限由 `IntegratedServer::resolveOpLevel` 在 OP 列表之上叠加「主机 + `IntegratedServerParams::allowCommands` → Owner(4)」运行时判定（不写 `ops.json`）。`allowCommands` 由客户端 `WorldLaunchConfig.allowCommands` 透传（`ClientApplicationSession::initializeGameSession`）；未透传或未开作弊时主机权限为 0，`/tp`、`/gamemode` 等权限 2 命令会因节点被跳过而误报成其它字面量的 `Expected literal`。命令分发（`handleChatMessagePacket`）与登录（`handleLoginRequestPacket`）权限解析统一走 `resolveOpLevel`。

### 4. 线程池职责分离
计算池和IO池职责不同，不能复用同一个 `ServerWorkerPool`。`ServerChunkManager` 只接受外部注入的池指针。

### 5. 生命周期管理
`SingleLevelStorageManager` 的异步任务需要在 `open()` 之后才可使用。共享资源的保存/关闭由 `MinecraftServer` 顶层统一编排，析构函数只做兜底释放。

### 6. 数据包处理需验证会话
会话可能在数据包分发和处理之间断开，务必验证玩家数据有效性。

### 7. 声音/状态广播链路
实体声音通过 `Entity::playSound()` → `ServerWorld::playSound()` → `MinecraftServer::broadcastSound()` 路径广播。实体状态通过 `IWorld::broadcastEntityStatus()` 接口广播。

### 8. 方块破坏动画广播链路
方块破坏动画通过以下链路广播：
- **服务端挖掘**：`MiningManager.tick()` → `setOnBreakAnimBroadcast` 回调 → `ServerWorld::destroyBlockProgress()` → `MinecraftServer::broadcastBlockBreakProgressInRange()` → `BlockBreakAnimPacket` 广播
- **实体破门**：`BreakDoorGoal` 直接调用 `IWorld::destroyBlockProgress()` → 同上链路
- **排除破坏者**：`broadcastBlockBreakProgressInRange()` 通过 `playerEntityManager().getPlayerIdByEntityId()` 将 breakerId 转为 PlayerId，在遍历玩家时跳过破坏者自身。对应 MC Java `ServerLevel.destroyBlockProgress()` 中 `serverplayer.getId() != breakerId` 的过滤逻辑。
- **PlayerId↔EntityId 映射**：`ServerPlayerEntityManager` 维护双向映射，`getPlayerEntityId(PlayerId)` 和 `getPlayerIdByEntityId(EntityId)` 用于两个方向的转换。

### 8a. 粒子广播链路
携带附加数据的粒子通过 `IWorld` 虚接口 + `ServerWorld` 广播回调 + `MinecraftServer::broadcastXxxParticleInRange` 路径广播给附近玩家：
- **方块粒子**：`IWorld::addBlockParticle()` → `ServerWorld::m_onBroadcastBlockParticle` 回调 → `MinecraftServer::broadcastBlockParticleInRange()` → `ParticlePacket::createBlock()`（携带 BlockState ID）
- **物品粒子**：`IWorld::addItemParticle()` → `ServerWorld::m_onBroadcastItemParticle` 回调 → `MinecraftServer::broadcastItemParticleInRange()` → `ParticlePacket::createItem()`（携带 `ItemStack` 序列化字节流）
- **实体效果粒子**：`IWorld::addEntityEffectParticle()` → `ServerWorld::m_onBroadcastEntityEffectParticle` 回调 → `MinecraftServer::broadcastEntityEffectParticleInRange()` → `ParticlePacket::createEntityEffect()`（携带 ARGB 颜色）
- **范围过滤**：各 `broadcastXxxParticleInRange` 默认范围 256 格，遍历 `playerEntityManager()` 中所有在线玩家，跳过距离超出的玩家。对应 MC Java `ServerLevel.sendParticles()` 的距离裁剪逻辑。
- **回调注册**：`MinecraftServer::attachWorldBindings()` 中通过 `world->setOnBroadcastXxxParticle()` 注册回调，将 `ServerWorld` 与 `MinecraftServer` 的广播方法绑定。

### 8b. 方块实体数据广播链路
方块实体数据变化后通过 `ServerWorld` 回调 + `MinecraftServer::broadcastBlockEntityInRange` 路径广播给附近客户端，对应 MC Java `PlayerList.broadcast(null, x, y, z, 64.0, dimension, new ClientboundBlockEntityDataPacket)`：
- **触发入口**：方块实体数据变化后调用 `ServerWorld::broadcastBlockEntity(pos)`
- **回调转发**：`ServerWorld` 通过 `m_onBroadcastBlockEntity` 回调转发给 `MinecraftServer`（避免 ServerWorld 直接依赖 MinecraftServer）
- **回调实现**（`MinecraftServer::attachWorldBindings()` 中注册）：
  1. `ServerWorld::getBlockEntity(pos)` 获取方块实体
  2. `entity->getUpdateTag()` 生成 NBT 复合标签（默认实现调用 `saveToNBT()` 写入完整状态）
  3. `BlockEntityDataPacket::serializeNbtToBytes(tag)` 序列化为 Java 版大端序字节流
  4. 调用 `MinecraftServer::broadcastBlockEntityInRange(pos, type, nbtData, 64.0f)`
- **范围过滤**：`broadcastBlockEntityInRange` 默认范围 64 格，遍历 `playerEntityManager()` 中所有已登录且有连接的玩家，通过 `math::distanceSq` 计算玩家与方块的距离平方，跳过超出 `rangeSq` 的玩家。封装的 `BlockEntityDataPacket`（PacketType=237）通过 `sendPacketToPlayer` 发送。
- **使用场景**：
  - `ServerPlayer::handleUpdateSignPacket()` — 告示牌编辑完成后广播新文本
  - `ServerPlayer::openSignEditor()` — 打开告示牌编辑器前先发送当前内容（避免编辑器打开时覆盖已有内容）
  - 其他需要同步方块实体数据变化的场景

### 9. 关服时玩家运行时状态回写（savePlayerRuntimeState 钩子）

**问题背景**：`saveAllWorldData()` 落盘区块、level.dat、玩家缓存数据，但在线玩家的位置、生命、饥饿、经验、背包等运行时状态从未回写到 `PlayerDataManager` 缓存——`PlayerDataManager::fromPlayer()` 虽然存在但全项目无调用方，导致玩家退出后最新进度丢失。

**钩子机制**：
- `MinecraftServer::savePlayerRuntimeState()` 是一个虚函数钩子（默认空实现），由子类 override 提供具体遍历逻辑。基类无法直接实现，因为 `playerEntityManager()` 是纯虚函数。
- `IntegratedServer::savePlayerRuntimeState()` 和 `StandaloneServer::savePlayerRuntimeState()` 均遍历所有维度的在线 `Player` 实体，调用 `PlayerDataManager::fromPlayer()` 提取运行时状态，再用 `savePlayer()` 更新缓存并标记脏。后续 `stopCore()` → `shutdownManagers()` → `saveAllWorldData()` 会通过 `PlayerDataManager::saveAll()` 把缓存落盘到 RocksDB。

**调用时机（关键，避免数据竞争）**：
- `IntegratedServer::stop()`：在 `m_serverThread->join()` 之后、`clearAll()` 之前调用。join 确保主循环已退出，clearAll 之前确保玩家实体仍存在于 EntityManager 中。
- `StandaloneServer::stop()`：在 `m_serverThread->join()` 之后、`stopCore()` 之前调用。join 确保主循环（含 `tick()`）已退出，避免与正在执行的 tick 产生数据竞争；stopCore 之前确保维度和玩家实体仍然有效。

**只读外来存档**：`isSharedStorageReadonlyForeignWorld()` 返回 true 时，`savePlayerRuntimeState()` 直接跳过，不写盘。

**UUID 来源覆盖（关键）**：`Player` 实体的 `m_uuid` 由登录流程（`handleLoginRequestPacket`）计算离线 UUID 后存入 `ServerPlayerData`，但**未回写到实体本身**。若直接用 `fromPlayer()` 提取的 `uuid` 字段落盘，会以空字符串作为 RocksDB key，导致下次登录 `loadPlayer(uuid)` 查询不到。因此 `savePlayerRuntimeState()` 在调用 `fromPlayer()` 后，会用 `PlayerManager` 中的权威 UUID（`playerData->uuid`）覆盖 `saveData.uuid`，确保落盘 key 与登录查询的 key 一致。

### 10. StandaloneServer 主循环线程归属
`StandaloneServer::run()` 是非阻塞的——它在内部启动 `m_serverThread` 运行 `_mainLoop()`，立即返回。线程由 `StandaloneServer` 自身持有（与 `IntegratedServer::initialize()` 一致），`stop()` 中先 join 再清理。这与早期版本不同：早期版本由 `main.cpp` 在外部 `std::thread` 中调用阻塞式 `run()`，导致 `stop()` 无法等待主循环退出，存在与 `tick()` 的数据竞争。现在 `stop()` 的顺序是：设置 `m_running=false` → join 主循环线程 → `savePlayerRuntimeState()` → `stopCore()` → 关闭网络/设置/性能追踪。

### 11. 集成服务器局域网发布（publishToLan 双路径架构）

**功能背景**：原版 Minecraft 集成服务器（单机开房）支持通过 `/publish [port] [allowCheats]` 命令将单机世界发布到局域网，允许其他玩家通过 TCP 加入。对应原版 `MinecraftIntegratedServer.publishServer()`。

**接口**：`IServer::publishToLan(i32 port, bool allowCheats)` 纯虚方法。
- `IntegratedServer`：启动 TCP 监听器，运行时切换作弊开关。
- `StandaloneServer`：返回 `ErrorCode::Unsupported`（已通过 TCP 对外服务，无法再次发布）。

**双路径架构**：`IntegratedServer` 在发布前只有本地客户端一条路径（`LocalConnectionPair` + `m_clientInventory` 零拷贝优化）。发布后新增 TCP 监听器路径，同一局游戏同时服务本地与远程玩家。通过 `sessionId` 区分：
- `sessionId == 0`：本地客户端，走 `LocalEndpoint`（`_sendToClient` 直接写入队列）。
- `sessionId != 0`：远程 TCP 玩家，走 `PlayerManager` → `ServerPlayerData::send()` → `TcpConnection` → `TcpSession`。

**需分支的虚函数**：以下方法在 `IntegratedServer` 中均按 `playerId == m_clientPlayerId` 或 `sessionId == 0` 分流到对应路径：
| 方法 | 本地客户端路径 | 远程 TCP 玩家路径 |
|------|---------------|------------------|
| `pollNetwork()` | `m_serverEndpoint->receive()` | `m_lanTcpServer->poll()` |
| `broadcastPacket()` | `_sendToClient()` | `forEachPlayer` 过滤 Tcp 连接 |
| `sendPacketToPlayer()` | `_sendToClient()` | `ServerPlayerData::send()` |
| `getPlayerIdForSession()` | 返回 `m_clientPlayerId` | `PlayerManager::getPlayerIdBySession()` |
| `handleLoginRequestPacket()` | 本地登录流程 | `_handleRemoteLoginRequest()` |
| `handleHotbarSelectPacket()` | `m_clientInventory` | `InventoryManager::setSelectedSlot()` |
| `handleContainerClickPacket()` | `ContainerPacketHandler` + `m_openMenu` | `ContainerManager::handleClick()` |
| `handleCloseContainerPacket()` | `_closeCurrentContainer()` | `ContainerManager::closeContainer()` |
| `openContainerRequest()` | `_openContainerMenu()` | `ContainerManager::openContainer()` |
| `getHeldItemForPlacement()` | `m_clientInventory.getSelectedStack()` | `InventoryManager::getHeldItem()` |
| `getSelectedHotbarSlot()` | `m_clientInventory.getSelectedSlot()` | `InventoryManager::getSelectedSlot()` |
| `setInventoryItem()` | `m_clientInventory.setItem()` | `InventoryManager::setItem()` |
| `syncPlayerInventory()` | `_sendPlayerInventory()` | `InventoryManager::syncToClient()` |
| `tryOpenCraftingContainer()` | `_openCraftingTableMenu()` | `ContainerManager::openContainer(Crafting)` |

**远程登录流程**（`_handleRemoteLoginRequest`）：与 `StandaloneServer::handleLoginRequestPacket` 基本一致——创建 `TcpConnection`、分配 `PlayerId`、生成离线 UUID、`addPlayer` + `mapSessionToPlayer`、`setupInitialPlayerState`、`createPlayerEntity`、`playerJoinDimension`、`resolveOpLevel`（远程玩家不享受主机作弊提升）、从存档加载玩家数据、`inventoryManager().initializeInventory()` + 创造模式填充、发送登录响应 / 权限等级 / 初始游戏状态 / 物品栏同步。

**作弊开关运行时切换**：`publishToLan(allowCheats)` 直接修改 `m_params.allowCommands`，`resolveOpLevel` 读取此字段判定主机是否提升为 Owner。此修改不落盘 level.dat，关闭局域网发布后作弊状态恢复到原始设置。

**资源清理**（`stop()` 顺序）：`m_running=false` → `m_connectionPair->disconnect()` → join 主循环线程 → `savePlayerRuntimeState()`（含远程玩家）→ `clearAll()` → `stopCore()` → `m_lanTcpServer->stop()` + reset → 清理 `m_remotePlayerEntityIds` → reset `m_clientConnection` / `m_connectionPair`。`requestStop()` 也会通知 `m_lanTcpServer->stop()` 停止接受新连接。

**TODO（后续集成验证）**：远程玩家登录流程的运行时正确性目前仅通过编译验证，缺少端到端集成测试（需 mock TcpSession 或真实 TCP 连接验证远程玩家能加入并交互）。当前依赖 `StandaloneServer` 的同类流程作为正确性参考，后续应补充专门针对 `IntegratedServer` 远程路径的集成测试。
