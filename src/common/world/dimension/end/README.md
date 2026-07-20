# 末地维度战斗系统

本目录包含末地维度特有的战斗管理逻辑。

## 文件

| 文件 | 说明 |
|------|------|
| `EndDragonFight.hpp/cpp` | 末影龙战斗管理器，协调击杀奖励（龙蛋放置、折跃门生成、经验掉落区分）、旧存档状态扫描、末影龙存活检测、Boss 栏同步及水晶重生序列 |
| `DragonRespawnAnimation.hpp/cpp` | 末影龙重生动画的 5 阶段枚举与各阶段 tick 函数（`dragon_respawn::` 命名空间） |
| `IDragonBossBar.hpp` | 末影龙 Boss 栏抽象接口，解耦 common 与 server 层；含默认空实现 `NullDragonBossBar` |

## EndDragonFight

末影龙战斗管理器，协调末影龙击杀奖励、旧存档状态检测、末影龙存活追踪和 Boss 栏同步。

### 职责

1. **追踪战斗状态**: 记录龙是否已被击杀（`previouslyKilled` 标志）
2. **末影龙存活检测**: 在状态扫描时通过 `IWorld::getEntitiesByType()` 检测末影龙是否存活，并追踪其 UUID
3. **末影龙失联重生**: 当龙 UUID 为空或长时间未被 `updateDragon()` 调用时，通过 `_findOrCreateDragon()` 重新查找或生成末影龙
4. **首次击杀放置龙蛋**: 仅首次击杀末影龙时在祭坛顶部放置龙蛋
5. **折跃门生成**: 每次击杀末影龙生成一个末地折跃门，最多 20 个
6. **经验掉落区分**: 首次击杀 12000 XP，后续击杀 500 XP
7. **出口传送门创建**: 龙死亡后创建激活态出口传送门
8. **旧存档状态扫描**: 首次加载旧世界时检测出口传送门是否存在以推断 `previouslyKilled`
9. **Boss 栏同步**: 通过 `IDragonBossBar` 接口同步龙血量百分比、自定义名称和可见性；服务端通过 `setDragonBossBar()` 注入真实实现

### Boss 栏同步

EndDragonFight 持有一个 `IDragonBossBar` 实例（默认 `NullDragonBossBar`），对应 MC 1.21.11 `EndDragonFight.dragonEvent`（`ServerBossEvent`）。

#### 生命周期

- **构造时**: 创建 `NullDragonBossBar` 作为默认值
- **服务端初始化时**: 通过 `setDragonBossBar()` 注入 `ServerDragonBossBar`（见 `src/server/bossbar/`）
- **每 tick**: 调用 `setVisible(!dragonKilled)` 更新可见性
- **每 20 tick**: 扫描 192 格半径内玩家，通过 `replacePlayers()` 增量更新可见玩家列表
- **updateDragon()**: 由末影龙每 tick 调用，同步血量百分比和自定义名称
- **setDragonKilled()**: 设置百分比为 0、隐藏 Boss 栏

#### 接口设计

`IDragonBossBar` 定义在 common 层，使 `EndDragonFight`（common）不依赖 `ServerBossInfo`（server）。关键方法：

| 方法 | 对应 MC Java | 说明 |
|------|-------------|------|
| `setPercent(f32)` | `ServerBossEvent.setProgress` | 同步血量百分比 |
| `setName(unique_ptr<ITextComponent>)` | `ServerBossEvent.setName` | 同步显示名称 |
| `setVisible(bool)` | `ServerBossEvent.setVisible` | 同步可见性 |
| `addPlayer(PlayerId)` | `ServerBossEvent.addPlayer` | 添加可见玩家 |
| `removePlayer(PlayerId)` | `ServerBossEvent.removePlayer` | 移除可见玩家 |
| `replacePlayers(set<PlayerId>)` | EndDragonFight.updatePlayers 差集逻辑 | 一次性替换玩家列表，内部计算差集避免闪烁 |
| `hasPlayers()` | `getPlayers().isEmpty()` | 用于 tick 中判断是否跳过重逻辑 |
| `getPlayers()` | `ServerBossEvent.getPlayers()` | 返回当前可见玩家集合，用于 `setRespawnStage(END)` 遍历触发 `SUMMONED_ENTITY` 进度 |

