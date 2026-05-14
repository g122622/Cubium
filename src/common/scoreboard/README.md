# 记分板模块 (Scoreboard)

## 目录结构

```
scoreboard/
├── core/                           # 核心数据类型
│   ├── Scoreboard.hpp/cpp          # 记分板管理器
│   ├── ScoreObjective.hpp/cpp      # 目标类
│   ├── Score.hpp/cpp               # 分数类
│   ├── ScoreCriteria.hpp/cpp       # 判据基类和注册表
│   ├── ScoreCriteriaRenderType.hpp/cpp  # 渲染类型枚举
│   ├── Team.hpp                    # 队伍抽象类
│   ├── ScorePlayerTeam.hpp/cpp     # 队伍实现类
│   └── TeamEnums.hpp               # 队伍枚举类型
├── criteria/                       # 判据实现
│   ├── DummyCriteria.hpp/cpp       # 手动设置判据
│   ├── TriggerCriteria.hpp/cpp     # 触发器判据
│   ├── DeathCountCriteria.hpp/cpp  # 死亡计数判据
│   ├── KillCountCriteria.hpp/cpp   # 击杀计数判据
│   ├── ReadOnlyCriteria.hpp/cpp    # 只读判据（health、food 等）
│   └── TeamKillCriteria.hpp/cpp    # 队伍击杀判据
├── network/                        # 网络同步
│   └── ScoreboardPackets.hpp/cpp   # 记分板网络数据包
└── storage/                        # 持久化
    ├── ScoreboardSaveData.hpp/cpp  # 数据序列化结构
    └── ScoreboardDataManager.hpp/cpp  # 数据管理器
```

## 核心类

### Scoreboard

记分板核心管理器，负责管理目标、分数和队伍。

```cpp
#include "scoreboard/core/Scoreboard.hpp"
#include "scoreboard/core/ScoreCriteria.hpp"

mc::scoreboard::Scoreboard scoreboard;

// 创建目标
auto* criteria = mc::scoreboard::ScoreCriteriaRegistry::instance().getCriteria("dummy");
auto* objective = scoreboard.addObjective("kills", *criteria);

// 设置分数
auto* score = scoreboard.getOrCreateScore("Steve", *objective);
score->setScorePoints(10);

// 创建队伍
auto* team = scoreboard.createTeam("red");
team->addMember("Steve");
team->setColor(mc::TextFormatting::Red);

// 设置显示槽位
scoreboard.setObjectiveInDisplaySlot(mc::scoreboard::DisplaySlot::Sidebar, objective);
```

### ScoreCriteriaRegistry

判据注册表单例，管理所有判据类型。

```cpp
#include "scoreboard/core/ScoreCriteria.hpp"

// 初始化时注册内置判据
mc::scoreboard::ScoreCriteriaRegistry::instance().registerBuiltinCriteria();

// 获取判据
auto* criteria = mc::scoreboard::ScoreCriteriaRegistry::instance().getCriteria("dummy");

// 检查判据是否存在
bool exists = mc::scoreboard::ScoreCriteriaRegistry::instance().hasCriteria("deathCount");
```

## 内置判据

| 判据名 | 类 | 只读 | 说明 |
|--------|-----|------|------|
| `dummy` | DummyCriteria | 否 | 手动设置分数 |
| `trigger` | TriggerCriteria | 否 | 玩家可触发（需 enable） |
| `deathCount` | DeathCountCriteria | 否 | 死亡时自动 +1 |
| `playerKillCount` | PlayerKillCountCriteria | 否 | 击杀玩家时自动 +1 |
| `totalKillCount` | TotalKillCountCriteria | 否 | 击杀任何实体时自动 +1 |
| `health` | HealthCriteria | 是 | 显示生命值（心形） |
| `food` | FoodCriteria | 是 | 显示饥饿值 |
| `air` | AirCriteria | 是 | 显示氧气值 |
| `armor` | ArmorCriteria | 是 | 显示护甲值 |
| `xp` | XpCriteria | 是 | 显示经验值 |
| `level` | LevelCriteria | 是 | 显示等级 |
| `teamkill.{color}` | TeamKillCriteria | 否 | 击杀指定颜色队伍玩家 |
| `killedByTeam.{color}` | TeamKillCriteria | 否 | 被指定颜色队伍玩家击杀 |

## 触发器系统

触发器（Trigger）是一种特殊的判据类型，允许普通玩家修改自己的分数。

### 工作流程

1. **创建目标**：管理员使用 `/scoreboard objectives add <name> trigger` 创建 trigger 类型目标
2. **启用触发器**：管理员使用 `/scoreboard players enable <player> <objective>` 为玩家启用触发器
   - 这会创建一个初始分数（如果不存在）并解锁分数
