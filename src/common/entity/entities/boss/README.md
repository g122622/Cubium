# Boss实体 (Boss Entities)

本目录包含Boss级怪物的实现。

## 目录结构

```
boss/
├── EnderDragonEntity.hpp/cpp  # 末影龙 + EnderDragonPartEntity + BossEntity基类
├── WardenAngerLevel.hpp       # 监守者怒气等级枚举与工具函数
├── WardenEntity.hpp/cpp       # 监守者（来自1.19荒野更新，由SculkShrieker召唤）
├── WitherEntity.hpp/cpp       # 凋灵 + WitherDoNothingGoal + WitherRandomFlyGoal
└── README.md                  # 本文档
```

## 内部模块关系

```
┌─────────────────┐
│   BossEntity    │ (末影龙、凋灵的公共基类)
│ extends MobEntity│
└────────┬────────┘
         │
    ┌────┴────┐
    │         │
    ▼         ▼
┌───────────────┐    ┌─────────────────┐
│EnderDragonEntity│    │  WitherEntity   │
│                │    │                 │
│ 管理多个        │    │ implements      │
│ EnderDragonPart│    │ IRangedAttackMob│
└───────┬────────┘    └─────────────────┘
        │
        ▼
┌───────────────────┐
│EnderDragonPartEntity│ (龙部件碰撞体：头/颈/身/尾/翼)
│ extends Entity    │
└───────────────────┘
```

**继承层次**：
- `Entity` → `LivingEntity` → `MobEntity` → `BossEntity` → `EnderDragonEntity`
- `Entity` → `LivingEntity` → `MobEntity` (+ `IRangedAttackMob`) → `WitherEntity`
- `Entity` → `LivingEntity` → `MobEntity` → `CreatureEntity` → `MonsterEntity` → `WardenEntity`
- `Entity` → `EnderDragonPartEntity`（独立的龙部件实体）

## 上下游外部依赖关系

**依赖的模块**：
- `entity/core/`：Entity、LivingEntity、MobEntity 基类，DataParameter 数据同步
- `entity/ai/goal/`：Goal 系统（WitherDoNothingGoal、WitherRandomFlyGoal 等）
- `entity/ai/controller/FlyingMovementController`：飞行移动控制器（凋灵专用 maxTurn=10）
- `entity/interfaces/IRangedAttackMob.hpp`：远程攻击接口（凋灵）
- `entity/damage/DamageSource.hpp`：伤害来源
- `entity/effect/`：药水效果（凋灵免疫判断）
- `world/IWorld.hpp`：世界接口

**被哪些模块使用**：
- `server/world/ServerWorld.hpp`：Boss 生成和管理
- `client/renderer/`：Boss 生命条渲染
- `entity/entities/effect/EnderCrystalEntity.hpp`：末影水晶与末影龙的关联

## 容易踩的坑

### 1. 末影龙部件位置同步

末影龙有 8 个碰撞部件（头、颈、身、尾1-3、左翼、右翼），每个部件都是独立的 `EnderDragonPartEntity`。部件位置需要在 `tick()` 中根据龙的位置和动画状态更新，使用环形缓冲区存储历史位置用于颈部和尾部动画插值。**部件不是独立的实体，不会保存到存档，不要在部件上调用会触发存档的方法**。

### 2. 凋灵三头目标追踪

凋灵有三个独立追踪目标的头，通过 `EntityDataManager` 同步到客户端。主头追踪 `attackTarget`，侧头每 10-20 tick 搜索范围内最近的非亡灵生物。**创造模式和旁观者模式的玩家不会被作为目标**，这一点在 `updateHeadTargets()` 中实现。

### 3. 凋灵无敌阶段

凋灵生成后有 220 tick (11秒) 的无敌阶段，期间：
- 不能移动、跳跃、看向
- 不能进行远程攻击（`canRangedAttack()` 返回 false）
- 持续恢复生命值
- 通过 `WitherDoNothingGoal` 阻止所有行动