#### 玩家追踪

`_updatePlayers()` 对应 MC Java `EndDragonFight.updatePlayers()`：

- 追踪中心：`(0, 128, 0)`（末地原点上方 128 格）
- 追踪半径：`PLAYER_TRACKING_RADIUS = 192.0f`
- 扫描间隔：`TIME_BETWEEN_PLAYER_SCANS = 20` tick
- 使用 `replacePlayers()` 一次性更新，避免 `removeAllPlayers()` + 逐个 `addPlayer()` 导致的客户端闪烁

### 旧存档状态扫描

当 `needsStateScanning = true` 时，`tick()` 方法在每个游戏 tick 检查竞技场区块是否加载。一旦加载完成，执行状态扫描：

- **检测活跃出口传送门**: 扫描原点周围 17×17 区块范围内的 END_PORTAL 方块。如果找到，说明龙曾被击杀过，设置 `previouslyKilled = true`
- **创建非激活讲台**: 如果未找到活跃出口传送门且不存在讲台结构，创建非激活讲台（不含传送门方块）
- **检测末影龙存活**: 通过 `IWorld::getEntitiesByType(EntityTypeKeys::ENDER_DRAGON)` 查询末影龙实体：
  - 无末影龙 → `dragonKilled = true`
  - 有末影龙 → 记录其 UUID，`dragonKilled = false`；若无传送门则 `discard()` 该龙（无效状态）
  - 安全修正：若 `!previouslyKilled && dragonKilled`，强制 `dragonKilled = false`
- **扫描完成后**: 将 `needsStateScanning` 设为 `false` 并持久化到存档

新世界（无存档数据）默认 `needsStateScanning = false`，因为新世界不存在旧状态需要扫描。

### 数据持久化

`EndDragonFight::Data` 支持序列化/反序列化，用于存档保存：

- `needsStateScanning` - 是否需要扫描旧世界状态
- `previouslyKilled` - 是否曾经击杀过龙
- `dragonKilled` - 龙当前是否已死
- `dragonUUID` - 末影龙的 UUID（可选，nullopt 或空字符串表示无龙或未追踪）
- `gateways` - 剩余折跃门索引列表

对应 MC Java 存档格式中 `Dragon`（UUID）和 `Gateways` 字段。

### 折跃门位置算法

20 个折跃门均匀分布在以原点为中心、半径 96 格的圆上，Y=75：
- 角度 = 2 * (-PI + (PI/20) * i)，i = 0..19
- X = floor(96 * cos(angle))
- Z = floor(96 * sin(angle))
- 初始顺序用世界种子随机打乱，每次击杀消耗一个

### updateDragon 调用链

末影龙每 tick 调用 `EndDragonFight::updateDragon()`：

1. `EnderDragonEntity::tick()` → `BossEntity::tick()` → ... → `EndDragonFight::updateDragon(*this)`
2. `updateDragon` 检查龙 UUID 与 `m_dragonUUID` 是否匹配
3. 匹配时：同步血量百分比、重置 `ticksSinceDragonSeen`、同步自定义名称
4. 不匹配时：忽略（防止错误龙实体污染 Boss 栏）

对应 MC Java: `EnderDragonEntity.aiStep()` 中调用 `dragonFight.updateDragon(this)`。

### 末影龙失联重生

`tick()` 中每游戏 tick 检查末影龙失联状态。当 `!dragonKilled` 且满足以下任一条件时，调用 `_findOrCreateDragon()`：

- `m_dragonUUID` 为空（从未追踪或已被清除）
- `++m_ticksSinceDragonSeen >= MAX_TICKS_BEFORE_DRAGON_RESPAWN`（1200 tick，约 60 秒未通过 `updateDragon()` 看到龙）

