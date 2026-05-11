# command/support 目录

## 目录结构

```
support/
├── README.md              # 本文件
├── PlayerResolver.cpp     # 玩家选择器解析实现
├── PlayerResolver.hpp     # 玩家选择器解析接口
├── EffectResolver.cpp     # 效果选择器解析实现
└── EffectResolver.hpp     # 效果选择器解析接口
```

## 文件介绍

### PlayerResolver.hpp / PlayerResolver.cpp

玩家选择器解析器，用于将 `EntitySelector` 解析为实际的玩家 ID 列表。

#### 主要功能

1. **resolveSinglePlayerId** - 解析单个玩家选择器，返回一个玩家 ID
2. **resolvePlayerIds** - 解析多个玩家选择器，返回玩家 ID 列表
3. **getGameModeCommandName** - 将游戏模式枚举转换为命令字符串
4. **getDifficultyCommandName** - 将难度枚举转换为命令字符串

#### 支持的选择器类型

| 选择器 | 类型 | 描述 |
|--------|------|------|
| `@p` | SinglePlayer | 最近的玩家 |
| `@a` | AllPlayers | 所有玩家 |
| `@r` | RandomPlayer | 随机玩家 |
| `@s` | Self | 命令执行者 |
| 玩家名 | SinglePlayer | 按名称匹配 |

#### 支持的过滤条件

| 参数 | 格式 | 示例 | 描述 |
|------|------|------|------|
| `name` | `name=xxx` | `name=Steve` | 按名称匹配 |
| `name=!xxx` | `name=!Steve` | 排除指定名称 |
| `gamemode` | `gamemode=xxx` | `gamemode=survival` | 按游戏模式过滤 |
| `gamemode=!xxx` | `gamemode=!creative` | 排除指定游戏模式 |
| `distance` | `distance=..10` | `distance=..10` | 距离范围过滤 |
| `level` | `level=10..20` | `level=10..20` | 经验等级过滤 |
| `limit` | `limit=n` | `limit=3` | 结果数量限制 |
| `sort` | `sort=nearest` | `sort=nearest` | 排序方式 |
| `x_rotation` | `x_rotation=-45..45` | `x_rotation=-30..30` | 俯仰角范围（pitch，-90 到 90 度） |
| `y_rotation` | `y_rotation=170..-170` | `y_rotation=-90..90` | 偏航角范围（yaw，-180 到 180 度） |

#### 排序方式

- `nearest` - 按距离近到远
- `furthest` - 按距离远到近
- `random` - 随机排序
- `arbitrary` - 原始顺序

### EffectResolver.hpp / EffectResolver.cpp

效果选择器解析器，用于解析效果 ID 和持续时间等参数。

## 模块关系

```
┌─────────────────────────────────────────────────────────────┐
│                      Command System                          │
├─────────────────────────────────────────────────────────────┤
│                                                              │
│  ┌──────────────────┐    ┌──────────────────┐               │
│  │   Commands       │───▶│   Resolvers      │               │
│  │  (使用选择器)     │    │   (解析选择器)    │               │
│  └──────────────────┘    └──────────────────┘               │
│                                   │                          │
│                                   ▼                          │
│  ┌─────────────────────────────────────────────────────┐    │
│  │                    依赖项                             │    │
│  ├─────────────────────────────────────────────────────┤    │
│  │  • IServer - 访问 PlayerManager                      │    │
│  │  • ServerWorld - 访问玩家实体（等级过滤）             │    │
│  │  • ServerPlayerEntityManager - 获取 Player 实例      │    │
│  │  • ServerPlayerData - 访问玩家数据（位置、游戏模式）  │    │
│  │  • Player - 访问经验等级                             │    │
│  │  • EntitySelector - 选择器参数容器                   │    │
│  └─────────────────────────────────────────────────────┘    │
│                                                              │
└─────────────────────────────────────────────────────────────┘
```

## 使用方法

### 基本用法

```cpp
#include "server/command/support/PlayerResolver.hpp"
#include "server/command/ServerCommandSource.hpp"

// 在命令执行中使用
void MyCommand::execute(CommandContext& context) {
    ServerCommandSource& source = static_cast<ServerCommandSource&>(context.getSource());
    
    // 解析玩家选择器
    EntitySelector selector = context.getArgument<EntitySelector>("player");
    
    // 获取匹配的玩家
    std::vector<PlayerId> players = support::resolvePlayerIds(source, selector);
    
    for (PlayerId playerId : players) {
        // 处理每个玩家...
    }
}
```

### 等级过滤示例

```cpp
// 选择等级 10-20 之间的玩家
EntitySelector selector(EntitySelectorType::AllPlayers);
selector.level().setMin(10);
selector.level().setMax(20);

auto players = support::resolvePlayerIds(source, selector);
// 返回等级在 10-20 范围内的玩家列表
```

### 游戏模式过滤示例

```cpp
// 选择所有生存模式玩家
EntitySelector selector(EntitySelectorType::AllPlayers);
selector.setGameMode("survival");

auto players = support::resolvePlayerIds(source, selector);
```

## 实现细节

### 等级过滤实现

等级过滤需要访问玩家实体的经验数据，实现流程如下：

1. 检查 `selector.level().isUnbounded()` - 如果没有限制则跳过
2. 通过 `IServer::playerEntityManager()` 获取实体管理器
3. 通过 `ServerPlayerEntityManager::getPlayerEntity()` 获取 `Player` 实体
4. 调用 `Player::experienceLevel()` 获取经验等级
5. 使用 `IntRange::test()` 检查等级是否在范围内

