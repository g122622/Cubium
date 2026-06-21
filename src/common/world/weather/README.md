# Weather 模块

天气系统模块，负责管理游戏世界的天气状态和天气相关的计算。

## 目录结构

```
src/common/world/weather/
├── WeatherConstants.hpp  # 天气常量定义（持续时间、阈值、光照限制等）
├── WeatherState.hpp       # 天气状态数据结构（计时器、强度、睡眠判断等）
├── WeatherUtils.hpp       # 天气工具函数声明（降水类型、天空可见性、渲染参数）
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
- `server/world/ServerWorld` - 服务端世界天气状态管理
- `server/world/weather/WeatherManager` - 服务端天气周期逻辑
- `server/command/commands/WeatherCommand` - `/weather` 命令
- `common/entity/player/SleepManager` - 睡眠判断
- `client/world/ClientWeather` - 客户端天气状态同步
- `client/renderer/trident/particle` - 雨滴/雪花粒子渲染
- `client/renderer/trident/core/TridentEngine` - 天空渲染参数

## 容易踩的坑

### 1. 强度阈值判断 vs 状态标志判断

`raining`/`thundering` 标志和 `isRaining()`/`isThundering()` 方法可能返回不同结果。设置 `raining = true` 后，`rainStrength` 仍为 0，此时 `isRaining()` 返回 `false`。

- 游戏逻辑（如闪电生成）使用状态标志 `raining`/`thundering`
- 渲染效果使用强度阈值判断 `isRaining()`/`isThundering()`

### 2. 雷暴需要同时满足降雨条件

`thundering = true` 但 `raining = false` 是无效状态。雷暴时必须同时设置 `raining = true`。

### 3. 强度渐变需要手动更新

`WeatherState` 只存储状态，不自动更新强度。需要在 tick 中手动更新：
```cpp
if (state.raining) {
    state.rainStrength = std::min(1.0f, state.rainStrength + WeatherConstants::STRENGTH_CHANGE_RATE);
}
```
服务端 `WeatherManager` 封装了此逻辑。

### 4. 计时器倒计时需要外部驱动

`rainTime`、`thunderTime`、`clearWeatherTime` 不会自动减少，需要在 tick 中手动递减。

### 5. 睡眠时间范围差异

不同天气下睡眠时间范围不同：
- 晴天：12542 - 23459 ticks（仅夜间）
- 降雨：12010 - 23991 ticks（扩展范围）
- 雷暴：任何时间

使用 `canSleep(dayTime)` 方法而非硬编码判断。

### 6. clearWeatherTime 优先级

`clearWeatherTime > 0` 时会强制晴天，阻止天气周期。由 `/weather clear` 命令设置。

### 7. 插值需要保存上一帧强度

`getRainStrength(partialTick)` 依赖 `prevRainStrength`。每帧更新前需先保存：
```cpp
state.prevRainStrength = state.rainStrength;
state.rainStrength = calculateNewStrength();
```

### 8. 降水类型判断需要额外过滤

`getPrecipitationType(temperature)` 仅根据温度判断雨/雪。无降水生物群系需由调用者结合 `hasPrecipitation() == false`（即 `BiomeClimate::hasPrecipitation` 为 false）额外过滤，或直接使用 `canRainAt()`/`canSnowAt()` 方法。
