# 管理器 (Manager)

提供统一的存档 API，整合各个子系统。

## 文件说明

| 文件 | 职责 |
|------|------|
| `SaveManager.hpp/cpp` | 存档管理器，系统的主入口 |
| `PlayerDataManager.hpp/cpp` | 玩家数据管理，缓存和读写 |
| `DimensionDataManager.hpp/cpp` | 维度数据管理，各维度的存档路径 |

## 架构图

```
┌────────────────────────────────────────────────────────────────┐
│                        SaveManager                              │
│  (主入口，协调所有子系统)                                        │
├────────────────────────────────────────────────────────────────┤
│                                                                 │
│  ┌─────────────┐  ┌─────────────┐  ┌───────────────────────┐  │
│  │ LevelSave   │  │ IOWorker    │  │ PlayerDataManager     │  │
│  │             │  │             │  │                       │  │
│  │ 目录管理    │  │ 异步I/O     │  │ 玩家数据缓存          │  │
│  │ 路径访问    │  │ 任务队列    │  │ 读写操作              │  │
│  └─────────────┘  └─────────────┘  └───────────────────────┘  │
│                                                                 │
│  ┌─────────────┐  ┌─────────────┐  ┌───────────────────────┐  │
│  │ SessionLock │  │ LevelData   │  │ DimensionDataManager  │  │
│  │             │  │             │  │                       │  │
│  │ 文件锁      │  │ 世界元数据  │  │ 维度路径管理          │  │
│  └─────────────┘  └─────────────┘  └───────────────────────┘  │
│                                                                 │
└────────────────────────────────────────────────────────────────┘
```

## SaveManager 接口

```cpp
// 创建新世界
static Result<unique_ptr<SaveManager>>
createNew(const path& savesDir, const String& name, const WorldSettings& settings);

// 加载现有世界
static Result<unique_ptr<SaveManager>>
load(const path& worldDir);

// 区块操作
future<Result<unique_ptr<ChunkData>>> loadChunkAsync(ChunkCoord x, ChunkCoord z);
future<Result<void>> saveChunkAsync(const ChunkData& chunk);
bool hasChunk(ChunkCoord x, ChunkCoord z) const;

// 玩家操作
Result<unique_ptr<PlayerData>> loadPlayer(const UUID& id);
Result<void> savePlayer(const PlayerData& player);

// 世界数据
const LevelData& levelData() const;
Result<void> saveLevelData();

// 同步与关闭
Result<void> sync();
void close();
```

## 维度路径映射

| 维度 | 目录 |
|------|------|
| 主世界 | `region/` |
| 下界 | `DIM-1/region/` |
| 末地 | `DIM1/region/` |

## 使用流程

```cpp
// 1. 创建新世界
auto save = SaveManager::createNew("saves/", "MyWorld", settings).value();

// 2. 游戏循环中
save->saveChunkAsync(chunk);  // 异步保存

// 3. 加载区块
auto future = save->loadChunkAsync(x, z);
// ... 做其他事情 ...
auto chunk = future.get().value();

// 4. 玩家加入时
auto playerData = save->loadPlayer(playerId);

// 5. 关闭服务器时
save->sync();   // 确保数据写入
save->close();  // 释放资源
```

## 容易踩的坑

1. **生命周期**：SaveManager 应该在服务器生命周期内保持存在
2. **异步等待**：关闭前必须等待所有异步操作完成
3. **缓存一致性**：玩家数据修改后需要标记脏，下次保存时写入
4. **多维度**：不同维度的区块存储在不同目录
