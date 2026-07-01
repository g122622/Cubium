# 试炼类方块实体 (Trial Block Entities)

试炼密室相关的方块实体实现，包括试炼刷怪笼、宝库和自动合成器。

## 目录结构

```
trial/
├── TrialSpawnerBlockEntity.hpp/cpp  # 试炼刷怪笼（状态机、怪物生成、奖励弹出）
├── VaultBlockEntity.hpp/cpp          # 宝库（钥匙解锁、战利品弹出、玩家追踪）
├── CrafterBlockEntity.hpp/cpp        # 自动合成器（9格合成网格、槽位锁定、合成动画）
└── README.md
```

## 内部模块关系

```
BlockEntity (父模块基类)
       ↑
       ├─────────────────────┐──────────────────┐
       │                     │                  │
TrialSpawnerBlockEntity  VaultBlockEntity  CrafterBlockEntity
```

两个类相互独立，没有继承关系。

## VaultBlockEntity 宝库

### 状态机

```
INACTIVE → ACTIVE → UNLOCKING → EJECTING → ACTIVE (循环)
                     ↑                       ↓
                     └───────────────────────┘
```

| 状态 | 含义 | 红石比较器输出 |
|------|------|---------------|
| Inactive | 无人激活 | 0 |
| Active | 有玩家在范围内 | 0 |
| Unlocking | 钥匙已插入，解锁中 | 15 |
| Ejecting | 弹出奖励物品 | 15 |

### 核心逻辑

1. **玩家检测**（`detectPlayers`）：
   - 使用 `IWorld::getEntitiesInRange` 检测范围内的玩家
   - 激活范围 4.0 格，失活范围 4.5 格（迟滞设计）
   - 状态转换检测所有非旁观者玩家，不过滤已奖励玩家
   - 每 20 tick 检测一次

2. **钥匙插入**（`tryInsertKey`）：
   - 检查玩家手持物品是否匹配钥匙类型（TrialKey / OminousTrialKey）
   - 检查玩家是否已领取过奖励
   - 消耗钥匙物品（ItemStack::shrink）
   - 从战利品表生成物品列表（resolveItemsToEject）
   - 记录已奖励玩家（最多 128 人）
   - 播放音效（插入/失败/拒绝），带 15 tick 冷却防刷

3. **战利品弹出**（`ejectNextItem`）：
   - 从 LootTableManager 获取战利品表
   - 使用 LootContextBuilder 构建上下文（参数集为 chest）
   - 栈式弹出（后进先出），每 20 tick 弹出一个物品
   - 使用 ItemDropHelper::spawnItemEntity 向上弹出（速度 2.0）
   - 音高随进度渐变（0.8 + 0.4 × progress）

4. **玩家查找**（`findPlayerByUuid`）：
   - 通过遍历 `IWorld::getPlayers()` 按 UUID 匹配查找玩家

### 配置

| 配置项 | 普通宝库 | 不祥宝库 |
|--------|----------|----------|
| 钥匙物品 | TrialKey | OminousTrialKey |
| 战利品表 | chests/trial_chambers/reward | chests/trial_chambers/reward_ominous |
| 激活范围 | 4.0 | 4.0 |
| 失活范围 | 4.5 | 4.5 |

### 常量

| 常量 | 值 | 说明 |
|------|----|------|
| UNLOCKING_DURATION | 14 tick | 解锁动画持续时间 |
| EJECTION_INTERVAL | 20 tick | 每个物品弹出间隔 |
| EJECTION_AFTER_LAST_DURATION | 20 tick | 最后物品弹出后等待时间 |
| STATE_UPDATE_INTERVAL | 20 tick | 状态更新扫描间隔 |
| MAX_REWARDED_PLAYERS | 128 | 已领取奖励玩家上限 |
| INSERT_FAIL_SOUND_COOLDOWN | 15 tick | 插入失败音效冷却 |

### 持久化字段

| 字段 | 类型 | 说明 |
|------|------|------|
| state | i32 | 当前状态枚举值 |
| ominous | bool | 是否为不祥宝库 |
| rewarded_players | string[] | 已领取奖励的玩家UUID列表 |
| unlocking_start_tick | i64 | 解锁动画开始tick |
| ejection_end_tick | i64 | 当前弹出阶段结束tick |
| unlocking_player_uuid | string | 正在解锁的玩家UUID |
| last_insert_fail_sound_tick | i64 | 上次插入失败音效时间 |

## TrialSpawnerBlockEntity 试炼刷怪笼

### 状态机

```
INACTIVE → WAITING_FOR_PLAYERS → ACTIVE → WAITING_FOR_REWARD_EJECTION
                                     ↓                        ↓
                                EJECTING_REWARD ←─────────────┘
                                     ↓
                                COOLDOWN → WAITING_FOR_PLAYERS
```

| 状态 | 含义 | 红石比较器输出 |
|------|------|---------------|
| Inactive | 闲置 | 0 |
| WaitingForPlayers | 等待玩家 | 1 |
| Active | 生成怪物中 | 2 |
| WaitingForRewardEjection | 等待奖励弹出 | 3 |
| EjectingReward | 弹出奖励中 | 4 |
| Cooldown | 冷却中 | 4 |