### 4. 凋灵方块破坏

凋灵受伤后会破坏周围 3x4x3 范围内的方块，有 20 tick 冷却。**方块破坏受 `mobGriefing` 游戏规则控制**，需要检查 `IWorld::getGameRules()`。使用 `BlockTags::WITHER_IMMUNE` 标签判断不可破坏方块。破坏方块后调用 `spawnAfterBreak(nullptr, false)`，使得虫蚀方块等特殊方块能正确触发生成逻辑。

### 5. 凋灵充能状态

当凋灵生命值低于一半时进入充能状态（`isCharged()`），此时：
- 免疫箭矢伤害
- 发射蓝色凋灵之首（破坏方块）
- 外观产生蓝色护盾效果
- 充能时每个头 1/4 概率额外生成黄绿色 EntityEffect 粒子 (0.7, 0.7, 0.5)
- 无敌阶段每 8 tick 生成紫色 EntityEffect 粒子 (0.7, 0.7, 0.9)
- 注意：EntityEffect 粒子颜色通过 velocity 向量 (R, G, B) 传递

### 6. 凋灵飞行移动系统

凋灵使用 `FlyingMovementController(this, 10, false)` 控制飞行行为，与恶魂和恼鬼的飞行控制器不同：
- 俯仰角旋转速率 10 度/tick，偏航角 90 度/tick
- `hoversInPlace=false`：空闲时恢复重力，凋灵会缓慢下落
- `_updateFlightBehavior()` 在 `aiStep()` 中执行（重写了 `LivingEntity::aiStep()`），在 `LivingEntity::aiStep()` 之前调用，匹配 MC Java 的 `WitherBoss.aiStep()` 调用顺序。这样 `FlyingMovementController.tick()` 的旋转限制能正确覆盖 `_updateFlightBehavior()` 的直接 rotation 设置
- Y轴阻尼、目标追踪推力、自动面向运动方向的逻辑在 `_updateFlightBehavior()` 中实现
- `WitherRandomFlyGoal` 在无敌阶段外以 0.001 概率随机选择飞行目标（避水避岩浆），因 WitherEntity 继承自 MobEntity 而非 CreatureEntity，不能复用 `WaterAvoidingRandomFlyingGoal`
- 凋灵注册了 FLYING_SPEED 属性（0.6），供 FlyingMovementController 飞行时使用

### 6.5 凋灵侧头独立朝向计算

凋灵有三个独立的头，其中主头（index 0）由 `LookController` 控制朝向攻击目标，两侧头（index 0=左、1=右）需要独立追踪各自的 `HEAD_TARGET_2`/`HEAD_TARGET_3` 目标。对应 MC 1.21.11 `WitherBoss.aiStep()` 中 `j=0..1` 的循环：

**数据流（服务端权威计算）**：

```
WitherEntity::aiStep()
  ├─ _updateFlightBehavior()          // 1. 飞行行为（在 LivingEntity::aiStep 之前）
  ├─ LivingEntity::aiStep()           // 2. 父类物理/AI
  ├─ 备份 m_prevHeadXRot/YRot[2]      // 3. 备份上一 tick 侧头角度（供渲染插值）
  └─ _updateSideHeadRotations()       // 4. 计算侧头朝向
       ├─ j=0: 读 HEAD_TARGET_2 → 查目标实体
       │    ├─ 有目标: dx/dy/dz → atan2 → targetYaw/Pitch → _rotLerp 逼近
       │    │    pitch 限速 40°/tick, yaw 限速 10°/tick
       │    └─ 无目标: yaw _rotLerp 朝 bodyRot(renderYawOffset) 逼近, pitch 不变
       └─ j=1: 读 HEAD_TARGET_3 → 同上
```

**关键方法**：

