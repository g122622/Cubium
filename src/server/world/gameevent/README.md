# 游戏事件系统 - 服务端实现

本目录包含游戏事件系统中依赖服务端（`server`）模块的实现文件。

## 目录说明

由于 `src/common/` 不得依赖 `src/server/`（架构分层约束），所有头文件（`.hpp`）位于
`src/common/world/gameevent/`，而需要引用 `ServerWorld`、`ServerChunkManager` 等服务端类型的
实现文件（`.cpp`）则放置在本目录中。

## 文件列表

| 文件 | 说明 |
|------|------|
| `PositionSource.cpp` | `EntityPositionSource::getPosition()` 实现（需查询服务端实体管理器） |
| `GameEventListenerRegistry.cpp` | `EuclideanGameEventListenerRegistry` 实现（需查询服务端世界坐标） |
| `GameEventDispatcher.cpp` | `GameEventDispatcher` 实现（需访问服务端区块管理器） |
| `DynamicGameEventListener.cpp` | `DynamicGameEventListener` 实现（需访问服务端区块管理器） |
| `VibrationSystem.cpp` | `VibrationSystem::Ticker/Listener/User` 实现（需访问服务端世界、玩家状态、方块标签、区块加载级别） |

## VibrationSystem 服务端实现详情

`VibrationSystem.cpp` 实现了以下关键方法：

- **`User::isValidVibration()`** — 振动有效性验证，检查顺序：
  1. 事件频率为 0 则拒绝（非振动事件）
  2. 旁观者模式玩家拒绝
  3. 潜行实体 + `isIgnoredBySneaking()` 事件拒绝（潜行可忽略的事件：HIT_GROUND、PROJECTILE_SHOOT、STEP、SWIM、ITEM_INTERACT_START、ITEM_INTERACT_FINISH）
  4. `dampensVibrations()` 实体拒绝（如监守者、羊毛物品实体）
  5. `BlockTags::DAMPENS_VIBRATIONS` 方块拒绝（羊毛方块、羊毛地毯）
- **`Ticker::receiveVibration()`** — 振动接收处理，包含 `requiresAdjacentChunksToBeTicking()` 区块加载级别检查（监听器周围 3x3 区块必须全部处于 BlockTicking 级别且已加载，检查不通过时不清除振动，等待下次 tick 重试）

## 依赖关系

```
common/world/gameevent/*.hpp  (接口与类型定义，仅前向声明 ServerWorld)
      ↑
server/world/gameevent/*.cpp  (实现，include ServerWorld、BlockTags、Player 等头文件)
```
