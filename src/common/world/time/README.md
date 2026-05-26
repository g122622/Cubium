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

**【重要】MC 1.16.5 行为变更**：
- `dayTime` 是**无边界计数器**，不会自动取模
- `/time add 100000` 后 dayTime 可以是 125000（即 5 天 + 1000）
- 使用 `dayTimeOfDay()` 获取归一化的一天内时间 (0-23999)

**成员变量**：
- `m_dayTime` (i64)：累积的日光时间（无边界），可用于计算天数、月相等
- `m_gameTime` (i64)：游戏启动以来的总 tick 数，用于统计和月相计算
- `m_daylightCycleEnabled` (bool)：日光周期是否启用

**主要方法**：
| 方法 | 说明 |
|------|------|
| `tick()` | 更新时间（每 tick 调用一次） |
| `setDayTime(i64)` | 设置 dayTime（直接存储，不取模） |
| `addDayTime(i64)` | 增加 dayTime |
| `setGameTime(i64)` | 设置 gameTime |
| `setDaylightCycleEnabled(bool)` | 启用/禁用日光周期 |
| `dayTime()` | 获取累积的日光时间（可能超过 24000） |
| `dayTimeOfDay()` | 获取当前一天内的时间 (0-23999) |
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
        // MC 1.16.5 行为：dayTime 递增但不取模
        // 只有在读取一天内时间时才取模
        m_dayTime++;
    }
}
```
- 总是递增 gameTime
- 仅在日光周期启用时递增 dayTime
- **dayTime 不自动循环**，可以超过 24000

#### setDayTime() 方法
```cpp
void GameTime::setDayTime(i64 time) {
    // MC 1.16.5 行为：直接存储，不取模
    // dayTime 可以是任意值，包括负数
    m_dayTime = time;
}
```
- **不自动取模**，直接存储原始值
- 支持负数（例如 `/time set -100`）

#### dayTimeOfDay() 方法
```cpp
i64 GameTime::dayTimeOfDay() const {
    // 使用数学公式确保负数也能正确取模
    return ((m_dayTime % TICKS_PER_DAY) + TICKS_PER_DAY) % TICKS_PER_DAY;
}
```
- 返回归一化的一天内时间 (0-23999)
- 正确处理负数

#### isDay() 方法
```cpp
bool GameTime::isDay() const {
    i64 tod = dayTimeOfDay();
    return tod >= SUNRISE && tod < SUNSET;
}
```
- 白天定义：dayTimeOfDay() 在 [0, 12000) 范围内

#### dayTimeForNetwork() 方法
```cpp
i64 GameTime::dayTimeForNetwork() const {
    i64 tod = dayTimeOfDay();
    return m_daylightCycleEnabled ? tod : -tod;
}
```
- MC 协议规定：负数表示日光周期禁用
- 返回的是 dayTimeOfDay (0-23999) 而非原始 dayTime

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
- `dayTime()`：累积的日光时间（可能超过 24000）
- `dayTimeOfDay()`：归一化的一天内时间 (0-23999)
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
i64 rawDayTime = gameTime.dayTime();        // 累积时间（可能超过 24000）
i64 tod = gameTime.dayTimeOfDay();          // 归一化的一天内时间 (0-23999)
i64 totalTicks = gameTime.gameTime();       // 总 tick 数
i64 days = gameTime.dayCount();             // 天数

// 设置时间 (用于 /time set 命令)
gameTime.setDayTime(6000);   // 设置为正午
gameTime.setDayTime(100000); // 可以超过 24000

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

// 获取时间
i64 rawDayTime = timeManager.dayTime();       // 累积时间
i64 tod = timeManager.dayTimeOfDay();         // 归一化时间

// 获取内部 GameTime
const auto& gt = timeManager.gameTimeObj();
```

---

## 容易踩的坑

### 1. dayTime 可以超过 24000