- `_rotLerp(current, target, maxStep)`：MC `rotlerp` 等价实现，委托 `math::clampedRotate`。计算 `wrapDegrees(target - current)` 后 `clamp` 到 `[-maxStep, maxStep]`，返回 `current + clamped`（结果**不**包装到 `[-180, 180)`）。
- `_updateSideHeadRotations()`：镜像 MC `WitherBoss.aiStep()` 的 `j=0..1` 循环。头部位置通过 `_getHeadX/Y/Z(j+1)` 计算（使用 `renderYawOffset() + 180*(head-1)` 角度偏移，偏移 1.3 格）。目标实体眼睛 Y 坐标通过 `Entity::getEyeY()`（= `y() + eyeHeight()`）获取。
- `_getHeadX/Y/Z(head)`：`head<=0` 为主头（偏移 3.0），`head>=1` 为侧头（偏移 2.2，水平 1.3 格偏移）。`getScale()` 对凋灵恒为 1.0（无幼体凋灵）。

**公共访问器**（供渲染层读取）：

- `sideHeadPitch/Yaw(index)` / `prevSideHeadPitch/Yaw(index)`：返回当前/上一 tick 的侧头角度（度，不包装）。
- `getHeadTarget1/2/3ParamId()` / `getInvulTimeParamId()`：静态方法，返回 `DataParameter` 的 ID，供 `ClientEntity::syncMetadataFromDataManager` 读取网络同步的 `HEAD_TARGET` 值。

**客户端镜像**：由于 `ClientEntity` 不继承 `Entity`/`WitherEntity` 且 `WitherEntity::aiStep()` 不在客户端运行，客户端在 `ClientEntity::tickWitherSideHeads()` 中独立镜像此计算（详见 `src/client/world/entity/README.md`）。侧头角度本身**不**网络同步——只有 `HEAD_TARGET_1/2/3`（目标实体 ID）通过 `ir::play::SetEntityData` 同步，客户端根据目标 ID 本地重算朝向。

**与 MC 原版的一致性**：
- yaw 限速 10°/tick、pitch 限速 40°/tick（MC `rotlerp(xRotHeads[j], f2, 40)` / `rotlerp(yRotHeads[j], f1, 10)`）
- 无目标时 yaw 回正到 `yBodyRot`（Cubium `renderYawOffset()`），pitch 不变
- 头部位置偏移 1.3 格、Y 偏移 2.2（侧头）/ 3.0（主头），与 MC `getHeadX/Y/Z` 一致

### 6. Boss 生命条显示范围

`BossEntity::getHealthBarRange()` 定义了玩家可以看到 Boss 生命条的最大距离：
- 末影龙：256 格（末地战斗需要远距离可见）
- 凋灵：默认 100 格

### 7. 末影龙阶段系统

末影龙使用阶段系统控制行为，阶段定义在 `EnderDragonEntity::Phase` 枚举中。阶段切换需要通过 `setPhase()` 方法，不要直接修改内部状态。栖息阶段（SittingFlaming、SittingScanning、SittingAttacking）需要检查龙是否在末地传送门上方。

### 8. 末影水晶关联

末影龙通过 `closestEnderCrystal()` 和 `setClosestEnderCrystal()` 与末影水晶关联。末影水晶在 `healDragon()` 中设置此引用，用于渲染光束效果和治愈逻辑。**末影水晶被破坏时会触发 `onCrystalDestroyed()` 回调**。

### 9. Boss 条网络同步

Boss 生命条需要在客户端-服务端之间同步显示状态。服务端通过数据包发送 Boss 的当前生命值、名称、可见性等信息，客户端接收后渲染。Boss 实体被移除时需要清理客户端的 Boss 条显示。

### 10. EnderDragonPartEntity 不是完整实体

龙部件继承自 `Entity` 而非 `LivingEntity`，它们：
- 没有独立的生命值
- 不参与 AI 系统
- 不保存到 NBT
- 碰撞检测转发到父龙（`attackEntityPartFrom`）

**不要对龙部件调用 `hurt()` 或其他 LivingEntity 的方法**。

### 11. 末影龙/凋灵方块破坏与 spawnAfterBreak

