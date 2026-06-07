# Server Dimension 模块

服务端维度管理，负责多维度实例、玩家维度追踪、维度切换等功能。

## 目录结构

```
server/dimension/
├── ServerDimension.hpp         # 服务端维度实例（继承 Dimension 基类）
├── ServerDimension.cpp         # 服务端维度实现
├── ServerDimensionManager.hpp  # 服务端维度管理器（创建/管理所有维度实例）
├── ServerDimensionManager.cpp  # 服务端维度管理器实现
└── README.md                   # 本文档
```

## 内部模块关系

```
┌─────────────────────────────────────────────────────────────────┐
│                        MinecraftServer                           │
│  ┌─────────────────────────────────────────────────────────┐    │
│  │                ServerDimensionManager                     │    │
│  │  ┌─────────────────┐ ┌─────────────────┐ ┌─────────────┐ │    │
│  │  │ ServerDimension │ │ ServerDimension │ │ServerDimens.│ │    │
│  │  │   (Overworld)   │ │    (Nether)     │ │  (TheEnd)   │ │    │
│  │  │  ┌───────────┐  │ │  ┌───────────┐  │ │┌───────────┐│ │    │
│  │  │  │ServerWorld│  │ │  │ServerWorld│  │ ││ServerWorld││ │    │
│  │  │  └───────────┘  │ │  └───────────┘  │ │└───────────┘│ │    │
│  │  │  ┌───────────┐  │ │  ┌───────────┐  │ │┌───────────┐│ │    │
│  │  │  │同步管理器 │  │ │  │同步管理器 │  │ ││同步管理器 ││ │    │
│  │  │  │刷怪管理器 │  │ │  │刷怪管理器 │  │ ││刷怪管理器 ││ │    │
│  │  │  └───────────┘  │ │  └───────────┘  │ │└───────────┘│ │    │
│  │  └─────────────────┘ └─────────────────┘ └─────────────┘ │    │
│  └─────────────────────────────────────────────────────────┘    │
│                     玩家维度映射: PlayerId ↔ DimensionId          │
└─────────────────────────────────────────────────────────────────┘
```

- `ServerDimension` 持有单个维度的 `ServerWorld` runtime，以及维度级同步管理器和刷怪管理器
- 三个维度共享同一个世界级 `SingleLevelStorageManager`

## 上下游外部依赖关系

| 上游依赖（本模块依赖的） | 下游依赖（依赖本模块的） |
|------------------------|------------------------|
| `common/world/dimension` - Dimension/DimensionManager 基类 | `MinecraftServer` - 持有 ServerDimensionManager |
| `server/world/ServerWorld` - 每个维度持有一个 runtime | `server/player/ServerPlayer` - 通过维度映射追踪玩家 |
| `server/sync/` - 同步管理器（EntitySync/ChunkSend/BlockUpdateSync/LightSync） | `server/core/TeleportManager` - 同维度传送（维度切换由本模块处理） |
| `server/world/spawn/` - 刷怪管理器（NaturalSpawner/DespawnManager） | |
| `common/world/storage/SingleLevelStorageManager` - 共享存储门面 | |

## 容易踩的坑

1. **维度 ID 约定**: 主世界=0, 下界=1, 末地=2，与 MC 1.16.5 一致
2. **玩家维度同步**: 维度切换时必须同时更新 `m_playerDimensions` 和 `m_dimensionPlayers`
3. **区块卸载顺序**: 必须先卸载旧区块，再加载新区块，避免内存泄漏
4. **主世界不可卸载**: 主世界维度是默认出生点，不能卸载
5. **线程安全**: 维度切换涉及多个管理器，需要在主线程执行
6. **游戏模式获取**: 维度切换包中的游戏模式应从 `ServerPlayerData::gameMode` 获取，而非硬编码
7. **同步管理器是维度级的**: 同步管理器由各 `ServerDimension` 独立持有，不能从 `MinecraftServer` 直接访问。访问时需先获取目标维度：`dimensionManager.getDimension(id)->entitySyncManager()`
8. **刷怪管理器是维度级的**: NaturalSpawner 和 DespawnManager 由各 `ServerDimension` 独立持有，tick 时根据维度类型决定是否执行（仅主世界和下界有 hostile 刷怪，仅主世界有 passive 刷怪）
9. **初始化顺序**: `ServerDimension::initialize()` 中同步管理器的创建必须在 `ServerWorld::initialize()` 之后，因为同步管理器依赖 `ServerChunkManager` 和 `WorldLightManager`
10. **共享存储**: 三个维度共享同一个 `SingleLevelStorageManager`，不会重复打开同一个世界目录
