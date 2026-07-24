# 爆炸系统 (Explosion System)

## 目录结构

```text
explosion/
├── ExplosionMode.hpp           # 爆炸模式枚举定义（None / Break / Destroy）
├── ExplosionContext.hpp        # 爆炸上下文基类及实体/凋灵之首上下文
├── ExplosionContext.cpp        # ExplosionContext 实现
├── Explosion.hpp               # 爆炸核心类，执行完整爆炸流程
├── Explosion.cpp               # Explosion 实现
└── README.md                   # 本文档
```

## 内部模块关系

```text
ExplosionMode（枚举）
       │
       ▼
ExplosionContext ◄─────────── Explosion
       │                           │
       ├── EntityExplosionContext   │ 射线追踪、伤害计算
       │                           │ 方块破坏、掉落生成
       └── WitherSkullExplosionContext
           （凋灵之首爆炸上下文）
```

- **ExplosionMode**：定义爆炸对方块的影响方式，被 Explosion 使用
- **ExplosionContext**：抽象基类，允许自定义爆炸行为（如凋灵之首穿透高抗性方块）
- **EntityExplosionContext**：实体爆炸上下文，基于爆炸源实体提供默认行为
- **WitherSkullExplosionContext**：凋灵之首爆炸上下文，蓝色凋灵之首可以穿透高抗性方块（黑曜石等），但不能破坏 WITHER_IMMUNE 方块（基岩等）
- **Explosion**：核心类，协调整个爆炸流程，持有 ExplosionContext

## 上下游外部依赖关系

### 上游依赖（本模块依赖）

| 模块 | 用途 |
|------|------|
| `common/world/IWorld` | 世界接口，获取方块、实体、流体等 |
| `common/world/block/Block` | 方块定义，爆炸抗性、掉落表等 |
| `common/world/block/BlockTags` | 方块标签系统（WITHER_IMMUNE 等） |
| `common/world/fluid/Fluid` | 流体状态，影响爆炸抗性计算 |
| `common/entity/core/Entity` | 实体基类，伤害、击退、免疫检测 |
| `common/entity/damage/DamageSource` | 伤害来源，爆炸伤害类型 |
| `common/item/loot/LootTableManager` | 掉落表管理，方块掉落生成 |
| `common/util/math/random/Random` | 随机数生成 |
| `common/util/math/ray/Raycast` | 射线追踪，视线检测 |
| `common/entity/utils/ItemDropHelper` | 物品掉落工具类 |
| `common/item/enchantment/EnchantmentHelper` | 附魔检测，爆炸保护附魔 |
| `client/renderer/trident/particle/ParticleTypes` | 粒子效果 |
| `common/sound/SoundCategory` | 音效播放 |

### 下游依赖（被依赖）

| 调用方 | 用途 |
|--------|------|
| `ServerWorld` | 通过 `createExplosion()`/`createExplosionWithContext()` 创建爆炸 |
| `TNTBlock` | TNT 方块被点燃/爆炸时创建爆炸 |
| `BedBlock` | 床在其他维度使用时创建爆炸 |
| `RespawnAnchorBlock` | 重生锚在主世界使用时创建爆炸 |
| `CreeperEntity` | 苦力怕爆炸 |
| `WitherEntity` | 凋灵召唤/攻击时爆炸 |
| `AbstractFireballEntity` | 恶魂火球/凋灵之首爆炸（蓝色凋灵之首使用 WitherSkullExplosionContext） |
| `MinecartEntity`（TNT 矿车） | TNT 矿车爆炸 |
| `WindChargeEntity` | 风弹命中后调用 `applyWindBurst`，自行实现爆炸击退逻辑并通过 `IWorld::broadcastExplosion` 同步给客户端（不使用 `Explosion` 类，因为风弹不破坏方块） |

### 爆炸事件广播接口

`IWorld::broadcastExplosion(position, strength, affectedBlocks, playerKnockback)` 是 common 层访问爆炸同步网络的统一入口：

- 默认空实现，`WorldGenRegion` 等非服务端实现继承空实现
- `ServerWorld` 重写后委托给 `m_onBroadcastExplosion` 回调（由 `MinecraftServer::attachWorldBindings` 注册），最终调用 `MinecraftServer::broadcastExplosionInRange` 在 64 格范围内逐个发送 `Explosion IR`
- `Explosion` 类在 `explode()` 完成后由 `ServerWorld::createExplosion*` 系列方法调用此接口；`WindChargeEntity` 因不破坏方块而独立调用

对应 MC Java 的 `ServerLevel.explode()`：爆炸完成后遍历 64 格（`distanceToSqr < 4096.0`）内玩家发送 `ClientboundExplodePacket`，每个玩家收到的是属于自己的 `Optional<Vec3>` 击退向量（来自 `ServerExplosion.hitPlayers` 映射），客户端 `handleExplosion` 调用 `player.addDeltaMovement(vec)` 累加到现有速度上。

## 容易踩的坑

### #1. LootTableManager 为空时不掉落物品

**问题**：`Explosion` 构造时如果 `lootTableManager` 为 `nullptr`，`Destroy` 模式下不会生成任何掉落物。

**解决**：`ServerWorld::createExplosion()` 会自动传入 `LootTableManager`；直接构造 `Explosion` 时需显式传入。

### #2. 爆炸保护附魔计算

**问题**：爆炸保护附魔的 EPF（爆炸保护系数）有上限 20，减伤公式为 `damage × (1 - min(EPF, 20) / 25)`。

**解决**：代码中已正确处理，但自定义实体伤害逻辑时需注意此上限。

### #3. 流体爆炸抗性