```cpp
// 核心逻辑
if (!selector.level().isUnbounded()) {
    if (server != nullptr && world != nullptr) {
        Player* player = server->playerEntityManager().getPlayerEntity(playerData.playerId, *world);
        if (player != nullptr) {
            i32 level = player->experienceLevel();
            if (!selector.level().test(level)) {
                return false;
            }
        }
    }
}
```

### 角度过滤实现

角度过滤用于 `x_rotation`（俯仰角/pitch）和 `y_rotation`（偏航角/yaw）参数：

1. 检查 `selector.xRotation().isUnbounded()` / `selector.yRotation().isUnbounded()`
2. 优先使用实体实时角度（从 `Player` 实体获取 `pitch()` / `yaw()`）
3. 如果实体不可用，使用 `ServerPlayerData` 中存储的角度
4. 使用 `FloatRange::testAngle()` 检查角度是否在范围内

```cpp
// 核心逻辑
if (!selector.xRotation().isUnbounded()) {
    f32 pitch = playerData.pitch;
    if (server != nullptr && world != nullptr) {
        Player* player = server->playerEntityManager().getPlayerEntity(playerData.playerId, *world);
        if (player != nullptr) {
            pitch = player->pitch();
        }
    }
    if (!selector.xRotation().testAngle(pitch)) {
        return false;
    }
}
```

**角度环绕处理**：

角度范围测试使用 `FloatRange::testAngle()` 方法，处理 -180/180 度边界环绕：
- 角度值通过 `wrapDegrees()` 规范化到 [-180, 180) 范围
- 当 `min > max` 时表示范围跨越边界（如 `[170..-170]` 表示接近正北方向）
- 跨越边界时使用 OR 逻辑：`value >= min || value <= max`
- 普通范围使用 AND 逻辑：`value >= min && value <= max`

### 性能考虑

- 等级过滤需要为每个候选玩家获取实体，有额外开销
- 过滤顺序已优化：先进行廉价过滤（用户名、距离），最后进行昂贵过滤（等级）
- 如果 `world` 为空，等级过滤会被跳过（不报错）

## 依赖项

### 外部依赖

- `IServer` - 服务器接口
- `ServerWorld` - 服务端世界
- `ServerPlayerEntityManager` - 玩家实体管理器
- `PlayerManager` - 玩家数据管理器
- `ServerPlayerData` - 玩家数据结构
- `Player` - 玩家实体类
- `EntitySelector` - 选择器参数容器
- `IntRange`, `FloatRange` - 范围过滤器

### 内部依赖

- `common/command/arguments/EntityArgument.hpp` - EntitySelector 定义
- `common/entity/entities/player/Player.hpp` - Player 类
- `common/core/Types.hpp` - 基础类型定义

## 测试用例

测试文件位于 `tests/server/command/PlayerResolverTest.cpp`，涵盖：

- **IntRange 测试** - 等级范围边界条件
- **FloatRange 角度测试** - 角度范围环绕处理（-180/180 边界）
- **EntitySelector 角度测试** - x_rotation/y_rotation 设置和过滤
- **基础解析测试** - 空列表、用户名匹配、自选
- **排序测试** - 最近、最远排序
- **距离过滤测试** - 距离范围过滤
- **游戏模式过滤测试** - 各种游戏模式匹配和排除
- **限制测试** - 结果数量限制

**FloatRange 角度测试覆盖**：

| 测试用例 | 测试内容 |
|----------|----------|
| `UnboundedRangeAcceptsAnyAngle` | 无界范围接受任意角度 |
| `NormalRangeNoWraparound` | 普通范围 [10, 30] 不处理环绕 |
| `WraparoundRange` | 跨越边界范围 [170, -170] |
| `WraparoundRangeLarge` | 大范围跨越 [90, -90] |
| `PitchRangeNegative90To90` | 俯仰角范围 [-45, 45] |
| `OnlyMinBound` | 只有最小值 [45, ...] |
| `OnlyMaxBound` | 只有最大值 [..., 90] |
| `AngleNormalization` | 角度规范化（270°→-90°） |
| `ExactAngleMatch` | 精确角度匹配 [45, 45] |
| `FullCircleRange` | 完整圆 [-180, 180] |
| `FullCircleRangeWraparound` | 近完整圆 [-179, 179] |

## 容易踩的坑

### 1. 等级过滤需要世界实例

等级过滤需要通过 `ServerWorld` 获取玩家实体。如果 `ServerCommandSource::world()` 返回 `nullptr`，等级过滤将被跳过。

```cpp
// 注意：控制台命令可能没有 world
ServerCommandSource consoleSource = ServerCommandSource::forConsole(server);
// consoleSource.world() == nullptr
// 等级过滤将被跳过！
```

### 2. 玩家实体可能不存在

玩家可能只存在于 `PlayerManager` 但没有创建实体（如正在登录中）。代码已处理这种情况。

### 3. 游戏模式参数格式

游戏模式支持两种格式：
- 名称格式：`"survival"`, `"creative"`, `"adventure"`, `"spectator"`
- 数字格式：`"0"`, `"1"`, `"2"`, `"3"`

### 4. 异步环境安全

`ServerPlayerEntityManager::getPlayerEntity()` 返回的指针是临时的，不应存储。应在调用后立即使用返回值。

## 更新历史

- **2026-05-11**: 实现 x_rotation/y_rotation 角度范围过滤，添加 FloatRange::testAngle() 方法处理角度环绕
- **2026-05-09**: 实现经验等级过滤（`level=` 参数），修复 TODO
- **初始版本**: 实现基础选择器解析（用户名、游戏模式、距离、排序、限制）
