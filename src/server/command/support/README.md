# command/support 目录

## 目录结构

```
support/
├── PlayerResolver.hpp               # 玩家选择器解析接口（含 resolvePlayerName 辅助函数）
├── PlayerResolver.cpp               # 玩家选择器解析实现（@p/@a/@r/@s 及各种过滤条件）
├── EntityResolver.hpp               # 通用实体选择器解析接口（支持 @e 选择器和所有实体类型）
├── EntityResolver.cpp               # 通用实体选择器解析实现（type/tag/team/name/distance/volume/level/gamemode 过滤）
├── EffectResolver.hpp               # 效果类型解析接口
├── EffectResolver.cpp               # 效果类型解析实现（命令名称与EffectType的映射）
├── FunctionSuggestionProvider.hpp   # 函数参数 Tab 补全建议（查询 FunctionManager 提供函数名和标签名）
├── FunctionSuggestionProvider.cpp   # 函数建议实现
├── SpreadAlgorithm.hpp              # /spreadplayers 分散算法核心（SpreadPosition 结构、迭代分散函数，使用 IWorld 动态高度）
└── SpreadAlgorithm.cpp              # 分散算法实现（位置计算、安全检查、迭代推开逻辑，高度边界通过 IWorld::getMinBuildHeight() 获取）
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

- **PlayerResolver** - 将 EntitySelector 解析为实际的玩家 ID 列表，支持名称、游戏模式、距离、等级、角度、记分板、进度等过滤条件；提供 `resolvePlayerName()` 辅助函数，通过 PlayerId 获取真实玩家名称。仅处理玩家实体（@p/@a/@r/@s），不支持非玩家实体。
- **EntityResolver** - 将 EntitySelector 解析为通用实体指针列表（`std::vector<Entity*>`），支持所有实体类型包括非玩家实体（@e）。支持 type/tag/team/name/distance/dx/dy/dz/x_rotation/y_rotation/level/gamemode/scores/advancements 过滤条件。体积过滤（dx/dy/dz）使用实体碰撞箱与选择 AABB 的相交检查（`AABB.intersects(entity.boundingBox())`），而非位置点包含检查，以确保大型实体跨越选择边界时的正确行为。优先用于需要非玩家实体支持的命令。
- **EffectResolver** - 提供效果名称与 EffectType 枚举的双向转换

## 上下游外部依赖关系

### 上游依赖（本目录依赖的外部模块）

- `common/command/arguments/EntityArgument.hpp` - EntitySelector 定义
- `common/entity/core/Entity.hpp` - Entity 基类（getTypeId、getTags、position、pitch/yaw 等）
- `common/entity/entities/player/Player.hpp` - Player 类（获取经验等级、角度等）
- `common/entity/effect/EffectType.hpp` - 效果类型枚举
- `server/command/ServerCommandSource.hpp` - 命令源（提供 IServer、ServerWorld 访问）
- `server/world/ServerWorld.hpp` - 服务端世界（entityManager 空间查询）
- `server/entity/ServerPlayerEntityManager.hpp` - 玩家实体管理器
- `server/player/PlayerManager.hpp` - 玩家数据管理器
- `server/scoreboard/ServerScoreboard.hpp` - 记分板（scores 过滤）
- `common/advancement/AdvancementManager.hpp` - 进度管理器（advancements 过滤）

### 下游依赖（依赖本目录的模块）

- `server/command/commands/` 目录下的各类命令实现，如 `GamemodeCommand`、`EffectCommand`、`TpCommand`、`KillCommand`、`ClearCommand`、`DataCommand`、`TagCommand` 等

## 容易踩的坑

### 1. EntityResolver vs PlayerResolver 的选择

- 需要支持 @e（所有实体）选择器的命令应使用 EntityResolver（如 KillCommand、TagCommand、DataCommand）
- 仅处理玩家的命令可继续使用 PlayerResolver（如 GameModeCommand、TeleportCommand）
- EntityResolver 内部在处理 @p/@a/@r/@s 时复用了与 PlayerResolver 类似的逻辑，但直接返回 Entity* 而非 PlayerId

### 2. 等级过滤需要世界实例

等级过滤需要通过 `ServerWorld` 获取玩家实体。如果 `ServerCommandSource::world()` 返回 `nullptr`，等级过滤将被跳过。

### 3. 玩家实体可能不存在

玩家可能只存在于 `PlayerManager` 但没有创建实体（如正在登录中）。EntityResolver 的玩家收集逻辑通过 `ServerPlayerEntityManager` 获取在线玩家实体，已处理这种情况。

### 4. 游戏模式参数格式

游戏模式支持两种格式：
- 名称格式：`"survival"`, `"creative"`, `"adventure"`, `"spectator"`
- 数字格式：`"0"`, `"1"`, `"2"`, `"3"`

### 5. 异步环境安全

EntityResolver 返回的 `Entity*` 指针是临时的，不应跨 tick 存储。应在命令处理函数内立即使用返回值。

### 6. 角度环绕处理

`x_rotation`（俯仰角/pitch）和 `y_rotation`（偏航角/yaw）参数使用 `FloatRange::testAngle()` 处理 -180/180 度边界环绕。

### 7. NBT 和 predicate 过滤当前仅解析未完全实现

`nbt` 和 `predicate` 选择器参数已实现解析，但过滤逻辑待完善（依赖 Entity NBT 序列化和 LootConditionManager）。

### 8. resolvePlayerName 回退行为

`resolvePlayerName(source, playerId)` 在服务器不可用或玩家不在线时返回 `"player_<id>"` 格式的回退名称。命令层应优先使用此函数而非手动拼接。

### 9. 体积过滤使用 AABB 相交检查

EntityResolver 的体积过滤（dx/dy/dz 参数）使用 `AABB.intersects(entity.boundingBox())` 进行碰撞箱相交检查，而非位置点包含检查。`EntitySelector::createAabb()` 按 MC 原版逻辑构造相对 AABB（负值 delta 赋给 min 侧，正值赋给 max 侧，max 侧额外加 1.0），`EntityResolver` 中再平移到绝对坐标。当无 dx/dy/dz 但有 distance 最大值时，也从最大距离构造立方体 AABB 作为空间预过滤。