3. **玩家触发**：玩家使用 `/trigger <objective>` 命令修改自己的分数
   - `/trigger <objective>` - 分数 +1
   - `/trigger <objective> add <value>` - 增加指定值
   - `/trigger <objective> set <value>` - 设置为指定值
4. **自动锁定**：触发后分数自动锁定，需要管理员再次启用才能继续修改

### 分数锁定机制

`Score` 类提供了锁定机制：

```cpp
// 检查是否锁定
bool locked = score->isLocked();

// 锁定分数
score->setLocked(true);

// 解锁分数（用于启用触发器）
score->setLocked(false);
```

### TriggerCriteria

```cpp
#include "scoreboard/criteria/TriggerCriteria.hpp"

// 判据名称常量
constexpr const char* NAME = "trigger";

// 检查目标是否为 trigger 类型
if (objective->getCriteria().getName() == mc::scoreboard::TriggerCriteria::NAME) {
    // 可以用作触发器
}
```

### 与命令系统的集成

- `/trigger` 命令由 `TriggerCommand` 实现
- `/scoreboard players enable` 由 `ScoreboardCommand::enableTrigger()` 实现
- 详细实现见 `src/server/command/commands/TriggerCommand.cpp`

## 显示槽位

```cpp
enum class DisplaySlot : u8 {
    List = 0,           // Tab 列表
    Sidebar = 1,        // 侧边栏
    BelowName = 2,      // 名称下方
    SidebarTeamBlack = 3,   // 队伍侧边栏（16种颜色）
    // ...
    SidebarTeamWhite = 18
};
```

## 队伍属性

### 队伍名称格式化

`ScorePlayerTeam::formatName()` 方法用于格式化队员名称，将前缀、名称、后缀组合并应用队伍颜色：

```cpp
// 设置队伍颜色和前缀后缀
team->setColor(TextFormatting::Gold);
team->setPrefix(std::make_unique<StringTextComponent>("[ADMIN] "));
team->setSuffix(std::make_unique<StringTextComponent>(" ★"));

// 格式化名称
StringTextComponent playerName("Steve");
auto formatted = team->formatName(playerName);

// 结果: "[ADMIN] Steve ★"（金色）
// 前缀、名称、后缀作为子组件追加到空根组件
// 队伍颜色应用到根组件，子组件通过样式继承
```

**实现要点**：
- 创建空 `StringTextComponent` 作为根组件
- 依次追加前缀、名称、后缀的深拷贝
- 队伍颜色应用到根组件（`Reset` 颜色不应用）
- 子组件保留各自样式，可覆盖继承的颜色

### 可见性

- `Always` - 总是显示
- `Never` - 从不显示
- `HideForOtherTeams` - 对其他队伍隐藏
- `HideForOwnTeam` - 对自己队伍隐藏

### 碰撞规则

- `Always` - 总是碰撞
- `Never` - 从不碰撞
- `PushOtherTeams` - 推动其他队伍
- `PushOwnTeam` - 推动自己队伍

## 集成指南

### 1. 初始化

在服务器启动时注册内置判据：

```cpp
#include "scoreboard/core/ScoreCriteria.hpp"
#include "scoreboard/criteria/DummyCriteria.hpp"
// ... 其他判据头文件

void initScoreboard() {
    auto& registry = mc::scoreboard::ScoreCriteriaRegistry::instance();
    registry.registerCriteria(std::make_unique<mc::scoreboard::DummyCriteria>());
    registry.registerCriteria(std::make_unique<mc::scoreboard::TriggerCriteria>());
    // ... 注册其他判据
}
```

### 2. 事件集成

在玩家死亡、击杀实体等事件中更新分数：

```cpp
void onPlayerDeath(mc::ServerPlayer& player) {
    auto* scoreboard = player.getWorld().getScoreboard();
    if (!scoreboard) return;

    auto& registry = mc::scoreboard::ScoreCriteriaRegistry::instance();
    auto* deathCount = registry.getCriteria("deathCount");
    if (deathCount) {
        deathCount->onPlayerDeath(player.getName(), *scoreboard);
    }
}
```

### 3. 只读判据更新

在玩家 tick 中更新只读判据分数：

```cpp
void onPlayerTick(mc::ServerPlayer& player) {
    auto* scoreboard = player.getWorld().getScoreboard();
    if (!scoreboard) return;

    // 更新生命值
    updateReadOnlyScore(*scoreboard, "health", player.getName(), 
                        static_cast<i32>(player.getHealth()));

    // 更新饥饿值
    updateReadOnlyScore(*scoreboard, "food", player.getName(),
                        player.getFoodLevel());

    // ... 其他只读判据
}
```

## 测试

单元测试位于 `tests/server/scoreboard/`：

- `ScoreboardTest.cpp` - 核心逻辑测试
- `ScoreCriteriaTest.cpp` - 判据测试
- `TeamTest.cpp` - 队伍测试

## 参考

- Minecraft 1.16.5: `net.minecraft.scoreboard.*`
