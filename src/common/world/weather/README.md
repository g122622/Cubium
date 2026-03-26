# Weather 模块

天气系统模块，负责管理游戏世界的天气状态和天气相关的计算。

## 目录结构

```
src/common/world/weather/
├── WeatherConstants.hpp  # 天气常量定义
├── WeatherState.hpp       # 天气状态数据结构
├── WeatherUtils.hpp       # 天气工具函数声明
└── WeatherUtils.cpp       # 天气工具函数实现
```

## 文件详解

### WeatherConstants.hpp

**职责**: 定义天气系统使用的所有常量参数。

**主要内容**:

| 常量 | 值 | 说明 |
|------|-----|------|
| `MIN_CLEAR_TIME` | 12000 ticks | 晴天最短持续时间 (约 10 分钟) |
| `MAX_CLEAR_TIME` | 168000 ticks | 晴天最长持续时间 (约 140 分钟) |
| `MIN_RAIN_TIME` | 12000 ticks | 降雨最短持续时间 (约 10 分钟) |
| `MAX_RAIN_TIME` | 24000 ticks | 降雨最长持续时间 (约 20 分钟) |
| `MIN_THUNDER_TIME` | 3600 ticks | 雷暴最短持续时间 (约 3 分钟) |
| `MAX_THUNDER_TIME` | 15600 ticks | 雷暴最长持续时间 (约 13 分钟) |
| `DEFAULT_COMMAND_DURATION` | 6000 ticks | `/weather` 命令默认持续时间 (5 分钟) |
| `STRENGTH_CHANGE_RATE` | 0.01f | 天气强度渐变速率 (每 tick) |
| `LIGHTNING_CHANCE_DENOMINATOR` | 100000 | 闪电生成概率分母 |
| `SKELETON_HORSE_TRAP_CHANCE` | 0.01f | 骷髅马陷阱生成概率 |
| `RAIN_BED_START_TIME` | 12010 | 降雨时睡眠开始时间 |
| `RAIN_BED_END_TIME` | 23991 | 降雨时睡眠结束时间 |
| `CLEAR_BED_START_TIME` | 12542 | 晴天时睡眠开始时间 |
| `CLEAR_BED_END_TIME` | 23459 | 晴天时睡眠结束时间 |
| `THUNDER_SKY_LIGHT_LIMIT` | 10 | 雷暴时天空光照上限 |
| `RAIN_SKY_LIGHT_LIMIT` | 12 | 降雨时天空光照上限 |
| `RAIN_THRESHOLD` | 0.2f | 降雨强度阈值 |
| `THUNDER_THRESHOLD` | 0.9f | 雷暴强度阈值 |

**时间单位说明**: 所有时间单位为 ticks，20 ticks = 1 秒。

**概率分布**:
- 晴天: 109/121 ≈ 90.1%
- 降雨: 1/11 ≈ 9.1%
- 雷暴: 1/121 ≈ 0.8%

---

### WeatherState.hpp

**职责**: 定义天气状态数据结构，存储世界的完整天气状态。

**主要内容**:

#### 枚举类型

```cpp
enum class WeatherType : u8 {
    Clear = 0,   // 晴天 - 无降水
    Rain = 1,    // 降雨 - 正在下雨（非雷暴）
    Thunder = 2  // 雷暴 - 正在下雷暴（需要同时有降雨）
};
```

#### 类 WeatherState

**计时器成员**:
| 成员 | 类型 | 说明 |
|------|------|------|
| `clearWeatherTime` | `i32` | 晴天剩余时间，> 0 时强制保持晴天 |
| `rainTime` | `i32` | 降雨计时器，倒计时到 0 时切换降雨状态 |
| `thunderTime` | `i32` | 雷暴计时器，倒计时到 0 时切换雷暴状态 |

**状态标志成员**:
| 成员 | 类型 | 说明 |
|------|------|------|
| `raining` | `bool` | 是否正在降雨 |
| `thundering` | `bool` | 是否正在雷暴（雷暴时必须 `raining = true`） |

**渐变强度成员**:
| 成员 | 类型 | 说明 |
|------|------|------|
| `rainStrength` | `f32` | 当前降雨强度 (0.0 - 1.0)，用于渲染平滑过渡 |
| `prevRainStrength` | `f32` | 上一帧降雨强度（用于插值） |
| `thunderStrength` | `f32` | 当前雷暴强度 (0.0 - 1.0) |
| `prevThunderStrength` | `f32` | 上一帧雷暴强度（用于插值） |

