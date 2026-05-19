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

#### ObjectiveData

目标持久化数据（名称、判据、显示名、渲染类型）

- **displayName**: JSON 格式的 ITextComponent 序列化结果，例如 `{"text":"Deaths","color":"red"}`

#### ScoreData

分数持久化数据（玩家名、目标名、分数值、锁定状态）

#### TeamData

队伍持久化数据（名称、颜色、前缀后缀、成员列表等）

- **displayName**: JSON 格式的 ITextComponent
- **prefix**: JSON 格式的 ITextComponent（队伍前缀）
- **suffix**: JSON 格式的 ITextComponent（队伍后缀）

#### DisplaySlotData

显示槽位数据（槽位索引、目标名）

支持 NBT 格式的序列化/反序列化，兼容 MC 1.16.5 存档格式。

### ScoreboardDataManager

负责记分板数据的持久化存储和加载：

- **缓存机制**: 内存缓存 + 脏标记，减少磁盘 I/O
- **线程安全**: 使用互斥锁保护缓存访问
- **自动保存**: 析构时自动保存脏数据

#### 键设计

底层仍使用 RocksDB 列族 `scoreboard`，但对外必须经由 `WorldStorageService` 进入，键格式：

- `obj:{name}` - 目标数据
- `score:{objective}:{player}` - 分数数据
- `team:{name}` - 队伍数据
- `displayslots` - 显示槽位数据

## ITextComponent JSON 序列化

本模块支持 ITextComponent 的完整 JSON 序列化和反序列化，实现富文本的持久化存储。

### 序列化（保存时）

当保存记分板数据时，ITextComponent 对象会被转换为 JSON 字符串：

```cpp
// 目标显示名称
if (auto* displayName = objective->getDisplayName()) {
    objData.displayName = displayName->toJson().dump();
}

// 队伍前缀
if (auto* prefix = team->getPrefix()) {
    teamData.prefix = prefix->toJson().dump();
}

// 队伍后缀
if (auto* suffix = team->getSuffix()) {
    teamData.suffix = suffix->toJson().dump();
}
```

### 反序列化（加载时）

当加载记分板数据时，JSON 字符串会被解析为 ITextComponent 对象：

```cpp
// 从 JSON 创建显示名称
if (!objData.displayName.empty()) {
    try {
        nlohmann::json json = nlohmann::json::parse(objData.displayName);
        auto displayName = text::ITextComponent::fromJson(json);
        if (displayName) {
            objective->setDisplayName(std::move(displayName));
        }
    } catch (const nlohmann::json::exception&) {
        // JSON 解析失败，回退到纯文本
        objective->setDisplayName(std::make_unique<text::StringTextComponent>(objData.displayName));
    }
}

// 从 JSON 创建队伍前缀
if (!teamData.prefix.empty()) {
    try {
        nlohmann::json json = nlohmann::json::parse(teamData.prefix);
        auto prefix = text::ITextComponent::fromJson(json);
        if (prefix) {
            team->setPrefix(std::move(prefix));
        }
    } catch (const nlohmann::json::exception&) {
        team->setPrefix(std::make_unique<text::StringTextComponent>(teamData.prefix));
    }
}
```

### JSON 格式示例

```json
{
    "text": "Player Kills",
    "color": "red",
    "bold": true,
    "extra": [
        {"text": " [", "color": "gray"},
        {"text": "50", "color": "gold"},
        {"text": "]", "color": "gray"}
    ]
}
```

翻译组件示例：

```json
{
    "translate": "chat.type.announcement",
    "with": [
        {"text": "Server"},
        {"text": "Welcome!"}
    ],
    "color": "yellow"
}
```

## 使用方法

### 初始化

```cpp
#include "scoreboard/storage/ScoreboardDataManager.hpp"
#include "world/storage/WorldStorageService.hpp"

// 在 WorldStorageService 中
m_scoreboardDataManager = std::make_unique<scoreboard::ScoreboardDataManager>(*this);
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
objData.displayName = "{\"text\":\"Deaths\",\"color\":\"red\"}";  // JSON 格式
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
        } else {
            spdlog::error("ServerScoreboard: Failed to save scoreboard: {}", result.error().message());
        }
    }
}

void ServerScoreboard::load() {
    if (m_dataManager) {
        auto result = m_dataManager->loadScoreboard(*this);
        if (result.success()) {
            // 加载完成
        } else {
            spdlog::error("ServerScoreboard: Failed to load scoreboard: {}", result.error().message());
        }
    }
}
```

## 测试

位于 `tests/server/scoreboard/ScoreboardPersistenceTest.cpp`：

- **ObjectiveData_Serialize/Deserialize**: 目标数据序列化测试
- **ScoreData_Serialize/Deserialize**: 分数数据序列化测试
- **TeamData_Serialize/Deserialize**: 队伍数据序列化测试
- **DisplaySlotData_Serialize/Deserialize**: 显示槽数据测试
- **Scoreboard_RoundTrip**: 完整记分板往返测试
- **Objective_DisplayName_JsonSerialization**: 目标显示名 JSON 序列化测试
- **Team_DisplayName_JsonSerialization**: 队伍显示名 JSON 序列化测试
- **Team_PrefixSuffix_JsonSerialization**: 队伍前缀后缀 JSON 序列化测试
- **ITextComponent_RoundTrip**: ITextComponent 完整往返测试
- **InvalidJson_FallbackToPlainText**: 无效 JSON 回退测试

运行测试：

```powershell
./build/bin/Release/mc_tests.exe --gtest_filter="ScoreboardPersistence*"
```

## 依赖关系

- **WorldStorageService**: 单存档门面入口，ScoreboardDataManager 通过它访问底层数据库
- **NBT 序列化**: 使用 `util/nbt/NBT.hpp`
- **ITextComponent**: 使用 `util/text/ITextComponent.hpp` 进行 JSON 序列化
- **Scoreboard**: 核心记分板类

## 相关文件

- `src/common/world/storage/db/ColumnFamilies.hpp` - 列族定义（包含 `SCOREBOARD`）
- `src/common/world/storage/WorldStorageService.hpp` - 存储服务集成点
- `src/server/scoreboard/ServerScoreboard.hpp` - 服务端记分板
- `src/common/util/text/ITextComponent.hpp` - 文本组件接口
