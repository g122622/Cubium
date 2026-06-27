# 记分板存储模块

本目录包含记分板数据的持久化相关代码。

## 目录结构

```
storage/
├── ScoreboardSaveData.hpp       # 记分板数据序列化结构
├── ScoreboardSaveData.cpp       # 序列化实现
├── ScoreboardDataManager.hpp    # 数据管理器接口（缓存+脏标记）
├── ScoreboardDataManager.cpp    # 数据管理器实现
└── README.md                    # 本文件
```

## 内部模块关系

```
┌─────────────────────────────────────────────────────────────────────┐
│                        ScoreboardSaveData                            │
│  （数据序列化结构：ObjectiveData/ScoreData/TeamData/DisplaySlotData）│
│  - toNbt()/fromNbt() NBT 序列化                                      │
│  - serialize()/deserialize() 二进制序列化                            │
│  - fromScoreboard()/applyToScoreboard() 批量转换                      │
└─────────────────────────────────────────────────────────────────────┘
                                    │
                                    ▼
┌─────────────────────────────────────────────────────────────────────┐
│                     ScoreboardDataManager                            │
│  （数据管理器：持久化存储和加载）                                      │
│  - 缓存机制：内存缓存 + 脏标记，减少磁盘 I/O                           │
│  - 线程安全：互斥锁保护缓存访问                                        │
│  - 自动保存：析构时保存脏数据                                          │
│  - 键设计：obj:{name}/score:{obj}:{player}/team:{name}/displayslots   │
└─────────────────────────────────────────────────────────────────────┘
```

## 上下游外部依赖关系

### 本模块依赖的外部模块

| 依赖模块 | 依赖内容 | 使用位置 |
|---------|---------|---------|
| `common/core/Result.hpp` | Result<T> 错误处理 | ScoreboardSaveData、ScoreboardDataManager |
| `common/scoreboard/core/` | Scoreboard、ScoreObjective、ScorePlayerTeam | ScoreboardSaveData |
| `common/util/nbt/Nbt.hpp` | NBT 序列化 | ScoreboardSaveData |
| `common/util/text/ITextComponent.hpp` | 文本组件 JSON 序列化 | ScoreboardSaveData |
| `world/storage/SingleLevelStorageManager` | 世界存储门面 | ScoreboardDataManager |

### 被哪些模块依赖

| 依赖模块 | 依赖方式 | 使用场景 |
|---------|---------|---------|
| `server/scoreboard/ServerScoreboard` | setDataManager() | 服务端记分板持久化 |
| `server/world/ServerWorld` | 持有实例 | 世界级别的记分板管理 |

## 容易踩的坑

### ITextComponent JSON 序列化

目标显示名、队伍前缀后缀都是 JSON 格式的 ITextComponent。保存时调用 `toJson().dump()`，加载时需要 try-catch 处理 JSON 解析失败的情况，回退到纯文本。

### ScoreboardDataManager 键格式

不要直接拼接字符串，使用 `makeObjectiveKey()`/`makeScoreKey()`/`makeTeamKey()` 方法生成键，避免格式错误。

### 必须通过 SingleLevelStorageManager 访问

底层 RocksDB 列族 `scoreboard` 不对外直接暴露，必须经由 `SingleLevelStorageManager` 进入。ScoreboardDataManager 在构造时接收 storage 引用。

### displayName 可能为空

目标/队伍的 displayName 是可选的，加载时需要检查是否为空再解析 JSON，避免解析空字符串导致异常。

### 分数键的 playerName 可能包含冒号

玩家名理论上可以包含冒号，但当前键格式使用冒号分隔 `score:{objective}:{player}`。如果玩家名包含冒号会导致 `parseScoreKey()` 解析错误。目前 MC 玩家名不允许冒号，但自定义记分板实体名可能触发此问题。

### deleteObjective 会级联删除关联分数

删除目标时，`deleteObjective()` 会自动删除该目标下的所有分数（缓存和数据库），无需手动清理。这包括：
1. 从 `m_scoreCache` 中移除该目标的整个分数缓存
2. 从 `m_dirtyScores` 中移除该目标的所有脏分数条目
3. 使用前缀迭代器删除数据库中 `score:{objectiveName}:` 前缀的所有键

注意：`deleteTeam()` 不会级联删除成员的分数，因为队伍和分数之间没有直接关联。