**游戏规则成员**:
| 成员 | 类型 | 说明 |
|------|------|------|
| `weatherCycleEnabled` | `bool` | 天气周期是否启用（对应游戏规则 `doWeatherCycle`） |

**核心方法**:
| 方法 | 返回类型 | 说明 |
|------|----------|------|
| `weatherType()` | `WeatherType` | 获取当前天气类型 |
| `isRaining()` | `bool` | 是否正在下雨（使用强度阈值判断） |
| `isThundering()` | `bool` | 是否正在雷暴（使用强度阈值判断） |
| `getRainStrength(partialTick)` | `f32` | 获取插值后的降雨强度 |
| `getThunderStrength(partialTick)` | `f32` | 获取插值后的雷暴强度 |
| `canSleep(dayTime)` | `bool` | 是否可以睡觉 |
| `skyLightLimit()` | `u8` | 获取当前天空光照上限 |
| `resetWeather()` | `void` | 重置为晴天（玩家睡觉后调用） |

---

### WeatherUtils.hpp / WeatherUtils.cpp

**职责**: 提供天气相关的工具函数。

**函数列表**:

| 函数 | 说明 |
|------|------|
| `getPrecipitationType(temperature)` | 根据生物群系温度判断降水类型（0=无, 1=雨, 2=雪） |
| `canSeeSky(world, pos)` | 判断位置是否可以看到天空 |
| `canRainAt(world, pos)` | 判断位置是否可以降雨 |
| `canSnowAt(world, pos)` | 判断位置是否可以降雪 |
| `getRandomWeatherDuration(rng, min, max)` | 获取随机天气持续时间 |
| `getRandomClearDuration(rng)` | 生成随机晴天持续时间 |
| `getRandomRainDuration(rng)` | 生成随机降雨持续时间 |
| `getRandomThunderDuration(rng)` | 生成随机雷暴持续时间 |
| `calculateSkyDarkenFactor(rainStrength, thunderStrength)` | 计算天空颜色暗化因子 |
| `calculateCelestialVisibility(rainStrength)` | 计算太阳/月亮可见度 |
| `calculateStarBrightness(rainStrength, dayTime)` | 计算星星亮度 |

**计算公式**:
- 天空暗化因子 = `rainStrength * 0.3125 + thunderStrength * 0.1875`
- 天体可见度 = `1.0 - rainStrength`
- 降水类型: `temperature <= 0.15f` → 雪，否则 → 雨

---

## 文件关系图

```
WeatherConstants.hpp  ←──┬──← WeatherState.hpp
                         │
                         └──← WeatherUtils.hpp  ←── WeatherUtils.cpp
                                    ↑
                                    │
                              外部依赖:
                              - IWorld
                              - BlockPos
                              - IRandom
```

**依赖关系说明**:
1. `WeatherConstants.hpp` 是基础模块，无外部依赖
2. `WeatherState.hpp` 依赖 `WeatherConstants.hpp` 和 `MathUtils.hpp`
3. `WeatherUtils.hpp` 依赖 `WeatherConstants.hpp`，并前向声明 `IWorld`、`BlockPos`、`IRandom`
4. `WeatherUtils.cpp` 依赖 `WeatherUtils.hpp` 和具体实现类

---

## 模块整体说明

### 整体职责

天气模块负责:
1. **定义天气常量**: 天气持续时间、强度变化速率、光照限制等
2. **存储天气状态**: 当前天气类型、计时器、强度值、游戏规则标志
3. **提供工具函数**: 降水类型判断、天空可见性检测、天气持续时间生成、渲染参数计算

### 输入

| 输入类型 | 来源 | 说明 |
|----------|------|------|
| 游戏时间 | TimeManager | 用于睡眠判断 |
| 生物群系温度 | Biome | 用于降水类型判断 |
| 方块位置 | 调用者 | 用于天空可见性检测 |
| 随机数 | Random | 用于天气持续时间生成 |
| 游戏规则 | GameRules | `doWeatherCycle` 开关 |

### 输出

| 输出类型 | 消费者 | 说明 |
|----------|--------|------|
| 天气类型 | 客户端渲染、服务端逻辑 | 判断当前天气状态 |
| 天气强度 | 渲染器 | 用于渲染平滑过渡效果 |
| 天空光照上限 | 光照系统 | 雷暴/降雨时限制天空光照 |
| 睡眠判断 | 玩家系统 | 判断是否可以使用床 |
| 渲染参数 | 天空渲染器 | 天空颜色、天体可见度、星星亮度 |

