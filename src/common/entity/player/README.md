# 玩家系统模块

本目录包含玩家睡眠系统的核心组件。

## 目录结构

```
src/common/entity/player/
├── SleepResult.hpp      # 睡眠结果枚举定义
├── SleepManager.hpp     # 睡眠管理器（静态工具类）
├── SleepManager.cpp     # 睡眠管理器实现
└── README.md            # 本文档
```

## 文件说明

### SleepResult.hpp

定义睡眠尝试的结果枚举：

- `OK` - 成功入睡
- `NOT_POSSIBLE_HERE` - 此维度不能睡眠（下界/末地）
- `NOT_POSSIBLE_NOW` - 现在不是睡眠时间
- `TOO_FAR_AWAY` - 离床太远
- `OBSTRUCTED` - 床被阻挡
- `OTHER_PROBLEM` - 其他问题
- `NOT_SAFE` - 周围有怪物

提供辅助函数：
- `getSleepResultMessage()` - 获取对应的翻译键
- `isSleepSuccess()` - 检查是否成功

### SleepManager.hpp/cpp

睡眠系统的静态工具类，提供以下功能：

#### 时间检测

```cpp
static bool canSleepAtTime(i64 dayTime, bool isThundering, bool isRaining);
```

根据游戏时间和天气判断是否可以睡眠：
- 雷暴时：任何时间
- 降雨时：12010 - 23991 ticks
- 晴天时：12542 - 23459 ticks

#### 位置检测

```cpp
static bool isPlayerNearBed(const Vector3& playerPos, const BlockPos& bedPos);
```

检测玩家是否在床附近（水平 3 格，垂直 2 格）。

#### 床状态检测

```cpp
static bool isBedObstructed(const IWorld& world, const BlockPos& bedPos, Direction bedFacing);
```

检测床上方是否有足够空间。

```cpp
static bool isBedSurroundedByMonsters(IWorld& world, const BlockPos& bedPos, const Player& player);
```

检测床周围是否有怪物（8x5x8 范围）。

#### 起床位置计算

```cpp
static std::optional<Vector3> findWakeUpPosition(const IWorld& world, const BlockPos& bedPos, Direction bedFacing);
```

计算玩家醒来时的站立位置。

## ServerWorld 睡眠管理

ServerWorld 提供以下睡眠管理功能：

- `updateAllPlayersSleepingFlag()` - 更新全员睡眠标志
- `checkSleepStatus()` - 检查并处理全员睡眠
- `wakeUpAllPlayers()` - 唤醒所有玩家
- `skipToMorning()` - 跳到早晨

当所有非观察者玩家完全入睡（睡眠计时器 >= 100 ticks）时，自动跳过夜晚。

## 网络同步

SleepPacket 用于同步玩家的睡眠状态：
- 睡眠时发送带有床位置的包
- 唤醒时发送不带床位置的包

## 依赖关系

- `IWorld` - 世界接口，用于获取方块状态和实体
- `BlockPos` - 方块位置
- `Direction` - 方向枚举
- `WeatherConstants` - 天气时间常量
- `MonsterEntity` - 怪物实体基类

## 使用示例

```cpp
#include "entity/player/SleepManager.hpp"
#include "entity/player/SleepResult.hpp"

// 检查是否可以睡眠
bool canSleep = SleepManager::canSleepAtTime(dayTime, isThundering, isRaining);

// 检查玩家是否在床附近
if (SleepManager::isPlayerNearBed(playerPos, bedPos)) {
    // 尝试让玩家睡眠
    SleepResult result = player.trySleep(bedPos);
    if (result != SleepResult::OK) {
        player.sendSystemMessage(getSleepResultMessage(result));
    }
}
```

## 注意事项

1. 时间范围遵循 MC 1.16.5 标准
2. 怪物检测范围是床周围 8x5x8 区域
3. 起床位置优先级：床头前方 > 床尾前方 > 床两侧 > 床上方
4. 全员睡眠需要在 ServerWorld.tick() 中通过 checkSleepStatus() 触发

## 测试

测试文件位于 `tests/common/entity/player/SleepManagerTest.cpp`，覆盖：
- 时间检测逻辑
- 距离检测逻辑
- 睡眠结果消息映射
- isSleepSuccess 辅助函数
