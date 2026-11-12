# Weather 模块

天气系统模块，负责管理游戏世界的天气状态和天气相关的计算。

## 目录结构

```
src/common/world/weather/
├── WeatherConstants.hpp  # 天气常量（RAIN/THUNDER 的 DELAY/DURATION 区间、强度阈值、闪电概率）
├── WeatherState.hpp       # 天气状态数据结构（计时器、强度、门控查询）
├── WeatherUtils.hpp       # 天气工具函数声明（区间采样、降水类型、天空可见性、渲染参数）
└── WeatherUtils.cpp       # 天气工具函数实现
```

## 内部模块关系

```
WeatherConstants.hpp  ←──┬──← WeatherState.hpp
                         │
                         └──← WeatherUtils.hpp  ←── WeatherUtils.cpp
```

`WeatherConstants.hpp` 是基础模块，无外部依赖。`WeatherState.hpp` 依赖常量定义。`WeatherUtils.hpp` 依赖常量和前向声明。

## 上下游外部依赖

**上游依赖（本模块依赖）**:
- `common/core/Types.hpp` - 基础类型定义
- `common/util/math/MathUtils.hpp` - `lerp` 插值函数
- `common/util/math/random/Random.hpp` - 随机数生成器
- `common/world/IWorld.hpp` - 世界接口（前向声明）
- `common/world/biome/Biome.hpp` - 生物群系温度查询

**下游依赖（依赖本模块）**:
- `server/world/ServerWorld` - 服务端世界天气状态管理、逐区块闪电判定
- `server/world/weather/WeatherManager` - 服务端天气周期逻辑
- `server/network/sync/WeatherSyncService` - 按维度同步强度与开始/结束降雨
- `server/command/commands/WeatherCommand` - `/weather` 命令
- `client/world/ClientWeather` - 客户端天气状态同步
- `client/renderer/trident/particle` - 雨滴/雪花粒子渲染
- `client/renderer/trident/core/TridentEngine` - 天空渲染参数

睡眠判定不依赖本模块：`SleepManager` 直接使用
`common/world/lighting/InternalLightUtils::calculateSkyDarkening >= 4`
（对应原版 `BedRule.WHEN_DARK` → `Level.isDarkOutside`）。

## 容易踩的坑

### 1. 强度阈值判断 vs 状态标志判断

`raining`/`thundering` 标志和 `isRaining()`/`isThundering()` 方法可能返回不同结果。设置 `raining = true` 后，`rainStrength` 仍为 0，此时 `isRaining()` 返回 `false`。

原版游戏逻辑（闪电、降水）与渲染都以强度判定为准：`isRaining()` 用
`rainStrength > 0.2`，`isThundering()` 用 `雷暴强度 × 降雨强度 > 0.9`（`Level.getThunderLevel`）。
标志只表示"目标状态"，不要拿它当"当前是否正在下雨/雷暴"。

### 2. 雷暴需要同时满足降雨条件

`thundering = true` 但 `raining = false` 是无效状态。雷暴时必须同时设置 `raining = true`。

### 3. 强度渐变需要手动更新

`WeatherState` 只存储状态，不自动更新强度。需要在 tick 中手动更新：
```cpp
if (state.raining) {
    state.rainStrength = std::min(1.0f, state.rainStrength + WeatherConstants::STRENGTH_CHANGE_RATE);
}
```
服务端 `WeatherManager` 封装了此逻辑。雷暴强度只看 `thundering` 标志渐变
（不要求 `raining`）；对外查询 `getThunderStrength()` / `isThundering()` 时才乘降雨强度。
网络广播 ThunderStrengthChange 发送的是未乘雨强的原始值（`getRawThunderStrength`）。

### 4. 计时器倒计时需要外部驱动

`rainTime`、`thunderTime`、`clearWeatherTime` 不会自动减少，需要在 tick 中手动递减。

### 5. 睡眠不在本模块判定

原版 1.21.11 已无固定睡眠时间窗：`BedRule.WHEN_DARK` 调用 `Level.isDarkOutside()`，
即天空减暗因子 `>= 4`。降雨/+3、雷暴/+10 通过天空减暗自然扩大可睡时段。
不要在本模块重新引入时间窗常量。

### 6. clearWeatherTime 优先级

`clearWeatherTime > 0` 时会强制晴天，阻止天气周期。由 `/weather clear` 命令设置。
强制晴天分支会把 rain/thunder 计时器压到 0/1，而不是立即随机重置。

### 6b. DELAY / DURATION 两组区间

`rainTime`/`thunderTime` 耗尽后：正在下雨/雷暴时分别用 `RAIN_DURATION [12000,24000]` /
`THUNDER_DURATION [3600,15600]`，否则用 `RAIN_DELAY`/`THUNDER_DELAY [12000,180000]`。
四组均为闭区间，必须用 `IRandom::nextInt(min, max)`（或 `nextIntBetweenInclusive`）；
写成 `nextInt(max - min)` 会把上界丢 1，写成 `nextInt(max - min + 1)` 才是闭区间。

### 7. 插值需要保存上一帧强度

`getRainStrength(partialTick)` 依赖 `prevRainStrength`。每帧更新前需先保存：
```cpp
state.prevRainStrength = state.rainStrength;
state.rainStrength = calculateNewStrength();
```

### 8. 降水类型判断需要额外过滤

`getPrecipitationType(temperature)` 仅根据温度判断雨/雪。无降水生物群系需由调用者结合 `hasPrecipitation() == false`（即 `BiomeClimate::hasPrecipitation` 为 false）额外过滤，或直接使用 `canRainAt()`/`canSnowAt()` 方法。