末影龙的 `_destroyBlocksInAABB` 和凋灵的 `_breakNearbyBlocks` 在破坏方块后调用 `block.spawnAfterBreak(world, pos, *oldState, nullptr, false)`。这确保了虫蚀方块（InfestedBlock）等特殊方块在实体破坏时能正确触发生成逻辑（如蠹虫）。调用顺序：先 `setBlockState(airState, 3)` 移除方块，再调用 `spawnAfterBreak`，与 MC Java 行为一致。

**末影龙方块破坏规则**：末影龙使用 `BlockTags::DRAGON_IMMUNE` 标签判断不可破坏方块（基岩、黑曜石、末地石、铁栏杆、末地传送门等），使用 `BlockTags::DRAGON_TRANSPARENT` 标签判断龙透明方块（光照方块）。方块破坏受 `mobGriefing` 游戏规则控制。碰到 `DRAGON_IMMUNE` 方块后设置碰墙标志影响飞行行为。凋灵使用 `BlockTags::WITHER_IMMUNE` 标签。

### 12. 末影龙伤害来源限制

MC 原版中末影龙只接受两种伤害：
1. **玩家直接攻击**（`DamageSource.isPlayerSource() == true`）
2. **爆炸伤害**（`DamageSource.isExplosion() == true`），对应 MC 的 `ALWAYS_HURTS_ENDER_DRAGONS` 标签

非头部伤害减伤公式为 `damage / 4.0 + min(damage, 1.0)`，而非简单的 50% 减伤。末影水晶爆炸对龙造成 10 点 `IndirectEntityDamageSource(DamageType::Explosion, crystal, player)` 伤害，仅当被破坏的水晶是龙当前绑定的最近水晶时触发。

### 13. 末影龙碰墙减速（m_slowed）

`m_slowed` 标志（MC 原版 `inWall`）控制龙的飞行减速行为：

- **设置时机**：`_collideWithEntities()` 中检查龙头、颈、身三个部件的碰撞箱，调用 `_destroyBlocksInAABB()` 检测碰墙。若碰到 `DRAGON_IMMUNE` 方块或 `mobGriefing` 关闭时的方块，`m_slowed` 被设置为 `true`
- **影响**：当 `m_slowed` 为 `true` 时，翅膀扇动速度减半（`m_animTime` 增量从 0.01 降为 0.005）。MC 原版还会将移动速度乘以 0.8（待阶段系统实现后接入）
- **重置**：每帧在 `_collideWithEntities()` 中重新计算，若未碰到不可破坏方块则自动重置为 `false`

### 14. 末影水晶破坏的玩家归属

`onCrystalDestroyed()` 中，伤害来源的玩家归属按 MC 原版逻辑处理：

1. 如果 `source.getEntity()` 是 `Player`，直接使用该玩家作为 `causeEntity`
2. 否则，调用 `world->getClosestPlayer(crystalPos, 64.0f)` 搜索水晶位置 64 格内最近的玩家（对应 MC 的 `CRYSTAL_DESTROY_TARGETING`）
3. 如果两者都找不到玩家，`causePlayer` 为 `nullptr`，爆炸伤害使用 `DamageSources::explosion(crystal, nullptr)`

### 15. 末影龙死亡动画（`_onDeathUpdate` / `tickDeath` 重写）

末影龙拥有自定义的 200 tick（10 秒）死亡动画，与普通生物的 20 tick 死亡动画不同。实现要点：

**`tickDeath()` 重写**：`EnderDragonEntity` 重写了 `LivingEntity::tickDeath()`，委托给私有方法 `_onDeathUpdate()`。这一重写是**必须的**——否则 `LivingEntity::tickDeath()` 会在 `isDead()` 后 20 tick 调用 `remove()`，导致 200 tick 动画从未播放。`tickDeath()` 由 `LivingEntity::tick()` 在 `isDead()`（`m_health <= 0`）时自动调用，无需在 `EnderDragonEntity::tick()` 中显式调用 `_onDeathUpdate()`。

