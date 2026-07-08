# 末地维度战斗系统

本目录包含末地维度特有的战斗管理逻辑。

## 文件

| 文件 | 说明 |
|------|------|
| `EndDragonFight.hpp/cpp` | 末影龙战斗管理器，协调击杀奖励（龙蛋放置、折跃门生成、经验掉落区分）、旧存档状态扫描、末影龙存活检测及 Boss 栏同步 |
| `IDragonBossBar.hpp` | 末影龙 Boss 栏抽象接口，解耦 common 与 server 层；含默认空实现 `NullDragonBossBar` |

## EndDragonFight

末影龙战斗管理器，协调末影龙击杀奖励、旧存档状态检测、末影龙存活追踪和 Boss 栏同步。

### 职责

1. **追踪战斗状态**: 记录龙是否已被击杀（`previouslyKilled` 标志）
2. **末影龙存活检测**: 在状态扫描时通过 `IWorld::getEntitiesByType()` 检测末影龙是否存活，并追踪其 UUID
3. **首次击杀放置龙蛋**: 仅首次击杀末影龙时在祭坛顶部放置龙蛋
4. **折跃门生成**: 每次击杀末影龙生成一个末地折跃门，最多 20 个
5. **经验掉落区分**: 首次击杀 12000 XP，后续击杀 500 XP
6. **出口传送门创建**: 龙死亡后创建激活态出口传送门
7. **旧存档状态扫描**: 首次加载旧世界时检测出口传送门是否存在以推断 `previouslyKilled`
8. **Boss 栏同步**: 通过 `IDragonBossBar` 接口同步龙血量百分比、自定义名称和可见性；服务端通过 `setDragonBossBar()` 注入真实实现

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
- **检测末影龙存活**: 通过 `IWorld::getEntitiesByType(EntityTypeIdNumber::ENDER_DRAGON)` 查询末影龙实体：
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
