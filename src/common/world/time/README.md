# Time 模块

游戏时间管理系统，负责管理 Minecraft 世界的时间流逝、日光周期和昼夜判断。

## 目录结构

```
src/common/world/time/
├── GameTime.hpp    # 时间常量定义和 GameTime 类声明
└── GameTime.cpp    # GameTime 类实现
```

## 文件详细介绍

### GameTime.hpp

**职责**：定义时间常量和 GameTime 类接口。

**主要内容**：

#### TimeConstants 命名空间

Minecraft 1.16.5 时间系统常量：

| 常量 | 值 | 说明 |
|------|-----|------|
| `TICKS_PER_DAY` | 24000 | 一天的 tick 数 |
| `NOON` | 6000 | 正午时刻（太阳最高点） |
| `SUNSET` | 12000 | 日落时刻 |
| `MIDNIGHT` | 18000 | 午夜时刻 |
| `SUNRISE` | 0 | 日出时刻 |
| `TIME_SYNC_INTERVAL` | 20 | 时间同步间隔（ticks） |
| `DEFAULT_MS_PER_TICK` | 50 | 默认每 tick 毫秒数 |

#### GameTime 类

管理游戏世界的 dayTime 和 gameTime：

**成员变量**：
- `m_dayTime` (i64)：一天内的时间 (0-23999)，控制太阳/月亮位置
- `m_gameTime` (i64)：游戏启动以来的总 tick 数，用于统计和月相计算
- `m_daylightCycleEnabled` (bool)：日光周期是否启用

**主要方法**：
| 方法 | 说明 |
|------|------|
| `tick()` | 更新时间（每 tick 调用一次） |
| `setDayTime(i64)` | 设置 dayTime（自动取模处理） |
| `addDayTime(i64)` | 增加 dayTime |
| `setGameTime(i64)` | 设置 gameTime |
| `setDaylightCycleEnabled(bool)` | 启用/禁用日光周期 |
| `dayTime()` | 获取当前一天内的时间 |
| `gameTime()` | 获取总 tick 数 |
| `dayCount()` | 获取已过去的天数 |
| `isDay()` | 判断是否是白天 |
| `isNight()` | 判断是否是夜晚 |
| `dayTimeForNetwork()` | 获取用于网络同步的 dayTime |

---

### GameTime.cpp

**职责**：实现 GameTime 类的所有方法。

**主要实现细节**：

#### tick() 方法
```cpp
void GameTime::tick() {
    m_gameTime++;
    if (m_daylightCycleEnabled) {
        m_dayTime = (m_dayTime + 1) % TimeConstants::TICKS_PER_DAY;
    }
}
```
- 总是递增 gameTime
- 仅在日光周期启用时递增 dayTime
- dayTime 自动循环到 0

#### setDayTime() 方法
```cpp
void GameTime::setDayTime(i64 time) {
    m_dayTime = ((time % TimeConstants::TICKS_PER_DAY) + TimeConstants::TICKS_PER_DAY)
                % TimeConstants::TICKS_PER_DAY;
}
```
- 正确处理负数输入（例如 -100 → 23900）
- 确保结果始终在 [0, 23999] 范围内

#### isDay() 方法
```cpp
bool GameTime::isDay() const {
    return m_dayTime >= TimeConstants::SUNRISE && m_dayTime < TimeConstants::SUNSET;
}
```
- 白天定义：dayTime 在 [0, 12000) 范围内

#### dayTimeForNetwork() 方法
```cpp
i64 GameTime::dayTimeForNetwork() const {
    return m_daylightCycleEnabled ? m_dayTime : -m_dayTime;
}
```
- MC 协议规定：负数表示日光周期禁用

---

## 文件之间的关系

```
TimeConstants 命名空间（常量）
        ↓ 被使用
GameTime 类（接口）
        ↓ 实现
GameTime.cpp（实现）
```

- `TimeConstants` 提供 Minecraft 时间系统的核心常量
- `GameTime.hpp` 定义类的公共接口
- `GameTime.cpp` 实现具体逻辑，包括边界条件处理

---

## 模块整体职责

1. **时间流逝管理**：跟踪游戏世界的 tick 计数
2. **日光周期控制**：管理昼夜交替，支持启用/禁用
3. **时间查询**：提供当前时间、天数、白天/夜晚判断
4. **网络同步支持**：提供符合 MC 协议的时间编码

---

## 输入和输出

### 输入
- `tick()` 调用（每游戏 tick 一次）
- `setDayTime(time)` 时间设置命令
- `addDayTime(ticks)` 时间增加命令
- `setDaylightCycleEnabled(enabled)` 日光周期控制

### 输出
- `dayTime()`：当前一天内时间 (0-23999)
- `gameTime()`：游戏总 tick 数
- `dayCount()`：已过去的天数
- `isDay()` / `isNight()`：昼夜状态
- `dayTimeForNetwork()`：网络同步用时间值

---

## 依赖项

| 依赖 | 说明 |
|------|------|
| `common/core/Types.hpp` | 基本类型定义 (i64, u64 等) |

---

## 使用方法

### 基本使用

