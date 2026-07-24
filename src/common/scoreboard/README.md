# 记分板模块 (Scoreboard)

## 目录结构

```
scoreboard/
├── core/                                # 核心数据类型
│   ├── Scoreboard.hpp/cpp               # 记分板管理器（管理目标、分数、队伍）
│   ├── ScoreObjective.hpp/cpp           # 目标类（判据+显示名称+渲染类型，缓存格式化名称）
│   ├── Score.hpp/cpp                    # 分数类（玩家名+分数值+锁定状态）
│   ├── ScoreCriteria.hpp/cpp            # 判据基类和注册表（ScoreCriteriaRegistry 单例）
│   ├── ScoreCriteriaRenderType.hpp/cpp  # 渲染类型枚举（Integer/Hearts）和显示槽位枚举
│   ├── Team.hpp                         # 队伍抽象接口（含 getFormattedDisplayName 虚方法）
│   ├── ScorePlayerTeam.hpp/cpp          # 队伍实现类（成员管理、颜色、前后缀、可见性、格式化名称）
│   └── TeamEnums.hpp                    # 队伍枚举（TeamVisibility、TeamCollisionRule）
├── criteria/                            # 判据实现
│   ├── DummyCriteria.hpp/cpp            # 手动设置判据（dummy）
│   ├── TriggerCriteria.hpp/cpp          # 触发器判据（trigger，玩家可通过命令触发）
│   ├── DeathCountCriteria.hpp/cpp       # 死亡计数判据（deathCount）
│   ├── KillCountCriteria.hpp/cpp        # 击杀计数判据（playerKillCount/totalKillCount）
│   ├── ReadOnlyCriteria.hpp/cpp         # 只读判据基类及实现（health/food/air/armor/xp/level）
│   └── TeamKillCriteria.hpp/cpp         # 队伍击杀判据（teamkill.{color}/killedByTeam.{color}）
└── storage/                             # 持久化
    ├── ScoreboardSaveData.hpp/cpp       # 数据序列化结构（ObjectiveData/ScoreData/TeamData）
    └── ScoreboardDataManager.hpp/cpp    # 数据管理器（缓存+脏标记，依赖 SingleLevelStorageManager）
```

## 内部模块关系

```
┌─────────────────────────────────────────────────────────────────────┐
│                            Scoreboard                                │
│  （核心管理器：持有目标、分数、队伍、显示槽位）                        │
│  ├── m_objectives: map<string, unique_ptr<ScoreObjective>>          │
│  ├── m_playerScores: map<string, map<string, unique_ptr<Score>>>    │
│  ├── m_teams: map<string, unique_ptr<ScorePlayerTeam>>              │
│  └── m_displaySlots: array<ScoreObjective*, 19>                     │
└─────────────────────────────────────────────────────────────────────┘
         │                    │                    │
         ▼                    ▼                    ▼
┌─────────────────┐  ┌─────────────────┐  ┌─────────────────┐
│ ScoreObjective  │  │     Score       │  │ ScorePlayerTeam │
│ - name          │  │ - playerName    │  │ - name          │
│ - criteria*     │  │ - score         │  │ - members       │
│ - displayName   │  │ - locked        │  │ - color/prefix/ │
│ - renderType    │  │ - objective*    │  │   suffix        │
└─────────────────┘  └─────────────────┘  └─────────────────┘
         │
         ▼
┌─────────────────────────────────────────────────────────────────────┐
│                         ScoreCriteria                                │
│  （判据基类，定义分数更新方式）                                        │
│  ├── DummyCriteria        - 手动设置                                  │
│  ├── TriggerCriteria      - 玩家可触发（需 enable）                    │
│  ├── DeathCountCriteria   - 死亡时 +1                                 │
│  ├── KillCountCriteria    - 击杀时 +1（玩家/全部）                      │
│  ├── ReadOnlyCriteria     - 只读（health/food/air/armor/xp/level）    │
│  └── TeamKillCriteria     - 队伍击杀（teamkill.{color}/killedByTeam）  │
└─────────────────────────────────────────────────────────────────────┘
         │
         ▼
┌─────────────────────────────────────────────────────────────────────┐
│                     ScoreCriteriaRegistry                            │
│  （单例，管理所有判据类型的注册和查找）                                │
│  - registerBuiltinCriteria() 注册所有内置判据                         │
│  - getCriteria(name)        获取判据                                  │
└─────────────────────────────────────────────────────────────────────┘

【数据流】
ScoreCriteria.onPlayerDeath/onPlayerKill → Scoreboard.forAllObjectives → Score.setScorePoints
```

