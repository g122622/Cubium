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
| `VibrationSystemServer.cpp` | `VibrationSystem::Ticker/Listener/User` 服务端实现（需访问服务端世界、玩家状态、方块标签、区块加载级别、粒子系统、进度触发器） |

## VibrationSystem 服务端实现详情

`VibrationSystemServer.cpp` 实现了以下关键方法：

- **`User::isValidVibration()`** — 振动有效性验证，检查顺序：
  1. 事件频率为 0 则拒绝（非振动事件）
  2. 旁观者模式玩家拒绝
  3. 潜行实体 + `isIgnoredBySneaking()` 事件拒绝（潜行可忽略的事件：HIT_GROUND、PROJECTILE_SHOOT、STEP、SWIM、ITEM_INTERACT_START、ITEM_INTERACT_FINISH）
     - 当 `canTriggerAvoidVibration()` 返回 true 且源实体为玩家时，触发 `AvoidVibrationTrigger` 进度（对应 MC 原版 `CriteriaTriggers.AVOID_VIBRATION.trigger()`）
  4. `dampensVibrations()` 实体拒绝（如监守者、羊毛物品实体）
  5. `BlockTags::DAMPENS_VIBRATIONS` 方块拒绝（羊毛方块、羊毛地毯）
- **`Listener::handleGameEvent()`** — 游戏事件处理入口，验证振动有效性并调度
- **`Listener::forceScheduleVibration()`** — 强制调度振动（不验证有效性）
- **`Listener::scheduleVibration()`** — 振动调度，将候选振动添加到选择器
- **`Ticker::tick()`** — 每 tick 驱动振动传播：
  1. `tryReloadVibrationParticle()` — 区块重载后重发振动粒子
  2. 从选择器选择候选振动
  3. 递减传播时间，归零时调用 `receiveVibration()`
- **`Ticker::trySelectAndScheduleVibration()`** — 从选择器选择最佳候选振动，设置传播时间，并在振动源位置发送 `ParticleTypeId::Vibration` 粒子效果
- **`Ticker::receiveVibration()`** — 振动接收处理，包含 `requiresAdjacentChunksToBeTicking()` 区块加载级别检查（监听器周围 3x3 区块必须全部处于 BlockTicking 级别且已加载，检查不通过时不清除振动，等待下次 tick 重试）
- **`Ticker::tryReloadVibrationParticle()`** — 区块重新加载后重发振动粒子，通过插值计算粒子当前位置（源位置 → 监听器位置，按传播进度插值）

## AvoidVibrationTrigger 集成

当玩家潜行成功避免振动时，`isValidVibration()` 会触发 `AvoidVibrationTrigger` 进度。
触发路径：

```
VibrationSystem::User::isValidVibration()
  → 源实体正在潜行 && isIgnoredBySneaking(event)
    → canTriggerAvoidVibration()
      → ServerPlayer::getAdvancements()
        → CriterionTriggers::instance().getTrigger<AvoidVibrationTrigger>()
          → AbstractCriterionTrigger<AvoidVibrationTriggerInstance>::trigger(*advancements, predicate)
```

注意：由于 `isValidVibration()` 是 `const` 方法，但触发进度需要修改玩家状态，
因此使用了 `const_cast<Entity*>` 来获取非 const 指针。这在语义上是安全的，
因为 MC 原版 Java 中 `isValidVibration` 不是 const 方法，直接调用 `CriteriaTriggers.AVOID_VIBRATION.trigger()`。

## 振动粒子效果

振动系统在两个位置发送粒子：

1. **`trySelectAndScheduleVibration()`** — 振动被选中时，在振动源位置发送 `ParticleTypeId::Vibration` 粒子
2. **`tryReloadVibrationParticle()`** — 区块重新加载后，在插值位置重发粒子（基于传播进度计算源→监听器之间的当前位置）

当前粒子使用简单的 `addParticle()` 发送，不支持 MC 原版的定向飞行粒子效果
（需要 `VibrationParticleOption` 携带 PositionSource 和 arrivalInTicks 参数，
待粒子系统扩展后实现）。

## 依赖关系

```
common/world/gameevent/*.hpp  (接口与类型定义，仅前向声明 ServerWorld)
      ↑
server/world/gameevent/*.cpp  (实现，include ServerWorld、BlockTags、Player 等头文件)
```