### 依赖项

**内部依赖**:
- `common/core/Types.hpp` - 基础类型定义
- `common/util/math/MathUtils.hpp` - 数学工具函数 (`lerp`)
- `common/util/math/Vector3.hpp` - 向量类型
- `common/util/math/random/Random.hpp` - 随机数生成器

**外部依赖** (前向声明):
- `IWorld` - 世界接口
- `BlockPos` - 方块位置
- `IRandom` - 随机数生成器接口

### 使用方法

#### 基本使用

```cpp
#include "common/world/weather/WeatherState.hpp"
#include "common/world/weather/WeatherUtils.hpp"
#include "common/world/weather/WeatherConstants.hpp"

// 创建天气状态
mc::weather::WeatherState state;

// 设置降雨
state.raining = true;
state.rainTime = 15000;

// 每帧更新强度渐变
state.prevRainStrength = state.rainStrength;
if (state.raining) {
    state.rainStrength = std::min(1.0f, state.rainStrength + mc::weather::WeatherConstants::STRENGTH_CHANGE_RATE);
} else {
    state.rainStrength = std::max(0.0f, state.rainStrength - mc::weather::WeatherConstants::STRENGTH_CHANGE_RATE);
}

// 查询天气类型
if (state.isRaining()) {
    // 处理降雨逻辑
}

// 获取插值后的强度（用于渲染）
float interpolatedStrength = state.getRainStrength(partialTick);

// 判断是否可以睡觉
if (state.canSleep(dayTime)) {
    // 允许玩家睡觉
}
```

#### 生成随机天气持续时间

```cpp
#include "common/util/math/random/Random.hpp"

mc::math::Random rng(seed);

// 生成随机晴天持续时间
i32 clearDuration = mc::weather::WeatherUtils::getRandomClearDuration(rng);

// 生成随机降雨持续时间
i32 rainDuration = mc::weather::WeatherUtils::getRandomRainDuration(rng);

// 生成随机雷暴持续时间
i32 thunderDuration = mc::weather::WeatherUtils::getRandomThunderDuration(rng);
```

#### 计算渲染参数

```cpp
// 计算天空暗化程度
f32 darkenFactor = mc::weather::WeatherUtils::calculateSkyDarkenFactor(
    state.rainStrength, 
    state.thunderStrength
);

// 计算太阳/月亮可见度
f32 celestialVisibility = mc::weather::WeatherUtils::calculateCelestialVisibility(
    state.rainStrength
);

// 计算星星亮度
f32 starBrightness = mc::weather::WeatherUtils::calculateStarBrightness(
    state.rainStrength, 
    dayTime
);

// 根据温度判断降水类型
i32 precipitationType = mc::weather::WeatherUtils::getPrecipitationType(biomeTemperature);
// 0 = 无降水, 1 = 雨, 2 = 雪
```

---

## 容易踩的坑

### 1. 强度阈值判断 vs 状态标志判断

**问题**: `raining`/`thundering` 标志和 `isRaining()`/`isThundering()` 方法可能返回不同结果。

```cpp
// 错误: 强度还未达到阈值
state.raining = true;
state.rainStrength = 0.0f;
// 此时 state.raining == true，但 state.isRaining() == false

// 正确: 根据需求选择合适的判断方式
// - 使用标志判断: 立即响应天气命令
// - 使用 isRaining(): 等待渲染过渡完成
```

**建议**: 
- 游戏逻辑（如闪电生成）使用状态标志
- 渲染效果使用强度阈值判断

### 2. 雷暴需要同时满足降雨条件

**问题**: 雷暴时 `thundering = true` 但 `raining = false` 是无效状态。

```cpp
// 错误: 只设置雷暴标志
state.thundering = true;
state.raining = false;  // 无效状态！

// 正确: 雷暴时必须同时设置降雨
state.thundering = true;
state.raining = true;
```

### 3. 强度渐变需要手动更新

**问题**: `WeatherState` 只存储状态，不自动更新强度。