```cpp
// MC 1.16.5 新行为：dayTime 是无边界计数器
gameTime.setDayTime(100000);
EXPECT_EQ(gameTime.dayTime(), 100000);     // 不取模，直接存储
EXPECT_EQ(gameTime.dayTimeOfDay(), 100000 % 24000);  // 归一化方法返回 0-23999
```

### 2. 使用正确的方法

```cpp
// 根据场景选择正确的方法：

// 场景 1：天体角度计算、时间显示、睡眠检测 → 使用 dayTimeOfDay()
i64 tod = gameTime.dayTimeOfDay();
f32 angle = getCelestialAngle(tod);

// 场景 2：保存到存档、统计总天数 → 使用 dayTime()
i64 rawTime = gameTime.dayTime();
saveToNBT(rawTime);

// 场景 3：网络同步 → 使用 dayTimeForNetwork()
i64 networkTime = gameTime.dayTimeForNetwork();
```

### 3. setDayTime 不再自动取模

```cpp
// 旧行为（错误）：
// gameTime.setDayTime(-100);
// EXPECT_EQ(gameTime.dayTime(), 23900);  // 自动取模

// 新行为（正确）：
gameTime.setDayTime(-100);
EXPECT_EQ(gameTime.dayTime(), -100);        // 直接存储
EXPECT_EQ(gameTime.dayTimeOfDay(), 23900);  // 归一化方法处理负数
```

### 4. dayTimeForNetwork 返回归一化值

```cpp
gameTime.setDayTime(25000);
gameTime.setDaylightCycleEnabled(true);
EXPECT_EQ(gameTime.dayTimeForNetwork(), 1000);  // 返回 25000 % 24000 = 1000

gameTime.setDaylightCycleEnabled(false);
EXPECT_EQ(gameTime.dayTimeForNetwork(), -1000);  // 负值表示禁用
```

### 5. 日光周期禁用时 dayTime 不递增

```cpp
gameTime.setDaylightCycleEnabled(false);
gameTime.tick();
EXPECT_EQ(gameTime.dayTime(), 0);  // dayTime 不变
EXPECT_EQ(gameTime.gameTime(), 1); // gameTime 仍然递增
```

### 6. isDay() 的边界条件

```cpp
// isDay() 返回 true 的范围是 [0, 12000)
gameTime.setDayTime(0);
EXPECT_TRUE(gameTime.isDay());      // 日出是白天

gameTime.setDayTime(11999);
EXPECT_TRUE(gameTime.isDay());      // 日落前一 tick

gameTime.setDayTime(12000);
EXPECT_FALSE(gameTime.isDay());     // 日落时刻是夜晚

// 超过 24000 的情况
gameTime.setDayTime(25000);         // 25000 % 24000 = 1000
EXPECT_TRUE(gameTime.isDay());      // 1000 是白天
```

---

## 涉及的测试用例

测试文件：`tests/common/test_time.cpp`

### GameTime 测试

| 测试名称 | 验证内容 |
|---------|---------|
| `InitialState` | 初始状态：dayTime=0, gameTime=0, daylightCycleEnabled=true |
| `TickIncrement` | tick() 正确递增 dayTime 和 gameTime |
| `DayTimeUnbounded` | dayTime 可以超过 24000，不自动取模 |
| `SetDayTimeUnbounded` | setDayTime 直接存储，不取模 |
| `AddDayTime` | addDayTime 正确处理超 24000 的情况 |
| `DaylightCycleDisabled` | 日光周期禁用时 dayTime 不递增 |
| `IsDayAndIsNight` | isDay() 和 isNight() 判断正确（包括超过 24000 的情况） |
| `DayCount` | dayCount() 正确计算天数 |
| `DayTimeForNetwork` | dayTimeForNetwork() 正确返回归一化值 |
| `DayTimeOfDayNegativeTime` | dayTimeOfDay() 正确处理负数 |

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
| `common/world/lighting/InternalLightUtils` | 内部光照计算（天空减暗等） |
