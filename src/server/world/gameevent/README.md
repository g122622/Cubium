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

## 振动遮挡检测

振动系统包含遮挡检测逻辑，用于判断振动信号是否被遮挡方块（如羊毛）阻挡。算法如下：

1. 将振动源位置和监听器位置对齐到方块中心（`floor + 0.5`）
2. 从源方块中心沿 6 个方向各偏移 1e-5（避免射线起点在方块边界上）
3. 对每个偏移起点，使用 `isBlockInLine()` 向监听器方块中心发射射线
4. 如果所有 6 个方向的射线都命中了 `OCCLUDES_VIBRATION_SIGNALS` 标签方块，则振动被遮挡

`isBlockInLine()` 是 `IWorld` 接口的虚方法，使用 DDA 算法沿两点之间的直线逐格遍历方块，对每个经过的方块调用谓词检查。`ServerWorld` 提供了具体实现。

**标签区别**：
- `OCCLUDES_VIBRATION_SIGNALS`：仅包含羊毛方块，用于遮挡检测（路径阻挡）
- `DAMPENS_VIBRATIONS`：包含羊毛 + 羊毛地毯，用于源点阻尼检测（源头吸收）

## VibrationSystem 服务端实现详情

`VibrationSystemServer.cpp` 实现了以下关键方法：

- **`User::isValidVibration()`** — 振动有效性验证，检查顺序：
  1. 事件频率为 0 则拒绝（非振动事件）
  2. 旁观者模式玩家拒绝
  3. 潜行实体 + `isIgnoredBySneaking()` 事件拒绝（潜行可忽略的事件：HIT_GROUND、PROJECTILE_SHOOT、STEP、SWIM、ITEM_INTERACT_START、ITEM_INTERACT_FINISH）
     - 当 `canTriggerAvoidVibration()` 返回 true 且源实体为玩家时，触发 `AvoidVibrationTrigger` 进度（对应 MC 原版 `CriteriaTriggers.AVOID_VIBRATION.trigger()`）
  4. `dampensVibrations()` 实体拒绝（如监守者、羊毛物品实体）
  5. `BlockTags::DAMPENS_VIBRATIONS` 方块拒绝（羊毛方块、羊毛地毯）
- **`Listener::handleGameEvent()`** — 游戏事件处理入口，验证振动有效性并调度。包含振动遮挡检测：如果振动源被 `OCCLUDES_VIBRATION_SIGNALS` 标签方块（羊毛）从所有 6 个方向完全包围，则振动信号被遮挡，无法传播到监听器
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

### tryReloadVibrationParticle 集成说明

`tryReloadVibrationParticle()` 检查 `data.shouldReloadVibrationParticle()` 标志，
该标志在 `VibrationSystem::Data` 的存档加载构造函数中被设为 `true`。

在 MC 原版中，`Data.CODEC` 反序列化时硬编码 `reloadVibrationParticle = true`，
由以下方块/实体在 NBT 加载时触发：
- `SculkSensorBlockEntity.load()` → `read("listener", Data.CODEC)`
- `SculkShriekerBlockEntity.load()` → `read("listener", Data.CODEC)`
- `Warden.load()` → `read("listener", Data.CODEC)`
- `Allay.load()` → `read("listener", Data.CODEC)`

当前项目中这些方块/实体尚未实现，因此 `tryReloadVibrationParticle()` 暂时不会执行实际逻辑。
当实现上述方块/实体的 NBT 序列化时，应使用 `Data(currentVibration, selector, travelTime, true)`
构造函数或调用 `data.setReloadVibrationParticle(true)` 来设置重载标志。

## 依赖关系

```
common/world/gameevent/*.hpp  (接口与类型定义，仅前向声明 ServerWorld)
      ↑
server/world/gameevent/*.cpp  (实现，include ServerWorld、BlockTags、Player 等头文件)
```
