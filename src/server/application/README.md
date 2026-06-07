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
- `IServer` 定义服务器接口契约
- `MinecraftServer` 实现共享逻辑，委托网络层给子类
- `IntegratedServer` 使用 `LocalConnectionPair` 实现单机通信
- `StandaloneServer` 使用 `TcpServer` 实现多人网络

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
- `server/command/` - CommandRegistry
- `server/menu/` - CraftingMenu
- `common/entity/inventory/container/` - 容器菜单实现
- `common/network/` - Packets, LocalConnection
- `common/world/` - World, Chunk, Lighting, Generation
- `common/entity/` - Player, Inventory, Loot
- `common/item/` - Items, BlockItems, Recipes
- `common/physics/` - PhysicsEngine
- `common/perfetto/` - Tracing

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
4. `initializeWorld()` - 命令注册
5. `initializeInteractionManagers()` - 交互管理器
6. `ServerDimension::initialize()` - 创建同步管理器和刷怪管理器
7. `setupWorldCallbacks()` - 世界事件回调

### 4. 线程池职责分离
计算池和IO池职责不同，不能复用同一个 `ServerWorkerPool`。`ServerChunkManager` 只接受外部注入的池指针。

### 5. 生命周期管理
`SingleLevelStorageManager` 的异步任务需要在 `open()` 之后才可使用。共享资源的保存/关闭由 `MinecraftServer` 顶层统一编排，析构函数只做兜底释放。

### 6. 数据包处理需验证会话
会话可能在数据包分发和处理之间断开，务必验证玩家数据有效性。

### 7. 声音/状态广播链路
实体声音通过 `Entity::playSound()` → `ServerWorld::playSound()` → `MinecraftServer::broadcastSound()` 路径广播。实体状态通过 `IWorld::broadcastEntityStatus()` 接口广播。