```cpp
// 错误: 期望强度自动渐变
state.raining = true;
// rainStrength 仍然是 0.0f

// 正确: 需要在 tick 中更新强度
if (state.raining) {
    state.rainStrength = std::min(1.0f, 
        state.rainStrength + WeatherConstants::STRENGTH_CHANGE_RATE);
} else {
    state.rainStrength = std::max(0.0f, 
        state.rainStrength - WeatherConstants::STRENGTH_CHANGE_RATE);
}
```

**注意**: 服务端 `WeatherManager` 封装了强度渐变逻辑。

### 4. 计时器倒计时需要外部驱动

**问题**: `rainTime`、`thunderTime`、`clearWeatherTime` 不会自动减少。

```cpp
// 需要在 tick 中手动减少
if (state.rainTime > 0) {
    state.rainTime--;
    if (state.rainTime == 0) {
        // 切换天气状态
    }
}
```

### 5. 睡眠时间范围差异

**问题**: 不同天气下睡眠时间范围不同。

| 天气 | 可睡眠时间范围 | 说明 |
|------|----------------|------|
| 晴天 | 12542 - 23459 ticks | 仅夜间 |
| 降雨 | 12010 - 23991 ticks | 扩展范围 |
| 雷暴 | 全天 | 任何时间 |

```cpp
// 错误: 不考虑天气差异
if (dayTime >= 12542 && dayTime <= 23459) {
    // 允许睡觉
}

// 正确: 使用 canSleep() 方法
if (state.canSleep(dayTime)) {
    // 允许睡觉
}
```

### 6. clearWeatherTime 与 rainTime/thunderTime 的优先级

**问题**: `clearWeatherTime > 0` 时会阻止天气周期。

```cpp
// 正确理解优先级
if (clearWeatherTime > 0) {
    // 强制晴天，忽略 rainTime/thunderTime
} else if (rainTime == 0) {
    // 可以触发新的降雨
}
```

### 7. 插值需要保存上一帧强度

**问题**: `getRainStrength(partialTick)` 依赖 `prevRainStrength`。

```cpp
// 错误: 每帧覆盖 rainStrength，未保存上一帧
state.rainStrength = calculateNewStrength();

// 正确: 先保存上一帧强度
state.prevRainStrength = state.rainStrength;
state.rainStrength = calculateNewStrength();
```

---

## 涉及的测试用例

测试文件: `tests/server/weather/WeatherManagerTest.cpp`

### WeatherState 测试

| 测试名称 | 验证内容 |
|----------|----------|
| `DefaultStateIsClear` | 默认状态为晴天 |
| `WeatherTypeReturnsCorrectValue` | `weatherType()` 返回正确类型 |
| `IsRainingUsesThreshold` | `isRaining()` 使用阈值判断 |
| `IsThunderingUsesThreshold` | `isThundering()` 使用阈值判断 |
| `GetRainStrengthInterpolates` | `getRainStrength()` 正确插值 |
| `GetThunderStrengthInterpolates` | `getThunderStrength()` 正确插值 |
| `ResetWeatherClearsAll` | `resetWeather()` 清除所有天气状态 |
| `CanSleepDuringThunder` | 雷暴时任何时间可睡觉 |
| `CanSleepDuringRainAtNight` | 降雨时夜间可睡觉 |
| `CanSleepDuringClearNightOnly` | 晴天仅夜间可睡觉 |
| `SkyLightLimitDependsOnWeather` | 天空光照上限随天气变化 |

### WeatherUtils 测试

| 测试名称 | 验证内容 |
|----------|----------|
| `GetPrecipitationTypeReturnsCorrectValue` | 降水类型判断正确 |
| `CalculateSkyDarkenFactor` | 天空暗化因子计算正确 |
| `CalculateCelestialVisibility` | 天体可见度计算正确 |
| `CalculateStarBrightness` | 星星亮度计算正确 |
| `GetRandomWeatherDurationInValidRange` | 随机持续时间在有效范围内 |

### WeatherConstants 测试

| 测试名称 | 验证内容 |
|----------|----------|
| `DurationsAreSensible` | 持续时间常量合理 |
| `ThresholdsAreValid` | 阈值常量有效 |
| `StrengthChangeRateIsSmall` | 强度变化速率合适 |
| `LightningChanceIsRare` | 闪电概率足够低 |

---

## 参考

本模块参考 Minecraft Java Edition 1.16.5 实现，主要对应类:
- `net.minecraft.world.World` - 天气状态存储
- `net.minecraft.world.server.ServerWorld` - 天气周期逻辑
- `net.minecraft.world.storage.WorldInfo` - 天气数据序列化
