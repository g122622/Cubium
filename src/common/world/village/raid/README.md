# 袭击系统 (Raid System)

本目录实现村庄袭击事件系统，包括掠夺者生成、波次管理和英雄追踪。

## 目录结构

```
raid/
├── RaiderType.hpp/cpp    # 掠夺者类型枚举和工具函数
├── Raid.hpp/cpp          # 单次袭击事件管理（波次、生成、胜负判定）
├── RaidManager.hpp/cpp   # 世界级袭击管理器（创建、tick、查询）
└── README.md             # 本文档
```

## 内部模块关系

```
RaidManager
    └── Raid（多个实例）
           ├── RaiderType（掠夺者类型枚举）
           ├── RaidWave（波次运行时数据，定义在Raid.hpp中）
           └── RaidParticipant（参与者贡献记录）
```

`RaidManager` 负责世界级别的袭击生命周期管理，每个 `Raid` 实例管理单次袭击的波次推进和袭击者追踪。**RaidWave 是 Raid.hpp 内定义的结构体，不是独立文件。**

## RaidManager 查询方法

- `getRaidAt(BlockPos)` — 返回指定位置附近的进行中袭击（按距离匹配，仅 Ongoing 状态）
- `getRaidForVillage(Village*)` — 返回指定村庄关联的袭击（含所有状态，用于防重复检查）
- `getOngoingRaidForVillage(Village*)` — 返回指定村庄的进行中袭击（仅 Ongoing 状态，用于 Village 袭击状态验证）
- `hasRaidAt(BlockPos)` — 判断指定位置是否在进行中袭击范围内

## 上下游外部依赖关系

**上游依赖（本目录依赖）：**
- `IWorld` - 世界接口（实体生成、tick、难度查询）
- `Village` / `VillageManager` - 村庄系统和村庄查询
- `AbstractRaiderEntity` 及其子类 - 掠夺者实体（`PillagerEntity`、`VindicatorEntity`、`EvokerEntity`、`RavagerEntity`、`WitchEntity`）
- `Player` - 玩家实体（不祥之兆检测）
- `DifficultyHelper` - 难度相关计算（波次数、是否允许生成）
- `math::Random` - 随机数生成

**下游依赖（被依赖）：**
- `ServerWorld` - 持有 `RaidManager` 实例，每 tick 调用
- `StandaloneServer` / `IntegratedServer` - 设置 `RaidCallbacks` 回调（号角声、英雄效果）

## 容易踩的坑

### 村庄指针可能为空
`Raid` 构造时传入的 `village` 指针可能为 `nullptr`，调用方必须在关键操作前检查 `isValid()` 或在实现中做防御性判断。当村庄被销毁时，关联的袭击会自动失效。

### 袭击者实体追踪不保证实体存活
`Raid::raiders()` 返回的 `EntityId` 列表只表示追踪 ID，不保证实体仍存在于世界中。使用前需通过 `IWorld::getEntity()` 验证。

### 英雄追踪与贡献值分离
`addHero()` 会同时添加到 `m_heroes` 集合和 `m_participants` 列表，但 `addContribution()` 不会自动添加英雄——必须先调用 `addHero()`。

### 波次间隔使用 tick 而非秒
`RaidConfig::WAVE_INTERVAL = 1200` 表示 1200 tick（约 60 秒），不是毫秒或秒。波间冷却由 `RAID_COOLDOWN_TICKS = 300`（约 15 秒）单独控制，对应 Java 版 `raidCooldownTicks`。

### Boss 栏进度遵循三段式语义
`Raid::getBossBarProgress()` 返回的是 `_updateBossBar()` 在最近一次 `tick()` 中缓存的值，外部不应直接重算。进度逻辑对齐 Java 版 1.21.11：
- 战斗中：`getHealthOfLivingRaiders() / m_totalHealth`，`m_totalHealth` 在 `spawnRaiders()` 中累加、波次切换时清零。
- 波间冷却：`(300 - raidCooldownTicks) / 300`，倒计时在 `onRaiderDeath()` 中清空波次时启动。
- 庆祝/结束：状态非 Ongoing 时返回 0。

### Boss 栏网络同步尚未接入
当前 `getBossBarProgress()` / `getBossBarTitle()` 是公共 API，但尚未被任何下游模块（命令系统、BossInfo 网络包、调试接口）消费。`_updateBossBar()` 仅写入本地 `m_cachedProgress`，未调用 `ServerBossInfo::setProgress()` 推送客户端。原因：Raid 位于 `src/common`，而 `ServerBossInfo` 位于 `src/server`，存在层级依赖约束；且客户端 `ir::play::BossEvent` 处理路径尚未实现。完整集成需要 `RaidManager` 持有 `ServerBossInfo` 引用、Raid 层通过回调转发进度、以及客户端 `BossEvent` 处理器落地。相关位置见 `Raid.hpp` 中 `TODO(bossbar-network-sync)` 注释。

### 难度和平滑影响袭击行为
- 和平难度下袭击会直接 `stop()`（`DifficultyHelper::allowsMobSpawning` 返回 false）
- 波次数由 `DifficultyHelper::getRaidWaves()` 决定：简单 3 波、普通 5 波、困难 7 波

### 不祥之兆等级影响总波次
总波次 = 基础波次 + `max(0, badOmenLevel - 1)`。等级 1 不增加波次，等级 2+ 才会增加。

### 回调在袭击 tick 内触发
`onRaidStarted`、`onRaidVictory`、`onRaidLoss` 等回调在 `RaidManager::tick()` 内部触发，回调内不应执行耗时操作或修改袭击状态。
