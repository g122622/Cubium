# Time 模块

游戏时间管理系统，负责管理 Minecraft 世界的时间流逝、日光周期和昼夜判断。

## 目录结构

```
src/common/world/time/
├── GameTime.hpp    # 时间常量定义和 GameTime 类声明
└── GameTime.cpp    # GameTime 类实现
```

## 内部模块关系

```
TimeConstants 命名空间（常量）
        ↓ 被使用
GameTime 类（接口）
        ↓ 实现
GameTime.cpp（实现）
```

- `TimeConstants` 提供 Minecraft 时间系统的核心常量（TICKS_PER_DAY、NOON、SUNSET 等）
- `GameTime.hpp` 定义类的公共接口
- `GameTime.cpp` 实现具体逻辑

## 上下游外部依赖关系

### 上游依赖（本模块依赖的）

| 依赖 | 说明 |
|------|------|
| `common/core/Types.hpp` | 基本类型定义 (i64, u64 等) |

### 下游依赖（依赖本模块的）

| 模块 | 说明 |
|------|------|
| `server/core/TimeManager` | 服务端时间管理器，封装 GameTime |
| `client/renderer/trident/sky/CelestialCalculations` | 天体计算（太阳位置、月相、天空颜色等） |
| `server/command/commands/TimeCommand` | /time 命令实现 |
| `common/network/ir/packets/play/PlayPackets.hpp` | `ir::play::SetTime` 网络同步（旧 `TimeUpdatePacket`/`ProtocolPackets.hpp` 已删除，统一走 IR） |
| `common/world/lighting/InternalLightUtils` | 内部光照计算（天空减暗等） |

## 模块整体职责

1. **时间流逝管理**：跟踪游戏世界的 tick 计数
2. **日光周期控制**：管理昼夜交替，支持启用/禁用
3. **时间查询**：提供当前时间、天数、白天/夜晚判断
4. **网络同步支持**：提供符合 MC 协议的时间编码

## 容易踩的坑

### 1. dayTime 可以超过 24000

MC 1.16.5 行为：`dayTime` 是**无边界计数器**，不会自动取模。`/time add 100000` 后 dayTime 可以是 125000。使用 `dayTimeOfDay()` 获取归一化的一天内时间 (0-23999)。

### 2. 使用正确的方法

根据场景选择正确的方法：
- **天体角度计算、时间显示、睡眠检测** → 使用 `dayTimeOfDay()`
- **保存到存档、统计总天数** → 使用 `dayTime()`
- **网络同步** → 使用 `dayTimeForNetwork()`

### 3. setDayTime 不自动取模

`setDayTime(-100)` 会直接存储 -100，不会自动取模为 23900。如需归一化时间，调用 `dayTimeOfDay()`。

### 4. dayTimeForNetwork 返回归一化值

`dayTimeForNetwork()` 返回的是 `dayTimeOfDay()` (0-23999)，而非原始 dayTime。当日光周期禁用时返回负值。

### 5. 日光周期禁用时 dayTime 不递增

`setDaylightCycleEnabled(false)` 后，`tick()` 只递增 gameTime，不递增 dayTime。

### 6. isDay() 的边界条件

`isDay()` 返回 true 的范围是 [0, 12000)。12000（日落时刻）属于夜晚。

### 7. 天数计算使用 gameTime

`dayCount()` 返回的是 `gameTime / 24000`，而非 `dayTime / 24000`。
