# 数据结构 (Data)

定义存档相关的数据结构。

## 文件说明

| 文件 | 职责 |
|------|------|
| `LevelData.hpp/cpp` | 世界元数据，对应 level.dat 文件 |
| `PlayerData.hpp/cpp` | 玩家存储数据，对应 playerdata/<uuid>.dat |
| `WorldSettings.hpp/cpp` | 世界生成设置 |
| `GameRules.hpp/cpp` | 游戏规则集合 |

## LevelData 字段

| 字段 | 类型 | 说明 |
|------|------|------|
| dataVersion | i32 | 数据版本（MC 1.16.5 = 2586） |
| levelName | String | 世界名称 |
| gameType | GameType | 游戏模式 |
| spawnX/Y/Z | i32 | 出生点坐标 |
| gameTime | i64 | 游戏总刻数 |
| dayTime | i64 | 一天内的时间 |
| clearWeatherTime | i32 | 晴天剩余时间 |
| rainTime | i32 | 降雨计时器 |
| thunderTime | i32 | 雷暴计时器 |
| randomSeed | i64 | 世界种子 |

## PlayerData 字段

| 字段 | 类型 | 说明 |
|------|------|------|
| uuid | UUID | 玩家唯一标识 |
| dimension | DimensionId | 当前维度 |
| posX/Y/Z | f64 | 位置坐标 |
| yaw/pitch | f32 | 视角 |
| health | f32 | 生命值 |
| foodLevel | i32 | 饥饿值 |
| xpLevel | i32 | 经验等级 |
| inventory | vector | 物品栏 |
| abilities | PlayerAbilities | 能力 |

## GameRules 规则

| 规则 | 默认值 | 说明 |
|------|--------|------|
| doDaylightCycle | true | 日光周期 |
| doMobSpawning | true | 生物生成 |
| doWeatherCycle | true | 天气周期 |
| keepInventory | false | 死亡保留物品 |
| mobGriefing | true | 生物破坏方块 |

## 容易踩的坑

1. **数据版本**：加载数据时需要检查版本兼容性
2. **默认值**：新世界的字段需要有合理的默认值
3. **NBT 类型**：注意 NBT 类型与 C++ 类型的对应
