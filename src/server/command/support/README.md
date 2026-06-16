# command/support 目录

## 目录结构

```
support/
├── PlayerResolver.hpp         # 玩家选择器解析接口（含 resolvePlayerName 辅助函数）
├── PlayerResolver.cpp         # 玩家选择器解析实现（@p/@a/@r/@s 及各种过滤条件）
├── EffectResolver.hpp         # 效果类型解析接口
├── EffectResolver.cpp         # 效果类型解析实现（命令名称与EffectType的映射）
├── SpreadAlgorithm.hpp        # /spreadplayers 分散算法核心（SpreadPosition 结构、迭代分散函数，使用 IWorld 动态高度）
└── SpreadAlgorithm.cpp        # 分散算法实现（位置计算、安全检查、迭代推开逻辑，高度边界通过 IWorld::getMinBuildHeight() 获取）
```

## 内部模块关系

```
┌─────────────────────────────────────────────────────────────┐
│                      Command System                          │
├─────────────────────────────────────────────────────────────┤
│                                                              │
│  ┌──────────────────┐    ┌──────────────────┐               │
│  │   Commands       │───▶│   Resolvers      │               │
│  │  (使用选择器)     │    │   (解析选择器)    │               │
│  └──────────────────┘    └──────────────────┘               │
│                                                          │
└─────────────────────────────────────────────────────────────┘
```

- **PlayerResolver** - 将 EntitySelector 解析为实际的玩家 ID 列表，支持名称、游戏模式、距离、等级、角度、记分板、进度等过滤条件；提供 `resolvePlayerName()` 辅助函数，通过 PlayerId 获取真实玩家名称
- **EffectResolver** - 提供效果名称与 EffectType 枚举的双向转换

## 上下游外部依赖关系

### 上游依赖（本目录依赖的外部模块）

- `common/command/arguments/EntityArgument.hpp` - EntitySelector 定义
- `common/entity/entities/player/Player.hpp` - Player 类（获取经验等级、角度等）
- `common/entity/effect/EffectType.hpp` - 效果类型枚举
- `server/command/ServerCommandSource.hpp` - 命令源（提供 IServer、ServerWorld 访问）
- `server/world/ServerWorld.hpp` - 服务端世界
- `server/entity/ServerPlayerEntityManager.hpp` - 玩家实体管理器
- `server/player/PlayerManager.hpp` - 玩家数据管理器
- `server/scoreboard/ServerScoreboard.hpp` - 记分板（scores 过滤）
- `common/advancement/AdvancementManager.hpp` - 进度管理器（advancements 过滤）

### 下游依赖（依赖本目录的模块）

- `server/command/commands/` 目录下的各类命令实现，如 `GamemodeCommand`、`EffectCommand`、`TpCommand`、`KillCommand`、`ClearCommand` 等

## 容易踩的坑

### 1. 等级过滤需要世界实例

等级过滤需要通过 `ServerWorld` 获取玩家实体。如果 `ServerCommandSource::world()` 返回 `nullptr`，等级过滤将被跳过。

```cpp
// 注意：控制台命令可能没有 world
ServerCommandSource consoleSource = ServerCommandSource::forConsole(server);
// consoleSource.world() == nullptr → 等级过滤将被跳过
```

### 2. 玩家实体可能不存在

玩家可能只存在于 `PlayerManager` 但没有创建实体（如正在登录中）。`resolvePlayerIds` 已处理这种情况，不会崩溃。

### 3. 游戏模式参数格式

游戏模式支持两种格式：
- 名称格式：`"survival"`, `"creative"`, `"adventure"`, `"spectator"`
- 数字格式：`"0"`, `"1"`, `"2"`, `"3"`

### 4. 异步环境安全

`ServerPlayerEntityManager::getPlayerEntity()` 返回的指针是临时的，不应存储。应在调用后立即使用返回值。

### 5. 角度环绕处理

`x_rotation`（俯仰角/pitch）和 `y_rotation`（偏航角/yaw）参数使用 `FloatRange::testAngle()` 处理 -180/180 度边界环绕：
- 角度值通过 `wrapDegrees()` 规范化到 [-180, 180) 范围
- 当 `min > max` 时表示范围跨越边界（如 `[170..-170]` 表示接近正北方向）

### 6. NBT 和 predicate 过滤当前仅解析未完全实现

`nbt` 和 `predicate` 选择器参数已实现解析，但过滤逻辑待完善（依赖 Entity NBT 序列化和 LootConditionManager）。

### 7. resolvePlayerName 回退行为

`resolvePlayerName(source, playerId)` 在服务器不可用或玩家不在线时返回 `"player_<id>"` 格式的回退名称。命令层应优先使用此函数而非手动拼接 `"player_" + std::to_string(playerId)`，以确保玩家在线时使用真实名称、离线时有统一回退。