### 核心逻辑

1. **玩家检测**（`detectPlayers`）：
   - 使用 `IWorld::getEntitiesInRange` 检测 14 格范围内玩家
   - 排除旁观者模式玩家
   - 不祥变体触发：检测持有 BadOmen 效果的玩家
   - 每 20 tick 检测一次

2. **怪物追踪**（`updateTrackedMobs`）：
   - 通过 `findEntityByUuid` 查找 UUID 对应的实体
   - 检查实体是否存活、是否在 47 格追踪范围内
   - 移除已死亡/不存在/超出范围的实体
   - 追踪变化时延迟下次生成（ticksBetweenSpawn）

3. **奖励弹出**（`ejectRewardForPlayer` / `ejectReward`）：
   - 50% 概率补给表 / 50% 概率钥匙表（不祥变体：70% / 30%）
   - 使用 LootTableManager + LootContextBuilder + LootParameterSets::chest() 生成物品
   - 通过 ItemDropHelper::spawnItemEntity 向上弹出（速度 2.0）
   - 逐玩家弹出，间隔 30 tick

4. **不祥变体**（`applyOminous`）：
   - 消耗玩家 BadOmen 效果
   - 给予 TrialOmen 效果（持续 18000 × amplifier ticks）
   - 刷怪笼转为不祥模式，怪物数量增加

5. **怪物生成**（`spawnMob`）：
   - 从 `Config::spawnPotentials` 加权随机选择实体类型
   - 在 spawnRange 范围内寻找无碰撞的生成位置（最多 20 次尝试）
   - 通过 EntityRegistry 创建实体，设置位置、旋转、持久化标记
   - 生成后追踪实体 UUID，播放试炼刷怪笼粒子效果
   - 缓存下次生成实体类型（`m_nextSpawnEntityId`）

### 配置

| 配置项 | 旋风人 | 近战 | 小型近战 | 远程 | 慢速远程 |
|--------|--------|------|----------|------|----------|
| baseTotalMobs | 2 | 6 | 12 | 6 | 6 |
| baseSimultaneousMobs | 1 | 3 | 4 | 3 | 3 |
| ticksBetweenSpawn | 20 | 40 | 20 | 40 | 80 |
| detectionRange | 14.0 | 14.0 | 14.0 | 14.0 | 14.0 |
| spawnRange | 4.0 | 4.0 | 4.0 | 4.0 | 4.0 |
| cooldownTicks | 36000 | 36000 | 36000 | 36000 | 36000 |
| spawnPotentials | breeze(1) | zombie(1), husk(1), spider(1) | silverfish(2), cave_spider(2), slime(1) | skeleton(1), stray(1), bogged(1) | skeleton(1), stray(1), bogged(1) |

### 常量

| 常量 | 值 | 说明 |
|------|----|------|
| PLAYER_SCAN_INTERVAL | 20 tick | 玩家扫描间隔 |
| DETECT_PLAYER_SPAWN_BUFFER | 40 tick | 新玩家检测后延迟生成缓冲 |
| MAX_MOB_TRACKING_DISTANCE | 47.0 格 | 怪物追踪最大距离 |
| TIME_BETWEEN_EJECTIONS | 30 tick | 奖励弹出间隔 |
| TRIAL_OMEN_PER_BAD_OMEN_LEVEL | 18000 tick | 每级不祥之兆→试炼之兆时长 |

### 持久化字段

| 字段 | 类型 | 说明 |
|------|------|------|
| state | i32 | 当前状态枚举值 |
| ominous | bool | 是否为不祥变体 |
| cooldown_ends_at | i64 | 冷却结束tick |
| ejecting_reward_ends_at | i64 | 奖励弹出结束tick |
| spawned_mobs_count | i32 | 已生成怪物总数 |
| tracked_players | string[] | 追踪的玩家UUID列表 |
| tracked_mobs | string[] | 追踪的怪物UUID列表 |
| current_mobs_count | i32 | 当前存活怪物数 |
| total_mobs_to_spawn | i32 | 需要生成的总怪物数 |
| max_simultaneous_mobs | i32 | 最大同时怪物数 |

## CrafterBlockEntity 自动合成器

### 核心逻辑

1. **红石触发**：CrafterBlock 的 `neighborChanged` 检测红石信号上升沿，调度4 tick延时后调用 `tick()` -> `_dispenseFrom()`。

2. **合成执行**（`_dispenseFrom`）：
   - 从9格物品构建 CraftingInput（禁用槽位视为空）
   - 通过 RecipeManager::findMatchingRecipe 查找匹配配方
   - 合成成功：设置 CRAFTING=true + craftingTicksRemaining=6，射出结果和剩余物品，消耗原料
   - 合成失败：播放失败音效

3. **合成动画**（`tick`）：
   - 每tick递减 craftingTicksRemaining
   - 倒计时到0时将 CRAFTING 状态重置为 false

4. **物品射出**（`_spawnItemEntity`）：
   - 在方块面朝方向偏移0.7格处生成物品实体
   - 使用高斯散射模拟MC原版发射效果