```cpp
#include "common/world/time/GameTime.hpp"

mc::time::GameTime gameTime;

// 每 tick 调用
gameTime.tick();

// 查询时间
i64 dayTime = gameTime.dayTime();      // 一天内时间
i64 totalTicks = gameTime.gameTime();  // 总 tick 数
i64 days = gameTime.dayCount();        // 天数

// 设置时间 (用于 /time set 命令)
gameTime.setDayTime(6000);  // 设置为正午

// 增加时间 (用于 /time add 命令)
gameTime.addDayTime(1000);

// 判断昼夜
if (gameTime.isNight()) {
    // 允许怪物生成
}

// 禁用日光周期 (用于 /gamerule doDaylightCycle false)
gameTime.setDaylightCycleEnabled(false);
```

### 与 TimeManager 配合

服务端使用 `TimeManager` 封装：

```cpp
#include "server/core/TimeManager.hpp"

mc::server::core::TimeManager timeManager(0, 0);

// 每 tick 更新
timeManager.tick();

// 获取内部 GameTime
const auto& gt = timeManager.gameTimeObj();
```

---

## 容易踩的坑

### 1. dayTime 范围是 [0, 23999]

```cpp
// 错误理解：一天是 1-24000
// 正确理解：一天是 0-23999
EXPECT_EQ(gameTime.dayTime(), 0);  // 日出时刻
EXPECT_EQ(gameTime.dayTime(), 23999);  // 日出前一 tick
```

### 2. setDayTime 自动处理负数

```cpp
gameTime.setDayTime(-100);
EXPECT_EQ(gameTime.dayTime(), 23900);  // 自动取模，不是错误

gameTime.setDayTime(25000);
EXPECT_EQ(gameTime.dayTime(), 1000);  // 超出范围也自动取模
```

### 3. dayTimeForNetwork 返回负值表示日光周期禁用

```cpp
gameTime.setDayTime(1000);
gameTime.setDaylightCycleEnabled(true);
EXPECT_EQ(gameTime.dayTimeForNetwork(), 1000);  // 正值

gameTime.setDaylightCycleEnabled(false);
EXPECT_EQ(gameTime.dayTimeForNetwork(), -1000);  // 负值！
```

### 4. 日光周期禁用时 dayTime 不递增

```cpp
gameTime.setDaylightCycleEnabled(false);
gameTime.tick();
EXPECT_EQ(gameTime.dayTime(), 0);  // dayTime 不变
EXPECT_EQ(gameTime.gameTime(), 1); // gameTime 仍然递增
```

### 5. isDay() 的边界条件

```cpp
// isDay() 返回 true 的范围是 [0, 12000)
gameTime.setDayTime(0);
EXPECT_TRUE(gameTime.isDay());      // 日出是白天

gameTime.setDayTime(11999);
EXPECT_TRUE(gameTime.isDay());      // 日落前一 tick

gameTime.setDayTime(12000);
EXPECT_FALSE(gameTime.isDay());     // 日落时刻是夜晚
```

---

## 涉及的测试用例

测试文件：`tests/common/test_time.cpp`

### GameTime 测试

| 测试名称 | 验证内容 |
|---------|---------|
| `InitialState` | 初始状态：dayTime=0, gameTime=0, daylightCycleEnabled=true |
| `TickIncrement` | tick() 正确递增 dayTime 和 gameTime |
| `DayTimeCycle` | dayTime 到 23999 后循环回 0 |
| `SetDayTimeNormalizesNegative` | setDayTime 正确处理负数和超范围值 |
| `AddDayTime` | addDayTime 正确处理循环 |
| `DaylightCycleDisabled` | 日光周期禁用时 dayTime 不递增 |
| `IsDayAndIsNight` | isDay() 和 isNight() 判断正确 |
| `DayCount` | dayCount() 正确计算天数 |
| `DayTimeForNetwork` | dayTimeForNetwork() 正确返回正/负值 |

### CelestialCalculations 测试

| 测试名称 | 验证内容 |
|---------|---------|
| `NoonCelestialAngleNearZero` | 正午天体角度接近 0 |
| `MidnightCelestialAngleNearHalf` | 午夜天体角度接近 0.5 |
| `CelestialAngleInRange` | 天体角度始终在 [0, 1] 范围内 |
| `MoonPhaseDay0FullMoon` | 第 0 天是满月 |
| `MoonPhaseDay8CycleBackToFullMoon` | 月相 8 天循环 |
| `SunDirectionNoonUpward` | 正午太阳方向向上 |
| `SunDirectionMidnightDownward` | 午夜太阳方向向下 |
| `StarBrightnessNoonZero` | 正午星星亮度为 0 |
| `StarBrightnessMidnightVisible` | 午夜星星可见 |

---

## 相关模块

| 模块 | 说明 |
|------|------|
| `server/core/TimeManager` | 服务端时间管理器，封装 GameTime |
| `client/renderer/trident/sky/CelestialCalculations` | 天体计算（太阳位置、月相、天空颜色等） |
| `server/command/commands/TimeCommand` | /time 命令实现 |
| `common/network/packet/ProtocolPackets` | TimeUpdatePacket 网络同步 |