且竞技场区块已加载（`_isArenaLoaded()` 返回 true）时触发。

#### _findOrCreateDragon

对应 MC Java `EndDragonFight.findOrCreateDragon()`：

1. 通过 `IWorld::getEntitiesByType(EntityTypeKeys::ENDER_DRAGON)` 查找已存在的末影龙
2. 过滤已移除（`isRemoved()`）的实体（防御性）
3. 若存在存活龙：记录其 UUID 到 `m_dragonUUID`，设置 `dragonKilled = false`，不重复生成
4. 若不存在：调用 `_createNewDragon()` 创建新龙

#### _createNewDragon

对应 MC Java `EndDragonFight.createNewDragon()`：

1. 从 `EntityRegistry` 获取 `ENDER_DRAGON` 的 `EntityType`
2. 通过工厂方法 `EntityType::create()` 创建实例（`Entity` 构造时自动生成随机 UUID）
3. 设置生成位置 `(0, DRAGON_SPAWN_Y, 0)`（DRAGON_SPAWN_Y=128，讲台正上方）
4. 设置随机 yaw（`world.getRandom().nextFloat() * 360`），pitch=0
5. 设置初始阶段为 `HoldingPattern`
6. 调用 `IWorld::spawnEntity()` 加入世界（分配 ID、注册追踪器）
7. 记录新龙 UUID 到 `m_dragonUUID`，重置 `dragonKilled = false` 和 `ticksSinceDragonSeen = 0`
8. 返回新龙的 `Entity*` 指针（对齐 MC Java 返回 `@Nullable EnderDragon`），供 `setRespawnStage(END)` 触发 `SUMMONED_ENTITY` 进度

#### 与 MC 原版的差异

- MC 原版在 `createNewDragon()` 中调用 `level.getChunkAt()` 强制加载生成位置区块；Cubium 的调用方（`tick()`）已通过 `_isArenaLoaded()` 保证区块加载，故不再重复
- MC 原版通过 `PhaseManager` 设置阶段；Cubium 的 `EnderDragonEntity` 直接提供 `setPhase()` 方法
- MC 原版使用 `List.get(0)` 取第一条龙；Cubium 同样取过滤后列表的首元素

### 末影水晶重生序列

玩家通过在出口传送门四周放置 4 个末影水晶触发的龙重生动画序列。对应 MC 1.21.11 `EndDragonFight.tryRespawn()` / `respawnDragon()` / `DragonRespawnAnimation`。

#### 触发条件（tryRespawn）

`tryRespawn()` 仅在龙已死（`dragonKilled = true`）且未在重生中时执行：

1. 确定出口传送门位置（`portalLocation`，默认 `(0, 0, 0)`）
2. 检查传送门上方一格（`portalLoc.up(1)`）四个水平方向各 2 格外是否有末影水晶
3. 四个方向全部有水晶时，收集 4 个水晶到列表，调用 `_respawnDragon()`
4. 任一方向缺水晶则直接返回（不启动重生）

#### 启动重生（_respawnDragon）

1. 清除出口传送门区域（`(-4..4) × 全高度 × (-4..4)`）的基岩/末地传送门方块，替换为末地石
2. 设置 `respawnStage = START`，`respawnTime = 0`
3. 创建非激活态出口传送门（`EndTeleporter::createExitPortal(false)`）
4. 记录 4 个重生水晶到 `m_respawnCrystals`

#### 5 阶段状态机（DragonRespawnAnimation）