**死亡动画时序**（对齐 MC 1.21.11 `EnderDragon.tickDeath`）：

| 时刻（dragonDeathTime） | 行为 |
|--------------------------|------|
| 1（且 `!isSilent()`） | 广播世界事件 `DRAGON_DEATH_SOUND`（1028）+ 播放 `ENTITY_ENDER_DRAGON_DEATH` 音效 |
| 每 tick | 龙 `move(MoverType::Self, (0, 0.1, 0))` 上升；所有子部件同步位移 |
| `[180, 200]` 每 tick | 生成 `HugeExplosion` 粒子（MC `EXPLOSION_EMITTER`），位置 `pos + (±4, +2±2, ±4)`，速度 `(0,0,0)` |
| `> 150` 且 `% 5 == 0` | 阶段性经验掉落：`floor(totalXP * 0.08)`，共 10 次（155、160、…、195、**200**） |
| 200 | 最终经验掉落 `floor(totalXP * 0.2)`；`EndDragonFight::setDragonKilled()` 放置龙蛋/折跃门/出口传送门；`remove()`；触发 `ENTITY_DIE` 游戏事件 |

**经验掉落总量**：首次击杀 12000，后续击杀 500。经验掉落受 `DO_MOB_LOOT` 游戏规则控制（MC 1.21.11 的 `mob_drops`），关闭时跳过所有掉落。注意 tick 200 同时满足阶段性条件（`>150 && %5==0`）和最终条件（`==200`），因此 tick 200 会同时触发 `floor(totalXP * 0.08)` 和 `floor(totalXP * 0.2)` 两次掉落。阶段掉落（10 次 × 8%）+ 最终掉落（20%）= 100%，与 MC 原版一致。

**子部件同步**：MC 在每个 tick 对 `subEntities` 调用 `setOldPosAndRot()` + `setPos(pos + vec3)`。Cubium 的 `Entity::setPosition()` 内部将 `m_prevPosition` 更新为当前位置再设置新位置，等价于 MC 的组合调用，因此直接遍历 `m_dragonParts` 调用 `setPosition(part.pos + riseVelocity)` 即可。

**`_updateDragonParts()` 调用顺序**：Cubium 在 `EnderDragonEntity::tick()` 中**先**调用 `_updateDragonParts()`，**再**调用 `BossEntity::tick()`（其内部 `LivingEntity::tick()` → `tickDeath()` → `_onDeathUpdate()`）。这一顺序对齐 MC 1.21.11：MC 在 `LivingEntity.tick()` → `aiStep()` 中通过 `tickPart()` 更新部件位置，随后 `tickDeath()` 执行死亡动画的部件位移。若 `_updateDragonParts()` 在 `tickDeath()` 之后调用，会覆盖死亡动画的部件位移，导致部件不跟随龙的上升。

**与 MC 的差异**：

- Cubium 没有 `globalLevelEvent`（全局广播世界事件），`DRAGON_DEATH_SOUND`（1028）通过 `playEvent` 广播给附近客户端（范围有限），同时通过 `playSound(ENTITY_ENDER_DRAGON_DEATH, volume=5.0)` 显式播放音效以近似 MC 的全局广播。
- MC 原版通过 `DragonDeathPhase` 在龙飞回祭坛后才将生命值设为 0 触发 `tickDeath`；Cubium 未实现 `DragonDeathPhase`，龙生命值降为 0 后立即开始死亡动画。

## 参考

- MC 1.21.11 `net.minecraft.world.entity.boss.enderdragon.EnderDragon`（`tickDeath` 死亡动画）
- MC 1.21.11 `net.minecraft.world.entity.boss.enderdragon.EnderDragonPart`（子部件）
- MC 1.21.11 `net.minecraft.world.entity.boss.enderdragon.phases.DragonDeathPhase`（死亡阶段，Cubium 未实现）
- MC 1.21.11 `net.minecraft.world.level.dimension.end.EndDragonFight`（`setDragonKilled`/`updateDragon`）
- MC 1.16.5 `EnderDragonEntity`（历史参考）
- MC 1.16.5 `EnderDragonPartEntity`
- MC 1.16.5 `WitherEntity`
- MC 1.16.5 `DragonPhaseManager`
- MC 1.21.11 `net.minecraft.world.entity.monster.warden.Warden`
- MC 1.21.11 `net.minecraft.world.entity.monster.warden.WardenAi`

