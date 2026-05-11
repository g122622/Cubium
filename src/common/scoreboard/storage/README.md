# 记分板存储模块

本目录包含记分板数据的持久化相关代码。

## 目录结构

```
storage/
├── ScoreboardSaveData.hpp       # 记分板数据序列化结构
├── ScoreboardSaveData.cpp       # 序列化实现
├── ScoreboardDataManager.hpp    # 数据管理器接口
├── ScoreboardDataManager.cpp    # 数据管理器实现
└── README.md                    # 本文件
```

## 文件说明

### ScoreboardSaveData

记分板数据的序列化结构，包含：

- **ObjectiveData**: 目标持久化数据（名称、判据、显示名、渲染类型）
- **ScoreData**: 分数持久化数据（玩家名、目标名、分数值、锁定状态）
- **TeamData**: 队伍持久化数据（名称、颜色、前缀后缀、成员列表等）
- **DisplaySlotData**: 显示槽位数据（槽位索引、目标名）

支持 NBT 格式的序列化/反序列化，兼容 MC 1.16.5 存档格式。

### ScoreboardDataManager

负责记分板数据的持久化存储和加载：

- **缓存机制**: 内存缓存 + 脏标记，减少磁盘 I/O
- **线程安全**: 使用互斥锁保护缓存访问
- **自动保存**: 析构时自动保存脏数据

#### 键设计

使用 RocksDB 列族 `scoreboard`，键格式：

- `obj:{name}` - 目标数据
- `score:{objective}:{player}` - 分数数据
- `team:{name}` - 队伍数据
- `displayslots` - 显示槽位数据

## 使用方法

### 初始化

```cpp
#include "scoreboard/storage/ScoreboardDataManager.hpp"
#include "world/storage/db/RocksDBDatabase.hpp"

// 在 WorldStorageService 中
m_scoreboardDataManager = std::make_unique<scoreboard::ScoreboardDataManager>(*m_db);
```

### 保存记分板

```cpp
// 保存整个记分板
auto result = m_scoreboardDataManager->saveScoreboard(m_scoreboard);
if (result.failed()) {
    spdlog::error("Failed to save scoreboard: {}", result.error().message());
}

// 或只保存单个目标
ScoreboardSaveData::ObjectiveData objData;
objData.name = "deaths";
objData.criteriaName = "deathCount";
objData.displayName = "{\"text\":\"Deaths\"}";
objData.renderType = "integer";
m_scoreboardDataManager->saveObjective(objData);
```

### 加载记分板

```cpp
// 加载整个记分板
auto result = m_scoreboardDataManager->loadScoreboard(m_scoreboard);
if (result.failed()) {
    spdlog::error("Failed to load scoreboard: {}", result.error().message());
}
```

### 集成到 ServerScoreboard

```cpp
void ServerScoreboard::save() {
    if (m_dirty && m_dataManager) {
        auto result = m_dataManager->saveScoreboard(*this);
        if (result.success()) {
            m_dirty = false;
        }
    }
}

void ServerScoreboard::load() {
    if (m_dataManager) {
        m_dataManager->loadScoreboard(*this);
    }
}
```

## 依赖关系

- **RocksDBDatabase**: 底层数据库
- **NBT 序列化**: 使用 `util/nbt/NBT.hpp`
- **Scoreboard**: 核心记分板类

## 相关文件

- `src/common/world/storage/db/ColumnFamilies.hpp` - 列族定义（包含 `SCOREBOARD`）
- `src/common/world/storage/WorldStorageService.hpp` - 存储服务集成点
- `src/server/scoreboard/ServerScoreboard.hpp` - 服务端记分板