**问题**：水和岩浆的爆炸抗性为 100.0，会消耗大量爆炸强度。

**解决**：`ExplosionContext::getExplosionResistance()` 已处理流体情况，取方块和流体抗性的较大值。

### #4. 爆炸衰减公式

**问题**：物品存活概率为 `1 - 1 / explosionRadius`，半径为 1 时物品 100% 消失。

**解决**：这是 MC 1.16.5 的正确行为，恶魂火球（半径 1）爆炸不掉落物品。

### #5. 射线追踪使用随机种子

**问题**：相同位置的爆炸如果使用相同种子，会产生相同的破坏模式。

**解决**：`Explosion` 使用位置坐标作为随机种子，保证相同位置的爆炸结果一致。

### #6. 玩家击退与游戏模式

**问题**：观察者模式玩家不受击退，创造模式飞行中也不受击退。

**解决**：代码中已通过 `GameModeUtils::isSpectator()` 和 `abilities.flying` 检测。

### #7. 爆炸模式差异

| 模式 | 破坏方块 | 生成掉落 | 用例 |
|------|---------|---------|------|
| None | ❌ | ❌ | mobGriefing = false 时的苦力怕 |
| Break | ✅ | ❌ | TNT |
| Destroy | ✅ | ✅ | 苦力怕、末地水晶 |

### #8. 爆炸路径中的 spawnAfterBreak

**要点**：`Explosion::_destroyBlocks` 在方块被移除后调用 `block.spawnAfterBreak(world, pos, state, nullptr, false)`。
- `tool` 参数为 `nullptr`（爆炸无工具），`dropExp` 为 `false`（爆炸不产生经验）
- 这意味着虫蚀方块（InfestedBlock）在爆炸中不会因为精准采集而不生成蠹虫（因为 tool = nullptr）
- 但 `doTileDrops` 游戏规则仍然生效：`doTileDrops = false` 时不生成蠹虫
- 调用顺序：`onBlockExploded` → `setBlockState(air)` → `spawnAfterBreak`，与 MC Java 一致
- `onBlockExploded` 签名包含 `const Explosion* explosion` 参数，允许方块在爆炸回调中访问爆炸信息（如间接源实体）

### #9. 蓝色凋灵之首的爆炸抗性穿透

**要点**：蓝色凋灵之首（dangerous skull）使用 `WitherSkullExplosionContext`，将不在 `WITHER_IMMUNE` 标签中的非空方块爆炸抗性限制为 `min(0.8, 原始抗性)`。
- 这使得蓝色凋灵之首可以破坏黑曜石（原始抗性 1200 → 限制为 0.8）、铁块等高抗性方块
- 但基岩（`WITHER_IMMUNE` 标签）仍不可破坏，保持原始高抗性
- 普通凋灵之首使用基类 `EntityExplosionContext`，不修改任何爆炸抗性
- 对应 MC Java 的 `WitherSkull.getBlockExplosionResistance()` 和 `WitherBoss.canDestroy()`

### #10. 玩家击退的双重应用防范

**背景**：`Explosion::_calculateAffectedEntities` 与 `WindChargeEntity::applyWindBurst` 都通过 `IWorld::broadcastExplosion` 把玩家击退向量以 `Explosion IR` 形式发给客户端。`Explosion IR` 在客户端通过 `addVelocity`（累加）应用击退，对应 MC Java `ClientPacketListener.handleExplosion` 调用 `player.addDeltaMovement(vec)`。

**关键约束**：玩家分支**不能**在服务端调用 `addVelocity` 修改玩家速度，否则：

1. 服务端 `addVelocity` 修改玩家速度 → `LivingEntity::hurt` 已设置 `hurtMarked` → `EntityTracker::tick` 通过 `EntityVelocityPacket`（"AndSelf" 模式）把更新后的速度同步给客户端 → 客户端 `setVelocity` 覆盖本地速度
2. `Explosion IR` 随后到达 → 客户端 `addVelocity` 累加击退 → 双重应用（先被覆盖，再累加，最终速度 = 服务端速度 + 击退，而非 客户端原速度 + 击退）

**修复方案**（已实施）：玩家分支采用「客户端权威速度」模型，与 MC Java `ServerPlayer` 速度由客户端发包同步回来的设计一致：

1. 玩家分支调用 `hurt()` 造成伤害（`hurt` 内部会设置 `hurtMarked`）
2. 立即调用 `clearHurtMarked()` 清除标记，阻止 `EntityTracker` 发送 `EntityVelocityPacket`
3. **不**调用 `addVelocity`（服务端玩家速度保持不变，等待客户端通过 `ServerboundMovePlayerPacket` 同步回来）
4. 把击退向量写入 `playerKnockback` 映射，通过 `Explosion IR` 发送给客户端
5. 客户端 `onExplosion` 回调调用 `addVelocity` 累加击退到本地玩家速度

**非玩家实体**（生物、掉落物等）仍由服务端权威同步速度：调用 `addVelocity` 修改服务端速度，依赖 `LivingEntity::hurt` 设置的 `hurtMarked` 通过 `EntityVelocityPacket` 同步给追踪此实体的客户端。

**EPF 一致性**：玩家分支与非玩家生物分支都使用 EPF 衰减后的 `knockback` 值（`impact * (1 - EPF * 0.15)`），保证 `Explosion IR` 中的击退向量与服务端（若有）应用的一致。对应 MC Java `ServerExplosion.hurtEntities` 第 197 行：`Vec3 vec32 = vec31.scale(d2)` 计算一次，`entity.push(vec32)` 与 `hitPlayers.put(player, vec32)` 共用同一向量。