## 15. 监守者（WardenEntity）实现状态

监守者出自 Minecraft 1.19 "荒野更新"，由 `SculkShriekerBlockEntity` 在警告等级达到 4 时召唤。
本实现为 **最小可召唤版本**，仅满足 `SculkShriekerHelper._trySummonWarden()` 的契约：

- `EntityRegistry.getType("minecraft:warden")` 返回非空
- `EntityType::canSummon()` 返回 true
- `EntityType::create(&world)` 返回有效的 `WardenEntity` 实例

### 已实现

| 特性 | 来源 |
|------|------|
| 实体类型注册（`minecraft:warden`）| `VanillaEntities.hpp` |
| 尺寸 0.9×2.9，眼睛高度 2.4 | MC 1.21.11 `Warden.getDefaultDimensions` |
| 属性：HP 500 / 速度 0.3 / 击退抗性 1.0 / 攻击击退 1.5 / 攻击 30 / 跟随 24 | MC 1.21.11 `Warden.createAttributes` |
| 免疫溺水、凋零伤害 | MC 1.21.11 `Warden.isInvulnerableTo` 简化版 |
| 摔落免疫 | MC 1.21.11 `Warden` 摸索阶段免疫的延伸 |
| `dampensVibrations()` 返回 true | MC 1.21.11 `Warden.dampensVibrations` |
| `isNonBoss()` 返回 false（与 WitherEntity 一致，避免触发自然生成限制）| 项目沿用 |
| `preventDespawn()` 返回 true（永不自然消失）| MC 1.21.11 `Warden.removeWhenFarAway` |
| `isDespawnPeaceful()` 返回 true（和平难度消失）| `MonsterEntity` 默认 |
| AI: SwimGoal / MeleeAttackGoal / WaterAvoidingRandomWalkingGoal / LookAtGoal / LookRandomlyGoal | 基础怪物行为 |
| AI: HurtByTargetGoal / NearestAttackableTargetGoal\<Player\> | 基础敌对行为 |
| 经验值 5 | MC 1.21.11 `Warden.xpReward = 5` |
| 不在阳光下燃烧 | `setBurnsInDaylight(false)` |
| **WardenAngerLevel 枚举（Calmed/Agitated/Angry）** | MC 1.21.11 `AngerLevel` |
| **简化怒气系统（m_anger 聚合值 + increaseAnger/clearAnger）** | MC 1.21.11 `AngerManagement` 简化版 |
| **CLIENT_ANGER_LEVEL 数据参数同步** | MC 1.21.11 `Warden.CLIENT_ANGER_LEVEL` |
| **怒气等级驱动的环境音效切换** | MC 1.21.11 `Warden.getAmbientSound` |
| **怒气每 20 tick 衰减** | MC 1.21.11 `Warden.customServerAiStep` 简化 |
| **完整 WARDEN_* SoundEvents（21 个）** | MC 1.21.11 `SoundEvents` |

### 未实现（已留有显式 TODO 注释）

