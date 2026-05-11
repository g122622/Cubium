# 统计系统

本目录实现 Minecraft 1.16.5 风格的玩家统计系统。

## 目录结构

```
src/server/stats/
├── StatType.hpp/cpp       # 统计类型枚举和工具函数
├── Stat.hpp/cpp           # 统计项类
├── StatRegistry.hpp/cpp   # 统计注册表
├── StatisticsManager.hpp/cpp  # 玩家统计管理器
└── README.md              # 本文档
```

## 核心类

### StatType

统计类型枚举，定义 Minecraft 的九大类统计：

| 类型 | 格式 | 说明 |
|------|------|------|
| `Mined` | `minecraft.mined:{block_id}` | 挖掘方块次数 |
| `Crafted` | `minecraft.crafted:{item_id}` | 合成物品次数 |
| `Used` | `minecraft.used:{item_id}` | 使用物品次数 |
| `Broken` | `minecraft.broken:{item_id}` | 物品损坏次数 |
| `PickedUp` | `minecraft.picked_up:{item_id}` | 拾取物品次数 |
| `Dropped` | `minecraft.dropped:{item_id}` | 丢弃物品次数 |
| `Killed` | `minecraft.killed:{entity_id}` | 击杀实体次数 |
| `KilledBy` | `minecraft.killed_by:{entity_id}` | 被实体击杀次数 |
| `Custom` | `minecraft.custom:{stat_id}` | 自定义统计 |

### Stat

单个统计项，包含：
- 统计类型
- 统计 ID
- 当前值（64位整数）

### StatRegistry

统计注册表（单例），负责：
- 注册所有内置统计（方块挖掘、物品使用、实体击杀、自定义统计）
- 查询统计是否存在
- 获取指定类型的所有统计

### StatisticsManager

玩家统计管理器，负责：
- 管理单个玩家的所有统计数据
- 提供增删改查接口
- NBT 序列化/反序列化（用于存档）
- 标记脏数据（用于增量保存）

## 与其他系统的集成

### 与事件系统集成

通过 `ServerEventBus` 监听游戏事件并更新统计：

```cpp
// 方块破坏 -> 增加挖掘统计
eventBus.onBlockBreak([&](const BlockBreakEvent& e) {
    stats.incrementMined(e.blockState->block().name());
});

// 实体击杀 -> 增加击杀统计
eventBus.onPlayerKillEntity([&](const PlayerKillEntityEvent& e) {
    stats.incrementKilled(e.entityType->name());
});

// 物品合成 -> 增加合成统计
eventBus.onItemCrafted([&](const ItemCraftedEvent& e) {
    stats.incrementCrafted(e.item.name(), e.count);
});
```

### 与 ServerPlayer 集成

每个 ServerPlayer 应持有 StatisticsManager 实例：

```cpp
class ServerPlayer {
    stats::StatisticsManager m_statistics;
public:
    stats::StatisticsManager& statistics() { return m_statistics; }
};
```

### 与记分板判据集成

统计判据格式 `minecraft.{type}:{id}` 可直接用于记分板目标：

```cpp
// 创建统计目标
auto* criteria = ScoreCriteriaRegistry::instance().getCriteria("minecraft.mined:minecraft.stone");
auto* objective = scoreboard.addObjective("stones_mined", *criteria);
```

## 自定义统计列表

Minecraft 1.16.5 内置的自定义统计（部分）：

- `play_one_minute` - 游戏时间（tick）
- `walk_one_cm` - 行走距离
- `sprint_one_cm` - 疾跑距离
- `jump` - 跳跃次数
- `deaths` - 死亡次数
- `mob_kills` - 击杀生物次数
- `player_kills` - 击杀玩家次数
- ...（见 StatRegistry.cpp 完整列表）

## 持久化

统计数据通过 NBT 格式保存，与玩家数据一起存储：

```json
{
  "stats": {
    "minecraft:mined": {
      "minecraft:stone": 1234,
      "minecraft:dirt": 567
    },
    "minecraft:crafted": {
      "minecraft:diamond_sword": 5
    },
    "minecraft:custom": {
      "minecraft:play_one_minute": 72000,
      "minecraft:jump": 1500
    }
  }
}
```

## 参考

- Minecraft 1.16.5: `net.minecraft.stats.Stats`
- Minecraft 1.16.5: `net.minecraft.stats.Stat`
- Minecraft 1.16.5: `net.minecraft.stats.StatisticsManager`