5. **槽位锁定**：每个槽位可独立启用/禁用，禁用槽位在合成时视为空。

### 时序

```
Tick 0: 红石信号上升沿 → TRIGGERED=true, 调度4 tick延时
Tick 4: tick() → _dispenseFrom() → 查配方、射出物品、消耗原料、CRAFTING=true、craftingTicksRemaining=6
Tick 5~9: tick() → craftingTicksRemaining 递减
Tick 10: tick() → craftingTicksRemaining=0 → CRAFTING=false
```

### 红石信号下降沿

红石信号消失时，TRIGGERED 和 CRAFTING 同时重置为 false，craftingTicksRemaining 清零。

### 持久化字段

| 字段 | 类型 | 说明 |
|------|------|------|
| Items | array | 9格物品 |
| disabled_slots | int[] | 禁用槽位索引列表 |
| triggered | int | 红石触发状态 (0/1) |
| crafting_ticks_remaining | int | 合成动画剩余tick |

## 上下游外部依赖关系

### 上游依赖（谁使用了这个模块）

- `world/block/blocks/trial/` - 试炼刷怪笼方块、宝库方块创建和访问对应的方块实体
- `world/chunk/` - 区块加载时反序列化方块实体
- `server/` - 服务器处理玩家交互时访问方块实体

### 下游依赖（这个模块依赖了谁）

- `world/blockentity/BlockEntity.hpp` - 方块实体基类
- `world/blockentity/BlockEntityType.hpp` - 方块实体类型枚举
- `entity/Player.hpp` - 玩家实体（检测、追踪、物品消耗）
- `entity/core/Entity.hpp` - 实体基类（UUID查询、存活检测）
- `entity/effect/EffectType.hpp` - 效果类型（BadOmen、TrialOmen）
- `entity/effect/EffectInstance.hpp` - 效果实例（工厂方法）
- `entity/utils/ItemDropHelper.hpp` - 物品弹出工具
- `item/Items.hpp` - 物品注册表（TrialKey、OminousTrialKey）
- `item/loot/LootTableManager.hpp` - 战利品表管理器
- `item/loot/context/LootContextBuilder.hpp` - 战利品上下文构建器
- `item/loot/context/LootParams.hpp` - 战利品参数（THIS_ENTITY）
- `item/loot/context/LootParameterSets.hpp` - 战利品参数集（chest）
- `resource/ResourceLocation.hpp` - 资源位置
- `sound/SoundCategory.hpp` - 音效类别
- `world/IWorld.hpp` - 世界接口（实体查询、音效、粒子、战利品表）

## 容易踩的坑

### 1. 玩家检测范围迟滞

VaultBlockEntity 使用 activationRange=4.0 和 deactivationRange=4.5 的迟滞设计。
激活时检测 4.0 格内的玩家，失活时检测 4.5 格内的玩家，防止边界频繁切换状态。

### 2. 已奖励玩家过滤位置

VaultBlockEntity 的 `detectPlayers` 不过滤已奖励玩家（影响状态转换），
已奖励过滤仅在 `tryInsertKey` 中进行（阻止重复插入钥匙）。
这保证了宝库对已奖励玩家仍然保持激活状态。

### 3. TrialSpawnerBlockEntity 怪物数量计算

怪物数量根据额外玩家数（总玩家数 - 1）动态计算：
- 总怪物数 = baseTotalMobs + totalMobsAddedPerPlayer × 额外玩家数
- 同时怪物数 = baseSimultaneousMobs + simultaneousMobsAddedPerPlayer × 额外玩家数
- 不祥变体使用更高的加成系数

### 4. VaultBlockEntity 战利品弹出顺序

使用栈式弹出（后进先出），每次从 `m_itemsToEject` 末尾取出一个物品弹出。
音高随进度渐变：0.8 + 0.4 × (1 - 剩余/总数)。

### 5. 不祥变体触发条件

玩家需要持有不祥之兆效果进入试炼刷怪笼范围才能触发不祥变体。
触发后消耗不祥之兆，给予试炼之兆效果（持续 18000 × amplifier ticks）。

### 6. 怪物生成 TODO

`spawnMob` 方法目前仅更新计数器，完整的实体生成逻辑需要：
- 从配置或数据包读取可生成的实体类型池
- 在 spawnRange 范围内寻找合适的生成位置（碰撞检测、视线检测）
- 通过 EntityRegistry 创建实体
- 设置持久化标记（不自然消失）
- 播放生成音效和粒子效果

### 7. UUID 查询性能

`findPlayerByUuid` 和 `findEntityByUuid` 通过遍历实体列表匹配 UUID。
当实体数量较多时可能存在性能问题，后续可考虑在 EntityManager 中添加 UUID 索引。

### 8. TrialSpawnerBlockEntity 的 NBT 修改权限

`onlyOpsCanSetNbt()` 返回 true，试炼刷怪笼的 NBT 数据只能由 OP 级玩家修改（MC Java 中 TrialSpawner 属于 `OP_ONLY_CUSTOM_DATA` 集合）。