| 阶段 | 持续时间 | 主要行为 |
|------|----------|----------|
| `START` | 1 tick | 将所有重生水晶光束指向 `(0, 128, 0)`，立即切换到 `PREPARING_TO_SUMMON_PILLARS` |
| `PREPARING_TO_SUMMON_PILLARS` | 100 tick | 在 tick 0/50/51/52/95+ 播放音效事件 3001；100 tick 后切换到 `SUMMONING_PILLARS` |
| `SUMMONING_PILLARS` | 400 tick (10柱×40) | 每 40 tick 处理一根柱子：tick%40==0 切光束到柱顶，tick%40==39 移除柱区方块+爆炸+重新生成柱子+水晶；所有柱子处理完后切换到 `SUMMONING_DRAGON` |
| `SUMMONING_DRAGON` | 100 tick | tick 0 切光束到 `(0, 128, 0)`；<5 每 tick 播放 3001；>=80 每 tick 播放 3001；100 tick 时爆炸所有水晶+discard+切换到 `END` |
| `END` | 0 tick | 空操作，`setRespawnStage(END)` 会清除重生状态、创建新龙 |

每阶段对应 `dragon_respawn::` 命名空间内的一个 tick 函数（`tickStart` / `tickPreparingToSummonPillars` / `tickSummoningPillars` / `tickSummoningDragon` / `tickEnd`），由 `EndDragonFight::tick()` 在 `respawnStage` 有值时分发调用。

#### 阶段切换（setRespawnStage）

`setRespawnStage(world, stage)` 仅在重生序列进行中可调用：

- 非 `END` 阶段：设置 `respawnStage = stage`，重置 `respawnTime = 0`
- `END` 阶段：清除 `respawnStage`，设置 `dragonKilled = false`，清空重生水晶列表，调用 `_createNewDragon()` 创建新龙。新龙创建成功后，遍历 Boss 栏可见玩家集合（`IDragonBossBar::getPlayers()`），对每个玩家调用 `IWorld::onSummonedEntity(playerId, newDragon)`，由 `ServerWorld` 发布 `SummonedEntityEvent` 并经 `AdvancementEventHandler` 触发 `SummonedEntityTrigger`（对应 MC Java `CriteriaTriggers.SUMMONED_ENTITY.trigger(serverplayer, enderdragon)`）

#### 中止重生（onCrystalDestroyed）

末影水晶被破坏时（`EnderCrystalEntity::hurt()` → `EndDragonFight::onCrystalDestroyed()`）：

- 若重生序列进行中且被破坏的水晶是重生水晶：中止重生序列，重置柱顶水晶（清除无敌和光束），重新生成激活态出口传送门
- 否则：更新 `crystalsAlive` 计数，若存在末影龙则调用 `dragon->onCrystalDestroyed()` 触发龙息等反应

#### 柱顶水晶管理

- `_updateCrystalCount()`：每 `TIME_BETWEEN_CRYSTAL_SCANS`（100）tick 遍历所有柱顶碰撞箱（`EndSpike::getTopBoundingBox()`），统计末影水晶数量到 `m_crystalsAlive`
- `resetSpikeCrystals()`：遍历所有柱顶水晶，清除无敌标志和光束目标（用于重生中止或完成后）

#### 数据持久化

`EndDragonFight::Data` 新增字段：

- `isRespawning` - 是否正在重生（存档恢复时用于重启重生序列）
- `exitPortalLocation` - 出口传送门位置（序列化为 `[x, y, z]` JSON 数组）

#### 与 MC 原版的差异

- MC 使用 `enum + abstract tick()`，Cubium 使用 `enum class + 命名空间内静态 tick 函数`（避免抽象类开销，便于测试）
- MC 通过 `level.explode(null, x, y, z, 5.0F, BLOCK)` 触发爆炸，Cubium 通过 `IWorld::createExplosion(pos, 5.0f, ExplosionMode::Break, false, nullptr)`
- MC 通过 `level.removeBlock(pos, false)` 移除方块，Cubium 通过 `IWorld::setBlockState(pos, airState, 3)`
- MC 的 `SpikeFeature.place()` 在重生阶段调用 `placeSpike()` 重新生成柱子，Cubium 直接调用 `EndSpikeFeature::placeSpike(IWorld&, ...)` 运行时重载
