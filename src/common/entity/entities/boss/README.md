# Boss实体 (Boss Entities)

本目录包含Boss级怪物的实现。

## 目录结构

```
boss/
├── EnderDragonEntity.hpp/cpp  # 末影龙 + EnderDragonPartEntity + BossEntity基类
├── WitherEntity.hpp/cpp       # 凋灵 + WitherDoNothingGoal
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
- `Entity` → `EnderDragonPartEntity`（独立的龙部件实体）

## 上下游外部依赖关系

**依赖的模块**：
- `entity/core/`：Entity、LivingEntity、MobEntity 基类，DataParameter 数据同步
- `entity/ai/goal/`：Goal 系统（WitherDoNothingGoal 等）
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

## 参考

- MC 1.16.5 `EnderDragonEntity`
- MC 1.16.5 `EnderDragonPartEntity`
- MC 1.16.5 `WitherEntity`
- MC 1.16.5 `DragonPhaseManager`