| 子系统 | 阻塞原因 |
|--------|----------|
| `VibrationSystem`（振动感知）| 需先实现 `game_event` 总线与 `VibrationSystem.User/Data/Listener` |
| `AngerManagement`（按目标怒气管理）| 需 Brain 系统与 `MemoryModuleType`；当前使用简化版单一聚合怒气代替 |
| `SonicBoom`（音爆远程攻击）| 需远程攻击目标 + 音爆粒子/伤害实现 |
| `Pose::EMERGING` / `Pose::DIGGING`（钻地动画与姿态免疫）| 需 Pose 系统扩展 |
| `Roar` / `Sniff` 动画 | 需 AnimationState 系统 |
| `Darkness` 周期性效果（每 6 秒给附近玩家施加黑暗）| 需 `MobEffectUtil.addEffectToPlayersAround` |
| 心跳音效（根据怒气调整频率）| 需心跳动画系统 |
| 触碰怒气（实体触碰监守者增加怒气，20 tick 冷却）| 需 Brain 记忆模块 |
| `WardenAi` 完整行为系统 | 需 Brain + Behavior 系统全套基建 |
| 怒气触发的 `listeningSound` 播放 | 需 VibrationSystem 接入后由 increaseAngerAt 调用 |

### 召唤入口

`SculkShriekerHelper::_trySummonWarden(ServerWorld&, const BlockPos&)` 是监守者的唯一召唤入口，
对应 MC 1.21.11 `SculkShriekerBlockEntity.trySummonWarden`。召唤前置条件：

1. `SculkShriekerBlockEntity.canSummonWarden()` 返回 true（警告等级 ≥ 4 且冷却结束）
2. 附近 48 格内无其他监守者
3. 找到有效的生成位置（下方方块有完整上表面，生成位和上方为空气）

召唤成功后，监守者位置设为 `(checkPos.x + 0.5, checkPos.y, checkPos.z + 0.5)`，
旋转角为随机方向（0–360°），跳过生成规则检查（监守者为特殊召唤）。

### 16. 监守者怒气等级（WardenAngerLevel）

监守者根据当前怒气值（`m_anger`）分为三个等级，对应不同的环境音效与倾听音效：

| 等级 | 怒气范围 | 环境音效 | 倾听音效 |
|------|----------|----------|----------|
| Calmed | [0, 40) | `ENTITY_WARDEN_AMBIENT` | `ENTITY_WARDEN_LISTENING` |
| Agitated | [40, 80) | `ENTITY_WARDEN_AGITATED` | `ENTITY_WARDEN_LISTENING_ANGRY` |
| Angry | [80, 150] | `ENTITY_WARDEN_ANGRY` | `ENTITY_WARDEN_LISTENING_ANGRY` |

**实现要点**：

- `WardenAngerLevel` 是 `enum class : u8`，定义于 `WardenAngerLevel.hpp`，
  与 MC 1.21.11 `net.minecraft.world.entity.monster.warden.AngerLevel` 对齐。
- 工具函数 `wardenAngerLevelByAnger(int)` / `wardenAngerLevelMinimumAnger(level)` /
  `wardenAngerLevelIsAngry(level)` / `wardenAngerLevelAmbientSound(level)` /
  `wardenAngerLevelListeningSound(level)` 提供等级查询与音效映射。
- `WardenEntity` 持有 `m_anger`（i32）作为服务端权威怒气值，通过
  `CLIENT_ANGER_LEVEL` 数据参数同步到客户端。
- `WardenEntity::getAngerLevel()` 返回当前等级，`getAmbientSound()` 据此返回
  对应音效（与 MC 1.21.11 `Warden.getAmbientSound` 行为一致，姿态判断待
  Pose 系统扩展后补充）。
- `WardenEntity::updateAITasks()` 每 20 tick 衰减 1 点怒气，对应 MC 1.21.11
  `Warden.customServerAiStep` 中 `angerManagement.tick` 调用。
- 怒气上限 `ANGER_LIMIT = 150`，防止无限增长。

**与 MC 原版的差异**：

MC 原版 `AngerManagement` 按**每个目标实体**分别跟踪怒气值，
`getActiveAnger(target)` 返回当前目标对应的怒气。项目当前使用**单一聚合怒气**
`m_anger` 代替，所有来源的怒气累加到同一字段。这一简化足以支撑环境音效切换、
客户端同步、心跳频率调整等表现，但无法实现"对不同目标有不同怒气等级"的语义。
完整 `AngerManagement` 实现（依赖 Brain 系统与 `MemoryModuleType`）后可平滑迁移。