## 上下游外部依赖关系

### 本模块依赖的外部模块

| 依赖模块 | 依赖内容 | 使用位置 |
|---------|---------|---------|
| `common/core/Types.hpp` | i32、u8 等基础类型 | 全模块 |
| `common/core/Result.hpp` | Result<T> 错误处理 | ScoreCriteria、ScoreboardSaveData、ScoreboardDataManager |
| `common/util/text/ITextComponent.hpp` | 文本组件（显示名称、前缀后缀） | Team、ScoreObjective、ScorePlayerTeam |
| `common/util/text/ComponentUtils.hpp` | wrapInSquareBrackets 方括号包裹工具 | ScoreObjective、ScorePlayerTeam |
| `common/util/nbt/Nbt.hpp` | NBT 序列化 | ScoreboardSaveData |
| `common/network/ir/packets/play/PlayPackets.hpp` | IR 网络包结构（SetObjective/SetScore/SetDisplayObjective/SetPlayerTeam） | ServerScoreboard 出站同步 |
| `world/storage/SingleLevelStorageManager` | 世界存储门面 | ScoreboardDataManager |

### 被哪些模块依赖

目前本模块是独立的记分板系统实现，预计被以下模块依赖（待集成）：
- `server/world/ServerWorld` - 世界持有记分板实例
- `server/player/ServerPlayer` - 玩家死亡/击杀事件触发判据更新
- `server/command/ScoreboardCommand` - 记分板命令处理
- `server/command/TriggerCommand` - 触发器命令处理
- `client/gui/GuiManager` - 客户端显示记分板

## 容易踩的坑

### 目标/队伍名称限制

- **目标名称**：最大 16 字符，仅允许字母、数字、下划线、连字符
- **队伍名称**：最大 16 字符，仅允许字母、数字、下划线、连字符
- **玩家名称**：最大 40 字符
- **分数范围**：INT32_MIN ~ INT32_MAX（-2147483648 ~ 2147483647）

### Trigger 判据的锁定机制

触发器判据的分数在玩家触发后**自动锁定**，需要管理员通过 `/scoreboard players enable` 再次启用才能继续修改。Score 类通过 `isLocked()`/`setLocked()` 管理此状态。

### 只读判据不能通过命令修改

`health`、`food`、`air`、`armor`、`xp`、`level` 这些判据的 `isReadOnly()` 返回 true，分数由游戏自动更新，命令设置分数会被忽略。

### 显示槽位映射

共 19 个显示槽位：
- 0: list（Tab 列表）
- 1: sidebar（侧边栏）
- 2: belowName（名称下方）
- 3-18: sidebar.team.{color}（16 种颜色的队伍专属侧边栏）

### 队伍名称格式化

`ScorePlayerTeam::formatName()` 方法返回的是**新创建的组件**，不是修改原组件。队伍颜色应用到根组件，子组件保留各自样式可覆盖继承的颜色。

`ScorePlayerTeam::getFormattedDisplayName()` 和 `ScoreObjective::getFormattedDisplayName()` 均使用 `ComponentUtils::wrapInSquareBrackets()` 包裹显示名称（翻译键 `chat.square_brackets`），悬停事件显示内部名称。`ScoreObjective` 会缓存格式化结果，在 `setDisplayName` 时自动更新。

### ScoreCriteriaRegistry 初始化

必须在服务器启动时调用 `registerBuiltinCriteria()` 注册所有内置判据，否则 `getCriteria()` 返回 nullptr。判据实例由注册表持有唯一所有权。

### ScoreboardDataManager 键前缀

持久化键格式：
- `"obj:{name}"` - 目标
- `"score:{objective}:{player}"` - 分数
- `"team:{name}"` - 队伍
- `"displayslots"` - 显示槽位

不要直接拼接字符串，使用 `makeObjectiveKey()`/`makeScoreKey()`/`makeTeamKey()` 方法。
