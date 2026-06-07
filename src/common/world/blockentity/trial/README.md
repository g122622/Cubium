# 试炼类方块实体 (Trial Block Entities)

试炼密室相关的方块实体实现，包括试炼刷怪笼和宝库。

## 目录结构

```
trial/
├── TrialSpawnerBlockEntity.hpp/cpp  # 试炼刷怪笼（状态机、怪物生成、奖励弹出）
├── VaultBlockEntity.hpp/cpp          # 宝库（钥匙解锁、战利品弹出、玩家追踪）
└── README.md
```

## 内部模块关系

```
BlockEntity (父模块基类)
       ↑
       ├─────────────────────┐
       │                     │
TrialSpawnerBlockEntity  VaultBlockEntity
```

两个类相互独立，没有继承关系。

## 上下游外部依赖关系

### 上游依赖（谁使用了这个模块）

- `world/block/blocks/trial/` - 试炼刷怪笼方块、宝库方块创建和访问对应的方块实体
- `world/chunk/` - 区块加载时反序列化方块实体
- `server/` - 服务器处理玩家交互时访问方块实体

### 下游依赖（这个模块依赖了谁）

- `world/blockentity/BlockEntity.hpp` - 方块实体基类
- `world/blockentity/BlockEntityType.hpp` - 方块实体类型枚举
- `entity/Player.hpp` - 玩家实体（检测、追踪）
- `entity/LivingEntity.hpp` - 生物实体（怪物追踪）
- `entity/loot/LootTableManager.hpp` - 战利品表管理器
- `resource/ResourceLocation.hpp` - 资源位置

## 容易踩的坑

### 1. TrialSpawnerBlockEntity 状态机转换

状态机有6种状态，转换条件复杂：
- Inactive → WaitingForPlayers：检测到玩家进入范围
- WaitingForPlayers → Active：玩家数量满足条件
- Active → WaitingForRewardEjection：所有怪物被击杀
- WaitingForRewardEjection → EjectingReward：准备完成
- EjectingReward → Cooldown：奖励弹出完成
- Cooldown → WaitingForPlayers：冷却结束

### 2. TrialSpawnerBlockEntity 怪物数量计算

怪物数量根据玩家数量动态计算：
- 总怪物数 = baseTotalMobs + totalMobsAddedPerPlayer * (玩家数 - 1)
- 同时怪物数 = baseSimultaneousMobs + simultaneousMobsAddedPerPlayer * (玩家数 - 1)
- 不祥变体会使用更高的加成系数

### 3. VaultBlockEntity 玩家追踪限制

`m_rewardedPlayers` 最多存储 128 个玩家 UUID，超出后不再记录新玩家。这是为了防止内存无限增长。

### 4. VaultBlockEntity 战利品弹出规则

战利品弹出有多层抽取：
1. 80% 概率从稀有表抽 1 次，20% 概率从普通表抽 1 次
2. 总是从普通表抽 1-3 次
3. 普通宝库 25% 概率从独有表抽 1 次；不祥宝库 75% 概率

### 5. 不祥变体触发条件

玩家需要持有不祥之兆效果进入试炼刷怪笼范围才能触发不祥变体。触发后消耗不祥之兆，给予试炼之兆效果。

### 6. 红石比较器输出

- TrialSpawnerBlockEntity：Inactive=0, WaitingForPlayers=1, Active=2, WaitingForRewardEjection=3, EjectingReward=4, Cooldown=4
- VaultBlockEntity：Active=0, Unlocking/Ejecting=15
